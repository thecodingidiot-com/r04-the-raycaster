#!/usr/bin/env python3
"""Regenerates src/trig.c's 256-step fixed-point sin/cos table.

The 65816 has no floating point and this project carries no runtime
trig at all -- camera_rebuild_axes() looks angle up in this table
instead of computing sin/cos every turn. The table itself is computed
once, offline, with ordinary float math; only using it on the console
is hardware-constrained, not generating it.
"""
import math

N = 256
FRAC = 4096  # FIX(1.0), see fixed.h's FIX_FRAC_BITS


def table(fn):
    values = [round(fn(2 * math.pi * i / N) * FRAC) for i in range(N)]
    rows = []
    for i in range(0, len(values), 12):
        rows.append("    " + ", ".join(str(v) for v in values[i:i + 12]) + ",")
    return "\n".join(rows)


CONTENT = """#include "trig.h"

/*
** One full circle in 256 steps, values scaled by FIX(1.0) = 4096.
** Generated once, offline, by an ordinary Python script using
** math.sin/math.cos -- nothing about computing this table is itself
** hardware-constrained, only using it at runtime is. See trig_gen.py.
*/
const t_fix g_sin_table[TRIG_STEPS] =
{{
{sin}
}};

const t_fix g_cos_table[TRIG_STEPS] =
{{
{cos}
}};
""".format(sin=table(math.sin), cos=table(math.cos))

with open("src/trig.c", "w") as f:
    f.write(CONTENT)
print("wrote src/trig.c")
