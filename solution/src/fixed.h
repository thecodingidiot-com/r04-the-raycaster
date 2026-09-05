#ifndef FIXED_H
# define FIXED_H

# include <stdint.h>

/*
** t_fix is int32_t, and spelling it that way is not pedantry -- it is
** the only spelling that means the same thing on both compilers this
** project is built with.
**
** 816-tcc, the 65816 cross-compiler, is a 16-bit-int compiler:
**   int  = 2 bytes    long      = 2 bytes    long long = 4 bytes
** while host gcc, which builds test.sh's checks, is a 64-bit compiler:
**   int  = 4 bytes    long      = 8 bytes    long long = 8 bytes
**
** So `long` -- the obvious-looking choice for a 32-bit fixed-point
** type -- is 16 bits on the console and 64 bits on the machine that
** tests it. Neither is 32. A host test written against `long` passes
** while proving nothing, because it is exercising a type four times
** wider than the one the cartridge will actually run. int32_t is 4
** bytes on both, so the test and the target finally agree.
**
** The assertion below is not decoration. It is a compile error, on
** whichever toolchain drifts first.
*/
typedef int32_t t_fix;

extern char g_assert_fix_is_32_bits[sizeof(t_fix) == 4 ? 1 : -1];

# define FIX_FRAC_BITS  12

/*
** Two conversions, not one, and the difference matters on this
** hardware.
**
** FIX() is for CONSTANTS -- FIX(0.66), FIX(1) -- and multiplies in
** floating point so a fractional literal survives. Every use sits in
** a constant expression the compiler folds before the ROM is built,
** so no float ever reaches the 65816, which has no way to execute one.
**
** fix_from_int() is for RUNTIME integers -- a map coordinate, a loop
** counter. It shifts, so it stays integer arithmetic, and it casts to
** t_fix BEFORE shifting: `map_x * 4096` evaluated in 816-tcc's 16-bit
** int overflows at map_x = 8, which a 13-column map reaches.
*/
# define FIX(value)         ((t_fix)((value) * 4096.0))
# define fix_from_int(i)    ((t_fix)(i) << FIX_FRAC_BITS)

# define fix_int(value)     ((int)((value) >> FIX_FRAC_BITS))

/*
** The 65816 has no MUL/DIV opcodes at all -- unlike the Mega Drive's
** 68000 (r02), which has real hardware MULU/DIVU instructions, or an
** x86 FPU (r01/r03). The SNES compensates with a memory-mapped
** multiply/divide unit (WRMPYA/WRMPYB, WRDIVL/WRDIVH/WRDVDD), but that
** unit is only 8x8->16 multiply and 16/8->16 divide -- far too narrow
** for a 32-bit fixed type on its own. 816-tcc's own runtime builds
** wider multiply and divide out of those narrower pieces for us, the
** same way SGDK's fix32Mul/fix32Div did for r02. We never call the
** hardware unit directly; we trust the compiler runtime the same way,
** and the real cost shows up as measured frame time later in this
** chapter, not as a compile error.
**
** Both intermediates stay inside 32 bits, but only just, and only
** because their callers keep their arguments small on purpose --
** see safe_inv() in raycaster.c, which exists for exactly this reason.
*/
# define fix_mul(a, b)  ((t_fix)(((t_fix)(a) * (t_fix)(b)) >> FIX_FRAC_BITS))
# define fix_div(a, b)  ((t_fix)(((t_fix)(a) << FIX_FRAC_BITS) / (t_fix)(b)))

#endif
