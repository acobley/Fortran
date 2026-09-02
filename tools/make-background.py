#!/usr/bin/env python3
"""Build resource/background.png from the DXi's res/background.bmp.

Two deliberate changes to the artwork.

1. The painted word HARMONICS sat at y 342..353, directly underneath the
   harmonic invert buttons, which the editor places at y 333..355. The
   buttons covered it. The word is moved down and the panel grown to make
   room.

2. The canvas is widened on the right to carry the three envelope displays,
   which have no counterpart in the DXi. Nothing of the original moves: the
   new strip is appended beyond the artwork's right edge.

3. The row of harmonic invert buttons is labelled "Inv". The DXi left that
   row unlabelled, which makes it read as a per-harmonic on/off switch when
   it is a phase inversion - it multiplies the harmonic's level by -1
   (ForTran.cpp:789) and never mutes anything. The word is not typeset: it
   is lifted from the artwork's own "Inv" in the Harminc ADSR panel, so the
   font, weight and colour match exactly and no font has to be available to
   run this script.

4. The row of harmonic phase knobs is labelled "Phase". That row is a real
   -90 to +90 degree phase control per harmonic and always was, but it is
   unlabelled too, so it reads as another anonymous row of knobs.

   The word does not appear anywhere in the artwork, so it is SET from the
   artwork's own letters: P from "Poles", a, s and e from "Release", and an
   h built from the l of "Poles" laid over the n of "Inv" - which is what
   an h is in this typeface. Rendering it with a font was tried first and
   rejected: the closest available (Carlito Bold, Liberation Sans Bold,
   Lato Bold) all came out lighter and wider than the artwork's face, and
   would have sat visibly wrong 40 px from the transplanted "Inv".

Kept as a script rather than a hand-edited PNG so the change is legible and
can be redone if the source artwork is revisited.

Why it is done this way. The obvious approach - insert a band of duplicated
rows to push everything below it down - does not work here, because the
panel has NO ink-free row: the labels ("Cutoff Q Poles", the formant digits,
"Level") run right down to the panel's bottom border at y 365. Duplicating
any row in that region smears the letter strokes crossing it into long
vertical streaks.

So instead: the border band and the outer background below it (y >= 365) are
moved down, the gap is filled with a per-column estimate of the panel's
interior colour, and only the HARMONICS label is relocated. Every other
label stays exactly where the original put it.

    python3 tools/make-background.py
"""

import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(HERE)
SRC = os.path.join(PROJECT, "..", "ForTran-DXI", "res", "background.bmp")
DST = os.path.join(PROJECT, "resource", "background.png")

# The label. Padded well beyond the ink: a box tight to the dark pixels
# (x 400..496, y 342..353) clips the anti-aliasing, and erasing from rows
# that still hold faint grey leaves ghost streaks where the strokes were.
LABEL = (394, 337, 503, 359)

# The panel's bottom border begins here, and everything from here down is
# border and outer background - no text.
BORDER_TOP = 365

# How far to move it all down. The label ends up at y 362..373: seven pixels
# clear of the buttons above, twelve clear of the border below.
GROW = 20

# How much to add on the right, for the three envelope panels. 200 px of
# panel with a 12 px margin either side.
#
# The strip is made by repeating the artwork's right-most column. That works
# because the gradient has already flattened horizontally by the time it
# reaches the edge - the last five columns of any row differ by at most one
# or two levels - while it still varies a lot vertically. Repeating the
# column therefore continues the vertical gradient exactly and leaves no
# seam; extrapolating the horizontal gradient over 224 px would drift.
WIDEN = 224

# The artwork's own "Inv", in the Harminc ADSR panel. Ink occupies x 739..754
# and y 265..273; the box is padded past that so the anti-aliasing is not
# clipped - a box tight to the ink is what left ghost streaks the first time
# the HARMONICS label was moved. "Enable" starts at x 765, so the right edge
# has to stay below that.
SRC_INV = (736, 261, 759, 278)

# Clear columns beside it, used to estimate the background the word is
# anti-aliased against. Between "Inv" and "Enable", verified ink-free.
SRC_INV_BG_COLS = (756, 764)

# Where it goes: left-aligned under the first invert button (x 290) and on
# the same baseline as HARMONICS, so the panel gets one tidy label line
# rather than two. The ink lands at x 290..305, y 365..373; the buttons end
# at y 355.
DST_INV = (287, 361)

# --- the composed "Phase" label ------------------------------------------
#
# The two label bands the glyphs come from. Both are nine ink rows with the
# baseline on the last, so a glyph cut from either band lands on a shared
# baseline with no per-letter alignment: an x-height letter simply has blank
# rows at the top of its box.
BAND_RIGHT = (265, 274)          # "Sustain  Release  Inv  Enable"
BAND_LEFT = (350, 359)           # "Cutoff  Q  Poles"

# Ink-free columns beside each band, for estimating the background those
# letters were anti-aliased against.
BG_RIGHT = (756, 764)            # between "Inv" and "Enable"
BG_LEFT = (128, 136)             # right of "Poles"

# Letters, as (x-first, x-last) inclusive. Verified to have a clear column
# either side, so nothing of a neighbour is cut in with them.
GLYPH_P = ((93, 99), BAND_LEFT, BG_LEFT)        # Poles
GLYPH_L = ((108, 109), BAND_LEFT, BG_LEFT)      # Poles - just the stem
GLYPH_N = ((742, 747), BAND_RIGHT, BG_RIGHT)    # Inv
GLYPH_E = ((688, 693), BAND_RIGHT, BG_RIGHT)    # Release
GLYPH_A = ((695, 700), BAND_RIGHT, BG_RIGHT)    # Release
GLYPH_S = ((702, 707), BAND_RIGHT, BG_RIGHT)    # Release

# One blank column between letters, which is what the artwork's own words use.
GLYPH_GAP = 1

# Where it goes: under the first phase knob. The knobs are drawn at y 292 and
# their bitmap's ink ends at y 320; the invert buttons start at y 333. That
# leaves twelve rows, and the nine-row label sits at 322..330 - one row clear
# of the knobs above, two of the buttons below, so it reads with the knobs.
# "Inv" sitting below the buttons settles any remaining doubt: each label is
# under the row it names.
DST_PHASE = (290, 322)

# The navy the labels are painted in, taken from the darkest pixel of the
# word itself at run time - see transplant_label.


# Rows to sample when estimating the interior colour per column. Close
# enough to the border that the gradient is right, wide enough that ink is
# always a minority of the samples.
SAMPLE = (328, BORDER_TOP)


def interior_colour(px, w, y0, y1):
    """Per column, the median pixel of rows y0..y1.

    The panel gradient is nearly flat vertically over this span, so the
    median is a good estimate of the background at any row in it - and
    unlike copying a single row it cannot pick up a letter stroke, because
    ink is always the minority of a column.

    Deliberately a plain median, with NO "is this ink?" threshold. An
    absolute cutoff does not work across this artwork: the left-hand panel's
    background is darker than the centre one's, so any threshold that
    excludes ink in the pale panel also excludes the *background* of the
    dark one, leaving a handful of anti-aliasing highlights to be averaged
    into a bright streak. That is exactly what the first attempt did, and it
    painted white bars under the word "Cutoff".
    """
    out = []
    for x in range(w):
        vals = sorted((px[x, y] for y in range(y0, y1)), key=sum)
        out.append(vals[len(vals) // 2])
    return out


def transplant_label(out, src, box, dest, bg_cols):
    """Copy a painted word to a different part of the canvas.

    Not a paste. The HARMONICS label could be pasted as a rectangle because
    it moves straight down within the same columns, where the gradient is
    nearly flat. This one moves 450 px left and 100 px up, from a mid-cyan
    background to a near-white one, so its rectangle would land as a
    conspicuous cyan patch.

    So the ink is keyed out instead. Each pixel is modelled as the navy ink
    laid over the local background with some coverage, the coverage is
    recovered from how far the pixel falls below that background, and the
    ink is composited onto whatever is at the destination. Anti-aliasing
    survives intact because coverage is what anti-aliasing is.

    The white drop shadow is deliberately NOT carried over. It exists to
    separate the word from the mid-cyan it sits on; the destination is
    almost white, where a white shadow is invisible anyway - which is
    exactly how HARMONICS looks on the same pale ground.
    """
    x0, y0, x1, y1 = box
    sp = src.load()
    op = out.load()

    # Per row, the background the word was painted on. Sampled from clear
    # columns beside it: the gradient runs vertically, so a row estimate is
    # right across the whole width of the word.
    bg = {}
    for y in range(y0, y1):
        vals = [sp[x, y] for x in range(bg_cols[0], bg_cols[1])]
        bg[y] = vals[len(vals) // 2]

    # The ink colour, from the darkest pixel of the word itself.
    ink = min((sp[x, y] for x in range(x0, x1) for y in range(y0, y1)), key=sum)

    dx, dy = dest
    for y in range(y0, y1):
        b = bg[y]
        # The green channel has the widest swing here - background 220,
        # ink 30 - so it carries the coverage most cleanly.
        span = b[1] - ink[1]
        if span <= 0:
            continue
        for x in range(x0, x1):
            p = sp[x, y]
            alpha = (b[1] - p[1]) / float(span)
            if alpha <= 0.0:
                continue                      # background, or the white shadow
            if alpha > 1.0:
                alpha = 1.0
            tx, ty = dx + (x - x0), dy + (y - y0)
            d = op[tx, ty]
            op[tx, ty] = tuple(
                int(round(d[c] * (1.0 - alpha) + ink[c] * alpha)) for c in range(3)
            )
    return ink


def glyph_coverage(px, spec, ink):
    """One painted letter, as a grid of ink coverage 0..1.

    Same keying as transplant_label: the letter is modelled as ink over the
    local background, and coverage is recovered from how far each pixel
    falls below a per-row estimate of that background.
    """
    (x0, x1), (y0, y1), bg_cols = spec
    grid = []
    for y in range(y0, y1):
        vals = sorted((px[x, y] for x in range(bg_cols[0], bg_cols[1])), key=sum)
        bg = vals[len(vals) // 2]
        span = float(bg[1] - ink[1])
        row = []
        for x in range(x0, x1 + 1):
            a = (bg[1] - px[x, y][1]) / span if span > 0 else 0.0
            row.append(0.0 if a < 0.0 else (1.0 if a > 1.0 else a))
        grid.append(row)
    return grid


def overlay(base, over):
    """`over` laid onto the left of `base`, taking whichever is more solid.

    Used to make an h: in this typeface an h is an n whose left stem runs up
    to ascender height, and an l IS that stem. Both come from nine-row bands
    with a shared baseline, so they need no alignment.
    """
    out = [row[:] for row in base]
    for y in range(min(len(out), len(over))):
        for x in range(min(len(out[y]), len(over[y]))):
            if over[y][x] > out[y][x]:
                out[y][x] = over[y][x]
    return out


def compose_label(out, src, glyphs, dest, ink):
    """Set a word from single letters and composite it onto the canvas."""
    op = out.load()
    dx, dy = dest
    x = dx
    for grid in glyphs:
        for row_index, row in enumerate(grid):
            for col_index, alpha in enumerate(row):
                if alpha <= 0.0:
                    continue
                tx, ty = x + col_index, dy + row_index
                d = op[tx, ty]
                op[tx, ty] = tuple(
                    int(round(d[c] * (1.0 - alpha) + ink[c] * alpha)) for c in range(3)
                )
        x += len(grid[0]) + GLYPH_GAP
    return x - GLYPH_GAP - dx


def main():
    if not os.path.exists(SRC):
        sys.exit("cannot find the DXi artwork at %s" % SRC)

    src = Image.open(SRC).convert("RGB")
    w, h = src.size
    if (w, h) != (840, 388):
        sys.exit("expected the original 840x388 artwork, got %dx%d" % (w, h))

    label = src.crop(LABEL)
    px = src.load()
    fill = interior_colour(px, w, SAMPLE[0], SAMPLE[1])

    out = Image.new("RGB", (w + WIDEN, h + GROW))

    # 1. Everything above the border, unchanged - every label keeps its place.
    out.paste(src.crop((0, 0, w, BORDER_TOP)), (0, 0))

    # 2. The new gap, filled with the panel's own interior colour.
    gap = Image.new("RGB", (w, GROW))
    gp = gap.load()
    for x in range(w):
        for y in range(GROW):
            gp[x, y] = fill[x]
    out.paste(gap, (0, BORDER_TOP))

    # 3. The border band and the outer background, moved down.
    out.paste(src.crop((0, BORDER_TOP, w, h)), (0, BORDER_TOP + GROW))

    # 4. Erase the label where it was, and put it back lower.
    op = out.load()
    x0, y0, x1, y1 = LABEL
    for x in range(x0, x1):
        for y in range(y0, y1):
            op[x, y] = fill[x]
    out.paste(label, (x0, y0 + GROW))

    # 5. The strip on the right: the last column, repeated. Done after the
    #    vertical work so it picks up the moved border and the grown height,
    #    and the new area is a seamless continuation of the panel's edge.
    if WIDEN > 0:
        edge = out.crop((w - 1, 0, w, h + GROW))
        for x in range(w, w + WIDEN):
            out.paste(edge, (x, 0))

    # 6. Label the invert row. Done last, onto the finished canvas, because
    #    it composites against whatever is already at the destination.
    ink = transplant_label(out, src, SRC_INV, DST_INV, SRC_INV_BG_COLS)
    print("Inv label: x %d..%d, under the first invert button (buttons end y 355), ink %s"
          % (DST_INV[0] + 3, DST_INV[0] + 18, ink))

    # 7. Label the phase row, set from the artwork's own letters.
    sp = src.load()
    letters = [
        glyph_coverage(sp, GLYPH_P, ink),
        overlay(glyph_coverage(sp, GLYPH_N, ink), glyph_coverage(sp, GLYPH_L, ink)),
        glyph_coverage(sp, GLYPH_A, ink),
        glyph_coverage(sp, GLYPH_S, ink),
        glyph_coverage(sp, GLYPH_E, ink),
    ]
    width = compose_label(out, src, letters, DST_PHASE, ink)
    print("Phase label: x %d..%d, y %d..%d (knobs end y 320, buttons start y 333)"
          % (DST_PHASE[0], DST_PHASE[0] + width - 1,
             DST_PHASE[1], DST_PHASE[1] + (BAND_RIGHT[1] - BAND_RIGHT[0]) - 1))

    out.save(DST)
    print("wrote %s  %dx%d  (was %dx%d)" % (DST, out.size[0], out.size[1], w, h))
    print("right-hand strip: x %d..%d, for the envelope displays" % (w, w + WIDEN - 1))
    print("HARMONICS ink moves y 342..353 -> %d..%d; buttons end at y 355" % (342 + GROW, 353 + GROW))
    print("panel border moves y %d -> %d" % (BORDER_TOP, BORDER_TOP + GROW))


if __name__ == "__main__":
    main()
