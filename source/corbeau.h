#pragma once
// ======================= MODE CORBEAUX (page 32) =======================
// Tout est dessiné par le code (polygones, ellipses) : aucune image. Fond précalculé une seule fois,
// puis seules les zones qui bougent sont redessinées et envoyées à l'écran (zones sales).
// Les sons des corbeaux se mélangent au son déjà en cours (Matinale) sur des canaux libres du haut-parleur.
#include <stdint.h>
#include <pgmspace.h>
// Deux jeux de sons, chacun dans son espace de noms (les tableaux portent les memes noms) :
//   cbs = sons synthetiques (sons_corbeau.h), cbr = vrais cris (sons_corbeau_reel.h, facultatif).
#if __has_include("sons_corbeau.h")
namespace cbs {
#include "sons_corbeau.h"
#ifdef SONS_CORBEAU_RATE
static const uint32_t RATE = SONS_CORBEAU_RATE;
#else
static const uint32_t RATE = 16000;
#endif
}
#undef SONS_CORBEAU_RATE
#define CB_AVEC_SONS 1
#endif
#if __has_include("sons_corbeau_reel.h")
namespace cbr {
#include "sons_corbeau_reel.h"
#ifdef SONS_CORBEAU_RATE
static const uint32_t RATE = SONS_CORBEAU_RATE;
#else
static const uint32_t RATE = 16000;
#endif
}
#undef SONS_CORBEAU_RATE
#define CB_AVEC_REEL 1
#endif

extern uint32_t finAffVolume;
extern uint32_t dernierSecouement;

namespace cb {

static const int NCB = 5;        // corbeaux
static const int NOB = 4;        // objets au sol / qui tombent
static const int NPE = 6;        // perchoirs
static const int NPART = 44;    // plumes
static const int NFX = 6;       // éclats lumineux
static const int NENT = NCB + NOB + 5;
static const int H_SCENE = 206; // la barre de nourriture occupe le bas
static const float SOL_Y = 201.0f;
static const float PERCH[NPE][2] = {{100, 141}, {140, 135.8f}, {180, 130.3f}, {222, 125.6f}, {88, 62.5f}, {24, 94.5f}};

enum { E_PERCHE = 0, E_DECOLLE, E_VOL, E_ATTERRIT, E_SOL, E_DEHORS, E_POOF };
enum { B_RIEN = 0, B_PERCHOIR, B_NOUR, B_SORTIR, B_OBJET, B_ORBITE, B_VISITEUR };
enum { T_GRAINE = 0, T_VIANDE, T_NOIX, T_PAIN, T_BAIES, T_BRILLANT, T_BAGUE, T_KUNAI, T_SHURIKEN, T_PARCHEMIN, T_NB };

// ---------- petits outils ----------
static inline float mnf(float a, float b) { return a < b ? a : b; }
static inline float mxf(float a, float b) { return a > b ? a : b; }
static inline float lim(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float pas(float x) { x = lim(x, 0.0f, 1.0f); return x * x * (3.0f - 2.0f * x); }
static inline float rnd(float a, float b) { return a + (b - a) * (random(10001) / 10000.0f); }
static inline float rnd01() { return random(10001) / 10000.0f; }
static inline int ar(float v) { return (int)lroundf(v); }

// ---------- état général ----------
static M5Canvas fond(&M5.Display);
static bool ouvert = false, fondOk = false, debug = false, plein = true, sec = false;
static int bpp = 2;
static uint32_t tDernier = 0, tSec = 0, tPressee = 0;
static int nImg = 0, fps = 0, msDessin = 0;
static int choix = 0, presse = -1;
static float gAction = 2.0f, gPoof = 18.0f, gObjet = 30.0f, gAttrib = 0.0f;
static bool volVu = false, barreSale = false, sonsReels = false;
static int memTas = 0, memPsram = 0;
static uint32_t secVu = 0;

struct Rect { int16_t x0, y0, x1, y1; };   // x1, y1 exclus
static int bbx0, bby0, bbx1, bby1;
static inline void bbReset() { bbx0 = 9999; bby0 = 9999; bbx1 = -9999; bby1 = -9999; }
static inline void bbPt(float x, float y) {
  int xi = (int)floorf(x), yi = (int)floorf(y);
  if (xi < bbx0) bbx0 = xi; if (xi > bbx1) bbx1 = xi; if (yi < bby0) bby0 = yi; if (yi > bby1) bby1 = yi;
}
static Rect bbRect() { Rect r; r.x0 = (int16_t)(bbx0 - 2); r.y0 = (int16_t)(bby0 - 2); r.x1 = (int16_t)(bbx1 + 3); r.y1 = (int16_t)(bby1 + 3); return r; }

// ---------- primitives (suivent la boîte englobante ; ne dessinent pas en mode « sec ») ----------
static void tri(M5Canvas& g, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c) {
  bbPt(x0, y0); bbPt(x1, y1); bbPt(x2, y2); if (sec) return;
  g.fillTriangle(ar(x0), ar(y0), ar(x1), ar(y1), ar(x2), ar(y2), c);
}
static void disque(M5Canvas& g, float x, float y, float r, uint16_t c) {
  bbPt(x - r, y - r); bbPt(x + r, y + r); if (sec) return;
  g.fillCircle(ar(x), ar(y), ar(r) < 1 ? 1 : ar(r), c);
}
static void ell(M5Canvas& g, float x, float y, float rx, float ry, uint16_t c) {
  bbPt(x - rx, y - ry); bbPt(x + rx, y + ry); if (sec) return;
  int a = ar(rx), b = ar(ry); g.fillEllipse(ar(x), ar(y), a < 1 ? 1 : a, b < 1 ? 1 : b, c);
}
static void ligne(M5Canvas& g, float x0, float y0, float x1, float y1, uint16_t c) {
  bbPt(x0, y0); bbPt(x1, y1); if (sec) return;
  g.drawLine(ar(x0), ar(y0), ar(x1), ar(y1), c);
}
static void poly(M5Canvas& g, const float* p, int n, uint16_t c) {      // polygone en éventail depuis son centre
  float cx = 0, cy = 0;
  for (int i = 0; i < n; i++) { cx += p[i * 2]; cy += p[i * 2 + 1]; }
  cx /= n; cy /= n;
  for (int i = 0; i < n; i++) { int j = (i + 1) % n; tri(g, cx, cy, p[i * 2], p[i * 2 + 1], p[j * 2], p[j * 2 + 1], c); }
}

// ---------- objets et nourriture (dessinés par le code) ----------
static void dessinerObjet(M5Canvas& g, int type, float x, float y, float r, float rot, float tw) {
  float cr = cosf(rot), sr = sinf(rot);
  float q[24];
  auto T = [&](float px, float py, float& X, float& Y) { X = x + (px * cr - py * sr) * r; Y = y + (px * sr + py * cr) * r; };
  auto PG = [&](const float* pts, int n, uint16_t c) { for (int i = 0; i < n; i++) T(pts[i * 2], pts[i * 2 + 1], q[i * 2], q[i * 2 + 1]); poly(g, q, n, c); };
  auto E = [&](float cx, float cy, float rx, float ry, uint16_t c) { float X, Y; T(cx, cy, X, Y); ell(g, X, Y, rx * r, ry * r, c); };
  switch (type) {
    case T_GRAINE: { E(0, 0, 1.0f, 0.6f, C(250, 208, 70)); E(-0.3f, -0.2f, 0.4f, 0.2f, C(255, 235, 140)); } break;
    case T_NOIX: { E(0, 0, 1.0f, 0.92f, C(150, 98, 48)); E(0.38f, 0.42f, 0.46f, 0.36f, C(112, 68, 32)); E(-0.3f, -0.3f, 0.4f, 0.3f, C(214, 158, 92)); } break;
    case T_VIANDE: {
      static const float a[14] = {-1.15f, 0.1f, -0.7f, -0.7f, 0.2f, -0.82f, 1.0f, -0.4f, 1.15f, 0.35f, 0.35f, 0.8f, -0.55f, 0.72f};
      PG(a, 7, C(206, 58, 62)); E(-0.15f, -0.15f, 0.55f, 0.3f, C(240, 118, 106));
      static const float b[8] = {1.0f, -0.1f, 1.45f, -0.2f, 1.45f, 0.2f, 1.0f, 0.3f};
      PG(b, 4, C(242, 232, 212)); E(1.5f, -0.18f, 0.2f, 0.2f, C(242, 232, 212)); E(1.5f, 0.2f, 0.2f, 0.2f, C(242, 232, 212));
    } break;
    case T_PAIN: { E(-0.55f, 0.25f, 0.5f, 0.36f, C(236, 208, 152)); E(0.3f, -0.3f, 0.46f, 0.33f, C(226, 190, 130)); E(0.6f, 0.35f, 0.36f, 0.26f, C(242, 218, 168)); } break;
    case T_BAIES: {
      E(-0.5f, 0.3f, 0.6f, 0.6f, C(222, 28, 60)); E(0.5f, 0.25f, 0.6f, 0.6f, C(222, 28, 60)); E(0.0f, -0.45f, 0.58f, 0.58f, C(240, 44, 72));
      E(-0.68f, 0.1f, 0.18f, 0.18f, C(255, 175, 185)); E(0.32f, 0.05f, 0.18f, 0.18f, C(255, 175, 185));
    } break;
    case T_BRILLANT: {
      E(0, 0, 1.0f, 1.0f, C(255, 230, 130)); E(0, 0, 0.66f, 0.66f, C(210, 170, 60)); E(-0.22f, -0.22f, 0.26f, 0.26f, C(255, 250, 214));
      if (tw > 0.02f) { float L = 1.3f + 2.6f * tw, w = 0.3f * (0.5f + tw);
        float h[8] = {-L, 0, 0, -w, L, 0, 0, w}; PG(h, 4, C(255, 252, 238)); float v[8] = {0, -L, w, 0, 0, L, -w, 0}; PG(v, 4, C(255, 252, 238)); }
    } break;
    case T_BAGUE: {
      E(0, 0, 0.9f, 0.9f, C(226, 26, 48)); E(0, 0, 0.5f, 0.5f, C(60, 6, 14)); E(0, -0.92f, 0.3f, 0.3f, C(255, 70, 90)); E(-0.5f, -0.62f, 0.2f, 0.2f, C(255, 196, 196));
    } break;
    case T_KUNAI: {
      static const float a[10] = {1.55f, 0, 0.35f, -0.46f, -0.5f, -0.3f, -0.5f, 0.3f, 0.35f, 0.46f};
      PG(a, 5, C(178, 188, 214));
      static const float b[8] = {-0.5f, -0.14f, -1.1f, -0.14f, -1.1f, 0.14f, -0.5f, 0.14f};
      PG(b, 4, C(60, 54, 70)); E(-1.5f, 0, 0.4f, 0.4f, C(176, 182, 204)); E(-1.5f, 0, 0.18f, 0.18f, C(30, 4, 10));
    } break;
    case T_SHURIKEN: {
      static const float a[16] = {0, -1.3f, 0.34f, -0.34f, 1.3f, 0, 0.34f, 0.34f, 0, 1.3f, -0.34f, 0.34f, -1.3f, 0, -0.34f, -0.34f};
      PG(a, 8, C(186, 194, 216)); E(0, 0, 0.22f, 0.22f, C(44, 8, 18));
    } break;
    case T_PARCHEMIN: {
      static const float a[8] = {-1.0f, -0.46f, 1.0f, -0.46f, 1.0f, 0.46f, -1.0f, 0.46f};
      PG(a, 4, C(236, 214, 164)); E(-1.0f, 0, 0.2f, 0.5f, C(204, 162, 102)); E(1.0f, 0, 0.2f, 0.5f, C(204, 162, 102));
      static const float b[8] = {-0.18f, -0.5f, 0.18f, -0.5f, 0.18f, 0.5f, -0.18f, 0.5f};
      PG(b, 4, C(192, 30, 46));
    } break;
  }
  if (type >= T_BAGUE && tw > 0.02f) {      // petit éclat sur les objets
    float X, Y; T(0.62f, -0.72f, X, Y); float L = (0.9f + 1.5f * tw) * r, w = 0.2f * (0.6f + tw) * r;
    tri(g, X - L, Y, X, Y - w, X + L, Y, C(255, 252, 238)); tri(g, X - L, Y, X, Y + w, X + L, Y, C(255, 252, 238));
    tri(g, X, Y - L, X + w, Y, X, Y + L, C(255, 252, 238)); tri(g, X, Y - L, X - w, Y, X, Y + L, C(255, 252, 238));
  }
}

// ---------- décor (dessiné une seule fois dans « fond ») ----------
// ---------- ciel, lune, etoiles, brume (dessines une seule fois dans « fond ») ----------
static inline uint16_t CF(float r, float g, float b) { return C((uint8_t)lim(r, 0, 255), (uint8_t)lim(g, 0, 255), (uint8_t)lim(b, 0, 255)); }
struct StopC { float y; float r, g, b; };
static const StopC STC[5] = {{0, 10, 1, 5}, {72, 72, 6, 12}, {125, 140, 14, 20}, {192, 104, 11, 16}, {240, 30, 3, 8}};
static void cielRGB(float y, float& r, float& g, float& b) {
  int k = 0; while (k < 3 && y >= STC[k + 1].y) k++;
  float u = (y - STC[k].y) / (STC[k + 1].y - STC[k].y);
  r = lerpf(STC[k].r, STC[k + 1].r, u); g = lerpf(STC[k].g, STC[k + 1].g, u); b = lerpf(STC[k].b, STC[k + 1].b, u);
}
static const float LUNE_X = 274.0f, LUNE_Y = 40.0f, LUNE_R = 20.0f;      // lune petite, dans le coin en haut a droite
struct Crat { float ox, oy, r, dk; };
static const Crat CRAT[6] = {{-6.5f, -4.5f, 4.2f, 0.20f}, {5.5f, 3.0f, 5.0f, 0.16f}, {-2.5f, 8.5f, 3.0f, 0.16f}, {8.0f, -8.0f, 2.6f, 0.14f}, {-10.5f, 4.0f, 2.4f, 0.12f}, {1.5f, -9.5f, 2.2f, 0.10f}};
static void fondRGB(float x, float y, float& r, float& g, float& b) {      // couleur exacte du ciel (halo + lune) en un point
  cielRGB(y, r, g, b);
  float dx = x - LUNE_X, dy = y - LUNE_Y, d = sqrtf(dx * dx + dy * dy);
  float w = expf(-mxf(d - LUNE_R, 0.0f) / 17.0f) * 0.50f * lim(1.0f - (d - LUNE_R) / 58.0f, 0.0f, 1.0f);
  r += 105 * w; g += 13 * w; b += 11 * w;
  if (d < LUNE_R) {
    float ed = d / LUNE_R, sh = 1.0f - 0.20f * lim((dx * 0.45f + dy * 0.75f) / LUNE_R, -1, 1);
    float e3 = ed * ed * ed, f0 = lerpf(1.0f, 0.8f, e3);
    float cr_ = 206 * sh * f0, cg_ = 38 * sh * f0, cb_ = 30 * sh * f0;
    for (int i = 0; i < 6; i++) {
      float ddx = x - (LUNE_X + CRAT[i].ox), ddy = y - (LUNE_Y + CRAT[i].oy), dd = sqrtf(ddx * ddx + ddy * ddy);
      if (dd < CRAT[i].r) { float f = 1.0f - CRAT[i].dk * (1.0f - 0.5f * dd / CRAT[i].r); cr_ *= f; cg_ *= f; cb_ *= f; }
    }
    float a = lim((LUNE_R - d) / 1.3f, 0, 1);
    r = lerpf(r, cr_, a); g = lerpf(g, cg_, a); b = lerpf(b, cb_, a);
  }
}
static void brume(M5Canvas& g, float cx, float cy, float w, float h, float a) {
  for (int dy = -(int)h; dy <= (int)h; dy++) {
    float ay = 1.0f - (float)(dy * dy) / (h * h); int y = (int)cy + dy;
    if (y < 0 || y >= H_SCENE) continue;
    for (int x = (int)(cx - w); x < (int)(cx + w); x += 4) {
      float ax = 1.0f - fabsf((x + 2 - cx) / w); ax = ax * ax; float al = a * ay * ax;
      if (al < 0.03f || x < 0 || x > 316) continue;
      float r, gg, b; fondRGB(x + 2.0f, (float)y, r, gg, b);
      g.drawFastHLine(x, y, 4, CF(r * (1 - al) + 24 * al, gg * (1 - al) + 1 * al, b * (1 - al) + 9 * al));
    }
  }
}
static float solHaut(float x) { return 202.0f + 1.2f * sinf(x / 37.0f) + 0.8f * sinf(x / 13.0f + 1.0f); }
static void ciel(M5Canvas& g) {
  for (int y = 0; y < 240; y++) { float r, gg, b; cielRGB((float)y, r, gg, b); g.drawFastHLine(0, y, 320, CF(r, gg, b)); }
  const int R = (int)(LUNE_R + 58);
  for (int y = (int)LUNE_Y - R; y <= (int)LUNE_Y + R; y++) {
    if (y < 0 || y >= H_SCENE) continue;
    for (int x = (int)LUNE_X - R; x <= (int)LUNE_X + R;) {
      float dx = x - LUNE_X, dy = y - LUNE_Y, dd = sqrtf(dx * dx + dy * dy);
      int step = dd < LUNE_R + 3 ? 1 : 3;
      if (x >= 0 && x < 320 - step && dd < R) {
        float r, gg, b; fondRGB(x + (step - 1) * 0.5f, (float)y, r, gg, b);
        if (step == 1) g.drawPixel(x, y, CF(r, gg, b)); else g.drawFastHLine(x, y, step, CF(r, gg, b));
      }
      x += step;
    }
  }
  uint32_t seed = 12345;     // etoiles : toujours les memes
  for (int i = 0; i < 34; i++) {
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sx = 3 + (int)((seed >> 8) % 314);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sy = 3 + (int)((seed >> 8) % 100);
    if (hypotf(sx - LUNE_X, sy - LUNE_Y) < LUNE_R + 30) continue;
    int v = (int)((seed >> 4) % 100);
    if (v < 70) g.drawPixel(sx, sy, C(150, 62, 74));
    else if (v < 93) g.drawPixel(sx, sy, C(205, 112, 120));
    else { g.drawPixel(sx, sy, C(240, 170, 170)); g.drawPixel(sx - 1, sy, C(150, 50, 60)); g.drawPixel(sx + 1, sy, C(150, 50, 60)); g.drawPixel(sx, sy - 1, C(150, 50, 60)); g.drawPixel(sx, sy + 1, C(150, 50, 60)); }
  }
  brume(g, 230, 70, 100, 3, 0.40f); brume(g, 120, 34, 80, 2, 0.35f); brume(g, 236, 96, 120, 4, 0.40f);
  brume(g, 90, 118, 110, 4, 0.40f); brume(g, 230, 160, 130, 5, 0.40f); brume(g, 60, 176, 110, 5, 0.40f);
  for (int x = 0; x < 320; x++) {
    float yl = 190.0f + 4.0f * sinf(x / 50.0f + 2.0f) + 3.0f * sinf(x / 21.0f);
    g.drawFastVLine(x, (int)yl, 240 - (int)yl, C(46, 6, 11));
    float ys = solHaut((float)x);
    g.drawFastVLine(x, (int)ys, 240 - (int)ys, C(5, 2, 4));
  }
}

static void membre(M5Canvas& g, const float (*p)[2], int n, float w0, float w1, bool bouts) {   // branche : spline lissée, largeur qui s'affine
  uint16_t nr = C(5, 2, 4);
  const int N = 6;
  float prx = 0, pry = 0, prw = 0; bool a = false;
  int tot = (n - 1) * N;
  for (int i = 0; i < n - 1; i++) {
    const float* p0 = p[i > 0 ? i - 1 : 0]; const float* p1 = p[i]; const float* p2 = p[i + 1]; const float* p3 = p[i + 2 < n ? i + 2 : n - 1];
    for (int j = 0; j < N; j++) {
      float t = (float)j / N, t2 = t * t, t3 = t2 * t;
      float x = 0.5f * (2 * p1[0] + (-p0[0] + p2[0]) * t + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2 + (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3);
      float y = 0.5f * (2 * p1[1] + (-p0[1] + p2[1]) * t + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2 + (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3);
      float u = (float)(i * N + j) / tot;
      float w = lerpf(w0, w1, powf(u, 0.85f)) * (1.0f + 0.10f * sinf(u * 21.0f + w0));
      if (a) {
        float dx = x - prx, dy = y - pry, l = sqrtf(dx * dx + dy * dy); if (l < 0.01f) continue;
        float nx = -dy / l, ny = dx / l, h0 = prw * 0.5f, h1 = w * 0.5f;
        tri(g, prx + nx * h0, pry + ny * h0, prx - nx * h0, pry - ny * h0, x + nx * h1, y + ny * h1, nr);
        tri(g, prx - nx * h0, pry - ny * h0, x - nx * h1, y - ny * h1, x + nx * h1, y + ny * h1, nr);
        disque(g, x, y, h1, nr);
      } else if (bouts) disque(g, x, y, w * 0.5f, nr);
      prx = x; pry = y; prw = w; a = true;
    }
  }
  if (bouts) disque(g, p[n - 1][0], p[n - 1][1], w1 * 0.5f, nr);
}

static float ytop(float x) {      // dessus de la grosse branche horizontale
  static const float pt[6][2] = {{64, 152}, {100, 141}, {132, 137}, {164, 132}, {196, 128.5f}, {236, 124}};
  if (x <= pt[0][0]) return pt[0][1];
  for (int i = 0; i < 5; i++) if (x <= pt[i + 1][0]) { float u = (x - pt[i][0]) / (pt[i + 1][0] - pt[i][0]); return lerpf(pt[i][1], pt[i + 1][1], u); }
  return pt[5][1];
}

static void arbre(M5Canvas& g) {
  uint16_t nr = C(5, 2, 4);
  static const float tronc[8][2] = {{72, 254}, {67, 222}, {60, 192}, {66, 160}, {61, 130}, {51, 104}, {54, 78}, {46, 52}};
  membre(g, tronc, 8, 42, 10, true);
  g.fillTriangle(28, 254, 44, 232, 120, 254, nr); g.fillTriangle(44, 232, 100, 232, 120, 254, nr);
  g.fillTriangle(44, 232, 56, 208, 100, 232, nr); g.fillTriangle(56, 208, 86, 208, 100, 232, nr);
  static const float r1[3][2] = {{44, 238}, {24, 246}, {4, 252}}, r2[3][2] = {{94, 240}, {114, 248}, {136, 254}};
  membre(g, r1, 3, 12, 3, true); membre(g, r2, 3, 11, 3, true);
  g.fillCircle(53, 100, 6, nr); g.fillCircle(69, 168, 7, nr); g.fillCircle(57, 206, 8, nr); g.fillCircle(65, 134, 5, nr);
  static const float c1[3][2] = {{46, 54}, {39, 36}, {44, 18}}, c2[3][2] = {{52, 58}, {60, 40}, {57, 25}};
  membre(g, c1, 3, 9, 2.5f, false); membre(g, c2, 3, 8, 2.2f, false);
  // grosse branche horizontale : le perchoir
  float px = 56, py = ytop(56), pth = 15;
  for (int x = 59; x <= 242; x += 3) {
    float u = lim((x - 62) / 176.0f, 0, 1), thk = lerpf(15, 4.8f, u) * (1.0f + 0.10f * sinf(x / 9.0f)), yt = ytop((float)x);
    tri(g, px, py, x, yt, px, py + pth, nr); tri(g, x, yt, x, yt + thk, px, py + pth, nr);
    px = (float)x; py = yt; pth = thk;
  }
  static const float b1[5][2] = {{58, 134}, {40, 119}, {26, 98}, {12, 92}, {-4, 76}};
  static const float b2[4][2] = {{52, 94}, {68, 81}, {80, 67}, {98, 61}};
  static const float b3[5][2] = {{64, 172}, {46, 174}, {30, 165}, {14, 173}, {2, 186}};
  membre(g, b1, 5, 14, 4, true); membre(g, b2, 4, 11, 3.6f, true); membre(g, b3, 5, 11, 4.5f, true);
  static const float s1[3][2] = {{60, 126}, {70, 115}, {77, 104}};
  membre(g, s1, 3, 7, 5, false);
  static const int xs[3] = {118, 150, 186}, ls[3] = {14, 11, 10};
  for (int i = 0; i < 3; i++) { float yb = ytop((float)xs[i]); float t[3][2] = {{(float)xs[i], yb + 8}, {xs[i] + 2.0f, yb + 8 + ls[i] * 0.5f}, {xs[i] + 3.0f, yb + 8.0f + ls[i]}}; membre(g, t, 3, 5.2f, 3.2f, false); }
  { float yb = ytop(228); float t[3][2] = {{228, yb + 3}, {231, yb - 6}, {230, yb - 14}}; membre(g, t, 3, 3.8f, 2.6f, false); }
  g.fillEllipse(60, 178, 3, 6, C(110, 10, 18));     // le trou du tronc : on voit le ciel rouge
}

static void barreCb(M5Canvas& g, int sel, int pr) {   // barre de nourriture, en bas
  g.fillRect(0, H_SCENE, 320, 34, C(5, 2, 4));
  g.fillRoundRect(4, 208, 312, 30, 10, C(16, 5, 9)); g.drawRoundRect(4, 208, 312, 30, 10, C(96, 18, 26));
  for (int i = 0; i < 6; i++) {
    int cx = 30 + i * 52, cy = 223;
    if (i == sel) { g.fillCircle(cx, cy, 14, C(70, 16, 20)); g.drawCircle(cx, cy, 14, C(214, 60, 52)); }
    if (i == pr) g.drawCircle(cx, cy, 11, C(255, 230, 200));
    dessinerObjet(g, i, (float)cx, (float)cy, 6.5f, 0.0f, 0.0f);
  }
}
static void menuCb(M5Canvas& g) {
  g.fillRoundRect(2, 2, 46, 20, 8, C(14, 4, 8)); g.drawRoundRect(2, 2, 46, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160)); g.drawString("Menu", 25, 12); g.setTextDatum(top_left);
}

static void boutonSons(M5Canvas& g) {
  g.fillRoundRect(52, 2, 118, 20, 8, C(14, 4, 8)); g.drawRoundRect(52, 2, 118, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160));
  g.drawString(sonsReels ? "Sons : réels" : "Sons : synthétiques", 111, 12); g.setTextDatum(top_left);
}
static void boutonVisiteur(M5Canvas& g) {
  g.fillRoundRect(176, 2, 58, 20, 8, C(14, 4, 8)); g.drawRoundRect(176, 2, 58, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160)); g.drawString("Visiteur", 205, 12); g.setTextDatum(top_left);
}
static void construireFond() {
  ciel(fond); arbre(fond); menuCb(fond); boutonSons(fond); boutonVisiteur(fond); barreCb(fond, choix, -1);
}

// ---------- corbeaux ----------
struct Aile { float ea, eh, sa, sh; };      // élévation du bras / de la main (degrés), recul du bras / de la main
static const Aile A_HAUT = {72, 34, 0.20f, 0.80f}, A_BAS = {-34, -58, 0.45f, 0.85f}, A_PLANE = {20, 4, 0.45f, 0.90f}, A_FLARE = {58, 64, 0.05f, 0.10f};
static Aile mixA(const Aile& a, const Aile& b, float t) { Aile o; o.ea = lerpf(a.ea, b.ea, t); o.eh = lerpf(a.eh, b.eh, t); o.sa = lerpf(a.sa, b.sa, t); o.sh = lerpf(a.sh, b.sh, t); return o; }
static float coup(float p) {      // 0 = ailes en haut, 1 = en bas : anticipation, claquement vers le bas, remontée plus lente
  if (p < 0.14f) return -0.24f * pas(p / 0.14f);
  if (p < 0.40f) return lerpf(-0.24f, 1.0f, pas((p - 0.14f) / 0.26f));
  return 1.0f - pas((p - 0.40f) / 0.60f);
}
static inline void ressort(float& x, float& v, float cible, float w, float dt) {      // ressort amorti : démarre et s'arrête en douceur
  v += ((cible - x) * w * w - 2.0f * w * v) * dt; x += v * dt;
}
enum { A_REPOS = 0, A_IMMO, A_REGARDE, A_LISSE_DOS, A_LISSE_POIT, A_GRATTE, A_SECOUE };
static float gCalme = 0.0f;       // un seul corbeau à la fois fait sa toilette ou se gratte

struct Corbeau {
  float x, y, vx, vy, s, dv;      // dv : direction visuelle (-1..1), se retourne en douceur
  int8_t d; uint8_t etat, but, act; int8_t perch, item, sub; bool visible, rapporte, fuite, pied, aWp;
  float t, tfin, tx, ty, wx, wy;
  float phase, hz, planeT, vitesse, surs; int8_t restBat;
  float ha, hav, haC, hb, hbv, hbC, look, tLook;      // tête : angle du cou, angle du bec (ressorts)
  float pitch, pv, pitchC, pr;                        // inclinaison du corps (ressort) ; pr = posture perchée
  float bec, clin, tClin, flash;
  float fan, fanC, wl, wlC, bob, hop, peck, resp, ruf, grat, gratC, tAct, dAct;
  float fx, fy, fp, ox, oy;                           // point d'appui des pattes (x, y, pente) ; décalage d'atterrissage
  float ang, ray, om; bool gerbe;                     // orbite autour du visiteur
  uint8_t objet;
  Aile pose, poseC;
};
struct Obj { bool actif; uint8_t type; float x, y, vy, t; bool repos; int8_t reserve; };
struct Plume { float x, y, vx, vy, rot, vr, vie, dur; bool actif; };
struct Eclat { float x, y, t, dur, taille; bool actif; };

static Corbeau cr[NCB];
static Obj ob[NOB];
static Plume pl[NPART];
static Eclat fx[NFX];
static Rect nouv[NENT], anc[NENT];
static Rect liste[40]; static int nListe = 0;

// ---------- visiteur mystérieux : silhouette de dos sur la grosse branche, entourée d'une nuée ----------
enum { V_RIEN = 0, V_NUEE, V_PRESENT };
struct Visiteur { uint8_t ph; float t, dur, arm, armC, tArm, flinch, sw, tRecrut; };
static Visiteur V;
static float gVisite = 60.0f;                 // délai avant la prochaine apparition spontanée
static float tAnim = 0.0f;                    // temps de l'image en cours (le même pour les deux passes de dessin)
struct Mini { float x, y, a, r, w, ph, vie, vx, vy; uint8_t mode; bool actif; };      // petit corbeau de la nuée : mode 1 = tourbillon, 2 = envol
static const int NMINI = 18;
static Mini mini[NMINI];
static const float VX = 166.0f;               // où il se tient sur la branche
static inline float visPiedsY() { return ytop(VX) + 0.4f; }
static void ptVis(int k, float& x, float& y) {      // points d'appui des corbeaux : 0 épaule gauche, 1 épaule droite, 2 dessus de la tête, 3 main du bras tendu
  float y0 = visPiedsY();
  switch (k) {
    case 0: x = VX - 10.0f + V.sw * 0.9f; y = y0 - 47.0f; break;
    case 1: x = VX + 10.0f + V.sw * 0.9f; y = y0 - 47.0f; break;
    case 2: x = VX + V.sw; y = y0 - 65.5f; break;
    default: { float phi = lerpf(0.14f, 1.75f, pas(V.arm)), sx = VX + 10.5f + V.sw * 0.9f, sy = y0 - 45.5f; x = sx + sinf(phi) * 25.0f; y = sy + cosf(phi) * 25.0f - 3.2f; } break;
  }
}

// ---------- sons (mélangés à la musique en cours : canaux 0 à 3, la Matinale utilise le 7) ----------
enum { SN_CROA1 = 0, SN_CROA2, SN_CROA3, SN_CROASSE1, SN_CROASSE2, SN_AILES, SN_PLUMES, SN_ATTERRIT, SN_POP_APP, SN_POP_DISP, SN_GAZ, SN_CROC, SN_ALARME, SN_TETON, SN_NB };
static uint32_t dernierSon[SN_NB];
// reglage « Sons : reels / synthetiques » (memorise : cle « cbsons »)
static bool deuxJeux() {
#if defined(CB_AVEC_SONS) && defined(CB_AVEC_REEL)
  return true;
#else
  return false;
#endif
}
static void chargerSons() {
  uint8_t v = prefs.getUChar("cbsons", 255);      // 255 = jamais choisi
  bool reel = false;
#ifdef CB_AVEC_REEL
  reel = (v == 255) ? true : (v == 1);
#endif
#ifndef CB_AVEC_SONS
  reel = true;
#endif
  sonsReels = reel;
}

// ---------- œil du corbeau : choisi dans le Dex (clés mémoire nouvelles : « cbeil » et « dexcr ») ----------
struct OeilCu { uint8_t ok, teinte, marques, motif; };      // l'œil que l'on crée soi-même
static OeilCu oCu = {0, 0, 3, 0};
static uint8_t oeilActif = 0;                               // 0 = œil rouge normal ; 1 à 7 = œil du Dex ; 8 = ton œil
static uint16_t oeilAnn = 0, oeilCoeur = 0;
static void teinteRVB(float h, float& r, float& g, float& b) {      // teinte 0..255 -> couleur vive
  float x = h * 6.0f / 256.0f; int i = (int)x; float f = x - i, q = 1.0f - f;
  switch (i % 6) { case 0: r = 1; g = f; b = 0; break; case 1: r = q; g = 1; b = 0; break; case 2: r = 0; g = 1; b = f; break; case 3: r = 0; g = q; b = 1; break; case 4: r = f; g = 0; b = 1; break; default: r = 1; g = 0; b = q; break; }
  r *= 255; g *= 255; b *= 255;
}
static void oeilRVB(int idx, float* c) {      // c = anneau r,g,b puis cœur r,g,b
  static const float T[9][6] = {{130, 8, 12, 255, 58, 40}, {130, 8, 12, 255, 58, 40}, {100, 4, 10, 255, 40, 44}, {150, 50, 0, 255, 130, 30}, {70, 40, 120, 190, 150, 255}, {140, 140, 170, 245, 245, 255}, {40, 90, 160, 170, 215, 255}, {20, 0, 0, 255, 20, 30}, {0, 0, 0, 0, 0, 0}};
  if (idx < 0 || idx > 8) idx = 0;
  for (int i = 0; i < 6; i++) c[i] = T[idx][i];
  if (idx == 8) { float r, g, b; teinteRVB(oCu.teinte, r, g, b); c[3] = r; c[4] = g; c[5] = b; c[0] = r * 0.45f; c[1] = g * 0.45f; c[2] = b * 0.45f; }
}
static void appliquerOeil() { float c[6]; oeilRVB(oeilActif, c); oeilAnn = CF(c[0], c[1], c[2]); oeilCoeur = CF(c[3], c[4], c[5]); }
static void chargerOeil() {
  oeilActif = prefs.getUChar("cbeil", 0); if (oeilActif > 8) oeilActif = 0;
  if (prefs.getBytesLength("dexcr") == sizeof(OeilCu)) prefs.getBytes("dexcr", &oCu, sizeof(OeilCu));
  if (oeilActif == 8 && !oCu.ok) oeilActif = 0;
  appliquerOeil();
}
static void choisirOeil(int a) { oeilActif = (uint8_t)a; prefs.putUChar("cbeil", oeilActif); appliquerOeil(); }
static void sauverOeilCu() { prefs.putBytes("dexcr", &oCu, sizeof(OeilCu)); }

static void son(int id, int vol = 200) {
  if (id < 0 || id >= SN_NB || !R.actif[M_SON] || R.volume == 0) return;
  const uint8_t* d = nullptr; uint32_t n = 0, rate = 16000;
#ifdef CB_AVEC_REEL
  if (sonsReels) {
    static const uint8_t* const D[SN_NB] = {cbr::SON_CROA1, cbr::SON_CROA2, cbr::SON_CROA3, cbr::SON_CROASSE1, cbr::SON_CROASSE2, (const uint8_t*)nullptr, (const uint8_t*)nullptr, (const uint8_t*)nullptr, (const uint8_t*)nullptr, (const uint8_t*)nullptr, (const uint8_t*)nullptr, (const uint8_t*)nullptr, cbr::SON_ALARME, cbr::SON_CROA_TETON};
    static const uint32_t N[SN_NB] = {cbr::SON_CROA1_LEN, cbr::SON_CROA2_LEN, cbr::SON_CROA3_LEN, cbr::SON_CROASSE1_LEN, cbr::SON_CROASSE2_LEN, 0, 0, 0, 0, 0, 0, 0, cbr::SON_ALARME_LEN, cbr::SON_CROA_TETON_LEN};
    d = D[id]; n = N[id]; rate = cbr::RATE; if (!d || !n) d = nullptr;
  }
#endif
#ifdef CB_AVEC_SONS
  if (!d) {
    static const uint8_t* const D[SN_NB] = {cbs::SON_CROA1, cbs::SON_CROA2, cbs::SON_CROA3, cbs::SON_CROASSE1, cbs::SON_CROASSE2, cbs::SON_AILES, cbs::SON_PLUMES, cbs::SON_ATTERRIT, cbs::SON_POP_APPARAIT, cbs::SON_POP_DISPARAIT, cbs::SON_GAZOUILLIS, cbs::SON_CROC, (const uint8_t*)nullptr, (const uint8_t*)nullptr};
    static const uint32_t N[SN_NB] = {cbs::SON_CROA1_LEN, cbs::SON_CROA2_LEN, cbs::SON_CROA3_LEN, cbs::SON_CROASSE1_LEN, cbs::SON_CROASSE2_LEN, cbs::SON_AILES_LEN, cbs::SON_PLUMES_LEN, cbs::SON_ATTERRIT_LEN, cbs::SON_POP_APPARAIT_LEN, cbs::SON_POP_DISPARAIT_LEN, cbs::SON_GAZOUILLIS_LEN, cbs::SON_CROC_LEN, 0, 0};
    d = D[id]; n = N[id]; rate = cbs::RATE;
  }
#endif
  if (!d || !n) return;
  uint32_t m = millis(); if (m - dernierSon[id] < 150) return; dernierSon[id] = m;
  if (!M5.Speaker.isEnabled()) {
    if (lectureEnCours) return;      // la Matinale demarre : on ne touche pas au haut-parleur
    M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume);
  }
  static uint8_t canal = 0; canal = (canal + 1) & 3;
  M5.Speaker.setChannelVolume(canal, vol);
  M5.Speaker.playRaw(d, n, rate, false, 1, canal, true);
}
static void sonCri() { static const int ids[7] = {SN_CROA1, SN_CROA2, SN_CROA3, SN_CROASSE1, SN_CROASSE2, SN_ALARME, SN_TETON}; int n = sonsReels ? 7 : 5; son(ids[random(n)], 190); }

// ---------- effets ----------
static void plumes(float x, float y, int n) {
  for (int k = 0; k < n; k++) {
    int idx = -1; float pire = 1e9f;
    for (int i = 0; i < NPART; i++) { if (!pl[i].actif) { idx = i; break; } if (pl[i].vie < pire) { pire = pl[i].vie; idx = i; } }
    Plume& p = pl[idx]; float a = rnd(0, 6.283f), v = rnd(30, 105);
    p.x = x + rnd(-6, 6); p.y = y + rnd(-5, 5); p.vx = cosf(a) * v; p.vy = sinf(a) * v * 0.8f - 14; p.rot = rnd(0, 6.283f); p.vr = rnd(-8, 8); p.dur = rnd(0.8f, 1.4f); p.vie = p.dur; p.actif = true;
  }
}
static void eclat(float x, float y, float dur, float taille) {
  int idx = 0; float pire = 1e9f;
  for (int i = 0; i < NFX; i++) { if (!fx[i].actif) { idx = i; pire = -1; break; } if (fx[i].dur - fx[i].t < pire) { pire = fx[i].dur - fx[i].t; idx = i; } }
  fx[idx].x = x; fx[idx].y = y; fx[idx].t = 0; fx[idx].dur = dur; fx[idx].taille = taille; fx[idx].actif = true;
}
static const float JAMBE = 5.6f;     // longueur des pattes visibles (sous le ventre)
static void hancheC(float s, float pitch, float dv, float& hx, float& hy) {      // position de la hanche par rapport au centre du corps
  float ct = cosf(pitch), st = sinf(pitch);
  hx = dv * s * (0.8f * ct + 4.7f * st); hy = s * (-0.8f * st + 4.7f * ct);
}
static void cible(const Corbeau& c, float sx, float sy, float dir, float& tx, float& ty) {      // centre du corps quand les pattes sont sur (sx, sy)
  float hx, hy; hancheC(c.s, c.pr, dir, hx, hy); tx = sx - hx; ty = sy - JAMBE * c.s - hy;
}
static void posOeil(const Corbeau& c, float& X, float& Y) {
  float ct = cosf(c.pitch), st = sinf(c.pitch);
  float hu = 8.2f + 4.8f * cosf(c.ha), hv = -2.0f + 4.8f * sinf(c.ha), cb = cosf(c.hb), sb = sinf(c.hb);
  float u = hu + 1.6f * cb + 1.0f * sb, v = hv + 1.6f * sb - 1.0f * cb;
  X = c.x + c.dv * c.s * (u * ct + v * st); Y = c.y + c.bob + c.s * (-u * st + v * ct);
}

// ---------- dessin d'un corbeau : silhouette noire, seuls les yeux brillent ----------
struct Rep { float cx, cy, d, s, ct, st; void p(float u, float v, float& X, float& Y) const { X = cx + d * s * (u * ct + v * st); Y = cy + s * (-u * st + v * ct); } };
static void polyS(M5Canvas& g, const float* p, int n, uint16_t c) {      // éventail depuis le premier point (pour les formes en étoile)
  for (int i = 1; i < n - 1; i++) tri(g, p[0], p[1], p[i * 2], p[i * 2 + 1], p[i * 2 + 2], p[i * 2 + 3], c);
}
static void dirAile(float e, float sw, float& du, float& dv) {
  float er = e * 0.0174533f; float a = -sw * cosf(er), b = -sinf(er); float l = sqrtf(a * a + b * b); if (l < 0.001f) l = 1; du = a / l; dv = b / l;
}
static void aile(M5Canvas& g, const Rep& R_, float su, float sv, const Aile& a, float wl, uint16_t col) {
  // panneau de l'avant-bras + 6 grandes plumes qui s'écartent en éventail
  if (wl < 0.04f) return;
  float dau, dav, dhu, dhv; dirAile(a.ea, a.sa, dau, dav); dirAile(a.eh, a.sh, dhu, dhv);
  float la = 17.0f * wl, lh = 23.0f * wl;
  float tau = -dav, tav = dau; if (tau > 0.05f || (fabsf(tau) <= 0.05f && tav < 0)) { tau = -tau; tav = -tav; }
  float thu = -dhv, thv = dhu; if (thu > 0.05f || (fabsf(thu) <= 0.05f && thv < 0)) { thu = -thu; thv = -thv; }
  float Wu = su + dau * la, Wv = sv + dav * la;
  float q[10];
  R_.p(su, sv, q[0], q[1]); R_.p(Wu - tau * 1.3f, Wv - tav * 1.3f, q[2], q[3]); R_.p(Wu + tau * 9.0f * wl, Wv + tav * 9.0f * wl, q[4], q[5]);
  R_.p(su + dau * la * 0.45f + tau * 10.0f * wl, sv + dav * la * 0.45f + tav * 10.0f * wl, q[6], q[7]); R_.p(su + tau * 6.0f * wl, sv + tav * 6.0f * wl, q[8], q[9]);
  polyS(g, q, 5, col);
  { float tx_, ty_;      // pointes déchirées au bord de fuite
    R_.p(Wu + tau * 14.0f * wl - dhu * 2.0f, Wv + tav * 14.0f * wl - dhv * 2.0f, tx_, ty_); tri(g, q[4], q[5], q[6], q[7], tx_, ty_, col);
    R_.p(su + dau * la * 0.45f + tau * 15.0f * wl, sv + dav * la * 0.45f + tav * 15.0f * wl, tx_, ty_); tri(g, q[6], q[7], q[8], q[9], tx_, ty_, col); }
  for (int k = 0; k < 7; k++) {
    float phi = k * 0.14f + ((k & 1) ? 0.05f : 0.0f), cph = cosf(phi), sph = sinf(phi);
    float du = dhu * cph + thu * sph, dv = dhv * cph + thv * sph, L = lh * (1.0f - 0.045f * k) * ((k & 1) ? 0.80f : 1.04f);
    float ru = Wu + dhu * 0.7f * k + tau * 0.6f * k, rv = Wv + dhv * 0.7f * k + tav * 0.6f * k, w = 2.4f * wl + 0.7f;
    float x1, y1, x2, y2, x3, y3;
    R_.p(ru - thu * w, rv - thv * w, x1, y1); R_.p(ru + thu * w, rv + thv * w, x2, y2); R_.p(ru + du * L, rv + dv * L, x3, y3);
    tri(g, x1, y1, x2, y2, x3, y3, col);
  }
}

static void dessinerCorbeau(M5Canvas& g, const Corbeau& c) {
  const uint16_t NOIR = C(4, 3, 6);
  Rep R_; R_.cx = c.x; R_.cy = c.y + c.bob; R_.d = c.dv; R_.s = c.s; R_.ct = cosf(c.pitch); R_.st = sinf(c.pitch);
  const float s = c.s, dv = c.dv;
  float q[32];
  auto P = [&](float u, float v, float& X, float& Y) { R_.p(u, v, X, Y); };
  // pattes : le pied reste posé sur le support, la hanche suit le corps (les orteils agrippent la branche)
  if (c.pied) {
    float hx, hy; P(0.8f, 4.7f, hx, hy); float sg = dv >= 0 ? 1.0f : -1.0f;
    for (int k = 0; k < 2; k++) {
      float sk = k ? 1.0f : -1.0f;
      float hipx = hx + sk * 1.3f * s * dv, hipy = hy - 0.4f;
      float fxk = c.fx + sk * 2.0f * s * dv, fyk = c.fy + c.fp * (fxk - c.fx);
      if (k == 1 && c.grat > 0.02f) {      // il se gratte : une patte monte vers la tête
        float sx_, sy_; P(7.0f, -0.5f, sx_, sy_); float a = pas(c.grat), wob = sinf(c.tAct * 26.0f) * 0.8f * a;
        fxk = lerpf(fxk, sx_ + wob, a); fyk = lerpf(fyk, sy_ + wob * 0.6f, a);
      }
      tri(g, hipx - 1.1f, hipy, hipx + 1.1f, hipy, fxk, fyk, NOIR);
      ligne(g, hipx, hipy, fxk, fyk, NOIR);
      ligne(g, fxk, fyk, fxk + sg * 3.4f * s, fyk + 0.3f, NOIR); ligne(g, fxk, fyk, fxk + sg * 2.4f * s, fyk - 0.7f, NOIR); ligne(g, fxk, fyk, fxk - sg * 1.8f * s, fyk + 0.3f, NOIR);
    }
  } else if (c.fan > 0.3f) {      // approche d'un atterrissage : les pattes pendent vers l'avant
    float hx, hy; P(0.8f, 4.7f, hx, hy); float e = c.fan;
    for (int k = 0; k < 2; k++) { float sk = k ? 1.0f : -1.0f; ligne(g, hx + sk * 1.3f * s * dv, hy, hx + dv * (3.0f + sk * 1.5f) * s, hy + 6.0f * s * e, NOIR); }
  }
  // ailes ouvertes (vol)
  if (c.wl > 0.04f) {
    Aile a2 = c.pose; a2.ea += 14; a2.eh += 10;
    aile(g, R_, 3.2f, -3.8f, a2, c.wl * 0.92f, NOIR);
    aile(g, R_, 1.6f, -3.2f, c.pose, c.wl, NOIR);
  }
  // queue en coin (s'ouvre en éventail ; elle bat quand il se secoue)
  { float wag = (c.ruf <= 0.01f) ? 0.0f : 1.2f * c.ruf * sinf(c.tAct * 38.0f), f = c.fan;
    const float t[10] = {-8.0f, -2.6f, -24.5f, 2.0f - 6.0f * f + wag, -26.8f, 3.8f + wag, -24.5f, 5.4f + 6.0f * f + wag, -8.0f, 3.8f};
    for (int i = 0; i < 5; i++) P(t[i * 2], t[i * 2 + 1], q[i * 2], q[i * 2 + 1]);
    poly(g, q, 5, NOIR); }
  // corps : la respiration gonfle la poitrine, le secouement hérisse les plumes
  float br = 1.0f + 0.05f * sinf(c.resp * 2.7f) + 0.09f * c.ruf * sinf(c.tAct * 46.0f);
  for (int i = 0; i < 14; i++) { float a = i * (6.2832f / 14.0f); P(cosf(a) * 11.8f * (1.0f + 0.02f * (br - 1.0f) * 5.0f), sinf(a) * 5.9f * br, q[i * 2], q[i * 2 + 1]); }
  poly(g, q, 14, NOIR);
  for (int i = 0; i < 10; i++) { float a = i * 0.6283f; P(6.6f + cosf(a) * 5.4f * (0.5f + 0.5f * br), 1.4f + sinf(a) * 5.2f * br, q[i * 2], q[i * 2 + 1]); }      // poitrine bombée
  poly(g, q, 10, NOIR);
  if (c.wl < 0.6f) {      // aile repliée sur le flanc, par-dessus la queue
    const float wp[12] = {5.5f, -5.6f, -5.0f, -6.6f, -17.0f, -2.0f, -23.0f, 3.6f, -13.0f, 3.4f, -2.0f, 3.6f};
    for (int i = 0; i < 6; i++) P(wp[i * 2], wp[i * 2 + 1], q[i * 2], q[i * 2 + 1]);
    poly(g, q, 6, NOIR);
  }
  // cou et tête : la tête tourne (angle du cou ha, angle du bec hb)
  float hu = 8.2f + 4.8f * cosf(c.ha), hv = -2.0f + 4.8f * sinf(c.ha), cb = cosf(c.hb), sb = sinf(c.hb);
  auto PH = [&](float p, float qq, float& X, float& Y) { P(hu + p * cb - qq * sb, hv + p * sb + qq * cb, X, Y); };
  float n1x, n1y, n2x, n2y, hhx, hhy; P(3.5f, -5.2f, n1x, n1y); P(7.5f, 3.8f, n2x, n2y); P(hu, hv, hhx, hhy);
  tri(g, n1x, n1y, n2x, n2y, hhx, hhy, NOIR);
  disque(g, hhx, hhy, 5.0f * s, NOIR);
  { float a1x, a1y, a2x, a2y, a3x, a3y; PH(-3.8f, 1.5f, a1x, a1y); PH(0.8f, 4.0f, a2x, a2y); PH(-4.6f, 6.0f, a3x, a3y); tri(g, a1x, a1y, a2x, a2y, a3x, a3y, NOIR); }      // gorge hirsute
  { float o = c.bec, Ax, Ay, Bx, By, Mx, My, Tx, Ty, Lx, Ly, Ux, Uy;      // bec lourd, qui s'ouvre quand il croasse
    PH(2.4f, -2.8f, Ax, Ay); PH(2.6f, 3.3f, Bx, By); PH(2.8f, 0.2f, Mx, My); PH(13.5f, 1.0f - 3.4f * o, Tx, Ty); PH(11.8f, 2.2f + 3.6f * o, Lx, Ly); PH(7.6f, -3.0f, Ux, Uy);
    tri(g, Ax, Ay, Mx, My, Tx, Ty, NOIR); tri(g, Mx, My, Bx, By, Lx, Ly, NOIR); tri(g, Ax, Ay, Ux, Uy, Tx, Ty, NOIR); }
  // objet porté dans le bec
  if (c.objet) {
    float X, Y; PH(11.0f, 2.6f, X, Y);
    float tw = (sinf(millis() * 0.0081f) > 0.6f) ? 1.0f : 0.0f;
    dessinerObjet(g, c.objet - 1, X + dv * 1.5f, Y + 2.0f, 3.6f * s + 0.8f, dv > 0 ? 0.4f : 2.7f, tw);
  }
  // œil rouge qui brille
  { float ex, ey; PH(1.6f, -1.0f, ex, ey);
    if (c.clin < 0.5f) { disque(g, ex, ey, 1.7f * s + 0.6f, oeilAnn); disque(g, ex, ey, 1.1f * s + 0.2f, oeilCoeur); }
    else ligne(g, ex - 1.5f, ey, ex + 1.5f, ey, C(70, 8, 12));
    if (c.flash > 0.01f) { disque(g, ex, ey, 3.0f + 7.0f * c.flash, CF(255, 60 + 120 * c.flash, 40 + 90 * c.flash)); disque(g, ex, ey, 1.8f, C(255, 240, 225)); } }
}


// ---------- le visiteur, de dos : long manteau, col haut, cheveux attachés en queue basse (dessiné par le code) ----------
static void dessinerVisiteur(M5Canvas& g) {
  const uint16_t NOIR = C(4, 3, 6), RIM = C(96, 18, 24);
  const float t = tAnim, y0 = visPiedsY(), sw = V.sw, arm = V.arm;
  auto X = [&](float u, float h) { return VX + u + sw * powf(h / 60.0f, 1.4f); };
  auto Y = [&](float h) { return y0 - h; };
  auto T = [&](float au, float ah, float bu, float bh, float cu, float ch) { tri(g, X(au, ah), Y(ah), X(bu, bh), Y(bh), X(cu, ch), Y(ch), NOIR); };
  auto fan = [&](const float* p, int n) { for (int i = 1; i < n - 1; i++) T(p[0], p[1], p[i * 2], p[i * 2 + 1], p[i * 2 + 2], p[i * 2 + 3]); };
  auto cen = [&](const float* p, int n) {
    float cx = 0, cy = 0; for (int i = 0; i < n; i++) { cx += p[i * 2]; cy += p[i * 2 + 1]; } cx /= n; cy /= n;
    for (int i = 0; i < n; i++) { int j = (i + 1) % n; T(cx, cy, p[i * 2], p[i * 2 + 1], p[j * 2], p[j * 2 + 1]); }
  };
  float f1 = 1.6f * sinf(t * 2.3f), f2 = 1.6f * sinf(t * 2.3f + 1.7f), f3 = 1.0f * sinf(t * 3.1f + 0.6f);
  { const float b1[8] = {-9, 0, -9.5f, 4, -5, 4, -4.5f, 0}, b2[8] = {4.5f, 0, 5, 4, 9.5f, 4, 9, 0}; fan(b1, 4); fan(b2, 4); }      // bottes
  { const float l[14] = {0, 46, -10.5f, 46, -9.5f, 30, -11.5f, 18, -15.5f + f1, 3, -7.5f + f3, 1.5f, -0.7f, 4};      // deux pans de manteau, fente dans le dos
    const float r[14] = {0, 46, 10.5f, 46, 9.5f, 30, 11.5f, 18, 15.5f + f2, 3, 7.5f - f3, 1.5f, 0.7f, 4};
    fan(l, 7); fan(r, 7); }
  { const float e[8] = {-10.5f, 46, -6.5f, 49, 6.5f, 49, 10.5f, 46}; cen(e, 4); }      // épaules
  { const float m[10] = {-10.5f, 46, -13.8f - f3 * 0.3f, 34, -14.5f - f3 * 0.5f, 22, -9.8f, 21, -9.0f, 40}; fan(m, 5); }      // manche gauche qui pend
  // manche droite : elle se tend quand on garde le doigt sur lui
  float phi = lerpf(0.14f, 1.75f, pas(arm)), du = sinf(phi), dh = -cosf(phi), nu = -dh, nh = du, Su = 10.0f, Sh = 45.0f, ln = 25.0f;
  { const float m[8] = {Su + nu * 3.2f, Sh + nh * 3.2f, Su - nu * 3.2f, Sh - nh * 3.2f, Su + du * ln - nu * 3.0f, Sh + dh * ln - nh * 3.0f, Su + du * ln + nu * 3.8f, Sh + dh * ln + nh * 3.8f}; cen(m, 4); }
  if (arm > 0.2f) { float a = pas(arm); T(Su + du * 6, Sh + dh * 6, Su + du * ln, Sh + dh * ln, Su + du * 14 - 1, Sh + dh * 14 - 6 * a); }
  { float hx = Su + du * (ln + 1.5f), hy = Sh + dh * (ln + 1.5f); disque(g, X(hx, hy), Y(hy), 2.0f, NOIR); }
  { const float c[12] = {-8, 46, -6.8f, 58, -2.6f, 56.5f, 2.6f, 56.5f, 6.8f, 58, 8, 46}; cen(c, 6); }      // col haut
  ell(g, X(0, 61), Y(61), 4.9f, 5.3f, NOIR); ell(g, X(0, 63.4f), Y(63.4f), 4.6f, 3.6f, NOIR);      // tête de dos
  T(-4.2f, 64, -6.5f, 60.5f + f3 * 0.4f, -3.4f, 61);
  { float pu = 0, ph = 0, pw = 0; bool a = false;      // cheveux attachés en queue basse, qui ondule au vent
    for (int i = 0; i < 8; i++) {
      float s = i / 7.0f, u = -2.0f * s + 3.4f * s * s * sinf(t * 2.6f - s * 2.2f) + sw * 0.3f + 1.5f * s, h = 57.5f - 25.0f * s, w = lerpf(1.9f, 0.4f, s);
      if (a) { T(pu - pw, ph, pu + pw, ph, u - w, h); T(pu + pw, ph, u + w, h, u - w, h); }
      pu = u; ph = h; pw = w; a = true;
    } }
  ligne(g, X(10.5f, 46), Y(46), X(15.5f + f2, 3), Y(3), RIM); ligne(g, X(6.8f, 58), Y(58), X(8, 46), Y(46), RIM); ligne(g, X(4.0f, 63), Y(63), X(4.9f, 60), Y(60), RIM);      // liseré côté lune
}

// ---------- petit corbeau de la nuée ----------
static void dessinerMini(M5Canvas& g, const Mini& m) {
  float S = 3.4f * ((m.mode == 2) ? lim(m.vie * 1.4f, 0.2f, 1.0f) : 1.0f);
  float c = cosf(m.a), sn = sinf(m.a), fv = (c < 0) ? -1.0f : 1.0f;
  auto P = [&](float u, float v, float& X, float& Y) { X = m.x + (u * c - v * sn) * S; Y = m.y + (u * sn + v * c) * S; };
  const uint16_t NOIR = C(4, 3, 6); float q[14];
  P(1.0f, 0, q[0], q[1]); P(0.5f, -0.34f, q[2], q[3]); P(-0.6f, -0.3f, q[4], q[5]); P(-1.1f, 0, q[6], q[7]); P(-0.6f, 0.3f, q[8], q[9]); P(0.5f, 0.34f, q[10], q[11]);
  poly(g, q, 6, NOIR);
  float hx, hy; P(0.95f, -0.12f, hx, hy); disque(g, hx, hy, 0.34f * S + 0.4f, NOIR);
  float bx, by, bx2, by2, bx3, by3; P(1.15f, -0.2f, bx, by); P(1.75f, 0, bx2, by2); P(1.15f, 0.1f, bx3, by3); tri(g, bx, by, bx2, by2, bx3, by3, NOIR);
  float vt = -1.7f * cosf(6.2832f * 9.0f * tAnim + m.ph) * fv;
  P(0.45f, -0.1f, q[0], q[1]); P(-0.55f, -0.1f, q[2], q[3]); P(-0.95f, vt * 0.78f, q[4], q[5]); P(-0.45f, vt, q[6], q[7]); P(0.2f, vt * 0.7f, q[8], q[9]);
  poly(g, q, 5, NOIR);
  if (S > 2.6f) disque(g, hx + c * 0.05f * S, hy - 0.05f * S, 0.7f, C(255, 52, 38));
}

// ---------- logique ----------
static float perchY(int i) { return i < 4 ? ytop(PERCH[i][0]) + 0.4f : PERCH[i][1]; }
static float perchP(int i) { static const float PT_[2] = {-0.33f, 0.43f}; return i < 4 ? (ytop(PERCH[i][0] + 4) - ytop(PERCH[i][0] - 4)) / 8.0f : PT_[i - 4]; }
static bool perchOccupe(int i) { if (V.ph != V_RIEN && (i == 1 || i == 2)) return true; for (int k = 0; k < NCB; k++) if (cr[k].perch == i) return true; return false; }
static int perchLibre() {
  int n = 0, l[NPE]; for (int i = 0; i < NPE; i++) if (!perchOccupe(i)) l[n++] = i;
  return n ? l[random(n)] : -1;
}
static int nbDehors() { int n = 0; for (int k = 0; k < NCB; k++) if (cr[k].etat == E_DEHORS || (cr[k].etat == E_VOL && cr[k].but == B_SORTIR)) n++; return n; }

static void initCorbeau(Corbeau& c, float s) {
  memset(&c, 0, sizeof(c)); c.s = s; c.d = 1; c.dv = 1; c.visible = true; c.perch = -1; c.item = -1;
  c.pr = 0.34f + rnd(-0.04f, 0.06f); c.pitch = c.pitchC = c.pr; c.ha = c.haC = -0.42f; c.hb = c.hbC = 0.07f;
  c.pose = c.poseC = A_PLANE; c.tClin = rnd(1.0f, 4.0f); c.resp = rnd(0, 6.28f); c.act = A_IMMO; c.dAct = rnd(1.0f, 5.0f);
}
static void placer(Corbeau& c, int i) {      // posé sur la branche : les pattes sur le dessus, le corps déduit
  c.perch = i; c.fx = PERCH[i][0]; c.fy = perchY(i); c.fp = perchP(i); c.pied = true; c.vx = c.vy = 0; c.etat = E_PERCHE; c.but = B_RIEN; c.t = 0; c.tfin = rnd(4, 11);
  c.visible = true; c.wl = c.wlC = 0; c.fan = c.fanC = 0; c.hop = 0; c.bob = 0; c.sub = 0; c.dv = (float)c.d; c.pitch = c.pitchC = c.pr; c.pv = 0; c.ox = c.oy = 0;
  c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.0f, 5.0f); c.grat = c.gratC = 0; c.ha = c.haC = -0.42f; c.hb = c.hbC = 0.07f; c.hav = c.hbv = 0;
  float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy); c.x = c.fx - hx; c.y = c.fy - JAMBE * c.s - hy;
}
static void decoller(Corbeau& c, float tx, float ty, uint8_t but) {      // pour B_SORTIR tx,ty = point à atteindre ; sinon = appui des pattes visé
  c.etat = E_DECOLLE; c.t = 0; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.sub = 0; c.act = A_REPOS; c.tAct = 0;
  c.d = (tx >= c.x) ? 1 : -1; c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = 0.9f;
}
static void entrer(Corbeau& c, float tx, float ty, uint8_t but) {      // arrive par un bord de l'écran
  bool gauche = (tx < 160) ? (rnd01() < 0.7f) : (rnd01() < 0.3f);
  c.x = gauche ? -34.0f : 354.0f; c.y = rnd(18, 110); c.d = gauche ? 1 : -1; c.dv = (float)c.d; c.vx = c.d * 90.0f; c.vy = 0; c.pied = false;
  c.etat = E_VOL; c.t = 1.5f; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.visible = true; c.sub = 0; c.act = A_REPOS;
  c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = rnd01(); c.wl = c.wlC = 1; c.fan = c.fanC = 0; c.pitch = c.pitchC = 0.1f; c.pose = c.poseC = A_BAS;
}
static void sortir(Corbeau& c) {
  c.perch = -1; c.item = -1;
  float tx = (rnd01() < 0.5f) ? -50.0f : 370.0f;
  if (c.x < 100) tx = -50; else if (c.x > 220) tx = 370;
  decoller(c, tx, rnd(8, 90), B_SORTIR);
}
static void versPerchoir(Corbeau& c) {
  int i = perchLibre();
  if (i < 0) { if (nbDehors() < 3) sortir(c); else { c.t = 0; if (c.etat == E_SOL) { c.but = B_RIEN; c.sub = 2; } } return; }
  c.perch = i; c.item = -1; decoller(c, PERCH[i][0], perchY(i), B_PERCHOIR);
}
static void apresSol(Corbeau& c) {
  if (rnd01() < 0.3f && nbDehors() < 2) sortir(c); else versPerchoir(c);
}

static void attribuer(int iO);
static void lacher(int type, float x) {       // une nourriture tombe du ciel
  int idx = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { idx = i; break; }
  if (idx < 0) { float vieux = -1; for (int i = 0; i < NOB; i++) if (ob[i].t > vieux && ob[i].repos) { vieux = ob[i].t; idx = i; } if (idx < 0) return; }
  Obj& o = ob[idx]; o.actif = true; o.type = (uint8_t)type; o.x = lim(x, 12, 308); o.y = -6; o.vy = 20; o.t = 0; o.repos = false; o.reserve = -1;
  for (int k = 0; k < NCB; k++) if (cr[k].item == idx) { cr[k].item = -1; if (cr[k].but == B_NOUR) cr[k].but = B_RIEN; }
  son(SN_CROC, 110);
  for (int k = 0; k < NCB; k++) {      // les corbeaux perchés tournent la tête vers la nourriture tout de suite
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible) continue;
    c.d = (o.x >= c.x) ? 1 : -1; c.look = lim(atan2f(solHaut(o.x) - c.y, fabsf(o.x - c.x) + 8.0f) * 0.7f, -0.3f, 0.8f); c.tLook = 1.6f; c.clin = 0; c.tClin = rnd(1.5f, 3.0f);
  }
  attribuer(idx);                      // et l'un d'eux part sans attendre
}

static void attribuer(int iO) {       // un corbeau libre vient chercher la nourriture
  Obj& o = ob[iO]; int best = -1; float bd = 1e9f;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    bool ok = (c.etat == E_PERCHE && c.objet == 0 && c.perch < 10) || c.etat == E_DEHORS || (c.etat == E_SOL && c.but == B_RIEN);
    if (!ok || c.but == B_NOUR || c.rapporte) continue;
    float px = (c.etat == E_DEHORS) ? 160.0f + (k % 2 ? 200.0f : -200.0f) : c.x; float dd = fabsf(px - o.x) + rnd(0, 25);
    if (dd < bd) { bd = dd; best = k; }
  }
  if (best < 0) return;
  Corbeau& c = cr[best]; float x0 = (c.etat == E_DEHORS) ? ((best % 2) ? 354.0f : -34.0f) : c.x;
  int dsg = (o.x >= x0) ? 1 : -1;
  float gx = lim(o.x - dsg * 19.0f * c.s, 10, 310), gy = solHaut(gx);
  c.perch = -1; c.item = iO; o.reserve = best; c.objet = 0;
  if (c.etat == E_PERCHE || c.etat == E_SOL) { c.sub = 0; decoller(c, gx, gy, B_NOUR); }
  else { entrer(c, gx, gy, B_NOUR); }
  c.item = iO;
}

static void atterrir(Corbeau& c) {
  if (c.but == B_VISITEUR && c.perch >= 10) { ptVis(c.perch - 10, c.fx, c.fy); c.fp = 0; }
  else if (c.but == B_PERCHOIR && c.perch >= 0) { c.fx = PERCH[c.perch][0]; c.fy = perchY(c.perch); c.fp = perchP(c.perch); }
  else { c.fx = c.tx; c.fy = c.ty; c.fp = 0; }
  float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy);
  c.ox = c.x - (c.fx - hx); c.oy = c.y - (c.fy - JAMBE * c.s - hy);      // l'écart restant se résorbe en douceur
  c.pied = true; c.etat = E_ATTERRIT; c.t = 0; c.vx = c.vy = 0; son(SN_ATTERRIT, 170);
}

// ----- moments de calme d'un corbeau perché : immobile, regarde autour, lisse ses plumes, se gratte, se secoue -----
static void prochainActe(Corbeau& c) {
  uint8_t prec = c.act; c.tAct = 0; int r = random(100); bool lib = gCalme <= 0;
  if ((prec == A_LISSE_DOS || prec == A_LISSE_POIT) && random(100) < 45) { c.act = A_SECOUE; c.dAct = 0.9f; return; }
  if (r < 30) { c.act = A_IMMO; c.dAct = rnd(3, 8); }
  else if (r < 55) { c.act = A_REGARDE; c.dAct = rnd(3, 6); c.sub = -1; }
  else if (r < 73 && lib) { c.act = random(2) ? A_LISSE_DOS : A_LISSE_POIT; c.dAct = rnd(3.0f, 4.5f); gCalme = c.dAct + rnd(2, 5); }
  else if (r < 84 && lib) { c.act = A_GRATTE; c.dAct = rnd(1.9f, 2.6f); gCalme = c.dAct + rnd(2, 5); }
  else if (r < 90 && lib) { c.act = A_SECOUE; c.dAct = 0.9f; gCalme = c.dAct + rnd(1, 3); }
  else { c.act = A_IMMO; c.dAct = rnd(2, 5); }
}
static void acteur(Corbeau& c, float dt) {
  c.tAct += dt; if (c.tAct >= c.dAct) prochainActe(c);
  float a = c.tAct, dA = c.dAct;
  float e = pas(a / 0.6f) * (1.0f - pas((a - (dA - 0.6f)) / 0.6f));
  switch (c.act) {
    case A_IMMO: c.haC = -0.42f + 0.015f * sinf(c.resp * 0.5f); c.hbC = c.haC + 0.49f; break;
    case A_REGARDE: {
      static const float AN[4] = {-0.9f, -0.1f, -0.62f, 0.12f};
      int k = ((int)(a / 1.3f) + (int)(c.s * 10)) & 3;
      if (k != c.sub) { c.sub = k; if (k == 2 && random(100) < 45) { c.d = -c.d; c.surs = 0.22f; } }      // il se retourne d'un petit saut
      c.haC = AN[k]; c.hbC = c.haC + 0.49f; c.pitchC = c.pr + 0.05f * sinf(a * 3.0f);
    } break;
    case A_LISSE_DOS: c.haC = lerpf(-0.42f, -2.55f, e); c.hbC = lerpf(0.07f, 3.15f, e) + 0.28f * e * sinf(a * 17.0f); c.pitchC = c.pr + 0.05f * e * sinf(a * 8.5f); c.bob += 0.4f * c.s * e * sinf(a * 17.0f); break;
    case A_LISSE_POIT: c.haC = lerpf(-0.42f, 1.95f, e); c.hbC = lerpf(0.07f, 2.2f, e) + 0.25f * e * sinf(a * 15.0f); c.pitchC = c.pr - 0.10f * e; break;
    case A_GRATTE: { float g = pas(a / 0.35f) * (1.0f - pas((a - (dA - 0.35f)) / 0.35f)); c.gratC = g; c.haC = lerpf(-0.42f, -0.15f, g); c.hbC = lerpf(0.07f, 0.4f, g); c.pitchC = c.pr - 0.07f * g; } break;
    case A_SECOUE: c.ruf = sinf(3.1416f * a / dA); c.pitchC = c.pr + 0.06f * c.ruf * sinf(a * 38.0f); c.haC = -0.42f; c.hbC = 0.07f; break;
    default: c.haC = -0.42f + 0.07f * sinf(c.resp * 0.8f + c.s * 7.0f); c.hbC = c.haC + 0.49f; break;
  }
}

static void majCorbeau(Corbeau& c, int idx, float dt) {
  c.t += dt; c.resp += dt;
  c.tClin -= dt; if (c.tClin <= 0) { c.clin = 1; c.tClin = rnd(2.2f, 5.5f); }
  if (c.clin > 0) { c.clin -= dt * 4.5f; if (c.clin < 0) c.clin = 0; }
  if (c.bec > 0) { c.bec -= dt * 3.0f; if (c.bec < 0) c.bec = 0; }
  if (c.flash > 0) { c.flash -= dt * 4.0f; if (c.flash < 0) c.flash = 0; }
  if (c.tLook > 0) c.tLook -= dt;
  c.bob = 0; c.hop = 0; c.ruf = 0;
  c.haC = -0.42f; c.hbC = 0.07f; c.gratC = 0; c.wlC = 0; c.fanC = 0; c.poseC = A_PLANE; c.pitchC = c.pr;
  if (c.surs > 0) { c.surs -= dt; if (c.surs < 0) c.surs = 0; if (c.etat == E_PERCHE || c.etat == E_SOL) c.bob = -3.2f * c.s * sinf(c.surs / 0.30f * 3.1416f); }

  switch (c.etat) {
    case E_PERCHE: {
      c.pied = true; c.vx = c.vy = 0;
      if (c.perch >= 10) { if (V.ph == V_RIEN) { c.perch = -1; versPerchoir(c); break; } ptVis(c.perch - 10, c.fx, c.fy); c.fp = 0; }      // posé sur le visiteur : il en suit le balancement
      c.bob += 0.30f * c.s * sinf(c.resp * 2.7f);                   // respiration
      c.pitchC = c.pr + 0.015f * sinf(c.resp * 0.9f);               // léger balancement
      acteur(c, dt);
      if (c.objet && c.t > 9.0f) { c.objet = 0; eclat(c.x + c.dv * 18 * c.s, c.y - 2, 0.3f, 0.7f); }
      if (c.perch < 10 && c.t >= c.tfin && gAction <= 0 && c.act <= A_REGARDE) {
        int r = random(100);
        if (r < 40) { versPerchoir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 62 && nbDehors() < 2) { sortir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 80) { c.d = -c.d; c.surs = 0.22f; c.t = 0; c.tfin = rnd(3, 9); }
        else { c.bec = 1; sonCri(); c.t = 0; c.tfin = rnd(4, 11); }
      }
    } break;
    case E_DECOLLE: {
      float u = c.t / 0.11f; c.poseC = A_HAUT; c.wlC = 1; c.pitchC = 0.55f; c.haC = -0.55f; c.hbC = -0.06f;
      c.bob = 2.6f * c.s * pas(u > 1 ? 1 : u);                // il s'accroupit avant de sauter
      if (c.t < 0.02f) son(SN_AILES, 200);
      if (c.t >= 0.11f) {
        c.etat = E_VOL; c.t = 0; c.vx = c.d * 46.0f; c.vy = -105.0f; c.bob = 0; c.pied = false;
        if (c.gerbe) { c.gerbe = false; c.vx = rnd(-120, 120); c.vy = -rnd(110, 170); }      // envol en gerbe c.perch = (c.but == B_PERCHOIR) ? c.perch : -1;
        float dist = hypotf(c.tx - c.x, c.ty - c.y);
        if (c.but == B_SORTIR) { c.wx = c.x + c.d * 70.0f; c.wy = mnf(c.y - 50.0f, 60.0f) - rnd(0, 20); c.aWp = true; }
        else if (dist > 110 && c.but != B_ORBITE) { c.wx = (c.x + c.tx) * 0.5f + rnd(-20, 20); c.wy = mnf(c.y, c.ty) - rnd(14, 50); c.aWp = true; }
      }
    } break;
    case E_VOL: {
      c.wlC = 1; c.pied = false;
      float gx, gy; bool orb = (c.but == B_ORBITE);
      if (orb) { c.ang += c.om * dt; float lead = (c.om > 0 ? 0.35f : -0.35f); gx = VX + c.ray * cosf(c.ang + lead); gy = visPiedsY() - 42.0f + c.ray * 0.5f * sinf(c.ang + lead); }
      else if (c.aWp) { gx = c.wx; gy = c.wy; } else if (c.but == B_SORTIR) { gx = c.tx; gy = c.ty; }
      else { if (c.but == B_VISITEUR && c.perch >= 10) ptVis(c.perch - 10, c.tx, c.ty); cible(c, c.tx, c.ty, (float)c.d, gx, gy); }
      float dx = gx - c.x, dy = gy - c.y, dist = sqrtf(dx * dx + dy * dy); if (dist < 0.001f) dist = 0.001f;
      if (c.aWp && dist < 18) { c.aWp = false; break; }
      bool fin = !c.aWp && !orb, proche = fin && dist < 38.0f && c.but != B_SORTIR;
      float sp = fin ? lim(dist * 2.4f + 16.0f, 16.0f, c.vitesse) : c.vitesse;
      if (c.but == B_SORTIR) sp = c.vitesse * 1.05f;
      if (orb) sp = 125.0f;
      float k = mnf(1.0f, dt * 3.0f);
      c.vx += (dx / dist * sp - c.vx) * k; c.vy += (dy / dist * sp - c.vy) * k;
      c.x += c.vx * dt; c.y += c.vy * dt;
      if (fabsf(c.vx) > 12 && !proche) c.d = (c.vx > 0) ? 1 : -1; else if (proche) c.d = (dx >= 0) ? 1 : -1;
      c.pitchC = proche ? 0.62f : lim(-c.vy * 0.006f + 0.1f, -0.4f, 0.5f); c.haC = -0.30f; c.hbC = 0.19f;
      // battements ou plané
      if (proche) {
        c.phase += 3.5f * dt; if (c.phase >= 1) c.phase -= 1;
        c.poseC = mixA(A_FLARE, A_BAS, 0.30f * lim(coup(c.phase), 0, 1)); c.fanC = 1;
        c.bob = -1.0f * c.s * coup(c.phase);
      } else if (c.planeT > 0) {
        c.planeT -= dt; c.poseC = A_PLANE; c.poseC.ea += 2.0f * sinf(c.t * 9.0f); c.vy += 6.0f * dt;
      } else {
        float h = c.hz * (c.t < 1.0f ? 1.25f : 1.0f);
        float p0 = c.phase; c.phase += h * dt;
        if (c.phase >= 1) {
          c.phase -= 1; c.restBat--;
          if (c.restBat <= 0) { c.restBat = 3 + random(4); if (c.t > 1.2f && c.but != B_SORTIR && rnd01() < 0.55f && dist > 60) c.planeT = rnd(0.5f, 1.3f); }
        }
        if (p0 > 0.9f && c.phase < 0.1f) son(SN_AILES, 120);
        float cp = coup(c.phase); c.poseC = mixA(A_HAUT, A_BAS, cp); c.bob = -2.4f * c.s * (cp - 0.4f);
      }
      if (c.but == B_SORTIR) { if (c.x < -32 || c.x > 352) { c.etat = E_DEHORS; c.t = 0; c.visible = false; c.tfin = c.fuite ? rnd(2.0f, 4.5f) : (c.rapporte ? rnd(3.5f, 6.0f) : rnd(6, 16)); c.fuite = false; c.wl = c.wlC = 0; } }
      else if (fin && (dist < 2.5f || (dist < 7.0f && sqrtf(c.vx * c.vx + c.vy * c.vy) < 20.0f))) atterrir(c);
    } break;
    case E_ATTERRIT: {
      float u = c.t / 0.34f; c.pied = true;
      c.wlC = (c.t > 0.06f) ? 0.0f : 1.0f; c.poseC = A_FLARE; c.fanC = (u < 0.55f) ? 1.0f : 0.0f;
      c.bob = 1.5f * c.s * sinf(lim(u, 0, 1) * 3.1416f);
      if (c.t >= 0.34f) {
        c.t = 0; c.wlC = 0; c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.5f, 4.0f);
        if (c.but == B_PERCHOIR) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(5, 13); c.ox = c.oy = 0; }
        else if (c.but == B_VISITEUR) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(5, 13); c.ox = c.oy = 0; c.d = (c.perch == 10) ? -1 : 1; }
        else if (c.but == B_NOUR || c.but == B_OBJET) { c.etat = E_SOL; c.sub = 0; }
        else { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; }
      }
    } break;
    case E_SOL: {
      c.pied = true; c.pitchC = c.pr * 0.45f;
      if (c.but == B_NOUR && c.sub <= 1) {
        Obj* o = (c.item >= 0 && c.item < NOB && ob[c.item].actif && ob[c.item].reserve == idx) ? &ob[c.item] : nullptr;
        if (!o) { c.but = B_RIEN; c.item = -1; apresSol(c); break; }
        if (!o->repos) { c.d = (o->x >= c.x) ? 1 : -1; c.haC = -0.15f; c.hbC = 0.34f; break; }      // la nourriture tombe encore : il attend
        float gx = o->x - c.d * 19.0f * c.s;
        if (c.sub == 0) {
          float dxm = gx - c.x; c.d = (o->x >= c.x) ? 1 : -1;
          if (fabsf(dxm) < 1.8f) { c.sub = 1; c.peck = 0; }
          else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 34.0f * dt); c.hop = -fabsf(sinf(c.t * 11.0f)) * 2.6f * c.s; c.bob = c.hop; c.pitchC = c.pr * 0.6f; c.haC = -0.2f; c.hbC = 0.3f; }
        } else if (c.sub == 1) {
          c.peck += dt; float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f));
          c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
          if (c.peck > 0.24f && o->actif && o->reserve == idx) {      // il prend la nourriture
            son(SN_CROC, 200);
            if (o->type >= T_BRILLANT) { c.objet = o->type + 1; o->actif = false; c.item = -1; c.sub = 2; c.peck = 0; c.t = 0; c.flash = 0.5f; }
            else { o->actif = false; c.item = -1; c.sub = 3; c.peck = 0; }
          }
        }
      }
      if (c.but == B_NOUR && c.sub == 3) {       // il picore ce qu'il a pris
        c.peck += dt; float dip = fabsf(sinf(c.peck / 0.40f * 3.1416f)); c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
        if (c.peck > 0.9f && c.peck - dt <= 0.9f) { son(SN_GAZ, 150); }
        if (c.peck > 1.8f) { c.but = B_RIEN; apresSol(c); }
      } else if (c.but == B_NOUR && c.sub == 2) { c.peck += dt; if (c.peck > 0.7f) { c.but = B_RIEN; apresSol(c); } }
      else if (c.but == B_OBJET) {
        if (c.sub == 0) { c.sub = 1; c.peck = 0; c.d = (c.x < 160) ? 1 : -1; }
        else if (c.sub == 1) {
          c.peck += dt; c.bec = lim((c.peck - 0.35f) * 4.0f, 0, 1); c.haC = 0.2f; c.hbC = 0.7f;
          if (c.peck > 0.55f && c.objet) {       // il dépose l'objet devant lui
            int idxO = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { idxO = i; break; }
            if (idxO < 0) idxO = 0;
            Obj& o = ob[idxO]; o.actif = true; o.type = c.objet - 1; o.x = c.x + c.d * 20.0f * c.s; o.y = solHaut(o.x) - 8.0f; o.vy = -40; o.t = 0; o.repos = false; o.reserve = -2;
            c.objet = 0; son(SN_CROC, 170); c.sub = 2; c.peck = 0;
          }
        } else if (c.sub == 2) { c.peck += dt; if (c.peck > 0.9f) { c.but = B_RIEN; apresSol(c); } }
      }
      if (c.but == B_RIEN && c.sub == 2 && c.etat == E_SOL && c.t > 0.5f) { c.t = 0; apresSol(c); }
    } break;
    case E_DEHORS: {
      c.visible = false; c.wl = c.wlC = 0;
      if (c.t >= c.tfin) {
        if (c.rapporte) {           // il revient avec un objet qu'il dépose par terre
          c.rapporte = false; c.objet = 7 + random(4);
          float gx = rnd(110, 300); c.perch = -1; entrer(c, gx, solHaut(gx), B_OBJET);
        } else {
          int i = perchLibre();
          if (i >= 0) { c.perch = i; entrer(c, PERCH[i][0], perchY(i), B_PERCHOIR); }
          else c.tfin = c.t + 1.0f;
        }
      }
    } break;
    case E_POOF: {
      if (c.t > 0.07f) c.visible = false;
      if (c.t >= c.tfin) {          // il réapparaît ailleurs, en un clin d'œil
        int i = perchLibre(); if (i < 0) i = 0;
        c.d = (rnd01() < 0.5f) ? 1 : -1; placer(c, i); c.flash = 1.0f;
        float X, Y; posOeil(c, X, Y); eclat(X, Y, 0.25f, 1.0f); plumes(c.x, c.y, 10); son(SN_POP_APP, 220); son(SN_PLUMES, 120);
      }
    } break;
  }

  // la tête suit un regard imposé (toucher, nourriture) ; tous les mouvements sont lissés
  if (c.tLook > 0 && (c.etat == E_PERCHE || (c.etat == E_SOL && c.but != B_NOUR))) { c.haC = c.look; c.hbC = c.look + 0.49f; }
  ressort(c.ha, c.hav, c.haC, 11.0f, dt); ressort(c.hb, c.hbv, c.hbC, 12.0f, dt); ressort(c.pitch, c.pv, c.pitchC, 9.0f, dt);
  c.dv += ((float)c.d - c.dv) * mnf(1.0f, dt * 8.0f);
  c.wl += (c.wlC - c.wl) * mnf(1.0f, dt * ((c.etat == E_VOL || c.etat == E_DECOLLE) ? 18.0f : 10.0f));
  c.fan += (c.fanC - c.fan) * mnf(1.0f, dt * 8.0f);
  c.grat += (c.gratC - c.grat) * mnf(1.0f, dt * 12.0f);
  { float kp = mnf(1.0f, dt * (c.etat == E_VOL ? 30.0f : 14.0f));
    c.pose.ea += (c.poseC.ea - c.pose.ea) * kp; c.pose.eh += (c.poseC.eh - c.pose.eh) * kp; c.pose.sa += (c.poseC.sa - c.pose.sa) * kp; c.pose.sh += (c.poseC.sh - c.pose.sh) * kp; }
  // position : les pattes restent posées sur le support, le corps s'en déduit
  { float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy);
    switch (c.etat) {
      case E_PERCHE: { float k = expf(-dt * 9.0f); c.ox *= k; c.oy *= k; c.x = c.fx - hx + c.ox; c.y = c.fy - JAMBE * c.s - hy + c.oy; } break;
      case E_DECOLLE: c.x = c.fx - hx; c.y = c.fy - JAMBE * c.s - hy; break;
      case E_ATTERRIT: { float e = pas(c.t / 0.34f); c.x = c.fx - hx + c.ox * (1 - e); c.y = c.fy - JAMBE * c.s - hy + c.oy * (1 - e); } break;
      case E_SOL: c.fx = c.x + hx; c.fy = solHaut(c.fx); c.fp = 0; c.y = c.fy - JAMBE * c.s - hy; break;
      default: break;
    } }
}

static void declencherPoof(Corbeau& c) {
  float X, Y; c.flash = 1.0f; posOeil(c, X, Y); eclat(X, Y, 0.22f, 1.0f);
  plumes(c.x, c.y, 14); son(SN_POP_DISP, 220); son(SN_PLUMES, 120);
  c.etat = E_POOF; c.t = 0; c.tfin = rnd(0.6f, 1.5f); c.perch = -1; c.wl = c.wlC = 0;
}


// ---------- le visiteur : apparition, interactions, disparition ----------
static bool ptOccupe(int p) { for (int k = 0; k < NCB; k++) if (cr[k].perch == 10 + p) return true; return false; }
static void mettreEnOrbite(Corbeau& c) {      // le corbeau rejoint la nuée qui tourne autour du visiteur
  float cy = visPiedsY() - 42.0f;
  c.item = -1; c.rapporte = false; c.fuite = false; c.sub = 0; c.objet = 0;
  switch (c.etat) {
    case E_DEHORS: entrer(c, VX, cy, B_ORBITE); break;
    case E_PERCHE: case E_SOL: case E_ATTERRIT: c.perch = -1; decoller(c, VX, cy, B_ORBITE); break;
    case E_POOF: return;      // il revient d'abord ; il sera recruté ensuite
    default: c.perch = -1; c.but = B_ORBITE; c.aWp = false; break;      // déjà en vol
  }
  c.ang = atan2f((c.y - cy) / 0.5f, c.x - VX); c.om = (random(2) ? 1.0f : -1.0f) * rnd(1.6f, 2.3f); c.ray = rnd(62, 92);
}
static void envoyerSur(int p) {      // un corbeau de la nuée vient se poser sur le visiteur (épaule, tête ou main)
  int best = -1; float bd = 1e9f, px, py; ptVis(p, px, py);
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; if (c.etat != E_VOL || c.but != B_ORBITE) continue;
    float d = fabsf(c.x - px) + fabsf(c.y - py); if (d < bd) { bd = d; best = k; }
  }
  if (best < 0) return;
  Corbeau& c = cr[best]; c.but = B_VISITEUR; c.perch = 10 + p; c.tx = px; c.ty = py; c.aWp = false;
}
static void liberer(int p) {      // le corbeau posé sur ce point repart en gerbe
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; if (c.perch != 10 + p) continue;
    c.gerbe = true; c.perch = -1; mettreEnOrbite(c);
  }
}
static void gerbe() {      // un toucher sur le visiteur : tous les corbeaux s'envolent en gerbe
  if (V.ph != V_PRESENT) return;
  V.flinch = 1.0f; float y0 = visPiedsY();
  for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if (c.perch >= 10) { c.flash = 1.0f; c.gerbe = true; c.perch = -1; mettreEnOrbite(c); } }
  plumes(VX, y0 - 45, 14); sonCri(); son(SN_PLUMES, 170);
}
static void bras() {      // toucher long : il tend le bras, un corbeau vient s'y poser
  if (V.ph != V_PRESENT) return;
  V.armC = 1.0f; V.tArm = 0; V.flinch = 0.4f; son(SN_GAZ, 150);
}
static void recruter() {
  for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if ((c.etat == E_PERCHE && c.perch < 10 && c.objet == 0) || c.etat == E_DEHORS) mettreEnOrbite(c); }
  for (int p = 0; p < 2; p++) if (!ptOccupe(p)) envoyerSur(p);
  if (V.t > 3.0f && !ptOccupe(2) && rnd01() < 0.5f) envoyerSur(2);
  if (V.arm > 0.85f && !ptOccupe(3)) envoyerSur(3);
}
static void minisEnvol(float x, float y, int n) {
  int k = 0;
  for (int i = 0; i < NMINI && k < n; i++) {
    Mini& m = mini[i]; if (m.actif && m.mode == 1) continue;
    float a = rnd(0, 6.283f), v = rnd(70, 150);
    m.actif = true; m.mode = 2; m.x = x + rnd(-6, 6); m.y = y + rnd(-10, 10); m.vx = cosf(a) * v; m.vy = sinf(a) * v * 0.8f - 25; m.a = atan2f(m.vy, m.vx); m.ph = rnd(0, 6.28f); m.vie = 1.0f; k++;
  }
}
static void invoquer() {      // la nuée tourbillonne, puis éclate : le visiteur apparaît
  if (V.ph != V_RIEN) return;
  memset(&V, 0, sizeof(V)); V.ph = V_NUEE; gAction = 4.0f;
  for (int k = 0; k < NCB; k++) mettreEnOrbite(cr[k]);
  for (int i = 0; i < NOB; i++) if (ob[i].actif) ob[i].reserve = -1;
  float cy = visPiedsY() - 42.0f;
  for (int i = 0; i < NMINI; i++) {
    Mini& m = mini[i]; m.actif = true; m.mode = 1; m.vy = rnd(0, 6.283f); m.r = rnd(55, 95); m.w = ((i & 1) ? 1.0f : -1.0f) * rnd(2.2f, 3.4f); m.vx = (m.r - 8.0f) / 2.6f; m.ph = rnd(0, 6.28f); m.vie = 1.0f;
    m.x = VX + m.r * cosf(m.vy); m.y = cy + 0.55f * m.r * sinf(m.vy); m.a = m.vy + ((m.w > 0) ? 1.5708f : -1.5708f);
  }
  sonCri(); son(SN_AILES, 220); son(SN_PLUMES, 160);
}
static void apparition() {
  V.ph = V_PRESENT; V.t = 0; V.dur = rnd(11.0f, 15.0f); V.arm = V.armC = 0; V.tArm = 0; V.tRecrut = 0; V.flinch = 0.8f;
  float cy = visPiedsY() - 35.0f;
  eclat(VX, cy, 0.35f, 3.2f); plumes(VX, cy, 40); son(SN_POP_APP, 230); son(SN_PLUMES, 160); sonCri();
  for (int i = 0; i < NMINI; i++) if (mini[i].actif && mini[i].mode == 1) { Mini& m = mini[i]; float a = atan2f(m.y - cy, m.x - VX), v = rnd(60, 140); m.mode = 2; m.vx = cosf(a) * v; m.vy = sinf(a) * v - 20; m.a = atan2f(m.vy, m.vx); m.vie = 1.0f; }
}
static void finVisite() {      // il se disperse en corbeaux et en plumes, en un clin d'œil
  if (V.ph == V_RIEN) return;
  float cy = visPiedsY() - 35.0f;
  if (V.ph == V_PRESENT) { eclat(VX, cy, 0.3f, 2.6f); plumes(VX, cy, 36); minisEnvol(VX, cy, 14); son(SN_POP_DISP, 230); son(SN_PLUMES, 160); sonCri(); }
  for (int i = 0; i < NMINI; i++) if (mini[i].actif && mini[i].mode == 1) mini[i].actif = false;
  V.ph = V_RIEN; V.arm = V.armC = 0; gVisite = rnd(110.0f, 180.0f);
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    if (c.perch >= 10 && (c.etat == E_PERCHE || c.etat == E_ATTERRIT)) { c.perch = -1; versPerchoir(c); }
    else if (c.but == B_ORBITE || c.but == B_VISITEUR) {
      int i = perchLibre();
      if (i >= 0 && random(100) < 65) { c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); c.aWp = false; }
      else { c.perch = -1; c.but = B_SORTIR; c.tx = (c.x < 160) ? -50.0f : 370.0f; c.ty = rnd(8, 90); c.aWp = false; c.fuite = true; }
    }
  }
}
static void majVisiteur(float dt) {
  if (V.ph == V_RIEN) return;
  V.t += dt; float t = tAnim;
  V.sw = 1.1f * sinf(t * 0.9f) + 0.4f * sinf(t * 2.3f) + V.flinch * 1.4f * sinf(t * 28.0f);
  if (V.flinch > 0) { V.flinch -= dt * 2.2f; if (V.flinch < 0) V.flinch = 0; }
  if (V.ph == V_NUEE) { if (V.t >= 2.6f) apparition(); return; }
  V.arm += (V.armC - V.arm) * mnf(1.0f, dt * 4.5f);
  if (V.armC > 0.5f) { V.tArm += dt; if (V.tArm > 4.5f) { V.armC = 0; liberer(3); } }
  V.tRecrut -= dt; if (V.tRecrut <= 0) { V.tRecrut = 0.8f; recruter(); }
  if (V.t >= V.dur) finVisite();
}
static void majMinis(float dt) {
  float cy = visPiedsY() - 42.0f;
  for (int i = 0; i < NMINI; i++) {
    Mini& m = mini[i]; if (!m.actif) continue;
    if (m.mode == 1) {
      m.a += m.w * dt; m.r -= m.vx * dt; if (m.r < 8) m.r = 8;
      m.x = VX + m.r * cosf(m.vy); m.y = cy + 0.55f * m.r * sinf(m.vy); m.a = m.vy + ((m.w > 0) ? 1.5708f : -1.5708f);
      m.a += 0.0f;      // le cap est calculé au dessin : tangente au cercle
    } else {
      m.x += m.vx * dt; m.y += m.vy * dt; m.vx *= (1.0f - 0.8f * dt); m.vy = m.vy * (1.0f - 0.8f * dt) - 10.0f * dt; m.vie -= dt / 0.9f;
      if (m.vie <= 0) m.actif = false;
    }
  }
}

static void secousse() {
  gAction = 2.5f;
  int reste = random(NCB);
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    if (c.etat == E_PERCHE && c.perch < 10) {
      c.flash = 1.0f; c.surs = 0.30f; c.bec = 1; plumes(c.x, c.y, 5);
      if (k != reste) { sortir(c); c.fuite = true; }
    } else if (c.etat == E_VOL || c.etat == E_SOL || c.etat == E_ATTERRIT) { c.flash = 0.8f; c.bec = 1; }
  }
  if (V.ph == V_PRESENT) gerbe();
  sonCri(); son(SN_PLUMES, 150);
}
static void reagirToucher(Corbeau& c, float tx, float ty) {
  gAction = 1.5f; c.flash = 1.0f; c.bec = 1; c.clin = 0;
  if (c.etat == E_PERCHE || c.etat == E_SOL) {
    c.d = (tx >= c.x) ? 1 : -1; c.look = lim(atan2f(ty - (c.y - 6.0f * c.s), fabsf(tx - c.x) + 6.0f), -0.9f, 0.8f); c.tLook = 1.3f;      // il tourne la tête vers le doigt
  }
  sonCri();
  if (c.etat == E_PERCHE) {
    c.surs = 0.30f;
    int r = random(100);
    if (r < 35) { }                                                        // un sursaut et un cri
    else if (r < 62) declencherPoof(c);                                    // clin d'oeil : il disparait
    else if (r < 85) versPerchoir(c);                                      // il change de branche
    else sortir(c);                                                        // il s'envole
  } else if (c.etat == E_SOL) c.surs = 0.30f;
}

static void majGlobal(float dt) {
  gAction -= dt; gPoof -= dt; gObjet -= dt; gAttrib -= dt; gCalme -= dt;
  majVisiteur(dt); majMinis(dt);
  gVisite -= dt; if (gVisite <= 0 && V.ph == V_RIEN) { if (nbDehors() < 3) invoquer(); else gVisite = 10.0f; }
  if (gPoof <= 0) {
    gPoof = rnd(20, 42);
    int k = random(NCB); for (int j = 0; j < NCB; j++) { Corbeau& c = cr[(k + j) % NCB]; if (c.etat == E_PERCHE && c.objet == 0 && c.perch < 10) { declencherPoof(c); break; } }
  }
  if (gObjet <= 0) {
    gObjet = rnd(32, 60);
    int k = random(NCB);
    for (int j = 0; j < NCB; j++) {
      Corbeau& c = cr[(k + j) % NCB];
      if (c.rapporte || c.but == B_NOUR) continue;
      if (c.etat == E_DEHORS) { c.rapporte = true; c.tfin = c.t + rnd(1, 2.5f); break; }
      if (c.etat == E_PERCHE && c.perch < 10 && nbDehors() < 2 && V.ph == V_RIEN) { c.rapporte = true; sortir(c); break; }
    }
  }
  if (gAttrib <= 0) {            // nourriture sans corbeau : on essaie d'en envoyer un
    gAttrib = 0.6f;
    for (int i = 0; i < NOB; i++) if (ob[i].actif && ob[i].reserve == -1) attribuer(i);
  }
  for (int i = 0; i < NOB; i++) {
    Obj& o = ob[i]; if (!o.actif) continue;
    if (!o.repos) {
      o.vy += 330.0f * dt; o.y += o.vy * dt; o.t = 0;
      float sol = solHaut(o.x) - 3.2f;
      if (o.y >= sol) { o.y = sol; if (o.vy > 60) { o.vy = -o.vy * 0.3f; } else { o.vy = 0; o.repos = true; o.t = 0; } }
    } else { o.t += dt; if (o.t > (o.reserve >= 0 ? 40.0f : 14.0f)) { o.actif = false; for (int k = 0; k < NCB; k++) if (cr[k].item == i) { cr[k].item = -1; } } }
  }
  for (int i = 0; i < NPART; i++) {
    Plume& p = pl[i]; if (!p.actif) continue;
    p.vie -= dt; if (p.vie <= 0) { p.actif = false; continue; }
    p.x += p.vx * dt; p.y += p.vy * dt; p.vx *= (1.0f - 1.6f * dt); p.vy = p.vy * (1.0f - 1.6f * dt) + 34.0f * dt; p.rot += p.vr * dt;
  }
  for (int i = 0; i < NFX; i++) { Eclat& e = fx[i]; if (!e.actif) continue; e.t += dt; if (e.t >= e.dur) e.actif = false; }
}

// ---------- dessin de la scène ----------
static void dessinerEntites(M5Canvas& g, bool sech) {
  sec = sech;
  // objets
  for (int i = 0; i < NOB; i++) {
    bbReset(); Obj& o = ob[i];
    if (o.actif) {
      float tw = (o.type >= T_BRILLANT && o.repos) ? ((sinf(millis() * 0.0075f + i) > 0.55f) ? 1.0f : 0.0f) : 0.0f;
      dessinerObjet(g, o.type, o.x, o.y, o.type >= T_BAGUE ? 5.5f : 4.6f, o.repos ? 0.0f : o.y * 0.12f, tw);
    }
    nouv[NCB + i] = bbRect();
  }
  bbReset(); if (V.ph == V_PRESENT) dessinerVisiteur(g); nouv[NCB + NOB + 3] = bbRect();      // le visiteur, avant les corbeaux : ils se posent devant lui
  for (int k = 0; k < NCB; k++) { bbReset(); if (cr[k].visible) dessinerCorbeau(g, cr[k]); nouv[k] = bbRect(); }
  bbReset(); for (int i = 0; i < NMINI; i++) if (mini[i].actif) dessinerMini(g, mini[i]); nouv[NCB + NOB + 4] = bbRect();
  // plumes
  bbReset();
  for (int i = 0; i < NPART; i++) {
    Plume& p = pl[i]; if (!p.actif) continue;
    float cs = cosf(p.rot), sn = sinf(p.rot), L = 3.6f * (0.5f + p.vie / p.dur * 0.5f), w = 1.3f;
    tri(g, p.x - cs * L, p.y - sn * L, p.x + cs * L, p.y + sn * L, p.x - sn * w, p.y + cs * w, C(5, 3, 6));
    tri(g, p.x - cs * L, p.y - sn * L, p.x + cs * L, p.y + sn * L, p.x + sn * w, p.y - cs * w, C(5, 3, 6));
  }
  nouv[NCB + NOB] = bbRect();
  // éclats rouges de l'œil
  bbReset();
  for (int i = 0; i < NFX; i++) {
    Eclat& f = fx[i]; if (!f.actif) continue;
    float u = f.t / f.dur, a = 1.0f - u, r = (4.0f + 14.0f * u) * f.taille;
    disque(g, f.x, f.y, r, C((uint8_t)(150 * a + 20), (uint8_t)(10 * a), (uint8_t)(12 * a)));
    disque(g, f.x, f.y, r * 0.6f, C(255, (uint8_t)(60 * a + 20), (uint8_t)(40 * a)));
    disque(g, f.x, f.y, r * 0.25f, C(255, 230, 210));
  }
  nouv[NCB + NOB + 1] = bbRect();
  // texte de mesure (mode debug)
  bbReset();
  if (debug) {
    bbPt(238, 2); bbPt(318, 30);
    if (!sech) {
      g.fillRoundRect(238, 2, 80, 28, 4, C(0, 0, 0)); g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(120, 255, 150)); g.setTextDatum(top_left);
      char b[40]; snprintf(b, sizeof(b), "%d i/s  %d ms", fps, msDessin); g.drawString(b, 242, 4);
      snprintf(b, sizeof(b), "T%dk P%dk", memTas, memPsram); g.drawString(b, 242, 16);
    }
  }
  nouv[NCB + NOB + 2] = bbRect();
  sec = false;
}

static void ajoutRect(Rect r) {
  if (r.x0 < 0) r.x0 = 0; if (r.y0 < 0) r.y0 = 0; if (r.x1 > 320) r.x1 = 320; if (r.y1 > H_SCENE) r.y1 = H_SCENE;
  if (r.x1 <= r.x0 || r.y1 <= r.y0) return;
  if (nListe < 40) liste[nListe++] = r;
}
static void fusionner() {
  bool chg = true;
  while (chg) {
    chg = false;
    for (int i = 0; i < nListe && !chg; i++) for (int j = i + 1; j < nListe && !chg; j++) {
      Rect& a = liste[i]; Rect& b = liste[j];
      if (a.x0 < b.x1 + 2 && b.x0 < a.x1 + 2 && a.y0 < b.y1 + 2 && b.y0 < a.y1 + 2) {
        if (b.x0 < a.x0) a.x0 = b.x0; if (b.y0 < a.y0) a.y0 = b.y0; if (b.x1 > a.x1) a.x1 = b.x1; if (b.y1 > a.y1) a.y1 = b.y1;
        liste[j] = liste[--nListe]; chg = true;
      }
    }
  }
}
static void restaurer(const Rect& r) {      // recopie du fond précalculé dans la zone
  uint8_t* d = (uint8_t*)cv.getBuffer(); const uint8_t* s = (const uint8_t*)fond.getBuffer(); if (!d || !s) return;
  for (int y = r.y0; y < r.y1; y++) memcpy(d + ((size_t)y * 320 + r.x0) * bpp, s + ((size_t)y * 320 + r.x0) * bpp, (size_t)(r.x1 - r.x0) * bpp);
}
static void pousser(const Rect& r) {
  M5.Display.setClipRect(r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0); cv.pushSprite(0, 0); M5.Display.clearClipRect();
}

// ---------- ouverture / fermeture ----------
static void ouvrir() {
  ouvert = true; fondOk = false; plein = true; chargerSons(); chargerOeil();
  bpp = cv.getColorDepth() / 8; if (bpp < 1) bpp = 1;
  if (ESP.getFreePsram() < 300000UL && ESP.getFreeHeap() < 260000UL) return;
  fond.setColorDepth(cv.getColorDepth()); fond.setPsram(true);
  if (!fond.createSprite(320, 240)) return;
  fondOk = true;
  construireFond();
  memset(cr, 0, sizeof(cr)); memset(ob, 0, sizeof(ob)); memset(pl, 0, sizeof(pl)); memset(fx, 0, sizeof(fx)); memset(anc, 0, sizeof(anc)); memset(dernierSon, 0, sizeof(dernierSon));
  static const float tailles[NCB] = {1.02f, 0.90f, 0.85f, 0.96f, 0.88f};
  for (int k = 0; k < NCB; k++) initCorbeau(cr[k], tailles[k]);
  cr[0].d = -1; cr[1].d = 1; cr[2].d = 1; placer(cr[0], 3); placer(cr[1], 1); placer(cr[2], 4);
  for (int k = 0; k < 3; k++) cr[k].tfin = rnd(3, 8);
  gCalme = 0; memset(&V, 0, sizeof(V)); memset(mini, 0, sizeof(mini)); gVisite = rnd(45.0f, 80.0f); tAnim = millis() * 0.001f;
  cr[3].etat = E_DEHORS; cr[3].visible = false; cr[3].t = 0; cr[3].tfin = 1.2f;
  cr[4].etat = E_DEHORS; cr[4].visible = false; cr[4].t = 0; cr[4].tfin = 6.0f;
  gAction = 3.0f; gPoof = rnd(14, 24); gObjet = rnd(24, 40); gAttrib = 0; choix = 0; presse = -1; tDernier = millis(); tSec = millis(); nImg = 0; fps = 0; msDessin = 0; volVu = false;
  if (R.actif[M_SON] && !lectureEnCours && !M5.Speaker.isEnabled()) { M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume); }
  Serial.printf("[corbeau] ouvert : heap=%u psram=%u\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
}
static void quitter() {
  if (!ouvert) return;
  ouvert = false;
  if (fondOk) { fond.deleteSprite(); fondOk = false; }
  if (!lectureEnCours && M5.Speaker.isEnabled()) { M5.Speaker.stop(); M5.Speaker.end(); M5.Mic.begin(); }     // la musique en cours (Matinale) n'est jamais touchée
  Serial.printf("[corbeau] fermé : heap=%u psram=%u\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
}

// ---------- une image ----------
static void dessiner() {
  if (!ouvert) ouvrir();
  if (!fondOk) {
    cv.fillScreen(C(10, 0, 4)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 150, 140));
    cv.drawString("Pas assez de mémoire", 160, 110); cv.drawString("pour les corbeaux", 160, 134); cv.setTextDatum(top_left); cv.pushSprite(0, 0); return;
  }
  uint32_t m0 = millis();
  float dt = (m0 - tDernier) / 1000.0f; bool longue = dt > 0.4f; tDernier = m0; if (dt > 0.05f) dt = 0.05f; if (dt < 0.001f) dt = 0.001f;
  if (longue) plein = true;
  // volume affiché par le programme sur l'écran : on redessine tout quand il disparaît
  if (millis() < finAffVolume) volVu = true; else if (volVu) { volVu = false; plein = true; }
  if (dernierSecouement != secVu) { if (secVu != 0 || dernierSecouement != 0) { if (millis() - dernierSecouement < 600) secousse(); } secVu = dernierSecouement; }
  tAnim = millis() * 0.001f;
  for (int k = 0; k < NCB; k++) majCorbeau(cr[k], k, dt);
  majGlobal(dt);
  if (presse >= 0 && millis() - tPressee > 180) { presse = -1; barreCb(fond, choix, -1); barreSale = true; }
  nListe = 0;
  dessinerEntites(cv, true);        // 1) boîtes englobantes (rien n'est dessiné)
  if (plein) {
    memcpy(cv.getBuffer(), fond.getBuffer(), (size_t)320 * 240 * bpp);
  } else {
    for (int e = 0; e < NENT; e++) { if (anc[e].x1 > anc[e].x0) ajoutRect(anc[e]); if (nouv[e].x1 > nouv[e].x0) ajoutRect(nouv[e]); }
    fusionner();
    for (int i = 0; i < nListe; i++) restaurer(liste[i]);
  }
  dessinerEntites(cv, false);       // 2) dessin réel
  for (int e = 0; e < NENT; e++) anc[e] = nouv[e];
  Rect barre; barre.x0 = 0; barre.y0 = H_SCENE; barre.x1 = 320; barre.y1 = 240;
  if (plein || barreSale) restaurer(barre);       // ce qui a pu déborder sur la barre est effacé
  M5.Display.startWrite();
  if (plein) { cv.pushSprite(0, 0); plein = false; }
  else { for (int i = 0; i < nListe; i++) pousser(liste[i]); if (barreSale) pousser(barre); }
  barreSale = false;
  M5.Display.endWrite();
  nImg++; msDessin = (int)(millis() - m0);
  if (millis() - tSec >= 1000) { fps = nImg; nImg = 0; tSec = millis(); memTas = (int)(ESP.getFreeHeap() / 1024); memPsram = (int)(ESP.getFreePsram() / 1024); if (debug) Serial.printf("[corbeau] %d i/s dessin=%d ms heap=%u psram=%u\n", fps, msDessin, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram()); }
}

// ---------- toucher ----------
static uint8_t tpCible = 0; static bool tpFait = false; static uint32_t tpT0 = 0; static float tpX = 0;
static void toucher(const m5::touch_detail_t& t) {
  if (!ouvert || !fondOk) return;
  if (tpCible && t.isPressed() && !tpFait && millis() - tpT0 > 600) { tpFait = true; if (tpCible == 1) bras(); else invoquer(); }
  if (t.wasReleased()) { if (tpCible && !tpFait) { if (tpCible == 1) gerbe(); else lacher(choix, tpX); } tpCible = 0; }
  if (!t.wasPressed()) return;
  tpCible = 0; tpFait = false;
  int x = t.x, y = t.y;
  if (y >= H_SCENE) {
    int i = (x - 4) / 52; if (i < 0) i = 0; if (i > 5) i = 5;
    choix = i; presse = i; tPressee = millis(); barreCb(fond, choix, presse); barreSale = true;
    lacher(i, rnd(112, 300));
    return;
  }
  if (x < 50 && y < 24) { page = 6; bip(700, 40); return; }
  if (x >= 52 && x < 172 && y < 24) {
    if (deuxJeux()) { sonsReels = !sonsReels; prefs.putUChar("cbsons", sonsReels ? 1 : 0); boutonSons(fond); plein = true; sonCri(); }
    return;
  }
  if (x >= 270 && y < 20) { debug = !debug; plein = true; return; }
  if (x >= 176 && x < 236 && y < 24) { if (V.ph == V_RIEN) invoquer(); else finVisite(); return; }
  if (V.ph == V_PRESENT && x >= VX - 26 && x < VX + (V.arm > 0.3f ? 40 : 26) && y >= visPiedsY() - 76 && y < visPiedsY() + 2) { tpCible = 1; tpT0 = millis(); return; }
  {
    int best = -1; float bd = 1e9f;
    for (int k = 0; k < NCB; k++) {
      Corbeau& c = cr[k]; if (!c.visible || c.etat == E_DEHORS || c.etat == E_POOF) continue;
      float rx = (c.wl > 0.3f ? 30.0f : 20.0f) * c.s, ry = (c.wl > 0.3f ? 26.0f : 16.0f) * c.s;
      float dx = fabsf(x - c.x) / rx, dy = fabsf(y - (c.y + c.bob)) / ry;
      if (dx < 1 && dy < 1 && dx + dy < bd) { bd = dx + dy; best = k; }
    }
    if (best >= 0) { reagirToucher(cr[best], (float)x, (float)y); return; }
  }
  if (V.ph == V_RIEN && x >= 70 && x < 244 && fabsf(y - ytop((float)x)) < 14) { tpCible = 2; tpT0 = millis(); tpX = (float)x; return; }
  lacher(choix, (float)x);
}

}  // namespace cb

static inline void corbeauDessiner() { cb::dessiner(); }
static inline void corbeauToucher(const m5::touch_detail_t& t) { cb::toucher(t); }
static inline void corbeauQuitter() { cb::quitter(); }
static inline void corbeauSonVolume() { cb::son(cb::SN_CROC, 255); }
