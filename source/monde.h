#pragma once
// ======================= MONDE ÉLARGI, CAMÉRA ET VIE DES CORBEAUX (monde.h) =======================
// Inclus par corbeau.h, dans l'espace de noms cb, après les définitions dont il a besoin.
// Le paysage (ciel, collines et arbres lointains, sol, maison, mare, champ, arbre) est dessiné une seule fois en quatre
// bandes de pixels ; la caméra (vue d'ensemble, suivi d'un oiseau) les recompose à chaque image avec de la parallaxe.
// Les corbeaux partent en excursion (maison, mare, champ) puis reviennent : ils utilisent le même code de vol que sur l'arbre.
static void chasser(Corbeau& c);
static void choc(float x, float y, Corbeau* a, Corbeau* b, float force);
static void chamaille();
static void volGroupe();
static void appelMutuel();

namespace mo {

// ---------- étendue du monde (l'arbre est entre x = 20 et 250, le sol vers y = 202) ----------
static const float MX0 = -240.0f, MX1 = 560.0f;            // largeur des bandes
static const float FY0 = 80.0f, FY1 = 208.0f;              // bande lointaine : arbres lointains et collines
static const float NY0 = 80.0f, NY1 = 256.0f;              // bande proche : sol, maison, mare, champ, clôtures
static const float AX0 = -20.0f, AY0 = 0.0f;               // bande de l'arbre (origine)
static const int WL = (int)(800.0f * ZN + 0.5f), HF = (int)(128.0f * ZN + 0.5f), HN = (int)(176.0f * ZN + 0.5f), WA = (int)(271.0f * ZN + 0.5f), HA = (int)(257.0f * ZN + 0.5f);      // tailles en pixels à l'échelle ZN
static const float HX = 430.0f, HB = 199.0f;               // maison : milieu et pied
static const float PX = -70.0f, PY = 205.0f, PRX = 34.0f, PRY = 5.5f;      // mare
static const float CH0 = -176.0f, CH1 = -112.0f;           // champ : bords gauche et droit
static const uint16_t CLE_TR = 0x1818;                        // couleur transparente (identique dans les deux ordres d'octets)

enum { D_MAISON = 0, D_MARE, D_CHAMP, D_EPOUV, D_DISPUTE, D_NB };
enum { CM_NON = 0, CM_LARGE, CM_SUIVI };

static M5Canvas sCiel(&M5.Display), sFar(&M5.Display), sNear(&M5.Display), sArbre(&M5.Display);
static bool pret = false, tentee = false, vigFort = true;
static uint8_t* vigK = nullptr;                            // table du vignettage (un octet par pixel de la scène)
static float tMonde = 0.0f;

// ---------- petits outils de dessin (coordonnées du monde, vue courante) ----------
static void rect(M5Canvas& g, float x0, float y0, float x1, float y1, uint16_t c) { tri(g, x0, y0, x1, y0, x1, y1, c); tri(g, x0, y0, x1, y1, x0, y1, c); }
static void epais(M5Canvas& g, float x0, float y0, float x1, float y1, float w, uint16_t c) {
  float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy); if (l < 0.01f) return;
  float nx = -dy / l * w * 0.5f, ny = dx / l * w * 0.5f;
  tri(g, x0 + nx, y0 + ny, x0 - nx, y0 - ny, x1 + nx, y1 + ny, c); tri(g, x0 - nx, y0 - ny, x1 - nx, y1 - ny, x1 + nx, y1 + ny, c);
}
static void ellO(M5Canvas& g, float x, float y, float rx, float ry, uint16_t c) {      // contour d'ellipse
  int a = ar(rx * zs), b = ar(ry * zs); if (a < 1) a = 1; if (b < 1) b = 1; g.drawEllipse(ar(TX(x)), ar(TY(y)), a, b, c);
}
static float roofY(float x) { float u = fabsf(x - HX) / 48.0f; return HB - 44.0f - 40.0f * (1.0f - (u > 1.0f ? 1.0f : u)); }
static float echelleX(float x) {      // perspective : les oiseaux rapetissent vers la maison, grossissent vers la mare
  if (x > 350.0f) return lerpf(1.0f, 0.72f, lim((x - 350.0f) / 60.0f, 0.0f, 1.0f));
  if (x < -30.0f) return lerpf(1.0f, 1.10f, lim((-30.0f - x) / 60.0f, 0.0f, 1.0f));
  return 1.0f;
}

// ---------- cuisson du ciel (même dessin que la vue normale, sans les arbres ni le sol) ----------
static void fixerVue(float x0, float y0) { zs = ZN; ztx = -x0 * ZN; zty = -y0 * ZN; }
static void bakeCiel() {
  VueId v_; zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs);
  M5Canvas& g = sCiel;
  for (int sy = 0; sy < H_SCENE; sy++) { float r, gg, b; cielRGB((sy - zty) / zs, r, gg, b); g.drawFastHLine(0, sy, 320, CF(r, gg, b)); }
  float mxs = LUNE_X * zs + ztx, mys = LUNE_Y * zs + zty; int Rr = (int)((LUNE_R + 58) * zs);
  for (int sy = (int)mys - Rr; sy <= (int)mys + Rr; sy++) {
    if (sy < 0 || sy >= H_SCENE) continue;
    for (int sx = (int)mxs - Rr; sx <= (int)mxs + Rr;) {
      float dx = sx - mxs, dy = sy - mys, dd = sqrtf(dx * dx + dy * dy);
      int step = dd < (LUNE_R + 3) * zs ? 1 : 2;
      if (sx >= 0 && sx < 320 - step && dd < Rr) {
        float r, gg, b; fondRGB((sx + (step - 1) * 0.5f - ztx) / zs, (sy - zty) / zs, r, gg, b);
        if (step == 1) g.drawPixel(sx, sy, CF(r, gg, b)); else g.drawFastHLine(sx, sy, step, CF(r, gg, b));
      }
      sx += step;
    }
  }
  uint32_t seed = 4242;
  for (int i = 0; i < ambEt * 2; i++) {
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sx = 2 + (int)((seed >> 8) % 316);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sy = 2 + (int)((seed >> 8) % 140);
    if (hypotf(sx - mxs, sy - mys) < (LUNE_R + 24) * zs) continue;
    int v = (int)((seed >> 4) % 100);
    float sk = (OP.hor == 2) ? 0.50f : ((OP.hor == 1) ? 0.75f : 1.0f); g.drawPixel(sx, sy, v < 70 ? CF(150 * sk, 62 * sk, 74 * sk) : CF(205 * sk, 112 * sk, 120 * sk));
  }
  brume(g, 110, 100, 100, 3, 0.35f); brume(g, 210, 128, 110, 3, 0.35f); brume(g, 90, 150, 100, 3, 0.35f);
  if (OP.hor) {
    for (int i = 0; i < (OP.hor == 2 ? 4 : 2); i++) {
      float oy = -9.0f + i * 7.0f + 1.5f * sinf(i * 2.1f), ox = 16.0f * sinf(i * 2.3f + 0.6f), rx = (30.0f + 9.0f * ((i * 5) % 3)) * zs, ry = (2.2f + 0.7f * (i % 3)) * zs;
      g.fillEllipse((int)(mxs + ox * zs), (int)(mys + oy * zs), (int)rx, (int)(ry + 0.5f), CF(15 + 4 * (i % 2), 1, 3));
      g.fillEllipse((int)(mxs + (ox + 14.0f) * zs), (int)(mys + (oy + 1.5f) * zs), (int)(rx * 0.5f), (int)(ry * 0.8f + 0.5f), CF(11, 0, 2));
    }
    brume(g, 70, 200, 150, 8, 0.85f); brume(g, 250, 204, 150, 7, 0.85f); brume(g, 160, 198, 120, 6, 0.80f);
  }
  float gy = solHaut(160.0f) * zs + zty;
  seed = 777;
  for (int i = 0; i < 2600; i++) {
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gx = (int)((seed >> 8) % 320);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gyy = (int)((seed >> 8) % (uint32_t)((int)gy - 6 > 1 ? (int)gy - 6 : 1));
    if (hypotf(gx - mxs, gyy - mys) < (LUNE_R + 30) * zs) continue;
    float r, gg, b; cielRGB((gyy - zty) / zs, r, gg, b); bool cl = (seed >> 3) & 1;
    g.drawPixel(gx, gyy, CF(r * (cl ? 1.12f : 0.88f) + (cl ? 7 : 0), gg * (cl ? 1.12f : 0.88f), b * (cl ? 1.12f : 0.88f)));
  }
}

// ---------- bande lointaine : arbres lointains tous différents, puis deux plans de collines ----------
static void bakeLoin() {
  VueId v_; fixerVue(MX0, FY0);
  M5Canvas& g = sFar; g.fillScreen(CLE_TR);
  static const float LT[][4] = {{282, 78, 0, 0.30f}, {334, 56, 3, 0.42f}, {238, 46, 2, 0.50f}, {8, 62, 4, 0.45f}, {-62, 72, 1, 0.50f}, {-30, 44, 3, 0.55f},
    {-98, 56, 0, 0.58f}, {-150, 70, 2, 0.50f}, {-205, 60, 4, 0.55f}, {372, 58, 4, 0.55f}, {500, 66, 1, 0.45f}, {548, 52, 3, 0.50f}};
  for (unsigned i = 0; i < sizeof(LT) / sizeof(LT[0]); i++) {
    float sx = LT[i][0] * zs + ztx, sy = 197.0f * zs + zty;
    arbreLoin(g, (int)LT[i][2], sx, sy, LT[i][1] * zs * 1.3f, LT[i][3]);
  }
  float gy = solHaut(160.0f) * zs + zty; int h = g.height();
  for (int x = 0; x < g.width(); x++) {
    float xs = (float)x + 160.0f + (MX0 - 160.0f) * ZN;      // colonne de l'écran quand la caméra est au repos : les collines coïncident avec la vue normale
    float yl1 = gy - 24.0f + 6.0f * sinf(xs / 40.0f + 1.0f) + 4.0f * sinf(xs / 17.0f); g.drawFastVLine(x, (int)yl1, h - (int)yl1, CF(66 * ambCol, 9 * ambCol, 15 * ambCol));
    float yl2 = gy - 12.0f + 5.0f * sinf(xs / 55.0f + 2.0f) + 3.0f * sinf(xs / 23.0f); g.drawFastVLine(x, (int)yl2, h - (int)yl2, CF(46 * ambCol, 6 * ambCol, 11 * ambCol));
  }
}

// ---------- bande proche : sol noir, mare, champ, chemin, maison, clôtures, poteaux, épouvantail ----------
static void dessinerMaison(M5Canvas& g) {
  const uint16_t mur = C(9, 3, 6), toit = C(6, 2, 4), liser = C(96, 16, 28), porte = C(14, 4, 7), fen = C(150, 92, 28), cad = C(5, 2, 4);
  rect(g, HX - 38, HB - 46, HX + 38, HB, mur);
  tri(g, HX - 48, HB - 44, HX + 48, HB - 44, HX, HB - 84, toit);
  rect(g, HX - 49, HB - 46, HX + 49, HB - 43, toit);
  ligne(g, HX - 48, HB - 45, HX, HB - 85, liser); ligne(g, HX, HB - 85, HX + 48, HB - 45, C(60, 10, 20));      // liseré de lune sur les pans du toit
  rect(g, HX + 21, HB - 82, HX + 31, HB - 58, toit); rect(g, HX + 20, HB - 84, HX + 32, HB - 81, toit); ligne(g, HX + 20, HB - 85, HX + 32, HB - 85, liser);
  rect(g, HX - 27, HB - 35, HX - 11, HB - 18, fen); rect(g, HX - 20, HB - 35, HX - 18, HB - 18, cad); rect(g, HX - 27, HB - 27.5f, HX - 11, HB - 25.5f, cad);      // fenêtre faiblement éclairée
  rect(g, HX + 8, HB - 28, HX + 20, HB, porte); disque(g, HX + 17.5f, HB - 13, 0.9f, C(120, 80, 24));
  rect(g, HX + 6, HB, HX + 22, HB + 1.5f, mur);
  disque(g, HX, HB - 62, 4.2f, C(60, 38, 12)); rect(g, HX - 0.8f, HB - 66.2f, HX + 0.8f, HB - 57.8f, cad); rect(g, HX - 4.2f, HB - 62.8f, HX + 4.2f, HB - 61.2f, cad);      // lucarne
}
static void cloture(M5Canvas& g, float xa, float xb, float h, uint16_t pc) {
  for (float x = xa; x <= xb + 0.1f; x += 14.0f) { float y = solHaut(x) - 0.5f; rect(g, x - 1.3f, y - h, x + 1.3f, y, pc); }
  float ya = solHaut(xa), yb = solHaut(xb);
  ligne(g, xa, ya - h * 0.8f, xb, yb - h * 0.8f, pc); ligne(g, xa, ya - h * 0.4f, xb, yb - h * 0.4f, pc);
}
static void poteau(M5Canvas& g, float x, float h, uint16_t pc) {
  float y = solHaut(x); epais(g, x, y, x, y - h, 2.4f, pc); rect(g, x - 8, y - h + 3, x + 8, y - h + 5.2f, pc); rect(g, x - 5, y - h + 10, x + 5, y - h + 11.6f, pc);
}
static void fil(M5Canvas& g, float x0, float y0, float x1, float y1, float aff, uint16_t pc) {
  float px = x0, py = y0;
  for (int i = 1; i <= 7; i++) { float t = i / 7.0f, x = lerpf(x0, x1, t), y = lerpf(y0, y1, t) + aff * 4.0f * t * (1 - t); ligne(g, px, py, x, y, pc); px = x; py = y; }
}
static void dessinerEpouvantail(M5Canvas& g, float x, uint16_t pc) {
  float y = solHaut(x);
  epais(g, x, y, x, y - 44, 2.6f, pc); epais(g, x - 17, y - 36, x + 17, y - 34, 2.2f, pc);      // poteau et bras
  tri(g, x - 6, y - 36, x + 6, y - 36, x, y - 20, pc); disque(g, x, y - 45, 4.6f, pc);
  tri(g, x - 8, y - 48, x + 8, y - 48, x, y - 58, pc); rect(g, x - 10, y - 49, x + 10, y - 47.4f, pc);      // chapeau
  epais(g, x - 17, y - 35, x - 21, y - 22, 1.3f, pc); epais(g, x + 17, y - 35, x + 21, y - 23, 1.3f, pc);      // chiffons qui pendent
}
static void bakeProche() {
  VueId v_; fixerVue(MX0, NY0);
  M5Canvas& g = sNear; g.fillScreen(CLE_TR); int h = g.height();
  for (int x = 0; x < g.width(); x++) { float ys = solHaut((x - ztx) / zs) * zs + zty; g.drawFastVLine(x, (int)ys, h - (int)ys, C(5, 2, 4)); }
  const uint16_t noir = C(4, 1, 3);
  // champ : sillons qui convergent vers l'horizon (perspective)
  { const float vx = (CH0 + CH1) * 0.5f, vy = 150.0f;
    for (int i = 0; i <= 8; i++) {
      float xb = CH0 - 26.0f + i * 20.0f, xt = vx + (xb - vx) * (203.0f - vy) / (256.0f - vy);
      ligne(g, xt, 203.0f, xb, 256.0f, C(20, 4, 9)); ligne(g, xt + 1.0f, 203.0f, xb + 2.5f, 256.0f, C(12, 3, 6));
    } }
  // chemin de terre vers la maison
  tri(g, HX + 6, HB + 1.5f, HX + 22, HB + 1.5f, HX + 70, 256, C(15, 4, 8)); tri(g, HX + 6, HB + 1.5f, HX + 70, 256, HX - 36, 256, C(15, 4, 8));
  // mare : eau sombre, reflet rougeâtre, roseaux
  ell(g, PX, PY, PRX + 2.5f, PRY + 1.4f, C(12, 4, 8)); ell(g, PX, PY, PRX, PRY, C(42, 8, 21)); ell(g, PX + 2, PY + 0.4f, PRX * 0.62f, PRY * 0.55f, C(62, 12, 30));
  for (int i = 0; i < 9; i++) {
    float rx0 = PX - PRX + 2.0f + i * 2.2f + (i > 4 ? PRX * 1.5f : 0.0f), hh = 12.0f + 7.0f * ((i * 5) % 4) * 0.5f, lean = ((i % 3) - 1) * 2.2f;
    epais(g, rx0, PY - 0.5f, rx0 + lean, PY - hh, 1.0f, noir); if (i % 2 == 0) ell(g, rx0 + lean, PY - hh, 1.0f, 2.6f, noir);
  }
  // maison, clôtures, poteaux et fils, épouvantail
  dessinerMaison(g);
  cloture(g, 360.0f, 392.0f, 13.0f, noir); cloture(g, 470.0f, 540.0f, 13.0f, noir);
  poteau(g, 366.0f, 56.0f, noir); poteau(g, 520.0f, 50.0f, noir);
  fil(g, 366 - 7, solHaut(366) - 53, 520 - 7, solHaut(520) - 47, 7.0f, noir); fil(g, 366 + 7, solHaut(366) - 53, 520 + 7, solHaut(520) - 47, 7.0f, noir);
  fil(g, 366 - 5, solHaut(366) - 46, 520 - 5, solHaut(520) - 40, 6.0f, noir);
  dessinerEpouvantail(g, (CH0 + CH1) * 0.5f, noir);
}
static void bakeArbre() {
  VueId v_; fixerVue(AX0, AY0);
  sArbre.fillScreen(CLE_TR); arbre(sArbre);
}
static void bakeVig() {
  if (ambVig <= 0.0f) { if (vigK) { free(vigK); vigK = nullptr; } return; }
  if (!vigK) vigK = (uint8_t*)ps_malloc((size_t)H_SCENE * 320);
  if (!vigK) return;
  for (int y = 0; y < H_SCENE; y++) {
    float dy = (y - 100.0f) / 106.0f;
    for (int x = 0; x < 320; x++) {
      float dx = (x - 160.0f) / 165.0f, d2 = dx * dx + dy * dy, f = 1.0f;
      if (d2 >= 0.35f) { float t = lim((d2 - 0.35f) / 1.15f, 0.0f, 1.0f); f = 1.0f - 0.93f * ambVig * t * t * (3.0f - 2.0f * t); }
      vigK[y * 320 + x] = (uint8_t)(f * 255.0f + 0.5f);
    }
  }
}
static void vigAppliquer() {      // bords noirs de l'ambiance Horreur, appliqués sur le paysage recomposé
  if (!vigK) return;
  uint8_t* b = (uint8_t*)cv.getBuffer(); if (!b) return;
  for (int y = 24; y < H_SCENE; y++) {
    const uint8_t* kk = vigK + y * 320; uint8_t* p = b + (size_t)y * 640;
    for (int x = 0; x < 320; x++, p += 2) {
      int k = kk[x]; if (k >= 254) continue;
      uint16_t v = vigFort ? (uint16_t)((p[0] << 8) | p[1]) : (uint16_t)((p[1] << 8) | p[0]);
      uint16_t r5 = (uint16_t)((((v >> 11) & 31) * k) >> 8), g6 = (uint16_t)((((v >> 5) & 63) * k) >> 8), b5 = (uint16_t)(((v & 31) * k) >> 8);
      v = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
      if (vigFort) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)(v & 255); } else { p[1] = (uint8_t)(v >> 8); p[0] = (uint8_t)(v & 255); }
    }
  }
}
static bool creer(M5Canvas& s, int w, int h) { s.setColorDepth(cv.getColorDepth()); s.setPsram(true); if (!s.createSprite(w, h)) return false; s.setPivot(0.0f, 0.0f); return true; }
static void libererSprites() {
  if (sCiel.getBuffer()) sCiel.deleteSprite(); if (sFar.getBuffer()) sFar.deleteSprite(); if (sNear.getBuffer()) sNear.deleteSprite(); if (sArbre.getBuffer()) sArbre.deleteSprite();
  if (vigK) { free(vigK); vigK = nullptr; }
  pret = false;
}
static void rebake() {      // appelé par construireFond : première fois = création des bandes, ensuite = redessin (changement d'ambiance)
  if (bpp != 2) return;
  if (!pret) {
    if (tentee) return;
    tentee = true;
    if (ESP.getFreePsram() < 1300000UL) return;
    if (!creer(sCiel, 320, H_SCENE) || !creer(sFar, WL, HF) || !creer(sNear, WL, HN) || !creer(sArbre, WA, HA)) { libererSprites(); return; }
    sFar.drawPixel(0, 0, C(248, 0, 0)); vigFort = (((const uint8_t*)sFar.getBuffer())[0] == 0xF8);      // ordre des octets : détecté par un essai
    pret = true;
  }
  bakeCiel(); bakeLoin(); bakeProche(); bakeArbre(); bakeVig();
}

// ---------- effets vivants : fumée, fenêtre, eau, ondes, gouttes ----------
struct Onde { bool on; float x, y, t; };
struct Goutte { bool on; float x, y, vx, vy, t; };
static Onde onde[4];
static Goutte gout[14];
static void spawnOnde(float x, float y) { for (int i = 0; i < 4; i++) if (!onde[i].on) { onde[i].on = true; onde[i].x = x; onde[i].y = y; onde[i].t = 0; return; } }
static void spawnGoutte(float x, float y, float vx, float vy) { for (int i = 0; i < 14; i++) if (!gout[i].on) { gout[i].on = true; gout[i].x = x; gout[i].y = y; gout[i].vx = vx; gout[i].vy = vy; gout[i].t = 0; return; } }
static bool vis(float x0, float x1) { return TX(x1) > -12.0f && TX(x0) < 332.0f; }
static void dyn() {
  M5Canvas& g = cv; const float t = tAnim; sec = false;
  if (vis(HX - 50, HX + 50)) {
    float f = 0.82f + 0.18f * sinf(t * 9.0f) * sinf(t * 2.3f + 1.0f); if (fmodf(t, 41.0f) < 0.22f) f = 0.12f;      // fenêtre qui vacille, parfois noire
    rect(g, HX - 27, HB - 35, HX - 11, HB - 18, CF(150 * f, 92 * f, 28 * f)); rect(g, HX - 20, HB - 35, HX - 18, HB - 18, C(5, 2, 4)); rect(g, HX - 27, HB - 27.5f, HX - 11, HB - 25.5f, C(5, 2, 4));
    for (int i = 0; i < 6; i++) {      // fumée de la cheminée
      float ph = fmodf(t * 0.16f + i / 6.0f, 1.0f), x = HX + 26.0f + 5.0f * sinf(ph * 6.0f + i) + ph * 14.0f, y = HB - 86.0f - ph * 38.0f;
      float r0, g0, b0; cielRGB(y, r0, g0, b0);
      disque(g, x, y, 1.6f + ph * 3.8f, CF(lerpf(22.0f, r0, ph * 0.8f), lerpf(5.0f, g0, ph * 0.8f), lerpf(9.0f, b0, ph * 0.8f)));
    }
  }
  if (vis(PX - PRX - 4, PX + PRX + 4)) {
    for (int i = 0; i < 4; i++) {      // reflets sur l'eau
      float a = 0.5f + 0.5f * sinf(t * 1.7f + i * 2.1f), x = PX + 12.0f * sinf(t * 0.6f + i * 1.9f) - 6.0f, y = PY + (i - 1.5f) * 2.0f, l = 5.0f + 5.0f * a;
      if (fabsf(x - PX) + l > PRX * (1.0f - fabsf(y - PY) / PRY * 0.5f)) continue;
      ligne(g, x - l, y, x + l, y, CF(40 + 40 * a, 8 + 6 * a, 20 + 12 * a));
    }
    for (int i = 0; i < 4; i++) if (onde[i].on) { float u = onde[i].t / 1.3f, r = 2.0f + 9.0f * u; ellO(g, onde[i].x, onde[i].y, r, r * 0.32f, CF(120 * (1 - u), 40 * (1 - u), 60 * (1 - u))); }
  }
  for (int i = 0; i < 14; i++) if (gout[i].on) disque(g, gout[i].x, gout[i].y, 0.9f, C(150, 70, 90));
}

// ---------- caméra ----------
static uint8_t cm = CM_NON; static bool cmAuto = false, cmFin = false;
static float cz = ZN, cxx = 160.0f, cyy = 128.0f, czv = 0.0f, cxv = 0.0f, cyv = 0.0f;
static float cmT = 0.0f, cmDur = 0.0f, gLarge = 60.0f, tPerdu = 0.0f, tRetour = 0.0f;
static int8_t suivi = -1;
static int8_t lpBird = -1; static uint32_t lpT = 0; static float lpX = 0, lpY = 0;
static const char* etiquette() { return cm == CM_LARGE ? "Large" : (cm == CM_SUIVI ? "Suivi" : "Vue"); }
static void majBouton() { boutonVue(fond); syncUI(); plein = true; }
static bool camOn() { return pret && cm != CM_NON; }
static void demarrer(uint8_t mode, bool autom) {
  cmFin = false; cmAuto = autom; cmT = 0.0f; tPerdu = 0.0f; tRetour = 0.0f; cmDur = autom ? rnd(11.0f, 16.0f) : 1e9f;
  if (cm == CM_NON) { cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f; }
  cm = mode; majBouton();
}
static void fermer() { if (cm != CM_NON) { cmFin = true; } }
static int choisirSuivi();
static void basculer() {
  if (!pret) return;
  if (cm == CM_NON || cmFin) demarrer(CM_LARGE, false);
  else if (cm == CM_LARGE) { int k = choisirSuivi(); if (k >= 0) { suivi = (int8_t)k; demarrer(CM_SUIVI, false); } else fermer(); }
  else fermer();
  majBouton();
}
static void demandeLarge(bool on) { if (!pret) return; if (on && cm == CM_NON) demarrer(CM_LARGE, false); else if (!on && cm != CM_NON) fermer(); }

// ---------- excursions : maison, mare, champ ----------
struct Rt { uint8_t dest, ph, spot, n, hops; float t, dur, tx, sB, tw; bool away; };
static Rt rt[NCB];
static int8_t rang[NAD];
static int8_t lien[NAD][NAD];
static float gTrip = 20.0f;
static int nbAilleurs() { int n = 0; for (int k = 0; k < NAD; k++) if (rt[k].away && cr[k].etat != E_DEHORS && cr[k].etat != E_OFF) n++; return n; }
static int spotsPris(int d, int sp) { for (int k = 0; k < NAD; k++) if (rt[k].away && rt[k].dest == d && rt[k].spot == sp) return 1; return 0; }
static void spotMaison(int dest, int sp, float& x, float& y, float& pente, int& d) {
  if (dest == D_EPOUV) { float cx = (CH0 + CH1) * 0.5f; x = cx + ((sp == 0) ? -13.0f : 13.0f); y = solHaut(cx) - 36.6f; pente = 0.0f; d = (sp == 0) ? -1 : 1; return; }      // sur les bras de l'épouvantail
  static const float OFS[4] = {-20.0f, 14.0f, 0.0f, 26.5f};
  x = HX + OFS[sp]; y = (sp == 3) ? HB - 84.0f : roofY(x); pente = (sp == 3 || sp == 2) ? 0.0f : ((x < HX) ? -0.85f : 0.85f); d = (sp == 1) ? 1 : -1;
}
static int choisirDest(int idx) {
  float w[D_NB] = {30.0f, 36.0f, 26.0f, 16.0f};
  if (rt[idx].away) w[rt[idx].dest] = 0.0f;
  bool m = false; for (int s = 0; s < 4; s++) if (!spotsPris(D_MAISON, s)) m = true; if (!m) w[D_MAISON] = 0.0f;
  if (spotsPris(D_MARE, 0) && spotsPris(D_MARE, 1)) w[D_MARE] = 0.0f;
  if (spotsPris(D_EPOUV, 0) && spotsPris(D_EPOUV, 1)) w[D_EPOUV] = 0.0f;
  w[D_DISPUTE] = 0.0f; float tot = w[0] + w[1] + w[2] + w[3]; if (tot <= 0.0f) return -1;
  float r = rnd01() * tot; for (int i = 0; i < D_NB; i++) { if (r < w[i]) return i; r -= w[i]; } return D_CHAMP;
}
static bool excursion(Corbeau& c, int idx, int dest) {
  Rt& r = rt[idx]; float tx = 0, ty = 0; int sp = 0;
  if (dest == D_MAISON) { do { sp = random(4); } while (spotsPris(D_MAISON, sp) && random(30) != 0); if (spotsPris(D_MAISON, sp)) return false; float pe; int dd; spotMaison(D_MAISON, sp, tx, ty, pe, dd); }
  else if (dest == D_MARE) { sp = spotsPris(D_MARE, 0) ? 1 : (spotsPris(D_MARE, 1) ? 0 : (int)random(2)); tx = (sp == 0) ? PX + PRX + 5.0f : PX - PRX - 5.0f; ty = solHaut(tx); }
  else if (dest == D_EPOUV) { sp = spotsPris(D_EPOUV, 0) ? 1 : (spotsPris(D_EPOUV, 1) ? 0 : (int)random(2)); float pe; int dd; spotMaison(D_EPOUV, sp, tx, ty, pe, dd); }
  else { sp = 0; tx = rnd(CH0 + 8.0f, CH1 - 8.0f); ty = solHaut(tx); }
  if (!r.away) { r.sB = c.s; r.hops = 0; }
  r.away = true; r.dest = (uint8_t)dest; r.spot = (uint8_t)sp; r.ph = 0; r.t = 0; r.tx = tx; r.n = 0; r.dur = 0; r.tw = 0;
  c.perch = -1; c.item = -1; c.rapporte = false;
  decoller(c, tx, ty, B_MONDE);
  return true;
}
static bool partir(Corbeau& c) {      // appelé par sortir() : une sortie devient une excursion vers la maison, la mare ou le champ
  if (!pret || c.fuite || c.rapporte || V.ph != V_RIEN || GP.ph || appelOn) return false;
  int idx = (int)(&c - cr); if (idx < 0 || idx >= NAD) return false;
  if (nbAilleurs() >= 2 || random(100) < 15) return false;
  int d = choisirDest(idx); if (d < 0) return false;
  return excursion(c, idx, d);
}
static void retour(Corbeau& c, int idx) {      // retour vers un perchoir libre de l'arbre
  Rt& r = rt[idx]; int i = -1;
  if (random(100) < 65) {      // il préfère se poser près d'un ami déjà perché sur l'arbre
    int ami = -1, best = 0; for (int k = 0; k < NAD; k++) if (k != idx && cr[k].etat == E_PERCHE && cr[k].perch >= 0 && cr[k].perch < NPE && lien[idx][k] > best) { best = lien[idx][k]; ami = k; }
    if (ami >= 0) { float bd = 1e9f; for (int pp = 0; pp < NPE; pp++) if (!perchOccupe(pp)) { float d = fabsf(PERCH[pp][0] - cr[ami].x); if (d < bd) { bd = d; i = pp; } } }
  }
  if (i < 0) i = perchPour(c);
  if (i < 0) { r.ph = 9; r.t = 0; return; }
  r.ph = 8; c.perch = i; c.item = -1; decoller(c, PERCH[i][0], perchY(i), B_PERCHOIR);
}
static void apres(Corbeau& c, int idx) {      // fin de la routine : une autre destination (parfois) ou retour à l'arbre
  Rt& r = rt[idx]; r.hops++;
  if (r.hops < 3 && random(100) < 30 && nbAilleurs() <= 2) { int d = choisirDest(idx); if (d >= 0 && excursion(c, idx, d)) return; }
  retour(c, idx);
}
static void arrivee(Corbeau& c) {      // fin de l'atterrissage sur un site de l'excursion
  int idx = (int)(&c - cr); if (idx < 0 || idx >= NAD) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; return; }
  Rt& r = rt[idx]; r.t = 0; r.ph = 0; r.n = 0; c.ox = c.oy = 0; c.t = 0; c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.5f, 3.5f); c.sub = 0; c.peck = 0; c.om = 0;
  if (r.dest == D_MAISON || r.dest == D_EPOUV) {
    float x, y, pe; int d; spotMaison(r.dest, r.spot, x, y, pe, d); c.fx = x; c.fy = y; c.fp = pe; c.d = (int8_t)d;
    c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); r.dur = rnd(11.0f, 19.0f);
  } else {
    c.etat = E_SOL; r.dur = 0;
    c.d = (r.dest == D_MARE) ? ((r.spot == 0) ? -1 : 1) : ((random(2)) ? 1 : -1);
    if (r.dest == D_DISPUTE) c.d = (ob[r.n % NOB].x >= c.x) ? 1 : -1;
  }
}
static void dispute(Corbeau& c, int idx, float dt);
static void sol(Corbeau& c, int idx, float dt) {      // au sol : boire à la mare, picorer dans le champ (appelé à chaque image par majCorbeau)
  Rt& r = rt[idx]; r.t += dt;
  if (r.ph == 9) { if (r.t > 2.0f) retour(c, idx); return; }
  if (r.dest == D_DISPUTE) { dispute(c, idx, dt); return; }
  if (r.dest == D_MARE) {
    if (r.ph == 0) { if (r.t > 1.1f) { r.ph = 1; r.t = 0; r.n = (uint8_t)(3 + random(3)); r.tw = 0; } return; }
    if (r.ph == 1) {      // boire : la tête plonge vers l'eau, remonte et avale la tête en arrière
      const float cyc = 1.9f; float u = r.t / cyc;
      float dip = pas((u - 0.08f) / 0.22f) * (1.0f - pas((u - 0.38f) / 0.22f)), up = pas((u - 0.50f) / 0.12f) * (1.0f - pas((u - 0.80f) / 0.12f));
      c.haC = lerpf(-0.30f, 0.88f, dip) - 0.55f * up; c.hbC = lerpf(0.20f, 1.08f, dip) - 0.65f * up; c.pitchC = c.pr * 0.45f - 0.42f * dip + 0.10f * up;
      if (up > 0.3f) c.bec = mxf(c.bec, 0.5f);
      if (dip > 0.92f && r.tw < 0.5f) { r.tw = 1.0f; spawnOnde(c.x + c.d * 17.0f * c.s, PY + 0.5f); son(SN_GAZ, 70); }
      if (dip < 0.5f) r.tw = 0.0f;
      if (r.t >= cyc) { r.t -= cyc; if (r.n > 0) r.n--; if (r.n == 0) { r.t = 0; r.ph = (random(100) < 45) ? 2 : 3; } }
      return;
    }
    if (r.ph == 2) {      // il se baigne : ailes qui battent dans l'eau, éclaboussures
      float fl = sinf(r.t * 24.0f); c.pitchC = 0.05f; c.bob = 1.4f * c.s + 0.6f * fl; c.wlC = 0.55f + 0.2f * fl; c.poseC = mixA(A_PLANE, A_HAUT, 0.5f + 0.5f * fl); c.haC = -0.2f; c.hbC = 0.3f;
      if (((int)(r.t * 9.0f)) != ((int)((r.t - dt) * 9.0f))) { spawnGoutte(c.x + rnd(-6, 6), c.y + 2.0f * c.s, rnd(-30, 30), rnd(-70, -35)); spawnOnde(c.x + rnd(-8, 8), PY + rnd(-1, 1)); }
      if (r.t > 2.8f) { r.ph = 3; r.t = 0; c.wlC = 0; }
      return;
    }
    if (r.ph == 3) {      // il se secoue
      c.ruf = sinf(3.1416f * lim(r.t / 1.2f, 0.0f, 1.0f)); c.pitchC = c.pr * 0.45f + 0.06f * c.ruf * sinf(r.t * 38.0f);
      if (((int)(r.t * 14.0f)) != ((int)((r.t - dt) * 14.0f))) spawnGoutte(c.x + rnd(-8, 8), c.y - 2.0f * c.s, rnd(-40, 40), rnd(-50, -20));
      if (r.t > 1.2f) apres(c, idx);
      return;
    }
  } else {      // champ : marche, picore, trois fois quatre à six fois
    if (r.ph == 0) { if (r.t > 0.9f) { r.ph = 1; r.t = 0; r.n = (uint8_t)(4 + random(3)); c.sub = 0; } return; }
    if (c.sub == 0) { c.tx = lim(c.x + rnd(-40.0f, 40.0f), CH0 + 6.0f, CH1 - 6.0f); c.sub = 1; c.d = (c.tx >= c.x) ? 1 : -1; }
    else if (c.sub == 1) {
      float dxm = c.tx - c.x;
      if (fabsf(dxm) < 1.8f) { c.sub = 2; c.peck = 0; }
      else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 36.0f * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f)); c.hop = -fabsf(sinf(c.t * 10.0f)) * 2.4f * c.s; c.bob = c.hop; c.pitchC = c.pr * 0.55f; c.haC = c.pitchC - 0.2f; c.hbC = c.haC + 0.5f; }
    } else {
      c.peck += dt; float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f)); c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
      if (c.peck > 1.26f) { if (r.n > 0) r.n--; if (r.n == 0) apres(c, idx); else c.sub = 0; }
    }
  }
}

// ---------- vie sociale : rangs, liens d'amitié ou de rivalité (mémorisés pendant la session), interactions ----------
// rang[k] : 0 = le plus dominant ; lien[a][b] : de -3 (rivaux) à +3 (amis). Le groupe plus complet (rôles, mémoire NVS) peut s'y brancher : rangDe() et lienDe().
static int rangDe(int k) { return (k >= 0 && k < NAD) ? rang[k] : 0; }
static int lienDe(int a, int b) { return (a >= 0 && b >= 0 && a < NAD && b < NAD) ? lien[a][b] : 0; }
static void lienAj(int a, int b, int d) {
  int v = lien[a][b] + d; if (v > 3) v = 3; if (v < -3) v = -3; lien[a][b] = lien[b][a] = (int8_t)v;
}
enum { S_CHAM = 0, S_TOIL, S_PARADE, S_BAGARRE, S_APPELS, S_NB };
struct Soc { bool on; uint8_t type, step; int8_t a, b, c3; };
static Soc S; static float tS = 0.0f, gSoc = 8.0f;
static bool libreSoc(int k) {
  Corbeau& c = cr[k];
  return c.visible && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && !rt[k].away;
}
static void vers(Corbeau& c, const Corbeau& o) { c.d = (o.x >= c.x) ? 1 : -1; }
static void figer(Corbeau& c, float dur) { c.act = A_IMMO; c.tAct = 0.0f; c.dAct = dur; c.hum = 1.0f; }
static bool choisirPaire(int& a, int& b) {
  int l[NAD], n = 0; for (int k = 0; k < NAD; k++) if (libreSoc(k)) l[n++] = k;
  if (n < 2) return false;
  a = l[random(n)]; int best = -1; float bd = 1e9f;
  for (int i = 0; i < n; i++) { int k = l[i]; if (k == a) continue; float d = fabsf(cr[k].x - cr[a].x) + rnd(0.0f, 30.0f); if (d < bd) { bd = d; best = k; } }
  if (best < 0 || bd > 150.0f) return false;
  b = best; return true;
}
static void debutSoc() {
  int a, b; if (!choisirPaire(a, b)) return;
  if (rang[a] > rang[b]) { int t = a; a = b; b = t; }      // a = le plus dominant des deux
  int li = lien[a][b]; float amis = (li > 0) ? (float)li : 0.0f, rivaux = (li < 0) ? (float)(-li) : 0.0f;
  float w[S_NB] = {12.0f + 10.0f * rivaux, 8.0f + 10.0f * amis, 6.0f + 6.0f * amis, 3.0f + 7.0f * rivaux, 7.0f};
  int c3 = -1; { int n = 0, l[NAD]; for (int k = 0; k < NAD; k++) if (k != a && k != b && libreSoc(k)) l[n++] = k; if (n) c3 = l[random(n)]; }
  if (c3 < 0) w[S_APPELS] = 0.0f;
  float tot = 0.0f; for (int i = 0; i < S_NB; i++) tot += w[i];
  float r = rnd01() * tot; int ty = 0; for (; ty < S_NB - 1; ty++) { if (r < w[ty]) break; r -= w[ty]; }
  memset(&S, 0, sizeof(S)); S.on = true; S.type = (uint8_t)ty; S.a = (int8_t)a; S.b = (int8_t)b; S.c3 = (int8_t)c3; tS = 0.0f;
}
static void majSoc(float dt) {
  gSoc -= dt;
  if (!S.on) {
    if (gSoc <= 0.0f) {
      gSoc = rnd(4.5f, 9.5f) / actFreq;
      if (V.ph == V_RIEN && !GP.ph && !appelOn && !panneau && tAlerte <= 0.0f && AP.n == 0) {
        if (random(100) < 22) { int r = random(3); if (r == 0) chamaille(); else if (r == 1) volGroupe(); else appelMutuel(); }
        else debutSoc();
      }
    }
    return;
  }
  Corbeau& A = cr[S.a]; Corbeau& B = cr[S.b];
  bool ok = A.etat == E_PERCHE && B.etat == E_PERCHE && A.perch >= 0 && B.perch >= 0 && V.ph == V_RIEN && !appelOn && tAlerte <= 0.0f && !GP.ph;
  if (S.type == S_BAGARRE && S.step >= 3) ok = (V.ph == V_RIEN);
  if (!ok) { S.on = false; return; }
  tS += dt; const float T = tS;
  switch (S.type) {
    case S_CHAM:      // le dominant fait face au dominé, crie, le bouscule ; le dominé cède sa place
      if (S.step == 0) { vers(A, B); vers(B, A); A.act = A_APPEL; A.tAct = 0.0f; A.dAct = 1.3f; figer(B, 4.0f); A.hum = 1.0f; S.step = 1; }
      else if (S.step == 1 && T > 1.3f) { A.surs = 0.32f; A.bec = 1.0f; son(SN_CROASSE1, 190); B.surs = 0.4f; B.flash = 0.6f; plumes(B.x, B.y - 2.0f * B.s, 3); vib(25); S.step = 2; }
      else if (S.step == 2 && T > 1.9f) {
        A.act = A_SECOUE; A.tAct = 0.0f; A.dAct = 0.9f; lienAj(S.a, S.b, -1); S.step = 3;
        if (random(100) < 65) versPerchoir(B);
      } else if (S.step == 3 && T > 3.1f) S.on = false;
      break;
    case S_TOIL:      // toilettage mutuel : l'un lisse la tête de l'autre, qui ferme les yeux
      if (S.step == 0) { vers(A, B); vers(B, A); figer(A, 5.5f); figer(B, 5.5f); S.step = 1; }
      A.look = A.ha0 + 0.12f + 0.20f * sinf(T * 8.0f); A.tLook = 0.3f; B.look = B.ha0 + 0.30f; B.tLook = 0.3f; B.clin = 1.0f;
      if (((int)(T * 3.0f)) != ((int)((T - dt) * 3.0f))) A.bec = 0.45f;
      if (T > 4.8f) { lienAj(S.a, S.b, 1); S.on = false; }
      break;
    case S_PARADE:      // parade : ailes ouvertes, petits sauts devant l'autre, qui répond
      if (S.step == 0) { vers(A, B); vers(B, A); A.act = A_ETIRE; A.tAct = 0.0f; A.dAct = 3.6f; A.hum = 1.0f; figer(B, 4.0f); S.step = 1; }
      A.d = (B.x >= A.x) ? 1 : -1; B.look = B.ha0 - 0.1f; B.tLook = 0.3f;
      if (S.step <= 4 && T > 0.7f + 0.8f * (S.step - 1)) { A.surs = 0.3f; A.bec = 0.6f; S.step++; }
      if (T > 3.9f) { B.bec = 1.0f; sonCri(); lienAj(S.a, S.b, 1); S.on = false; }
      break;
    case S_BAGARRE:      // brève bagarre : ailes ouvertes, cris, choc, plumes ; le perdant s'enfuit
      if (S.step == 0) { vers(A, B); vers(B, A); A.act = B.act = A_ETIRE; A.tAct = B.tAct = 0.0f; A.dAct = B.dAct = 1.8f; A.bec = B.bec = 1.0f; A.hum = B.hum = 1.0f; son(SN_CROASSE2, 200); sonCri(); S.step = 1; }
      else if (S.step == 1 && T > 0.9f) { choc((A.x + B.x) * 0.5f, (A.y + B.y) * 0.5f - 2.0f, &A, &B, 150.0f); plumes((A.x + B.x) * 0.5f, (A.y + B.y) * 0.5f, 6); A.surs = B.surs = 0.35f; S.step = 2; }
      else if (S.step == 2 && T > 1.5f) {
        bool aGagne = (rang[S.a] < rang[S.b]) ? (random(100) < 75) : (random(100) < 35);
        Corbeau& Gw = aGagne ? A : B; Corbeau& Lz = aGagne ? B : A;
        Gw.act = A_SECOUE; Gw.tAct = 0.0f; Gw.dAct = 0.9f; sonCri(); lienAj(S.a, S.b, -2); S.step = 3; chasser(Lz);
      } else if (S.step == 3 && T > 2.5f) S.on = false;
      break;
    default: {      // appels à trois : chacun répond à tour de rôle
      Corbeau& C3 = cr[S.c3 < 0 ? S.a : S.c3];
      if (S.step == 0) { vers(A, B); A.act = A_APPEL; A.tAct = 0.0f; A.dAct = 1.2f; A.hum = 1.0f; S.step = 1; }
      else if (S.step == 1 && T > 0.9f) { vers(B, A); B.act = A_APPEL; B.tAct = 0.0f; B.dAct = 1.2f; B.hum = 1.0f; S.step = 2; }
      else if (S.step == 2 && T > 1.8f) { vers(C3, B); C3.act = A_APPEL; C3.tAct = 0.0f; C3.dAct = 1.2f; C3.hum = 1.0f; S.step = 3; }
      else if (S.step == 3 && T > 3.3f) { lienAj(S.a, S.b, 1); S.on = false; }
    } break;
  }
}

// ---------- nourriture : effets sur chaque corbeau (énergie, humeur, force) et compétition pour un gros morceau ----------
static float en[NAD], tHer[NAD], tJoie[NAD], sBase[NAD]; static int8_t forceK[NAD];
static void miettes(Corbeau& c) {      // le pain se partage : deux miettes tombent près de lui
  for (int j = 0; j < 2; j++) {
    int i = -1; for (int q = 0; q < NOB; q++) if (!ob[q].actif) { i = q; break; } if (i < 0) return;
    Obj& o = ob[i]; o.actif = true; o.type = T_GRAINE; o.x = lim(c.x + (j ? 14.0f : -14.0f) * c.s, 12.0f, 308.0f); o.y = solHaut(o.x) - 3.2f; o.vy = 0.0f; o.t = 0.0f; o.repos = true; o.reserve = -1;
  }
}
static void mange(Corbeau& c, int idx, int type) {      // chaque nourriture a un effet réel et visible
  if (idx < 0 || idx >= NAD) return;
  switch (type) {
    case T_GRAINE: en[idx] += 0.10f; break;
    case T_VIANDE: en[idx] += 0.45f; tHer[idx] = 25.0f; if (forceK[idx] < 3) forceK[idx]++; c.flash = 0.6f; break;      // beaucoup d'énergie : plus gros, plus fort
    case T_NOIX: en[idx] += 0.20f; son(SN_CROC, 235); c.surs = 0.22f; break;      // cassée au bec
    case T_PAIN: en[idx] += 0.30f; miettes(c); break;
    case T_BAIES: en[idx] += 0.12f; tJoie[idx] = 20.0f; c.hum = 1.0f; break;      // la joie : il chante et sautille
    default: break;
  }
  if (en[idx] > 1.0f) en[idx] = 1.0f;
}
static bool peutPrendre(int iO) {      // un gros morceau n'est pas pris tant qu'un rival n'est pas venu le disputer
  for (int k = 0; k < NAD; k++) if (rt[k].away && rt[k].dest == D_DISPUTE && rt[k].n == iO && rt[k].ph < 2 && cr[k].etat != E_DEHORS) return false;
  return true;
}
static void compet(int iO) {      // appelé après qu'un corbeau est parti chercher la nourriture : un seul gros morceau, un rival vient le lui disputer
  Obj& o = ob[iO]; if (!o.actif) return;
  int W = o.reserve; if (W < 0 || W >= NAD) return;
  bool gros = (o.type == T_VIANDE || o.type == T_NOIX || o.type == T_PAIN || o.type == T_BRILLANT);
  int nOb = 0; for (int i = 0; i < NOB; i++) if (ob[i].actif) nOb++;
  if (!gros || nOb > 1 || !pret || nbAilleurs() >= 3 || random(100) < 25) return;      // plusieurs morceaux : chacun le sien, sans dispute
  int best = -1; float bd = 1e9f;
  for (int k = 0; k < NAD; k++) { if (k == W || !libreSoc(k)) continue; float d = fabsf(cr[k].x - o.x) + rnd(0.0f, 40.0f) - 8.0f * (forceK[k] + (NAD - rang[k]) * 0.5f); if (d < bd) { bd = d; best = k; } }
  if (best < 0) return;
  Corbeau& c = cr[best]; Rt& r = rt[best];
  int sideW = (cr[W].x < o.x) ? -1 : 1; float tx = lim(o.x - sideW * 24.0f * c.s, 12.0f, 308.0f), ty = solHaut(tx);
  if (!r.away) { r.sB = c.s; r.hops = 0; }
  r.away = true; r.dest = D_DISPUTE; r.spot = 0; r.ph = 0; r.t = 0.0f; r.tx = tx; r.n = (uint8_t)iO; r.dur = 0.0f; r.tw = 0.0f;
  c.perch = -1; c.item = -1; c.rapporte = false; decoller(c, tx, ty, B_MONDE);
}
static void dispute(Corbeau& c, int idx, float dt) {      // au sol, face au détenteur du morceau : ailes ouvertes, cris, choc, puis le plus fort l'emporte
  Rt& r = rt[idx]; int iO = r.n % NOB; Obj& o = ob[iO]; int W = o.reserve;
  bool vivant = o.actif && W >= 0 && W < NAD && cr[W].but == B_NOUR && cr[W].etat == E_SOL && cr[W].item == iO;
  if (r.ph < 2 && !vivant) { retour(c, idx); return; }
  if (r.ph == 3) { retour(c, idx); return; }
  c.d = (o.x >= c.x) ? 1 : -1;
  if (r.ph == 0) {
    c.wlC = 0.8f; c.poseC = mixA(A_PLANE, A_HAUT, 0.5f); c.pitchC = c.pr * 0.45f - 0.15f; c.bec = mxf(c.bec, 0.4f);
    float dxm = o.x - c.x, ec = 24.0f * c.s;      // il s'approche jusqu'à portée de bec
    if (fabsf(dxm) > ec) c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm) - ec, 34.0f * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f));
    if (r.t > 1.2f) { r.ph = 1; r.t = 0.0f; }
  } else if (r.ph == 1) {
    choc((c.x + cr[W].x) * 0.5f, c.y - 2.0f, &c, &cr[W], 160.0f); plumes((c.x + cr[W].x) * 0.5f, c.y, 6); son(SN_CROASSE2, 210); sonCri();
    c.surs = 0.35f; cr[W].surs = 0.35f; cr[W].flash = 0.6f; c.flash = 0.6f;
    float sc = (float)(NAD - rang[idx]) + 2.0f * forceK[idx] + (1.0f - en[idx]) * 2.0f + rnd(0.0f, 3.0f);
    float sw = (float)(NAD - rang[W]) + 2.0f * forceK[W] + (1.0f - en[W]) * 2.0f + rnd(0.0f, 3.0f);
    r.tw = (sc > sw) ? 1.0f : 0.0f; r.ph = 2; r.t = 0.0f;
  } else {
    c.wlC = (r.tw > 0.5f) ? 0.5f : 0.0f; c.poseC = mixA(A_PLANE, A_HAUT, 0.4f);
    if (r.t > 0.8f) {
      if (r.tw > 0.5f && o.actif) {      // il lui prend le morceau : l'autre recule et s'en va
        o.reserve = idx; c.item = iO; c.but = B_NOUR; c.sub = 0; c.peck = 0.0f; lienAj(idx, W, -1);
        Corbeau& w = cr[W]; w.item = -1; w.but = B_RIEN; w.sub = 2; w.peck = 0.0f; w.t = 0.0f; w.surs = 0.3f;
        r.away = false; c.s = r.sB; r.ph = 0;
      } else { lienAj(idx, W, -1); retour(c, idx); }
    }
  }
}

// ---------- suivi d'un oiseau ----------
static int choisirSuivi() {
  int l[NAD], n = 0;
  for (int k = 0; k < NAD; k++) if (rt[k].away && cr[k].visible && cr[k].etat != E_DEHORS && cr[k].etat != E_POOF && cr[k].etat != E_OFF) l[n++] = k;
  if (n) return l[random(n)];
  for (int k = 0; k < NAD; k++) if (cr[k].visible && cr[k].etat == E_PERCHE && cr[k].perch >= 0 && cr[k].perch < 10) l[n++] = k;
  return n ? l[random(n)] : -1;
}
static void suivre(int k) { if (!pret || k < 0 || k >= NAD) return; suivi = (int8_t)k; demarrer(CM_SUIVI, false); }
static void armer(int b) { lpBird = (int8_t)b; lpT = millis(); }
static void appuiLong(const m5::touch_detail_t& t) {      // toucher long sur un oiseau : on le suit
  if (lpBird < 0) return;
  if (t.wasReleased()) { lpBird = -1; return; }
  if (t.isPressed() && millis() - lpT > 700) { int b = lpBird; lpBird = -1; if (b < NAD) suivre(b); }
}
static void bornes(float z, float& x, float& y) {
  float hw = 160.0f / z + 4.0f, a = MX0 + hw, b = MX1 - hw; if (a > b) { a = b = (MX0 + MX1) * 0.5f; }
  x = lim(x, a, b); y = lim(y, 40.0f, 200.0f);
}
static bool camPilote(float dt) {
  if (!pret) return false;
  const bool bloque = GP.ph || V.ph != V_RIEN || H.actif || appelOn;
  if (cm == CM_NON) {
    gLarge -= dt * actFreq;
    if (gLarge <= 0.0f) { gLarge = rnd(50.0f, 90.0f); if (!bloque && !panneau && !cbAbsent && millis() - tToucher > 5000UL) demarrer(CM_LARGE, true); }
    if (cm == CM_NON) return false;
  }
  cmT += dt;
  { static int lent = 0;      // si l'image prend trop de temps (vue recomposée), on referme la caméra et on arrête les vues spontanées
    if (msDessin > 80) { if (++lent > 20 && cmT > 2.0f && !cmFin) { fermer(); gLarge = 1e8f; lent = 0; } } else if (lent > 0) lent--; }
  if (bloque && !cmFin) fermer();
  float tz = ZN, tx = 160.0f, ty = 128.0f;
  if (!cmFin) {
    if (cm == CM_LARGE) {
      tz = 0.5f; tx = 190.0f + 28.0f * sinf(cmT * 0.28f); ty = 118.0f;
      if (cmAuto && cmT > cmDur) fermer();
    } else {
      Corbeau& c = cr[suivi < 0 ? 0 : suivi]; Rt& r = rt[suivi < 0 ? 0 : suivi];
      bool ok = (suivi >= 0) && c.visible && c.etat != E_DEHORS && c.etat != E_POOF && c.etat != E_OFF;
      if (!ok) { tPerdu += dt; if (tPerdu > 1.5f) fermer(); } else tPerdu = 0.0f;
      if (ok) {
        switch (c.etat) {
          case E_SOL: tz = 1.35f; break;
          case E_PERCHE: tz = (c.but == B_MONDE) ? 1.18f : 1.05f; break;
          case E_VOL: case E_DECOLLE: tz = 0.88f; break;
          default: tz = 1.1f; break;
        }
        if (S.on && (S.a == suivi || S.b == suivi)) tz = 1.3f;      // une interaction entre oiseaux : on se rapproche
        tx = c.x + c.vx * 0.30f; ty = c.y + c.bob - 8.0f; if (c.etat == E_SOL) ty = c.y - 12.0f;
        if (!r.away && c.etat == E_PERCHE && c.perch >= 0) {      // encore (ou de nouveau) sur l'arbre
          tRetour += dt;
          if (cmT > 2.6f && tRetour > 2.6f && cmT < 8.0f && nbAilleurs() < 3) { int d = choisirDest(suivi); if (d >= 0) excursion(c, suivi, d); }      // il part en excursion : la caméra le suit
          if (tRetour > 4.5f && cmT > 8.0f) fermer();      // revenu à l'arbre : le suivi s'arrête
        } else tRetour = 0.0f;
      }
    }
  }
  bornes(tz, tx, ty);
  ressort(cz, czv, tz, 3.6f, dt); ressort(cxx, cxv, tx, 3.4f, dt); ressort(cyy, cyv, ty, 3.4f, dt);
  if (cmFin && fabsf(cz - ZN) < 0.003f && fabsf(cxx - 160.0f) < 0.6f && fabsf(cyy - 128.0f) < 0.6f) {
    cm = CM_NON; cmFin = false; suivi = -1; cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f;
    zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); majBouton(); return false;
  }
  zs = cz; ztx = 160.0f - cxx * cz; zty = 128.0f - cyy * cz;
  return true;
}
static void afficher(M5Canvas& s, int w, int h, float ox, float oy, float f) {
  if (ox >= 320.0f || oy >= (float)H_SCENE || ox + w * f <= 0.0f || oy + h * f <= 24.0f) return;
  if (fabsf(f - 1.0f) < 0.004f) s.pushSprite(&cv, (int)lroundf(ox), (int)lroundf(oy), CLE_TR);
  else s.pushRotateZoom(&cv, ox, oy, 0.0f, f, f, CLE_TR);
}
static void composer() {      // le paysage vu par la caméra : ciel fixe, plan lointain (parallaxe), plan proche, arbre, effets
  const float f = cz / ZN;
  memcpy(cv.getBuffer(), sCiel.getBuffer(), (size_t)H_SCENE * 320 * 2);
  { float zl = ZN + (cz - ZN) * 0.45f, cxl = 160.0f + (cxx - 160.0f) * 0.55f, cyl = 128.0f + (cyy - 128.0f) * 0.70f;
    afficher(sFar, WL, HF, 160.0f + (MX0 - cxl) * zl, 128.0f + (FY0 - cyl) * zl, zl / ZN);
    int yfb = (int)(128.0f + (FY1 - cyl) * zl) - 1;      // sous les collines lointaines : la lande, jusqu'au sol noir de la bande proche
    if (yfb < H_SCENE) cv.fillRect(0, yfb, 320, H_SCENE - yfb, CF(46 * ambCol, 6 * ambCol, 11 * ambCol)); }
  afficher(sNear, WL, HN, ztx + MX0 * zs, zty + NY0 * zs, f);
  { int yb = (int)floorf(zty + NY1 * zs) - 1; if (yb < H_SCENE) cv.fillRect(0, yb, 320, H_SCENE - yb, C(5, 2, 4)); }
  afficher(sArbre, WA, HA, ztx + AX0 * zs, zty + AY0 * zs, f);
  dyn();
  vigAppliquer();
  memcpy(cv.getBuffer(), fond.getBuffer(), (size_t)24 * 320 * 2);
}

// ---------- mise à jour générale (appelée par majGlobal) ----------
static void maj(float dt) {
  if (!pret) return;
  tMonde += dt;
  for (int i = 0; i < 4; i++) if (onde[i].on) { onde[i].t += dt; if (onde[i].t > 1.3f) onde[i].on = false; }
  for (int i = 0; i < 14; i++) if (gout[i].on) { Goutte& q = gout[i]; q.t += dt; q.vy += 260.0f * dt; q.x += q.vx * dt; q.y += q.vy * dt; if (q.t > 1.2f || q.y > PY + 2.0f) q.on = false; }
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; Rt& r = rt[k]; if (!r.away) continue;
    if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_OFF || c.etat == E_TENU || c.etat == E_TIR) { r.away = false; c.s = r.sB; continue; }
    if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but != B_MONDE) { r.away = false; c.s = r.sB; continue; }      // de retour sur l'arbre
    float tgt = r.sB * echelleX(c.x); c.s += (tgt - c.s) * mnf(1.0f, dt * 3.0f);      // perspective : plus petit vers la maison, plus grand vers la mare
    if (c.etat == E_PERCHE && c.but == B_MONDE) {      // sur le toit : il observe, puis repart
      r.t += dt;
      if (r.ph == 0 && r.t > r.dur) apres(c, k);
      else if (r.ph == 9 && r.t > 2.0f) retour(c, k);
      else if (r.ph == 0 && r.t > r.dur * 0.5f && r.n == 0) { r.n = 1; c.bec = 1; sonCri(); }
    }
  }
  majSoc(dt);
  for (int k = 0; k < NAD; k++) {      // énergie qui baisse, gros après la viande, joie des baies, sieste quand il est rassasié
    Corbeau& c = cr[k]; en[k] = mxf(0.0f, en[k] - 0.004f * dt); if (tHer[k] > 0.0f) tHer[k] -= dt; if (tJoie[k] > 0.0f) tJoie[k] -= dt;
    if (!rt[k].away && c.etat != E_DEHORS && c.etat != E_OFF) { float tg = sBase[k] * (1.0f + 0.07f * lim(tHer[k] / 3.0f, 0.0f, 1.0f)); if (fabsf(c.s - tg) > 0.002f) c.s += (tg - c.s) * mnf(1.0f, dt * 2.0f); }
    if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN) {
      if (tJoie[k] > 0.0f && random(1000) < 6) { sonCri(); c.surs = 0.25f; c.bec = 1.0f; }
      if (en[k] > 0.85f && c.act <= A_REGARDE && random(1000) < 5) { c.act = A_DORT; c.tAct = 0.0f; c.dAct = rnd(9.0f, 15.0f); en[k] = 0.7f; }
    }
    if (rt[k].away && rt[k].dest == D_DISPUTE && rt[k].ph == 0 && c.etat != E_SOL) { rt[k].tw += dt; if (rt[k].tw > 6.0f) rt[k].ph = 3; }
  }
  gTrip -= dt;
  if (gTrip <= 0.0f) {      // de temps en temps, un corbeau de l'arbre part vivre sa vie ailleurs
    gTrip = rnd(16.0f, 32.0f) / actFreq;
    if (V.ph == V_RIEN && !GP.ph && !appelOn && !panneau && nbAilleurs() < 2) {
      int l[NAD], n = 0;
      for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (!rt[k].away && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f) l[n++] = k; }
      if (n) { int k = l[random(n)], d = choisirDest(k); if (d >= 0) excursion(cr[k], k, d); }
    }
  }
}
static void reinit() {
  memset(rt, 0, sizeof(rt)); memset(onde, 0, sizeof(onde)); memset(gout, 0, sizeof(gout)); memset(&S, 0, sizeof(S)); gSoc = rnd(5.0f, 9.0f);
  for (int k = 0; k < NAD; k++) { en[k] = 0.4f; tHer[k] = tJoie[k] = 0.0f; forceK[k] = 0; sBase[k] = cr[k].s; }
  for (int k = 0; k < NAD; k++) rang[k] = (int8_t)k;
  for (int k = NAD - 1; k > 0; k--) { int j = random(k + 1); int8_t q = rang[k]; rang[k] = rang[j]; rang[j] = q; }
  for (int i = 0; i < NAD; i++) for (int j = i; j < NAD; j++) lien[i][j] = lien[j][i] = (i == j) ? 0 : (int8_t)(random(5) - 2);
  cm = CM_NON; cmFin = false; suivi = -1; lpBird = -1; cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f;
  gLarge = rnd(40.0f, 70.0f); gTrip = rnd(10.0f, 20.0f); tMonde = 0.0f;
}
static void fermerMonde() { libererSprites(); tentee = false; cm = CM_NON; cmFin = false; suivi = -1; memset(rt, 0, sizeof(rt)); }

}  // namespace mo
