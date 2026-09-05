# r04-the-raycaster

Companion repository for **r04 — The Raycaster on the SNES** at
[thecodingidiot.com](https://thecodingidiot.com) — the fourth chapter of
Part III, The Rendering Journey.

r03's DDA raycaster, moved onto a 3.58 MHz 65816 with no floating point,
no divide instruction, no bitmap mode, and 128×112 pixels to draw into.
The renderer is deliberately unchanged in shape. Only the arithmetic and
the output path had to be rebuilt — and the point of the chapter is what
that costs.

---

## Follow my journey

Working through r04 alongside the implementation pages? Build the
cartridge step by step, then run the tester.

```bash
git clone https://github.com/thecodingidiot-com/r04-the-raycaster.git r04-practice
cd r04-practice
export PVSNESLIB_HOME=/path/to/pvsneslib     # the directory holding devkitsnes/ and pvsneslib/
cd solution && make
bash ../test.sh
```

All tests must pass before the chapter is complete.

---

## Follow your journey

Building this independently? Here is the full project brief.

Port a working DDA raycaster to the SNES without softening the
algorithm: one ray per screen column, stepped through a grid map one
cell boundary at a time, the nearest wall hit setting that column's
height. Then measure honestly what it costs, because that measurement
is the actual subject.

Three things the hardware forces, none of which change the algorithm:

- **No floating point, and no divide instruction either.** The 65816
  has a memory-mapped multiply/divide unit, but it is 8×8→16 multiply
  and 16/8→16 divide — far too narrow for a 32-bit fixed-point type.
  Every distance becomes a 32-bit fixed-point value with 12 fractional
  bits, and the compiler's runtime builds the wide multiply and divide
  out of narrow pieces, exactly as SGDK's `fix32Mul`/`fix32Div` did on
  the Mega Drive in r02.
- **No `sin`/`cos`.** A 256-entry fixed-point table, generated offline
  by an ordinary Python script (`trig_gen.py`), and a camera angle
  that is one byte wide — so turning wraps for free on overflow with
  no explicit mask.
- **No filesystem.** A cartridge has none, so r03's `fopen`-ed map
  file becomes an array of strings compiled straight into the ROM,
  copied into a mutable RAM grid at startup.

And one thing the hardware forces that *does* change the renderer: the
SNES has no bitmap mode at all. See below.

Source is split by concern, one file per module:

| File | Contents |
| --- | --- |
| `main.c` | the game loop: read the pad, cast, draw |
| `fixed.h` | the 32-bit fixed-point type and its four operations |
| `vec2.c` / `vec2.h` | a small 2D vector type: add, subtract, scale, dot |
| `camera.c` / `camera.h` | position, one-byte angle, and the derived `forward`/`right` axes |
| `trig.c` / `trig.h` | the 256-entry sine/cosine table (`trig_gen.py` generates it) |
| `map.c` / `map_data.c` / `map.h` | the ROM-resident grid and the start-marker resolution |
| `raycaster.c` / `raycaster.h` | the DDA itself — unchanged in shape from r03 |
| `render.c` / `render.h` | the Mode 7 framebuffer, and getting it to the screen |

---

## Mode 7 as a framebuffer

The SNES draws tiles. You do not hand the PPU pixels; you hand it an
arrangement of small pictures it already knows. A raycaster produces
neither — it produces one column of arbitrary pixels at a time, and no
two frames share a tile.

Mode 7 is the one crack in that. Its tile data is 8bpp *linear* — one
plain byte per pixel, 64 bytes per tile, no bitplanes. Fill the tilemap
once with the numbers 0..255 in a fixed arrangement, never touch it
again, and those 256 tiles stop behaving like tiles and start behaving
like a framebuffer.

That framebuffer is **128×128 and can never be larger**: 256 tiles ×
64 pixels is 16384 pixels, the entire supply of distinct pixels the
mode can address. This project renders 128×112, because 128×112 doubles
to exactly 256×224 — every framebuffer pixel becomes a clean 2×2 block
on screen, where stretching a square 128×128 to the same output would
need 1.75× vertically and land rows unevenly.

Two further pieces of Mode 7 hardware do real work here:

- **The tilemap and the pixel data are byte-interleaved.** Mode 7
  keeps the tilemap in the LOW byte of each VRAM word and the tile
  pixel data in the HIGH byte of the very same word. Both uploads
  target address `0x0000` and differ only in which half of the word
  they land in — `VRAM_INCLOW` versus `VRAM_INCHIGH`.
- **The PPU will re-scatter the buffer for you, in the exact order a
  raycaster wants.** Laid out tile by tile, walking down one screen
  column means a stride of 8 bytes, breaking every 8 pixels into a
  different tile 64 bytes away — a layout that fights a column-based
  renderer on every pixel. With VMAIN's 10-bit address translation the
  PPU rewrites each incoming address as `aaaaaa BBB ccccccc` →
  `aaaaaa ccccccc BBB`, which turns each 1024-byte block of the buffer
  into an 8-wide, 128-tall vertical strip in column-major order. One
  screen column becomes **128 consecutive bytes**. The scatter still
  happens — it happens inside the PPU's address path during the DMA,
  at no cost to the 65816.

---

## Building the solution

```bash
export PVSNESLIB_HOME=/path/to/pvsneslib
cd solution
make
```

Produces `raycaster.sfc`. Run it in any SNES emulator; on a Debian box
with RetroArch:

```bash
retroarch -L /usr/lib/x86_64-linux-gnu/libretro/snes9x_libretro.so raycaster.sfc
```

Controls: Left/Right on the d-pad to turn, Up/Down to move forward and
back.

The red bar across the top of the screen is how long the previous frame
took, at four VBlanks per pixel — 15 pixels is one second. It is drawn
at four per pixel rather than one because at one per pixel this
renderer pegs the bar at full width every single frame, which is
informative exactly once.

---

## What the tester checks

**Build** — the cartridge compiles and links with zero warnings.

**A host-side logic tester** — `fixed.h`, `vec2.c`, `camera.c`,
`trig.c`, `map.c`, `map_data.c` and `raycaster.c` include no `snes.h`
anywhere, so host `gcc` compiles and runs them directly, asserting real
numbers:

- **`t_fix` is exactly 32 bits.** This check comes first because every
  number after it is meaningless if it fails. On 816-tcc a `long` is
  2 bytes; on host gcc it is 8; `int32_t` is 4 on both. A host test
  written against the wrong type passes happily while proving nothing
  about the cartridge — which is precisely the trap this project fell
  into before the check existed.
- The ROM-resident map parses to the right dimensions, and the marked
  start cell resolves to open floor facing south.
- A closed, symmetric room: casting a ray at every column across the
  full field of view at a flat wall reports the *same* perpendicular
  distance for all of them — the actual numeric proof there is no
  fisheye distortion, not an eyeballed screenshot.
- Moving the camera forward shortens the perpendicular distance to a
  wall ahead by exactly the distance moved.
- Sweeping the camera through all 256 angles and casting across the
  whole screen never produces a distance that could only have come
  from an overflowed 32-bit intermediate — asserting the guarantee
  `safe_inv()`'s ceiling exists to provide, rather than trusting it.

---

## License

MIT License. See [LICENSE](LICENSE).
