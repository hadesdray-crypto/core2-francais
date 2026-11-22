#pragma once
// ======================= DEX DES YEUX (page 33) =======================
// Un carnet d'yeux imaginaires : liste, fiche, effet sur le corbeau, création d'un œil.
// Tous les motifs sont dessinés par le code (triangles, cercles). Tout le contenu est inventé pour le jeu,
// sauf les « Le saviez-vous ? », qui sont de vrais faits sur les corbeaux.
// Rien n'est redessiné tant que rien ne bouge : le toucher reste immédiat.

extern uint32_t finAffVolume;
#if __has_include("sons_pouvoirs.h")
#include "sons_pouvoirs.h"
#define DX_SONS 1
#endif

namespace dx {

static const int NE = 12;
enum { M_VIRG = 0, M_PALES, M_LAMES, M_ANN, M_BLANC, M_PET, M_FLEUR };
struct OeilDef {
  const char* court; const char* nom; uint8_t motif; uint8_t r, g, b; uint8_t etoiles, danger;
  const char* l1; const char* l2; const char* p1; const char* p2; const char* p3; const char* anec; const char* effet;
};
static const OeilDef DEF[7] = {
  {"Virgules", "Œil à virgules", M_VIRG, 214, 24, 34, 3, 2, "Origine : lignée des yeux rouges", "Éveil : après une forte émotion", "voit les gestes très nets", "devine le prochain coup", "copie un geste vu",
   "Le grand corbeau est le plus lourd des passereaux.", "Ses yeux brillent d'un rouge franc, avec trois virgules."},
  {"Pales", "Œil à pales", M_PALES, 214, 24, 34, 4, 4, "Origine : forme avancée des yeux rouges", "Éveil : après un grand chagrin", "illusion du temps déformé", "flammes noires", "armure spectrale",
   "Les corbeaux simulent parfois une cachette pour tromper leurs voisins.", "Ses yeux deviennent d'un rouge profond, plus sombre."},
  {"Lames", "Œil à lames", M_LAMES, 236, 90, 30, 4, 3, "Origine : variante rare des yeux rouges", "Éveil : après un choc violent", "traverse les objets", "portail qui aspire", "voyage en dimension de poche",
   "Les corneilles reconnaissent les visages humains.", "Ses yeux virent à l'orange vif, comme des braises."},
  {"Anneaux", "Œil à anneaux", M_ANN, 176, 150, 220, 5, 5, "Origine : lignée du Sage, très rare", "Éveil : héritage ancien", "voit les flux d'énergie", "attire et repousse", "invoque des alliés",
   "Leurs ailes froissent l'air comme de la soie.", "Ses yeux deviennent violets, cerclés d'anneaux."},
  {"Blanc", "Œil blanc", M_BLANC, 240, 240, 250, 4, 3, "Origine : grande famille aux yeux pâles", "Éveil : dès la naissance", "vision presque à 360 degrés", "voit à travers les murs", "voit les réseaux d'énergie",
   "Un corbeau peut vivre plus de 23 ans dans la nature.", "Ses yeux deviennent blancs et lumineux, sans pupille."},
  {"Pétales", "Œil à pétales", M_PET, 120, 176, 236, 5, 4, "Origine : fusion de deux lignées", "Éveil : après une greffe rare", "anime un géant de pierre", "boules de lumière", "permet de voler",
   "Des corneilles glissent sur une écorce, comme sur une planche à neige.", "Ses yeux deviennent bleu glace, en fleur blanche."},
  {"6 pétales", "Œil à six pétales", M_FLEUR, 214, 24, 34, 5, 4, "Origine : version éternelle des yeux rouges", "Éveil : après avoir appris une vérité", "flammes noires des deux yeux", "vue qui ne baisse plus", "garde les pouvoirs de l'œil d'origine",
   "Des corbeaux auraient guidé des loups vers leurs proies.", "Ses yeux deviennent noirs, avec six pétales rouges."},
};

static int scP = 0, scB = 0, scPmax = 0, scBmax = 0, hP = 0;      // défilement du cadre de droite et du « Le saviez-vous ? »
static int zone = 0, sc0 = 0; static bool defile = false;
static int ecran = 0;              // 0 liste, 1 fiche, 2 sur le corbeau, 3 création
static int sel = 0;                // œil choisi (0..7)
static bool ouvert = false, sale = true, volVu = false;
static int animDe = 0, animVers = 0, animSens = 0; static uint32_t animT0 = 0;      // glissement d'une fiche à l'autre
static cb::Corbeau cbx; static bool cbxOk = false; static uint32_t tCb = 0;
// toucher
static bool bas = false; static int sx = 0, sy = 0, px = 0, py = 0; static uint32_t st = 0;
static int cibleBt = -1;

static inline uint16_t BGc() { return C(20, 3, 8); }
static inline uint16_t CFl(float r, float g, float b) { return cb::CF(r, g, b); }
static inline float rad(float a) { return a * 0.0174533f; }
static inline void polaire(float cx, float cy, float r, float a, float& x, float& y) { x = cx + r * cosf(rad(a)); y = cy + r * sinf(rad(a)); }
static inline uint16_t ton(float r, float g, float b, float k) { return CFl(r * k, g * k, b * k); }

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
static void cercle(M5Canvas& g, float x, float y, float r, uint16_t c) { int rr = cb::ar(r); if (rr < 1) rr = 1; g.drawCircle(cb::ar(x), cb::ar(y), rr, c); }
static void iris(M5Canvas& g, float cx, float cy, float R, int motif, float cr, float cg, float cbv, int n, float rot) {
  const uint16_t NOIR = C(12, 2, 6);
  if (motif == M_BLANC) {
    cb::disque(g, cx, cy, R, C(236, 236, 250)); cb::disque(g, cx, cy, R * 0.86f, C(248, 248, 255)); cercle(g, cx, cy, R, NOIR); if (R > 20) cercle(g, cx, cy, R - 1, NOIR);
    for (int k = 0; k < 4; k++) { float x, y; polaire(cx, cy, R * 0.15f, rot + 45 + k * 90, x, y); cb::disque(g, x, y, cb::mxf(0.6f, R * 0.035f), NOIR); }
    return;
  }
  if (motif == M_PET) {
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
  uint16_t col = CFl(cr, cg, cbv);
  if (motif == M_ANN) {
    cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.7f));
    for (int i = 8; i >= 1; i--) { float r = R * i / 8.0f; cb::disque(g, cx, cy, r, ton(cr, cg, cbv, (i % 2) ? 0.75f : 1.05f)); if (R > 14) cercle(g, cx, cy, r, NOIR); }
    cb::disque(g, cx, cy, R * 0.07f + 0.5f, NOIR); cercle(g, cx, cy, R, NOIR);
    return;
  }
  if (motif == M_FLEUR) {
    cb::disque(g, cx, cy, R, C(14, 6, 10)); cercle(g, cx, cy, R, NOIR);
    for (int k = 0; k < 6; k++) {
      float a = rot + 90 + k * 60 - 90, nx = cosf(rad(a + 90)), ny = sinf(rad(a + 90)), L1x, L1y, R1x, R1y, L2x, L2y, R2x, R2y, b0x, b0y, tx, ty, x, y;
      polaire(cx, cy, R * 0.12f, a, b0x, b0y); polaire(cx, cy, R * 0.92f, a, tx, ty);
      float w1 = sinf(3.1416f * 0.3f) * R * 0.22f, w2 = sinf(3.1416f * 0.6f) * R * 0.22f;
      polaire(cx, cy, R * 0.12f + R * 0.78f * 0.3f, a, x, y); L1x = x + nx * w1; L1y = y + ny * w1; R1x = x - nx * w1; R1y = y - ny * w1;
      polaire(cx, cy, R * 0.12f + R * 0.78f * 0.6f, a, x, y); L2x = x + nx * w2; L2y = y + ny * w2; R2x = x - nx * w2; R2y = y - ny * w2;
      cb::tri(g, b0x, b0y, L1x, L1y, R1x, R1y, col); cb::tri(g, L1x, L1y, R1x, R1y, L2x, L2y, col); cb::tri(g, R1x, R1y, L2x, L2y, R2x, R2y, col); cb::tri(g, L2x, L2y, R2x, R2y, tx, ty, col);
    }
    cb::disque(g, cx, cy, R * 0.16f, C(14, 6, 10));
    return;
  }
  cb::disque(g, cx, cy, R, ton(cr, cg, cbv, 0.75f)); cb::disque(g, cx, cy, R * 0.94f, col);
  if (motif == M_VIRG) {
    cercle(g, cx, cy, R * 0.6f, NOIR); cb::disque(g, cx, cy, R * 0.2f, NOIR);
    for (int i = 0; i < n; i++) virgule(g, cx, cy, R * 0.6f, rot + i * 360.0f / (n > 0 ? n : 1), (n <= 3) ? R * 0.17f : R * 0.13f, NOIR);
  } else if (motif == M_PALES) {
    for (int k = 0; k < 3; k++) {
      float a = rot + k * 120, E[16][2]; int m = 0;
      for (int i = 0; i < 8; i++) { float t = i / 7.0f; polaire(cx, cy, R * (0.32f + 0.6f * t), a - 50.0f * powf(1 - t, 1.5f), E[m][0], E[m][1]); m++; }
      for (int i = 7; i >= 0; i--) { float t = i / 7.0f; polaire(cx, cy, R * (0.32f + 0.6f * t), a + 22.0f * powf(1 - t, 0.7f), E[m][0], E[m][1]); m++; }
      for (int i = 1; i < 15; i++) cb::tri(g, E[0][0], E[0][1], E[i][0], E[i][1], E[i + 1][0], E[i + 1][1], NOIR);
    }
    cb::disque(g, cx, cy, R * 0.33f, ton(cr, cg, cbv, 1.1f));
  } else if (motif == M_LAMES) {
    cercle(g, cx, cy, R * 0.58f, NOIR);
    for (int k = 0; k < 3; k++) {
      float a = rot + k * 120, x1, y1, x2, y2, x3, y3;
      polaire(cx, cy, R * 0.2f, a - 48, x1, y1); polaire(cx, cy, R * 0.97f, a, x2, y2); polaire(cx, cy, R * 0.2f, a + 48, x3, y3);
      cb::tri(g, x1, y1, x2, y2, x3, y3, NOIR);
    }
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
// œil n° i (0..6 = les yeux du Dex, 7 = celui que l'on crée soi-même)
static void couleurOeil(int i, int& motif, float& r, float& g, float& b, int& n) {
  if (i < 7) { motif = DEF[i].motif; r = DEF[i].r; g = DEF[i].g; b = DEF[i].b; n = 3; return; }
  static const uint8_t MOT[4] = {M_ANN, M_VIRG, M_PALES, M_FLEUR};
  motif = MOT[cb::oCu.motif & 3]; cb::teinteRVB(cb::oCu.teinte, r, g, b); n = cb::oCu.marques;
}
static bool decouvert(int i) { return i < 7 || (i == 7 && cb::oCu.ok); }
static int nbDecouverts() { int n = 0; for (int i = 0; i < 8; i++) if (decouvert(i)) n++; return n; }
static const char* courtOeil(int i) { return i < 7 ? DEF[i].court : "Ton œil"; }
static const char* nomOeil(int i) { return i < 7 ? DEF[i].nom : "Ton œil"; }
static int suivant(int i, int sens) { for (int k = 0; k < 8; k++) { i = (i + sens + 8) % 8; if (decouvert(i)) return i; } return i; }

// ---------- cri du pouvoir de l'œil (touche l'œil sur la fiche, ou à l'ouverture de la fiche) ----------
static void ecouter(int i) {
#ifdef DX_SONS
  static const uint8_t* const D[5] = {POUVOIR_1, POUVOIR_2, POUVOIR_3, POUVOIR_4, POUVOIR_5};
  static const uint32_t N[5] = {POUVOIR_1_LEN, POUVOIR_2_LEN, POUVOIR_3_LEN, POUVOIR_4_LEN, POUVOIR_5_LEN};
  static const uint8_t MAPP[8] = {0, 1, 4, 3, 2, 3, 2, 0};
  if (R.volume == 0 || lectureEnCours) return;
  if (!M5.Speaker.isRunning()) { M5.Mic.end(); M5.Speaker.begin(); }
  M5.Speaker.setVolume(R.volume); M5.Speaker.setChannelVolume(5, 255);
  int p = MAPP[i & 7]; M5.Speaker.playRaw(D[p], N[p], POUVOIRS_RATE, false, 1, 5, true);
#else
  (void)i;
#endif
}
// ---------- petits éléments d'interface ----------
static void etoile(M5Canvas& g, float cx, float cy, float r, uint16_t c) {
  float p[10][2];
  for (int i = 0; i < 10; i++) polaire(cx, cy, (i % 2 == 0) ? r : r * 0.45f, -90 + i * 36, p[i][0], p[i][1]);
  for (int i = 0; i < 10; i++) { int j = (i + 1) % 10; cb::tri(g, cx, cy, p[i][0], p[i][1], p[j][0], p[j][1], c); }
}
// La police de l'écran n'a ni « Œ/œ » ni la puce « • » (elles s'affichaient en carrés) : on les remplace avant de dessiner.
// Seuls des caractères latins accentués (é è ê à â ç î ô û ù É À Ç « » °) restent : la police les dessine bien.
static String fr(const char* s) {
  String o; o.reserve(strlen(s) + 6);
  const uint8_t* p = (const uint8_t*)s;
  while (*p) {
    if (p[0] == 0xC5 && p[1] == 0x92) { const uint8_t c = p[2]; o += ((c >= 'a' && c <= 'z') || c >= 0xC3) ? "Oe" : "OE"; p += 2; }
    else if (p[0] == 0xC5 && p[1] == 0x93) { o += "oe"; p += 2; }
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
  texte(g, ox + 8, 11, titre, C(255, 236, 230), 2, true); texte(g, ox + 312, 11, droite, C(255, 96, 100), 3, true);
}
static void bouton(M5Canvas& g, int x, int w, const char* lib, uint16_t fond = 0) {
  g.fillRoundRect(x, 206, w, 30, 8, fond ? fond : C(60, 8, 18)); g.drawRoundRect(x, 206, w, 30, 8, C(255, 70, 80));
  texte(g, x + w / 2, 221, lib, C(255, 236, 230), 1, true);
}

// ---------- écran 0 : la liste ----------
static void ecranListe(M5Canvas& g) {
  g.fillScreen(BGc());
  char nb[12]; snprintf(nb, sizeof(nb), "%d/12", nbDecouverts()); entete(g, 0, "DEX DES YEUX", nb);
  for (int i = 0; i < NE; i++) {
    int c = i % 4, r = i / 4, x = 6 + c * 78, y = 27 + r * 48, w = 74, h = 44;
    bool ok = decouvert(i);
    g.fillRoundRect(x, y, w, h, 6, ok ? C(34, 6, 14) : C(24, 4, 10)); g.drawRoundRect(x, y, w, h, 6, (i == sel && ok) ? C(255, 70, 80) : C(100, 18, 28));
    char num[6]; snprintf(num, sizeof(num), "%03d", i + 1); texte(g, x + 3, y + 1, num, ok ? C(255, 96, 100) : C(100, 40, 50));
    if (ok) {
      int motif, n; float cr, cg, cbv; couleurOeil(i, motif, cr, cg, cbv, n);
      oeil(g, x + 37, y + 18, 22, 11, motif, cr, cg, cbv, n, C(34, 6, 14));
      texte(g, x + 37, y + 35, courtOeil(i), C(255, 236, 230), 1);
    } else { texte(g, x + 37, y + 18, "?", C(120, 40, 50), 1, true); texte(g, x + 37, y + 35, "???", C(120, 40, 50), 1); }
  }
  char s[24]; snprintf(s, sizeof(s), "Découverts : %d/12", nbDecouverts()); texte(g, 8, 172, s, C(255, 236, 230));
  for (int i = 0; i < NE; i++) { int x = 128 + i * 15; g.fillRect(x, 172, 11, 11, i < nbDecouverts() ? C(214, 24, 34) : C(40, 8, 14)); g.drawRect(x, 172, 11, 11, C(110, 20, 30)); }
  g.fillRoundRect(4, 187, 312, 17, 5, C(8, 40, 44)); g.drawRoundRect(4, 187, 312, 17, 5, C(30, 140, 150));
  texte(g, 160, 196, "Contenu inventé pour le jeu (sauf les vrais faits)", C(170, 235, 240), 1);
  bouton(g, 4, 102, "RETOUR"); bouton(g, 108, 102, "CRÉER"); bouton(g, 212, 102, "CORBEAUX");
}

// ---------- écran 1 : la fiche d'un œil (ox = décalage, pour le glissement) ----------
static void fiche(M5Canvas& g, int idx, int ox) {
  g.fillRect(ox, 0, 320, 240, BGc());
  char tit[40], num[12]; snprintf(tit, sizeof(tit), "N°%03d  %s", idx + 1, nomOeil(idx)); snprintf(num, sizeof(num), "%d/12", idx + 1);
  entete(g, ox, tit, num);
  int motif, n; float cr, cg, cbv; couleurOeil(idx, motif, cr, cg, cbv, n);
  oeil(g, ox + 78, 69, 70, 38, motif, cr, cg, cbv, n, BGc());
  { int sx_ = ox + 128, sy_ = 30; uint16_t cs = C(120, 230, 240); g.fillRect(sx_, sy_ + 3, 4, 6, cs); g.fillTriangle(sx_ + 4, sy_ + 3, sx_ + 4, sy_ + 9, sx_ + 9, sy_ + 12, cs); g.fillTriangle(sx_ + 4, sy_ + 3, sx_ + 9, sy_, sx_ + 9, sy_ + 12, cs); g.drawLine(sx_ + 12, sy_ + 3, sx_ + 14, sy_ + 6, cs); g.drawLine(sx_ + 14, sy_ + 6, sx_ + 12, sy_ + 9, cs); g.drawLine(sx_ + 15, sy_ + 1, sx_ + 18, sy_ + 6, cs); g.drawLine(sx_ + 18, sy_ + 6, sx_ + 15, sy_ + 11, cs); }      // icône haut-parleur
  // rareté et danger
  int et = idx < 7 ? DEF[idx].etoiles : 3, da = idx < 7 ? DEF[idx].danger : 2;
  g.fillRoundRect(ox + 4, 116, 148, 42, 6, C(10, 36, 40)); g.drawRoundRect(ox + 4, 116, 148, 42, 6, C(30, 140, 150));
  texte(g, ox + 10, 121, "Rareté", C(120, 230, 240)); texte(g, ox + 10, 140, "Danger", C(120, 230, 240));
  for (int k = 0; k < 5; k++) etoile(g, ox + 78 + k * 15, 127, 6.0f, k < et ? C(255, 205, 80) : C(70, 50, 30));
  for (int k = 0; k < 5; k++) { uint16_t c = k < da ? CFl(255, 200 - k * 36, 30) : C(50, 14, 14); g.fillRect(ox + 72 + k * 15, 140, 12, 12, c); g.drawRect(ox + 72 + k * 15, 140, 12, 12, C(110, 30, 20)); }
  g.fillRoundRect(ox + 6, 96, 28, 13, 4, C(20, 120, 130)); texte(g, ox + 20, 103, "JEU", C(255, 255, 255), 1);      // étiquette « JEU » dans le coin de l'œil, loin des étoiles
  // texte (cadre de droite) : défile au doigt (haut/bas) quand il est plus long que le cadre
  g.fillRoundRect(ox + 156, 26, 160, 132, 6, C(12, 2, 6)); g.drawRoundRect(ox + 156, 26, 160, 132, 6, C(120, 22, 32));
  const uint16_t BL = C(255, 236, 230), RS = C(255, 96, 100);
  const int X = ox + 162, Wd = 140, y0 = 30, hv = 124;
  g.setClipRect(ox + 158, 28, 156, 128);
  int y = y0 - scP;
  if (idx < 7) {
    y += bloc(g, X, y, Wd, DEF[idx].l1, 14, BL, 9) * 14 + 1;
    y += bloc(g, X, y, Wd, DEF[idx].l2, 14, BL, 9) * 14 + 3;
    texte(g, X, y, "Pouvoirs :", RS); y += 16;
    const char* P[3] = {DEF[idx].p1, DEF[idx].p2, DEF[idx].p3};
    for (int k = 0; k < 3; k++) { puce(g, X, y, RS); y += bloc(g, X + 10, y, Wd - 10, P[k], 14, BL, 9) * 14 + 1; }
  } else {
    y += bloc(g, X, y, Wd, "Origine : toi", 14, BL, 9) * 14 + 1;
    y += bloc(g, X, y, Wd, "Éveil : à ta façon", 14, BL, 9) * 14 + 3;
    texte(g, X, y, "Caractéristiques :", RS); y += 16;
    char b[40]; snprintf(b, sizeof(b), "%d virgule%s", n, n > 1 ? "s" : ""); puce(g, X, y, RS); texte(g, X + 10, y, b, BL); y += 15;
    puce(g, X, y, RS); texte(g, X + 10, y, "couleur de ton choix", BL); y += 15;
    puce(g, X, y, RS); texte(g, X + 10, y, "un œil unique", BL); y += 15;
  }
  g.clearClipRect();
  hP = y + scP - y0 + 2; { int mx = hP - hv; if (mx < 0) mx = 0; if (scP > mx) scP = mx; scPmax = mx; }
  if (scPmax > 0) {      // barre de défilement + flèches
    const int tx = ox + 311, th = 124; int bh = th * hv / hP; if (bh < 12) bh = 12; int by = 30 + (th - bh) * scP / scPmax;
    g.fillRoundRect(tx, 30, 3, th, 1, C(50, 12, 18)); g.fillRoundRect(tx, by, 3, bh, 1, C(255, 96, 100));
    if (scP < scPmax) g.fillTriangle(ox + 300, 150, ox + 308, 150, ox + 304, 155, C(255, 205, 80));
    if (scP > 0) g.fillTriangle(ox + 300, 36, ox + 308, 36, ox + 304, 31, C(255, 205, 80));
  }
  // le saviez-vous ? (défile aussi si le texte est long)
  g.fillRoundRect(ox + 4, 160, 312, 44, 6, C(40, 28, 6)); g.drawRoundRect(ox + 4, 160, 312, 44, 6, C(200, 150, 40));
  texte(g, ox + 10, 162, "LE SAVIEZ-VOUS ?", C(255, 205, 80));
  g.setClipRect(ox + 8, 175, 300, 27);
  { int n = bloc(g, ox + 10, 176 - scB, 296, idx < 7 ? DEF[idx].anec : "Les corneilles reconnaissent les visages humains.", 13, BL, 9);
    int mx = n * 13 - 26; if (mx < 0) mx = 0; if (scB > mx) scB = mx; scBmax = mx;
    g.clearClipRect();
    if (mx > 0) { g.fillRoundRect(ox + 310, 176, 3, 25, 1, C(80, 56, 14)); int bh = 25 * 26 / (n * 13); if (bh < 8) bh = 8; g.fillRoundRect(ox + 310, 176 + (25 - bh) * scB / mx, 3, bh, 1, C(255, 205, 80)); } }
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
  oeil(g, 260, 48, 28, 14, motif, cr, cg, cbv, n, C(12, 2, 6));
  texte(g, 210, 70, "SUR TON CORBEAU", C(120, 230, 240)); g.fillRoundRect(288, 84, 24, 13, 4, C(20, 120, 130)); texte(g, 300, 91, "JEU", C(255, 255, 255), 1);
  bloc(g, 210, 100, 100, sel < 7 ? DEF[sel].effet : "Ton œil prend ta couleur et brille la nuit.", 14, C(255, 236, 230), 6);
  bool actif = (cb::oeilActif == sel + 1 && cb::oeilActif != 0 && cb::oeilActif == (uint8_t)prefs.getUChar("cbeil", 0));
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
  int motif, n; float cr, cg, cbv; couleurOeil(7, motif, cr, cg, cbv, n);
  oeil(g, 250, 78, 58, 30, motif, cr, cg, cbv, n, BGc());
  texte(g, 250, 124, "TON ŒIL", C(255, 236, 230), 1, true);
  snprintf(s, sizeof(s), "%d virgule%s", n, n > 1 ? "s" : ""); texte(g, 250, 144, s, C(200, 150, 150), 1);
  texte(g, 250, 164, "Touche pour régler", C(180, 120, 120), 1);
  bouton(g, 4, 102, "ENREGISTRER"); bouton(g, 108, 102, "HASARD"); bouton(g, 212, 102, "RETOUR");
}

// ---------- mise en page générale ----------
static void majApercuOeil() { if (ecran == 2) { cb::oeilActif = (uint8_t)(sel + 1); cb::appliquerOeil(); } }
static void quitterApercu() { cb::chargerOeil(); }      // on revient à l'œil réellement choisi
static void ouvrir() { ouvert = true; scP = scB = 0; defile = false; zone = 0; ecran = 0; sel = 0; sale = true; volVu = false; animSens = 0; cbxOk = false; bas = false; cb::chargerOeil(); }
static void quitter() { if (!ouvert) return; ouvert = false; quitterApercu(); if (!lectureEnCours && M5.Speaker.isRunning()) { M5.Speaker.stop(); M5.Speaker.end(); M5.Mic.begin(); } }
static void pousser() { cv.pushSprite(0, 0); }
static void dessiner() {
  if (!ouvert) ouvrir();
  uint32_t m = millis();
  if (m < finAffVolume) volVu = true; else if (volVu) { volVu = false; sale = true; }
  if (animSens != 0) {      // glissement d'une fiche à l'autre
    float e = (m - animT0) / 220.0f; if (e >= 1) { e = 1; }
    float p = cb::pas(e);
    fiche(cv, animDe, (int)(-animSens * 320 * p)); fiche(cv, animVers, (int)(animSens * 320 * (1 - p)));
    pousser(); if (e >= 1) { animSens = 0; sale = false; }
    return;
  }
  if (ecran == 2 && m - tCb >= 45) { tCb = m; sale = true; }
  if (!sale) return;
  sale = false;
  switch (ecran) { case 0: ecranListe(cv); break; case 1: fiche(cv, sel, 0); break; case 2: ecranCorbeau(cv); break; default: ecranCreer(cv); break; }
  pousser();
}

// ---------- toucher : glisser = fiche suivante / précédente, toucher = choisir ----------
static void aller(int sens) {      // fiche suivante (+1) ou précédente (-1), avec glissement
  int v = suivant(sel, sens); if (v == sel) return;
  if (ecran == 1) { animDe = sel; animVers = v; animSens = sens; animT0 = millis(); }
  sel = v; scP = scB = 0; majApercuOeil(); sale = true;
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
    if (b == 0) aller(-1); else if (b == 1) { ecran = 0; sale = true; } else if (b == 2) aller(1); else { ecran = 2; majApercuOeil(); sale = true; }
    // (le défilement repart en haut à chaque changement de fiche)
  } else if (ecran == 2) {
    if (b == 0) { quitterApercu(); ecran = 1; scP = scB = 0; sale = true; }
    else if (b == 1) { bool actif = (cb::oeilActif == sel + 1 && prefs.getUChar("cbeil", 0) == sel + 1); cb::choisirOeil(actif ? 0 : sel + 1); majApercuOeil(); if (actif) { ecran = 1; } sale = true; }
    else { page = 32; bip(700, 40); }
  } else {
    if (b == 0) { cb::oCu.ok = 1; cb::sauverOeilCu(); cb::choisirOeil(8); sel = 7; ecran = 1; sale = true; }
    else if (b == 1) { cb::oCu.teinte = (uint8_t)random(256); cb::oCu.marques = (uint8_t)random(7); cb::oCu.motif = (uint8_t)random(4); sale = true; }
    else { ecran = 0; sale = true; }
  }
}
static int boutonSous(int x) {
  if (ecran == 1) { int b = (x - 4) / 78; return b < 0 ? 0 : (b > 3 ? 3 : b); }
  int b = (x - 4) / 104; return b < 0 ? 0 : (b > 2 ? 2 : b);
}
static void toucherEcran(int x, int y) {
  if (y >= 206) { toucherBouton(boutonSous(x)); return; }
  if (ecran == 0) {
    for (int i = 0; i < NE; i++) { int c = i % 4, r = i / 4, cx = 6 + c * 78, cy = 27 + r * 48; if (x >= cx && x < cx + 74 && y >= cy && y < cy + 44 && decouvert(i)) { sel = i; ecran = 1; scP = scB = 0; sale = true; ecouter(i); return; } }
  } else if (ecran == 1) {
    if (x < 152 && y >= 26 && y < 112) ecouter(sel);      // toucher l'œil : écouter le cri du pouvoir
  } else if (ecran == 3) {
    if (y >= 128 && y < 162) for (int i = 0; i < 4; i++) { int bx = 12 + i * 41; if (x >= bx && x < bx + 37) { cb::oCu.motif = (uint8_t)i; sale = true; } }
    creerGlisser(x, y);
  }
}
static void toucher(const m5::touch_detail_t& t) {
  if (!ouvert) return;
  int x = t.x, y = t.y;
  if (t.wasPressed()) {
    bas = true; sx = px = x; sy = py = y; defile = false; zone = 0;
    if (ecran == 3) creerGlisser(x, y);
    else if (ecran == 1 && animSens == 0) { if (x >= 156 && x < 316 && y >= 26 && y < 158) { zone = 1; sc0 = scP; } else if (y >= 160 && y < 205) { zone = 2; sc0 = scB; } }
  }
  else if (t.isPressed() && bas) {
    px = x; py = y; if (ecran == 3 && sy < 206) creerGlisser(x, y);
    if (zone && (defile || abs(py - sy) > 7) && abs(py - sy) >= abs(px - sx)) {      // glisser vers le haut / le bas : le texte défile
      defile = true; int v = sc0 - (py - sy), mx = (zone == 1) ? scPmax : scBmax; v = v < 0 ? 0 : (v > mx ? mx : v);
      if (zone == 1) { if (v != scP) { scP = v; sale = true; } } else if (v != scB) { scB = v; sale = true; }
    }
  }
  if (t.wasReleased() && bas) {
    bas = false; if (defile) { defile = false; zone = 0; return; }
    int dx = px - sx, dy = py - sy;
    if ((ecran == 1 || ecran == 2) && abs(dx) > 40 && abs(dy) < 45 && sy < 206) { aller(dx < 0 ? 1 : -1); return; }
    if (abs(dx) < 14 && abs(dy) < 14) toucherEcran(sx, sy);
  }
}

}  // namespace dx

static inline void dexDessiner() { dx::dessiner(); }
static inline void dexToucher(const m5::touch_detail_t& t) { dx::toucher(t); }
static inline void dexQuitter() { dx::quitter(); }
