#pragma once
// ======================= MODE CORBEAUX (page 32) =======================
// Tout est dessiné par le code (polygones, ellipses) : aucune image. Fond précalculé une seule fois,
// puis seules les zones qui bougent sont redessinées et envoyées à l'écran (zones sales).
// Les sons des corbeaux se mélangent au son déjà en cours (Matinale) sur des canaux libres du haut-parleur.
#if __has_include("sons_corbeau.h")
#include "sons_corbeau.h"
#define CB_AVEC_SONS 1
#endif

extern uint32_t finAffVolume;

namespace cb {

static const int NCB = 5;        // corbeaux
static const int NOB = 4;        // objets au sol / qui tombent
static const int NPE = 6;        // perchoirs
static const int NPART = 28;    // plumes
static const int NFX = 6;       // éclats lumineux
static const int NENT = NCB + NOB + 3;
static const int H_SCENE = 206; // la barre de nourriture occupe le bas
static const float SOL_Y = 201.0f;
static const float PERCH[NPE][2] = {{100, 141}, {140, 135.8f}, {180, 130.3f}, {222, 125.6f}, {88, 62.5f}, {24, 94.5f}};

enum { E_PERCHE = 0, E_DECOLLE, E_VOL, E_ATTERRIT, E_SOL, E_DEHORS, E_POOF };
enum { B_RIEN = 0, B_PERCHOIR, B_NOUR, B_SORTIR, B_OBJET };
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
static bool volVu = false, barreSale = false;

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
static void ciel(M5Canvas& g) {
  struct S { float y; uint8_t r, gg, b; };
  static const S st[5] = {{0, 10, 1, 5}, {72, 72, 6, 12}, {125, 140, 14, 20}, {192, 104, 11, 16}, {240, 30, 3, 8}};
  for (int y = 0; y < 240; y++) {
    int k = 0; while (k < 3 && y >= st[k + 1].y) k++;
    float u = (y - st[k].y) / (st[k + 1].y - st[k].y);
    g.drawFastHLine(0, y, 320, C((uint8_t)lerpf(st[k].r, st[k + 1].r, u), (uint8_t)lerpf(st[k].gg, st[k + 1].gg, u), (uint8_t)lerpf(st[k].b, st[k + 1].b, u)));
  }
  // grande lune rouge, avec un halo par couronnes
  const float mx = 225, my = 82, mr = 56;
  for (int ring = 4; ring >= 1; ring--) {
    float rr = mr + ring * 9.0f, w = 0.13f * (5 - ring);
    for (int y = (int)(my - rr); y <= (int)(my + rr); y++) {
      if (y < 0 || y >= H_SCENE) continue;
      float dy = y - my, h = rr * rr - dy * dy; if (h <= 0) continue; float dx = sqrtf(h);
      int x0 = (int)(mx - dx), x1 = (int)(mx + dx); if (x0 < 0) x0 = 0; if (x1 > 319) x1 = 319; if (x1 < x0) continue;
      int k = 0; while (k < 3 && y >= st[k + 1].y) k++;
      float u = (y - st[k].y) / (st[k + 1].y - st[k].y);
      float r0 = lerpf(st[k].r, st[k + 1].r, u), g0 = lerpf(st[k].gg, st[k + 1].gg, u), b0 = lerpf(st[k].b, st[k + 1].b, u);
      g.drawFastHLine(x0, y, x1 - x0 + 1, C((uint8_t)lim(r0 + 70 * w, 0, 255), (uint8_t)lim(g0 + 10 * w, 0, 255), (uint8_t)lim(b0 + 8 * w, 0, 255)));
    }
  }
  g.fillCircle((int)mx, (int)my, (int)mr, C(214, 36, 28));
  g.fillCircle((int)mx + 12, (int)my + 14, 22, C(204, 33, 26));
  g.fillCircle((int)mx - 18, (int)my - 20, 15, C(207, 34, 27));
  g.fillCircle((int)mx + 20, (int)my - 24, 11, C(221, 40, 31));
  // collines lointaines puis sol noir
  for (int x = 0; x < 320; x++) {
    float yl = 190.0f + 4.0f * sinf(x / 50.0f + 2.0f) + 3.0f * sinf(x / 21.0f);
    g.drawFastVLine(x, (int)yl, 240 - (int)yl, C(46, 6, 11));
    float ys = 203.0f + 3.0f * sinf(x / 37.0f) + 2.0f * sinf(x / 13.0f + 1.0f);
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

static void construireFond() {
  ciel(fond); arbre(fond); menuCb(fond); barreCb(fond, choix, -1);
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

struct Corbeau {
  float x, y, vx, vy, s; int8_t d; uint8_t etat, but; int8_t perch, item, sub; bool visible, rapporte;
  float t, tfin, tx, ty, wx, wy; bool aWp;
  float phase, hz, planeT, vitesse; int8_t restBat;
  float tx_, ty_, tCx, tCy, tTete, clin, tClin, bec, tCri, flash, pitch, pitchC, fan, wl, bob, hop, peck;
  uint8_t objet;
  Aile pose;
  Rect anc; bool ancOk;
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

// ---------- sons (mélangés à la musique en cours : canaux 0 à 3, la Matinale utilise le 7) ----------
enum { SN_CROA1 = 0, SN_CROA2, SN_CROA3, SN_CROASSE1, SN_CROASSE2, SN_AILES, SN_PLUMES, SN_ATTERRIT, SN_POP_APP, SN_POP_DISP, SN_GAZ, SN_CROC, SN_NB };
static uint32_t dernierSon[SN_NB];
static void son(int id, int vol = 200) {
#ifdef CB_AVEC_SONS
  static const uint8_t* const D[SN_NB] = {SON_CROA1, SON_CROA2, SON_CROA3, SON_CROASSE1, SON_CROASSE2, SON_AILES, SON_PLUMES, SON_ATTERRIT, SON_POP_APPARAIT, SON_POP_DISPARAIT, SON_GAZOUILLIS, SON_CROC};
  static const uint32_t N[SN_NB] = {SON_CROA1_LEN, SON_CROA2_LEN, SON_CROA3_LEN, SON_CROASSE1_LEN, SON_CROASSE2_LEN, SON_AILES_LEN, SON_PLUMES_LEN, SON_ATTERRIT_LEN, SON_POP_APPARAIT_LEN, SON_POP_DISPARAIT_LEN, SON_GAZOUILLIS_LEN, SON_CROC_LEN};
  if (id < 0 || id >= SN_NB || !R.actif[M_SON] || R.volume == 0) return;
  uint32_t m = millis(); if (m - dernierSon[id] < 150) return; dernierSon[id] = m;
  if (!M5.Speaker.isEnabled()) {
    if (lectureEnCours) return;      // la Matinale démarre : on ne touche pas au haut-parleur
    M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume);
  }
  static uint8_t canal = 0; canal = (canal + 1) & 3;
  M5.Speaker.setChannelVolume(canal, vol);
  M5.Speaker.playRaw(D[id], N[id], SONS_CORBEAU_RATE, false, 1, canal, true);
#else
  (void)id; (void)vol;
#endif
}
static void sonCri() { static const int ids[5] = {SN_CROA1, SN_CROA2, SN_CROA3, SN_CROASSE1, SN_CROASSE2}; son(ids[random(5)], 190); }

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
static void posOeil(const Corbeau& c, float& X, float& Y) {
  float ct = cosf(c.pitch), st = sinf(c.pitch), u = 13.6f + c.tx_, v = -4.4f + c.ty_;
  X = c.x + c.d * c.s * (u * ct + v * st); Y = c.y + c.bob + c.s * (-u * st + v * ct);
}

// ---------- dessin d'un corbeau : silhouette noire, seuls les yeux brillent ----------
struct Rep { float cx, cy, d, s, ct, st; void p(float u, float v, float& X, float& Y) const { X = cx + d * s * (u * ct + v * st); Y = cy + s * (-u * st + v * ct); } };

static void dirAile(float e, float sw, float& du, float& dv) {
  float er = e * 0.0174533f; float a = -sw * cosf(er), b = -sinf(er); float l = sqrtf(a * a + b * b); if (l < 0.001f) l = 1; du = a / l; dv = b / l;
}
static void aile(M5Canvas& g, const Rep& R_, float su, float sv, const Aile& a, float wl, uint16_t col) {
  if (wl < 0.12f) return;
  float dau, dav, dhu, dhv; dirAile(a.ea, a.sa, dau, dav); dirAile(a.eh, a.sh, dhu, dhv);
  float la = 19.0f * wl, lh = 24.0f * wl;
  float tau = -dav, tav = dau; if (tau > 0.05f || (fabsf(tau) <= 0.05f && tav < 0)) { tau = -tau; tav = -tav; }
  float thu = -dhv, thv = dhu; if (thu > 0.05f || (fabsf(thu) <= 0.05f && thv < 0)) { thu = -thu; thv = -thv; }
  float Eu = su + dau * la, Ev = sv + dav * la, Tu = Eu + dhu * lh, Tv = Ev + dhv * lh;
  float L[28];
  int n = 0;
  auto add = [&](float u, float v) { R_.p(u, v, L[n * 2], L[n * 2 + 1]); n++; };
  add(su, sv);                                                    // épaule
  add(Eu - tau * 1.6f, Ev - tav * 1.6f);                          // coude, bord d'attaque
  add(Tu, Tv);                                                    // bout de l'aile
  add(Tu - dhu * 4.0f + thu * 4.5f, Tv - dhv * 4.0f + thv * 4.5f);
  add(Tu - dhu * 7.5f + thu * 2.6f, Tv - dhv * 7.5f + thv * 2.6f);     // creux entre deux plumes
  add(Tu - dhu * 9.0f + thu * 8.5f, Tv - dhv * 9.0f + thv * 8.5f);
  add(Tu - dhu * 12.5f + thu * 5.5f, Tv - dhv * 12.5f + thv * 5.5f);
  add(Tu - dhu * 15.0f + thu * 11.0f, Tv - dhv * 15.0f + thv * 11.0f);
  add(Eu + tau * 12.0f * wl, Ev + tav * 12.0f * wl);
  add(su + dau * la * 0.5f + tau * 11.0f * wl, sv + dav * la * 0.5f + tav * 11.0f * wl);
  add(su + tau * 6.5f * wl, sv + tav * 6.5f * wl);
  poly(g, L, n, col);
}

static void dessinerCorbeau(M5Canvas& g, const Corbeau& c) {
  const uint16_t NOIR = C(4, 3, 6);
  Rep R_; R_.cx = c.x; R_.cy = c.y + c.bob; R_.d = (float)c.d; R_.s = c.s; R_.ct = cosf(c.pitch); R_.st = sinf(c.pitch);
  float q[24];
  auto P = [&](float u, float v, float& X, float& Y) { R_.p(u, v, X, Y); };
  // pattes (en repère de l'écran : elles restent verticales)
  bool poseSol = (c.etat == E_PERCHE || c.etat == E_SOL || (c.etat == E_ATTERRIT && c.t > 0.18f) || (c.etat == E_DECOLLE));
  if (poseSol) {
    float fy = c.y + 10.5f * c.s, hy0 = c.y + c.bob + 3.5f * c.s;       // les pieds restent posés, le corps monte et descend
    float hx = c.x + c.d * c.s * 1.0f;
    for (int k = 0; k < 2; k++) {
      float lx = hx + (k ? 3.2f : -3.0f) * c.s * c.d;
      tri(g, lx - 1.0f, hy0, lx + 1.0f, hy0, lx, fy, NOIR);
      ligne(g, lx, fy, lx + c.d * 3.4f * c.s, fy + 0.4f, NOIR); ligne(g, lx, fy, lx - c.d * 1.8f * c.s, fy + 0.6f, NOIR);
    }
  } else if (c.etat == E_ATTERRIT) {
    float hx = c.x + c.d * c.s * 3.0f; float fy = c.y + 9.0f * c.s + c.bob;       // pattes en avant à l'atterrissage
    ligne(g, hx, c.y + c.bob + 3.5f * c.s, hx + c.d * 4.0f * c.s, fy, NOIR); ligne(g, hx - 3.0f * c.d, c.y + c.bob + 3.5f * c.s, hx + c.d * 1.0f * c.s, fy, NOIR);
  }
  // aile lointaine puis aile proche
  if (c.wl > 0.12f) {
    Aile a2 = c.pose; a2.ea += 14; a2.eh += 10;
    aile(g, R_, 3.2f, -3.8f, a2, c.wl * 0.92f, NOIR);
    aile(g, R_, 1.6f, -3.2f, c.pose, c.wl, NOIR);
  }
  // queue en coin (s'ouvre en éventail à l'atterrissage)
  { float f = c.fan; float t[10] = {-9, -2.2f, -24.5f, 1.4f - 6.0f * f, -26.0f, 1.6f, -24.5f, 1.8f + 6.0f * f, -9, 3.6f};
    for (int i = 0; i < 5; i++) P(t[i * 2], t[i * 2 + 1], q[i * 2], q[i * 2 + 1]); poly(g, q, 5, NOIR); }
  // corps
  for (int i = 0; i < 12; i++) { float a = i * 0.5236f; P(cosf(a) * 11.8f, sinf(a) * 5.9f, q[i * 2], q[i * 2 + 1]); }
  poly(g, q, 12, NOIR);
  // poitrine et dos, pour une silhouette plus de corbeau que d'œuf
  { float x1, y1, x2, y2, x3, y3; P(8, -5.5f, x1, y1); P(13, -0.5f, x2, y2); P(2, 5.6f, x3, y3); tri(g, x1, y1, x2, y2, x3, y3, NOIR); }
  // tête
  float hu = 12.2f + c.tx_, hv = -3.9f + c.ty_;
  { float X, Y; P(hu, hv, X, Y); disque(g, X, Y, 4.5f * c.s, NOIR); }
  { float x1, y1, x2, y2, x3, y3; P(hu - 4.5f, hv + 1.0f, x1, y1); P(hu + 0.5f, hv + 4.2f, x2, y2); P(hu - 5.5f, hv + 6.4f, x3, y3); tri(g, x1, y1, x2, y2, x3, y3, NOIR); }   // gorge hirsute
  { float x1, y1, x2, y2, x3, y3; P(hu - 3.0f, hv - 2.0f, x1, y1); P(hu - 3.0f, hv + 3.0f, x2, y2); P(2.0f, -1.0f, x3, y3); tri(g, x1, y1, x2, y2, x3, y3, NOIR); }       // cou
  // bec lourd, qui s'ouvre quand il croasse
  { float o = c.bec; float ax, ay, mx, my, tx, ty, bx_, by_, lx, ly;
    P(hu + 2.0f, hv - 1.9f, ax, ay); P(hu + 2.2f, hv + 2.9f, bx_, by_); P(hu + 2.4f, hv + 0.2f, mx, my);
    P(hu + 12.0f, hv + 0.9f - 3.2f * o, tx, ty); P(hu + 11.0f, hv + 1.6f + 3.4f * o, lx, ly);
    tri(g, ax, ay, mx, my, tx, ty, NOIR); tri(g, mx, my, bx_, by_, lx, ly, NOIR); }
  // objet porté dans le bec
  if (c.objet) {
    float X, Y; P(hu + 11.0f, hv + 2.0f, X, Y);
    float tw = (sinf(millis() * 0.0081f) > 0.6f) ? 1.0f : 0.0f;
    dessinerObjet(g, c.objet - 1, X + c.d * 2.0f, Y + 2.0f, 3.6f * c.s + 0.8f, c.d > 0 ? 0.4f : 2.7f, tw);
  }
  // œil rouge qui brille
  if (c.clin < 0.5f) {
    float X, Y; P(hu + 1.6f, hv - 0.7f, X, Y);
    disque(g, X, Y, 2.5f * c.s + 0.6f, C(120, 8, 12)); disque(g, X, Y, 1.5f * c.s + 0.3f, C(255, 58, 40)); disque(g, X, Y, 0.7f, C(255, 215, 190));
  } else { float X, Y; P(hu + 1.6f, hv - 0.7f, X, Y); ligne(g, X - 1.5f, Y, X + 1.5f, Y, C(70, 8, 12)); }
  if (c.flash > 0.01f) { float X, Y; posOeil(c, X, Y); disque(g, X, Y, 3.0f + 7.0f * c.flash, C(255, (uint8_t)(60 + 120 * c.flash), (uint8_t)(40 + 90 * c.flash))); disque(g, X, Y, 1.8f, C(255, 240, 225)); }
}

// ---------- logique ----------
static bool perchOccupe(int i) { for (int k = 0; k < NCB; k++) if (cr[k].perch == i) return true; return false; }
static int perchLibre() {
  int n = 0, l[NPE]; for (int i = 0; i < NPE; i++) if (!perchOccupe(i)) l[n++] = i;
  return n ? l[random(n)] : -1;
}
static int nbDehors() { int n = 0; for (int k = 0; k < NCB; k++) if (cr[k].etat == E_DEHORS || (cr[k].etat == E_VOL && cr[k].but == B_SORTIR)) n++; return n; }
static float pieds(const Corbeau& c) { return c.s * 10.5f; }

static void nouvTete(Corbeau& c) { c.tCx = rnd(-1.6f, 1.6f); c.tCy = rnd(-1.2f, 1.2f); c.tTete = rnd(0.6f, 2.8f); }
static void initCorbeau(Corbeau& c, float s) {
  memset(&c, 0, sizeof(c)); c.s = s; c.d = 1; c.visible = true; c.perch = -1; c.item = -1; c.wl = 0; c.pose = A_PLANE; c.pitch = 0.2f; c.pitchC = 0.2f;
  c.tClin = rnd(1.0f, 4.0f); c.tTete = rnd(0.5f, 2.0f);
}
static void placer(Corbeau& c, int i) {
  c.perch = i; c.x = PERCH[i][0]; c.y = PERCH[i][1] - pieds(c); c.vx = c.vy = 0; c.etat = E_PERCHE; c.but = B_RIEN; c.t = 0; c.tfin = rnd(4, 11);
  c.visible = true; c.wl = 0; c.fan = 0; c.hop = 0; c.bob = 0; c.sub = 0;
}
static void decoller(Corbeau& c, float tx, float ty, uint8_t but) {
  c.etat = E_DECOLLE; c.t = 0; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.sub = 0;
  c.d = (tx >= c.x) ? 1 : -1; c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = 0.9f;
}
static void entrer(Corbeau& c, float tx, float ty, uint8_t but) {      // arrive par un bord de l'écran
  bool gauche = (tx < 160) ? (rnd01() < 0.7f) : (rnd01() < 0.3f);
  c.x = gauche ? -34.0f : 354.0f; c.y = rnd(18, 110); c.d = gauche ? 1 : -1; c.vx = c.d * 90.0f; c.vy = 0;
  c.etat = E_VOL; c.t = 1.5f; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.visible = true; c.sub = 0;
  c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = rnd01(); c.wl = 1; c.fan = 0; c.pitchC = 0.1f;
}
static void sortir(Corbeau& c) {
  c.perch = -1; c.item = -1;
  float tx = (rnd01() < 0.5f) ? -50.0f : 370.0f;
  if (c.x < 100) tx = -50; else if (c.x > 220) tx = 370;
  decoller(c, tx, rnd(8, 90), B_SORTIR);
}
static void versPerchoir(Corbeau& c) {
  int i = perchLibre();
  if (i < 0) { if (nbDehors() < 3) sortir(c); else { c.etat = E_SOL; c.but = B_RIEN; c.sub = 2; c.t = 0; } return; }
  c.perch = i; c.item = -1; decoller(c, PERCH[i][0], PERCH[i][1] - pieds(c), B_PERCHOIR);
}
static void apresSol(Corbeau& c) {
  if (rnd01() < 0.3f && nbDehors() < 2) sortir(c); else versPerchoir(c);
}

static void lacher(int type, float x) {       // une nourriture tombe du ciel
  int idx = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { idx = i; break; }
  if (idx < 0) { float vieux = -1; for (int i = 0; i < NOB; i++) if (ob[i].t > vieux && ob[i].repos) { vieux = ob[i].t; idx = i; } if (idx < 0) return; }
  Obj& o = ob[idx]; o.actif = true; o.type = (uint8_t)type; o.x = lim(x, 12, 308); o.y = -6; o.vy = 20; o.t = 0; o.repos = false; o.reserve = -1;
  for (int k = 0; k < NCB; k++) if (cr[k].item == idx) { cr[k].item = -1; if (cr[k].but == B_NOUR) cr[k].but = B_RIEN; }
}

static void attribuer(int iO) {       // un corbeau libre vient chercher la nourriture
  Obj& o = ob[iO]; int best = -1; float bd = 1e9f;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    bool ok = (c.etat == E_PERCHE && c.objet == 0) || c.etat == E_DEHORS || (c.etat == E_SOL && c.but == B_RIEN);
    if (!ok || c.but == B_NOUR || c.rapporte) continue;
    float px = (c.etat == E_DEHORS) ? 160.0f + (k % 2 ? 200.0f : -200.0f) : c.x; float dd = fabsf(px - o.x) + rnd(0, 25);
    if (dd < bd) { bd = dd; best = k; }
  }
  if (best < 0) return;
  Corbeau& c = cr[best]; float x0 = (c.etat == E_DEHORS) ? ((best % 2) ? 354.0f : -34.0f) : c.x;
  int dsg = (o.x >= x0) ? 1 : -1;
  float gx = lim(o.x - dsg * 19.0f * c.s, 10, 310), gy = SOL_Y - pieds(c);
  c.perch = -1; c.item = iO; o.reserve = best; c.objet = 0;
  if (c.etat == E_PERCHE || c.etat == E_SOL) { c.sub = 0; decoller(c, gx, gy, B_NOUR); }
  else { entrer(c, gx, gy, B_NOUR); }
  c.item = iO;
}

static void atterrir(Corbeau& c) {
  c.x = c.tx; c.y = c.ty; c.vx = c.vy = 0; c.etat = E_ATTERRIT; c.t = 0; c.bob = 0; c.pitchC = 0.2f; son(SN_ATTERRIT, 170);
  if (c.but == B_PERCHOIR || c.but == B_NOUR || c.but == B_OBJET) c.pose = A_FLARE;
}

static void majCorbeau(Corbeau& c, int idx, float dt) {
  c.t += dt;
  // clignement et tête
  c.tClin -= dt; if (c.tClin <= 0) { c.clin = 1; c.tClin = rnd(1.8f, 5.0f); }
  if (c.clin > 0) { c.clin -= dt * 4.5f; if (c.clin < 0) c.clin = 0; }
  c.tTete -= dt; if (c.tTete <= 0) nouvTete(c);
  c.tx_ += (c.tCx - c.tx_) * mnf(1.0f, dt * 9.0f); c.ty_ += (c.tCy - c.ty_) * mnf(1.0f, dt * 9.0f);
  if (c.bec > 0) { c.bec -= dt * 3.0f; if (c.bec < 0) c.bec = 0; }
  if (c.flash > 0) { c.flash -= dt * 4.0f; if (c.flash < 0) c.flash = 0; }
  c.pitch += (c.pitchC - c.pitch) * mnf(1.0f, dt * 8.0f);
  c.bob = 0; c.hop = 0;

  switch (c.etat) {
    case E_PERCHE: {
      c.wl = 0; c.fan = 0; c.pitchC = 0.22f; c.vx = c.vy = 0;
      if (c.perch >= 0) { c.x = PERCH[c.perch][0]; c.y = PERCH[c.perch][1] - pieds(c); }
      if (c.objet && c.t > 9.0f) { c.objet = 0; eclat(c.x + c.d * 18 * c.s, c.y - 2, 0.3f, 0.7f); }
      if (c.t >= c.tfin && gAction <= 0) {
        int r = random(100);
        if (r < 40) { versPerchoir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 62 && nbDehors() < 2) { sortir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 80) { c.d = -c.d; c.t = 0; c.tfin = rnd(3, 9); }
        else { c.bec = 1; sonCri(); c.t = 0; c.tfin = rnd(4, 11); }
      }
    } break;
    case E_DECOLLE: {
      float u = c.t / 0.20f; c.pose = A_HAUT; c.wl = pas(c.t / 0.12f); c.pitchC = 0.5f; c.fan = 0;
      c.bob = 2.6f * c.s * pas(u > 1 ? 1 : u);                // il s'accroupit avant de sauter
      if (c.t < 0.02f) son(SN_AILES, 200);
      if (c.t >= 0.20f) {
        c.etat = E_VOL; c.t = 0; c.vx = c.d * 46.0f; c.vy = -105.0f; c.bob = 0; c.perch = (c.but == B_PERCHOIR) ? c.perch : -1;
        float dist = hypotf(c.tx - c.x, c.ty - c.y);
        if (c.but == B_SORTIR) { c.wx = c.x + c.d * 70.0f; c.wy = mnf(c.y - 50.0f, 60.0f) - rnd(0, 20); c.aWp = true; }
        else if (dist > 110) { c.wx = (c.x + c.tx) * 0.5f + rnd(-20, 20); c.wy = mnf(c.y, c.ty) - rnd(14, 50); c.aWp = true; }
      }
    } break;
    case E_VOL: {
      c.wl = 1; c.fan = lerpf(c.fan, 0.0f, mnf(1.0f, dt * 5.0f));
      float gx = c.aWp ? c.wx : c.tx, gy = c.aWp ? c.wy : c.ty;
      float dx = gx - c.x, dy = gy - c.y, dist = sqrtf(dx * dx + dy * dy); if (dist < 0.001f) dist = 0.001f;
      if (c.aWp && dist < 18) { c.aWp = false; break; }
      bool fin = !c.aWp, proche = fin && dist < 38.0f && c.but != B_SORTIR;
      float sp = fin ? lim(dist * 2.4f + 16.0f, 16.0f, c.vitesse) : c.vitesse;
      if (c.but == B_SORTIR) sp = c.vitesse * 1.05f;
      float k = mnf(1.0f, dt * 3.0f);
      c.vx += (dx / dist * sp - c.vx) * k; c.vy += (dy / dist * sp - c.vy) * k;
      c.x += c.vx * dt; c.y += c.vy * dt;
      if (fabsf(c.vx) > 12 && !proche) c.d = (c.vx > 0) ? 1 : -1; else if (proche) c.d = (dx >= 0) ? 1 : -1;
      c.pitchC = proche ? 0.62f : lim(-c.vy * 0.006f + 0.1f, -0.4f, 0.5f);
      // battements ou plané
      if (proche) {
        c.phase += 3.5f * dt; if (c.phase >= 1) c.phase -= 1;
        c.pose = mixA(A_FLARE, A_BAS, 0.30f * lim(coup(c.phase), 0, 1)); c.fan = lerpf(c.fan, 1.0f, mnf(1.0f, dt * 6.0f));
        c.bob = -1.0f * c.s * coup(c.phase);
      } else if (c.planeT > 0) {
        c.planeT -= dt; c.pose = A_PLANE; c.pose.ea += 2.0f * sinf(c.t * 9.0f); c.vy += 6.0f * dt;
      } else {
        float h = c.hz * (c.t < 1.0f ? 1.25f : 1.0f);
        float p0 = c.phase; c.phase += h * dt;
        if (c.phase >= 1) {
          c.phase -= 1; c.restBat--;
          if (c.restBat <= 0) { c.restBat = 3 + random(4); if (c.t > 1.2f && c.but != B_SORTIR && rnd01() < 0.55f && dist > 60) c.planeT = rnd(0.5f, 1.3f); }
        }
        if (p0 > 0.9f && c.phase < 0.1f) son(SN_AILES, 120);
        float cp = coup(c.phase); c.pose = mixA(A_HAUT, A_BAS, cp); c.bob = -2.4f * c.s * (cp - 0.4f);
      }
      if (c.but == B_SORTIR) { if (c.x < -32 || c.x > 352) { c.etat = E_DEHORS; c.t = 0; c.visible = false; c.tfin = c.rapporte ? rnd(3.5f, 6.0f) : rnd(6, 16); c.wl = 0; } }
      else if (fin && (dist < 2.5f || (dist < 7.0f && sqrtf(c.vx * c.vx + c.vy * c.vy) < 20.0f))) atterrir(c);
    } break;
    case E_ATTERRIT: {
      float u = c.t / 0.34f;
      c.wl = 1.0f - pas((c.t - 0.06f) / 0.28f); c.pose = mixA(A_FLARE, A_PLANE, 0.0f); c.fan = 1.0f - pas(u);
      c.bob = 1.5f * c.s * sinf(lim(u, 0, 1) * 3.1416f); c.pitchC = 0.3f;
      if (c.t >= 0.34f) {
        c.wl = 0; c.fan = 0; c.t = 0;
        if (c.but == B_PERCHOIR) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(5, 13); }
        else if (c.but == B_NOUR || c.but == B_OBJET) { c.etat = E_SOL; c.sub = 0; }
        else { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; }
      }
    } break;
    case E_SOL: {
      c.wl = 0; c.fan = 0; c.pitchC = 0.05f;
      if (c.but == B_NOUR && c.sub <= 1) {
        Obj* o = (c.item >= 0 && c.item < NOB && ob[c.item].actif && ob[c.item].repos && ob[c.item].reserve == idx) ? &ob[c.item] : nullptr;
        if (!o) { c.but = B_RIEN; c.item = -1; apresSol(c); break; }
        float gx = o->x - c.d * 19.0f * c.s;
        if (c.sub == 0) {
          float dxm = gx - c.x; c.d = (o->x >= c.x) ? 1 : -1;
          if (fabsf(dxm) < 1.8f) { c.sub = 1; c.peck = 0; }
          else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 34.0f * dt); c.hop = -fabsf(sinf(c.t * 11.0f)) * 2.6f * c.s; c.bob = c.hop; c.pitchC = 0.12f; }
        } else if (c.sub == 1) {
          c.peck += dt; float ph = c.peck / 0.42f; float dip = fabsf(sinf(ph * 3.1416f));
          c.pitchC = -0.55f * dip; c.ty_ = 5.5f * dip; c.tCy = c.ty_; c.tTete = 1.0f;
          if (c.peck > 0.24f && o->actif && o->reserve == idx) {      // il prend la nourriture
            son(SN_CROC, 200);
            if (o->type >= T_BRILLANT) { c.objet = o->type + 1; o->actif = false; c.item = -1; c.sub = 2; c.peck = 0; c.t = 0; c.flash = 0.5f; }
            else { o->actif = false; c.item = -1; c.sub = 3; c.peck = 0; }
          }
        }
      }
      if (c.but == B_NOUR && c.sub == 3) {       // il picore ce qu'il a pris
        c.peck += dt; float dip = fabsf(sinf(c.peck / 0.40f * 3.1416f)); c.pitchC = -0.5f * dip; c.ty_ = 5.0f * dip; c.tCy = c.ty_; c.tTete = 1.0f;
        if (c.peck > 0.9f && c.peck - dt <= 0.9f) { son(SN_GAZ, 150); }
        if (c.peck > 1.8f) { c.but = B_RIEN; apresSol(c); }
      } else if (c.but == B_NOUR && c.sub == 2) { c.peck += dt; if (c.peck > 0.7f) { c.but = B_RIEN; apresSol(c); } }
      else if (c.but == B_OBJET) {
        if (c.sub == 0) { c.sub = 1; c.peck = 0; c.d = (c.x < 160) ? 1 : -1; }
        else if (c.sub == 1) {
          c.peck += dt; c.bec = lim((c.peck - 0.35f) * 4.0f, 0, 1);
          if (c.peck > 0.55f && c.objet) {       // il dépose l'objet devant lui
            int idxO = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { idxO = i; break; }
            if (idxO < 0) idxO = 0;
            Obj& o = ob[idxO]; o.actif = true; o.type = c.objet - 1; o.x = c.x + c.d * 20.0f * c.s; o.y = SOL_Y - 8.0f; o.vy = -40; o.t = 0; o.repos = false; o.reserve = -2;
            c.objet = 0; son(SN_CROC, 170); c.sub = 2; c.peck = 0;
          }
        } else if (c.sub == 2) { c.peck += dt; if (c.peck > 0.9f) { c.but = B_RIEN; apresSol(c); } }
      }
      if (c.but == B_RIEN && c.sub == 2 && c.etat == E_SOL && c.t > 0.5f) { c.t = 0; apresSol(c); }
    } break;
    case E_DEHORS: {
      c.visible = false; c.wl = 0;
      if (c.t >= c.tfin) {
        if (c.rapporte) {           // il revient avec un objet qu'il dépose par terre
          c.rapporte = false; c.objet = 7 + random(4);
          float gx = rnd(110, 300); c.perch = -1; entrer(c, gx, SOL_Y - pieds(c), B_OBJET);
        } else {
          int i = perchLibre();
          if (i >= 0) { c.perch = i; entrer(c, PERCH[i][0], PERCH[i][1] - pieds(c), B_PERCHOIR); }
          else c.tfin = c.t + 1.0f;
        }
      }
    } break;
    case E_POOF: {
      if (c.t > 0.07f) c.visible = false;
      if (c.t >= c.tfin) {          // il réapparaît ailleurs, en un clin d'œil
        int i = perchLibre(); if (i < 0) i = 0;
        placer(c, i); c.d = (rnd01() < 0.5f) ? 1 : -1; c.flash = 1.0f;
        float X, Y; posOeil(c, X, Y); eclat(X, Y, 0.25f, 1.0f); plumes(c.x, c.y, 10); son(SN_POP_APP, 220); son(SN_PLUMES, 120);
      }
    } break;
  }
}

static void declencherPoof(Corbeau& c) {
  float X, Y; c.flash = 1.0f; posOeil(c, X, Y); eclat(X, Y, 0.22f, 1.0f);
  plumes(c.x, c.y, 14); son(SN_POP_DISP, 220); son(SN_PLUMES, 120);
  c.etat = E_POOF; c.t = 0; c.tfin = rnd(0.6f, 1.5f); c.perch = -1; c.wl = 0;
}

static void majGlobal(float dt) {
  gAction -= dt; gPoof -= dt; gObjet -= dt; gAttrib -= dt;
  if (gPoof <= 0) {
    gPoof = rnd(20, 42);
    int k = random(NCB); for (int j = 0; j < NCB; j++) { Corbeau& c = cr[(k + j) % NCB]; if (c.etat == E_PERCHE && c.objet == 0) { declencherPoof(c); break; } }
  }
  if (gObjet <= 0) {
    gObjet = rnd(32, 60);
    int k = random(NCB);
    for (int j = 0; j < NCB; j++) {
      Corbeau& c = cr[(k + j) % NCB];
      if (c.rapporte || c.but == B_NOUR) continue;
      if (c.etat == E_DEHORS) { c.rapporte = true; c.tfin = c.t + rnd(1, 2.5f); break; }
      if (c.etat == E_PERCHE && nbDehors() < 2) { c.rapporte = true; sortir(c); break; }
    }
  }
  if (gAttrib <= 0) {            // nourriture sans corbeau : on essaie d'en envoyer un
    gAttrib = 0.6f;
    for (int i = 0; i < NOB; i++) if (ob[i].actif && ob[i].repos && ob[i].reserve == -1) attribuer(i);
  }
  for (int i = 0; i < NOB; i++) {
    Obj& o = ob[i]; if (!o.actif) continue;
    if (!o.repos) {
      o.vy += 330.0f * dt; o.y += o.vy * dt; o.t = 0;
      float sol = SOL_Y - 3.0f;
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
  for (int k = 0; k < NCB; k++) { bbReset(); if (cr[k].visible) dessinerCorbeau(g, cr[k]); nouv[k] = bbRect(); }
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
    bbPt(228, 2); bbPt(318, 30);
    if (!sech) {
      g.fillRoundRect(228, 2, 90, 28, 4, C(0, 0, 0)); g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(120, 255, 150)); g.setTextDatum(top_left);
      char b[40]; snprintf(b, sizeof(b), "%d i/s  %d ms", fps, msDessin); g.drawString(b, 232, 4);
      snprintf(b, sizeof(b), "T%dk P%dk", (int)(ESP.getFreeHeap() / 1024), (int)(ESP.getFreePsram() / 1024)); g.drawString(b, 232, 16);
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
  ouvert = true; fondOk = false; plein = true;
  bpp = cv.getColorDepth() / 8; if (bpp < 1) bpp = 1;
  if (ESP.getFreePsram() < 300000UL && ESP.getFreeHeap() < 260000UL) return;
  fond.setColorDepth(cv.getColorDepth()); fond.setPsram(true);
  if (!fond.createSprite(320, 240)) return;
  fondOk = true;
  construireFond();
  memset(cr, 0, sizeof(cr)); memset(ob, 0, sizeof(ob)); memset(pl, 0, sizeof(pl)); memset(fx, 0, sizeof(fx)); memset(anc, 0, sizeof(anc)); memset(dernierSon, 0, sizeof(dernierSon));
  static const float tailles[NCB] = {1.12f, 0.96f, 0.88f, 1.02f, 0.92f};
  for (int k = 0; k < NCB; k++) initCorbeau(cr[k], tailles[k]);
  placer(cr[0], 3); placer(cr[1], 1); placer(cr[2], 4); cr[0].d = -1; cr[1].d = 1; cr[2].d = 1;
  for (int k = 0; k < 3; k++) { cr[k].tfin = rnd(3, 8); nouvTete(cr[k]); }
  cr[3].etat = E_DEHORS; cr[3].visible = false; cr[3].t = 0; cr[3].tfin = 1.2f;
  cr[4].etat = E_DEHORS; cr[4].visible = false; cr[4].t = 0; cr[4].tfin = 6.0f;
  gAction = 3.0f; gPoof = rnd(14, 24); gObjet = rnd(24, 40); gAttrib = 0; choix = 0; presse = -1; tDernier = millis(); tSec = millis(); nImg = 0; fps = 0; msDessin = 0; volVu = false;
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
  if (millis() - tSec >= 1000) { fps = nImg; nImg = 0; tSec = millis(); if (debug) Serial.printf("[corbeau] %d i/s dessin=%d ms heap=%u psram=%u\n", fps, msDessin, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram()); }
}

// ---------- toucher ----------
static void toucher(const m5::touch_detail_t& t) {
  if (!ouvert || !fondOk || !t.wasPressed()) return;
  int x = t.x, y = t.y;
  if (y >= H_SCENE) {
    int i = (x - 4) / 52; if (i < 0) i = 0; if (i > 5) i = 5;
    choix = i; presse = i; tPressee = millis(); barreCb(fond, choix, presse); barreSale = true;
    lacher(i, rnd(112, 300));
    return;
  }
  if (x < 50 && y < 24) { page = 6; bip(700, 40); return; }
  if (x >= 270 && y < 20) { debug = !debug; plein = true; return; }
  lacher(choix, (float)x);
}

}  // namespace cb

static inline void corbeauDessiner() { cb::dessiner(); }
static inline void corbeauToucher(const m5::touch_detail_t& t) { cb::toucher(t); }
static inline void corbeauQuitter() { cb::quitter(); }
