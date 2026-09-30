#!/usr/bin/env python3
# PS5 RetroArch - the Dolphin core's motion frame, checked without a console.
#
#   python3 -m unittest tests.test_dolphin_motion
#
# patches/dolphin/ps5-port.patch rotates libretro.h's sensor frame (X right, Y
# up, Z toward the player) into the Wii Remote's (X left, Y toward the player,
# Z up) at the point where the core reads the six values. A wrong sign there is
# a remote that swings the wrong way in a game, which only a console run shows;
# so the function is compiled here out of the patch's own text and checked
# against the poses and turns that matter, and for being a rotation rather than
# a reflection, which is what lets it serve the gyroscope's axes too.

from __future__ import annotations

import re
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PATCH = ROOT / 'patches/dolphin/ps5-port.patch'

HARNESS = r'''
#include <cassert>
#include <cmath>
#include <cstdio>
%s
static bool same(float a, float b) { return std::fabs(a - b) < 1e-6f; }
int main()
{
    float x, y, z;
    // A pad lying flat: libretro's Y (up) carries +1 g, and the remote's "up" is its Z.
    x = 0.0f; y = 1.0f; z = 0.0f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(x, 0.0f) && same(y, 0.0f) && same(z, 1.0f));
    // Right side up (left handle down): libretro +X; the remote's X is left, so negative.
    x = 1.0f; y = 0.0f; z = 0.0f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(x, -1.0f) && same(y, 0.0f) && same(z, 0.0f));
    // The player's edge up (front edge down): libretro +Z; the remote's Y is toward the player.
    x = 0.0f; y = 0.0f; z = 1.0f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(x, 0.0f) && same(y, 1.0f) && same(z, 0.0f));
    // Gyroscope: a flat left turn is libretro +Y; the remote's yaw left is +Z.
    x = 0.0f; y = 2.0f; z = 0.0f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(z, 2.0f) && same(x, 0.0f) && same(y, 0.0f));
    // The front edge rising is libretro +X; the remote's +X is pitch down, so negative.
    x = 0.5f; y = 0.0f; z = 0.0f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(x, -0.5f));
    // Left handle down (clockwise seen from the player's edge) is libretro +Z; the
    // remote's +Y is roll left, which is that same movement.
    x = 0.0f; y = 0.0f; z = 0.25f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(y, 0.25f) && same(x, 0.0f) && same(z, 0.0f));
    // A rotation, not a reflection: the images of X and Y cross to the image of Z.
    float ax = 1, ay = 0, az = 0, bx = 0, by = 1, bz = 0, cx = 0, cy = 0, cz = 1;
    LibretroToWiimoteFrame(ax, ay, az);
    LibretroToWiimoteFrame(bx, by, bz);
    LibretroToWiimoteFrame(cx, cy, cz);
    assert(same(ay * bz - az * by, cx) && same(az * bx - ax * bz, cy) &&
           same(ax * by - ay * bx, cz));
    // Magnitudes survive: a unit is a unit either side.
    x = 0.3f; y = -0.4f; z = 1.2f;
    LibretroToWiimoteFrame(x, y, z);
    assert(same(x * x + y * y + z * z, 0.09f + 0.16f + 1.44f));
    std::puts("Dolphin motion frame: rotation into the Wii Remote's frame PASS");
    return 0;
}
'''


def conversion_source() -> str:
    """The LibretroToWiimoteFrame function, as the patch adds it."""
    text = PATCH.read_text(encoding='utf-8')
    start = text.index('+static void LibretroToWiimoteFrame(')
    end = text.index('+}\n', start) + len('+}\n')
    lines = text[start:end].splitlines()
    assert all(line.startswith('+') for line in lines), lines
    return '\n'.join(line[1:] for line in lines) + '\n'


class DolphinMotionFrame(unittest.TestCase):
    def test_patch_calls_the_rotation_for_both_sensors(self):
        text = PATCH.read_text(encoding='utf-8')
        # Once for the accelerometer, once for the gyroscope, each under the port's guard.
        calls = re.findall(r'^\+  LibretroToWiimoteFrame\((\w+), (\w+), (\w+)\);$', text, re.M)
        self.assertEqual(calls, [('ax', 'ay', 'az'), ('gx', 'gy', 'gz')])
        self.assertEqual(text.count('+#ifdef __PROSPERO__\n+  LibretroToWiimoteFrame('), 2)

    def test_rotation_into_the_remote_frame(self):
        with tempfile.TemporaryDirectory() as td:
            source = Path(td, 'motion.cpp')
            source.write_text(HARNESS % conversion_source(), encoding='utf-8')
            binary = str(Path(td) / 'motion')
            subprocess.run(['c++', '-std=c++17', '-O2', '-Wall', '-Wextra', str(source),
                            '-o', binary], cwd=ROOT, check=True)
            subprocess.run([binary], cwd=ROOT, check=True, timeout=15)


if __name__ == '__main__':
    unittest.main()
