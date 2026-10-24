# ForTran designs

- `ForTran-ConceptA-SignalFlow.png`, `ConceptB-PartialMatrix.png`, `ConceptC-FormantLandscape.png` - first concepts (R2Synth palette).
- `ForTran-Polivoks-1-SignalFlow-StudioWorn.png` - Concept A layout on R2Synth's Polivoks black panel: colour OLEDs, 70s sci-fi lettering, knurled knobs and slide faders, rotary selectors in place of every segment switch; studio-worn finish.
- `ForTran-Polivoks-2-SpectrumBank-RoadCase.png` - R2Synth's own frame (three boxes over a strip). Partials as a bank of ten knob columns under an OLED level bar graph, the selected partial in its own box; road-case finish (gouges, dents, chipped paint, cracked mod OLED).
- `ForTran-Polivoks-3-Matrix-BarnFind.png` - programmer matrix: every partial's eight parameters as knobs, nothing behind a selection; monochrome white OLEDs, red TRAN; barn-find finish (dust coat, yellowed legends, rust, mug ring).

- `ForTran-Polivoks-2b-SpectrumBank-StudioWorn.png` - design 2's layout with design 1's lighter studio-worn finish (`weather.py d2-clean.png d2-layout.json out 1 ...`).
Sources in `source/polivoks/`: `python3 render.py d1 d2 d3` (clean PNG + layout dump), then
`python3 weather.py dN-clean.png dN-layout.json out <1|2|3> <name>.png` (port of R2Synth's render-weathered.py;
design 3 used `SLEEVE='[1500,450]' SWIPE='[1360,866]' MUG='[760,1010]'`).
