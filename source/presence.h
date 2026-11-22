#pragma once
// presence.h : PRESENCE / ABSENCE (3.1.6).
// Deux etats pour tous les univers : ABSENCE (personne n'interagit depuis ~25 s : 4 images/s, mouvements rares et lents, lumiere basse)
// et PRESENCE (toucher, secousse, changement d'orientation, bruit fort, branchement : animations completes).
// Entre les deux : une scene (ENTREE / SORTIE) de ~3 s avec mise en scene. Rattrapage : des traces de ce qui s'est passe pendant l'absence.
// Ce fichier ne depend d'aucun autre fichier du programme (le dessin recoit le canevas en parametre).
#include <M5Unified.h>

namespace pr {

enum { PRESENT = 0, SORTIE = 1, ABSENT = 2, ENTREE = 3 };
static const uint32_t DELAI_ABS = 25000;      // sans interaction : on part
static const uint32_t DUREE_ENTREE = 3400, DUREE_SORTIE = 3000;
static const uint32_t PAS_TRACE = 30000;      // une trace de plus toutes les 30 s d'absence (3 au plus)

static uint8_t etat = PRESENT;
static float sc = 1.0f;                       // avancement de la scene : 0 = parti (lumiere eteinte), 1 = la (lumiere allumee)
static uint32_t derInter = 0, tTick = 0, debAbsence = 0, tEveil = 0, tClic = 0, masque = 0;
static uint32_t dureeAbs = 0, graine = 1;
static uint8_t nbTraces = 0;
static bool lumOn = true, actif = true, vraieArrivee = false;
static uint8_t clicEvt = 0;                   // 1 = on vient d'allumer, 2 = on vient d'eteindre (lu une fois par le dessin)
static float lumLisse = 1.0f;

static inline bool absent() { return actif && etat == ABSENT; }
static inline bool calme() { return actif && (etat == ABSENT || etat == SORTIE); }

// Une interaction : l'utilisateur est la.
static void interaction() {
  if (!actif) return;
  uint32_t m = millis(); derInter = m;
  if (etat == ABSENT) {
    dureeAbs = m - debAbsence; etat = ENTREE; tEveil = m; masque = 0; vraieArrivee = true;
    { uint32_t q = dureeAbs / PAS_TRACE; nbTraces = (uint8_t)(q > 3 ? 3 : q); } graine = (debAbsence ? debAbsence : 1) * 2654435761UL;
  } else if (etat == SORTIE) {
    etat = ENTREE; tEveil = m; masque = 0; vraieArrivee = false; nbTraces = 0;
  }
}

// Appelee a chaque tour de boucle : fait avancer la scene. sceneVisible : une page de personnage/univers est a l'ecran.
static void tick(bool sceneVisible, bool occupe = false) {
  static bool init = false; uint32_t m = millis(); if (!init) { init = true; tTick = m; derInter = m; }
  float dt = (m - tTick) / 1000.0f; tTick = m; if (dt > 0.2f) dt = 0.2f; if (dt < 0) dt = 0;
  if (!actif) { etat = PRESENT; sc = 1.0f; lumOn = true; lumLisse = 1.0f; return; }
  if (!sceneVisible && etat == PRESENT) derInter = m;      // on ne s'absente que devant une scene
  if (occupe) { if (etat == PRESENT) derInter = m; else if (etat == SORTIE) interaction(); }      // un evenement est en cours dans la scene : personne ne s'en va
  switch (etat) {
    case PRESENT: if ((int32_t)(m - derInter) > (int32_t)DELAI_ABS) { etat = SORTIE; debAbsence = m; } break;
    case SORTIE: sc -= dt * 1000.0f / DUREE_SORTIE; if (sc <= 0.0f) { sc = 0.0f; etat = ABSENT; } break;
    case ENTREE: sc += dt * 1000.0f / DUREE_ENTREE; if (sc >= 1.0f) { sc = 1.0f; etat = PRESENT; derInter = m; } break;
    default: sc = 0.0f; break;
  }
  bool on = (sc >= 0.5f);
  if (etat == PRESENT) on = true; else if (etat == ABSENT) on = false;
  if (on != lumOn) { lumOn = on; tClic = m; clicEvt = on ? 1 : 2; }
  float cible = lumOn ? 1.0f : 0.10f;
  if (lumOn && tClic && m - tClic < 450) {      // l'ampoule papillote avant de s'allumer franchement
    float u = (m - tClic) / 1000.0f;
    if (u < 0.07f) cible = 1.0f; else if (u < 0.16f) cible = 0.10f; else if (u < 0.25f) cible = 0.65f; else if (u < 0.31f) cible = 0.10f; else cible = 1.0f;
  }
  if (cible < lumLisse) { lumLisse -= dt * 6.0f; if (lumLisse < cible) lumLisse = cible; } else lumLisse = cible;
}

static uint8_t consommerClic() { uint8_t c = clicEvt; clicEvt = 0; return c; }

// Cadence : en absence 4 images/s (puis 2,5 au bout de 2 minutes) ; sinon la cadence normale demandee.
static uint32_t periode(uint32_t base) {
  if (!absent()) return base;
  uint32_t p = ((millis() - debAbsence) > 120000UL) ? 400u : 250u; return base > p ? base : p;
}

// Niveau de lumiere de 1 a 8 (8 = plein) : la piece (ampoule) et les corbeaux (lueur du ciel qui revient).
static int niveauPiece() { int n = (int)(lumLisse * 8.0f + 0.5f); return n < 1 ? 1 : (n > 8 ? 8 : n); }
static int niveauCb() {
  if (!actif) return 8;
  float u = sc < 0 ? 0 : (sc > 1 ? 1 : sc); u = u * u * (3.0f - 2.0f * u);
  int n = (int)((0.5f + 0.5f * u) * 8.0f + 0.5f); return n < 1 ? 1 : (n > 8 ? 8 : n);
}
// Rythme du monde (corbeaux) : 1 en presence ; en absence les evenements sont rares.
static float facteurMonde() { if (!actif) return 1.0f; if (etat == ABSENT) return 0.3f; if (etat == SORTIE) return 0.3f + 0.7f * (sc > 1 ? 1 : sc); return 1.0f; }

// Secondes depuis l'arrivee (grand nombre quand on n'est pas en train d'arriver).
static float depuisArrivee() { return (etat == ENTREE || etat == PRESENT) ? (millis() - tEveil) / 1000.0f : 1e6f; }
// Un evenement unique, a s secondes apres l'arrivee (id de 0 a 31) : vrai une seule fois par arrivee.
static bool unefois(int id, float s) {
  if (!vraieArrivee || id < 0 || id > 31 || (masque & (1UL << id))) return false;
  float t = depuisArrivee(); if (t < s || t > s + 4.0f) return false;
  masque |= (1UL << id); return true;
}

// Mouvement du Core2 : on le prend en main ou on le secoue (la lecture est appelee a chaque tour par la physique).
static void imu(float ax, float ay, float az, float sec) {
  static float rx = 0, ry = 1, rz = 0, agit = 0; static bool init = false; static uint32_t t0 = 0;
  uint32_t m = millis(); float dt = (m - t0) / 1000.0f; t0 = m; if (dt > 0.5f || dt < 0) dt = 0.02f;
  if (!init) { rx = ax; ry = ay; rz = az; init = true; }
  float a = 1.0f - expf(-dt / 3.0f); rx += (ax - rx) * a; ry += (ay - ry) * a; rz += (az - rz) * a;
  if (fabsf(ax - rx) + fabsf(ay - ry) + fabsf(az - rz) > 0.35f) { interaction(); rx = ax; ry = ay; rz = az; }     // l'orientation a change
  agit += (sec - agit) * (1.0f - expf(-dt / 0.15f));
  if (agit > 0.18f) { interaction(); agit = 0; }                                                                  // secousse ou main qui le tient
}

// ---------- assombrir le canevas : lumiere n/8 (n de 0 a 8), lignes y0 a y1 exclu, tres rapide (deux pixels a la fois) ----------
static int ordreOctets = -1;     // 1 : octet fort en premier (cas habituel du canevas) ; 0 : ordre natif
static void assombrir(M5Canvas& g, int y0, int y1, int n) {
  if (n >= 8 || g.getColorDepth() != 16) return;
  uint8_t* b = (uint8_t*)g.getBuffer(); if (!b) return;
  if (y0 < 0) y0 = 0; if (y1 > g.height()) y1 = g.height();
  int W = g.width(); if (W & 1) return;
  if (ordreOctets < 0) {
    uint8_t s0 = b[0], s1 = b[1]; g.drawPixel(0, 0, g.color565(248, 0, 0)); ordreOctets = (b[0] == 0xF8) ? 1 : 0; b[0] = s0; b[1] = s1;
  }
  if (n < 0) n = 0;
  uint32_t* w = (uint32_t*)(void*)b + (size_t)y0 * (W / 2); size_t cnt = (size_t)(y1 - y0) * (W / 2);
  const bool sw = (ordreOctets == 1);
  const bool b2 = n & 4, b1 = n & 2, b0 = n & 1;
  for (size_t i = 0; i < cnt; i++) {
    uint32_t v = w[i];
    if (sw) v = ((v & 0x00FF00FFUL) << 8) | ((v >> 8) & 0x00FF00FFUL);
    uint32_t r = 0;
    if (b2) r += (v >> 1) & 0x7BEF7BEFUL;
    if (b1) r += (v >> 2) & 0x39E739E7UL;
    if (b0) r += (v >> 3) & 0x18E318E3UL;
    if (sw) r = ((r & 0x00FF00FFUL) << 8) | ((r >> 8) & 0x00FF00FFUL);
    w[i] = r;
  }
}

// ---------- la piece : interrupteur, traces, vie minimale ----------
static uint32_t alea(uint32_t& s) { s = s * 1664525UL + 1013904223UL; return s >> 8; }

// Interrupteur au mur de droite (le personnage l'actionne en arrivant et en partant).
static void dessinerInterrupteur(M5Canvas& g, bool on, bool appui) {
  const int x = 296, y = 84, w = 14, h = 22;
  g.fillRoundRect(x, y, w, h, 3, g.color565(46, 50, 70)); g.drawRoundRect(x, y, w, h, 3, g.color565(110, 120, 160));
  int ty = on ? y + 3 : y + h / 2 + 1;
  g.fillRoundRect(x + 3, ty, w - 6, h / 2 - 3, 2, on ? g.color565(255, 225, 120) : g.color565(70, 74, 100));
  if (on) g.drawCircle(x + w / 2, y + h / 2, 17, g.color565(70, 60, 40));
  if (appui) { g.drawCircle(x + w / 2, y + h / 2, 11, g.color565(255, 255, 255)); g.drawCircle(x + w / 2, y + h / 2, 15, g.color565(255, 240, 160)); }
}

// Traces de l'absence : ce qui a ete fait pendant que personne ne regardait (visibles jusqu'a ce que le personnage range).
static void dessinerTraces(M5Canvas& g, int n, uint32_t gr, float depuis, int gamX, int px, int py) {
  for (int i = 0; i < n && i < 3; i++) {
    uint32_t s = (gr ? gr : 1) + 977UL * (uint32_t)(i + 1); alea(s);
    float finT = 10.0f + i * 9.0f;                       // il range : la premiere trace disparait au bout de 10 s, les autres ensuite
    if (depuis > finT) continue;
    int k = (int)(alea(s) % 4), x0 = 90 + (int)(alea(s) % 150), y0 = 172 + (int)(alea(s) % 18);
    x0 += px / 2; y0 += py / 2;
    if (depuis > finT - 1.5f && ((int)(depuis * 10) & 1)) continue;        // clignote avant de disparaitre
    if (k == 0) { int bx = gamX + px + 4; for (int j = 0; j < 6; j++) g.fillCircle(bx + j * 7 + (int)(alea(s) % 5), 170 + (int)(alea(s) % 9), 2, g.color565(80, 255, 140)); }
    else if (k == 1) { for (int j = 0; j < 5; j++) { int ex = x0 + j * 12, ey = y0 + ((j & 1) ? 5 : 0); g.fillEllipse(ex, ey, 3, 2, g.color565(150, 90, 220)); g.fillEllipse(ex + 4, ey - 1, 1, 1, g.color565(200, 150, 255)); } }
    else if (k == 2) { g.fillEllipse(x0 + 4, y0 + 8, 14, 3, g.color565(30, 90, 70)); g.fillRoundRect(x0 - 8, y0, 22, 8, 3, g.color565(80, 255, 170)); g.drawRoundRect(x0 - 8, y0, 22, 8, 3, g.color565(200, 255, 230)); g.fillRect(x0 + 14, y0 + 2, 5, 4, g.color565(200, 200, 230)); }
    else { g.fillRect(x0, y0, 16, 11, g.color565(225, 225, 235)); g.drawRect(x0, y0, 16, 11, g.color565(150, 150, 170)); for (int j = 0; j < 3; j++) g.drawFastHLine(x0 + 2, y0 + 2 + j * 3, 11 - j * 2, g.color565(140, 140, 170)); }
  }
}

// Vie minimale de la piece vide : une diode qui clignote lentement et une poussiere qui flotte (peu couteux).
static void dessinerNuit(M5Canvas& g) {
  uint32_t m = millis();
  if ((m / 1800) % 3 == 0) g.fillCircle(14, 12, 2, g.color565(255, 40, 60));
  float fx = 40.0f + (float)((m / 90) % 240), fy = 96.0f + 14.0f * sinf(m / 2300.0f);
  g.drawPixel((int)fx, (int)fy, g.color565(190, 200, 235)); g.drawPixel((int)fx + 1, (int)fy, g.color565(110, 120, 160));
}

// Position du personnage pendant la scene : il entre par la droite, va a l'interrupteur, allume, puis rejoint sa place (et l'inverse a la sortie).
static float xScene(float xFinal) {
  const float xs = 276.0f, x0 = 372.0f;
  float u = sc < 0 ? 0 : (sc > 1 ? 1 : sc);
  auto fac = [](float t) { t = t < 0 ? 0 : (t > 1 ? 1 : t); return t * t * (3.0f - 2.0f * t); };
  if (u <= 0.40f) return x0 + (xs - x0) * fac(u / 0.40f);
  if (u <= 0.55f) return xs;
  return xs + (xFinal - xs) * fac((u - 0.55f) / 0.45f);
}
static bool enChemin() { return actif && (etat == ENTREE || etat == SORTIE) && (sc < 0.40f || sc > 0.55f) && sc > 0.0f && sc < 1.0f; }
static bool alInterrupteur() { return actif && (etat == ENTREE || etat == SORTIE) && sc >= 0.40f && sc <= 0.55f; }
static bool persoVisible() { return !actif || sc > 0.02f; }

// Nombre de traces a montrer et age de la plus recente (secondes) : en absence elles apparaissent peu a peu.
static int tracesVisibles() { if (!actif) return 0; if (etat == ABSENT || etat == SORTIE) return 0; return nbTraces; }
static int tracesPendantAbsence() { uint32_t q = (millis() - debAbsence) / PAS_TRACE; return (int)(q > 3 ? 3 : q); }
static uint32_t graineAbsence() { return (debAbsence ? debAbsence : 1) * 2654435761UL; }

}  // namespace pr
