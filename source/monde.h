#pragma once
// ======================= MONDE ÉLARGI, CAMÉRA ET VIE DES CORBEAUX (monde.h) =======================
// Inclus par corbeau.h, dans l'espace de noms cb, après les définitions dont il a besoin.
// Le paysage (ciel, collines et arbres lointains, sol, vieille tour en ruine, portique, stèles, mare, champ, épouvantail, arbre) est
// dessiné une seule fois en quatre bandes de pixels ; la caméra (vue d'ensemble, suivi d'un oiseau) les recompose à chaque image avec de la parallaxe.
// Les corbeaux partent en excursion (tour, portique, stèles, mare, champ, épouvantail) puis reviennent : ils utilisent le même code de vol que sur l'arbre.
// Échanges à longue distance (appels et réponses, poursuites, vols de parade, tournées de groupe, regroupement sur alerte) : section « longue distance ».
static void alerte();
namespace fam { static bool libre(int k); }
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
static const float TRX = 430.0f, PQX = 322.0f;             // vieille tour de guet (milieu) et portique de pierre
static float TRB() { return 203.0f + 1.2f * sinf(TRX / 37.0f) + 0.8f * sinf(TRX / 13.0f + 1.0f); }      // pied de la tour (un peu enfoncé dans le sol)
static float PQB() { return 203.0f + 1.2f * sinf(PQX / 37.0f) + 0.8f * sinf(PQX / 13.0f + 1.0f); }
static const float STL[5][3] = {{346.0f, 28.0f, -0.12f}, {386.0f, 34.0f, 0.10f}, {470.0f, 30.0f, -0.09f}, {498.0f, 22.0f, 0.15f}, {-204.0f, 24.0f, 0.10f}};      // stèles : pied, hauteur, inclinaison
struct Mon { uint8_t tour, lng; };      // réglages : décor « tour en ruine » (1) ou « simple » (0) ; échanges à longue distance (1) ou non (0)
static Mon RGM = {1, 1};
static const float PX = -70.0f, PY = 205.0f, PRX = 34.0f, PRY = 5.5f;      // mare
static const float CH0 = -176.0f, CH1 = -112.0f;           // champ : bords gauche et droit
static const uint16_t CLE_TR = 0x1818;                        // couleur transparente (identique dans les deux ordres d'octets)

enum { D_TOUR = 0, D_MARE, D_CHAMP, D_EPOUV, D_DISPUTE, D_STELE, D_PORT, D_NB };
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
static float echelleX(float x) {      // perspective : les oiseaux rapetissent vers la tour, grossissent vers la mare
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

// ---------- bande lointaine : arbres lointains tous différents, deux plans de collines, épouvantail au loin ----------
static void dessinerEpouvantail(M5Canvas& g, float x, float y, float k, uint16_t pc);
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
  if (RGM.tour) dessinerEpouvantail(g, 566.0f - 40.0f, 193.0f, 0.5f, CF(30 * ambCol, 4 * ambCol, 9 * ambCol));      // épouvantail au loin, derrière la tour
}

// ---------- bande proche : sol noir, mare, champ, chemin, vieille tour en ruine, portique, stèles, clôture, épouvantail, brume basse ----------
static void dessinerTour(M5Canvas& g) {      // tour de guet en ruine : sommet cassé, fenêtre à lanterne rouge, mur écroulé, souche de mur, gravats
  const float b = TRB(), x = TRX;
  const uint16_t pierre = C(11, 4, 7), ombre = C(6, 2, 4), liser = C(96, 16, 28), joint = C(20, 7, 11), noir = C(3, 1, 2), clair = C(40, 9, 15);
  // souche de mur à gauche et mur écroulé à droite (en escalier)
  rect(g, x - 52, b - 16, x - 20, b, pierre); tri(g, x - 52, b - 16, x - 38, b - 16, x - 46, b - 22, pierre); ligne(g, x - 52, b - 16, x - 38, b - 16, liser);
  rect(g, x + 20, b - 28, x + 34, b, pierre); tri(g, x + 20, b - 28, x + 34, b - 28, x + 26, b - 34, pierre); ligne(g, x + 20, b - 28, x + 26, b - 34, liser);
  rect(g, x + 34, b - 18, x + 50, b, pierre); tri(g, x + 34, b - 18, x + 50, b - 18, x + 44, b - 23, pierre); ligne(g, x + 34, b - 18, x + 50, b - 18, liser);
  rect(g, x + 50, b - 9, x + 66, b, pierre); ligne(g, x + 50, b - 9, x + 66, b - 9, liser);
  for (int i = 0; i < 10; i++) { float gx = x + 66.0f + i * 6.5f + 2.0f * ((i * 7) % 3), gh = 2.0f + (i % 3) * 1.4f; tri(g, gx - 3, b, gx + 3, b, gx + ((i % 2) ? 1.0f : -1.0f), b - gh, pierre); }      // gravats
  for (int i = 0; i < 6; i++) { float gx = x - 62.0f - i * 6.0f, gh = 2.0f + (i % 2) * 1.6f; tri(g, gx - 3, b, gx + 3, b, gx, b - gh, pierre); }
  // corps : colonne gauche haute, partie droite plus basse, cassure en biais
  rect(g, x - 21, b - 100, x + 1, b, pierre);
  rect(g, x + 1, b - 78, x + 21, b, pierre);
  tri(g, x + 1, b - 100, x + 1, b - 78, x + 10, b - 78, pierre);
  tri(g, x + 10, b - 78, x + 21, b - 78, x + 15, b - 84, pierre);
  rect(g, x - 21, b - 105, x - 15, b - 100, pierre); rect(g, x - 9, b - 105, x - 3, b - 100, pierre);      // créneaux restants
  rect(g, x + 15, b - 78, x + 21, b, ombre); rect(g, x - 21, b - 6, x + 21, b, ombre);
  ligne(g, x - 21, b - 100, x - 21, b, liser); ligne(g, x - 21, b - 105, x - 15, b - 105, liser); ligne(g, x - 9, b - 105, x - 3, b - 105, liser); ligne(g, x - 15, b - 100, x - 9, b - 100, C(60, 10, 20));
  ligne(g, x + 1, b - 100, x + 1, b - 78, C(60, 10, 20)); ligne(g, x + 1, b - 78, x + 10, b - 78, C(60, 10, 20)); ligne(g, x + 10, b - 78, x + 15, b - 84, C(60, 10, 20));
  for (int rg = 0; rg < 12; rg++) {      // assises de pierre
    float yy = b - 96.0f + rg * 8.0f; if (yy > b - 4) break;
    ligne(g, x - 21, yy, (yy < b - 78) ? x + 1 : x + 21, yy, joint);
    float vx = x - 12.0f + ((rg & 1) ? 11.0f : 0.0f); ligne(g, vx, yy, vx, yy + 8.0f, joint); if (yy > b - 76) { float v2 = vx + 20.0f; if (v2 < x + 20) ligne(g, v2, yy, v2, yy + 8.0f, joint); }
  }
  ligne(g, x - 6, b - 100, x - 10, b - 76, noir); ligne(g, x - 10, b - 76, x - 5, b - 58, noir); ligne(g, x + 9, b - 78, x + 5, b - 52, noir); ligne(g, x + 5, b - 52, x + 8, b - 36, noir);      // fissures
  rect(g, x - 5, b - 66, x + 5, b - 50, noir); tri(g, x - 5, b - 66, x + 5, b - 66, x, b - 72, noir);      // fenêtre en arc (la lanterne rouge est ajoutée par dyn)
  rect(g, x - 5, b - 56, x + 5, b - 50, C(44, 7, 10));
  rect(g, x - 6, b - 26, x + 6, b, noir); tri(g, x - 6, b - 26, x + 6, b - 26, x, b - 33, noir);      // porte
  rect(g, x - 14, b - 40, x - 12.5f, b - 31, noir); rect(g, x + 12, b - 44, x + 13.5f, b - 35, noir);      // meurtrières
  ligne(g, x - 20, b - 1, x + 20, b - 1, clair);
}
static void dessinerPortique(M5Canvas& g) {      // portique de pierre à moitié effondré, une lanterne éteinte pend du linteau
  const float b = PQB(), x = PQX;
  const uint16_t pierre = C(11, 4, 7), liser = C(96, 16, 28), noir = C(3, 1, 2), joint = C(20, 7, 11);
  rect(g, x - 17, b - 58, x - 10, b, pierre); ligne(g, x - 17, b - 58, x - 17, b, liser);
  rect(g, x + 10, b - 40, x + 17, b, pierre); tri(g, x + 10, b - 40, x + 17, b - 40, x + 14, b - 46, pierre); ligne(g, x + 10, b - 40, x + 14, b - 46, liser);
  rect(g, x - 24, b - 64, x + 6, b - 58, pierre); tri(g, x - 24, b - 64, x - 30, b - 68, x - 22, b - 64, pierre); tri(g, x + 6, b - 64, x + 6, b - 58, x + 13, b - 55, pierre);
  ligne(g, x - 24, b - 64, x + 6, b - 64, liser); ligne(g, x - 30, b - 68, x - 24, b - 64, liser);
  rect(g, x - 21, b - 53, x + 3, b - 51, pierre);
  ligne(g, x - 17, b - 40, x - 10, b - 40, joint); ligne(g, x - 17, b - 20, x - 10, b - 20, joint); ligne(g, x + 10, b - 20, x + 17, b - 20, joint); ligne(g, x - 12, b - 64, x - 9, b - 58, noir);
  rect(g, x - 20, b - 3, x - 7, b, pierre); rect(g, x + 7, b - 3, x + 20, b, pierre);
  ligne(g, x - 7, b - 51, x - 7, b - 44, noir); rect(g, x - 9.5f, b - 44, x - 4.5f, b - 37, C(46, 8, 10));      // lanterne (rallumée par dyn)
}
static void stele(M5Canvas& g, float x, float h, float lean) {
  const float b = solHaut(x) + 1.0f, w = 5.5f, tx = lean * h;
  const uint16_t pierre = C(12, 5, 8), liser = C(90, 15, 26), joint = C(24, 8, 13);
  tri(g, x - w, b, x + w, b, x + w + tx, b - h, pierre); tri(g, x - w, b, x + w + tx, b - h, x - w + tx, b - h, pierre);
  tri(g, x - w + tx, b - h, x + w + tx, b - h, x + tx, b - h - 3.4f, pierre);
  ligne(g, x - w, b, x - w + tx, b - h, liser); ligne(g, x - w + tx, b - h, x + tx, b - h - 3.4f, liser);
  for (int i = 0; i < 3; i++) { float u = 0.40f + 0.16f * i; ligne(g, x - 2.2f + tx * u, b - h * u, x + 2.2f + tx * u, b - h * u, joint); }
  rect(g, x - w - 1.5f, b - 1.6f, x + w + 1.5f, b, C(6, 2, 4));
}
static void cloture(M5Canvas& g, float xa, float xb, float h, uint16_t pc) {
  for (float x = xa; x <= xb + 0.1f; x += 14.0f) { float y = solHaut(x) - 0.5f; float hh = (((int)x / 14) % 3 == 0) ? h * 0.6f : h; rect(g, x - 1.3f, y - hh, x + 1.3f, y, pc); }
  float ya = solHaut(xa), yb = solHaut(xb);
  ligne(g, xa, ya - h * 0.8f, xa + (xb - xa) * 0.6f, yb - h * 0.8f + (ya - yb) * 0.4f + 3.0f, pc); ligne(g, xa, ya - h * 0.4f, xb, yb - h * 0.4f, pc);      // lisse du haut cassée
}
static void dessinerEpouvantail(M5Canvas& g, float x, float y, float k, uint16_t pc) {
  epais(g, x, y, x, y - 44 * k, 2.6f * k + 0.4f, pc); epais(g, x - 17 * k, y - 36 * k, x + 17 * k, y - 34 * k, 2.2f * k + 0.4f, pc);      // poteau et bras
  tri(g, x - 6 * k, y - 36 * k, x + 6 * k, y - 36 * k, x, y - 20 * k, pc); disque(g, x, y - 45 * k, 4.6f * k, pc);
  tri(g, x - 8 * k, y - 48 * k, x + 8 * k, y - 48 * k, x, y - 58 * k, pc); rect(g, x - 10 * k, y - 49 * k, x + 10 * k, y - 47.4f * k, pc);      // chapeau
  epais(g, x - 17 * k, y - 35 * k, x - 21 * k, y - 22 * k, 1.3f * k + 0.3f, pc); epais(g, x + 17 * k, y - 35 * k, x + 21 * k, y - 23 * k, 1.3f * k + 0.3f, pc);      // chiffons qui pendent
}
static void brumeBasse(M5Canvas& g, int n) {      // brume basse : petits traits clairs posés sur la ligne du sol (un seul dessin, pas de calcul à chaque image)
  uint32_t sd = 991; const int w = g.width();
  for (int i = 0; i < n; i++) {
    sd = (sd * 1103515245UL + 12345UL) & 0x7fffffffUL; int px = (int)((sd >> 8) % (uint32_t)w);
    sd = (sd * 1103515245UL + 12345UL) & 0x7fffffffUL; int dy = (int)((sd >> 8) % 9) - 3;
    sd = (sd * 1103515245UL + 12345UL) & 0x7fffffffUL; int len = 8 + (int)((sd >> 8) % 20);
    float wx = (px - ztx) / zs, ys = solHaut(wx) * zs + zty - dy; float k = ambCol * (0.55f + 0.45f * ((sd >> 3) & 1));
    g.drawFastHLine(px, (int)ys, len, CF(30 * k, 9 * k, 16 * k));
  }
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
  // mare : eau sombre, reflet rougeâtre, roseaux
  ell(g, PX, PY, PRX + 2.5f, PRY + 1.4f, C(12, 4, 8)); ell(g, PX, PY, PRX, PRY, C(42, 8, 21)); ell(g, PX + 2, PY + 0.4f, PRX * 0.62f, PRY * 0.55f, C(62, 12, 30));
  for (int i = 0; i < 9; i++) {
    float rx0 = PX - PRX + 2.0f + i * 2.2f + (i > 4 ? PRX * 1.5f : 0.0f), hh = 12.0f + 7.0f * ((i * 5) % 4) * 0.5f, lean = ((i % 3) - 1) * 2.2f;
    epais(g, rx0, PY - 0.5f, rx0 + lean, PY - hh, 1.0f, noir); if (i % 2 == 0) ell(g, rx0 + lean, PY - hh, 1.0f, 2.6f, noir);
  }
  if (RGM.tour) {
    // chemin de terre qui mène à la tour
    tri(g, TRX - 5, TRB(), TRX + 5, TRB(), TRX + 62, 256, C(15, 4, 8)); tri(g, TRX - 5, TRB(), TRX + 62, 256, TRX - 40, 256, C(15, 4, 8));
    dessinerPortique(g); dessinerTour(g);
    for (int i = 0; i < 5; i++) stele(g, STL[i][0], STL[i][1], STL[i][2]);
    cloture(g, 520.0f, 548.0f, 13.0f, noir);
  } else cloture(g, 360.0f, 392.0f, 13.0f, noir);
  dessinerEpouvantail(g, (CH0 + CH1) * 0.5f, solHaut((CH0 + CH1) * 0.5f), 1.0f, noir);
  brumeBasse(g, RGM.tour ? 520 : 260);
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
static void chargerMon();
static void rebake() {      // appelé par construireFond : première fois = création des bandes, ensuite = redessin (changement d'ambiance ou de décor)
  if (bpp != 2) return;
  chargerMon();
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
struct Halo { bool on; int8_t d; float x, y, t, amp; };
static const int NHALO = 4;
static Halo halo[NHALO];
struct Goutte { bool on; float x, y, vx, vy, t; };
static Onde onde[4];
static Goutte gout[14];
static void spawnOnde(float x, float y) { for (int i = 0; i < 4; i++) if (!onde[i].on) { onde[i].on = true; onde[i].x = x; onde[i].y = y; onde[i].t = 0; return; } }
static void spawnGoutte(float x, float y, float vx, float vy) { for (int i = 0; i < 14; i++) if (!gout[i].on) { gout[i].on = true; gout[i].x = x; gout[i].y = y; gout[i].vx = vx; gout[i].vy = vy; gout[i].t = 0; return; } }
static bool vis(float x0, float x1) { return TX(x1) > -12.0f && TX(x0) < 332.0f; }
static void dyn() {
  M5Canvas& g = cv; const float t = tAnim; sec = false;
  if (RGM.tour && vis(TRX - 26, TRX + 26)) {      // lanterne rouge dans la fenêtre de la tour : faible, qui vacille, parfois presque éteinte
    float f = 0.70f + 0.25f * sinf(t * 7.0f) * sinf(t * 2.1f + 1.0f); if (fmodf(t, 37.0f) < 0.30f) f = 0.10f;
    const float b = TRB();
    disque(g, TRX, b - 55.0f, 8.0f, CF(30 * f, 3 * f, 4 * f)); rect(g, TRX - 4, b - 62, TRX + 4, b - 50, CF(46 * f, 5 * f, 6 * f));
    rect(g, TRX - 2.2f, b - 59, TRX + 2.2f, b - 52, CF(220 * f, 34 * f, 24 * f));
    rect(g, TRX - 0.5f, b - 62, TRX + 0.5f, b - 50, C(3, 1, 2));
  }
  if (RGM.tour && vis(PQX - 12, PQX + 4)) {      // lanterne du portique
    float f = 0.55f + 0.30f * sinf(t * 5.3f + 2.0f) * sinf(t * 1.7f); const float b = PQB();
    disque(g, PQX - 7.0f, b - 40.5f, 5.5f, CF(24 * f, 3 * f, 4 * f)); disque(g, PQX - 7.0f, b - 40.5f, 1.9f, CF(210 * f, 36 * f, 26 * f));
  }
  if (RGM.tour) {      // brume basse qui dérive lentement
    for (int i = 0; i < 3; i++) {
      float x = fmodf(t * (5.0f + 2.0f * i) + i * 230.0f, 800.0f) + MX0, y = solHaut(x) - 2.5f - 2.0f * i;
      if (vis(x, x + 44.0f)) { float k = ambCol; ligne(g, x, y, x + 44.0f, y, CF(34 * k, 10 * k, 18 * k)); ligne(g, x + 8.0f, y - 1.2f, x + 36.0f, y - 1.2f, CF(24 * k, 7 * k, 13 * k)); }
    }
  }
  for (int i = 0; i < NHALO; i++) if (halo[i].on) {      // halo sonore : trois petits arcs qui s'éloignent de la tête, dans le sens du regard
    Halo& q = halo[i]; if (!vis(q.x - 30.0f, q.x + 30.0f)) continue;
    for (int j = 0; j < 3; j++) {
      float u = (q.t - 0.16f * j) / 0.9f; if (u <= 0.0f || u >= 1.0f) continue;
      float r = 4.0f + 20.0f * u * q.amp, fa = (1.0f - u) * q.amp, px = 0, py = 0;
      for (int sgm = 0; sgm <= 4; sgm++) {
        float a = -0.75f + 0.375f * sgm, X = q.x + q.d * r * cosf(a), Y = q.y + r * 0.85f * sinf(a);
        if (sgm) ligne(g, px, py, X, Y, CF(230 * fa, 70 * fa, 60 * fa)); px = X; py = Y;
      }
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
struct Rt { uint8_t dest, ph, spot, n, hops, cop, seq, grp; float t, dur, tx, sB, tw, tl; bool away; };      // cop : compagnon + 1 ; seq : prochaine destination + 1 ; grp : vol de groupe (haltes courtes) ; tl : minuteur du duo
static void liaisonArrivee(Corbeau& c, int idx);
static void finR(Rt& r) { r.away = false; r.cop = r.seq = r.grp = 0; r.tl = 0.0f; }
static Rt rt[NCB];
static int8_t rang[NAD];
static int8_t lien[NAD][NAD];
static float gTrip = 20.0f;
static int nbAilleurs() { int n = 0; for (int k = 0; k < NAD; k++) if (rt[k].away && cr[k].etat != E_DEHORS && cr[k].etat != E_OFF) n++; return n; }
static int spotsPris(int d, int sp) { for (int k = 0; k < NAD; k++) if (rt[k].away && rt[k].dest == d && rt[k].spot == sp) return 1; return 0; }
static int nSp(int d) { return d == D_TOUR ? 4 : (d == D_STELE ? 3 : 2); }      // places de perchoir par site
static bool placeLibre(int d) { if (d == D_CHAMP) return true; for (int s = 0; s < nSp(d); s++) if (!spotsPris(d, s)) return true; return false; }
static void spotPerch(int dest, int sp, float& x, float& y, float& pente, int& d) {      // appui des pattes sur chaque site perché
  pente = 0.0f; d = (sp & 1) ? 1 : -1;
  if (dest == D_EPOUV) { float cx = (CH0 + CH1) * 0.5f; x = cx + ((sp == 0) ? -13.0f : 13.0f); y = solHaut(cx) - 36.6f; d = (sp == 0) ? -1 : 1; return; }      // sur les bras de l'épouvantail
  if (dest == D_TOUR) {
    const float b = TRB();
    switch (sp) {
      case 0: x = TRX - 6.0f; y = b - 100.0f; d = -1; break;      // sommet de la tour, côté gauche (face à l'arbre)
      case 1: x = TRX + 15.0f; y = b - 78.0f; d = 1; break;      // sommet cassé, côté droit
      case 2: x = TRX + 27.0f; y = b - 28.0f; d = -1; break;      // mur écroulé
      default: x = TRX - 34.0f; y = b - 16.0f; d = -1; break;      // souche de mur
    }
    return;
  }
  if (dest == D_PORT) { const float b = PQB(); if (sp == 0) { x = PQX - 12.0f; y = b - 64.0f; d = -1; } else { x = PQX + 13.0f; y = b - 40.0f; d = 1; } return; }      // linteau et pilier brisé
  if (dest == D_STELE) { int i = sp < 3 ? sp : 0; x = STL[i][0] + STL[i][2] * STL[i][1]; y = solHaut(STL[i][0]) + 1.0f - STL[i][1] - 3.0f; d = (i & 1) ? 1 : -1; return; }      // sur une stèle
  x = TRX; y = TRB(); d = 1;
}
static int choisirDest(int idx) {
  float w[D_NB] = {0};
  if (RGM.tour) { w[D_TOUR] = 30.0f; w[D_STELE] = 14.0f; w[D_PORT] = 12.0f; }
  w[D_MARE] = 34.0f; w[D_CHAMP] = 24.0f; w[D_EPOUV] = 16.0f;
  if (rt[idx].away) w[rt[idx].dest] = 0.0f;
  float tot = 0.0f;
  for (int i = 0; i < D_NB; i++) { if (i == D_DISPUTE || !placeLibre(i)) w[i] = 0.0f; tot += w[i]; }
  if (tot <= 0.0f) return -1;
  float r = rnd01() * tot; for (int i = 0; i < D_NB; i++) { if (r < w[i]) return i; r -= w[i]; } return D_CHAMP;
}
static bool excursion(Corbeau& c, int idx, int dest) {
  Rt& r = rt[idx]; float tx = 0, ty = 0; int sp = 0;
  if (dest == D_TOUR || dest == D_STELE || dest == D_PORT || dest == D_EPOUV) {
    if (dest != D_EPOUV && !RGM.tour) return false;
    int n = nSp(dest), st = random(n); sp = -1; for (int i = 0; i < n; i++) { int s = (st + i) % n; if (!spotsPris(dest, s)) { sp = s; break; } }
    if (sp < 0) return false; float pe; int dd; spotPerch(dest, sp, tx, ty, pe, dd);
  }
  else if (dest == D_MARE) { sp = spotsPris(D_MARE, 0) ? 1 : (spotsPris(D_MARE, 1) ? 0 : (int)random(2)); tx = (sp == 0) ? PX + PRX + 5.0f : PX - PRX - 5.0f; ty = solHaut(tx); }
  else { sp = 0; tx = (random(100) < 55) ? lim((CH0 + CH1) * 0.5f + rnd(-30.0f, 30.0f), CH0 + 8.0f, CH1 - 8.0f) : rnd(CH0 + 8.0f, CH1 - 8.0f); ty = solHaut(tx); }      // il picore souvent au pied de l'épouvantail
  if (!r.away) { r.sB = c.s; r.hops = 0; }
  r.away = true; r.dest = (uint8_t)dest; r.spot = (uint8_t)sp; r.ph = 0; r.t = 0; r.tx = tx; r.n = 0; r.dur = 0; r.tw = 0;
  c.perch = -1; c.item = -1; c.rapporte = false;
  decoller(c, tx, ty, B_MONDE);
  if (r.cop) { int p = r.cop - 1; if (p >= 0 && p < NAD && p != idx && rt[p].away && cr[p].etat == E_DECOLLE && cr[p].but == B_MONDE && cr[p].t < 0.3f) { c.vitesse = cr[p].vitesse; c.hz = cr[p].hz; } }      // deux oiseaux en duo volent au même rythme, côte à côte
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
  if (r.cop) {      // un duo rentre ensemble : il se pose près de son compagnon
    int p = r.cop - 1;
    if (p >= 0 && p < NAD && p != idx && cr[p].perch >= 0 && cr[p].perch < NPE && (cr[p].but == B_PERCHOIR || cr[p].etat == E_PERCHE)) {
      float bd = 1e9f; for (int pp = 0; pp < NPE; pp++) if (!perchOccupe(pp)) { float d = fabsf(PERCH[pp][0] - PERCH[cr[p].perch][0]); if (d < bd) { bd = d; i = pp; } }
    }
  }
  if (i < 0 && random(100) < 65) {      // il préfère se poser près d'un ami déjà perché sur l'arbre
    int ami = -1, best = 0; for (int k = 0; k < NAD; k++) if (k != idx && cr[k].etat == E_PERCHE && cr[k].perch >= 0 && cr[k].perch < NPE && lien[idx][k] > best) { best = lien[idx][k]; ami = k; }
    if (ami >= 0) { float bd = 1e9f; for (int pp = 0; pp < NPE; pp++) if (!perchOccupe(pp)) { float d = fabsf(PERCH[pp][0] - cr[ami].x); if (d < bd) { bd = d; i = pp; } } }
  }
  if (i < 0) i = perchPour(c);
  if (i < 0) { r.ph = 9; r.t = 0; return; }
  r.ph = 8; c.perch = i; c.item = -1; decoller(c, PERCH[i][0], perchY(i), B_PERCHOIR);
  if (r.cop) { int p = r.cop - 1; if (p >= 0 && p < NAD && p != idx && rt[p].ph == 8 && (cr[p].etat == E_DECOLLE || cr[p].etat == E_VOL) && cr[p].but == B_PERCHOIR) { c.vitesse = cr[p].vitesse; c.hz = cr[p].hz; } }      // retour côte à côte
}
static void apres(Corbeau& c, int idx) {      // fin de la routine : une autre destination (parfois) ou retour à l'arbre
  Rt& r = rt[idx]; r.hops++;
  if (r.seq) { int d = r.seq - 1; r.seq = 0; if ((d == D_MARE || d == D_TOUR || d == D_STELE || d == D_PORT) && !placeLibre(d)) d = D_CHAMP; if (excursion(c, idx, d)) return; }      // vol de groupe : prochaine étape (arbre, tour, mare)
  else if (!r.cop && !r.grp && r.hops < 3 && random(100) < 30 && nbAilleurs() <= 2) { int d = choisirDest(idx); if (d >= 0 && excursion(c, idx, d)) return; }
  retour(c, idx);
}
static void arrivee(Corbeau& c) {      // fin de l'atterrissage sur un site de l'excursion
  int idx = (int)(&c - cr); if (idx < 0 || idx >= NAD) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; return; }
  Rt& r = rt[idx]; r.t = 0; r.ph = 0; r.n = 0; c.ox = c.oy = 0; c.t = 0; c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.5f, 3.5f); c.sub = 0; c.peck = 0; c.om = 0;
  if (r.dest == D_TOUR || r.dest == D_EPOUV || r.dest == D_STELE || r.dest == D_PORT) {
    float x, y, pe; int d; spotPerch(r.dest, r.spot, x, y, pe, d); c.fx = x; c.fy = y; c.fp = pe; c.d = (int8_t)d;
    c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); r.dur = r.cop ? rnd(12.0f, 16.0f) : (r.grp ? rnd(3.5f, 5.5f) : rnd(11.0f, 19.0f));
    c.act = A_REGARDE; c.tAct = 0; c.dAct = rnd(4.0f, 7.0f); c.sub = -1;      // il observe depuis son perchoir
  } else {
    c.etat = E_SOL; r.dur = 0;
    c.d = (r.dest == D_MARE) ? ((r.spot == 0) ? -1 : 1) : ((random(2)) ? 1 : -1);
    if (r.dest == D_DISPUTE) c.d = (ob[r.n % NOB].x >= c.x) ? 1 : -1;
  }
  if (r.cop) liaisonArrivee(c, idx);
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
        finR(r); c.s = r.sB; r.ph = 0;
      } else { lienAj(idx, W, -1); retour(c, idx); }
    }
  }
}

// ---------- longue distance : appels et réponses, poursuites, vols de parade, tournées de groupe, regroupement sur alerte ----------
// Un seul échange à la fois, toutes les 18 à 34 s (divisé par l'Activité) ; réglage Réglages > Image > « Échanges lointains ». Aucun dessin nouveau en vue normale (la tête se tourne,
// le cri répond) ; en vue large ou en suivi, de petits arcs « halo sonore » sortent du bec (trois traits, quelques lignes par image seulement pendant un cri).
enum { L_APPEL = 0, L_RAPPEL, L_POURSUITE, L_PARADE, L_TOURNEE, L_ALERTE, L_NB };
struct Lng { bool on, inv; uint8_t type, step; int8_t a, b, c3; float t; };
static Lng LG; static float gLng = 20.0f, tAlertePrec = 0.0f;
static void chargerMon() {
  static bool lu = false; if (lu) return; lu = true;
  RGM.tour = prefs.getUChar("cbmtour", 1) ? 1 : 0; RGM.lng = prefs.getUChar("cblng", 1) ? 1 : 0;
}
static void spawnHalo(const Corbeau& c, float amp) {
  int k = 0; float tm = -1.0f;
  for (int i = 0; i < NHALO; i++) { if (!halo[i].on) { k = i; break; } if (halo[i].t > tm) { tm = halo[i].t; k = i; } }
  Halo& h = halo[k]; h.on = true; h.d = (c.d >= 0) ? 1 : -1; h.x = c.x + h.d * 15.0f * c.s; h.y = c.y - 6.0f * c.s; h.t = 0.0f; h.amp = amp;
}
static void appelSon(bool loin) { static const int ids[4] = {SN_CROA1, SN_CROA2, SN_CROA3, SN_CROASSE1}; son(ids[random(4)], loin ? 115 : 190); }      // un cri lointain est plus faible
static void crier(Corbeau& c, const Corbeau& vers, bool loin, float amp) {      // il se tourne vers l'autre, lève la tête, crie ; un halo sort du bec
  c.d = (vers.x >= c.x) ? 1 : -1; c.bec = 1.0f; c.hum = 1.0f; c.look = c.ha0 - 0.40f; c.tLook = 1.1f; c.surs = 0.12f;
  appelSon(loin); spawnHalo(c, amp);
}
static bool loinOk(int k) {      // un oiseau posé sur un site du monde (tour, stèle, portique, épouvantail, mare, champ)
  const Corbeau& c = cr[k]; const Rt& r = rt[k];
  return r.away && c.but == B_MONDE && c.visible && (c.etat == E_PERCHE || c.etat == E_SOL) && r.ph < 8 && r.dest != D_DISPUTE;
}
static bool arbreOkL(int k) { const Corbeau& c = cr[k]; return !rt[k].away && c.visible && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN; }
static bool dispo(int k) { return fam::libre(k) && !rt[k].away; }
static int nLibres(int d) { if (d == D_CHAMP) return 9; int n = 0; for (int s = 0; s < nSp(d); s++) if (!spotsPris(d, s)) n++; return n; }
static void camPour(float dur) {      // la vue large (ou le suivi) accompagne l'échange, une fois sur deux environ
  if (!pret || panneau || cmFin || cm == CM_SUIVI || millis() - tToucher < 5000UL || random(100) >= 60) return;
  if (cm == CM_NON) { demarrer(CM_LARGE, true); gLarge = rnd(50.0f, 90.0f); }
  if (cm == CM_LARGE) cmDur = mxf(cmDur, cmT + dur);
}
static void rentrer(int k, float delai) {      // un oiseau du monde rentre à l'arbre : posé = il part dans un instant, en vol = il change de cap
  Corbeau& c = cr[k]; Rt& r = rt[k]; r.cop = r.seq = r.grp = 0; r.tl = 0.0f;
  if (c.but != B_MONDE) return;
  if (c.etat == E_PERCHE || c.etat == E_SOL) { if (r.ph != 8) { r.ph = 9; r.t = 2.0f - delai; } c.bec = 1; c.hum = 1.0f; }
  else if (c.etat == E_VOL || c.etat == E_DECOLLE) {
    int i = perchPour(c); if (i < 0) return;
    c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); c.aWp = false; c.d = (c.tx >= c.x) ? 1 : -1; r.ph = 8;
  }
}
static void regrouper() {      // alerte : tous les oiseaux du monde rentrent à l'arbre en se suivant, les mieux classés les premiers
  int n = 0;
  for (int k = 0; k < NAD; k++) {
    Rt& r = rt[k]; Corbeau& c = cr[k]; if (!r.away || r.dest == D_DISPUTE || c.but != B_MONDE) continue;
    if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_OFF || c.etat == E_TENU || c.etat == E_TIR) continue;
    rentrer(k, 0.15f + 0.22f * (float)rang[k]); n++;
  }
  if (n && cm == CM_NON && pret && !panneau && millis() - tToucher > 5000UL && random(100) < 50) { demarrer(CM_LARGE, true); cmDur = 10.0f; }
}
static void decorChange() {      // le décor passe à « simple » : ceux qui sont sur la tour, le portique ou les stèles rentrent
  if (LG.on) LG.on = false;
  if (RGM.tour) return;
  for (int k = 0; k < NAD; k++) { Rt& r = rt[k]; if (r.away && (r.dest == D_TOUR || r.dest == D_STELE || r.dest == D_PORT)) rentrer(k, 0.1f * (float)k); }
}
static void finLng() { LG.on = false; gLng = rnd(18.0f, 34.0f); }
static void finDuo(Corbeau& c, int k) {      // le duo se sépare du site : prochaine étape ensemble ou retour côte à côte
  Rt& r = rt[k];
  if (r.seq) { int d = r.seq - 1; r.seq = 0; if ((d == D_MARE || d == D_TOUR || d == D_STELE || d == D_PORT) && !placeLibre(d)) d = D_CHAMP; if (excursion(c, k, d)) return; }
  r.ph = 9; r.t = 1.4f;
}
static void liaisonArrivee(Corbeau& c, int idx) {      // appelé à chaque atterrissage d'un oiseau lié à un compagnon (duo)
  Rt& r = rt[idx]; int p = r.cop - 1;
  if (p < 0 || p >= NAD || p == idx || !rt[p].away || rt[p].cop != idx + 1) { r.cop = 0; return; }
  Corbeau& q = cr[p];
  if (!((q.etat == E_PERCHE || q.etat == E_SOL) && q.but == B_MONDE)) return;      // le compagnon est encore en vol : le premier arrivé attend
  const bool parade = (LG.on && LG.type == L_PARADE);
  c.d = (q.x >= c.x) ? 1 : -1; q.d = -c.d; float tm = rnd(2.6f, 3.6f); r.tl = rt[p].tl = tm;
  Corbeau* duo[2] = {&c, &q};
  for (int i = 0; i < 2; i++) { Corbeau& z = *duo[i]; z.hum = 1.0f; if (z.etat == E_PERCHE) { z.act = parade ? A_ETIRE : A_APPEL; z.tAct = 0.0f; z.dAct = parade ? 2.6f : 1.4f; } }
  c.bec = 1.0f; spawnHalo(c, 0.8f); if (parade) son(SN_AILES, 190); else appelSon(false);
}
static void debutLng() {
  int far_[NAD], nf = 0, tr[NAD], nt = 0;
  for (int k = 0; k < NAD; k++) { if (loinOk(k) && !rt[k].cop) far_[nf++] = k; else if (dispo(k)) tr[nt++] = k; }
  for (int i = nt - 1; i > 0; i--) { int j = random(i + 1); int q = tr[i]; tr[i] = tr[j]; tr[j] = q; }
  float w[L_NB] = {0};
  if (nf >= 1 && nt >= 1) { w[L_APPEL] = 34.0f; w[L_RAPPEL] = placeLibre(rt[far_[0]].dest) ? 22.0f : 0.0f; w[L_ALERTE] = 7.0f; }
  if (nf == 0 && nbAilleurs() == 0) {
    if (nt >= 2) { w[L_POURSUITE] = 28.0f; w[L_PARADE] = 22.0f; }
    if (nt >= 3) w[L_TOURNEE] = 28.0f;
  }
  float tot = 0.0f; for (int i = 0; i < L_NB; i++) tot += w[i]; if (tot <= 0.0f) return;
  float r = rnd01() * tot; int ty = 0; for (; ty < L_NB - 1; ty++) { if (r < w[ty]) break; r -= w[ty]; }
  Lng n; memset(&n, 0, sizeof(n)); n.on = true; n.type = (uint8_t)ty; n.a = n.b = n.c3 = -1;
  switch (ty) {
    case L_APPEL: n.a = (int8_t)far_[random(nf)]; n.b = (int8_t)tr[0]; n.inv = (random(100) < 40); if (nt >= 2 && random(100) < 50) n.c3 = (int8_t)tr[1]; camPour(10.0f); break;
    case L_RAPPEL: n.a = (int8_t)far_[random(nf)]; n.b = (int8_t)tr[0]; camPour(16.0f); break;
    case L_ALERTE: n.a = (int8_t)far_[random(nf)]; n.b = (int8_t)tr[0]; camPour(12.0f); break;
    case L_POURSUITE: {      // le plus haut placé poursuit l'autre
      int x = tr[0], y = tr[1]; if (rang[x] < rang[y]) { int q = x; x = y; y = q; }      // x = le dominé qui fuit
      n.a = (int8_t)y; n.b = (int8_t)x; camPour(14.0f);
    } break;
    case L_PARADE: {      // deux amis partent ensemble vers un site à deux places, côte à côte
      int a = tr[0], b = -1, best = -99; for (int i = 1; i < nt; i++) { int li = lien[a][tr[i]]; if (li > best) { best = li; b = tr[i]; } }
      int cand[4], nc = 0; if (RGM.tour) { if (nLibres(D_TOUR) >= 2) cand[nc++] = D_TOUR; if (nLibres(D_STELE) >= 2) cand[nc++] = D_STELE; if (nLibres(D_PORT) >= 2) cand[nc++] = D_PORT; } if (nLibres(D_MARE) >= 2) cand[nc++] = D_MARE;
      if (b < 0 || nc == 0) return;
      int d1 = cand[random(nc)], d2 = (d1 == D_MARE) ? D_CHAMP : D_MARE, sq = (random(100) < 55) ? d2 + 1 : 0;
      rt[a].cop = (uint8_t)(b + 1); rt[b].cop = (uint8_t)(a + 1); rt[a].seq = rt[b].seq = (uint8_t)sq; rt[a].grp = rt[b].grp = 1;
      bool oka = excursion(cr[a], a, d1), okb = oka && excursion(cr[b], b, d1);
      if (!okb) { rt[a].cop = rt[b].cop = 0; rt[a].seq = rt[b].seq = 0; rt[a].grp = rt[b].grp = 0; if (oka) {} return; }
      n.a = (int8_t)a; n.b = (int8_t)b; son(SN_AILES, 200); camPour(16.0f);
    } break;
    default: {      // tournée de groupe : arbre, tour (ou épouvantail), puis mare
      int d1 = RGM.tour ? D_TOUR : D_EPOUV; int m = nLibres(d1); if (m > nt) m = nt; if (m > 3) m = 3; if (m < 2) return;
      int d2 = placeLibre(D_MARE) ? D_MARE : D_CHAMP, lead = tr[0], ok = 0;
      for (int i = 1; i < m; i++) if (rang[tr[i]] < rang[lead]) lead = tr[i];
      int idx[3], ni = 0;
      for (int i = 0; i < m; i++) { int k = tr[i]; rt[k].grp = 1; rt[k].seq = (uint8_t)(d2 + 1); if (excursion(cr[k], k, d1)) { idx[ni++] = k; ok++; } else { rt[k].grp = 0; rt[k].seq = 0; } }
      if (ok < 2) { for (int i = 0; i < ni; i++) { rt[idx[i]].grp = 0; rt[idx[i]].seq = 0; } return; }
      lead = idx[0];
      for (int i = 0; i < ni; i++) if (idx[i] != lead) { cr[idx[i]].vitesse = cr[lead].vitesse + rnd(-3.0f, 3.0f); cr[idx[i]].hz = cr[lead].hz; }
      n.a = (int8_t)idx[0]; n.b = (int8_t)idx[1]; n.c3 = (ni > 2) ? (int8_t)idx[2] : (int8_t)-1; sonCri(); son(SN_AILES, 200); camPour(20.0f);
    } break;
  }
  LG = n; tAlertePrec = tAlerte;
}
static void majLng(float dt) {
  if (tAlerte > 0.0f && tAlertePrec <= 0.0f) regrouper();      // alerte (de n'importe quelle origine) : regroupement général
  tAlertePrec = tAlerte;
  const bool calme = V.ph == V_RIEN && !GP.ph && !appelOn && !panneau && !H.actif;
  if (!LG.on) {
    gLng -= dt * actFreq;
    if (gLng <= 0.0f) { gLng = rnd(18.0f, 34.0f); if (RGM.lng && pret && calme && tAlerte <= 0.0f && AP.n == 0 && !S.on) debutLng(); }
    return;
  }
  if (!calme || !RGM.lng || (tAlerte > 0.0f && LG.type != L_ALERTE)) { finLng(); return; }
  LG.t += dt;
  switch (LG.type) {
    case L_APPEL: {      // un oiseau loin appelle, un autre répond (ou l'inverse), le premier rappelle
      if (!loinOk(LG.a) || rt[LG.a].cop || !arbreOkL(LG.b)) { finLng(); break; }
      const int p = LG.inv ? LG.b : LG.a, q = LG.inv ? LG.a : LG.b; const bool loinP = (p == LG.a);
      if (LG.step == 0) { crier(cr[p], cr[q], loinP, 1.0f); LG.step = 1; }
      else if (LG.step == 1 && LG.t > 1.7f) { crier(cr[q], cr[p], !loinP, 0.75f); LG.step = 2; }
      else if (LG.step == 2 && LG.t > 3.3f) { crier(cr[p], cr[q], loinP, 0.8f); if (LG.c3 >= 0 && arbreOkL(LG.c3)) crier(cr[LG.c3], cr[LG.a], false, 0.6f); LG.step = 3; }
      else if (LG.step == 3 && LG.t > 5.0f) { lienAj(LG.a, LG.b, 1); finLng(); }
    } break;
    case L_RAPPEL: {      // un oiseau de l'arbre appelle un compagnon resté loin, vole jusqu'à lui ; ils rentrent ensemble
      if (LG.step < 3 && (!loinOk(LG.a) || rt[LG.a].cop || !arbreOkL(LG.b))) { finLng(); break; }
      if (LG.step == 0) { crier(cr[LG.b], cr[LG.a], false, 1.0f); LG.step = 1; }
      else if (LG.step == 1 && LG.t > 1.5f) { crier(cr[LG.a], cr[LG.b], true, 0.75f); LG.step = 2; }
      else if (LG.step == 2 && LG.t > 3.0f) {
        int dest = rt[LG.a].dest; rt[LG.b].cop = (uint8_t)(LG.a + 1); rt[LG.a].cop = (uint8_t)(LG.b + 1);
        if (placeLibre(dest) && excursion(cr[LG.b], LG.b, dest)) { rt[LG.b].grp = 0; LG.step = 3; LG.t = 0.0f; } else { rt[LG.b].cop = 0; rt[LG.a].cop = 0; finLng(); }
      }
      else if (LG.step == 3 && (LG.t > 18.0f || !rt[LG.b].away)) { lienAj(LG.a, LG.b, 1); finLng(); }
    } break;
    case L_POURSUITE: {      // l'un en poursuit un autre d'un bout à l'autre du monde ; le fuyard se pose, l'autre rentre
      if (LG.step == 0) {
        if (!arbreOkL(LG.a) || !arbreOkL(LG.b)) { finLng(); break; }
        crier(cr[LG.a], cr[LG.b], false, 1.0f); cr[LG.b].surs = 0.3f; cr[LG.b].d = (cr[LG.a].x >= cr[LG.b].x) ? 1 : -1; LG.step = 1;
      } else if (LG.step == 1 && LG.t > 0.9f) {
        if (!arbreOkL(LG.a) || !arbreOkL(LG.b)) { finLng(); break; }
        int dest = choisirDest(LG.b); if (dest < 0 || !excursion(cr[LG.b], LG.b, dest)) { finLng(); break; }
        cr[LG.b].vitesse = rnd(108.0f, 124.0f);
        Corbeau& y = cr[LG.a]; Rt& ry = rt[LG.a]; ry.away = true; ry.sB = y.s; ry.dest = D_CHAMP; ry.ph = 8; ry.hops = 9; ry.cop = ry.seq = ry.grp = 0; ry.t = 0.0f;
        y.perch = -1; y.item = -1; y.pj = LG.b; y.tOrb = 9.0f; decoller(y, cr[LG.b].tx, cr[LG.b].ty, B_POURSUITE);
        LG.step = 2; LG.t = 0.0f;
      } else if (LG.step == 2 && (cr[LG.a].but != B_POURSUITE || LG.t > 18.0f)) { lienAj(LG.a, LG.b, -1); finLng(); }
    } break;
    case L_PARADE: if (LG.t > 17.0f) finLng(); break;
    case L_TOURNEE: if (LG.t > 22.0f) finLng(); break;
    default: {      // alerte lointaine : la sentinelle de la tour donne l'alarme, tout le monde se regroupe
      if (LG.step == 0) { if (!loinOk(LG.a)) { finLng(); break; } Corbeau& a = cr[LG.a]; a.d = (a.x > 160.0f) ? -1 : 1; a.bec = 1.0f; a.hum = 1.0f; a.surs = 0.3f; son(SN_ALARME, 150); spawnHalo(a, 1.0f); LG.step = 1; }
      else if (LG.step == 1 && LG.t > 1.4f) { spawnHalo(cr[LG.a], 1.0f); appelSon(true); LG.step = 2; }
      else if (LG.step == 2 && LG.t > 2.4f) { alerte(); LG.step = 3; }
      else if (LG.step == 3 && LG.t > 3.6f) finLng();
    } break;
  }
}
static void majPoursuites() {      // une poursuite qui arrive sur un oiseau posé s'arrête : surprise, plumes, puis retour à l'arbre
  for (int k = 0; k < NAD; k++) {
    Corbeau& y = cr[k]; if (y.but != B_POURSUITE || y.etat != E_VOL || y.pj < 0 || y.pj >= NAD) continue;
    Corbeau& x = cr[y.pj]; if (!rt[y.pj].away || x.but != B_MONDE || x.etat == E_VOL || x.etat == E_DECOLLE) continue;
    if (hypotf(x.x - y.x, x.y - y.y) < 34.0f * y.s) { y.tOrb = 0.03f; x.surs = 0.35f; x.bec = 1.0f; x.flash = 0.5f; y.bec = 1.0f; plumes(x.x, x.y - 2.0f * x.s, 4); sonCri(); lienAj(k, y.pj, -1); }
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
      if (LG.on && LG.a >= 0 && LG.b >= 0) { const float xa = cr[LG.a].x, xb = cr[LG.b].x; tx = lerpf(tx, (xa + xb) * 0.5f, 0.85f); tz = (fabsf(xa - xb) < 190.0f) ? 0.6f : 0.5f; }      // l'échange à distance est au centre de l'image
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
        if (LG.on && (LG.a == suivi || LG.b == suivi)) {      // un échange à distance : la caméra garde les deux oiseaux dans l'image
          const int o = (LG.a == suivi) ? LG.b : LG.a;
          if (o >= 0 && cr[o].visible && cr[o].etat != E_DEHORS) { const float dd = fabsf(cr[o].x - c.x); if (dd < 420.0f) { tx = (c.x + cr[o].x) * 0.5f; tz = lim(330.0f / (dd + 60.0f), 0.62f, 1.1f); } }
        }
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
    if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_OFF || c.etat == E_TENU || c.etat == E_TIR) { finR(r); c.s = r.sB; continue; }
    if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but != B_MONDE) { finR(r); c.s = r.sB; continue; }      // de retour sur l'arbre
    if (r.cop && r.tl > 0.0f) { r.tl -= dt; if (r.tl <= 0.0f) { r.tl = 0.0f; finDuo(c, k); } }      // le duo se sépare du site
    float tgt = r.sB * echelleX(c.x); c.s += (tgt - c.s) * mnf(1.0f, dt * 3.0f);      // perspective : plus petit vers la maison, plus grand vers la mare
    if (c.etat == E_PERCHE && c.but == B_MONDE) {      // sur le toit : il observe, puis repart
      r.t += dt;
      if (r.ph == 0 && r.t > r.dur) apres(c, k);
      else if (r.ph == 9 && r.t > 2.0f) retour(c, k);
      else if (r.ph == 0 && r.t > r.dur * 0.5f && r.n == 0) { r.n = 1; c.bec = 1; sonCri(); spawnHalo(c, 0.8f); }
    }
  }
  for (int i = 0; i < NHALO; i++) if (halo[i].on) { halo[i].t += dt; if (halo[i].t > 1.3f) halo[i].on = false; }
  majSoc(dt);
  majLng(dt); majPoursuites();
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
  chargerMon(); memset(rt, 0, sizeof(rt)); memset(onde, 0, sizeof(onde)); memset(gout, 0, sizeof(gout)); memset(halo, 0, sizeof(halo)); memset(&LG, 0, sizeof(LG)); memset(&S, 0, sizeof(S)); gSoc = rnd(5.0f, 9.0f); gLng = rnd(14.0f, 24.0f); tAlertePrec = 0.0f;
  for (int k = 0; k < NAD; k++) { en[k] = 0.4f; tHer[k] = tJoie[k] = 0.0f; forceK[k] = 0; sBase[k] = cr[k].s; }
  for (int k = 0; k < NAD; k++) rang[k] = (int8_t)k;
  for (int k = NAD - 1; k > 0; k--) { int j = random(k + 1); int8_t q = rang[k]; rang[k] = rang[j]; rang[j] = q; }
  for (int i = 0; i < NAD; i++) for (int j = i; j < NAD; j++) lien[i][j] = lien[j][i] = (i == j) ? 0 : (int8_t)(random(5) - 2);
  cm = CM_NON; cmFin = false; suivi = -1; lpBird = -1; cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f;
  gLarge = rnd(40.0f, 70.0f); gTrip = rnd(10.0f, 20.0f); tMonde = 0.0f;
}
static void fermerMonde() { libererSprites(); tentee = false; cm = CM_NON; cmFin = false; suivi = -1; memset(rt, 0, sizeof(rt)); memset(halo, 0, sizeof(halo)); memset(&LG, 0, sizeof(LG)); }

}  // namespace mo
