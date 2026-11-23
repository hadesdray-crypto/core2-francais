#pragma once
// ======================= MONDE ÉLARGI, CAMÉRA ET VIE DES CORBEAUX (monde.h) =======================
// Inclus par corbeau.h, dans l'espace de noms cb, après les définitions dont il a besoin.
// Le paysage (ciel, collines et arbres lointains, sol, vieille tour en ruine, portique, stèles, mare, champ, épouvantail, arbre) est
// dessiné une seule fois en quatre bandes de pixels ; la caméra (vue d'ensemble, suivi d'un oiseau) les recompose à chaque image avec de la parallaxe.
// Les corbeaux partent en excursion (tour, portique, stèles, mare, champ, épouvantail) puis reviennent : ils utilisent le même code de vol que sur l'arbre.
// Échanges à longue distance (appels et réponses, poursuites, vols de parade, tournées de groupe, regroupement sur alerte) : section « longue distance ».
static void alerte();
namespace fam { static bool libre(int k); static bool detourner(Corbeau& c, int idx); static float persMul(int k, int type); static int dominant(int k); static float faimPetit(); }
static void chasser(Corbeau& c);
static void choc(float x, float y, Corbeau* a, Corbeau* b, float force);
static void chamaille();
static void volGroupe();
static void appelMutuel();

namespace mo {

// ---------- étendue du monde (l'arbre est entre x = 20 et 250, le sol vers y = 202) ----------
static const float MX0 = -240.0f, MX1 = 560.0f;            // largeur des bandes
static const float NY0 = 0.0f, NY1 = 257.0f;                // UN SEUL PLAN PRINCIPAL (plus de plan lointain à parallaxe ni de plan de l'arbre séparés) : collines, arbres lointains, sol, mare, champ, tour, stèles, clôtures et arbre
static const int WL = (int)(800.0f * ZN + 0.5f), HN = (int)(257.0f * ZN + 0.5f);      // tailles en pixels à l'échelle ZN
static const float TRX = 430.0f, PQX = 322.0f;             // vieille tour de guet (milieu) et portique de pierre
static float TRB() { return 203.0f + 1.2f * sinf(TRX / 37.0f) + 0.8f * sinf(TRX / 13.0f + 1.0f); }      // pied de la tour (un peu enfoncé dans le sol)
static float PQB() { return 203.0f + 1.2f * sinf(PQX / 37.0f) + 0.8f * sinf(PQX / 13.0f + 1.0f); }
static const float STL[5][3] = {{346.0f, 28.0f, -0.12f}, {386.0f, 34.0f, 0.10f}, {470.0f, 30.0f, -0.09f}, {498.0f, 22.0f, 0.15f}, {-204.0f, 24.0f, 0.10f}};      // stèles : pied, hauteur, inclinaison
struct Mon { uint8_t tour, lng, bet, tres, exc; };      // exc : excursions lointaines (tour, steles, portique) 0 rares (defaut), 1 normales, 2 frequentes (cle cbexc)      // réglages : décor « tour en ruine » (1) ou « simple » (0) ; échanges à longue distance (1) ou non (0)
static Mon RGM = {1, 1, 1, 1, 0};      // tres : tresors caches 0 non, 1 rares (defaut), 2 frequents (cle cbtres)      // bet : petites betes 0 non, 1 peu (2), 2 beaucoup (4)
static const float PX = -4.0f, PY = 205.0f, PRX = 42.0f, PRY = 6.5f;      // mare
static uint16_t cTour = 0, cMare = 0;      // tresors caches dans la tour et dans la mare (cle cbcach)
static const float CH0 = -176.0f, CH1 = -112.0f;
static const float RV0 = 250.0f, RV1 = 318.0f;           // zone de droite : petit ruisseau et bande d'herbe sombre (entre l'arbre et le portique)           // champ : bords gauche et droit
static const uint16_t CLE_TR = 0x1818;                        // couleur transparente (identique dans les deux ordres d'octets)

enum { D_TOUR = 0, D_MARE, D_CHAMP, D_EPOUV, D_DISPUTE, D_STELE, D_PORT, D_BETE, D_TRES, D_NB };      // D_TRES : un corbeau porte un objet brillant a la tour ou a la mare      // D_BETE : un corbeau part attraper une petite bete
enum { CM_NON = 0, CM_LARGE, CM_SUIVI };

static M5Canvas sCiel(&M5.Display), sNear(&M5.Display);      // ciel fixe + le plan principal (un seul sprite de 411 Ko au lieu de trois bandes : 216 Ko de PSRAM en moins)
static bool pret = false, tentee = false, vigFort = true;
static uint32_t sigBakeOk = 0; static bool bakeSaut = false;      // 3.1.32 : empreinte du paysage cuit (si elle n'a pas change, le paysage garde a la fermeture est reutilise)
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
  int a = ar(rx * zs), b = ar(ry * zs); if (a < 1) a = 1; if (b < 1) b = 1; const float X = TX(x), Y = TY(y); if (!coordOk(X, Y)) return;
  bbPt(X - a, Y - b); bbPt(X + a, Y + b);
  if (sec) { hmix(0x500000u | (uint16_t)ar(X)); hmix(((uint32_t)(uint16_t)ar(Y) << 16) | (uint16_t)a); hmix(((uint32_t)(uint16_t)b << 16) | c); cmdPush(5, c, ar(X), ar(Y), a, b); return; }
  g.drawEllipse(ar(X), ar(Y), a, b, c);
}
static float echelleX(float x) {      // 3.1.27 perspective par lieu (rapport a un oiseau de l'arbre = 1,0) : mare un peu plus petit, arbre 1,0, puis il RAPETISSE progressivement vers le fond : portique 0,48, steles 0,45 a 0,40, tour 0,40
  if (x < -70.0f) return 0.95f;
  if (x < 20.0f) return lerpf(0.95f, 1.0f, (x + 70.0f) / 90.0f);
  if (x < 230.0f) return 1.0f;
  if (x < 322.0f) return lerpf(1.0f, 0.48f, (x - 230.0f) / 92.0f);
  if (x < 430.0f) return lerpf(0.48f, 0.40f, (x - 322.0f) / 108.0f);
  return 0.40f;
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
  bkN = 0; brumeD(g, 110, 100, 100, 3, 0.35f); brumeD(g, 210, 128, 110, 3, 0.35f); brumeD(g, 90, 150, 100, 3, 0.35f);
  if (OP.hor) {
    for (int i = 0; i < (OP.hor == 2 ? 4 : 2); i++) {
      float oy = -9.0f + i * 7.0f + 1.5f * sinf(i * 2.1f), ox = 16.0f * sinf(i * 2.3f + 0.6f), rx = (30.0f + 9.0f * ((i * 5) % 3)) * zs, ry = (2.2f + 0.7f * (i % 3)) * zs;
      g.fillEllipse((int)(mxs + ox * zs), (int)(mys + oy * zs), (int)rx, (int)(ry + 0.5f), CF(15 + 4 * (i % 2), 1, 3));
      g.fillEllipse((int)(mxs + (ox + 14.0f) * zs), (int)(mys + (oy + 1.5f) * zs), (int)(rx * 0.5f), (int)(ry * 0.8f + 0.5f), CF(11, 0, 2));
    }
    brumeD(g, 70, 200, 150, 8, 0.85f); brumeD(g, 250, 204, 150, 7, 0.85f); brumeD(g, 160, 198, 120, 6, 0.80f);
  }
  float gy = solHaut(160.0f) * zs + zty;
  seed = 777;
  for (int i = 0; i < (2600 >> OP.dec); i++) {
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gx = (int)((seed >> 8) % 320);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gyy = (int)((seed >> 8) % (uint32_t)((int)gy - 6 > 1 ? (int)gy - 6 : 1));
    if (hypotf(gx - mxs, gyy - mys) < (LUNE_R + 30) * zs) continue;
    float r, gg, b; cielRGB((gyy - zty) / zs, r, gg, b); bool cl = (seed >> 3) & 1;
    g.drawPixel(gx, gyy, CF(r * (cl ? 1.12f : 0.88f) + (cl ? 7 : 0), gg * (cl ? 1.12f : 0.88f), b * (cl ? 1.12f : 0.88f)));
  }
}

// ---------- bande lointaine : arbres lointains tous différents, deux plans de collines, épouvantail au loin ----------
static void dessinerEpouvantail(M5Canvas& g, float x, float y, float k, uint16_t pc);
static void bakeLoin(M5Canvas& g) {      // dessiné en premier dans le plan principal : le sol et le reste le recouvrent
  VueId v_; fixerVue(MX0, NY0);
  static const float LT[][4] = {{282, 78, 0, 0.30f}, {334, 56, 3, 0.42f}, {238, 46, 2, 0.50f}, {8, 62, 4, 0.45f}, {-62, 72, 1, 0.50f}, {-30, 44, 3, 0.55f},
    {-98, 56, 0, 0.58f}, {-150, 70, 2, 0.50f}, {-205, 60, 4, 0.55f}, {372, 58, 4, 0.55f}, {500, 66, 1, 0.45f}, {548, 52, 3, 0.50f}};
  for (unsigned i = 0; i < sizeof(LT) / sizeof(LT[0]); i++) {
    if (!decOk((int)i)) continue;      // decor allege : un arbre lointain sur trois en moins ; minimal : un sur deux
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
static void dessinerMare(M5Canvas& g) {
  const uint16_t noir = C(4, 1, 3);
  // mare : eau sombre, reflet rougeâtre, roseaux
  ell(g, PX, PY, PRX + 2.5f, PRY + 1.4f, C(12, 4, 8)); ell(g, PX, PY, PRX, PRY, C(42, 8, 21)); ell(g, PX + 2, PY + 0.4f, PRX * 0.62f, PRY * 0.55f, C(62, 12, 30));
  for (int i = 0; i < 9; i++) {
    float rx0 = PX - PRX + 2.0f + i * 2.2f + (i > 4 ? PRX * 1.5f : 0.0f), hh = 12.0f + 7.0f * ((i * 5) % 4) * 0.5f, lean = ((i % 3) - 1) * 2.2f;
    epais(g, rx0, PY - 0.5f, rx0 + lean, PY - hh, 1.0f, noir); if (i % 2 == 0) ell(g, rx0 + lean, PY - hh, 1.0f, 2.6f, noir);
  }
}
static void dessinerZoneDroite(M5Canvas& g, bool bas) {      // petit ruisseau, pierres plates et bande d'herbe sombre a droite de l'arbre (meme style sombre que la mare)
  const uint16_t eau0 = C(12, 4, 8), eau1 = C(34, 7, 18), eau2 = C(56, 11, 27), herbe1 = C(9, 17, 9), herbe2 = C(15, 25, 13), pierre = C(21, 10, 14), pierreH = C(40, 20, 26);
  const float cx = 286.0f, gy = 205.0f;
  if (bas) {      // le ruisseau descend vers l'avant (visible surtout en vue reculee)
    tri(g, cx - 12.0f, gy, cx + 10.0f, gy, cx + 46.0f, 256.0f, eau0); tri(g, cx - 12.0f, gy, cx + 46.0f, 256.0f, cx - 40.0f, 256.0f, eau0);
    tri(g, cx - 8.0f, gy + 1.0f, cx + 6.0f, gy + 1.0f, cx + 36.0f, 256.0f, eau1); tri(g, cx - 8.0f, gy + 1.0f, cx + 36.0f, 256.0f, cx - 28.0f, 256.0f, eau1);
    for (int i = 0; i < 4; i++) { float u = (i + 1) * 0.2f, yy = gy + (256.0f - gy) * u, xx = cx - 2.0f + 18.0f * u + 3.0f * sinf(i * 2.4f); ligne(g, xx - 5.0f - 7.0f * u, yy, xx + 5.0f + 7.0f * u, yy, eau2); }
  }
  ell(g, cx, gy + 0.3f, 33.0f, 3.4f, eau0);      // berge
  ell(g, cx, gy + 0.5f, 31.0f, 2.4f, eau1);      // eau
  ell(g, cx - 5.0f, gy + 0.4f, 17.0f, 1.0f, eau2);      // reflet rougeatre
  for (int i = 0; i < 4; i++) { float x = cx - 24.0f + i * 15.0f + 3.0f * sinf(i * 1.7f); ell(g, x, gy - 0.4f, 2.8f, 1.5f, pierre); ligne(g, x - 1.5f, gy - 1.2f, x + 1.0f, gy - 1.3f, pierreH); }
  ell(g, 262.0f, solHaut(262.0f) - 0.2f, 8.0f, 2.8f, pierre); ligne(g, 258.0f, solHaut(262.0f) - 2.5f, 266.0f, solHaut(262.0f) - 2.7f, pierreH);      // pierre plate au soleil des lezards
  for (int i = 0; i < 17; i++) {      // bande d'herbe sombre
    float x = RV0 + 2.0f + i * 4.0f + 1.6f * sinf(i * 2.3f); if (x > RV1) break;
    float by = solHaut(x) + 0.6f, h = 4.0f + (float)((i * 7) % 5) + 0.8f * (float)(i % 2), lean = ((i % 3) - 1) * 1.8f;
    epais(g, x, by, x + lean, by - h, 0.9f, (i & 1) ? herbe1 : herbe2); epais(g, x + 1.3f, by, x + 1.3f - lean * 0.6f, by - h * 0.7f, 0.9f, herbe1); epais(g, x - 1.2f, by, x - 1.2f + lean * 0.5f, by - h * 0.6f, 0.9f, herbe2);
  }
}
static void decorNormal(M5Canvas& g) {      // la vue normale : le bord droit de la mare a gauche, le ruisseau et l'herbe a droite (dessin unique dans le fond)
  dessinerMare(g); dessinerZoneDroite(g, false);
}
static void bakeProchePh(int ph) {      // le plan principal est cuit en trois morceaux (un par image pendant le chargement des corbeaux)
  VueId v_; fixerVue(MX0, NY0);
  M5Canvas& g = sNear; const uint16_t noir = C(4, 1, 3);
  if (ph == 0) {
  g.fillScreen(CLE_TR); int h = g.height();
  bakeLoin(g);      // collines et arbres lointains, puis le sol par-dessus
  for (int x = 0; x < g.width(); x++) { float ys = solHaut((x - ztx) / zs) * zs + zty; g.drawFastVLine(x, (int)ys, h - (int)ys, C(5, 2, 4)); }
  } else if (ph == 1) {
  // champ : sillons qui convergent vers l'horizon (perspective)
  { const float vx = (CH0 + CH1) * 0.5f, vy = 150.0f;
    for (int i = 0; i <= 8; i++) {
      float xb = CH0 - 26.0f + i * 20.0f, xt = vx + (xb - vx) * (203.0f - vy) / (256.0f - vy);
      ligne(g, xt, 203.0f, xb, 256.0f, C(20, 4, 9)); ligne(g, xt + 1.0f, 203.0f, xb + 2.5f, 256.0f, C(12, 3, 6));
    } }
  dessinerMare(g); dessinerZoneDroite(g, true);
  if (RGM.tour) {
    // chemin de terre qui mène à la tour
    tri(g, TRX - 5, TRB(), TRX + 5, TRB(), TRX + 62, 256, C(15, 4, 8)); tri(g, TRX - 5, TRB(), TRX + 62, 256, TRX - 40, 256, C(15, 4, 8));
    dessinerPortique(g); dessinerTour(g);
    for (int i = 0; i < 5; i++) stele(g, STL[i][0], STL[i][1], STL[i][2]);
    cloture(g, 520.0f, 548.0f, 13.0f, noir);
  } else cloture(g, 360.0f, 392.0f, 13.0f, noir);
  } else {
  dessinerEpouvantail(g, (CH0 + CH1) * 0.5f, solHaut((CH0 + CH1) * 0.5f), 1.0f, noir);
  brumeBasse(g, (RGM.tour ? 520 : 260) >> OP.dec);      // décor allégé : moitié moins de traits de brume ; minimal : le quart
  arbre(g);      // l'arbre au premier plan, dans le même plan principal
  }
}
static void bakeProche() { for (int ph = 0; ph < 3; ph++) bakeProchePh(ph); }
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
static void vigAppliquer(M5Canvas& d, int y0, int y1) {      // bords noirs de l'ambiance Horreur, appliqués sur le paysage recomposé (rangées y0 à y1)
  if (!vigK) return;
  uint8_t* b = (uint8_t*)d.getBuffer(); if (!b) return;
  if (y0 < 24) y0 = 24; if (y1 > H_SCENE) y1 = H_SCENE;
  for (int y = y0; y < y1; y++) {
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
  if (sCiel.getBuffer()) sCiel.deleteSprite(); if (sNear.getBuffer()) sNear.deleteSprite();
  if (vigK) { free(vigK); vigK = nullptr; }
  pret = false; sigBakeOk = 0;
}
static void chargerMon();
static uint32_t sigBake() {      // tout ce dont dependent le ciel et le plan principal cuits
  uint32_t h = 2166136261u;
  h = (h ^ (uint32_t)OP.hor) * 16777619u; h = (h ^ (uint32_t)OP.amb) * 16777619u; h = (h ^ (uint32_t)OP.dec) * 16777619u; h = (h ^ (uint32_t)RGM.tour) * 16777619u;
  h = (h ^ (uint32_t)(int)(ambVig * 1000.0f)) * 16777619u; h = (h ^ (uint32_t)ambEt) * 16777619u; h = (h ^ (uint32_t)bpp) * 16777619u;
  return h;
}
static bool gardeMonde() { return sCiel.getBuffer() && sNear.getBuffer(); }      // les sprites du paysage sont encore en memoire (gardes a la fermeture)
// Cuisson PROGRESSIVE (3.1.32) : une etape par image pendant l'ecran de chargement. 0 : memoire + lecture des reglages ; 1 : ciel ; 2 a 4 : plan principal (3 morceaux) ; 5 : vignettage.
// Si le paysage garde en memoire a la fermeture correspond encore aux reglages, toutes les etapes sont sautees (bakeSaut).
static void bakePas(int p) {
  if (bpp != 2) return;
  if (p == 0) {
    chargerMon(); bakeSaut = false;
    if (!pret) {
      if (sCiel.getBuffer() && sNear.getBuffer()) { pret = true; if (sigBake() == sigBakeOk) bakeSaut = true; return; }      // paysage garde a la fermeture
      if (tentee) { bakeSaut = true; return; }
      tentee = true;
      if (ESP.getFreePsram() < 1300000UL) { bakeSaut = true; return; }
      if (!creer(sCiel, 320, H_SCENE) || !creer(sNear, WL, HN)) { libererSprites(); bakeSaut = true; return; }
      sNear.drawPixel(0, 0, C(248, 0, 0)); vigFort = (((const uint8_t*)sNear.getBuffer())[0] == 0xF8);      // ordre des octets : détecté par un essai
      pret = true; sigBakeOk = 0;
    } else if (sigBake() == sigBakeOk) bakeSaut = true;
    return;
  }
  if (bakeSaut) return;
  if (p == 1) { sigBakeOk = 0; bakeCiel(); } else if (p == 2) bakeProchePh(0); else if (p == 3) bakeProchePh(1); else if (p == 4) bakeProchePh(2); else { bakeVig(); sigBakeOk = sigBake(); }
}
static void rebake() {      // tout d'un coup (ancien comportement ; le chargement progressif appelle bakePas)
  for (int p = 0; p <= 5; p++) bakePas(p);
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
static void dynPart(int kp) {      // morceau kp du décor animé (0 lanterne de la tour, 1 lanterne du portique, 2 à 4 les trois bandes de brume, 5 halos sonores, 6 eau, 7 gouttes) : chacun a son propre rectangle, dessiné après les couches du paysage
  M5Canvas& g = cv; const float t = tAnim;
  if (kp == 0 && RGM.tour && vis(TRX - 26, TRX + 26)) {      // lanterne rouge dans la fenêtre de la tour : faible, qui vacille, parfois presque éteinte
    float f = 0.70f + 0.25f * sinf(t * 7.0f) * sinf(t * 2.1f + 1.0f); if (fmodf(t, 37.0f) < 0.30f) f = 0.10f;
    const float b = TRB();
    disque(g, TRX, b - 55.0f, 8.0f, CF(30 * f, 3 * f, 4 * f)); rect(g, TRX - 4, b - 62, TRX + 4, b - 50, CF(46 * f, 5 * f, 6 * f));
    rect(g, TRX - 2.2f, b - 59, TRX + 2.2f, b - 52, CF(220 * f, 34 * f, 24 * f));
    rect(g, TRX - 0.5f, b - 62, TRX + 0.5f, b - 50, C(3, 1, 2));
    if (cTour > 0 && decLvl() < 2) {      // la cache : quelques reflets dans l'ombre de la porte (au plus 9 petits points)
      const int n = cTour > 9 ? 9 : (int)cTour;
      for (int i = 0; i < n; i++) {
        const float gx = TRX - 4.5f + (float)((i * 7) % 10), gy = b - 2.2f - (float)((i * 5) % 4) * 1.4f; const bool vif = sinf(t * 2.1f + i * 1.9f) > 0.25f;
        disque(g, gx, gy, 0.8f, vif ? CF(235, 190, 120) : CF(120, 86, 56));
        if (vif && (i & 1)) { ligne(g, gx - 1.9f, gy, gx + 1.9f, gy, CF(200, 150, 100)); ligne(g, gx, gy - 1.9f, gx, gy + 1.9f, CF(200, 150, 100)); }
      }
    }
  }
  if (kp == 1 && RGM.tour && vis(PQX - 12, PQX + 4)) {      // lanterne du portique
    float f = 0.55f + 0.30f * sinf(t * 5.3f + 2.0f) * sinf(t * 1.7f); const float b = PQB();
    disque(g, PQX - 7.0f, b - 40.5f, 5.5f, CF(24 * f, 3 * f, 4 * f)); disque(g, PQX - 7.0f, b - 40.5f, 1.9f, CF(210 * f, 36 * f, 26 * f));
  }
  if (kp >= 2 && kp <= 4 && RGM.tour && decLvl() < 2 && (kp == 2 || decLvl() == 0)) {      // brume basse qui dérive lentement : une bande par rectangle (décor minimal ou allègement maximal : supprimée ; allégé : une seule bande)
    for (int i = kp - 2; i <= kp - 2; i++) {
      float x = fmodf(t * (5.0f + 2.0f * i) + i * 230.0f, 800.0f) + MX0, y = solHaut(x) - 2.5f - 2.0f * i;
      if (vis(x, x + 44.0f)) { float k = ambCol; ligne(g, x, y, x + 44.0f, y, CF(34 * k, 10 * k, 18 * k)); ligne(g, x + 8.0f, y - 1.2f, x + 36.0f, y - 1.2f, CF(24 * k, 7 * k, 13 * k)); }
    }
  }
  if (kp == 5) for (int i = 0; i < NHALO; i++) if (halo[i].on) {      // halo sonore : trois petits arcs qui s'éloignent de la tête, dans le sens du regard
    Halo& q = halo[i]; if (!vis(q.x - 30.0f, q.x + 30.0f)) continue;
    for (int j = 0; j < 3; j++) {
      float u = (q.t - 0.16f * j) / 0.9f; if (u <= 0.0f || u >= 1.0f) continue;
      float r = 4.0f + 20.0f * u * q.amp, fa = (1.0f - u) * q.amp, px = 0, py = 0;
      const int SG = decLvl() ? 2 : 4;      // allègement : trois segments au lieu de cinq pour l'arc
      for (int sgm = 0; sgm <= SG; sgm++) {
        float a = -0.75f + (1.5f / (float)SG) * sgm, X = q.x + q.d * r * cosf(a), Y = q.y + r * 0.85f * sinf(a);
        if (sgm) ligne(g, px, py, X, Y, CF(230 * fa, 70 * fa, 60 * fa)); px = X; py = Y;
      }
    }
  }
  if (kp == 6 && vis(PX - PRX - 4, PX + PRX + 4)) {
    if (decLvl() == 0) for (int i = 0; i < 4; i++) {      // reflets sur l'eau (supprimés avec un décor allégé ou pendant un allègement)
      float a = 0.5f + 0.5f * sinf(t * 1.7f + i * 2.1f), x = PX + 12.0f * sinf(t * 0.6f + i * 1.9f) - 6.0f, y = PY + (i - 1.5f) * 2.0f, l = 5.0f + 5.0f * a;
      if (fabsf(x - PX) + l > PRX * (1.0f - fabsf(y - PY) / PRY * 0.5f)) continue;
      ligne(g, x - l, y, x + l, y, CF(40 + 40 * a, 8 + 6 * a, 20 + 12 * a));
    }
    if (cMare > 0 && decLvl() < 2) {      // la cache au fond de l'eau : de petits eclats qui scintillent (au plus 6)
      const int n = cMare > 6 ? 6 : (int)cMare;
      for (int i = 0; i < n; i++) { if (sinf(t * 1.9f + i * 2.4f) < 0.1f) continue; disque(g, PX - 22.0f + i * 9.0f + 3.0f * sinf(i * 3.1f), PY + 0.6f + 1.6f * sinf(i * 1.7f), 0.7f, CF(205, 150, 112)); }
    }
    for (int i = 0; i < 4; i++) if (onde[i].on) { float u = onde[i].t / 1.3f, r = 2.0f + 9.0f * u; ellO(g, onde[i].x, onde[i].y, r, r * 0.32f, CF(120 * (1 - u), 40 * (1 - u), 60 * (1 - u))); }
  }
  if (kp == 7) for (int i = 0; i < 14; i++) if (gout[i].on) disque(g, gout[i].x, gout[i].y, 0.9f, C(150, 70, 90));
}

// ---------- caméra ----------
static uint8_t cm = CM_NON; static bool cmAuto = false, cmFin = false;
static float cz = ZN, cxx = 160.0f, cyy = 128.0f, czv = 0.0f, cxv = 0.0f, cyv = 0.0f;
static float cmT = 0.0f, cmDur = 0.0f, gLarge = 60.0f, tPerdu = 0.0f, tRetour = 0.0f;
static int8_t suivi = -1;
static bool reculLent = false; static float tReculRe = 0.0f; static int8_t suiviRet = -1;      // suiviRet : le corbeau suivi avant un événement (main, appel) : la vue suivie revient dessus      // recul de la vue : coupé si l'image devient trop lente ; délai avant de reculer de nouveau
static int8_t lpBird = -1; static uint32_t lpT = 0; static float lpX = 0, lpY = 0;
// vues choisies (OP.recul) : 0 normale, 1 arbre et rivière, 2 mare, 3 vieux bâtiments, 4 large ; le suivi d'un corbeau est un mode à part
static const float VUE_Z[6] = {1.0f, 0.92f, 0.80f, 1.25f, 0.46f, 1.04f};      // échelle de la caméra
static const float VUE_X[6] = {160.0f, 112.0f, -20.0f, 404.0f, 160.0f, 116.0f};      // centre (monde) : arbre + bord de la mare ; la mare a gauche avec le champ ; portique, stèles, tour
static const float VUE_Y[6] = {128.0f, 126.0f, 130.0f, 145.0f, 118.0f, 130.0f};      // 5 : le nid (branche secondaire, à gauche du tronc) et les branches de nuit
static const char* const VUE_NOM[6] = {"Vue", "Arbre", "Mare", "Ruine", "Large", "Nid"};      // étiquette du bouton (32 px de large)
static bool vueFixe() { return OP.recul > 0 || suiviRet >= 0 || (cm == CM_SUIVI && !cmFin); }      // une vue a été choisie : aucun événement spontané ne la quitte
static const char* etiquette() { return cm == CM_SUIVI ? "Suivi" : ((OP.recul > 0 && OP.recul <= 5) ? VUE_NOM[OP.recul] : (cm == CM_LARGE ? "Large" : "Vue")); }
static void majBouton() { boutonVue(fond); syncUI(); plein = true; }
static bool camOn() { return pret && cm != CM_NON; }
static void demarrer(uint8_t mode, bool autom) {
  suiviRet = -1; cmFin = false; cmAuto = autom; cmT = 0.0f; tPerdu = 0.0f; tRetour = 0.0f; cmDur = autom ? rnd(11.0f, 16.0f) : 1e9f;
  if (cm == CM_NON) { cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f; }
  cm = mode; majBouton();
}
static void fermer() { if (cm != CM_NON) { cmFin = true; } }
static int choisirSuivi();
static void reglerRecul(uint8_t n);
static void basculer() {
  if (!pret) return;
  if (cm == CM_SUIVI) { reglerRecul(0); return; }      // le suivi se termine : retour à la vue normale
  if (OP.recul < 5) { reglerRecul((uint8_t)(OP.recul + 1)); return; }      // Normal > Arbre > Mare > Vieux bâtiments > Large
  int k = choisirSuivi(); if (k >= 0) { suivi = (int8_t)k; demarrer(CM_SUIVI, false); majBouton(); } else reglerRecul(0);      // puis le suivi d'un corbeau, puis la vue normale
}
static void demandeLarge(bool on) { if (!pret) return; if (on && cm == CM_NON) demarrer(CM_LARGE, false); else if (!on && cm != CM_NON) fermer(); }
static void finSuivi(bool bloque) { if (OP.recul > 0 && !reculLent && !bloque) { suivi = -1; demarrer(CM_LARGE, false); } else fermer(); }      // le suivi se termine : la vue reculée reste (si elle est réglée)
static void reglerRecul(uint8_t n) {      // recul de la vue : 0 normale, 1 reculée, 2 très reculée (clé cbrecul) ; la vue large et le suivi existants font le travail
  if (n > 5) n = 5; OP.recul = n; prefs.putUChar("cbvue2", n); reculLent = false; tReculRe = 0.0f; suiviRet = -1;
  if (!pret) return;
  if (n == 0) { if (cm != CM_NON) fermer(); }
  else if (cm == CM_NON || cmFin) demarrer(CM_LARGE, false);
  else if (cm == CM_LARGE) { cmAuto = false; cmDur = 1e9f; }
  else if (cm == CM_SUIVI) { suivi = -1; demarrer(CM_LARGE, false); }
  majBouton();
}
static void reculer(int d) {      // pincement à deux doigts : se pincer recule d'un niveau, s'écarter avance d'un niveau
  if (!pret) { demandeLarge(d > 0); return; }
  const int n = (d > 0) ? 4 : 0;      // pincer = vue large, écarter = vue normale
  if (n == (int)OP.recul) { if (d < 0 && n == 0 && cm != CM_NON) fermer(); return; }
  reglerRecul((uint8_t)n);
}

// ---------- excursions : maison, mare, champ ----------
struct Rt { uint8_t dest, ph, spot, n, hops, cop, seq, grp; float t, dur, tx, sB, tw, tl; bool away; };      // cop : compagnon + 1 ; seq : prochaine destination + 1 ; grp : vol de groupe (haltes courtes) ; tl : minuteur du duo
static void liaisonArrivee(Corbeau& c, int idx);
static void finR(Rt& r) { r.away = false; r.cop = r.seq = r.grp = 0; r.tl = 0.0f; }
static Rt rt[NCB];
// ---------- petites betes : escargots, lezards, scarabees (BLOC AG) ----------
// Au plus 4 betes, une forme ou deux par image : une escargot lent, un lezard qui court par saccades, un scarabee qui trotte. Les corbeaux les reperent (la tete se tourne),
// vont les chercher, les attrapent et les mangent (energie : bloc J) ; un parent peut les garder pour le petit (bloc Y). Elles reapparaissent apres un delai.
// Reglage Petites betes : non / peu (2) / beaucoup (4), cle NVS cbbet (page Monde des reglages).
enum { BT_ESC = 0, BT_LEZ, BT_SCA };
struct Bete { bool on; uint8_t kind; int8_t res, dir; float x, ph, tm, mv, rep, ap, fuite; };      // res : corbeau qui la poursuit (-1 aucun) ; rep : delai avant de reapparaitre ; ap : apparition (0 a 1) ; fuite : le lezard detale
static const int NBT = 4;
static Bete bt[NBT];
static const uint8_t BT_K[NBT] = {BT_ESC, BT_LEZ, BT_SCA, BT_ESC};
static const float BT_Z0[NBT] = {40.0f, 252.0f, 150.0f, -92.0f};      // zones : pied de l'arbre, pierres a droite, sous l'arbre, berge gauche de la mare
static const float BT_Z1[NBT] = {112.0f, 314.0f, 238.0f, -54.0f};
static float gBete = 12.0f, repT = 0.0f; static int8_t repK = -1, repB = 0, beteVisee = -1;
static int nbBetes() { const int n = RGM.bet == 0 ? 0 : (RGM.bet == 1 ? 2 : 4), l = decLvl(); return l >= 2 ? 0 : ((l == 1 && n > 2) ? 2 : n); }      // decor allege : 2 betes au plus ; decor minimal (reglage ou allegement automatique) : aucune
static void mangeBete(Corbeau& c, int idx, int kind);
static bool beteDispo() { for (int i = 0; i < nbBetes(); i++) if (bt[i].on && bt[i].res < 0 && bt[i].ap > 0.9f && bt[i].fuite <= 0.0f) return true; return false; }
static int beteChoix() { int l[NBT], n = 0; for (int i = 0; i < nbBetes(); i++) if (bt[i].on && bt[i].res < 0 && bt[i].ap > 0.9f && bt[i].fuite <= 0.0f) l[n++] = i; return n ? l[random(n)] : -1; }
static void beteApparait(int i) {
  Bete& b = bt[i]; b.on = true; b.kind = BT_K[i]; b.res = -1; b.dir = random(2) ? 1 : -1; b.x = rnd(BT_Z0[i], BT_Z1[i]); b.ph = rnd(0.0f, 6.0f); b.tm = rnd(0.4f, 2.0f); b.mv = 0.0f; b.ap = 0.0f; b.fuite = 0.0f;
}
static void beteInit() { for (int i = 0; i < NBT; i++) { memset(&bt[i], 0, sizeof(Bete)); bt[i].res = -1; bt[i].rep = rnd(1.5f, 6.0f) + 2.0f * i; } gBete = rnd(8.0f, 16.0f); repK = -1; repT = 0.0f; }
static void majNbBetes() { for (int i = 0; i < NBT; i++) { if (i >= nbBetes()) { if (bt[i].on && bt[i].res >= 0) continue; bt[i].on = false; bt[i].rep = 1.0f; } else if (!bt[i].on && bt[i].rep > 8.0f) bt[i].rep = rnd(1.0f, 4.0f); } }
static void majBetes(float dt) {
  const int n = nbBetes();
  for (int i = 0; i < NBT; i++) {
    Bete& b = bt[i];
    if (b.on && b.res >= 0 && !(b.res < NAD && rt[b.res].away && rt[b.res].dest == D_BETE && rt[b.res].spot == i)) b.res = -1;      // le corbeau est parti ailleurs : elle est libre
    if (i >= n) { if (b.on && b.res < 0) b.on = false; continue; }
    if (!b.on) { b.rep -= dt; if (b.rep <= 0.0f && !panneau) beteApparait(i); continue; }
    b.ph += dt; if (b.ap < 1.0f) b.ap += dt * 2.0f;
    const float z0 = BT_Z0[i], z1 = BT_Z1[i];
    if (b.kind == BT_ESC) {      // escargot : tres lent, avance quelques secondes puis s'arrete
      if (b.res >= 0) { b.mv = 0.0f; continue; }
      if (b.mv > 0.0f) { b.x += b.dir * 1.7f * dt; b.mv -= dt; if (b.x < z0) { b.x = z0; b.dir = 1; } if (b.x > z1) { b.x = z1; b.dir = -1; } if (b.mv <= 0.0f) b.tm = rnd(3.0f, 7.0f); }
      else { b.tm -= dt; if (b.tm <= 0.0f) { b.mv = rnd(3.0f, 6.0f); if (random(100) < 35) b.dir = -b.dir; } }
    } else if (b.kind == BT_LEZ) {      // lezard : immobile, puis une course de quelques dixiemes de seconde
      if (b.fuite > 0.0f) { b.fuite -= dt; b.x += b.dir * 78.0f * dt; if (b.x < z0) { b.x = z0; b.dir = 1; } if (b.x > z1) { b.x = z1; b.dir = -1; } }
      else if (b.mv > 0.0f) { b.mv -= dt; b.x += b.dir * 50.0f * dt; if (b.x < z0) { b.x = z0; b.dir = 1; } if (b.x > z1) { b.x = z1; b.dir = -1; } if (b.mv <= 0.0f) b.tm = rnd(1.2f, 3.4f); }
      else { b.tm -= dt; if (b.tm <= 0.0f && b.res < 0) { b.mv = rnd(0.22f, 0.40f); b.dir = (b.x < z0 + 14.0f) ? 1 : ((b.x > z1 - 14.0f) ? -1 : (random(2) ? 1 : -1)); } else if (b.tm <= 0.0f) b.tm = 1.0f; }
    } else {      // scarabee : trotte, s'arrete, repart
      if (b.mv > 0.0f && b.res < 0) { b.mv -= dt; b.x += b.dir * 9.0f * dt; if (b.x < z0) { b.x = z0; b.dir = 1; } if (b.x > z1) { b.x = z1; b.dir = -1; } if (b.mv <= 0.0f) b.tm = rnd(0.6f, 2.0f); }
      else { b.tm -= dt; if (b.tm <= 0.0f && b.res < 0) { b.mv = rnd(0.8f, 2.4f); if (random(100) < 40) b.dir = -b.dir; } }
    }
  }
}
static void dessinBete(M5Canvas& g, const Bete& b) {
  const float k = 0.55f + 0.45f * lim(b.ap, 0.0f, 1.0f), y0 = solHaut(b.x) + 0.4f, d = (float)b.dir, x = b.x;
  if (b.kind == BT_ESC) {
    const float wob = sinf(b.ph * 3.0f) * 0.6f, ext = (b.res >= 0) ? 0.6f : 1.0f;
    ell(g, x - d * 1.2f * k, y0 - 1.2f * k, 4.6f * k * ext + 0.4f, 1.3f * k, C(56, 34, 38));      // corps
    ell(g, x + d * 3.2f * k * ext, y0 - 1.9f * k, 1.5f * k, 1.4f * k, C(66, 40, 44));      // tete
    ligne(g, x + d * 3.6f * k * ext, y0 - 2.8f * k, x + d * (4.2f + wob) * k * ext, y0 - 5.0f * k, C(70, 42, 46)); ligne(g, x + d * 2.6f * k * ext, y0 - 2.8f * k, x + d * (2.9f - wob) * k * ext, y0 - 5.2f * k, C(70, 42, 46));      // antennes
    disque(g, x - d * 0.8f * k, y0 - 3.6f * k, 3.4f * k, C(86, 38, 42)); disque(g, x - d * 0.8f * k, y0 - 3.6f * k, 2.0f * k, C(54, 20, 26)); disque(g, x - d * 0.8f * k, y0 - 3.6f * k, 0.9f * k, C(112, 52, 56));      // coquille en spirale
  } else if (b.kind == BT_LEZ) {
    const bool run = (b.fuite > 0.0f || b.mv > 0.0f);
    const float wag = run ? sinf(b.ph * 26.0f) * 1.3f : sinf(b.ph * 2.2f) * 0.5f, up = run ? 0.0f : 0.5f * fmaxf(0.0f, sinf(b.ph * 1.6f));
    ell(g, x, y0 - 2.0f * k - up, 5.2f * k, 1.6f * k, C(44, 62, 38));      // corps
    ell(g, x + d * 0.5f * k, y0 - 2.6f * k - up, 2.6f * k, 0.7f * k, C(72, 96, 54));      // dos clair
    ell(g, x + d * 6.4f * k, y0 - 2.3f * k - up, 2.3f * k, 1.3f * k, C(48, 68, 42));      // tete
    disque(g, x + d * 7.0f * k, y0 - 2.9f * k - up, 0.7f * k, C(225, 44, 30));      // oeil rouge
    ligne(g, x - d * 4.6f * k, y0 - 2.0f * k, x - d * 8.0f * k, y0 - 1.8f * k + wag, C(40, 56, 34)); ligne(g, x - d * 8.0f * k, y0 - 1.8f * k + wag, x - d * 11.5f * k, y0 - 1.4f * k - wag, C(36, 50, 31));      // queue
    const float st = run ? ((sinf(b.ph * 30.0f) > 0) ? 1.2f : -1.2f) : 0.0f;
    ligne(g, x + d * 3.0f * k, y0 - 1.2f * k, x + d * (3.0f + st) * k, y0 + 0.2f, C(36, 50, 31)); ligne(g, x - d * 2.5f * k, y0 - 1.2f * k, x - d * (2.5f + st) * k, y0 + 0.2f, C(36, 50, 31));      // pattes
  } else {
    const float st = (b.mv > 0.0f && b.res < 0) ? ((sinf(b.ph * 22.0f) > 0) ? 1.0f : -1.0f) : 0.0f;
    ell(g, x, y0 - 1.6f * k, 2.7f * k, 1.7f * k, C(28, 34, 62));      // elytres
    disque(g, x + d * 3.0f * k, y0 - 1.4f * k, 1.1f * k, C(20, 24, 42));      // tete
    disque(g, x - d * 0.4f * k, y0 - 2.3f * k, 0.7f * k, C(120, 138, 176));      // reflet
    ligne(g, x + d * 3.6f * k, y0 - 2.2f * k, x + d * 5.0f * k, y0 - 3.6f * k, C(40, 44, 70));
    for (int j = -1; j <= 1; j++) ligne(g, x + j * 1.5f * k, y0 - 0.8f * k, x + j * 1.5f * k + st * (j == 0 ? -1.0f : 1.0f) + d * 0.8f, y0 + 0.4f, C(24, 28, 50));      // pattes
  }
}
static void dessinBetes(M5Canvas& g) {
  if (!pret) return;
  for (int i = 0; i < NBT; i++) if (bt[i].on && vis(bt[i].x - 14.0f, bt[i].x + 14.0f)) dessinBete(g, bt[i]);      // une bete deja prise en chasse reste dessinee jusqu'au bout
}

static int8_t rang[NAD];
static int8_t lien[NAD][NAD];
static float gTrip = 20.0f;
static int nbAilleurs() { int n = 0; for (int k = 0; k < NAD; k++) if (rt[k].away && cr[k].etat != E_DEHORS && cr[k].etat != E_OFF) n++; return n; }
static int spotsPris(int d, int sp) { for (int k = 0; k < NAD; k++) if (rt[k].away && rt[k].dest == d && rt[k].spot == sp) return 1; return 0; }
static int nSp(int d) { return d == D_TOUR ? 4 : (d == D_STELE ? 3 : 2); }      // places de perchoir par site
static bool placeLibre(int d) { if (d == D_CHAMP) return true; if (d == D_BETE) return beteDispo(); for (int s = 0; s < nSp(d); s++) if (!spotsPris(d, s)) return true; return false; }
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
static int farPct() { return RGM.exc == 0 ? 12 : (RGM.exc == 1 ? 40 : 100); }      // 3.1.28 : part des visites au fond (tour, steles, portique) gardee selon le reglage Excursions lointaines (rares / normales / frequentes)
static float lngMul() { return RGM.exc == 0 ? 2.2f : (RGM.exc == 1 ? 1.4f : 1.0f); }      // delai multiplie entre deux echanges a longue distance
static int pickPond(const int* l, int n, int type) {      // tire un corbeau parmi la liste selon son caractere (fam::persMul)
  float w[NAD], tot = 0.0f; if (n > NAD) n = NAD;
  for (int i = 0; i < n; i++) { w[i] = fam::persMul(l[i], type); tot += w[i]; }
  float r = rnd01() * tot; for (int i = 0; i < n; i++) { if (r < w[i]) return l[i]; r -= w[i]; } return l[n - 1];
}
static int choisirDest(int idx) {
  float w[D_NB] = {0};
  // 3.1.28 : la chasse (petites betes) et le champ (graines) sont les excursions les plus frequentes ; la visite au fond est rare (reglage) et depend du caractere du corbeau
  const float mf = fam::persMul(idx, 0) * (float)farPct() * 0.01f, mc = fam::persMul(idx, 1);
  if (RGM.tour) { w[D_TOUR] = 30.0f * mf; w[D_STELE] = 14.0f * mf; w[D_PORT] = 12.0f * mf; }
  w[D_MARE] = 26.0f; w[D_CHAMP] = 40.0f * mc; w[D_EPOUV] = 12.0f; if (beteDispo()) w[D_BETE] = 90.0f * mc;
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
  else if (dest == D_BETE) { int b = beteChoix(); if (beteVisee >= 0 && beteVisee < NBT && bt[beteVisee].on && bt[beteVisee].res < 0 && bt[beteVisee].fuite <= 0.0f) b = beteVisee; if (b < 0) return false; sp = b; tx = lim(bt[b].x + ((c.x < bt[b].x) ? -22.0f : 22.0f), MX0 + 8.0f, MX1 - 8.0f); ty = solHaut(tx); bt[b].res = (int8_t)idx; }
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
  int idx = (int)(&c - cr); if (idx < 0 || idx >= NAD || natSante(idx) > 0) return false;
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
    if (r.dest == D_BETE && r.spot < NBT) c.d = (bt[r.spot].x >= c.x) ? 1 : -1;
    if (r.dest == D_TRES) c.d = (r.spot == 0) ? 1 : -1;      // tresor : face a la porte de la tour, ou face a l'eau
  }
  if (r.cop) liaisonArrivee(c, idx);
}
static void dispute(Corbeau& c, int idx, float dt);
static void solBete(Corbeau& c, int idx, Rt& r, float dt) {      // au sol, face a une petite bete : il s'approche en sautillant, donne un coup de bec, avale
  const bool ok = (r.spot < NBT) && bt[r.spot].on && bt[r.spot].res == idx;
  Bete& b = bt[r.spot < NBT ? r.spot : 0];
  if (r.ph == 0) {      // il l'observe
    if (ok) c.d = (b.x >= c.x) ? 1 : -1;
    c.pitchC = c.pr * 0.5f; c.haC = -0.2f; c.hbC = 0.3f;
    if (r.t > 0.7f) { r.t = 0; r.n = 0; r.ph = ok ? 1 : 3; c.peck = 0; }
    return;
  }
  if (!ok && r.ph < 3) { r.ph = 3; r.t = 0; c.peck = 0; return; }      // elle a disparu : il picore a vide, puis repart
  if (r.ph == 1) {      // approche : marche et petits sauts jusqu'a une longueur de bec
    c.d = (b.x >= c.x) ? 1 : -1;
    const float gx = b.x - c.d * 17.0f * c.s, dxm = gx - c.x;
    if (fabsf(dxm) < 2.0f || r.t > 7.0f) { r.ph = 2; r.t = 0; c.peck = 0; r.tw = 0.0f; }
    else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 40.0f * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f)); c.hop = -fabsf(sinf(c.t * 10.0f)) * 2.4f * c.s; c.bob = c.hop; c.pitchC = c.pr * 0.55f; c.haC = c.pitchC - 0.2f; c.hbC = c.haC + 0.5f; }
    return;
  }
  if (r.ph == 2) {      // coup de bec
    c.peck += dt; const float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f));
    c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
    if (c.peck > 0.24f && r.tw < 0.5f) {
      r.tw = 1.0f;
      if (b.kind == BT_LEZ && r.n < 2 && random(100) < 40) { b.fuite = 0.45f; b.dir = (b.x >= c.x) ? 1 : -1; r.n++; r.ph = 1; r.t = 0; c.peck = 0; son(SN_CROC, 120); return; }      // le lezard detale : il le poursuit
      son(SN_CROC, 200); b.on = false; b.res = -1; b.rep = (RGM.bet >= 2) ? rnd(20.0f, 40.0f) : rnd(35.0f, 70.0f);      // pris
      { Rt sv = r; r.away = false; r.cop = r.seq = r.grp = 0; r.tl = 0.0f;
        if (fam::detourner(c, idx)) { c.s = sv.sB; return; }      // un parent la garde pour le petit
        r = sv; }
      r.ph = 4; r.t = 0; r.tw = 0.0f; r.n = b.kind; return;
    }
    if (c.peck > 1.3f) { r.ph = 1; r.t = 0; c.peck = 0; r.tw = 0.0f; }
    return;
  }
  if (r.ph == 4) {      // il avale : la tete se leve, le bec claque
    const float u = lim(r.t / 1.3f, 0.0f, 1.0f); c.haC = lerpf(0.4f, -0.5f, pas(u * 2.0f)); c.hbC = c.haC + 0.6f; c.pitchC = c.pr * 0.45f - 0.35f * pas(u * 2.0f) * 0.4f; c.bec = mxf(c.bec, (sinf(r.t * 22.0f) > 0.0f && u < 0.7f) ? 0.7f : 0.0f);
    if (r.tw < 0.5f && r.t > 0.5f) { r.tw = 1.0f; mangeBete(c, idx, (int)r.n); son(SN_GAZ, 150); }
    if (r.t > 1.4f) apres(c, idx);
    return;
  }
  { c.peck += dt; const float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f)); c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip; if (r.t > 1.3f) apres(c, idx); }      // rien a manger : il picore un instant
}
// ---------- tresors caches (3.1.26) ----------
// De temps en temps un corbeau decouvre un objet brillant au sol (bague, eclat), le prend au bec, s'envole au loin et le depose a l'interieur de la tour en ruine (le plus souvent)
// ou dans la mare (parfois), puis revient a l'arbre. La cache grandit (compteurs en memoire, cle cbcach) et se voit en vue Vieux batiments / Mare : reflets dans l'ombre de la porte, eclats dans l'eau.
// Reglage Tresors : non / rares / frequents (cle cbtres, page Monde). Cout : un objet au bec, au plus 9 + 6 petits points brillants, rien en decor minimal ni pendant l'allegement automatique.
static bool cSale = false; static float tCSale = 0.0f;
static int8_t tK = -1, tObj = -1; static uint8_t tPh = 0, tOu = 0; static float tT = 0.0f, gTres = 60.0f;      // tK : le corbeau en mission ; tPh 0 il va prendre l'objet, 1 il le porte ; tOu 0 tour, 1 mare
static void tresFlush() { if (cSale) { prefs.putUInt("cbcach", (uint32_t)cTour | ((uint32_t)cMare << 16)); cSale = false; } }
static bool tresorDepart(Corbeau& c, int idx) {      // appele quand le corbeau vient de ramasser l'objet : il s'envole vers la cache (renvoie faux : ce n'est pas un tresor, la routine normale continue)
  // 3.1.30 (precision de Hades) : un corbeau qui tient quelque chose de brillant (bague, eclat : trouve, lance, rapporte) l'emmene TOUJOURS a la tour, a chaque fois ; la mare seulement si le decor Tour est coupe
  if (idx < 0 || idx >= NAD || c.objet == 0 || !(c.objet == T_BRILLANT + 1 || c.objet == T_BAGUE + 1) || rt[idx].away || c.duvet >= 0.5f) return false;
  if (tK >= 0 && tK != idx) return false;      // un seul voyage de tresor a la fois
  if (tK == idx && tPh != 0) return false;
  tK = (int8_t)idx; tOu = RGM.tour ? 0 : 1;
  Rt& r = rt[idx]; const float tx = (tOu == 0) ? TRX - 11.0f : PX + PRX + 5.0f, ty = solHaut(tx);
  r.sB = c.s; r.hops = 0; r.away = true; r.dest = D_TRES; r.spot = tOu; r.ph = 0; r.t = 0; r.n = 0; r.dur = 0; r.tw = 0; r.cop = r.seq = r.grp = 0; r.tl = 0.0f;
  c.perch = -1; c.item = -1; c.rapporte = false;
  decoller(c, tx, ty, B_MONDE);
  tPh = 1; tT = 0.0f; tObj = -1; return true;
}
static void solTres(Corbeau& c, int idx, Rt& r, float dt) {      // au pied de la porte de la tour ou au bord de l'eau, l'objet brillant au bec
  const bool tour = (r.spot == 0);
  if (r.ph == 0) {      // il regarde autour de lui
    c.pitchC = c.pr * 0.5f; c.haC = -0.2f; c.hbC = 0.3f;
    if (r.t > 0.7f) { r.ph = tour ? 1 : 2; r.t = 0; c.peck = 0; }
    return;
  }
  if (r.ph == 1) {      // tour : il marche jusqu'a la porte
    const float dxm = (TRX - 8.0f) - c.x; c.d = 1;
    if (fabsf(dxm) < 1.8f || r.t > 8.0f) { r.ph = 2; r.t = 0; c.peck = 0; }
    else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 34.0f * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f)); c.hop = -fabsf(sinf(c.t * 10.0f)) * 2.4f * c.s; c.bob = c.hop; c.pitchC = c.pr * 0.55f; c.haC = c.pitchC - 0.2f; c.hbC = c.haC + 0.5f; }
    return;
  }
  if (r.ph == 2) {      // il depose l'objet : la tete plonge vers l'ombre de la porte (ou vers l'eau), le bec s'ouvre
    c.peck += dt; const float dip = fabsf(sinf(lim(c.peck / 0.9f, 0.0f, 1.0f) * 3.1416f));
    c.haC = lerpf(-0.3f, 0.7f, dip); c.hbC = lerpf(0.2f, 0.95f, dip); c.pitchC = c.pr * 0.45f - 0.34f * dip;
    if (c.peck > 0.45f && c.objet) {
      c.objet = 0; son(tour ? SN_CROC : SN_GAZ, 170); c.flash = 0.5f;
      if (tour) { if (cTour < 9999) cTour++; eclat(TRX, TRB() - 8.0f, 0.35f, 1.1f); }
      else { if (cMare < 9999) cMare++; const float wx = c.x + c.d * 17.0f * c.s; spawnOnde(wx, PY + 0.5f); spawnGoutte(wx, PY, rnd(-25, 25), rnd(-70, -40)); spawnGoutte(wx + 3.0f, PY, rnd(-25, 25), rnd(-60, -30)); }
      cSale = true; tCSale = 6.0f;
    }
    if (c.peck > 1.0f) { r.ph = 3; r.t = 0; c.haC = -0.2f; c.hbC = 0.3f; }
    return;
  }
  c.pitchC = c.pr * 0.5f; c.haC = -0.2f; c.hbC = 0.3f;      // il regarde sa cachette un instant, puis revient a l'arbre
  if (r.t > 0.9f) retour(c, idx);
}
static void sol(Corbeau& c, int idx, float dt) {      // au sol : boire à la mare, picorer dans le champ (appelé à chaque image par majCorbeau)
  Rt& r = rt[idx]; r.t += dt;
  if (r.ph == 9) { if (r.t > 2.0f) retour(c, idx); return; }
  if (r.dest == D_DISPUTE) { dispute(c, idx, dt); return; }
  if (r.dest == D_BETE) { solBete(c, idx, r, dt); return; }
  if (r.dest == D_TRES) { solTres(c, idx, r, dt); return; }
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
  return c.visible && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && !rt[k].away && natSante(k) < 2;
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
        Gw.act = A_SECOUE; Gw.tAct = 0.0f; Gw.dAct = 0.9f; sonCri(); lienAj(S.a, S.b, -2); S.step = 3; chasser(Lz); natBagarre(aGagne ? S.a : S.b, aGagne ? S.b : S.a, 0);
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
// 3.1.38 : la nourriture n'a d'interet que pour un corbeau qui a faim ; chacun a un plat prefere selon son trait dominant
static const uint8_t FAV[5] = {T_BAIES, T_GRAINE, T_NOIX, T_PAIN, T_VIANDE};      // audacieux : baies, calme : graines, curieux : noix, sociable : pain, costaud : viande
static int favori(int k) { const int m = fam::dominant(k); return (m >= 0 && m < 5) ? (int)FAV[m] : -1; }
static float envie(int k, int type) {      // 0 = ignore (rassasie), 1 = interesse, 1,6 = son plat prefere ; un objet brillant ne depend pas de la faim (gout du tresor) ; le petit et un adulte dont le petit a faim sont toujours preneurs
  if (type >= T_BRILLANT || k < 0 || k >= NAD) return 1.0f;
  if (fam::faimPetit() > 0.55f) return 1.0f;
  const bool fav = (favori(k) == type);
  if (en[k] >= (fav ? 0.80f : 0.60f)) return 0.0f;
  return fav ? 1.6f : 1.0f;
}
static void mange(Corbeau& c, int idx, int type) {      // chaque nourriture a un effet réel et visible
  if (idx < 0 || idx >= NAD) return;
  natNourri(idx);
  if (favori(idx) == type) { en[idx] += 0.05f; if (tJoie[idx] < 8.0f) tJoie[idx] = 8.0f; c.hum = 1.0f; }      // son plat prefere : plus de joie
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
static void mangeBete(Corbeau& c, int idx, int kind) {      // nourriture naturelle : l'escargot rassasie un peu, le lezard donne de la force, le scarabee de la bonne humeur
  if (idx < 0 || idx >= NAD) return;
  natNourri(idx);
  if (kind == BT_LEZ) { en[idx] += 0.26f; tHer[idx] = mxf(tHer[idx], 10.0f); c.flash = 0.35f; }
  else if (kind == BT_SCA) { en[idx] += 0.09f; tJoie[idx] = mxf(tJoie[idx], 8.0f); c.hum = 1.0f; }
  else en[idx] += 0.14f;
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
        o.reserve = idx; c.item = iO; c.but = B_NOUR; c.sub = 0; c.peck = 0.0f; lienAj(idx, W, -1); natBagarre(idx, W, 1);
        Corbeau& w = cr[W]; w.item = -1; w.but = B_RIEN; w.sub = 2; w.peck = 0.0f; w.t = 0.0f; w.surs = 0.3f;
        finR(r); c.s = r.sB; r.ph = 0;
      } else { lienAj(idx, W, -1); natBagarre(W, idx, 1); retour(c, idx); }
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
  RGM.bet = prefs.getUChar("cbbet", 1); if (RGM.bet > 2) RGM.bet = 1;
  RGM.tres = prefs.getUChar("cbtres", 1); if (RGM.tres > 2) RGM.tres = 1;
  RGM.exc = prefs.getUChar("cbexc", 0); if (RGM.exc > 2) RGM.exc = 0;
  { const uint32_t v = prefs.getUInt("cbcach", 0); cTour = (uint16_t)(v & 0xFFFFu); cMare = (uint16_t)(v >> 16); }
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
  return r.away && c.but == B_MONDE && c.visible && (c.etat == E_PERCHE || c.etat == E_SOL) && r.ph < 8 && r.dest != D_DISPUTE && r.dest != D_BETE;
}
static bool arbreOkL(int k) { const Corbeau& c = cr[k]; return !rt[k].away && c.visible && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN; }
static bool dispo(int k) { return fam::libre(k) && !rt[k].away; }
static int nLibres(int d) { if (d == D_CHAMP) return 9; int n = 0; for (int s = 0; s < nSp(d); s++) if (!spotsPris(d, s)) n++; return n; }
static void camPour(float dur) {      // la vue large (ou le suivi) accompagne l'échange, une fois sur deux environ
  if (!pret || !OP.cauto || vueFixe() || panneau || cmFin || cm == CM_SUIVI || millis() - tToucher < 5000UL || random(100) >= 60) return;
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
static void chasseBetes(float dt) {      // un corbeau de l'arbre repere une petite bete : il tourne la tete vers elle, puis part l'attraper
  if (repK >= 0) {
    Corbeau& c = cr[repK]; Bete& b = bt[repB];
    repT -= dt;
    if (!b.on || b.res >= 0 || c.etat != E_PERCHE || rt[repK].away || c.but != B_RIEN || c.perch < 0 || c.perch >= 10) { repK = -1; return; }
    c.d = (b.x >= c.x) ? 1 : -1; c.act = A_REGARDE; c.tAct = 0.0f; c.dAct = 1.0f;
    if (repT <= 0.0f) { int k = repK; repK = -1; if (nbAilleurs() < 2) { beteVisee = repB; excursion(cr[k], k, D_BETE); beteVisee = -1; } }
    return;
  }
  gBete -= dt;
  if (gBete > 0.0f) return;
  gBete = rnd(11.0f, 22.0f) / actFreq;
  if (nbBetes() == 0 || !beteDispo() || V.ph != V_RIEN || GP.ph || appelOn || panneau || nbAilleurs() >= 2) return;
  int l[NAD], n = 0;
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (!rt[k].away && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && fam::libre(k)) l[n++] = k; }
  if (!n) return;
  repK = (int8_t)pickPond(l, n, 1); repT = rnd(0.9f, 1.5f); repB = (int8_t)beteChoix(); if (repB < 0) repK = -1;
}
static void regrouper() {      // alerte : tous les oiseaux du monde rentrent à l'arbre en se suivant, les mieux classés les premiers
  int n = 0;
  for (int k = 0; k < NAD; k++) {
    Rt& r = rt[k]; Corbeau& c = cr[k]; if (!r.away || r.dest == D_DISPUTE || c.but != B_MONDE) continue;
    if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_OFF || c.etat == E_TENU || c.etat == E_TIR) continue;
    rentrer(k, 0.15f + 0.22f * (float)rang[k]); n++;
  }
  if (n && cm == CM_NON && pret && OP.cauto && !vueFixe() && !panneau && millis() - tToucher > 5000UL && random(100) < 50) { demarrer(CM_LARGE, true); cmDur = 10.0f; }
}
static void decorChange() {      // le décor passe à « simple » : ceux qui sont sur la tour, le portique ou les stèles rentrent
  if (LG.on) LG.on = false;
  if (RGM.tour) return;
  for (int k = 0; k < NAD; k++) { Rt& r = rt[k]; if (r.away && (r.dest == D_TOUR || r.dest == D_STELE || r.dest == D_PORT)) rentrer(k, 0.1f * (float)k); }
}
static void finLng() { LG.on = false; gLng = rnd(18.0f, 34.0f) * lngMul(); }
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
      int cand[4], nc = 0; if (RGM.tour && random(100) < farPct()) { if (nLibres(D_TOUR) >= 2) cand[nc++] = D_TOUR; if (nLibres(D_STELE) >= 2) cand[nc++] = D_STELE; if (nLibres(D_PORT) >= 2) cand[nc++] = D_PORT; } if (nLibres(D_MARE) >= 2) cand[nc++] = D_MARE;
      if (b < 0 || nc == 0) return;
      int d1 = cand[random(nc)], d2 = (d1 == D_MARE) ? D_CHAMP : D_MARE, sq = (random(100) < 55) ? d2 + 1 : 0;
      rt[a].cop = (uint8_t)(b + 1); rt[b].cop = (uint8_t)(a + 1); rt[a].seq = rt[b].seq = (uint8_t)sq; rt[a].grp = rt[b].grp = 1;
      bool oka = excursion(cr[a], a, d1), okb = oka && excursion(cr[b], b, d1);
      if (!okb) { rt[a].cop = rt[b].cop = 0; rt[a].seq = rt[b].seq = 0; rt[a].grp = rt[b].grp = 0; if (oka) {} return; }
      n.a = (int8_t)a; n.b = (int8_t)b; son(SN_AILES, 200); camPour(16.0f);
    } break;
    default: {      // tournée de groupe : arbre, tour (ou épouvantail), puis mare
      int d1 = (RGM.tour && random(100) < farPct()) ? D_TOUR : D_EPOUV; int m = nLibres(d1); if (m > nt) m = nt; if (m > 3) m = 3; if (m < 2) return;
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
    if (gLng <= 0.0f) { gLng = rnd(18.0f, 34.0f) * lngMul(); if (RGM.lng && pret && calme && tAlerte <= 0.0f && AP.n == 0 && !S.on) debutLng(); }
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
static void cibler(int b) { if (pret && cm == CM_SUIVI && !cmFin && b >= 0 && b < NAD && cr[b].visible) { suivi = (int8_t)b; tPerdu = 0.0f; tRetour = 0.0f; suiviRet = -1; } }      // le corbeau touché devient la cible du suivi
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
    if (suiviRet >= 0 && OP.recul == 0) {      // la vue suivie revient sur le même corbeau dès que l'événement est fini
      if (bloque || panneau) tReculRe = 0.0f;
      else { tReculRe += dt; if (tReculRe > 1.5f) { tReculRe = 0.0f; suivi = suiviRet; demarrer(CM_SUIVI, false); } }
    }
    if (OP.recul > 0 && !reculLent) {      // vue reculée tenue : elle revient dès que l'événement (gros plan, visiteur, main, appel) est fini
      if (bloque || panneau) tReculRe = 0.0f;
      else { tReculRe += dt; if (tReculRe > 1.5f && millis() - tToucher > 1200UL) { tReculRe = 0.0f; demarrer(CM_LARGE, false); } }
    }
    gLarge -= dt * actFreq;
    if (gLarge <= 0.0f) { gLarge = rnd(50.0f, 90.0f); if (OP.cauto && !vueFixe() && !bloque && !panneau && !cbAbsent && millis() - tToucher > 5000UL) demarrer(CM_LARGE, true); }
    if (cm == CM_NON) return false;
  }
  cmT += dt;
  { static int lent = 0;      // si l'image prend trop de temps (vue recomposée), on referme la caméra et on arrête les vues spontanées
    if (msDessin > 80) { if (++lent > 20 && cmT > 2.0f && !cmFin) { lent = 0; if (cmAuto) { fermer(); gLarge = 1e8f; } else qDeg = true; } } else if (lent > 0) lent--; }
  if (bloque && !cmFin && !(GP.ph && V.ph == V_RIEN && !H.actif && !appelOn)) { if (cm == CM_SUIVI && !cmAuto && suivi >= 0) suiviRet = suivi; fermer(); }      // 3.1.39 : pendant un gros plan (corbeau a la vitre) la vue ne bouge plus : l'arriere-plan reste fixe, pas de recomposition ; la vue se ferme seulement pour la main, un visiteur ou un appel      // événement voulu par l'utilisateur (main, appel, visiteur) : la vue le laisse passer puis revient
  float tz = ZN, tx = 160.0f, ty = 128.0f;
  if (!cmFin) {
    if (cm == CM_LARGE) {
      tz = 0.5f; tx = 190.0f + 28.0f * sinf(cmT * 0.28f); ty = 118.0f;      // vue large automatique (vue normale choisie)
      if (OP.recul >= 1) { tz = VUE_Z[OP.recul]; tx = VUE_X[OP.recul]; ty = VUE_Y[OP.recul]; }      // vue par lieu : fixe, la caméra ne bouge plus
      else if (LG.on && LG.a >= 0 && LG.b >= 0) { const float xa = cr[LG.a].x, xb = cr[LG.b].x; tx = lerpf(tx, (xa + xb) * 0.5f, 0.85f); tz = (fabsf(xa - xb) < 190.0f) ? 0.6f : 0.5f; }      // l'échange à distance est au centre de l'image
      if (cmAuto && cmT > cmDur) fermer();
    } else {
      Corbeau& c = cr[suivi < 0 ? 0 : suivi]; Rt& r = rt[suivi < 0 ? 0 : suivi];
      bool ok = (suivi >= 0) && c.visible && c.etat != E_DEHORS && c.etat != E_POOF && c.etat != E_OFF;
      if (!ok) { tPerdu += dt; if (tPerdu > 1.5f) { int k2 = cmAuto ? -1 : choisirSuivi(); if (k2 >= 0 && k2 != suivi) { suivi = (int8_t)k2; tPerdu = 0.0f; } else if (cmAuto) finSuivi(bloque); } } else tPerdu = 0.0f;
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
          if (cmAuto && cmT > 2.6f && tRetour > 2.6f && cmT < 8.0f && nbAilleurs() < 3) { int d = choisirDest(suivi); if (d >= 0) excursion(c, suivi, d); }      // il part en excursion : la caméra le suit
          if (cmAuto && tRetour > 4.5f && cmT > 8.0f) finSuivi(bloque);      // revenu à l'arbre : le suivi s'arrête
        } else tRetour = 0.0f;
      }
    }
  }
  if (cm == CM_SUIVI && OP.recul == 4) tz = mxf(0.5f, tz * 0.72f);      // le suivi recule aussi quand la vue large est choisie
  bornes(tz, tx, ty);
  ressort(cz, czv, tz, 3.6f, dt); ressort(cxx, cxv, tx, 3.4f, dt); ressort(cyy, cyv, ty, 3.4f, dt);
  if (cmFin && fabsf(cz - ZN) < 0.003f && fabsf(cxx - 160.0f) < 0.6f && fabsf(cyy - 128.0f) < 0.6f) {
    cm = CM_NON; cmFin = false; suivi = -1; cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f;
    zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); majBouton(); return false;
  }
  zs = cz; ztx = 160.0f - cxx * cz; zty = 128.0f - cyy * cz;
  return true;
}
static void afficher(M5Canvas& d, M5Canvas& s, int w, int h, float ox, float oy, float f) {
  if (ox >= 320.0f || oy >= (float)H_SCENE || ox + w * f <= 0.0f || oy + h * f <= 24.0f) return;
  if (fabsf(f - 1.0f) < 0.004f) s.pushSprite(&d, (int)lroundf(ox), (int)lroundf(oy), CLE_TR);
  else s.pushRotateZoom(&d, ox, oy, 0.0f, f, f, CLE_TR);
}
static void composerPar(M5Canvas& d, int y0, int y1, float czS, float cxS, float cyS) {      // le paysage vu par la caméra (rangées y0 à y1 de d) : ciel fixe puis UN SEUL plan principal ; la caméra est donnée par (czS, cxS, cyS)
  const float zsS = czS, ztxS = 160.0f - cxS * czS, ztyS = 128.0f - cyS * czS, f = czS / ZN;
  const bool bande = !(y0 <= 0 && y1 >= H_SCENE);
  if (bande) d.setClipRect(0, y0, 320, y1 - y0);
  memcpy((uint8_t*)d.getBuffer() + (size_t)y0 * 640, (const uint8_t*)sCiel.getBuffer() + (size_t)y0 * 640, (size_t)(y1 - y0) * 640);
  afficher(d, sNear, WL, HN, ztxS + MX0 * zsS, ztyS + NY0 * zsS, f);
  { int yb = (int)floorf(ztyS + NY1 * zsS) - 1; if (yb < H_SCENE) d.fillRect(0, yb, 320, H_SCENE - yb, C(5, 2, 4)); }
  if (bande) d.clearClipRect();
  vigAppliquer(d, y0, y1);      // le décor animé (lanterne, brume, halos, eau) est dessiné ensuite par dessinerEntites, par petits rectangles
  if (y0 <= 0) memcpy(d.getBuffer(), fond.getBuffer(), (size_t)24 * 320 * 2);
}
static void composer() { composerPar(cv, 0, H_SCENE, cz, cxx, cyy); }

// ---------- mise à jour générale (appelée par majGlobal) ----------
static void tresorDebut() {      // un corbeau perche trouve un objet brillant au sol : il va le prendre
  int l[NAD], n = 0;
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (!rt[k].away && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && fam::libre(k)) l[n++] = k; }
  if (!n) return;
  int j = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { j = i; break; }
  if (j < 0) return;
  const int k = pickPond(l, n, 2); static const uint8_t TT[4] = {T_BRILLANT, T_BAGUE, T_BRILLANT, T_BAGUE};
  Obj& o = ob[j]; o.actif = true; o.type = TT[random(4)]; o.x = rnd(100.0f, 240.0f); o.y = solHaut(o.x) - 3.2f; o.vy = 0; o.t = 0; o.repos = true; o.reserve = -1;
  Corbeau& c = cr[k]; c.d = (o.x >= c.x) ? 1 : -1; c.look = lim(atan2f(solHaut(o.x) - c.y, fabsf(o.x - c.x) + 8.0f) * 0.7f, -0.3f, 0.8f); c.tLook = 1.0f;
  tK = (int8_t)k; tObj = (int8_t)j; tPh = 0; tT = 0.0f; tOu = RGM.tour ? 0 : 1;
  attribuer(j, k);
  if (cr[k].but != B_NOUR) { o.actif = false; tK = -1; tObj = -1; }
}
static void majTresors(float dt) {
  if (tK >= 0) {
    tT += dt;
    if (tPh == 0) { if (tT > 30.0f || cr[tK].etat == E_OFF) { if (tObj >= 0 && ob[tObj].actif && ob[tObj].type >= T_BRILLANT && ob[tObj].reserve == tK) { ob[tObj].actif = false; ob[tObj].reserve = -1; } tK = -1; tObj = -1; } }
    else if (!(rt[tK].away && rt[tK].dest == D_TRES) || tT > 120.0f) tK = -1;
  }
  if (cSale) { tCSale -= dt; if (tCSale <= 0.0f && tK < 0) tresFlush(); }      // ecrit en memoire une fois le corbeau revenu, jamais pendant le voyage
  if (tK >= 0 || RGM.tres == 0) return;
  gTres -= dt * actFreq; if (gTres > 0.0f) return;
  gTres = (RGM.tres == 1) ? rnd(120.0f, 220.0f) : rnd(40.0f, 90.0f);
  if (decLvl() >= 2 || cbAbsent || V.ph != V_RIEN || GP.ph || appelOn || panneau || H.actif || nbAilleurs() >= 2 || repK >= 0 || LG.on || S.on) return;
  tresorDebut();
}
static void maj(float dt) {
  if (!pret) return;
  tMonde += dt;
  for (int i = 0; i < 4; i++) if (onde[i].on) { onde[i].t += dt; if (onde[i].t > 1.3f) onde[i].on = false; }
  for (int i = 0; i < 14; i++) if (gout[i].on) { Goutte& q = gout[i]; q.t += dt; q.vy += 260.0f * dt; q.x += q.vx * dt; q.y += q.vy * dt; if (q.t > 1.2f || q.y > PY + 2.0f) q.on = false; }
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; Rt& r = rt[k]; if (!r.away) continue;
    if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_OFF || c.etat == E_TENU || c.etat == E_TIR) { finR(r); c.s = r.sB; continue; }
    if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but != B_MONDE) { finR(r); continue; }      // de retour sur l'arbre
    if (r.cop && r.tl > 0.0f) { r.tl -= dt; if (r.tl <= 0.0f) { r.tl = 0.0f; finDuo(c, k); } }      // le duo se sépare du site
    float tgt = r.sB * echelleX(c.x); c.s += (tgt - c.s) * mnf(1.0f, dt * 4.0f);      // perspective : plus petit vers la maison, plus grand vers la mare
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
  majBetes(dt); chasseBetes(dt); majTresors(dt);
  for (int k = 0; k < NAD; k++) {      // énergie qui baisse, gros après la viande, joie des baies, sieste quand il est rassasié
    Corbeau& c = cr[k]; en[k] = mxf(0.0f, en[k] - 0.004f * dt); if (tHer[k] > 0.0f) tHer[k] -= dt; if (tJoie[k] > 0.0f) tJoie[k] -= dt;
    if (!rt[k].away && c.etat != E_DEHORS && c.etat != E_OFF) { float tg = sBase[k] * (1.0f + 0.07f * lim(tHer[k] / 3.0f, 0.0f, 1.0f)) * ((c.perch == 14 && c.but == B_VISITEUR) ? 1.28f : 1.0f); if (fabsf(c.s - tg) > 0.002f) c.s += (tg - c.s) * mnf(1.0f, dt * 2.0f); }      // 3.1.38 : l'oiseau posé dans la paume est plus gros (x1,28 : bien visible, en proportion de la grande main)
    if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN) {
      if (tJoie[k] > 0.0f && random(1000) < 6) { sonCri(); c.surs = 0.25f; c.bec = 1.0f; }
      if (en[k] > 0.85f && c.act <= A_REGARDE && random(1000) < 5) { c.act = A_DORT; c.tAct = 0.0f; c.dAct = rnd(9.0f, 15.0f); en[k] = 0.74f; }
    }
    if (rt[k].away && rt[k].dest == D_DISPUTE && rt[k].ph == 0 && c.etat != E_SOL) { rt[k].tw += dt; if (rt[k].tw > 6.0f) rt[k].ph = 3; }
  }
  gTrip -= dt;
  if (gTrip <= 0.0f) {      // de temps en temps, un corbeau de l'arbre part vivre sa vie ailleurs
    gTrip = rnd(16.0f, 32.0f) / actFreq;
    if (V.ph == V_RIEN && !GP.ph && !appelOn && !panneau && nbAilleurs() < 2) {
      int l[NAD], n = 0;
      for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (!rt[k].away && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f) l[n++] = k; }
      if (n) {
        const int k = pickPond(l, n, 3), d = choisirDest(k);
        if (d >= 0) {
          int a = -1;      // un sociable part au champ avec son meilleur ami (graines a deux)
          if (d == D_CHAMP && n >= 2 && nbAilleurs() == 0 && fam::persMul(k, 4) > 0.55f && random(100) < 45) { int bl = 0; for (int i = 0; i < n; i++) { const int q = l[i]; if (q != k && lien[k][q] > bl) { bl = lien[k][q]; a = q; } } }
          if (a >= 0) {
            rt[k].cop = (uint8_t)(a + 1); rt[a].cop = (uint8_t)(k + 1); rt[k].grp = rt[a].grp = 1; rt[k].seq = rt[a].seq = 0;
            const bool o1 = excursion(cr[k], k, d), o2 = o1 && excursion(cr[a], a, d);
            if (!o2) { rt[k].cop = rt[a].cop = 0; rt[k].grp = rt[a].grp = 0; }
          } else excursion(cr[k], k, d);
        }
      }
    }
  }
}
static void reinit() {
  chargerMon(); beteInit(); tK = -1; tObj = -1; tPh = 0; gTres = rnd(45.0f, 90.0f); memset(rt, 0, sizeof(rt)); memset(onde, 0, sizeof(onde)); memset(gout, 0, sizeof(gout)); memset(halo, 0, sizeof(halo)); memset(&LG, 0, sizeof(LG)); memset(&S, 0, sizeof(S)); gSoc = rnd(5.0f, 9.0f); gLng = rnd(14.0f, 24.0f); tAlertePrec = 0.0f;
  for (int k = 0; k < NAD; k++) { en[k] = 0.4f; tHer[k] = tJoie[k] = 0.0f; forceK[k] = 0; sBase[k] = cr[k].s; }
  for (int k = 0; k < NAD; k++) rang[k] = (int8_t)k;
  for (int k = NAD - 1; k > 0; k--) { int j = random(k + 1); int8_t q = rang[k]; rang[k] = rang[j]; rang[j] = q; }
  for (int i = 0; i < NAD; i++) for (int j = i; j < NAD; j++) lien[i][j] = lien[j][i] = (i == j) ? 0 : (int8_t)(random(5) - 2);
  reculLent = false; tReculRe = 0.0f; suiviRet = -1; cm = CM_NON; cmFin = false; suivi = -1; lpBird = -1; cz = ZN; cxx = 160.0f; cyy = 128.0f; czv = cxv = cyv = 0.0f;
  gLarge = rnd(40.0f, 70.0f); gTrip = rnd(10.0f, 20.0f); tMonde = 0.0f;
}
static void fermerMonde(bool garder = false) { tresFlush(); tK = -1; tObj = -1; if (garder) { pret = false; tentee = false; } else { libererSprites(); tentee = false; } cm = CM_NON; cmFin = false; suivi = -1; memset(rt, 0, sizeof(rt)); memset(halo, 0, sizeof(halo)); memset(&LG, 0, sizeof(LG)); }

}  // namespace mo
