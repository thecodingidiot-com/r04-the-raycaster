#include <snes.h>
#include "render.h"
#include "raycaster.h"

/*
** ---------------------------------------------------------------
** Getting a software-rendered picture onto a SNES screen at all
** ---------------------------------------------------------------
**
** The SNES has no bitmap mode. Every background mode it offers draws
** tiles from a tilemap -- you do not hand the PPU pixels, you hand it
** an arrangement of small pictures it already knows. A raycaster
** produces neither: it produces one column of arbitrary pixels at a
** time, and no two frames share a tile.
**
** Mode 7 is the one crack in that. Its tile data is 8bpp *linear* --
** one plain byte per pixel, 64 bytes per tile, no bitplanes -- so if
** the tilemap is filled once with the numbers 0..255 in a fixed
** arrangement and never touched again, those 256 tiles stop behaving
** like tiles and start behaving like a framebuffer.
**
** That framebuffer is 128x128 pixels and it can never be larger.
** 256 tiles x 64 pixels is 16384 pixels, and that is the whole supply
** of distinct pixels Mode 7 can address. This is the wall r01's
** 640x480 and r02's 320x224 never met.
**
** The upload uses a second Mode 7 peculiarity. Mode 7 keeps the
** tilemap in the LOW byte of each VRAM word and the tile pixel data
** in the HIGH byte of the very same word -- two unrelated things
** byte-interleaved through one address space. So both DMAs below
** target address 0x0000 and differ only in which half of the word
** they land in: VRAM_INCLOW for the map, VRAM_INCHIGH for the pixels.
*/

# define FB_SIZE    16384

/*
** ---------------------------------------------------------------
** Why a screen column is 128 bytes in a row here
** ---------------------------------------------------------------
**
** Laid out the obvious way, tile by tile, a framebuffer stores pixel
** (x, y) at tile (x/8, y/8), offset (y%8)*8 + (x%8). Walking DOWN one
** screen column then means a stride of 8 bytes, breaking every 8
** pixels into a different tile 64 bytes away. A raycaster writes
** nothing but vertical columns, so that layout fights it on every
** single pixel.
**
** VMAIN's address-remapping hardware removes the fight for free. With
** 10-bit translation the PPU rewrites each incoming VRAM address as
**
**     aaaaaa BBB ccccccc  ->  aaaaaa ccccccc BBB
**
** and worked through against Mode 7's tile layout, that turns each
** 1024-byte block of our buffer into an 8-pixel-wide, 128-pixel-tall
** vertical strip in COLUMN-MAJOR order. One screen column becomes 128
** consecutive bytes, which is exactly the shape raycaster_cast()
** produces them in. The scatter still happens -- it happens in the
** PPU's address path during the DMA, at no cost to the 65816.
**
** (The library's own bgInitMapTileSet7() passes VRAM_ADRTR_0B, which
** is the *no translation* setting, because its input is a .pc7 file
** some offline tool already arranged tile-by-tile. Nothing is
** rearranging ours; we are writing it fresh, sixty times a second we
** hope, so we may as well write it in the order the hardware wants.)
*/
# define FB_COL(x)      ((((x) >> 3) << 10) + (((x) & 7) << 7))
# define TILE_AT(tx, ty) (((tx) << 4) + (ty))

/* Incremented by PVSnesLib's own VBlank handler; the only clock this
** program has, and the honest unit for the number this chapter is
** really about. 60 of these is one second. */
extern u16      snes_vblank_count;

static u8       g_fb[FB_SIZE];
static u16      g_last_frame_vblanks;

static void load_palette(void)
{
    setPaletteColor(0, RGB5(0, 0, 0));
    setPaletteColor(PAL_FLOOR, RGB5(9, 9, 11));
    setPaletteColor(PAL_CEIL, RGB5(5, 6, 9));
    setPaletteColor(PAL_WALL_NS, RGB5(26, 21, 15));
    setPaletteColor(PAL_WALL_EW, RGB5(17, 13, 9));
    setPaletteColor(PAL_METER, RGB5(31, 8, 8));
}

/*
** The identity-ish matrix a framebuffer wants, written straight to
** the registers. PVSnesLib's setMode7() is not usable here: it
** installs a rotate/scale matrix meant for a racing-game floor, and
** the result is this buffer shrunk and repeated across the screen.
**
** A and D are 8.8 fixed point, so 0x0080 is exactly 0.5 -- the plane
** advances half a pixel per screen pixel, which doubles our 128x112
** buffer into the SNES's full 256x224 output with every framebuffer
** pixel becoming a clean 2x2 block.
*/
static void mode7_screen(void)
{
    REG_BGMODE = 7;
    REG_M7SEL = 0xC0;
    REG_M7A = 0x80; REG_M7A = 0x00;
    REG_M7B = 0x00; REG_M7B = 0x00;
    REG_M7C = 0x00; REG_M7C = 0x00;
    REG_M7D = 0x80; REG_M7D = 0x00;
    REG_M7X = 0x00; REG_M7X = 0x00;
    REG_M7Y = 0x00; REG_M7Y = 0x00;
    REG_M7HOFS = 0x00; REG_M7HOFS = 0x00;
    REG_M7VOFS = 0x00; REG_M7VOFS = 0x00;
    REG_TM = 0x01;
    REG_TS = 0x00;
    REG_NMITIMEN = INT_VBLENABLE | INT_JOYPAD_ENABLE;
}

/*
** Uploaded exactly once. After this the tilemap is furniture: 256
** tiles nailed into a fixed 16x16 arrangement, so that from here on
** "tile 137" means one specific 8x8 patch of screen and nothing else.
*/
static void upload_tilemap(void)
{
    int tx;
    int ty;
    int i;

    for (i = 0; i < FB_SIZE; i++)
        g_fb[i] = 0;
    for (ty = 0; ty < 16; ty++)
        for (tx = 0; tx < 16; tx++)
            g_fb[ty * 128 + tx] = (u8)TILE_AT(tx, ty);
    dmaCopyVram7(g_fb, 0x0000, 0x4000,
        VRAM_INCLOW | VRAM_ADRTR_0B | VRAM_ADRSTINC_1, 0x1800);
    for (i = 0; i < FB_SIZE; i++)
        g_fb[i] = 0;
}

static void draw_column(int column, t_hit const *hit)
{
    t_fix   perp_dist;
    int     line_h;
    int     start_y;
    int     end_y;
    int     y;
    u8      *col;
    u8      shade;

    /*
    ** perp_dist can land arbitrarily close to zero -- the camera
    ** standing right against, or inside, a wall cell -- and dividing
    ** by it unclamped produces a line height far past what an int can
    ** hold. Floor perp_dist BEFORE the division, not the result
    ** after: clamping line_h afterwards is too late, the damage is
    ** already in the value being clamped. r03 met this exact bug on
    ** the desktop and the fix is unchanged; only the arithmetic is.
    */
    perp_dist = hit->perp_dist;
    if (perp_dist < FIX(0.125))
        perp_dist = FIX(0.125);
    line_h = fix_int(fix_div(FIX(WINDOW_H), perp_dist));
    start_y = WINDOW_H / 2 - line_h / 2;
    end_y = start_y + line_h;
    if (start_y < 0)
        start_y = 0;
    if (end_y > WINDOW_H)
        end_y = WINDOW_H;
    if (hit->side == 0)
        shade = PAL_WALL_EW;
    else
        shade = PAL_WALL_NS;
    col = g_fb + FB_COL(column);
    y = 0;
    while (y < start_y)
        col[y++] = PAL_CEIL;
    while (y < end_y)
        col[y++] = shade;
    while (y < WINDOW_H)
        col[y++] = PAL_FLOOR;
}

/*
** How long the previous frame took, drawn as a bar across the top of
** the screen at four VBlanks per pixel. A crude readout and a
** completely honest one -- and four rather than one per pixel because
** at one per pixel this renderer pegs the bar at full width every
** single frame, which is informative exactly once.
**
** 15 pixels is one second. The whole 128-pixel width is 8.5 seconds.
*/
static void draw_frame_meter(void)
{
    int x;
    int y;
    u16 len;

    len = g_last_frame_vblanks >> 2;
    if (len > WINDOW_W)
        len = WINDOW_W;
    for (x = 0; x < (int)len; x++)
        for (y = 0; y < 3; y++)
            g_fb[FB_COL(x) + y] = PAL_METER;
}

void    render_init(void)
{
    setBrightness(0);
    WaitForVBlank();
    upload_tilemap();
    load_palette();
    mode7_screen();
    g_last_frame_vblanks = 0;
    setScreenOn();
}

void    render_frame(t_camera const *cam, t_map const *map)
{
    int     column;
    u16     started;
    t_hit   hit;

    started = snes_vblank_count;
    for (column = 0; column < WINDOW_W; column++) {
        hit = raycaster_cast(cam, map, column);
        draw_column(column, &hit);
    }
    draw_frame_meter();

    /*
    ** 16KB has to reach VRAM, and a VBlank period only carries about
    ** 6.5KB of DMA before the beam comes back. The frame physically
    ** cannot be handed over inside one VBlank, so the screen is
    ** switched off, the whole buffer is pushed across with no
    ** deadline, and the screen comes back on.
    **
    ** That is the second wall in this chapter and it is independent
    ** of the first. Even if the 65816 could trace 128 rays in a
    ** sixtieth of a second, the bus could not deliver the result.
    */
    WaitForVBlank();
    setBrightness(0);
    dmaCopyVram7(g_fb, 0x0000, 0x4000,
        VRAM_INCHIGH | VRAM_ADRTR_10B | VRAM_ADRSTINC_1, 0x1900);
    setBrightness(15);
    g_last_frame_vblanks = snes_vblank_count - started;
}
