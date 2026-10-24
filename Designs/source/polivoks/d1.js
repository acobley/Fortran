// Design 1 - Signal Flow: the Concept A arrangement on the Polivoks black
// panel. Partials | Spectral Filter | Amp, Modulation underneath. Colour OLEDs.
header('48000 HZ  /  16 VOICES  /  PITCH WHEEL 1 OCT', { wm: ['#f4f1e8', '#ffae2e'] });

// ---- PARTIALS ------------------------------------------------------------
section(16, 72, 520, 552, 'PARTIALS', 'TEN SINES AT K × THE NOTE');
const colX = i => 108 + i * 43;
const rowLab = (y, s) => text(30, y, s, { size: 9 });
rowLab(190, 'LEVEL'); rowLab(338, 'PHASE'); rowLab(368, 'INV'); rowLab(391, 'ENV');
for (let i = 0; i < 10; i++) {
  const cx = colX(i);
  fader(cx, 112, 254, P.level[i]);
  text(cx, 278, Math.round(P.level[i] * 100), { size: 9, font: MONO, weight: 400, anchor: 'middle', fill: T.muted });
  badge(cx, 300, i + 1, i === SEL);
  knob(cx, 334, 11, P.phase[i], null, null);
  lamp(cx, 364, P.inv[i]);
  lamp(cx, 387, P.env[i]);
}
// selected column bracket
line(colX(SEL) - 20, 102, colX(SEL) - 20, 400, T.red, 1.2, 'stroke-dasharray="2 3"');
line(colX(SEL) + 20, 102, colX(SEL) + 20, 400, T.red, 1.2, 'stroke-dasharray="2 3"');

// focus card
rect(30, 410, 492, 200, 'none', '#5a5852', 2, 1);
sub(44, 432, 'PARTIAL 3', { fill: T.ink });
text(130, 431, '3 × F  FORMANT ENV + HARMONIC LFO', { size: 8.6, weight: 400, fill: T.muted });
button(436, 419, 36, 18, '‹ 2'); button(476, 419, 36, 18, '4 ›');
envOled(44, 446, 190, 92, OLED.amber, P.A[SEL], P.D[SEL], P.S[SEL], P.R[SEL], { label: 'FORMANT 3', right: '0.8 ×', lvl: 0.75, total: 1.3, keyUp: 0.8 });
selector(78, 574, 11, ['OFF', 'ON'], 1, { label: 'ENV' });
text(150, 572, 'INV', { size: 9, anchor: 'end' }); lamp(164, 569, 0);
button(132, 584, 102, 18, 'COPY TO ALL', { size: 8.5 });
const kx = [276, 322, 368, 414, 474];
knob(kx[0], 466, 15, 0.18, ms(P.A[SEL]), 'A');
knob(kx[1], 466, 15, 0.48, ms(P.D[SEL]), 'D');
knob(kx[2], 466, 15, P.S[SEL], pc(P.S[SEL]), 'S');
knob(kx[3], 466, 15, 0.55, ms(P.R[SEL]), 'R');
knob(kx[4], 466, 17, 0.8, '80 %', 'LEVEL');
rule(256, 516, 508, 516);
sub(258, 534, 'HARMONIC LFO 3', { size: 9.5 });
knob(282, 566, 13, 0.35, '1.40 HZ', 'RATE', { vs: 8.5, ls: 8.5 });
knob(338, 566, 13, 0.3, '30 %', 'DEPTH', { vs: 8.5, ls: 8.5 });
oled(380, 530, 130, 64);
{ let d = ''; for (let i = 0; i <= 80; i++) { const x = 386 + i * 118 / 80, y = 566 - 16 * Math.sin(i / 80 * Math.PI * 4); d += (i ? ' L' : 'M') + f1(x) + ',' + f1(y); } trace(d, OLED.violet); }
otext(386, 543, 'LFO 3', T.oledText, 'start', 8);
otext(504, 590, '0.3 SIN', OLED.violet, 'end', 8);
glass(380, 530, 130, 64);

chevron(544, 262);

// ---- SPECTRAL FILTER --------------------------------------------------------
section(552, 72, 348, 392, 'SPECTRAL FILTER');
text(886, 98, 'BYPASS', { size: 9, anchor: 'end' }); lamp(830, 94, 0);
filterOled(566, 106, 320, 116, { sel: 3 });
selector(606, 268, 11, ['OFF', 'ON'], 1, { label: 'LP' });
selector(686, 268, 11, ['OFF', 'ON'], 0, { label: 'HP' });
selector(766, 268, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 1, { label: 'COMB', ns: 7.5, lr: 20 });
selector(846, 268, 11, ['OFF', 'ADD', 'MULT', 'MAX'], 0, { label: 'GRASS', ns: 7.5, lr: 20 });
knob(606, 338, 19, 0.62, '2.40K', 'CUTOFF');
knob(686, 338, 19, 0.35, '35 %', 'Q');
knob(766, 338, 19, 0.6, '4', 'POLES');
knob(846, 338, 19, 0.8, '+60 %', 'ENV LEVEL', { bipolar: true });
rule(566, 394, 886, 394);
sub(566, 412, 'FILTER ENV', { size: 9.5 });
envOled(566, 418, 120, 36, OLED.blue, 0.02, 0.35, 0.3, 0.3, { total: 1.3, keyUp: 0.8 });
[['A', '20 MS', 0.25], ['D', '350 MS', 0.5], ['S', '30 %', 0.3], ['R', '300 MS', 0.45]].forEach((k, i) =>
  knob(724 + i * 46, 416, 10, k[2], k[1], k[0], { vs: 8, ls: 8.5, vgap: 9, lgap: 10 }));

chevron(908, 262);

// ---- AMP ----------------------------------------------------------------
section(916, 72, 268, 392, 'AMP', 'ENVELOPE + OUTPUT');
envOled(930, 106, 240, 96, OLED.mint, 0.003, 0.6, 0.7, 0.42, { label: 'KEY UP AT THE DOTTED LINE', right: '613 MS', total: 1.4, keyUp: 0.85, lvl: 0.8 });
[['3 MS', 'A', 0.08], ['600 MS', 'D', 0.52], ['70 %', 'S', 0.7], ['420 MS', 'R', 0.5], ['100 %', 'LEVEL', 1]].forEach((k, i) =>
  knob(950 + i * 50, 240, 15, k[2], k[0], k[1]));
rule(930, 290, 1170, 290);
knob(966, 330, 19, 0.5, '-20.0 DB', 'LEVEL');
knob(1050, 330, 19, 0.5, 'C', 'PAN', { bipolar: true });
knob(1134, 330, 19, 0, 'OFF', 'GLIDE');
rule(930, 386, 1170, 386);
selector(962, 422, 11, ['OFF', 'ON'], 1, { label: 'LADDER', lgap: 15 });
otext(1012, 418, '+10.6 DB BELOW B4', T.muted, 'start', 8.5);
otext(1012, 432, 'CORNER 0.198 × NYQ', T.muted, 'start', 8.5);

// ---- MODULATION -------------------------------------------------------------
section(552, 476, 632, 148, 'MODULATION', 'PITCH + LEVEL LFOS, TEN HARMONIC LFOS');
sub(570, 524, 'VIBRATO', { size: 9.5 });
knob(590, 562, 17, 0.45, '5.20 HZ', 'RATE');
knob(650, 562, 17, 0.12, '12 %', 'DEPTH');
sub(708, 524, 'TREMOLO', { size: 9.5 });
knob(728, 562, 17, 0.3, '3.00 HZ', 'RATE');
knob(788, 562, 17, 0, 'OFF', 'DEPTH');
line(838, 512, 838, 612, '#3a3a3a', 1);
oled(856, 506, 314, 104);
otext(864, 520, 'HARMONIC LFO DEPTH  1-10', T.oledText, 'start', 8.5);
for (let i = 0; i < 10; i++) {
  const x = 868 + i * 30, top = 530, h = 56, dv = P.lfoDepth[i];
  rect(x, top, 18, h, '#0d0d12', null, 1);
  for (let s = 1; s < 8; s++) line(x + 1, top + h * s / 8, x + 17, top + h * s / 8, '#000', 1);
  if (dv > 0) rect(x, top + h - h * dv * 1.8, 18, h * dv * 1.8, i === SEL ? OLED.violet : hexA(OLED.violet, 0.55), null, 1, 1, 'filter="url(#bloom)"');
  otext(x + 9, 603, i + 1, i === SEL ? OLED.violet : T.oledText, 'middle', 8);
}
glass(856, 506, 314, 104);

render();
