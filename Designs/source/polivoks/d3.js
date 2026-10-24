// Design 3 - Programmer Matrix: every partial's whole voice as a column of
// knurled knobs, Polivoks/modular style - nothing hidden behind a selection.
// Monochrome white OLEDs (one colour, like a 128 x 64 module).
OLED = { amber: '#eef3ff', blue: '#eef3ff', mint: '#eef3ff', violet: '#eef3ff', white: '#eef3ff' };
header('48000 HZ  /  16 VOICES  /  PITCH WHEEL 1 OCT', { wm: ['#f4f1e8', '#e2342b'] });
const W = OLED.white;

// ---- PARTIALS MATRIX -----------------------------------------------------
section(16, 72, 568, 552, 'PARTIALS', 'TEN SINES AT K × THE NOTE');
oled(30, 106, 540, 84);
otext(38, 119, 'TEN FORMANT ENVELOPES', T.oledText);
otext(562, 119, 'P3', W, 'end');
for (let i = 9; i >= 0; i--) {
  const p = adsr(30, 106, 540, 84, P.A[i], P.D[i], P.S[i], P.R[i], 1.3, 0.8, P.level[i], 20);
  if (i === SEL) { path(`${p.d} L${f1(p.x1)},${f1(p.base)} L${f1(p.x0)},${f1(p.base)} Z`, 'none', 0, fillGrad(W)); trace(p.d, W, 1.8); }
  else path(p.d, hexA(W, 0.28), 1);
}
glass(30, 106, 540, 84);
const cx = i => 130 + i * 47;
const rows = [['LEVEL', P.level], ['PHASE', P.phase], ['ATTACK', P.A.map(a => a * 8)], ['DECAY', P.D.map(d => d * 0.9)],
  ['SUSTAIN', P.S], ['RELEASE', P.R.map(r => r * 1.4)], ['LFO RATE', P.lfoRate], ['LFO DEPTH', P.lfoDepth.map(d => d * 2)]];
for (let i = 0; i < 10; i++) badge(cx(i), 214, i + 1, i === SEL);
rows.forEach(([name, v], r) => {
  const y = 250 + r * 37;
  text(30, y + 3.5, name, { size: 8.6 });
  if (r === 2 || r === 6) line(30, y - 19, 570, y - 19, '#2c2c2c', 1);
  for (let i = 0; i < 10; i++) knob(cx(i), y, 12, v[i], null, null);
});
text(30, 558, 'INV', { size: 8.6 }); text(30, 581, 'ENV', { size: 8.6 });
for (let i = 0; i < 10; i++) { lamp(cx(i), 554, P.inv[i]); lamp(cx(i), 577, P.env[i]); }
// the selected column, framed in red like a patch-bay strip
rect(cx(SEL) - 19, 200, 38, 390, 'none', T.red, 3, 1.2);
rule(30, 598, 570, 598);
text(30, 614, 'PARTIAL 3 FRAMED  /  OLED SHOWS IT BRIGHT', { size: 8.6, weight: 400, fill: T.muted });

chevron(592, 200);

// ---- SPECTRAL FILTER -----------------------------------------------------------
section(600, 72, 584, 268, 'SPECTRAL FILTER');
lamp(1106, 94, 0); text(1170, 98, 'BYPASS', { size: 9, anchor: 'end' });
filterOled(614, 106, 300, 128, { sel: 3, label: 'AT A3', right: '2.40K' });
knob(644, 278, 17, 0.62, '2.40K', 'CUTOFF');
knob(718, 278, 17, 0.35, '35 %', 'Q');
knob(792, 278, 17, 0.6, '4', 'POLES');
knob(868, 278, 17, 0.8, '+60 %', 'ENV LVL', { bipolar: true });
line(934, 104, 934, 326, '#3a3a3a', 1);
selector(968, 140, 11, ['OFF', 'ON'], 1, { label: 'LP' });
selector(1046, 140, 11, ['OFF', 'ON'], 0, { label: 'HP' });
selector(1124, 140, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 1, { label: 'COMB', ns: 7.5, lr: 20 });
rule(950, 186, 1170, 186);
selector(980, 222, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 0, { label: 'GRASS', ns: 7.5, lr: 20 });
sub(1036, 210, 'FILTER ENV', { size: 9 });
[['A', '20 MS', 0.25], ['D', '350 MS', 0.5], ['S', '30 %', 0.3], ['R', '300 MS', 0.45]].forEach((k, i) =>
  knob(1048 + i * 38, 232, 9, k[2], null, k[0], { vgap: 11, ls: 8.5 }));
rule(950, 266, 1170, 266);
otext(950, 288, 'COMB ADD  /  GRASS OFF', T.muted, 'start', 8);
otext(950, 302, 'ENV 20 / 350 / 30% / 300', T.muted, 'start', 8);

// ---- AMP -------------------------------------------------------------------
section(600, 356, 290, 268, 'AMP', 'ENVELOPE + OUTPUT');
envOled(614, 390, 262, 86, W, 0.003, 0.6, 0.7, 0.42, { label: 'KEY UP AT THE LINE', right: '613 MS', total: 1.4, keyUp: 0.85, lvl: 0.8 });
[['3 MS', 'A', 0.08], ['600', 'D', 0.52], ['70 %', 'S', 0.7], ['420', 'R', 0.5]].forEach((k, i) =>
  knob(640 + i * 70, 508, 13, k[2], k[0], k[1], { vs: 8.5 }));
knob(640, 580, 12, 0.5, null, null); text(658, 577, 'LEVEL', { size: 8.6 }); otext(658, 590, '-20 DB', T.muted, 'start', 7.5);
knob(746, 580, 12, 0.5, null, null, { bipolar: true }); text(764, 577, 'PAN', { size: 8.6 }); otext(764, 590, 'C', T.muted, 'start', 7.5);
selector(846, 582, 9, ['OFF', 'ON'], 1, { label: 'LADDER', lgap: 22, ns: 7 });

// ---- MODULATION ----------------------------------------------------------------
section(906, 356, 278, 268, 'MOD', 'VIBRATO + TREMOLO');
oled(920, 390, 250, 86);
otext(928, 403, 'PITCH', T.oledText); otext(1162, 403, '5.20 HZ', W, 'end');
{ let d = ''; for (let i = 0; i <= 100; i++) { const x = 926 + i * 238 / 100, y = 424 - 7 * Math.sin(i / 100 * Math.PI * 9); d += (i ? ' L' : 'M') + f1(x) + ',' + f1(y); } trace(d, W, 1.4); }
otext(928, 449, 'LEVEL', T.oledText); otext(1162, 449, '3.00 HZ  OFF', T.oledText, 'end');
line(926, 464, 1164, 464, hexA(W, 0.35), 1.2, 'stroke-dasharray="2 3"');
glass(920, 390, 250, 86);
sub(920, 504, 'VIBRATO', { size: 9 }); sub(1050, 504, 'TREMOLO', { size: 9 });
knob(940, 538, 13, 0.45, '5.20', 'RATE', { vs: 8.5 });
knob(996, 538, 13, 0.12, '12 %', 'DEPTH', { vs: 8.5 });
knob(1070, 538, 13, 0.3, '3.00', 'RATE', { vs: 8.5 });
knob(1126, 538, 13, 0, 'OFF', 'DEPTH', { vs: 8.5 });
knob(940, 600, 9, 0, null, null); text(954, 597, 'GLIDE', { size: 8.6 }); otext(954, 609, 'OFF', T.muted, 'start', 7.5);

render();
