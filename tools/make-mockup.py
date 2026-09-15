#!/usr/bin/env python3
"""Draw the plug-in window as a host would show it, into docs/plugin-window.png.

There is no screenshot in this repository and no way to take one from a
headless session, so the window is COMPOSITED from the same parts the editor
loads: resource/background.png and the knob, dot, scale and lamp bitmaps, at
the positions ForTranEditor::open computes, drawn the way ForTranControls.cpp
draws them.

Anything the panel COMPUTES - the filter response, the waveform, the three
envelope traces - comes from tools/mockup-data.cpp, which is the real
FilterInfo and the real Adsr with every parameter at the table's own default.
Nothing here invents a curve.

What it shows is therefore a freshly instantiated plug-in: the DXi's own
default patch, formant 1 selected, all four filter shapes Off.

    tools/make-mockup.py            # needs the VST3 SDK for the data dumper
"""

import json
import math
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "resource")
OUT = os.path.join(ROOT, "docs", "plugin-window.png")

# -- the editor's own constants ------------------------------------------
KX, KY = 1.5, 1.625                 # MS Sans Serif 8pt dialog units
KNOB, TOGGLE, SELECTOR_H = 29.0, 22.0, 18.0
KNOB_MIN_DEG, KNOB_MAX_DEG = 30.0, 329.0
SLIDER_BOTTOM_INSET = 7.0
W, H = 1064, 408
ENV_X, ENV_W, ENV_H, ENV_TOP, ENV_GAP = 852, 200, 102, 6, 14
STRIP_ROW_TOP, STRIP_LABEL_TOP, LFO_BUTTON_TOP = 346, 377, 390
STRIP_COLS = (885, 952, 1019)

def px(d): return d * KX
def py(d): return d * KY

FONT_DIRS = ["/usr/share/fonts/truetype/liberation", "/System/Library/Fonts/Supplemental",
             "/usr/share/fonts/truetype/dejavu"]
FONT_FILES = ["LiberationSans-Regular.ttf", "Arial.ttf", "DejaVuSans.ttf"]

def font(size):
    for d in FONT_DIRS:
        for f in FONT_FILES:
            p = os.path.join(d, f)
            if os.path.exists(p):
                return ImageFont.truetype(p, size)
    return ImageFont.load_default()

F_SMALL = font(11)        # VSTGUI kNormalFontSmall
F_TINY = font(9)          # VSTGUI kNormalFontVerySmall


def panel_state():
    """Build and run tools/mockup-data.cpp; return its JSON."""
    sdk = os.environ.get("VST3_SDK_ROOT", os.path.join(ROOT, "external", "vst3sdk"))
    if not os.path.isdir(sdk):
        sys.exit("make-mockup: no VST3 SDK at %s - run ./setup-xcode.sh once, "
                 "or set VST3_SDK_ROOT." % sdk)
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "mockup-data")
        subprocess.run(
            [os.environ.get("CXX", "c++"), "-std=c++17", "-O2", "-I" + sdk,
             "-I" + os.path.join(ROOT, "source"),
             os.path.join(ROOT, "source", "ForTranParams.cpp"),
             os.path.join(ROOT, "source", "FilterInfo.cpp"),
             os.path.join(ROOT, "source", "Adsr.cpp"),
             os.path.join(ROOT, "tools", "mockup-data.cpp"), "-o", exe],
            check=True)
        return json.loads(subprocess.run([exe], check=True, capture_output=True, text=True).stdout)


class Canvas:
    """Straight-alpha drawing, so the translucent fills VSTGUI uses land the
    same way they do on screen."""

    def __init__(self, base):
        self.img = base.convert("RGBA")

    def overlay(self):
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        return layer, ImageDraw.Draw(layer)

    def merge(self, layer):
        self.img = Image.alpha_composite(self.img, layer)

    def paste(self, bmp, x, y):
        self.img.alpha_composite(bmp, (int(round(x)), int(round(y))))

    def rect(self, box, fill=None, outline=None, width=1):
        layer, d = self.overlay()
        d.rectangle(box, fill=fill, outline=outline, width=width)
        self.merge(layer)

    def line(self, pts, fill, width=1):
        layer, d = self.overlay()
        d.line(pts, fill=fill, width=width)
        self.merge(layer)

    def text(self, box, s, fnt, fill, align="center"):
        layer, d = self.overlay()
        x0, y0, x1, y1 = box
        cy = (y0 + y1) / 2.0
        if align == "center":
            d.text(((x0 + x1) / 2.0, cy), s, font=fnt, fill=fill, anchor="mm")
        elif align == "left":
            d.text((x0, cy), s, font=fnt, fill=fill, anchor="lm")
        else:
            d.text((x1, cy), s, font=fnt, fill=fill, anchor="rm")
        self.merge(layer)


def draw_knob(c, art, x, y, value):
    """FtKnob::draw - scale, knob, then the dot on a 299-degree sweep."""
    c.paste(art["scale"], x, y)
    cx = x + art["scale"].width / 2.0
    cy = y + art["scale"].height / 2.0
    kw, kh = art["knob"].width, art["knob"].height
    c.paste(art["knob"], cx - kw / 2.0, cy - kh / 2.0)
    dw, dh = art["dot"].width, art["dot"].height
    radius = kw / 2.0 - dw / 2.0 - 3.0
    pos = KNOB_MIN_DEG + value * (KNOB_MAX_DEG - KNOB_MIN_DEG)
    rad = math.radians(360.0 - pos)
    c.paste(art["dot"], cx - dw / 2.0 + radius * math.sin(rad),
            cy - dh / 2.0 + radius * math.cos(rad))


def draw_vslider(c, left, top, right, bottom, value):
    """FtVSlider::draw - groove, handle, and the handle's centre line."""
    cx = left + (right - left) / 2.0
    foot = bottom - SLIDER_BOTTOM_INSET
    c.rect([cx - 2.0, top + 4.0, cx + 2.0, foot - 4.0], fill=(0, 0, 0, 150))
    travel = (bottom - top) - SLIDER_BOTTOM_INSET - 12.0
    y = top + 6.0 + (1.0 - value) * travel
    c.rect([left + 1.0, y - 5.0, right - 1.0, y + 5.0],
           fill=(225, 230, 235, 255), outline=(0, 0, 0, 200), width=1)
    c.line([(left + 3.0, y), (right - 3.0, y)], fill=(90, 95, 100, 255))


# The trace colours the editor gives each panel, in its own order.
RESPONSE_TRACE = (120, 230, 160, 255)
WAVE_TRACE = (120, 200, 255, 255)
ENV_TRACES = ((255, 205, 120, 255),    # amplitude: amber
              (140, 235, 170, 255),    # filter: the response panel's green
              (185, 175, 255, 255))    # harmonic: violet


def draw_curve(c, box, data, bipolar, colour, fixed_scale=0.0, caption="", annotation=""):
    """FtCurveView::draw - plate, label band, then the trace."""
    left, top, right, bottom = box
    c.rect([left + 0.5, top + 0.5, right - 0.5, bottom - 0.5],
           fill=(0, 0, 0, 190), outline=(255, 255, 255, 110), width=1)

    band = 0.0
    if caption or annotation:
        band = 12.0
        tb = [left + 4.0, top + 3.0, right - 4.0, top + 14.0]
        if caption:
            c.text(tb, caption, F_TINY, (255, 255, 255, 200), align="left")
        if annotation:
            c.text(tb, annotation, F_TINY, (255, 255, 255, 140), align="right")

    if len(data) < 2:
        return

    peak = fixed_scale
    if peak <= 0.0:
        peak = max((abs(v) for v in data if math.isfinite(v)), default=1.0)
    if peak <= 0.0:
        peak = 1.0

    x0, width = left + 2.0, (right - left) - 4.0
    y0, height = top + 2.0 + band, (bottom - top) - 4.0 - band

    if bipolar:
        mid = y0 + height / 2.0
        c.line([(x0, mid), (x0 + width, mid)], fill=(255, 255, 255, 60))

    pts = []
    for i, raw in enumerate(data):
        v = raw / peak if math.isfinite(raw) else 0.0
        v = max(-1.0, min(1.0, v))
        x = x0 + width * i / (len(data) - 1)
        y = (y0 + height / 2.0 - v * (height / 2.0 - 1.0)) if bipolar else (y0 + height - v * height)
        pts.append((x, y))
    c.line(pts, fill=colour)
    return pts


def main():
    state = panel_state()
    ids, defaults = state["ids"], {int(k): v for k, v in state["defaults"].items()}
    def val(name, offset=0):
        return defaults[ids[name] + offset]

    art = {n: Image.open(os.path.join(RES, n + ".png")).convert("RGBA")
           for n in ("knob", "dot", "scale", "button_on", "button_off")}
    bg = Image.open(os.path.join(RES, "background.png"))
    if bg.size != (W, H):
        sys.exit("make-mockup: background.png is %dx%d, the editor is %dx%d"
                 % (bg.size[0], bg.size[1], W, H))
    c = Canvas(bg)

    def lamp(x, y, on):
        c.paste(art["button_on"] if on else art["button_off"], x, y)

    # -- harmonic columns: slider, phase knob, invert lamp ----------------
    for n in range(10):
        draw_vslider(c, px(193. + 20. * n), py(86.),
                     px(193. + 20. * n + 19.), py(86. + 86.), val("harm1", n))
        draw_knob(c, art, px(193. + 20. * n), py(180.), val("harmPhase1", n))
        lamp(px(193. + 20. * n), py(205.), val("harmInv1", n) >= 0.5)

    # -- amplitude envelope, filter envelope, LFOs ------------------------
    for name, x, y in (("envAttack", 409., 17.), ("envDecay", 451., 17.),
                       ("envSustain", 409., 52.), ("envRelease", 451., 52.),
                       ("envLevel", 492., 25.),
                       ("filterAttack", 89., 135.), ("filterDecay", 131., 135.),
                       ("filterSustain", 89., 170.), ("filterRelease", 131., 170.),
                       ("filterLevel", 107., 201.),
                       ("vibratoRate", 10., 17.), ("vibratoDepth", 49., 17.),
                       ("tremoloRate", 88., 17.), ("tremoloDepth", 127., 17.)):
        draw_knob(c, art, px(x), py(y), val(name))

    # -- filter sliders ----------------------------------------------------
    for name, x in (("cutoff", 21.), ("filterGain", 42.), ("poles", 63.)):
        draw_vslider(c, px(x), py(135.), px(x + 19.), py(135. + 80.), val(name))

    # -- the four shape selectors -----------------------------------------
    for x, text in zip((21., 54., 87., 120.), state["shapes"]):
        box = [px(x), py(113.), px(x + 30.), py(113.) + SELECTOR_H]
        c.rect([box[0] + 0.5, box[1] + 0.5, box[2] - 0.5, box[3] - 0.5],
               fill=(0, 0, 0, 255), outline=(255, 255, 255, 255), width=1)
        c.text(box, text, F_SMALL, (255, 255, 255, 255))

    # -- the formant bank: ten selectors, five knobs, ENV and INV ---------
    # Formant 1 is selected on a freshly opened editor, and only the selected
    # lamp is green - these are a radio group, not ten switches.
    row1x = (414., 431., 448., 466., 483.)
    row2x = (414., 432., 448., 466., 484.)
    for i in range(5):
        lamp(px(row1x[i]), py(182.), i == 0)
        lamp(px(row2x[i]), py(196.), False)
    for i, (x, y) in enumerate(zip((409., 451., 409., 451., 492.),
                                   (105., 105., 140., 140., 105.))):
        draw_knob(c, art, px(x), py(y), val("formantAttack1", i))
    lamp(px(491.), py(140.), val("formantInv1") >= 0.5)      # INV
    lamp(px(515.), py(140.), val("formantEnv1") >= 0.5)      # ENV

    # -- the two display panels -------------------------------------------
    draw_curve(c, [px(13.), py(49.), px(13. + 138.), py(49. + 63.)],
               state["response"], bipolar=False, colour=RESPONSE_TRACE)
    draw_curve(c, [px(219.), py(20.), px(219. + 138.), py(20. + 63.)],
               state["wave"], bipolar=True, colour=WAVE_TRACE)

    # -- the three envelope panels on the strip ---------------------------
    for i, env in enumerate(state["envelopes"]):
        top = ENV_TOP + i * (ENV_H + ENV_GAP)
        draw_curve(c, [ENV_X, top, ENV_X + ENV_W, top + ENV_H], env["data"],
                   bipolar=False, colour=ENV_TRACES[i], fixed_scale=1.0,
                   caption=env["caption"], annotation=env["annotation"])

    # -- output, pan, ladder, and the LFO button --------------------------
    for col, name, label in ((STRIP_COLS[0], "outputLevel", "Output"),
                             (STRIP_COLS[1], "pan", "Pan")):
        draw_knob(c, art, col - KNOB / 2.0, STRIP_ROW_TOP, val(name))
        c.text([col - 33, STRIP_LABEL_TOP, col + 33, STRIP_LABEL_TOP + 12],
               label, F_TINY, (255, 255, 255, 255))
    lamp(STRIP_COLS[2] - TOGGLE / 2.0, STRIP_ROW_TOP + (KNOB - TOGGLE) / 2.0,
         val("ladderEnable") >= 0.5)
    c.text([STRIP_COLS[2] - 33, STRIP_LABEL_TOP, STRIP_COLS[2] + 33, STRIP_LABEL_TOP + 12],
           "Ladder", F_TINY, (255, 255, 255, 255))

    btn = [ENV_X, LFO_BUTTON_TOP, ENV_X + ENV_W, LFO_BUTTON_TOP + 17]
    c.rect([btn[0] + 0.5, btn[1] + 0.5, btn[2] - 0.5, btn[3] - 0.5],
           fill=(0, 0, 0, 255), outline=(255, 255, 255, 160), width=1)
    c.text(btn, "Harmonic LFO...", F_TINY, (255, 255, 255, 255))

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    c.img.convert("RGB").save(OUT)
    print("wrote %s (%dx%d)" % (OUT, c.img.width, c.img.height))


if __name__ == "__main__":
    main()
