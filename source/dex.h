#pragma once
// ======================= DEX DES YEUX (page 33) =======================
// Le Dex complet : 26 yeux (textes dans dex_textes.h, faits vérifiés), liste défilante par familles,
// fiche en 4 sections (Histoire / Porteurs / Pouvoirs / Le saviez-vous ?), écran « créer ton œil »
// et bouton qui change les yeux des corbeaux. Les motifs sont dessinés par le code (triangles, cercles).
// Rien n'est redessiné tant que rien ne bouge : le toucher reste immédiat.

extern uint32_t finAffVolume;
#if __has_include("sons_pouvoirs.h")
#include "sons_pouvoirs.h"
#define DX_SONS 1
#endif
#include "dex_textes.h"
#if __has_include("narr_data.h")
#include "narr_data.h"
#define DX_NARR 1
#endif

namespace dx {

static const int LH = 13, TXH = 101, TXY = 99;      // hauteur de ligne, hauteur et haut du cadre de texte
static const int LARG = 288;                         // largeur utile du texte

static int scL = 0, scLmax = 0, hListe = 0; static float flingV = 0; static uint32_t tFling = 0;
static int scP = 0, scPmax = 0;      // défilement de la liste, du texte de la fiche
static int zone = 0, sc0 = 0; static bool defile = false; static float vy = 0;
static int ecran = 0;              // 0 liste, 1 fiche, 2 sur le corbeau, 3 création
static int sel = 0;                // œil choisi (0..NE-1 = Dex, NE = ton œil)
static int tab = 0;                // section de la fiche : 0 Histoire, 1 Porteurs, 2 Pouvoirs, 3 Le saviez-vous ?
static bool ouvert = false, sale = true, volVu = false;
static int animDe = 0, animVers = 0, animSens = 0, scAnim = 0; static uint32_t animT0 = 0;      // glissement d'une fiche à l'autre
static cb::Corbeau cbx; static bool cbxOk = false; static uint32_t tCb = 0;
// toucher
static bool bas = false; static int sx = 0, sy = 0, px = 0, py = 0;
static int cibleBt = -1;

static inline uint16_t BGc() { return C(20, 3, 8); }
static inline uint16_t CFl(float r, float g, float b) { return cb::CF(r, g, b); }
static inline float rad(float a) { return a * 0.0174533f; }
static inline void polaire(float cx, float cy, float r, float a, float& x, float& y) { x = cx + r * cosf(rad(a)); y = cy + r * sinf(rad(a)); }
static inline uint16_t ton(float r, float g, float b, float k) { return CFl(r * k, g * k, b * k); }
static inline int nbOeils() { return NE + (cb::oCu.ok ? 1 : 0); }
static inline bool decouvert(int i) { return i < NE || (i == NE && cb::oCu.ok); }

// ---------- dessin d'un œil ----------
static void virgule(M5Canvas& g, float cx, float cy, float ring, float a, float size, uint16_t c) {
  float hx, hy; polaire(cx, cy, ring, a, hx, hy); cb::disque(g, hx, hy, size, c);
  float P[7][2], Q[7][2];
  for (int i = 0; i <= 6; i++) {
    float t = i / 6.0f, ang = a - t * 65.0f, rr = ring - t * size * 0.5f, w = size * (1.0f - t) * 0.95f;
    polaire(cx, cy, rr + w, ang, P[i][0], P[i][1]); polaire(cx, cy, rr - w, ang, Q[i][0], Q[i][1]);
  }
  for (int i = 0; i < 6; i++) { cb::tri(g, P[i][0], P[i][1], P[i + 1][0], P[i + 1][1], Q[i][0], Q[i][1], c); cb::tri(g, P[i + 1][0], P[i + 1][1], Q[i + 1][0], Q[i + 1][1], Q[i][0], Q[i][1], c); }
}
static void virguleDroite(M5Canvas& g, float cx, float cy, float ring, float a, float size, uint16_t c) {      // virgule à queue droite
  float hx, hy, tx, ty; polaire(cx, cy, ring, a, hx, hy); cb::disque(g, hx, hy, size, c);
  float nx = cosf(rad(a)), ny = sinf(rad(a)), dx = cosf(rad(a - 90)), dy = sinf(rad(a - 90));
  tx = hx + dx * size * 3.0f; ty = hy + dy * size * 3.0f;
  cb::tri(g, hx + nx * size * 0.95f, hy + ny * size * 0.95f, hx - nx * size * 0.95f, hy - ny * size * 0.95f, tx, ty, c);
}
static void ruban(M5Canvas& g, float cx, float cy, float r0, float r1, float a0, float da, float w0, float w1, uint16_t c) {      // trait en spirale
  const int N = 10; float P[N + 1][2], Q[N + 1][2];
  for (int i = 0; i <= N; i++) {
    float t = (float)i / N, a = a0 + da * t, r = r0 + (r1 - r0) * t, w = w0 + (w1 - w0) * t;
    polaire(cx, cy, r + w, a, P[i][0], P[i][1]); polaire(cx, cy, r - w, a, Q[i][0], Q[i][1]);
  }
  for (int i = 0; i < N; i++) { cb::tri(g, P[i][0], P[i][1], P[i + 1][0], P[i + 1][1], Q[i][0], Q[i][1], c); cb::tri(g, P[i + 1][0], P[i + 1][1], Q[i + 1][0], Q[i + 1][1], Q[i][0], Q[i][1], c); }
}
static void cercle(M5Canvas& g, float x, float y, float r, uint16_t c) { int rr = cb::ar(r); if (rr < 1) rr = 1; g.drawCircle(cb::ar(x), cb::ar(y), rr, c); }
static void iris(M5Canvas& g, float cx, float cy, float R, int motif, float cr, float cg, float cbv, int n, float rot) {
  const uint16_t NOIR = C(12, 2, 6);
  if (motif == M_BLANC) {      // Byakugan (blanc) ou Byakugan doré (teinte donnée)
    uint16_t c1 = CFl(cr, cg, cbv), c2 = CFl(cr + 12 > 255 ? 255 : cr + 12, cg + 12 > 255 ? 255 : cg + 12, cbv + 5 > 255 ? 255 : cbv + 5);
    cb::disque(g, cx, cy, R, c1); cb::disque(g, cx, cy, R * 0.86f, c2); cercle(g, cx, cy, R, NOIR); if (R > 20) cercle(g, cx, cy, R - 1, NOIR);
    for (int k = 0; k < 4; k++) { float x, y; polaire(cx, cy, R * 0.15f, rot + 45 + k * 90, x, y); cb::disque(g, x, y, cb::mxf(0.6f, R * 0.035f), NOIR); }
    return;
  }
  if (motif == M_PET) {      // Tenseigan
    cb::disque(g, cx, cy, R, C(130, 186, 240)); cb::disque(g, cx, cy, R * 0.9f, C(150, 202, 248));
    const int NP = 14;
    for (int k = 0; k < NP; k++) {
      float a = rot + k * 360.0f / NP, L = (k % 2 == 0) ? 0.9f : 0.72f, x1, y1, x2, y2, x3, y3, x4, y4;
      polaire(cx, cy, R * 0.12f, a, x1, y1); polaire(cx, cy, R * 0.5f * L, a - 360.0f / NP * 0.45f, x2, y2); polaire(cx, cy, R * L, a, x3, y3); polaire(cx, cy, R * 0.5f * L, a + 360.0f / NP * 0.45f, x4, y4);
      cb::tri(g, x1, y1, x2, y2, x3, y3, C(248, 250, 255)); cb::tri(g, x1, y1, x3, y3, x4, y4, C(248, 250, 255));
    }
    cb::disque(g, cx, cy, R * 0.15f, C(24, 60, 130)); cercle(g, cx, cy, R, C(40, 70, 130));
    return;
  }
  if (motif == M_JOGAN) {      // Jōgan (dessin approché : iris bleu clair, petite pupille)
    cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.55f)); cb::disque(g, cx, cy, R * 0.92f, CFl(cr, cg, cbv)); cb::disque(g, cx, cy, R * 0.6f, ton(cr, cg, cbv, 1.12f));
    cb::disque(g, cx, cy, R * 0.2f, NOIR); cercle(g, cx, cy, R, NOIR);
    return;
  }
  uint16_t col = CFl(cr, cg, cbv);
  if (motif == M_ANN || motif == M_RINV) {      // Rinnegan : anneaux concentriques (+ 6 virgules pour celui de Sasuke)
    cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.7f));
    for (int i = 8; i >= 1; i--) { float r = R * i / 8.0f; cb::disque(g, cx, cy, r, ton(cr, cg, cbv, (i % 2) ? 0.75f : 1.05f)); if (R > 14) cercle(g, cx, cy, r, NOIR); }
    if (motif == M_RINV) for (int k = 0; k < 3; k++) { virgule(g, cx, cy, R * 0.30f, rot + k * 120, R * 0.075f, NOIR); virgule(g, cx, cy, R * 0.50f, rot + 60 + k * 120, R * 0.075f, NOIR); }
    cb::disque(g, cx, cy, R * 0.07f + 0.5f, NOIR); cercle(g, cx, cy, R, NOIR);
    return;
  }
  if (motif == M_RINSH) {      // Rinne Sharingan (dessin approché : ondes rouges et 9 virgules)
    cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.6f));
    for (int i = 6; i >= 1; i--) { float r = R * i / 6.0f; cb::disque(g, cx, cy, r, ton(cr, cg, cbv, (i % 2) ? 0.82f : 1.0f)); if (R > 14) cercle(g, cx, cy, r, NOIR); }
    for (int k = 0; k < 9; k++) virgule(g, cx, cy, R * 0.64f, rot + k * 40, R * 0.095f, NOIR);
    cb::disque(g, cx, cy, R * 0.12f, NOIR); cercle(g, cx, cy, R, NOIR);
    return;
  }
  if (motif == M_FLEUR || motif == M_FLEUR3) {      // Mangekyō de Sasuke (fleur à 6 pétales) et éternel (+ étoile noire à 3 pointes)
    cb::disque(g, cx, cy, R, C(14, 6, 10)); cercle(g, cx, cy, R, NOIR);
    for (int k = 0; k < 6; k++) {
      float a = rot + k * 60, nx = cosf(rad(a + 90)), ny = sinf(rad(a + 90)), L1x, L1y, R1x, R1y, L2x, L2y, R2x, R2y, b0x, b0y, tx, ty, x, y;
      polaire(cx, cy, R * 0.12f, a, b0x, b0y); polaire(cx, cy, R * 0.92f, a, tx, ty);
      float w1 = sinf(3.1416f * 0.3f) * R * 0.22f, w2 = sinf(3.1416f * 0.6f) * R * 0.22f;
      polaire(cx, cy, R * 0.12f + R * 0.78f * 0.3f, a, x, y); L1x = x + nx * w1; L1y = y + ny * w1; R1x = x - nx * w1; R1y = y - ny * w1;
      polaire(cx, cy, R * 0.12f + R * 0.78f * 0.6f, a, x, y); L2x = x + nx * w2; L2y = y + ny * w2; R2x = x - nx * w2; R2y = y - ny * w2;
      cb::tri(g, b0x, b0y, L1x, L1y, R1x, R1y, col); cb::tri(g, L1x, L1y, R1x, R1y, L2x, L2y, col); cb::tri(g, R1x, R1y, L2x, L2y, R2x, R2y, col); cb::tri(g, L2x, L2y, R2x, R2y, tx, ty, col);
    }
    if (motif == M_FLEUR3) {
      for (int k = 0; k < 3; k++) {
        float a = rot + 30 + k * 120, x1, y1, x2, y2, x3, y3;
        polaire(cx, cy, R * 0.09f, a - 90, x1, y1); polaire(cx, cy, R * 0.09f, a + 90, x2, y2); polaire(cx, cy, R * 0.5f, a, x3, y3);
        cb::tri(g, x1, y1, x2, y2, x3, y3, C(14, 6, 10));
      }
      cb::disque(g, cx, cy, R * 0.1f, col);
    } else cb::disque(g, cx, cy, R * 0.16f, C(14, 6, 10));
    return;
  }
  cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.75f)); cb::disque(g, cx, cy, R * 0.94f, col);
  if (motif == M_VIRG || motif == M_VIRGD) {      // Sharingan : virgules courbes ou droites
    cercle(g, cx, cy, R * 0.6f, NOIR); cb::disque(g, cx, cy, R * 0.2f, NOIR);
    for (int i = 0; i < n; i++) {
      float a = rot + i * 360.0f / (n > 0 ? n : 1), sz = (n <= 3) ? R * 0.17f : R * 0.13f;
      if (motif == M_VIRGD) virguleDroite(g, cx, cy, R * 0.6f, a, sz, NOIR); else virgule(g, cx, cy, R * 0.6f, a, sz, NOIR);
    }
  } else if (motif == M_PALES) {      // Mangekyō d'Itachi : 3 larges pales courbes
    for (int k = 0; k < 3; k++) {
      float a = rot + k * 120, E[16][2]; int m = 0;
      for (int i = 0; i < 8; i++) { float t = i / 7.0f; polaire(cx, cy, R * (0.32f + 0.6f * t), a - 50.0f * powf(1 - t, 1.5f), E[m][0], E[m][1]); m++; }
      for (int i = 7; i >= 0; i--) { float t = i / 7.0f; polaire(cx, cy, R * (0.32f + 0.6f * t), a + 22.0f * powf(1 - t, 0.7f), E[m][0], E[m][1]); m++; }
      for (int i = 1; i < 15; i++) cb::tri(g, E[0][0], E[0][1], E[i][0], E[i][1], E[i + 1][0], E[i + 1][1], NOIR);
    }
    cb::disque(g, cx, cy, R * 0.33f, ton(cr, cg, cbv, 1.1f));
  } else if (motif == M_LAMES) {      // Mangekyō d'Obito : 3 lames droites et un anneau
    cercle(g, cx, cy, R * 0.58f, NOIR);
    for (int k = 0; k < 3; k++) {
      float a = rot + k * 120, x1, y1, x2, y2, x3, y3;
      polaire(cx, cy, R * 0.2f, a - 48, x1, y1); polaire(cx, cy, R * 0.97f, a, x2, y2); polaire(cx, cy, R * 0.2f, a + 48, x3, y3);
      cb::tri(g, x1, y1, x2, y2, x3, y3, NOIR);
    }
    cb::disque(g, cx, cy, R * 0.14f, ton(cr, cg, cbv, 0.6f));
  } else if (motif == M_ETM) {      // Mangekyō éternel de Madara : 3 disques cerclés de noir et 3 larges bandes
    for (int k = 0; k < 3; k++) {
      float a = rot + 60 + k * 120, x1, y1, x2, y2, x3, y3, x4, y4;
      polaire(cx, cy, R * 0.16f, a - 20, x1, y1); polaire(cx, cy, R * 0.98f, a - 36, x2, y2); polaire(cx, cy, R * 0.98f, a + 36, x3, y3); polaire(cx, cy, R * 0.16f, a + 20, x4, y4);
      cb::tri(g, x1, y1, x2, y2, x3, y3, NOIR); cb::tri(g, x1, y1, x3, y3, x4, y4, NOIR);
    }
    cb::disque(g, cx, cy, R * 0.2f, NOIR);
    for (int k = 0; k < 3; k++) { float x, y; polaire(cx, cy, R * 0.6f, rot + k * 120, x, y); cb::disque(g, x, y, R * 0.21f, NOIR); cb::disque(g, x, y, R * 0.15f, ton(cr, cg, cbv, 1.05f)); }
  } else if (motif == M_SPIR) {      // spirale (Mangekyō d'Indra dans le manga, dessin approché)
    for (int k = 0; k < 3; k++) ruban(g, cx, cy, R * 0.16f, R * 0.9f, rot + k * 120, 250, R * 0.08f, R * 0.05f, NOIR);
    cb::disque(g, cx, cy, R * 0.14f, ton(cr, cg, cbv, 0.6f));
  }
  cercle(g, cx, cy, R, NOIR);
}
static void oeil(M5Canvas& g, float cx, float cy, float hw, float hh, int motif, float cr, float cg, float cbv, int n, uint16_t bg) {
  const int N = 14; float top[N + 1][2], bot[N + 1][2]; const uint16_t scl = C(236, 226, 226);
  for (int i = 0; i <= N; i++) { float t = -1 + 2.0f * i / N, k = powf(1 - t * t, 0.85f); top[i][0] = bot[i][0] = cx + hw * t; top[i][1] = cy - hh * k; bot[i][1] = cy + hh * k * 0.82f; }
  for (int i = 0; i < N; i++) { cb::tri(g, top[i][0], top[i][1], top[i + 1][0], top[i + 1][1], bot[i][0], bot[i][1], scl); cb::tri(g, top[i + 1][0], top[i + 1][1], bot[i + 1][0], bot[i + 1][1], bot[i][0], bot[i][1], scl); }
  iris(g, cx, cy, hh * 0.97f, motif, cr, cg, cbv, n, -90);
  for (int i = 0; i < N; i++) {      // paupières : on masque ce qui dépasse de l'amande
    cb::tri(g, top[i][0], cy - hh - 1, top[i + 1][0], cy - hh - 1, top[i][0], top[i][1], bg); cb::tri(g, top[i + 1][0], cy - hh - 1, top[i + 1][0], top[i + 1][1], top[i][0], top[i][1], bg);
    cb::tri(g, bot[i][0], cy + hh + 1, bot[i + 1][0], cy + hh + 1, bot[i][0], bot[i][1], bg); cb::tri(g, bot[i + 1][0], cy + hh + 1, bot[i + 1][0], bot[i + 1][1], bot[i][0], bot[i][1], bg);
  }
  g.fillRect((int)(cx - hw - 3), (int)(cy - hh - 2), 3, (int)(2 * hh + 4), bg); g.fillRect((int)(cx + hw), (int)(cy - hh - 2), 3, (int)(2 * hh + 4), bg);
}
// œil n° i (0..NE-1 = les yeux du Dex, NE = celui que l'on crée soi-même)
static void couleurOeil(int i, int& motif, float& r, float& g, float& b, int& n) {
  if (i < NE) { motif = DEF[i].motif; r = DEF[i].r; g = DEF[i].g; b = DEF[i].b; n = DEF[i].n; return; }
  static const uint8_t MOT[4] = {M_ANN, M_VIRG, M_PALES, M_FLEUR};
  motif = MOT[cb::oCu.motif & 3]; cb::teinteRVB(cb::oCu.teinte, r, g, b); n = cb::oCu.marques;
}
static const char* courtOeil(int i) { return i < NE ? DEF[i].court : "Ton œil"; }
static const char* nomOeil(int i) { return i < NE ? DEF[i].nom : "Ton œil"; }
static int suivant(int i, int sens) { int T = NE + 1; for (int k = 0; k < T; k++) { i = (i + sens + T) % T; if (decouvert(i)) return i; } return i; }

// ---------- cri du pouvoir de l'œil (touche l'œil sur la fiche, ou à l'ouverture de la fiche) ----------
static void ecouter(int i) {
#ifdef DX_SONS
  static const uint8_t* const D[5] = {POUVOIR_1, POUVOIR_2, POUVOIR_3, POUVOIR_4, POUVOIR_5};
  static const uint32_t N[5] = {POUVOIR_1_LEN, POUVOIR_2_LEN, POUVOIR_3_LEN, POUVOIR_4_LEN, POUVOIR_5_LEN};
  static const uint8_t MAPF[NF + 1] = {0, 1, 4, 3, 3, 2, 3, 0};      // un cri par famille
  if (R.volume == 0 || lectureEnCours) return;
  if (!M5.Speaker.isRunning()) { M5.Mic.end(); M5.Speaker.begin(); }
  M5.Speaker.setVolume(R.volume); M5.Speaker.setChannelVolume(5, 255);
  int p = MAPF[i < NE ? DEF[i].fam : NF]; M5.Speaker.playRaw(D[p], N[p], POUVOIRS_RATE, false, 1, 5, true);
#else
  (void)i;
#endif
}
// ---------- narration vocale de la fiche : ADPCM 4 bits décodé par petits morceaux (l'écran et le toucher ne sont jamais bloqués) ----------
#ifdef DX_NARR
static const int16_t NSTEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t NIDX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int NRC = 1024, NRCH = 6;      // échantillons par morceau, canal du haut-parleur réservé à la narration
static int16_t nrBuf[3][NRC]; static int nrSlot = 0, nrPred = 0, nrInd = 0, nrIdx = -1; static uint32_t nrPos = 0, nrDepart = 0; static bool nrOn = false, nrAffEtat = false;
static bool narrDispo(int i) { return i >= 0 && i < NARR_NB; }
static bool narrActive() { return nrOn || (M5.Speaker.isRunning() && M5.Speaker.isPlaying((uint8_t)NRCH) > 0); }
static void narrArreter() { nrOn = false; nrDepart = 0; if (M5.Speaker.isRunning()) M5.Speaker.stop((uint8_t)NRCH); }
static void narrPomper() {      // appelé à chaque image : remplit la file du haut-parleur (2 morceaux d'avance, 3 tampons tournants)
  if (!nrOn) return;
  if (!M5.Speaker.isRunning()) { nrOn = false; return; }
  for (int g = 0; g < 2 && nrOn && M5.Speaker.isPlaying((uint8_t)NRCH) < 2; g++) {
    const uint8_t* d = (const uint8_t*)NARR_P[nrIdx]; const uint32_t ns = NARR_NS[nrIdx];
    int n = 0; int16_t* b = nrBuf[nrSlot];
    while (n < NRC && nrPos < ns) {
      uint8_t by = d[nrPos >> 1]; int nib = (nrPos & 1) ? (by >> 4) : (by & 15);
      int step = NSTEP[nrInd], df = step >> 3; if (nib & 4) df += step; if (nib & 2) df += step >> 1; if (nib & 1) df += step >> 2;
      nrPred += (nib & 8) ? -df : df; if (nrPred > 32767) nrPred = 32767; if (nrPred < -32768) nrPred = -32768;
      nrInd += NIDX[nib]; if (nrInd < 0) nrInd = 0; if (nrInd > 88) nrInd = 88;
      b[n++] = (int16_t)nrPred; nrPos++;
    }
    if (n == 0) { nrOn = false; break; }
    M5.Speaker.playRaw(b, n, NARR_RATE, false, 1, NRCH, false); nrSlot = (nrSlot + 1) % 3;
    if (nrPos >= ns) { nrOn = false; break; }
  }
}
static void narrLancer(int i) {
  if (!narrDispo(i) || R.volume == 0 || lectureEnCours) return;
  narrArreter();
  if (!M5.Speaker.isRunning()) { M5.Mic.end(); M5.Speaker.begin(); }
  M5.Speaker.setVolume(R.volume); M5.Speaker.setChannelVolume(NRCH, 255);
  nrIdx = i; nrPos = 0; nrPred = 0; nrInd = 0; nrSlot = 0; nrOn = true; narrPomper();
}
static void narrPlanifier() { narrArreter(); nrDepart = millis() + 700; if (!nrDepart) nrDepart = 1; }      // la voix démarre si l'on reste un instant sur la fiche
#else
static bool narrDispo(int) { return false; } static bool narrActive() { return false; } static void narrArreter() {} static void narrPomper() {} static void narrLancer(int) {} static void narrPlanifier() {}
static uint32_t nrDepart = 0; static bool nrOn = false, nrAffEtat = false;
#endif
// ---------- petits éléments d'interface ----------
// La police de l'écran n'a ni « Œ/œ », ni les voyelles à trait (ō ū…), ni la puce « • » (elles s'affichaient en carrés) : on les remplace avant de dessiner.
// Seuls des caractères latins accentués (é è ê à â ç î ô û ù É À Ç « » °) restent : la police les dessine bien.
static String fr(const char* s) {
  String o; o.reserve(strlen(s) + 6);
  const uint8_t* p = (const uint8_t*)s;
  while (*p) {
    if (p[0] == 0xC5 && p[1] == 0x92) { const uint8_t c = p[2]; o += ((c >= 'a' && c <= 'z') || c >= 0xC3) ? "Oe" : "OE"; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0x93) { o += "oe"; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0x8C) { o += 'O'; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0x8D) { o += 'o'; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0xAA) { o += 'U'; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0xAB) { o += 'u'; p += 2; }
    else if (p[0] == 0xC4 && (p[1] == 0x80 || p[1] == 0x81)) { o += (p[1] == 0x80) ? 'A' : 'a'; p += 2; }
    else if (p[0] == 0xC4 && (p[1] == 0x92 || p[1] == 0x93)) { o += (p[1] == 0x92) ? 'E' : 'e'; p += 2; }
    else if (p[0] == 0xC4 && (p[1] == 0xAA || p[1] == 0xAB)) { o += (p[1] == 0xAA) ? 'I' : 'i'; p += 2; }
    else if (p[0] == 0xE2 && p[1] == 0x80 && (p[2] == 0x98 || p[2] == 0x99)) { o += '\''; p += 3; }
    else if (p[0] == 0xE2 && p[1] == 0x80 && p[2] == 0xA6) { o += "..."; p += 3; }
    else if (p[0] == 0xE2 && p[1] == 0x80 && (p[2] == 0x93 || p[2] == 0x94 || p[2] == 0xA2)) { o += '-'; p += 3; }
    else o += (char)*p++;
  }
  return o;
}
static void puce(M5Canvas& g, int x, int y, uint16_t c) { g.fillCircle(x + 3, y + 6, 2, c); }      // vraie puce ronde (x, y = coin haut-gauche de la ligne)
static void texte(M5Canvas& g, int x, int y, const char* s, uint16_t col, int datum = 0, bool gros = false) {
  if (gros) g.setFont(&fonts::lgfxJapanGothic_16); else g.setFont(&fonts::lgfxJapanGothic_12);
  g.setTextColor(col); g.setTextDatum(datum == 0 ? top_left : (datum == 1 ? middle_center : (datum == 2 ? middle_left : middle_right))); g.drawString(fr(s), x, y); g.setTextDatum(top_left);
}
static int bloc(M5Canvas& g, int x, int y, int w, const char* s, int lh, uint16_t col, int maxL) {      // texte coupé aux espaces ; renvoie le nombre de lignes
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(col); g.setTextDatum(top_left);
  String t = fr(s), ligne = ""; int n = 0, i = 0, L = t.length();
  while (i <= L) {
    int j = t.indexOf(' ', i); if (j < 0) j = L;
    String mot = t.substring(i, j), essai = ligne.length() ? ligne + " " + mot : mot;
    if (g.textWidth(essai.c_str()) <= w || !ligne.length()) ligne = essai;
    else { g.drawString(ligne, x, y + n * lh); n++; if (n >= maxL) return n; ligne = mot; }
    i = j + 1;
  }
  if (ligne.length() && n < maxL) { g.drawString(ligne, x, y + n * lh); n++; }
  return n;
}
static void entete(M5Canvas& g, int ox, const char* titre, const char* droite) {
  g.fillRect(ox, 0, 320, 22, C(44, 6, 14)); g.drawFastHLine(ox, 22, 320, C(214, 24, 34));
  g.setFont(&fonts::lgfxJapanGothic_16); bool petit = g.textWidth(fr(titre).c_str()) > 238;
  texte(g, ox + 8, 11, titre, C(255, 236, 230), 2, !petit); texte(g, ox + 312, 11, droite, C(255, 96, 100), 3, true);
}
static void bouton(M5Canvas& g, int x, int w, const char* lib, uint16_t fond = 0) {
  g.fillRoundRect(x, 206, w, 30, 8, fond ? fond : C(60, 8, 18)); g.drawRoundRect(x, 206, w, 30, 8, C(255, 70, 80));
  texte(g, x + w / 2, 221, lib, C(255, 236, 230), 1, true);
}

// ---------- écran 0 : la liste défilante, par familles ----------
static const int HH = 18, HE = 28, NR = 40;
static int nR = 0; static int8_t Rt[NR]; static int16_t Rv[NR], Ry[NR + 1];
static void batirListe() {      // lignes de la liste : un titre par famille, puis ses yeux
  nR = 0; int y = 0;
  for (int f = 0; f <= NF; f++) {
    bool perso = (f == NF); if (perso && !cb::oCu.ok) break;
    Rt[nR] = 0; Rv[nR] = f; Ry[nR] = y; nR++; y += HH;
    if (perso) { Rt[nR] = 1; Rv[nR] = NE; Ry[nR] = y; nR++; y += HE; }
    else for (int i = 0; i < NE; i++) if (DEF[i].fam == f) { Rt[nR] = 1; Rv[nR] = i; Ry[nR] = y; nR++; y += HE; }
  }
  Ry[nR] = y; hListe = y; scLmax = y + 4 - 180; if (scLmax < 0) scLmax = 0; if (scL > scLmax) scL = scLmax; if (scL < 0) scL = 0;
}
static void centrerListe() {
  for (int k = 0; k < nR; k++) if (Rt[k] == 1 && Rv[k] == sel) { scL = Ry[k] - 70; break; }
  if (scL > scLmax) scL = scLmax; if (scL < 0) scL = 0;
}
static void ecranListe(M5Canvas& g) {
  g.fillScreen(BGc());
  char nb[16]; snprintf(nb, sizeof(nb), "%d yeux", nbOeils()); entete(g, 0, "DEX DES YEUX", nb);
  g.setClipRect(0, 24, 320, 180);
  for (int k = 0; k < nR; k++) {
    int y = 25 + Ry[k] - scL, h = (Rt[k] == 0) ? HH : HE;
    if (y + h <= 24 || y >= 205) continue;
    if (Rt[k] == 0) {
      int f = Rv[k], c = 0; if (f < NF) for (int i = 0; i < NE; i++) if (DEF[i].fam == f) c++;
      char s[8]; snprintf(s, sizeof(s), "%d", f < NF ? c : 1);
      texte(g, 8, y + 3, f < NF ? FAMILLE[f] : "TON ŒIL", C(255, 96, 100)); texte(g, 300, y + 3, s, C(150, 70, 80), 3);
      g.drawFastHLine(6, y + 16, 296, C(90, 16, 26));
    } else {
      int i = Rv[k]; bool actif = (i == sel);
      g.fillRoundRect(4, y, 300, HE - 3, 6, C(34, 6, 14)); g.drawRoundRect(4, y, 300, HE - 3, 6, actif ? C(255, 70, 80) : C(100, 18, 28));
      int motif, n; float cr, cg, cbv; couleurOeil(i, motif, cr, cg, cbv, n);
      oeil(g, 34, y + 12, 24, 11, motif, cr, cg, cbv, n, C(34, 6, 14));
      texte(g, 66, y + 12, i < NE ? DEF[i].nom : "Ton œil", C(255, 236, 230), 2, true);
      if (i < NE && DEF[i].approx) texte(g, 296, y + 12, "~", C(150, 120, 120), 3, true);
    }
  }
  g.clearClipRect();
  if (scLmax > 0) {      // barre de défilement
    int th = 178, bh = th * 180 / (hListe + 4); if (bh < 14) bh = 14; int by = 25 + (th - bh) * scL / scLmax;
    g.fillRoundRect(312, 25, 3, th, 1, C(50, 12, 18)); g.fillRoundRect(312, by, 3, bh, 1, C(255, 96, 100));
  }
  bouton(g, 4, 102, "RETOUR"); bouton(g, 108, 102, "CRÉER"); bouton(g, 212, 102, "CORBEAUX");
}

// ---------- écran 1 : la fiche d'un œil, en sections ----------
static const char* texteSection(int idx, int t) {
  if (idx < NE) { const OeilDef& d = DEF[idx]; return t == 0 ? d.hist : (t == 1 ? d.port : (t == 2 ? d.pouv : d.sav)); }
  static const char* const P[4] = {"Cet œil est le tien : tu en as choisi la couleur, le motif et le nombre de virgules dans l'écran CRÉER.", "Toi, et tes corbeaux si tu l'actives.", "Aucun pouvoir : c'est seulement un jeu de couleurs.", "Les corneilles reconnaissent les visages humains."};
  return P[t & 3];
}
static const int MAXL = 72;
struct Mise { String t; uint16_t deb[MAXL]; uint8_t len[MAXL], fl[MAXL]; int n = 0, idx = -1, tab = -1; };      // fl : 1 = puce, 2 = retrait
static Mise mA, mB; static Mise* mc = &mA; static Mise* mt = &mB;
static void construire(Mise& m, int idx, int t) {      // coupe le texte de la section en lignes (une seule fois par section)
  m.idx = idx; m.tab = t; m.n = 0; m.t = fr(texteSection(idx, t));
  M5Canvas& g = cv; g.setFont(&fonts::lgfxJapanGothic_12);
  int L = m.t.length(), i = 0;
  while (i < L && m.n < MAXL) {
    int e = m.t.indexOf('\n', i); if (e < 0) e = L;
    bool bul = (m.t.charAt(i) == '-' && m.t.charAt(i + 1) == ' ');
    int ls = bul ? i + 2 : i, w = bul ? LARG - 10 : LARG; bool premiere = true;
    while (ls < e && m.n < MAXL) {
      int fin = ls, j = ls;
      for (;;) {
        int sp = m.t.indexOf(' ', j); if (sp < 0 || sp > e) sp = e;
        String s = m.t.substring(ls, sp);
        if (g.textWidth(s.c_str()) <= w || fin == ls) { fin = sp; if (sp >= e) break; j = sp + 1; } else break;
      }
      m.deb[m.n] = ls; m.len[m.n] = (uint8_t)(fin - ls > 250 ? 250 : fin - ls); m.fl[m.n] = bul ? (premiere ? 3 : 2) : 0; m.n++;
      premiere = false; ls = fin + 1;
    }
    i = e + 1;
  }
}
static void assurer() {
  if (mc->idx != sel || mc->tab != tab) construire(*mc, sel, tab);
  scPmax = mc->n * LH + 6 - TXH; if (scPmax < 0) scPmax = 0; if (scP > scPmax) scP = scPmax;
}
static const int TABX[4] = {5, 73, 145, 217}, TABW[4] = {66, 70, 70, 98};
static const char* const TABN[4] = {"Histoire", "Porteurs", "Pouvoirs", "Le saviez-vous ?"};
static void fiche(M5Canvas& g, int idx, int ox, int tb, int scrl, Mise& m) {
  g.fillRect(ox, 0, 320, 240, BGc());
  char num[12]; if (idx < NE) snprintf(num, sizeof(num), "%d/%d", idx + 1, NE); else num[0] = 0;
  entete(g, ox, nomOeil(idx), num);
  int motif, n; float cr, cg, cbv; couleurOeil(idx, motif, cr, cg, cbv, n);
  const uint16_t BL = C(255, 236, 230), RS = C(255, 96, 100);
  oeil(g, ox + 58, 50, 52, 24, motif, cr, cg, cbv, n, BGc());
  if (idx < NE && DEF[idx].approx) texte(g, ox + 112, 66, "~", C(150, 120, 120), 3, true);
  texte(g, ox + 120, 27, idx < NE ? FAMILLE[DEF[idx].fam] : "TON ŒIL", RS);
  if (narrDispo(idx)) { bool on = narrActive(); g.fillRoundRect(ox + 232, 24, 82, 22, 6, on ? C(90, 20, 24) : C(20, 90, 50)); g.drawRoundRect(ox + 232, 24, 82, 22, 6, C(255, 70, 80)); texte(g, ox + 273, 35, on ? "ARRÊTER" : "ÉCOUTER", C(255, 236, 230), 1); }
  bloc(g, ox + 120, 46, 192, idx < NE ? DEF[idx].qui : "Toi", 14, BL, 2);
  for (int k = 0; k < 4; k++) {      // les quatre sections
    bool on = (k == tb);
    g.fillRoundRect(ox + TABX[k], 78, TABW[k], 19, 5, on ? C(150, 22, 34) : C(40, 7, 14)); g.drawRoundRect(ox + TABX[k], 78, TABW[k], 19, 5, on ? C(255, 96, 100) : C(110, 20, 30));
    texte(g, ox + TABX[k] + TABW[k] / 2, 88, TABN[k], on ? C(255, 255, 255) : C(220, 170, 170), 1);
  }
  g.fillRoundRect(ox + 4, TXY, 312, TXH + 5, 6, C(12, 2, 6)); g.drawRoundRect(ox + 4, TXY, 312, TXH + 5, 6, C(120, 22, 32));
  g.setClipRect(ox + 6, TXY + 2, 303, TXH + 1);
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(BL); g.setTextDatum(top_left);
  for (int k = 0; k < m.n; k++) {
    int y = TXY + 3 - scrl + k * LH; if (y < TXY - LH || y > TXY + TXH + 2) continue;
    int x = ox + 12 + ((m.fl[k] & 2) ? 10 : 0);
    if (m.fl[k] == 3) puce(g, ox + 12, y, RS);
    g.setTextColor(BL); g.drawString(m.t.substring(m.deb[k], m.deb[k] + m.len[k]), x, y);
  }
  g.clearClipRect();
  int mx = m.n * LH + 6 - TXH; if (mx < 0) mx = 0;
  if (mx > 0) {      // barre de défilement + flèches
    int th = TXH - 2, bh = th * TXH / (m.n * LH + 6); if (bh < 12) bh = 12; int by = TXY + 3 + (th - bh) * (scrl > mx ? mx : scrl) / mx;
    g.fillRoundRect(ox + 310, TXY + 3, 3, th, 1, C(50, 12, 18)); g.fillRoundRect(ox + 310, by, 3, bh, 1, RS);
    if (scrl < mx) g.fillTriangle(ox + 298, TXY + TXH - 6, ox + 306, TXY + TXH - 6, ox + 302, TXY + TXH - 1, C(255, 205, 80));
    if (scrl > 0) g.fillTriangle(ox + 298, TXY + 8, ox + 306, TXY + 8, ox + 302, TXY + 3, C(255, 205, 80));
  }
  bouton(g, ox + 4, 74, "PRÉC"); bouton(g, ox + 82, 74, "LISTE"); bouton(g, ox + 160, 74, "SUIV"); bouton(g, ox + 238, 74, "CORBEAU");
}

// ---------- écran 2 : sur le corbeau ----------
static void ecranCorbeau(M5Canvas& g) {
  g.fillScreen(C(24, 3, 8));
  if (!cbxOk) { cb::initCorbeau(cbx, 2.3f); cbx.d = 1; cbx.dv = 1; cbx.pied = true; cbx.fx = 118; cbx.fy = 176; cbx.fp = 0; cbx.pr = 0.34f; cbx.pitch = 0.34f; cbxOk = true; }
  float t = millis() * 0.001f;
  cbx.resp = t; cbx.tAct = t; cbx.ha = -0.42f + 0.08f * sinf(t * 0.9f); cbx.hb = cbx.ha + 0.49f; cbx.pitch = cbx.pr + 0.02f * sinf(t * 0.9f);
  cbx.clin = ((millis() % 3600) < 130) ? 1.0f : 0.0f; cbx.bob = 0.3f * cbx.s * sinf(t * 2.7f);
  float hx, hy; cb::hancheC(cbx.s, cbx.pitch, cbx.dv, hx, hy); cbx.x = cbx.fx - hx; cbx.y = cbx.fy - cb::JAMBE * cbx.s - hy;
  int motif, n; float cr, cg, cbv; couleurOeil(sel, motif, cr, cg, cbv, n);
  float X, Y; cb::posOeil(cbx, X, Y);
  entete(g, 0, nomOeil(sel), (cb::oeilActif == sel + 1 && cb::oeilActif != 0) ? "ACTIF" : "aperçu");
  for (int k = 0; k < 14; k++) { float a = k * 25.7f + t * 12.0f, x2, y2; polaire(X, Y, 150, a, x2, y2); cb::ligne(g, X, Y, x2, y2, CFl(cr * 0.22f, cg * 0.22f, cbv * 0.22f)); }      // l'aura
  g.fillRoundRect(10, 176, 190, 7, 3, C(5, 2, 4));
  cb::dessinerCorbeau(g, cbx);
  iris(g, X, Y, 5.2f, motif, cr, cg, cbv, n, -90 + t * 40.0f);
  g.fillRoundRect(204, 28, 112, 172, 6, C(12, 2, 6)); g.drawRoundRect(204, 28, 112, 172, 6, C(30, 140, 150));
  oeil(g, 260, 52, 28, 14, motif, cr, cg, cbv, n, C(12, 2, 6));
  texte(g, 210, 76, "SUR TON CORBEAU", C(120, 230, 240));
  bloc(g, 210, 96, 100, "Les yeux de tes corbeaux prennent les couleurs de cet œil.", 14, C(255, 236, 230), 6);
  bool actif = (cb::oeilActif == sel + 1 && cb::oeilActif != 0 && cb::oeilActif == (uint8_t)prefs.getUChar("cbeil2", 0));
  { uint16_t ca = actif ? C(120, 255, 150) : C(180, 150, 150); if (actif) g.fillCircle(214, 192, 4, ca); else g.drawCircle(214, 192, 4, ca); texte(g, 222, 186, actif ? "Œil actif" : "Pas encore actif", ca); }
  bouton(g, 4, 102, "FICHE"); bouton(g, 108, 102, actif ? "RETIRER" : "ACTIVER", actif ? C(90, 20, 24) : C(20, 90, 50)); bouton(g, 212, 102, "CORBEAUX");
}

// ---------- écran 3 : créer un œil ----------
static void ecranCreer(M5Canvas& g) {
  g.fillScreen(BGc()); entete(g, 0, "CRÉER UN ŒIL", "");
  g.fillRoundRect(4, 28, 176, 172, 6, C(12, 2, 6)); g.drawRoundRect(4, 28, 176, 172, 6, C(120, 22, 32));
  texte(g, 12, 33, "COULEUR", C(255, 96, 100));
  for (int x = 0; x < 160; x++) { float r, gg, b; cb::teinteRVB(x * 255.0f / 159.0f, r, gg, b); g.drawFastVLine(12 + x, 48, 16, CFl(r, gg, b)); }
  { int kx = 12 + (int)(cb::oCu.teinte * 159 / 255); g.fillRect(kx - 2, 45, 5, 22, C(255, 255, 255)); g.drawRect(kx - 2, 45, 5, 22, C(0, 0, 0)); }
  char s[24]; snprintf(s, sizeof(s), "VIRGULES  %d", cb::oCu.marques); texte(g, 12, 76, s, C(255, 96, 100));
  g.fillRoundRect(12, 94, 160, 8, 4, C(60, 12, 20));
  { int kx = 12 + cb::oCu.marques * 160 / 6; g.fillRoundRect(12, 94, kx - 12 + 1, 8, 4, C(214, 24, 34)); g.fillRect(kx - 2, 89, 5, 18, C(255, 255, 255)); }
  texte(g, 12, 112, "MOTIF", C(255, 96, 100));
  static const uint8_t MOT[4] = {M_ANN, M_VIRG, M_PALES, M_FLEUR};
  float r0, g0, b0; cb::teinteRVB(cb::oCu.teinte, r0, g0, b0);
  for (int i = 0; i < 4; i++) {
    int x = 12 + i * 41, y = 128; bool on = (cb::oCu.motif == i);
    g.fillRoundRect(x, y, 37, 34, 5, on ? C(60, 14, 22) : C(26, 5, 10)); g.drawRoundRect(x, y, 37, 34, 5, on ? C(255, 70, 80) : C(100, 18, 28));
    iris(g, x + 18, y + 17, 12, MOT[i], r0, g0, b0, cb::oCu.marques, -90);
  }
  int motif, n; float cr, cg, cbv; couleurOeil(NE, motif, cr, cg, cbv, n);
  oeil(g, 250, 78, 58, 30, motif, cr, cg, cbv, n, BGc());
  texte(g, 250, 124, "TON ŒIL", C(255, 236, 230), 1, true);
  snprintf(s, sizeof(s), "%d virgule%s", n, n > 1 ? "s" : ""); texte(g, 250, 144, s, C(200, 150, 150), 1);
  texte(g, 250, 164, "Touche pour régler", C(180, 120, 120), 1);
  bouton(g, 4, 102, "ENREGISTRER"); bouton(g, 108, 102, "HASARD"); bouton(g, 212, 102, "RETOUR");
}

// ---------- mise en page générale ----------
static void majApercuOeil() { if (ecran == 2) { cb::oeilActif = (uint8_t)(sel + 1); cb::appliquerOeil(); } }
static void quitterApercu() { cb::chargerOeil(); }      // on revient à l'œil réellement choisi
static void ouvrir() { ouvert = true; scL = 0; scP = 0; defile = false; zone = 0; flingV = 0; ecran = 0; sel = 0; tab = 0; sale = true; volVu = false; animSens = 0; cbxOk = false; bas = false; mc->idx = mt->idx = -1; cb::chargerOeil(); batirListe(); }
static void quitter() { if (!ouvert) return; ouvert = false; narrArreter(); quitterApercu(); if (!lectureEnCours && M5.Speaker.isRunning()) { M5.Speaker.stop(); M5.Speaker.end(); M5.Mic.begin(); } }
static void pousser() { cv.pushSprite(0, 0); }
static void dessiner() {
  if (!ouvert) ouvrir();
  uint32_t m = millis();
  if (m < finAffVolume) volVu = true; else if (volVu) { volVu = false; sale = true; }
  if (ecran != 1 && (nrOn || nrDepart)) narrArreter();      // la voix ne continue jamais hors de la fiche
  if (ecran == 1 && nrDepart && (int32_t)(m - nrDepart) >= 0 && animSens == 0) { nrDepart = 0; narrLancer(sel); }
  narrPomper();
  { bool st = narrActive(); if (st != nrAffEtat) { nrAffEtat = st; if (ecran == 1) sale = true; } }
  if (animSens != 0) {      // glissement d'une fiche à l'autre
    float e = (m - animT0) / 220.0f; if (e >= 1) { e = 1; }
    float p = cb::pas(e);
    fiche(cv, animDe, (int)(-animSens * 320 * p), tab, scAnim, *mc); fiche(cv, animVers, (int)(animSens * 320 * (1 - p)), tab, 0, *mt);
    pousser(); if (e >= 1) { animSens = 0; Mise* x = mc; mc = mt; mt = x; sale = false; }
    return;
  }
  if (ecran == 0 && flingV != 0 && !bas) {      // la liste continue de glisser un instant après le geste
    float dt = (m - tFling) / 33.0f; if (dt > 3) dt = 3; tFling = m;
    scL += (int)(flingV * dt); flingV *= 0.92f; if (flingV > -0.5f && flingV < 0.5f) flingV = 0;
    if (scL < 0) { scL = 0; flingV = 0; } if (scL > scLmax) { scL = scLmax; flingV = 0; } sale = true;
  }
  if (ecran == 2 && m - tCb >= 45) { tCb = m; sale = true; }
  if (!sale) return;
  sale = false;
  switch (ecran) {
    case 0: ecranListe(cv); break;
    case 1: assurer(); fiche(cv, sel, 0, tab, scP, *mc); break;
    case 2: ecranCorbeau(cv); break;
    default: ecranCreer(cv); break;
  }
  pousser();
}

// ---------- toucher : glisser = fiche suivante / précédente ou défilement, toucher = choisir ----------
static void aller(int sens) {      // fiche suivante (+1) ou précédente (-1), avec glissement
  if (animSens != 0) return;
  int v = suivant(sel, sens); if (v == sel) return;
  if (ecran == 1) { assurer(); scAnim = scP; construire(*mt, v, tab); animDe = sel; animVers = v; animSens = sens; animT0 = millis(); }
  sel = v; scP = 0; majApercuOeil(); sale = true; if (narrDispo(v)) { if (ecran == 1) narrPlanifier(); } else ecouter(v);      // la voix (ou, sans voix, le cri de la famille) à chaque nouvelle fiche
}
static void creerGlisser(int x, int y) {
  if (y >= 44 && y < 68 && x >= 8 && x < 176) { cb::oCu.teinte = (uint8_t)cb::lim((x - 12) * 255 / 159, 0, 255); sale = true; }
  else if (y >= 86 && y < 108 && x >= 8 && x < 176) { cb::oCu.marques = (uint8_t)cb::lim(((x - 12) * 6 + 80) / 160, 0, 6); sale = true; }
}
static void toucherBouton(int b) {
  if (ecran == 0) {
    if (b == 0) { page = 6; bip(700, 40); }
    else if (b == 1) { ecran = 3; sale = true; }
    else { page = 32; bip(700, 40); }
  } else if (ecran == 1) {
    if (b == 0) aller(-1); else if (b == 1) { ecran = 0; batirListe(); centrerListe(); sale = true; } else if (b == 2) aller(1); else { ecran = 2; majApercuOeil(); sale = true; }
  } else if (ecran == 2) {
    if (b == 0) { quitterApercu(); ecran = 1; scP = 0; sale = true; }
    else if (b == 1) { bool actif = (cb::oeilActif == sel + 1 && prefs.getUChar("cbeil2", 0) == sel + 1); cb::choisirOeil(actif ? 0 : sel + 1); majApercuOeil(); if (actif) { ecran = 1; } sale = true; }
    else { page = 32; bip(700, 40); }
  } else {
    if (b == 0) { cb::oCu.ok = 1; cb::sauverOeilCu(); cb::choisirOeil(NE + 1); sel = NE; ecran = 1; scP = 0; batirListe(); sale = true; }
    else if (b == 1) { cb::oCu.teinte = (uint8_t)random(256); cb::oCu.marques = (uint8_t)random(7); cb::oCu.motif = (uint8_t)random(4); sale = true; }
    else { ecran = 0; batirListe(); sale = true; }
  }
}
static int boutonSous(int x) {
  if (ecran == 1) { int b = (x - 4) / 78; return b < 0 ? 0 : (b > 3 ? 3 : b); }
  int b = (x - 4) / 104; return b < 0 ? 0 : (b > 2 ? 2 : b);
}
static void toucherEcran(int x, int y) {
  if (y >= 206) { toucherBouton(boutonSous(x)); return; }
  if (ecran == 0) {
    int yy = y - 25 + scL;
    for (int k = 0; k < nR; k++) if (Rt[k] == 1 && yy >= Ry[k] && yy < Ry[k] + HE - 3 && y >= 24) { sel = Rv[k]; ecran = 1; scP = 0; sale = true; if (narrDispo(sel)) narrPlanifier(); else ecouter(sel); return; }
  } else if (ecran == 1) {
    if (x >= 232 && x < 316 && y >= 24 && y < 48 && narrDispo(sel)) { if (narrActive()) narrArreter(); else narrLancer(sel); sale = true; }
    else if (y >= 76 && y < 99) { for (int k = 0; k < 4; k++) if (x >= TABX[k] && x < TABX[k] + TABW[k] && k != tab) { tab = k; scP = 0; sale = true; } }
    else if (x < 114 && y >= 24 && y < 76) ecouter(sel);      // toucher l'œil : écouter le cri de la famille
  } else if (ecran == 3) {
    if (y >= 128 && y < 162) for (int i = 0; i < 4; i++) { int bx = 12 + i * 41; if (x >= bx && x < bx + 37) { cb::oCu.motif = (uint8_t)i; sale = true; } }
    creerGlisser(x, y);
  }
}
static void toucher(const m5::touch_detail_t& t) {
  if (!ouvert) return;
  int x = t.x, y = t.y;
  if (t.wasPressed()) {
    bas = true; sx = px = x; sy = py = y; defile = false; zone = 0; vy = 0; flingV = 0;
    if (ecran == 3) creerGlisser(x, y);
    else if (ecran == 0 && animSens == 0 && y >= 24 && y < 205) { zone = 3; sc0 = scL; }
    else if (ecran == 1 && animSens == 0 && y >= TXY && y < 205) { zone = 1; sc0 = scP; }
  }
  else if (t.isPressed() && bas) {
    int ancienY = py; px = x; py = y; if (ecran == 3 && sy < 206) creerGlisser(x, y);
    if (zone && (defile || abs(py - sy) > 7) && (defile || abs(py - sy) >= abs(px - sx))) {      // glisser vers le haut / le bas : la liste ou le texte défile
      defile = true; vy = 0.6f * vy + 0.4f * (float)(ancienY - py);
      if (zone == 3) { int v = sc0 - (py - sy); v = v < 0 ? 0 : (v > scLmax ? scLmax : v); if (v != scL) { scL = v; sale = true; } }
      else { int v = sc0 - (py - sy), mx = scPmax; v = v < 0 ? 0 : (v > mx ? mx : v); if (v != scP) { scP = v; sale = true; } }
    }
  }
  if (t.wasReleased() && bas) {
    bas = false;
    if (defile) { defile = false; if (zone == 3 && (vy > 2.0f || vy < -2.0f)) { flingV = vy > 40 ? 40 : (vy < -40 ? -40 : vy); tFling = millis(); sale = true; } zone = 0; return; }
    int dx = px - sx, dy = py - sy;
    if ((ecran == 1 || ecran == 2) && abs(dx) > 40 && abs(dy) < 45 && sy < 206) { aller(dx < 0 ? 1 : -1); return; }
    if (abs(dx) < 14 && abs(dy) < 14) toucherEcran(sx, sy);
  }
}

}  // namespace dx

static inline void dexDessiner() { dx::dessiner(); }
static inline void dexToucher(const m5::touch_detail_t& t) { dx::toucher(t); }
static inline void dexQuitter() { dx::quitter(); }
