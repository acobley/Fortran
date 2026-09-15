# Diagrams

Two drawings of the DSP, kept as HTML sources with a flat PNG render of each.
The HTML is the original; the PNG is what you get if you do not want to open
a browser. Regenerate a PNG from its source with any headless browser, e.g.

    python3 - <<'PY'
    from playwright.sync_api import sync_playwright
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        p = b.new_page(viewport={"width": 1100, "height": 1400},
                       device_scale_factor=1, color_scheme="light")
        p.goto("file://" + __import__("os").path.abspath("docs/signal-path.html"))
        p.wait_for_timeout(3000)
        p.screenshot(path="docs/signal-path.png", full_page=True)
        b.close()
    PY

Both pages render in the viewer's light or dark theme; the committed PNGs are
the light one. Keep the render under about 8000 px tall - taller images are
rejected by some transfer paths.

| | |
|---|---|
| `signal-path.*` | The whole voice, one sample at a time, then sixteen voices to the bus. |
| `partial-bank.*` | Stage 2 of that drawing opened up: one partial's two branches, and what differs between the ten. |
| `plugin-window.png` | The panel as a host shows it, freshly instantiated. |

## The window

`plugin-window.png` is not a screenshot - there is no way to take one from a
headless session - but neither is it a drawing of what the panel roughly looks
like. `tools/make-mockup.py` composites it from the same parts the editor
loads: `resource/background.png` and the knob, dot, scale and lamp bitmaps, at
the positions `ForTranEditor::open` computes, drawn the way
`ForTranControls.cpp` draws them.

Everything the panel *computes* comes from `tools/mockup-data.cpp`, which is
the real `FilterInfo` and the real `Adsr` with every parameter at the table's
own default - so the filter response, the waveform and the three envelope
traces are the ones the plug-in would actually show, not plausible curves.
That is why it needs the SDK:

    tools/make-mockup.py            # after ./setup-xcode.sh has fetched the SDK

What it shows is a fresh instance: the DXi's default patch, formant 1
selected, all four filter shapes Off, the harmonic envelope disabled (hence
the flat trace marked *off*), and the filter response flat at unity because
no shape is switched in. The value readout does not appear because it only
exists under the pointer.

The figures are hand-authored inline SVG - no library, no runtime, no
external images - so they stay legible at any size and diff as text.
