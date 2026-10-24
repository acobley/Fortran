// Design 2 - Spectrum Bank: R2Synth's own frame (three boxes over a full-width
// strip). Each section opens with an OLED; the partials become a bank of ten
// knob columns read off a level bar graph; the selected partial gets its own box.
header('48000 HZ  /  16 VOICES  /  PITCH WHEEL 1 OCT', { wm: ['#f4f1e8', '#f4f1e8'] });

// ---- PARTIALS ---------------------------------------------------------------
section(16, 72, 524, 360, 'PARTIALS', 'TEN SINES AT K × THE NOTE');
oled(30, 106, 496, 108);
otext(38, 119, 'LEVEL  1-10', T.oledText);
otext(518, 119, 'P3  64 %', OLED.amber, 'end');
const bx = i => 78 + i * 44.8;
for (let i = 0; i < 10; i++) {
  const x = bx(i), base = 200, h = 70 * P.level[i];
  for (let s = 0; s < Math.round(P.level[i] * 14); s++)
    rect(x - 13, base - 5 - s * 5, 26, 3.4, i === SEL ? OLED.amber : hexA(OLED.amber, 0.5), null, 0.5, 1, i === SEL ? 'filter="url(#bloom)"' : '');
  otext(x, 210, i + 1, i === SEL ? OLED.amber : T.oledText, 'middle', 8);
}
glass(30, 106, 496, 108);
const rowLab = (y, s) => text(30, y, s, { size: 9 });
rowLab(275, 'LEVEL'); rowLab(312, 'PHASE'); rowLab(343, 'INV'); rowLab(366, 'ENV');
for (let i = 0; i < 10; i++) {
  const x = bx(i);
  badge(x, 238, i + 1, i === SEL);
  knob(x, 271, 14, P.level[i], null, null);
  knob(x, 309, 10, P.phase[i], null, null);
  lamp(x, 339, P.inv[i]);
  lamp(x, 362, P.env[i]);
}
rule(30, 384, 526, 384);
text(30, 404, 'CLICK A NUMBER TO EDIT THAT PARTIAL BELOW', { size: 8.6, weight: 400, fill: T.muted });
button(406, 392, 120, 18, 'COPY 3 TO ALL', { size: 8.5 });

chevron(548, 252);

// ---- FILTER ---------------------------------------------------------------
section(556, 72, 340, 360, 'SPECTRAL FILTER');
lamp(844, 94, 0); text(882, 98, 'BYP', { size: 9, anchor: 'end' });
filterOled(570, 106, 312, 108, { sel: 3, label: 'AT A3', right: '2.40K' });
selector(604, 258, 11, ['OFF', 'ON'], 1, { label: 'LP' });
selector(682, 258, 11, ['OFF', 'ON'], 0, { label: 'HP' });
selector(760, 258, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 1, { label: 'COMB', ns: 7.5, lr: 20 });
selector(840, 258, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 0, { label: 'GRASS', ns: 7.5, lr: 20 });
knob(604, 330, 18, 0.62, '2.40K', 'CUTOFF');
knob(682, 330, 18, 0.35, '35 %', 'Q');
knob(760, 330, 18, 0.6, '4', 'POLES');
knob(840, 330, 18, 0.8, '+60 %', 'ENV LVL', { bipolar: true });
rule(570, 386, 882, 386);
sub(570, 412, 'ENV', { size: 9.5 });
[['A', '20 MS', 0.25], ['D', '350 MS', 0.5], ['S', '30 %', 0.3], ['R', '300 MS', 0.45]].forEach((k, i) =>
  { knob(640 + i * 66, 408, 9, k[2], null, null); text(640 + i * 66 + 14, 405, k[0], { size: 9 }); otext(640 + i * 66 + 14, 417, k[1], T.muted, 'start', 7.5); });

chevron(904, 252);

// ---- AMP ----------------------------------------------------------------------
section(912, 72, 272, 360, 'AMP', 'ENVELOPE + OUTPUT');
envOled(926, 106, 244, 108, OLED.mint, 0.003, 0.6, 0.7, 0.42, { label: 'KEY UP AT THE LINE', right: '613 MS', total: 1.4, keyUp: 0.85, lvl: 0.8 });
[['3 MS', 'A', 0.08], ['600 MS', 'D', 0.52], ['70 %', 'S', 0.7], ['420 MS', 'R', 0.5]].forEach((k, i) =>
  knob(950 + i * 64, 250, 15, k[2], k[0], k[1]));
rule(926, 300, 1170, 300);
knob(954, 336, 17, 0.5, '-20 DB', 'LEVEL');
knob(1020, 336, 17, 0.5, 'C', 'PAN', { bipolar: true });
knob(1086, 336, 17, 0, 'OFF', 'GLIDE');
selector(1146, 336, 10, ['OFF', 'ON'], 1, { label: 'LADDER', lgap: 22, ns: 7.5 });
rule(926, 390, 1170, 390);
otext(926, 410, 'LADDER +10.6 DB BELOW B4', T.muted, 'start', 8);
otext(926, 423, 'CORNER 0.198 × NYQUIST', T.muted, 'start', 8);

// ---- PARTIAL 3 ----------------------------------------------------------------
section(16, 448, 524, 176, 'PARTIAL 3', '3 × F  FORMANT ENV');
button(440, 459, 40, 18, '‹ 2'); button(486, 459, 40, 18, '4 ›');
envOled(30, 488, 172, 122, OLED.amber, P.A[SEL], P.D[SEL], P.S[SEL], P.R[SEL], { label: 'FORMANT 3', right: '0.8 ×', lvl: 0.75, total: 1.3, keyUp: 0.8 });
knob(236, 512, 14, 0.18, ms(P.A[SEL]), 'A', { vs: 8.5 });
knob(284, 512, 14, 0.48, ms(P.D[SEL]), 'D', { vs: 8.5 });
knob(332, 512, 14, P.S[SEL], pc(P.S[SEL]), 'S', { vs: 8.5 });
knob(380, 512, 14, 0.55, ms(P.R[SEL]), 'R', { vs: 8.5 });
knob(440, 512, 18, 0.8, '80 %', 'LEVEL');
selector(498, 512, 10, ['OFF', 'ON'], 1, { label: 'ENV', lgap: 24, ns: 7.5 });
rule(222, 562, 526, 562);
sub(222, 584, 'HARMONIC', { size: 9.5 }); sub(222, 598, 'LFO 3', { size: 9.5 });
knob(316, 586, 11, 0.35, null, null); text(332, 583, 'RATE', { size: 9 }); otext(332, 596, '1.40 HZ', T.muted, 'start', 8);
knob(410, 586, 11, 0.3, null, null); text(426, 583, 'DEPTH', { size: 9 }); otext(426, 596, '30 %', T.muted, 'start', 8);
text(510, 584, 'INV', { size: 9, anchor: 'end' }); lamp(516, 598, 0);

// ---- MODULATION ---------------------------------------------------------------
section(556, 448, 628, 176, 'MODULATION', 'PITCH + LEVEL, TEN HARMONIC LFOS');
sub(572, 498, 'VIBRATO', { size: 9.5 });
knob(594, 540, 18, 0.45, '5.20 HZ', 'RATE');
knob(656, 540, 18, 0.12, '12 %', 'DEPTH');
sub(712, 498, 'TREMOLO', { size: 9.5 });
knob(734, 540, 18, 0.3, '3.00 HZ', 'RATE');
knob(796, 540, 18, 0, 'OFF', 'DEPTH');
line(846, 486, 846, 612, '#3a3a3a', 1);
oled(862, 488, 308, 122);
otext(870, 502, 'HARMONIC LFOS  RATE / DEPTH', T.oledText, 'start', 8.5);
for (let i = 0; i < 10; i++) {
  const x = 878 + i * 29.5, cy = 548, r = 11, dv = P.lfoDepth[i], rt = P.lfoRate[i];
  circle(x, cy, r, 'none', '#1c1c26', 2.4);
  if (dv > 0) {
    const a = -135 + 270 * Math.min(1, dv * 2);
    const [x0, y0] = pol(x, cy, r, -135), [x1, y1] = pol(x, cy, r, a);
    path(`M${f1(x0)},${f1(y0)} A${r},${r} 0 ${a + 135 > 180 ? 1 : 0} 1 ${f1(x1)},${f1(y1)}`, i === SEL ? OLED.violet : hexA(OLED.violet, 0.6), 2.4, 'none', 'filter="url(#bloom)"');
    otext(x, 552, Math.round(rt * 4 * 10) / 10, OLED.violet, 'middle', 7);
  }
  otext(x, 584, i + 1, i === SEL ? OLED.violet : T.oledText, 'middle', 8);
}
otext(870, 603, 'HZ IN THE RING, DEPTH ROUND IT', T.oledText, 'start', 7.5);
glass(862, 488, 308, 122);

render();
