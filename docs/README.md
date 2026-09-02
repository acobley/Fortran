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

The figures are hand-authored inline SVG - no library, no runtime, no
external images - so they stay legible at any size and diff as text.
