#!/usr/bin/env python3
"""Build resource/button_on.png and button_off.png from the DXi's bitmaps.

The mapping is the whole point of this script, because the port originally
got it backwards.

Every one of the twenty-two lamp buttons in ForTran - the ten harmonic
invert toggles, the formant Enable and Inv toggles, and the ten formant
selectors - is set up in ForTranPropPage.cpp the same way:

    m_HarmInv1.SetBitmaps(IDB_GREEN, NULL, IDB_RED, NULL);

SetBitmaps takes the "in" bitmap first and the "out" bitmap second, and
CButtonST chooses between them in DrawTheBitmap (BtnST.cpp:750):

    if ((m_bIsCheckBox && bIsPressed) || (!m_bIsCheckBox && ...))
        byIndex = 0;                        // the "in" bitmap
    else
        byIndex = 1;                        // the "out" bitmap

All twenty-two are BS_AUTOCHECKBOX in ForTran.rc, so m_bIsCheckBox is TRUE
and the test reduces to "is it checked". Index 0 is the "in" bitmap, which
is IDB_GREEN.

    checked / on  -> GREEN
    unchecked/off -> RED

The port had button_on.png holding the red lamp and button_off.png the
green one, so every lamp read inverted. Regenerating from the source
bitmaps, keyed by what the state MEANS rather than what colour it is, is
what stops that happening again.

    python3 tools/make-buttons.py
"""

import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(HERE)
RES = os.path.join(PROJECT, "..", "ForTran-DXI", "res")
DST = os.path.join(PROJECT, "resource")

# state -> the DXi bitmap that state draws. See the docstring above; do not
# swap these without re-reading BtnST.cpp:750.
MAPPING = {
    "button_on.png": "Green.bmp",
    "button_off.png": "Red.bmp",
}


def main():
    for name, source in sorted(MAPPING.items()):
        path = os.path.join(RES, source)
        if not os.path.exists(path):
            sys.exit("cannot find the DXi bitmap at %s" % path)

        img = Image.open(path).convert("RGBA")
        if img.size != (22, 22):
            sys.exit("expected a 22x22 lamp, got %dx%d from %s" % (img.size + (source,)))

        img.save(os.path.join(DST, name))
        w, h = img.size
        print("wrote %s from %s  centre %s" % (name, source, img.getpixel((w // 2, h // 2))[:3]))

    print("checked/on = green, unchecked/off = red - as CButtonST drew them")


if __name__ == "__main__":
    main()
