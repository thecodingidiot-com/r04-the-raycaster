#!/bin/bash
# r04 — The Raycaster on the SNES / test.sh
#
# Builds the real cartridge with PVSnesLib, then checks the fixed-point
# and DDA math deterministically on the HOST -- fixed.h, vec2.c,
# camera.c, trig.c, map.c, map_data.c and raycaster.c never include
# snes.h, so host gcc can compile and run them directly.
#
# That portability is what makes the first check below matter. On
# 816-tcc a `long` is 2 bytes and on host gcc it is 8; only int32_t is
# 4 on both. A host test written against the wrong type passes happily
# while proving nothing about the cartridge.
#
# Set PVSNESLIB_HOME, then run:
#
#   bash test.sh

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOLUTION="${SCRIPT_DIR}/solution"

if [[ ! -t 1 ]]; then
    C_GREEN=""; C_RED=""; C_BOLD=""; C_RESET=""
else
    C_GREEN="\033[0;32m"; C_RED="\033[0;31m"; C_BOLD="\033[1m"; C_RESET="\033[0m"
fi

pass_count=0
fail_count=0
WORK_DIR=$(mktemp -d)
cleanup() { rm -rf "$WORK_DIR"; }
trap cleanup EXIT

hr() { echo "────────────────────────────────────────────────────────────────"; }
banner() { hr; echo "  r04 — The Raycaster on the SNES / test.sh"; hr; }
pass() { printf "  ${C_GREEN}PASS${C_RESET}  %s\n" "$1"; pass_count=$((pass_count + 1)); }
fail() {
    printf "  ${C_RED}FAIL${C_RESET}  %s\n" "$1"
    [[ -n "${2:-}" ]] && echo "        $2"
    fail_count=$((fail_count + 1))
}

banner

# ── build the real cartridge ─────────────────────────────────────────────────

if [[ -z "${PVSNESLIB_HOME:-}" ]]; then
    fail "PVSNESLIB_HOME is set" "export it to the directory containing devkitsnes/ and pvsneslib/"
    exit 1
fi
pass "PVSNESLIB_HOME is set"

echo "Building the cartridge..."
build_log=$(cd "$SOLUTION" && make clean >/dev/null 2>&1; cd "$SOLUTION" && make 2>&1)
if [[ "$?" -ne 0 ]]; then
    fail "cartridge builds" "$build_log"
    exit 1
fi
pass "cartridge builds"

# PVSnesLib's own build summary prints the phrase "No warnings or
# errors found.", so grep past that line rather than matching it.
build_diags=$(echo "$build_log" | grep -iE "warning|error" | grep -viE "no warnings or errors found|COMPILATION SUMMARY")
if [[ -n "$build_diags" ]]; then
    fail "cartridge builds with no warnings" "$build_diags"
else
    pass "cartridge builds with no warnings"
fi

if [[ -f "${SOLUTION}/raycaster.sfc" ]]; then
    pass "raycaster.sfc exists"
else
    fail "raycaster.sfc exists"
fi

# ── host logic tester ────────────────────────────────────────────────────────

cat > "$WORK_DIR/test_logic.c" <<'TESTC'
#include <stdio.h>
#include "fixed.h"
#include "vec2.h"
#include "camera.h"
#include "map.h"
#include "raycaster.h"

static int  g_pass = 0;
static int  g_fail = 0;

static void check_int(char const *label, long got, long want)
{
    if (got == want) {
        printf("PASS  %s (got %ld)\n", label, got);
        g_pass++;
    } else {
        printf("FAIL  %s (got %ld, want %ld)\n", label, got, want);
        g_fail++;
    }
}

static void check_fix_near(char const *label, t_fix got, double want, double eps)
{
    double  g = got / 4096.0;

    if ((g - want) <= eps && (want - g) <= eps) {
        printf("PASS  %s (got %.4f)\n", label, g);
        g_pass++;
    } else {
        printf("FAIL  %s (got %.4f, want %.4f)\n", label, g, want);
        g_fail++;
    }
}

int main(void)
{
    t_map       map;
    t_camera    cam;
    t_hit       hit;
    int         i;
    int         col;

    /*
    ** The type check comes first because every number below is
    ** meaningless if it is wrong. 816-tcc: long is 2 bytes. Host gcc:
    ** long is 8. int32_t is 4 on both, which is the entire point.
    */
    check_int("t_fix is exactly 32 bits", (long)sizeof(t_fix), 4);
    check_int("FIX(1) is one whole unit", (long)FIX(1), 4096);
    check_int("fix_from_int survives past the 16-bit cliff",
        (long)fix_from_int(12), 49152);
    check_fix_near("fix_mul", fix_mul(FIX(2.5), FIX(4)), 10.0, 0.001);
    check_fix_near("fix_div", fix_div(FIX(10), FIX(4)), 2.5, 0.001);

    map_load(&map);
    check_int("map has ten rows", map.rows, 10);
    check_int("map has thirteen columns", map.cols, 13);
    check_int("(0,0) is a wall", map_is_wall(&map, 0, 0), 1);
    check_int("the marked start cell is open floor",
        map_is_wall(&map, fix_int(map.start_pos.x), fix_int(map.start_pos.y)), 0);
    check_int("out of bounds is a wall", map_is_wall(&map, -1, 0), 1);
    check_int("the N marker resolves to the north-facing angle",
        map.start_angle, 192);

    /*
    ** A closed 5x5 room, camera centred and facing one flat wall:
    ** every column across the field of view must report the SAME
    ** perpendicular distance. That is what "no fisheye" actually
    ** means -- a flat wall has to render flat, not bulged, whichever
    ** column a given ray crosses. The numeric proof, not a screenshot.
    */
    {
        t_map       room;
        int         y;
        int         x;
        char const  *rows[5] = {"11111", "10001", "10001", "10001", "11111"};

        room.rows = 5;
        room.cols = 5;
        y = 0;
        while (y < 5) {
            x = 0;
            while (x < 5) {
                room.grid[y][x] = rows[y][x];
                x++;
            }
            room.grid[y][5] = '\0';
            y++;
        }
        camera_init(&cam, FIX(2.5), FIX(2.5), 0);
        i = 0;
        while (i <= 4) {
            col = i * (WINDOW_W - 1) / 4;
            hit = raycaster_cast(&cam, &room, col);
            check_fix_near("flat wall: every column reports the same perp_dist",
                hit.perp_dist, 1.5, 0.02);
            i++;
        }
        check_int("centre ray hits an X-side wall", hit.side, 0);
    }

    /*
    ** Moving straight toward a wall must shorten perp_dist by exactly
    ** the distance moved -- the same "does the number move by the
    ** right amount" discipline r01/r02/r03 all used.
    */
    camera_init(&cam, FIX(2.5), FIX(1.5), 0);
    hit = raycaster_cast(&cam, &map, WINDOW_W / 2);
    check_fix_near("depth from x=2.5 east toward the far wall",
        hit.perp_dist, 9.5, 0.02);
    camera_move(&cam, FIX(4));
    hit = raycaster_cast(&cam, &map, WINDOW_W / 2);
    check_fix_near("moving forward shortens perp_dist by the distance moved",
        hit.perp_dist, 5.5, 0.02);

    /*
    ** Every delta fed to fix_mul has to stay small enough that the
    ** 32-bit product cannot wrap. safe_inv()'s ceiling is what
    ** guarantees it, so assert the guarantee rather than trusting it:
    ** a ray sweeping the whole circle must never report a distance
    ** that could only come from an overflowed intermediate.
    */
    {
        int over = 0;
        int a;

        a = 0;
        while (a < 256) {
            camera_init(&cam, FIX(2.5), FIX(2.5), (unsigned char)a);
            col = 0;
            while (col < WINDOW_W) {
                hit = raycaster_cast(&cam, &map, col);
                if (hit.perp_dist < 0 || hit.perp_dist > FIX(64))
                    over++;
                col += 16;
            }
            a += 8;
        }
        check_int("no ray over a full sweep produces an overflowed distance",
            over, 0);
    }

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail > 0);
}
TESTC

logic_build_log=$(gcc -Wall -Wextra -I "${SOLUTION}/src" -o "$WORK_DIR/test_logic" \
    "$WORK_DIR/test_logic.c" \
    "${SOLUTION}/src/vec2.c" "${SOLUTION}/src/camera.c" "${SOLUTION}/src/trig.c" \
    "${SOLUTION}/src/map.c" "${SOLUTION}/src/map_data.c" "${SOLUTION}/src/raycaster.c" 2>&1)
if [[ "$?" -ne 0 ]]; then
    fail "host logic tester builds with no snes.h anywhere" "$logic_build_log"
    exit 1
fi
pass "host logic tester builds with no snes.h anywhere"

if [[ -n "$logic_build_log" ]]; then
    fail "host logic tester builds with no warnings" "$logic_build_log"
else
    pass "host logic tester builds with no warnings"
fi

echo
echo "Running the logic tester..."
logic_out=$("$WORK_DIR/test_logic")
logic_status=$?
echo "$logic_out" | grep -E "^PASS|^FAIL" | while read -r line; do echo "  $line"; done

pass_count=$((pass_count + $(echo "$logic_out" | grep -c "^PASS")))
fail_count=$((fail_count + $(echo "$logic_out" | grep -c "^FAIL")))
[[ "$logic_status" -ne 0 ]] && fail "all logic assertions pass" "see failures above"

echo
hr
printf "  ${C_BOLD}%d passed, %d failed${C_RESET}\n" "$pass_count" "$fail_count"
hr
[[ "$fail_count" -gt 0 ]] && exit 1
exit 0
