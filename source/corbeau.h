#pragma once
// ======================= MODE CORBEAUX (page 32) =======================
// Tout est dessiné par le code (polygones, ellipses) : aucune image. Fond précalculé une seule fois,
// puis seules les zones qui bougent sont redessinées et envoyées à l'écran (zones sales).
// Les sons des corbeaux se mélangent au son déjà en cours (Matinale) sur des canaux libres du haut-parleur.
#include <stdint.h>
#include <pgmspace.h>
#include "oeil_pal.h"
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
extern float seuilSecousse;

namespace cb {

static const int NCB = 9;        // corbeaux : 8 adultes (1 à 8 visibles) + l'emplacement du petit corbeau
static const int NAD = 8, PC = 8;
static const int NOB = 4;        // objets au sol / qui tombent
static const int NPE = 6;        // perchoirs
static const int NPART = 44;    // plumes
static const int NFX = 6;       // éclats lumineux
static const int NEXT = 16;                // rectangles en plus pour les groupes d'éléments dispersés (plumes, petits corbeaux de la nuée, éclats) : plusieurs petits rectangles au lieu d'un seul très grand
static const int NENT = NCB + NOB + 14 + NEXT;
static const int H_SCENE = 206; // la barre de nourriture occupe le bas
static const float SOL_Y = 201.0f;
static const float PERCH[NPE][2] = {{94, 141}, {138, 135.8f}, {182, 130.3f}, {224, 125.6f}, {88, 62.5f}, {24, 94.5f}};

enum { E_PERCHE = 0, E_DECOLLE, E_VOL, E_ATTERRIT, E_SOL, E_DEHORS, E_POOF, E_TENU, E_TIR, E_OFF };
enum { B_RIEN = 0, B_PERCHOIR, B_NOUR, B_SORTIR, B_OBJET, B_ORBITE, B_VISITEUR, B_ATTRAPE, B_FOURRAGE, B_BAIN, B_POURSUITE, B_MONDE, B_FAM };      // B_FAM : mission d'un parent (famille.h)
namespace mo { static void decorNormal(M5Canvas& g); static void dessinBetes(M5Canvas& g); static void majNbBetes(); static const char* etiquette(); static void rebake(); static void basculer(); static void demandeLarge(bool on); static void compet(int iO); static void decorChange(); static bool vueFixe(); static void cibler(int b); static float envie(int k, int type); static int favori(int k); }      // monde.h : paysage, camera, excursions
static void syncUI();
enum { T_GRAINE = 0, T_VIANDE, T_NOIX, T_PAIN, T_BAIES, T_BRILLANT, T_BAGUE, T_KUNAI, T_SHURIKEN, T_PARCHEMIN, T_OEUF, T_NB };

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
static M5Canvas fondL(&M5.Display);      // fond de la vue large
static M5Canvas* fondAct = &fond; static bool fondLOk = false; static float zoomU = 0.0f, zoomC = 0.0f;
// cache du paysage de la caméra (vue large / suivi) : le paysage n'est recomposé (par bandes, une bande par image) que si la caméra a bougé d'au moins un pixel ; entre deux, il est réutilisé comme fond
static M5Canvas camA(&M5.Display), camB(&M5.Display);      // A : paysage affiché ; B : paysage en cours de composition
static M5Canvas* camC = &camA; static M5Canvas* camN = &camB;
static bool camCache = false, camTente = false, cOk = false, nEnCours = false, camRapide = true; static int nBande = 0;
static float cAZ = 1.0f, cAX = 0.0f, cAY = 0.0f, nCz = 1.0f, nCx = 160.0f, nCy = 128.0f, cPZ = 1.0f, cPX = 0.0f, cPY = 0.0f;      // cA : transformation du paysage affiché ; nC : caméra du paysage en cours ; cP : transformation voulue à l'image précédente
static bool ouvert = false, fondOk = false, debug = false, plein = true, sec = false;
// ouverture et fermeture progressives (3.1.32)
static int8_t ldPas = -1; static uint8_t ldMode = 0; static bool ldAff = false, ldFondSaut = false;      // ldPas : etape de chargement en cours (-1 = aucun) ; ldMode : 0 ouverture complete, 1 reconstruction du decor apres un changement d'ambiance
static bool fondGarde = false; static uint32_t fondSig = 0, tQuit = 0;      // le fond (sprite) est garde en memoire apres la fermeture tant que les reglages visuels n'ont pas change
static int8_t qPas = -1; static bool qGarder = false;      // etape de fermeture differee en cours (-1 = aucune)
static int bpp = 2;
static bool creerCamCache() {
  if (camCache) return true; if (camTente) return false; camTente = true;
  if (ESP.getFreePsram() < 900000UL) return false;
  camA.setColorDepth(cv.getColorDepth()); camA.setPsram(true); camB.setColorDepth(cv.getColorDepth()); camB.setPsram(true);
  if (!camA.createSprite(320, 240)) return false;
  if (!camB.createSprite(320, 240)) { camA.deleteSprite(); return false; }
  camCache = true; cOk = false; nEnCours = false; camRapide = true; camC = &camA; camN = &camB; return true;
}
static void libererCamCache() { if (camCache) { camA.deleteSprite(); camB.deleteSprite(); } camCache = camTente = cOk = nEnCours = false; camRapide = true; camC = &camA; camN = &camB; }
static void majBordsCam(M5Canvas& d) {      // haut (boutons) et bas (barre) : identiques au fond normal
  memcpy(d.getBuffer(), fond.getBuffer(), (size_t)24 * 320 * bpp);
  memcpy((uint8_t*)d.getBuffer() + (size_t)206 * 320 * bpp, (const uint8_t*)fond.getBuffer() + (size_t)206 * 320 * bpp, (size_t)34 * 320 * bpp);
}
static uint32_t tDernier = 0, tSec = 0, tPressee = 0;
static int nImg = 0, fps = 0, msDessin = 0;
static int fpsMoy10 = 0, fpsMin = 0, fpsHist[5] = {0, 0, 0, 0, 0}, fpsHN = 0;      // compteur d'images/s : moyenne glissante sur 5 s (x10), plus bas des 5 dernières secondes
static float msMax = 0.0f; static uint32_t maxIntervalle = 0, tPrecImg = 0;      // pire intervalle entre deux images (ms) sur la seconde en cours, puis sur les 5 dernières
static int choix = 0, presse = -1;
static float gAction = 2.0f, gPoof = 18.0f, gObjet = 30.0f, gAttrib = 0.0f;
static bool volVu = false, barreSale = false, sonsReels = false;
// 3.1.38 : petite bulle d'information au-dessus de la barre (nom et effet d'une nourriture au toucher long, messages), stock de nourritures fortes qui se renouvelle
static char infoL1[40] = "", infoL2[56] = ""; static uint32_t infoT = 0, infoDur = 0; static bool gRassasie = false, stSale = false; static int8_t barI = -1; static uint32_t barT0 = 0; static bool barInfo = false;
static void infoMsg(const char* a, const char* b, uint32_t dur = 2600) { snprintf(infoL1, sizeof(infoL1), "%s", a); snprintf(infoL2, sizeof(infoL2), "%s", b ? b : ""); infoT = millis(); if (!infoT) infoT = 1; infoDur = dur; }
static float stK[2] = {3.0f, 4.0f};      // stock de viande (0) et de noix (1) : les nourritures fortes sont rares et se renouvellent
static const float STMAX[2] = {3.0f, 4.0f}, STREG[2] = {42.0f, 28.0f};      // maximum, secondes pour en retrouver une
static inline int stIdx(int type) { return type == 1 ? 0 : (type == 2 ? 1 : -1); }      // T_VIANDE = 1, T_NOIX = 2
static bool stockPrend(int type) {
  const int i = stIdx(type); if (i < 0) return true;
  if (stK[i] < 1.0f) { char b[48]; snprintf(b, sizeof(b), "Il en revient une dans %d s", (int)((1.0f - stK[i]) * STREG[i]) + 1); infoMsg(i == 0 ? "Plus de viande" : "Plus de noix", b); return false; }
  stK[i] -= 1.0f; stSale = true; return true;
}
static bool btnVis = false;      // 3.1.37 : bouton « Visiteur » en haut de l'écran (clé cbbvis) : masqué par défaut ; les visites (gros plan, visiteur mystérieux) restent comme avant
static uint8_t modeCb = 0, choixL = 0, appelSub = 0;      // mode de la barre : 0 nourrir, 1 main, 2 lancer, 3 appel / effrayer, 4 fronde

// ---------- réglages du mode (nouvelles clés mémoire : cbnb, cbvis, cbsec, cbvib, cbamb, cbfps, cboeufs) ----------
// nouvelles clés (2.9.0) : cbact (activité 1 à 5), cbgp (visites au gros plan 0 à 3), cboefq (fréquence des œufs 0 à 3), cbhor (ambiance Horreur)
// fps : 0 non, 1 images/s en petit coin, 2 mesures détaillées ; cad : cadence maximale 0 = 20, 1 = 25, 2 = 30 images/s (défaut) ; deg : allègement automatique des détails si l'image est trop lente ; ecr : écran rapide (80 MHz) gardé
struct Opt { uint8_t nb, vis, sec, vib, amb, fps, act, gp, oe, hor, q, barre, fiche, lance, taille, paume, pvis, recul, cad, deg, ecr, cauto, dec, tend, calin, ui; };      // 3.1.39 : tend = tendresse avec l'oiseau dans la paume (clé cbtend), calin = câlins rares / normaux / fréquents (clé cbcalin)      // dec : decor 0 complet (defaut, s'allege seul si l'image ralentit), 1 allege, 2 minimal ;      // barre : barre de métal du gros plan ; fiche : fiche au double toucher ; lance : nourriture posée au toucher / lancée au glisser      // hor : lumière 0 Clair, 1 Sombre (défaut), 2 Horreur ; q : qualité 0 Fluide (défaut), 1 Normal, 2 Riche
static Opt OP = {4, 1, 1, 1, 0, 0, 3, 2, 2, 1, 0, 1, 1, 1, 1, 1, 2, 0, 2, 1, 0, 0};      // paume : main ouverte paume vers le haut (1 oui) ; pvis : visites dans la paume (0 jamais, 1 rare, 2 normales, 3 souvent) ; recul : vue reculée (0 normale, 1 reculée, 2 très reculée)
static uint8_t nbMax = 4;      // 3.1.41 : maximum de corbeaux CHOISI par le joueur (cle cbnb : seul le joueur la change) ; OP.nb = effectif en service (peut depasser le maximum quand un oeuf attrape grandit : cle cbeff)
static inline float tailleF() { static const float F[3] = {0.744f, 0.818f, 0.930f}; return F[OP.taille > 2 ? 1 : OP.taille]; }      // taille des corbeaux (3.1.26 : tous 15 % plus petits qu'avant pour que l'arbre paraisse plus grand) : petit, normal, grand
static uint32_t tQ = 0, tBon = 0;
static bool qDeg = false, qMin = false;      // fluidité : quand l'image prend trop de temps, détails coûteux réduits (qDeg) puis encore plus (qMin), automatiquement ; rétablis quand ça repasse sous le seuil      // nombre de corbeaux (3 à 6), visiteur (0 jamais, 1 rare, 2 souvent), secousse, vibration, ambiance, images/s, activité (1 à 5), gros plan (0 jamais, 1 rare, 2 normale, 3 souvent), œufs (idem), horreur
static bool panneau = false, panSale = true, ambChange = false, oeufsSale = false;
static uint32_t nbOeufs = 0;               // œufs éclos
static int nAct = 5;
static float ambK = 1.0f, ambLune = 1.0f, ambBrume = 1.0f, ambRouge = 0.0f, ambCol = 1.0f, ambVig = 0.0f, ambBrC[3] = {24.0f, 1.0f, 9.0f}; static int ambEt = 34;
static bool horreur() { return OP.hor != 0; }
static void appliquerAmb() {
  switch (OP.amb) {
    case 1: ambK = 1.0f; ambRouge = 26.0f; ambLune = 0.55f; ambBrume = 1.9f; ambEt = 8; break;      // brume rouge
    case 2: ambK = 0.42f; ambRouge = 0.0f; ambLune = 0.7f; ambBrume = 0.4f; ambEt = 70; break;      // nuit sombre
    default: ambK = 1.0f; ambRouge = 0.0f; ambLune = 1.0f; ambBrume = 1.0f; ambEt = 34; break;      // lune et étoiles
  }
  ambCol = 1.0f; ambVig = 0.0f; ambBrC[0] = 24.0f; ambBrC[1] = 1.0f; ambBrC[2] = 9.0f;
  if (OP.hor == 2) {      // Horreur : ciel presque noir à lueur rouge très faible, lune sanguine voilée, brume noire, bords noirs, étoiles rares
    ambK *= 0.50f; ambRouge *= 0.2f; ambLune *= 0.60f; ambBrume = 1.0f; ambEt = (ambEt > 9) ? 9 : ambEt / 2 + 1; ambCol = 0.50f; ambVig = 1.0f;
    ambBrC[0] = 4.0f; ambBrC[1] = 0.0f; ambBrC[2] = 2.0f;
  } else if (OP.hor == 1) {      // Sombre (par défaut) : environ 35 % plus lumineux que l'Horreur, ciel et décor lisibles, lune bien visible, bords peu assombris
    ambK *= 0.60f; ambRouge *= 0.4f; ambLune *= 0.75f; ambBrume = 1.0f; ambEt = (ambEt > 18) ? 18 : ambEt; ambCol = 0.60f; ambVig = 0.60f;
    ambBrC[0] = 12.0f; ambBrC[1] = 1.0f; ambBrC[2] = 5.0f;
  }
}
static void appliquerSec() { static const float S[3] = {3.4f, 2.2f, 1.5f}; seuilSecousse = S[OP.sec > 2 ? 1 : OP.sec]; }
static float delaiPaume() {      // délai avant la prochaine visite d'un corbeau dans la paume (réglage Visites dans la paume)
  if (!OP.paume) return rnd(2.5f, 4.5f);
  switch (OP.pvis) { case 0: return 1e9f; case 1: return rnd(16.0f, 28.0f); case 3: return rnd(3.0f, 6.0f); default: return rnd(7.0f, 13.0f); }
}
static float delaiVisite(bool premiere) {
  switch (OP.vis) { case 0: return 1e9f; case 1: return premiere ? rnd(60.0f, 110.0f) : rnd(180.0f, 300.0f); default: return premiere ? rnd(25.0f, 45.0f) : rnd(50.0f, 95.0f); }
}
static inline void vib(int ms) { if (OP.vib) vibrer(ms); }
// fréquence des événements : l'activité (1 calme à 5 agité) accélère ou ralentit les minuteurs ; gros plan et œufs ont leur propre réglage
static float actFreq = 1.0f;
static bool cbAbsent = false; static uint32_t cbDepuis = 0;      // absence : personne n'a rien touché depuis 25 s (animations minimales)
static void appliquerAct() { static const float F[5] = {0.45f, 0.7f, 1.0f, 1.5f, 2.2f}; actFreq = F[(OP.act < 1 ? 1 : (OP.act > 5 ? 5 : OP.act)) - 1] * (cbAbsent ? 0.2f : 1.0f); }      // en absence : cinq fois moins d'événements
static float multFreq(uint8_t n) { static const float M[4] = {1e6f, 2.4f, 1.0f, 0.45f}; return M[n > 3 ? 2 : n]; }      // jamais, rare, normale, souvent : multiplicateur du délai
static void chargerOptions() {
  OP.nb = prefs.getUChar("cbnb", 4); if (OP.nb < 1) OP.nb = 1; if (OP.nb > 8) OP.nb = 8; nbMax = OP.nb; { const uint8_t e_ = prefs.getUChar("cbeff", 0); if (e_ > nbMax && e_ <= 8) OP.nb = e_; }      // de 1 à 8 corbeaux, 4 par défaut
  OP.vis = prefs.getUChar("cbvis", 1); if (OP.vis > 2) OP.vis = 1;
  OP.sec = prefs.getUChar("cbsec", 1); if (OP.sec > 2) OP.sec = 1;
  OP.vib = prefs.getUChar("cbvib", 1) ? 1 : 0;
  btnVis = prefs.getUChar("cbbvis", 0) ? true : false;
  OP.amb = prefs.getUChar("cbamb", 0); if (OP.amb > 2) OP.amb = 0;
  OP.fps = 0;      // le compteur d'images/s n'est JAMAIS affiché à l'écran (il reste dans /api/etat : fps, fpsmin, msimage, msmax)
  OP.cad = prefs.getUChar("cbcad", 2); if (OP.cad > 2) OP.cad = 2;      // cadence maximale : 20, 25 ou 30 images/s (défaut 30)
  OP.deg = prefs.getUChar("cbdeg", 1) ? 1 : 0;      // allègement automatique : oui par défaut
  OP.dec = prefs.getUChar("cbdec", 1); if (OP.dec > 2) OP.dec = 1;      // decor : ALLEGE par defaut (reglable : complet / allege / minimal)
  OP.ecr = prefs.getUChar("cbecr", 0) ? 1 : 0;      // écran rapide 80 MHz : non par défaut (à essayer dans Réglages > Fluidité)
  OP.act = prefs.getUChar("cbact", 3); if (OP.act < 1 || OP.act > 5) OP.act = 3;
  OP.gp = prefs.getUChar("cbgp", 2); if (OP.gp > 3) OP.gp = 2;
  OP.oe = prefs.getUChar("cboefq", 2); if (OP.oe > 3) OP.oe = 2;
  OP.hor = prefs.getUChar("cbniv", 1); if (OP.hor > 2) OP.hor = 1;      // lumière : Sombre par défaut (nouvelle clé cbniv)
  OP.q = prefs.getUChar("cbqual", 0); if (OP.q > 2) OP.q = 0;      // qualité : Fluide par défaut (nouvelle clé cbqual)
  OP.barre = prefs.getUChar("cbbarre", 1) ? 1 : 0; OP.fiche = prefs.getUChar("cbfiche", 1) ? 1 : 0; OP.lance = prefs.getUChar("cblance", 1) ? 1 : 0;      // fonctions du bloc W : toutes activées par défaut, réglables
  OP.paume = prefs.getUChar("cbpaume", 1) ? 1 : 0; OP.pvis = prefs.getUChar("cbpvis", 2); if (OP.pvis > 3) OP.pvis = 2; { uint8_t v_ = prefs.getUChar("cbvue2", 255); if (v_ == 255) v_ = prefs.getUChar("cbrecul", 0) ? 4 : 0; OP.recul = (v_ > 5) ? 0 : v_; } OP.cauto = prefs.getUChar("cbcam", 0) ? 1 : 0;      // caméra automatique : Non par défaut (clé cbcam) ;      // vue choisie : 0 normale, 1 reculée, 2 très reculée, 3 arbre et rivière, 4 mare, 5 vieux bâtiments (clé cbvue)      // bloc AA : main paume vers le haut, visites dans la paume, recul de la vue (clés cbpaume, cbpvis, cbrecul)
  OP.tend = prefs.getUChar("cbtend", 1) ? 1 : 0; OP.calin = prefs.getUChar("cbcalin", 1); if (OP.calin > 2) OP.calin = 1;
  OP.ui = prefs.getUChar("cbui", 1) ? 1 : 0;      // 3.1.44 : interface des boutons : 1 nouvelle (defaut), 0 ancienne (cle cbui, un octet)
  OP.taille = prefs.getUChar("cbtaille", 1); if (OP.taille > 2) OP.taille = 1;      // taille : normal par défaut
  nbOeufs = prefs.getUInt("cboeufs", 0);
  appliquerAct(); appliquerAmb(); appliquerSec(); debug = false;
}
// œuf magique
struct Oeuf { bool actif; float x, y, t, tc, tp; uint8_t etat; int8_t ponte; };      // etat : 0 posé, 1 qui craque
static Oeuf OE;
static float gOeuf = 50.0f, agePous = 0.0f;
static uint32_t tTestSon = 0;      // fin du bip de test (volume au maximum)
static float gFeuille = 2.0f, slowT = 0.0f;      // feuilles qui tombent ; ralenti sur les moments forts
static inline void ralenti(float d) { (void)d; }      // 3.1.26 : plus de ralenti sur les moments forts (le temps ne ralentit jamais : Hades veut aucun ralentissement)
static bool nbOeufsSale = false; static uint32_t tOeufsEcr = 0;      // le compteur d'oeufs est ecrit en memoire quelques secondes apres l'oeuf, pas en plein evenement
static float gSocial = 14.0f, tAlerte = 0.0f; static int dirAlerte = 1; static bool alerteFuite = false;
static float lueurOeil = 1.0f, gEcl = 40.0f, eclT = 0.0f, eclX = 200.0f, gBatt = 55.0f, battT = 0.0f;      // horreur : lueur des yeux (instable), éclair rouge lointain, battement de lumière (rares)
static uint32_t eclGraine = 1;
struct Ap2 { int8_t a, b, n; float t; }; static Ap2 AP;      // s'appeler entre eux

static int memTas = 0, memPsram = 0;
static float emaMaj = 0, emaCmp = 0, emaEnt = 0, emaPush = 0, emaCorb = 0, emaTot = 0;      // mesures lissées (ms) : logique, fond, dessin, envoi à l'écran, dont corbeaux
static uint32_t uCorb = 0;      // temps passé à dessiner les corbeaux pendant l'image (µs)
static inline uint32_t periodeMs() { static const uint8_t CAD[3] = {50, 40, 33}; if (cbAbsent) return 200u; return CAD[OP.cad > 2 ? 2 : OP.cad]; }      // en absence : 5 images/s ; sinon cadence maximale choisie (30 images/s par défaut, le dessin se dégrade en douceur s'il n'arrive pas à suivre)
static uint32_t secVu = 0;
static bool ecr80 = false; static uint32_t ecrEssaiFin = 0;      // écran rapide : 80 MHz actif ; fin de l'essai (s'il n'est pas gardé, retour automatique à 40 MHz)
static void vitesseEcran(bool rapide) {      // vitesse d'écriture du bus de l'écran : 40 MHz (d'usine) ou 80 MHz (deux fois plus vite pour envoyer l'image)
  auto* p = M5.Display.getPanel(); if (!p) return;
  auto* b = p->getBus(); if (!b) return;
  b->setClock(rapide ? 80000000u : 40000000u); ecr80 = rapide;
}

struct Rect { int16_t x0, y0, x1, y1; };   // x1, y1 exclus
static int bbx0, bby0, bbx1, bby1;
static uint32_t hAcc = 2166136261u;      // empreinte de ce qui serait dessiné (coordonnées entières et couleurs) : deux images identiques ont la même empreinte, le dessin est alors sauté
static inline void hmix(uint32_t v) { hAcc = (hAcc ^ v) * 16777619u; }
// ENREGISTREMENT : la premiere passe (boites englobantes) calcule deja toutes les coordonnees ; elle les range ici (type, couleur, coordonnees entieres) et la seconde passe les REJOUE sans refaire les calculs de projection, de sinus et de cosinus
struct Cmd { uint8_t t, r; uint16_t c; int16_t a[6]; };      // 16 octets : t 1 triangle, 2 disque, 3 ellipse pleine, 4 ligne, 5 contour d'ellipse
static const int CMDMAX = 4096;
static Cmd* cmdB = nullptr; static int cmdN = 0, cmdMax = 0; static bool cmdOvf = false, recOn = false;
static int cmdD[NENT + 3], cmdF[NENT + 3];      // debut et fin de chaque element (et des trois groupes : nuee, plumes, eclats) dans cmdB
static const int SL_MI = NENT, SL_PL = NENT + 1, SL_FX = NENT + 2;
static inline void cmdPush(uint8_t t, uint16_t c, int a0, int a1, int a2, int a3, int a4 = 0, int a5 = 0) {
  if (!recOn) return;
  if (cmdN >= cmdMax) { cmdOvf = true; return; }
  Cmd& m = cmdB[cmdN++]; m.t = t; m.c = c; m.a[0] = (int16_t)a0; m.a[1] = (int16_t)a1; m.a[2] = (int16_t)a2; m.a[3] = (int16_t)a3; m.a[4] = (int16_t)a4; m.a[5] = (int16_t)a5;
}
static void rejouer(M5Canvas& g, int d, int f) {
  for (int i = d; i < f; i++) {
    const Cmd& m = cmdB[i];
    switch (m.t) {
      case 1: g.fillTriangle(m.a[0], m.a[1], m.a[2], m.a[3], m.a[4], m.a[5], m.c); break;
      case 2: g.fillCircle(m.a[0], m.a[1], m.a[2], m.c); break;
      case 3: g.fillEllipse(m.a[0], m.a[1], m.a[2], m.a[3], m.c); break;
      case 4: g.drawLine(m.a[0], m.a[1], m.a[2], m.a[3], m.c); break;
      default: g.drawEllipse(m.a[0], m.a[1], m.a[2], m.a[3], m.c); break;
    }
  }
}
// NIVEAU DE DETAIL selon la taille a l'ecran : un corbeau de 15 pixels (vue tres reculee) n'a pas besoin de 14 sommets de corps ni de 7 plumes ; invisible a cette taille, jamais en qualite Riche, jamais pour le petit duveteux
static int lodG = 0;
static inline bool dgE() { return qDeg || lodG >= 1; }
static inline bool mnE() { return qMin || lodG >= 2; }
static inline int decLvl() { const int l = qMin ? 2 : (qDeg ? 1 : 0); return (int)OP.dec > l ? (int)OP.dec : l; }      // decor : 0 complet, 1 allege, 2 minimal (reglage Decor, ou allegement automatique)
static inline void bbReset() { bbx0 = 9999; bby0 = 9999; bbx1 = -9999; bby1 = -9999; hAcc = 2166136261u; }
static inline void bbPt(float x, float y) {
  int xi = (int)floorf(x), yi = (int)floorf(y);
  if (xi < bbx0) bbx0 = xi; if (xi > bbx1) bbx1 = xi; if (yi < bby0) bby0 = yi; if (yi > bby1) bby1 = yi;
}
static Rect bbRect() { Rect r; r.x0 = (int16_t)(bbx0 - 2); r.y0 = (int16_t)(bby0 - 2); r.x1 = (int16_t)(bbx1 + 3); r.y1 = (int16_t)(bby1 + 3); return r; }

// ---------- vue : échelle et décalage monde -> écran (1 = vue normale, 0.6 = vue large) ----------
static float zs = 1.0f, ztx = 0.0f, zty = 0.0f;
static const float zax = 160.0f, zay = 128.0f;      // point fixe du zoom
static const float ZN = 1.0f;                      // échelle de la vue normale (la vue large est à 0.6)
static inline float TX(float x) { return x * zs + ztx; }
static inline float TY(float y) { return y * zs + zty; }
static inline float bordG() { return -ztx / zs - 34.0f; }                   // bord gauche de l'écran, en coordonnées du monde, moins une marge
static inline float bordD() { return (320.0f - ztx) / zs + 34.0f; }
struct VueId { float a, b, c; VueId() { a = zs; b = ztx; c = zty; zs = 1.0f; ztx = 0.0f; zty = 0.0f; } ~VueId() { zs = a; ztx = b; zty = c; } };      // dessin d'interface : sans zoom

// ---------- primitives (suivent la boîte englobante ; ne dessinent pas en mode « sec ») ----------
static inline bool coordOk(float a, float b) { return fabsf(a) < 6000.0f && fabsf(b) < 6000.0f; }      // faux pour NaN, infini ou valeur énorme : protège contre les gels
static void tri(M5Canvas& g, float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c) {
  x0 = TX(x0); y0 = TY(y0); x1 = TX(x1); y1 = TY(y1); x2 = TX(x2); y2 = TY(y2);
  if (!coordOk(x0, y0) || !coordOk(x1, y1) || !coordOk(x2, y2)) return;
  bbPt(x0, y0); bbPt(x1, y1); bbPt(x2, y2);
  if (sec) { hmix(0x100000u | (uint16_t)ar(x0)); hmix(((uint32_t)(uint16_t)ar(y0) << 16) | (uint16_t)ar(x1)); hmix(((uint32_t)(uint16_t)ar(y1) << 16) | (uint16_t)ar(x2)); hmix(((uint32_t)(uint16_t)ar(y2) << 16) | c); cmdPush(1, c, ar(x0), ar(y0), ar(x1), ar(y1), ar(x2), ar(y2)); return; }
  g.fillTriangle(ar(x0), ar(y0), ar(x1), ar(y1), ar(x2), ar(y2), c);
}
static void disque(M5Canvas& g, float x, float y, float r, uint16_t c) {
  x = TX(x); y = TY(y); r *= zs; if (!coordOk(x, y) || !(r < 3000.0f)) return;
  bbPt(x - r, y - r); bbPt(x + r, y + r);
  if (sec) { hmix(0x200000u | (uint16_t)ar(x)); hmix(((uint32_t)(uint16_t)ar(y) << 16) | (uint16_t)ar(r)); hmix(c); cmdPush(2, c, ar(x), ar(y), ar(r) < 1 ? 1 : ar(r), 0); return; }
  g.fillCircle(ar(x), ar(y), ar(r) < 1 ? 1 : ar(r), c);
}
static void ell(M5Canvas& g, float x, float y, float rx, float ry, uint16_t c) {
  x = TX(x); y = TY(y); rx *= zs; ry *= zs; if (!coordOk(x, y) || !(rx < 3000.0f) || !(ry < 3000.0f)) return;
  bbPt(x - rx, y - ry); bbPt(x + rx, y + ry);
  if (sec) { hmix(0x300000u | (uint16_t)ar(x)); hmix(((uint32_t)(uint16_t)ar(y) << 16) | (uint16_t)ar(rx)); hmix(((uint32_t)(uint16_t)ar(ry) << 16) | c); { const int a_ = ar(rx), b_ = ar(ry); cmdPush(3, c, ar(x), ar(y), a_ < 1 ? 1 : a_, b_ < 1 ? 1 : b_); } return; }
  int a = ar(rx), b = ar(ry); g.fillEllipse(ar(x), ar(y), a < 1 ? 1 : a, b < 1 ? 1 : b, c);
}
static void ligne(M5Canvas& g, float x0, float y0, float x1, float y1, uint16_t c) {
  x0 = TX(x0); y0 = TY(y0); x1 = TX(x1); y1 = TY(y1); if (!coordOk(x0, y0) || !coordOk(x1, y1)) return;
  bbPt(x0, y0); bbPt(x1, y1);
  if (sec) { hmix(0x400000u | (uint16_t)ar(x0)); hmix(((uint32_t)(uint16_t)ar(y0) << 16) | (uint16_t)ar(x1)); hmix(((uint32_t)(uint16_t)ar(y1) << 16) | c); cmdPush(4, c, ar(x0), ar(y0), ar(x1), ar(y1)); return; }
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
    case T_OEUF: { E(0, 0, 1.0f, 1.25f, C(120, 12, 26)); E(0, 0, 0.84f, 1.1f, C(22, 7, 15)); E(-0.34f, -0.45f, 0.2f, 0.38f, C(222, 62, 84)); } break;
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
  float u = lim((y - STC[k].y) / (STC[k + 1].y - STC[k].y), 0.0f, 1.0f);
  r = lerpf(STC[k].r, STC[k + 1].r, u); g = lerpf(STC[k].g, STC[k + 1].g, u); b = lerpf(STC[k].b, STC[k + 1].b, u);
  r = r * ambK + ambRouge; g *= ambK; b *= ambK;
}
static const float LUNE_X = 274.0f, LUNE_Y = 40.0f, LUNE_R = 20.0f;      // lune petite, dans le coin en haut a droite
struct Crat { float ox, oy, r, dk; };
static const Crat CRAT[6] = {{-6.5f, -4.5f, 4.2f, 0.20f}, {5.5f, 3.0f, 5.0f, 0.16f}, {-2.5f, 8.5f, 3.0f, 0.16f}, {8.0f, -8.0f, 2.6f, 0.14f}, {-10.5f, 4.0f, 2.4f, 0.12f}, {1.5f, -9.5f, 2.2f, 0.10f}};
static void fondRGB(float x, float y, float& r, float& g, float& b) {      // couleur exacte du ciel (halo + lune) en un point
  cielRGB(y, r, g, b);
  float dx = x - LUNE_X, dy = y - LUNE_Y, d = sqrtf(dx * dx + dy * dy);
  float w = expf(-mxf(d - LUNE_R, 0.0f) / 17.0f) * 0.50f * lim(1.0f - (d - LUNE_R) / 58.0f, 0.0f, 1.0f) * ambLune;
  r += 105 * w; g += 13 * w; b += 11 * w;
  if (d < LUNE_R) {
    float ed = d / LUNE_R, sh = 1.0f - 0.20f * lim((dx * 0.45f + dy * 0.75f) / LUNE_R, -1, 1);
    float e3 = ed * ed * ed, f0 = lerpf(1.0f, 0.8f, e3);
    float cr_ = 206 * sh * f0 * ambLune, cg_ = 38 * sh * f0 * ambLune, cb_ = 30 * sh * f0 * ambLune;
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
      float ax = 1.0f - fabsf((x + 2 - cx) / w); ax = ax * ax; float al = lim(a * ambBrume, 0.0f, 0.9f) * ay * ax;
      if (al < 0.03f || x < 0 || x > 316) continue;
      float r, gg, b; fondRGB(x + 2.0f, (float)y, r, gg, b);
      g.drawFastHLine(x, y, 4, CF(r * (1 - al) + ambBrC[0] * al, gg * (1 - al) + ambBrC[1] * al, b * (1 - al) + ambBrC[2] * al));
    }
  }
}
static int bkN = 0;      // compteur des elements d'arriere-plan d'une cuisson (remis a zero au debut de chaque ciel)
static inline bool decOk(int k) { return OP.dec == 0 || (OP.dec == 1 ? (k % 3 != 2) : ((k & 1) == 0)); }      // decor allege : deux elements sur trois ; minimal : un sur deux
static inline void brumeD(M5Canvas& g, float cx, float cy, float w, float h, float a) { if (decOk(bkN++)) brume(g, cx, cy, w, h, a); }
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
  for (int i = 0; i < (ambEt * (3 - OP.dec)) / 3; i++) {      // etoiles : toutes (complet), les deux tiers (allege), le tiers (minimal)
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sx = 3 + (int)((seed >> 8) % 314);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sy = 3 + (int)((seed >> 8) % 100);
    if (hypotf(sx - LUNE_X, sy - LUNE_Y) < LUNE_R + 30) continue;
    int v = (int)((seed >> 4) % 100);
    if (v < 70) g.drawPixel(sx, sy, C(150, 62, 74));
    else if (v < 93) g.drawPixel(sx, sy, C(205, 112, 120));
    else { g.drawPixel(sx, sy, C(240, 170, 170)); g.drawPixel(sx - 1, sy, C(150, 50, 60)); g.drawPixel(sx + 1, sy, C(150, 50, 60)); g.drawPixel(sx, sy - 1, C(150, 50, 60)); g.drawPixel(sx, sy + 1, C(150, 50, 60)); }
  }
  bkN = 0;
  brumeD(g, 230, 70, 100, 3, 0.40f); brumeD(g, 120, 34, 80, 2, 0.35f); brumeD(g, 236, 96, 120, 4, 0.40f);
  brumeD(g, 90, 118, 110, 4, 0.40f); brumeD(g, 230, 160, 130, 5, 0.40f); brumeD(g, 60, 176, 110, 5, 0.40f);
  if (OP.amb == 1) { brumeD(g, 160, 100, 170, 8, 0.7f); brumeD(g, 110, 150, 150, 9, 0.7f); }
  for (int x = 0; x < 320; x++) {
    float yl = 190.0f + 4.0f * sinf(x / 50.0f + 2.0f) + 3.0f * sinf(x / 21.0f);
    g.drawFastVLine(x, (int)yl, 240 - (int)yl, C(46, 6, 11));
    float ys = solHaut((float)x);
    g.drawFastVLine(x, (int)ys, 240 - (int)ys, C(5, 2, 4));
  }
}

static uint16_t coulMembre = 0;      // couleur des branches (0 = le noir de l'arbre de premier plan) ; les arbres lointains sont plus clairs, noyés dans la brume
static void membre(M5Canvas& g, const float (*p)[2], int n, float w0, float w1, bool bouts) {   // branche : spline lissée, largeur qui s'affine
  uint16_t nr = coulMembre ? coulMembre : C(5, 2, 4);
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
  tri(g, 28, 254, 44, 232, 120, 254, nr); tri(g, 44, 232, 100, 232, 120, 254, nr);
  tri(g, 44, 232, 56, 208, 100, 232, nr); tri(g, 56, 208, 86, 208, 100, 232, nr);
  static const float r1[3][2] = {{44, 238}, {24, 246}, {4, 252}}, r2[3][2] = {{94, 240}, {114, 248}, {136, 254}};
  membre(g, r1, 3, 12, 3, true); membre(g, r2, 3, 11, 3, true);
  disque(g, 53, 100, 6, nr); disque(g, 69, 168, 7, nr); disque(g, 57, 206, 8, nr); disque(g, 65, 134, 5, nr);
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
  ell(g, 60, 178, 3, 6, C(110, 10, 18));     // le trou du tronc : on voit le ciel rouge
}

// ---------- arbres lointains : cinq silhouettes différentes (fourche, penché, touffu, conifère mort, saule tombant), chacune plus ou moins dans la brume ----------
static void arbreLoin(M5Canvas& g, int v, float bx, float by, float h, float brume) {      // (bx, by) = pied, h = hauteur, en pixels écran
  float r, gg, b; cielRGB((by - h * 0.5f - zty) / zs, r, gg, b);
  VueId vid_;
  coulMembre = CF(lerpf(5.0f, r, brume), lerpf(2.0f, gg, brume), lerpf(4.0f, b, brume));
  const uint16_t nr = coulMembre;
  float P[8][2]; int n = 0;
  auto pt = [&](float x, float y) { P[n][0] = bx + x * h; P[n][1] = by - y * h; n++; };
  auto br = [&](float w0, float w1) { if (n >= 2) membre(g, P, n, mxf(1.0f, w0 * h), mxf(0.6f, w1 * h), true); n = 0; };
  switch (v % 5) {
    case 0:      // grande fourche
      pt(0, 0); pt(0.01f, 0.25f); pt(-0.02f, 0.5f); pt(0.0f, 0.68f); br(0.06f, 0.025f);
      pt(-0.01f, 0.42f); pt(-0.14f, 0.58f); pt(-0.27f, 0.7f); pt(-0.34f, 0.84f); br(0.035f, 0.012f);
      pt(0, 0.6f); pt(0.1f, 0.76f); pt(0.18f, 0.9f); pt(0.2f, 1.03f); br(0.03f, 0.01f);
      pt(0, 0.68f); pt(-0.05f, 0.86f); pt(-0.04f, 1.0f); br(0.025f, 0.01f);
      pt(0.02f, 0.3f); pt(0.16f, 0.4f); pt(0.27f, 0.5f); br(0.02f, 0.008f); break;
    case 1:      // penché, branches clairsemées
      pt(0, 0); pt(0.05f, 0.25f); pt(0.15f, 0.5f); pt(0.3f, 0.68f); pt(0.42f, 0.78f); br(0.055f, 0.02f);
      pt(0.08f, 0.36f); pt(-0.08f, 0.5f); pt(-0.24f, 0.54f); br(0.03f, 0.01f);
      pt(0.22f, 0.6f); pt(0.3f, 0.85f); pt(0.28f, 1.0f); br(0.03f, 0.01f);
      pt(0.34f, 0.72f); pt(0.5f, 0.74f); pt(0.62f, 0.66f); br(0.025f, 0.01f);
      pt(0.1f, 0.3f); pt(0.22f, 0.28f); pt(0.3f, 0.2f); br(0.02f, 0.008f); break;
    case 2: {    // touffu : une boule de brindilles sur un tronc court
      pt(0, 0); pt(0.01f, 0.25f); pt(0, 0.42f); br(0.08f, 0.045f);
      static const float T[7][2] = {{-0.34f, 0.62f}, {-0.22f, 0.82f}, {-0.08f, 0.94f}, {0.08f, 0.96f}, {0.22f, 0.84f}, {0.34f, 0.66f}, {0.42f, 0.5f}};
      for (int i = 0; i < 7; i++) { pt(0, 0.42f); pt(T[i][0] * 0.5f + 0.03f * ((i & 1) ? 1 : -1), 0.42f + (T[i][1] - 0.42f) * 0.55f); pt(T[i][0], T[i][1]); br(0.03f, 0.008f); }
    } break;
    case 3:      // conifère mort : tronc droit et étages de branches
      pt(0, 0); pt(0, 0.5f); pt(0, 1.0f); br(0.05f, 0.012f);
      for (int i = 0; i < 5; i++) { float y = 0.22f + 0.15f * i, w = 0.30f - 0.05f * i;
        pt(0, y); pt(-w * 0.6f, y - 0.04f); pt(-w, y - 0.13f); br(0.02f, 0.006f); pt(0, y); pt(w * 0.6f, y - 0.04f); pt(w, y - 0.13f); br(0.02f, 0.006f); }
      break;
    default:     // saule tombant
      pt(0, 0); pt(-0.02f, 0.3f); pt(0.02f, 0.55f); br(0.07f, 0.03f);
      pt(0.02f, 0.55f); pt(0.16f, 0.7f); pt(0.3f, 0.66f); pt(0.36f, 0.4f); br(0.03f, 0.008f);
      pt(0.02f, 0.55f); pt(-0.14f, 0.72f); pt(-0.28f, 0.64f); pt(-0.34f, 0.38f); br(0.03f, 0.008f);
      pt(0.02f, 0.55f); pt(0.04f, 0.8f); pt(0.14f, 0.88f); pt(0.2f, 0.7f); br(0.025f, 0.008f); break;
  }
  tri(g, bx - 0.12f * h, by + 2, bx, by - 0.06f * h, bx + 0.12f * h, by + 2, nr);      // pied du tronc
  coulMembre = 0;
}
static const uint8_t LANCE[6] = {T_KUNAI, T_SHURIKEN, T_BAGUE, T_PARCHEMIN, T_BRILLANT, T_OEUF};      // objets que l'on peut lancer (interface ancienne)
static inline uint8_t lanceTyp(int i) { static const uint8_t LN[5] = {T_KUNAI, T_SHURIKEN, T_BAGUE, T_PARCHEMIN, T_OEUF}; return OP.ui ? LN[i < 0 ? 0 : (i > 4 ? 4 : i)] : LANCE[i < 0 ? 0 : (i > 5 ? 5 : i)]; }      // 3.1.44 : interface nouvelle = 5 objets, sans l'objet brillant (il reste dans la nourriture)
static inline int lanceVieux(int i) { return (OP.ui && i >= 4) ? 5 : i; }      // indice du texte d'aide (NL / EL) de l'objet
static void iconeMode(M5Canvas& g, int m, int cx, int cy, uint16_t col) {
  switch (m) {
    case 0: dessinerObjet(g, T_GRAINE, (float)cx - 3, (float)cy + 2, 5.0f, 0.0f, 0.0f); dessinerObjet(g, T_BAIES, (float)cx + 4, (float)cy - 3, 4.0f, 0.0f, 0.0f); break;      // nourrir
    case 1:      // main
      g.fillRoundRect(cx - 5, cy - 1, 10, 8, 2, col);
      for (int i = 0; i < 4; i++) g.fillRoundRect(cx - 5 + i * 3, cy - 8 + ((i == 1 || i == 2) ? 0 : 2), 2, 8, 1, col);
      g.fillTriangle(cx - 5, cy + 1, cx - 9, cy - 2, cx - 5, cy + 5, col);
      break;
    case 2: g.drawCircle(cx, cy, 6, col); g.drawFastHLine(cx - 9, cy, 18, col); g.drawFastVLine(cx, cy - 9, 18, col); g.fillCircle(cx, cy, 1, col); break;      // lancer : viseur
    case 3:      // appel : des ondes
      g.fillCircle(cx - 7, cy, 2, col);
      for (int k = 1; k <= 3; k++) { float r = 2.0f + 3.2f * k; for (int a = -2; a < 2; a++) { float a0 = a * 0.4f, a1 = (a + 1) * 0.4f; g.drawLine((int)(cx - 7 + r * cosf(a0)), (int)(cy + r * sinf(a0)), (int)(cx - 7 + r * cosf(a1)), (int)(cy + r * sinf(a1)), col); } }
      break;
    default: g.drawLine(cx, cy + 8, cx, cy + 1, col); g.drawLine(cx, cy + 1, cx - 6, cy - 7, col); g.drawLine(cx, cy + 1, cx + 6, cy - 7, col); g.drawLine(cx - 6, cy - 7, cx + 6, cy - 7, col); g.fillCircle(cx, cy - 5, 2, col); break;      // fronde
  }
}
// ---------- 3.1.44 : interface des boutons : une seule famille de pilules sombres (fond presque noir, filet rouge sombre, pictogrammes creme) ----------
// Tout est dessine dans le fond en cache (haut : 24 px, bas : 34 px) ; seul l'appui, le changement de mode ou de stock redessine la zone concernee.
static void bnPilule(M5Canvas& g, int x, int y, int w, int h, int r, bool on, bool pr) {
  if (on) { g.drawRoundRect(x - 2, y - 2, w + 4, h + 4, r + 2, C(46, 9, 13)); g.drawRoundRect(x - 1, y - 1, w + 2, h + 2, r + 1, C(92, 17, 23)); }      // petit halo rouge autour du bouton actif
  g.fillRoundRect(x, y, w, h, r, pr ? C(60, 14, 20) : (on ? C(40, 9, 15) : C(14, 5, 10)));
  g.drawRoundRect(x, y, w, h, r, pr ? C(255, 150, 120) : (on ? C(222, 66, 56) : C(104, 20, 26)));
}
static void bnIcone(M5Canvas& g, int id, int cx, int cy, uint16_t col, uint16_t bg) {      // pictogrammes de modes : 0 nourrir (epi), 1 main, 2 lancer (etoile de lancer), 3 voix (ondes)
  switch (id) {
    case 0:
      g.fillRoundRect(cx - 8, cy + 2, 17, 6, 3, col);      // bol
      g.fillCircle(cx - 4, cy - 3, 2, col); g.fillCircle(cx + 1, cy - 5, 2, col); g.fillCircle(cx + 5, cy - 2, 2, col);      // graines
      break;
    case 1:
      g.fillRoundRect(cx - 5, cy - 1, 10, 8, 2, col);
      for (int i = 0; i < 4; i++) g.fillRoundRect(cx - 5 + i * 3, cy - 8 + ((i == 1 || i == 2) ? 0 : 2), 2, 8, 1, col);
      g.fillTriangle(cx - 5, cy + 1, cx - 9, cy - 2, cx - 5, cy + 5, col);
      break;
    case 2:
      g.fillTriangle(cx, cy - 8, cx - 2, cy - 2, cx + 2, cy - 2, col); g.fillTriangle(cx + 8, cy, cx + 2, cy - 2, cx + 2, cy + 2, col);
      g.fillTriangle(cx, cy + 8, cx + 2, cy + 2, cx - 2, cy + 2, col); g.fillTriangle(cx - 8, cy, cx - 2, cy + 2, cx - 2, cy - 2, col);
      g.fillRect(cx - 2, cy - 2, 5, 5, col); g.fillCircle(cx, cy, 1, bg);
      break;
    default:
      g.fillCircle(cx - 6, cy, 2, col);
      for (int k = 1; k <= 3; k++) { const float r = 2.0f + 3.4f * k; for (int a = -2; a < 2; a++) { const float a0 = a * 0.4f, a1 = (a + 1) * 0.4f; g.drawLine((int)(cx - 6 + r * cosf(a0)), (int)(cy + r * sinf(a0)), (int)(cx - 6 + r * cosf(a1)), (int)(cy + r * sinf(a1)), col); } }
      break;
  }
}
static void bnIconeHaut(M5Canvas& g, int kind, int cx, int cy, uint16_t col, uint16_t bg) {      // pictogrammes du haut : 0 menu, 1 reglages (rouage), 2 visiteur, 3 vue (oeil)
  switch (kind) {
    case 0: g.fillRoundRect(cx - 5, cy - 5, 11, 2, 1, col); g.fillRoundRect(cx - 5, cy - 1, 11, 2, 1, col); g.fillRoundRect(cx - 5, cy + 3, 11, 2, 1, col); break;
    case 1: {
      static const int8_t D8[8][2] = {{6, 0}, {-6, 0}, {0, 6}, {0, -6}, {4, 4}, {-4, 4}, {4, -4}, {-4, -4}};
      g.fillCircle(cx, cy, 4, col); for (int i = 0; i < 8; i++) g.fillRect(cx + D8[i][0] - 1, cy + D8[i][1] - 1, 3, 3, col);
      g.fillCircle(cx, cy, 2, bg);
    } break;
    case 2: g.fillCircle(cx, cy - 3, 3, col); g.fillRoundRect(cx - 5, cy + 1, 11, 5, 2, col); break;
    default:
      g.fillTriangle(cx - 7, cy, cx, cy - 4, cx + 7, cy, col); g.fillTriangle(cx - 7, cy, cx, cy + 4, cx + 7, cy, col);
      g.fillCircle(cx, cy, 3, bg); g.fillCircle(cx, cy, 1, col);
      break;
  }
}
static inline int hautN() { return btnVis ? 4 : 3; }      // boutons du haut : Menu | Reglages | Vue (le bouton Visiteur, masque par defaut, s'intercale)
static void hautPilule(M5Canvas& g, int slot, int kind, const char* lab) {
  const int n = hautN(), w = (308 - 7 * (n - 1)) / n, x = 6 + slot * (w + 7);
  bnPilule(g, x, 1, w, 22, 10, false, false);
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_left); g.setTextColor(C(236, 224, 206));
  const int tw = g.textWidth(lab), tot = 14 + 5 + tw; const bool ico = (tot <= w - 8);
  const int sx = x + (w - (ico ? tot : tw)) / 2;
  if (ico) { bnIconeHaut(g, kind, sx + 7, 12, C(236, 224, 206), C(14, 5, 10)); g.drawString(lab, sx + 19, 12); } else g.drawString(lab, sx, 12);
  g.setTextDatum(top_left);
}
static void barreCbN(M5Canvas& g, int pr) {      // barre du bas : 4 modes (Nourrir, Main + fronde, Lancer, Voix = appel + effrayer) puis la nourriture (6) ou les objets (5)
  static const char* const LAB[4] = {"NOURRIR", "MAIN", "LANCER", "VOIX"};
  g.fillRect(0, H_SCENE, 320, 34, C(5, 2, 4));
  for (int m = 0; m < 4; m++) {
    const int x = 4 + m * 48; const bool on = (m == modeCb);
    bnPilule(g, x, 208, 46, 28, 9, on, false);
    bnIcone(g, m, x + 23, 217, on ? C(255, 240, 222) : C(196, 176, 160), on ? C(40, 9, 15) : C(14, 5, 10));
    g.setFont(&fonts::Font0); g.setTextDatum(middle_center); g.setTextColor(on ? C(255, 240, 222) : C(176, 156, 142)); g.drawString(LAB[m], x + 23, 230);
  }
  g.drawFastVLine(199, 212, 20, C(70, 13, 18));
  if (modeCb == 0 || modeCb == 2) {
    const int n = (modeCb == 0) ? 6 : 5, cur = (modeCb == 0) ? choix : choixL;
    for (int i = 0; i < n; i++) {
      const int x0 = 203 + (114 * i) / n, x1 = 203 + (114 * (i + 1)) / n, w = x1 - x0 - 2, cx = x0 + w / 2;
      const int typ = (modeCb == 0) ? i : lanceTyp(i);
      bnPilule(g, x0, 208, w, 28, 8, i == cur, i == pr);
      dessinerObjet(g, typ, (float)cx, (modeCb == 0) ? 217.0f : 222.0f, 5.0f, 0.0f, 0.0f);
      if (modeCb == 0 && stIdx(typ) >= 0) {      // combien il en reste (viande, noix : rares, le stock se renouvelle)
        const int n_ = (int)stK[stIdx(typ)]; char nb_[4]; snprintf(nb_, sizeof(nb_), "%d", n_);
        g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(n_ ? C(255, 240, 222) : C(255, 96, 84)); g.drawString(nb_, cx, 230);
      }
    }
  } else {
    bnPilule(g, 203, 208, 114, 28, 12, false, false);
    g.setFont(&fonts::Font0); g.setTextDatum(middle_center); g.setTextColor(C(222, 202, 186));
    g.drawString(modeCb == 1 ? "Tiens, caresse" : "Touche : appel", 260, 217); g.drawString(modeCb == 1 ? "Tire : fronde" : "Balaie : fuite", 260, 228);
  }
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(top_left);
}
static void barreCb(M5Canvas& g, int sel, int pr) {   // barre du bas : cinq modes à gauche, le choix d'objet (ou de geste) à droite (interface ancienne ; la nouvelle : barreCbN)
  (void)sel; VueId vid_;
  if (OP.ui) { barreCbN(g, pr); return; }
  g.fillRect(0, H_SCENE, 320, 34, C(5, 2, 4));
  g.fillRoundRect(2, 208, 316, 30, 10, C(16, 5, 9)); g.drawRoundRect(2, 208, 316, 30, 10, C(96, 18, 26));
  for (int m = 0; m < 5; m++) {
    int x = 5 + m * 34; bool on = (m == modeCb);
    g.fillRoundRect(x, 210, 31, 26, 7, on ? C(110, 24, 30) : C(28, 8, 14)); g.drawRoundRect(x, 210, 31, 26, 7, on ? C(255, 120, 90) : C(80, 16, 24));
    iconeMode(g, m, x + 15, 223, on ? C(255, 236, 220) : C(190, 150, 140));
  }
  g.drawFastVLine(177, 212, 22, C(96, 18, 26));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160));
  if (modeCb == 0 || modeCb == 2) {
    for (int i = 0; i < 6; i++) {
      int cx = 192 + i * 22, cy = 223, typ = (modeCb == 0) ? i : LANCE[i], cur = (modeCb == 0) ? choix : choixL;
      if (i == cur) { g.fillCircle(cx, cy, 11, C(70, 16, 20)); g.drawCircle(cx, cy, 11, C(214, 60, 52)); }
      if (i == pr) g.drawCircle(cx, cy, 9, C(255, 230, 200));
      dessinerObjet(g, typ, (float)cx, (float)cy, 5.2f, 0.0f, 0.0f);
      if (modeCb == 0 && stIdx(typ) >= 0) {      // 3.1.38 : combien il en reste (viande, noix : rares, le stock se renouvelle)
        const int n_ = (int)stK[stIdx(typ)]; char nb_[4]; snprintf(nb_, sizeof(nb_), "%d", n_);
        g.fillCircle(cx + 7, cy + 7, 5, n_ ? C(60, 14, 18) : C(120, 20, 20)); g.drawCircle(cx + 7, cy + 7, 5, n_ ? C(170, 60, 52) : C(255, 90, 80)); g.setTextColor(n_ ? C(255, 236, 220) : C(255, 200, 190)); g.drawString(nb_, cx + 7, cy + 7); g.setTextColor(C(230, 170, 160));
      }
    }
  } else if (modeCb == 3) {
    for (int i = 0; i < 2; i++) {
      int x = 182 + i * 68; bool on = (i == appelSub);
      g.fillRoundRect(x, 211, 64, 24, 6, on ? C(110, 24, 30) : C(28, 8, 14)); g.drawRoundRect(x, 211, 64, 24, 6, on ? C(255, 120, 90) : C(80, 16, 24));
      g.setTextColor(on ? C(255, 236, 220) : C(190, 150, 140)); g.drawString(i == 0 ? "Appeler" : "Effrayer", x + 32, 223);
    }
  } else {
    g.drawString(modeCb == 1 ? "Main : tiens, glisse" : "Fronde : tire, lâche", 250, 217); g.drawString(modeCb == 1 ? "un corbeau, balaie" : "un corbeau en arrière", 250, 230);
  }
  g.setTextDatum(top_left);
}
static void menuCb(M5Canvas& g) {
  if (OP.ui) { hautPilule(g, 0, 0, "Menu"); return; }
  g.fillRoundRect(2, 2, 46, 20, 8, C(14, 4, 8)); g.drawRoundRect(2, 2, 46, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160)); g.drawString("Menu", 25, 12); g.setTextDatum(top_left);
}

static void boutonSons(M5Canvas& g) {
  if (OP.ui) { hautPilule(g, 1, 1, "Réglages"); return; }
  g.fillRoundRect(52, 2, 86, 20, 8, C(14, 4, 8)); g.drawRoundRect(52, 2, 86, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160));
  g.drawString("Réglages", 95, 12); g.setTextDatum(top_left);
}
static void boutonVisiteur(M5Canvas& g) {
  if (!btnVis) return;
  if (OP.ui) { hautPilule(g, 2, 2, "Visiteur"); return; }
  g.fillRoundRect(176, 2, 58, 20, 8, C(14, 4, 8)); g.drawRoundRect(176, 2, 58, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160)); g.drawString("Visiteur", 205, 12); g.setTextDatum(top_left);
}

static void boutonVue(M5Canvas& g) {
  if (OP.ui) { hautPilule(g, hautN() - 1, 3, mo::etiquette()); return; }
  g.fillRoundRect(238, 2, 32, 20, 8, C(14, 4, 8)); g.drawRoundRect(238, 2, 32, 20, 8, C(110, 22, 30));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(230, 170, 160)); g.drawString(mo::etiquette(), 254, 12); g.setTextDatum(top_left);
}
static void cielLarge(M5Canvas& g);
static void vignette(M5Canvas& g) {      // horreur : bords noirs (dessiné une seule fois dans le fond, rien à recalculer à chaque image)
  if (ambVig <= 0.0f || bpp != 2) return;
  uint8_t* b = (uint8_t*)g.getBuffer(); if (!b) return;
  g.drawPixel(0, 0, C(248, 0, 0)); const bool fort = (b[0] == 0xF8); g.drawPixel(0, 0, C(0, 0, 0));      // ordre des octets : détecté par un essai
  for (int y = 0; y < H_SCENE; y++) {
    float dy = (y - 100.0f) / 106.0f;
    for (int x = 0; x < 320; x++) {
      float dx = (x - 160.0f) / 165.0f, d2 = dx * dx + dy * dy;
      if (d2 < 0.35f) continue;
      float t = lim((d2 - 0.35f) / 1.15f, 0.0f, 1.0f), f = 1.0f - 0.93f * ambVig * t * t * (3.0f - 2.0f * t);
      int k = (int)(f * 256.0f); uint8_t* p = b + ((size_t)y * 320 + x) * 2;
      uint16_t v = fort ? (uint16_t)((p[0] << 8) | p[1]) : (uint16_t)((p[1] << 8) | p[0]);
      uint16_t r5 = (uint16_t)((((v >> 11) & 31) * k) >> 8), g6 = (uint16_t)((((v >> 5) & 63) * k) >> 8), b5 = (uint16_t)(((v & 31) * k) >> 8);
      v = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
      if (fort) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)(v & 255); } else { p[1] = (uint8_t)(v >> 8); p[0] = (uint8_t)(v & 255); }
    }
  }
}
static void construireFond() {
  { VueId v_; zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); cielLarge(fond); arbre(fond); mo::decorNormal(fond); vignette(fond); } mo::rebake();      // les arbres lointains sont dessinés dans cielLarge, derrière les collines
  menuCb(fond); boutonSons(fond); boutonVisiteur(fond); boutonVue(fond); barreCb(fond, choix, -1);
  cOk = false; nEnCours = false;      // le paysage en cache de la caméra n'est plus valable (ambiance ou décor changés)
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
enum { A_REPOS = 0, A_IMMO, A_REGARDE, A_LISSE_DOS, A_LISSE_POIT, A_GRATTE, A_SECOUE, A_ETIRE, A_DORT, A_APPEL };
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
  bool rare; float duvet;                             // petit corbeau : œil doré, duvet
  float ocx, ocy, tOrb, pCal, pBav, pJou, pCra, hum; bool ocg;      // orbite locale ; personnalité et humeur
  float ro, rov;                                      // roulis du corps (ressort)
  float sq; Aile poseP;                               // écrasement / étirement ; pose d'aile de l'image précédente (flou de mouvement)
  int8_t pj;                                          // objet lancé poursuivi
  float tq, plume, wd, ha0;                           // posture perchée : chute de la queue, longueur de la plume dominante, aile qui pend, angle de repos de la tête
  uint8_t objet;
  float kx, ky;                                       // vitesse de choc (contacts entre corbeaux, objets lancés) : s'ajoute au mouvement, puis s'amortit
  Aile pose, poseC;
};
struct Obj { bool actif; uint8_t type; float x, y, vy, t; bool repos; int8_t reserve; };
struct Plume { float x, y, vx, vy, rot, vr, vie, dur; bool actif; };
struct Eclat { float x, y, t, dur, taille; bool actif; };

static Corbeau cr[NCB];
static bool natAile(int k); static bool natPatte(int k); static int natSurplus(); static uint8_t natSante(int k);      // nature.h (3.1.41)
namespace mo { static bool partir(Corbeau& c); }
struct VisiteGP { uint8_t ph; float t, dur, sx, sy, ex, ey, th, choc, tCl, cl, tCri, ouv, sc, dy, tChoc; int8_t k, n, nf, mir; bool jeune, impact, cri;
  float px, py, ps, dvv, pit, pk, x0, y0, s0, dv0, p0, ix[3], iy[3]; float fx0, fy0, ha0, hb0; int8_t fis; bool perche; uint32_t gr; };      // px..pit : position, taille, orientation actuelles ; x0..p0 : départ de la sortie ; ix, iy : points d'impact (fissures)      // visite d'un corbeau devant l'écran
static VisiteGP GP; static Corbeau vcb; static float gVisGP = 60.0f; static uint32_t tToucher = 0;
static Obj ob[NOB];
static Plume pl[NPART];
static Eclat fx[NFX];
static Rect nouv[NENT], anc[NENT];
static Rect liste[96]; static int nListe = 0;

// ---------- visiteur mystérieux : silhouette de dos sur la grosse branche, entourée d'une nuée ----------
enum { V_RIEN = 0, V_NUEE, V_PRESENT };
struct Visiteur { uint8_t ph; float t, dur, arm, armC, tArm, flinch, sw, tRecrut; };
static Visiteur V;
static float gVisite = 60.0f;                 // délai avant la prochaine apparition spontanée
static float tAnim = 0.0f;                    // temps de l'image en cours (le même pour les deux passes de dessin)
struct Mini { float x, y, a, r, w, ph, vie, vx, vy; uint8_t mode; bool actif; };      // petit corbeau de la nuée : mode 1 = tourbillon, 2 = envol
static const int NMINI = 18;
static Mini mini[NMINI];
struct ObjM { bool actif; uint8_t type; float u, t; };      // objet posé dans la paume
struct Main_ { bool actif, gauche; float x, y, ext, tx, ty, tPose, vit, tRec; float sp[5], cu[5]; uint8_t pose; ObjM om[3];
  float tHold, la; bool lok; float vx, vy, tilt, tiltV, exs, resp;      // poignet : vitesse, inclinaison (inertie), bout de manche qui glisse, respiration
  float ouv, ouvV, esp, espV, clos, closV, circ;      // gestes en direct : ouverture des doigts, écartement, main qui se referme sur l'oiseau, tour de cercle accumulé
  float spv[5], cuv[5]; bool ctrl; float lx, ly; float pal, palV, dip, dipV, halo; };      // pal : la main se tourne paume vers le haut (0 à 1) ; dip : la paume cède quand les pattes se posent ; halo : intensité de l'effet d'offrande      // un ressort par articulation de doigt ; le doigt de l'utilisateur pilote la main      // la main tendue (mode Main)
struct Trace { float x[48], y[48]; int n; float len; uint32_t t0; };      // tracé du doigt (reconnaissance de formes)
static Main_ H;
struct Tendre { float car, serre, cal, yeux, calT, sqT, carHold, tCal, tSon, coeurT, coeurX, coeurY; int8_t k; bool sqFait; };      // 3.1.39 : tendresse avec l'oiseau posé dans la paume : caresse (car), serrer doucement (serre), câlin sur les doigts (cal), yeux fermés (yeux)
static Tendre TD = {0, 0, 0, 0, 0, 0, 0, 10.0f, 0, 0, 0, 0, -1, false};
struct Braise { bool on; float x, y, vx, vy, t, dur; }; static Braise BRAIS[6]; static float tBraise = 0.0f;      // braises qui montent de la paume
static Trace TR; static uint32_t tLigne = 0; static float aLigne = 0;
struct Proj { bool actif; uint8_t type; float x, y, vx, vy, rot, vr, t, ox; int8_t dern; };      // dern : dernier corbeau heurté (pour ne pas le frapper à chaque image)      // objet lancé (mode Lancer)
static const int NPROJ = 3; static Proj proj[NPROJ];
struct Viser { bool on; float x0, y0, x, y; }; static Viser VS; static int8_t visType = -1; static int8_t cibleK = -1; static uint32_t cibleT = 0;      // cibleK : corbeau désigné par son nom pour recevoir la prochaine nourriture ou le prochain objet lancé      // visType : nourriture visée (mode Nourrir), -1 = objet du mode Lancer
struct Fronde { bool on; int8_t c; float ax, ay; }; static Fronde FR; static float tFrImm = 0.0f; static bool frTendu = false;      // 3.1.43 : tFrImm = apres un corbeau projete sur un autre corbeau, pas de blessure par collision pendant la poursuite ; frTendu = le bruit de tension de la fronde a deja ete joue
struct Doigt { bool bas, grab; float x0, y0, x, y, vx, vy; uint32_t t0, tp; int8_t cible; uint8_t type; bool fait; };
static Doigt D;
static bool iconDrag = false; static float dragX = 0.0f, dragY = 0.0f;      // 3.1.42 : une icone de nourriture glissee depuis la barre vers la paume
static int8_t mgK = -1; static float mgT = 0.0f;      // 3.1.42 : corbeau qui mange la nourriture posee dans la paume (index, temps)
static uint8_t tpCible = 0; static bool tpFait = false; static uint32_t tpT0 = 0; static float tpX = 0;
static bool appelOn = false;                  // un appel est en cours : les corbeaux tournent autour du doigt
static float orbCx = 166.0f, orbCy = 90.0f;   // centre de la nuée (le visiteur ou le doigt)
static const float VX = 166.0f;               // où il se tient sur la branche
static inline float visPiedsY() { return ytop(VX) + 0.4f; }
struct RepM { float hx, hy, ux, uy, S, ex, ey, mx; bool g; };
static RepM repereMain() {      // repère de la main : le bras (manche et avant-bras) est FIXE ; la paume pivote autour du poignet ; calculée comme une main gauche puis retournée pour le côté droit
  RepM r; r.g = H.gauche; r.S = 2.30f;      // 3.1.38 : grosse paume (3.1.26 : 1,37 ; la main était plus petite que l'oiseau posé dessus) : l'oiseau entier se voit bien
   float e = pas(H.ext);
  r.ex = H.exs; r.ey = 262.0f;
  const float hx0 = lerpf(r.ex, H.x, e), hy0 = lerpf(r.ey, H.y + 0.6f * H.resp, e);      // position de repos de la paume (la main sort du bas de l'écran)
  float a0 = lim(atan2f(r.ey - hy0, fabsf(hx0 - r.ex)), 0.15f, 1.3f);
  const float angR = -(0.06f + 0.26f * a0) - 0.15f, cr_ = cosf(angR), sr_ = sinf(angR), L0 = 8.5f * r.S;      // 3.1.42 : la main est un peu plus relevee (doigts vers le haut)
  const float wxs = hx0 - L0 * cr_, wys = hy0 - L0 * sr_;      // le poignet ne bouge jamais
  float tl = lim(H.tilt, -0.7f, 0.7f); const float ang = angR * (1.0f - 0.45f * lim(H.pal, 0.0f, 1.0f)) + (r.g ? tl : -tl);      // inclinaison du poignet (positive = vers le bas de l'écran)
  r.ux = cosf(ang); r.uy = sinf(ang); r.hx = wxs + L0 * r.ux; r.hy = wys + L0 * r.uy; r.mx = hx0;
  return r;
}
static inline void ptMain(const RepM& r, float u, float v, float& X, float& Y) {
  float X0 = r.hx + (u * r.ux - v * r.uy) * r.S; Y = r.hy + (u * r.uy + v * r.ux) * r.S; X = r.g ? X0 : 2.0f * r.mx - X0;
}
static const float PAUME_VK = 0.62f;      // la paume vue de profil : sa hauteur à l'écran est réduite à 62 % (3.1.38 : 55 % avant)
static inline void ptPaume(const RepM& r, float& X, float& Y) {      // le creux de la paume (main tournée vers le haut) : c'est là que les pattes se posent
  ptMain(r, -0.85f, 0.7f, X, Y); Y += 0.9f + H.dip;
}
static void ptVis(int k, float& x, float& y) {      // points d'appui des corbeaux : 0 épaule gauche, 1 épaule droite, 2 dessus de la tête, 3 main du bras tendu
  float y0 = visPiedsY();
  switch (k) {
    case 0: x = VX - 10.0f + V.sw * 0.9f; y = y0 - 47.0f; break;
    case 1: x = VX + 10.0f + V.sw * 0.9f; y = y0 - 47.0f; break;
    case 2: x = VX + V.sw; y = y0 - 65.5f; break;
    case 4: { RepM r_ = repereMain(); float ax, ay, bx, by; ptMain(r_, -1.0f, -7.0f, ax, ay); ptPaume(r_, bx, by); const float p_ = pas(H.pal); x = lerpf(ax, bx, p_); y = lerpf(ay, by, p_); } break;      // la main tendue : le creux de la paume quand elle est tournée vers le haut      // la main tendue du mode Main
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
static uint8_t oeilActif = 0;                               // 0 = œil rouge normal ; 1 à 26 = œil du Dex ; 27 = ton œil
static uint16_t oeilAnn = 0, oeilCoeur = 0;
static void teinteRVB(float h, float& r, float& g, float& b) {      // teinte 0..255 -> couleur vive
  float x = h * 6.0f / 256.0f; int i = (int)x; float f = x - i, q = 1.0f - f;
  switch (i % 6) { case 0: r = 1; g = f; b = 0; break; case 1: r = q; g = 1; b = 0; break; case 2: r = 0; g = 1; b = f; break; case 3: r = 0; g = q; b = 1; break; case 4: r = f; g = 0; b = 1; break; default: r = 1; g = 0; b = q; break; }
  r *= 255; g *= 255; b *= 255;
}
static void oeilRVB(int idx, float* c) {      // c = anneau r,g,b puis cœur r,g,b (table PALOEIL : oeil_pal.h)
  if (idx < 0 || idx > NB_OEILS) idx = 0;
  for (int i = 0; i < 6; i++) c[i] = PALOEIL[idx][i];
  if (idx == NB_OEILS) { float r, g, b; teinteRVB(oCu.teinte, r, g, b); c[3] = r; c[4] = g; c[5] = b; c[0] = r * 0.45f; c[1] = g * 0.45f; c[2] = b * 0.45f; }
}
static void appliquerOeil() { float c[6]; oeilRVB(oeilActif, c); oeilAnn = CF(c[0], c[1], c[2]); oeilCoeur = CF(c[3], c[4], c[5]); }
static void chargerOeil() {
  oeilActif = prefs.getUChar("cbeil2", 255);
  if (oeilActif == 255) {      // première fois avec le Dex complet : on convertit l'ancien choix (1..8) vers les nouveaux numéros
    static const uint8_t ANC[9] = {0, 3, 5, 6, 18, 23, 25, 10, NB_OEILS};
    uint8_t v = prefs.getUChar("cbeil", 0); oeilActif = ANC[v > 8 ? 0 : v]; prefs.putUChar("cbeil2", oeilActif);
  }
  if (oeilActif > NB_OEILS) oeilActif = 0;
  if (prefs.getBytesLength("dexcr") == sizeof(OeilCu)) prefs.getBytes("dexcr", &oCu, sizeof(OeilCu));
  if (oeilActif == NB_OEILS && !oCu.ok) oeilActif = 0;
  appliquerOeil();
}
static void choisirOeil(int a) { oeilActif = (uint8_t)a; prefs.putUChar("cbeil2", oeilActif); appliquerOeil(); }
static void sauverOeilCu() { prefs.putBytes("dexcr", &oCu, sizeof(OeilCu)); }

// ---------- sons d'apparition et de disparition : synthèse par code (souffle filtré + claquement + battement d'ailes), 16 kHz ----------
static uint8_t* sonApp = nullptr; static uint8_t* sonDisp = nullptr; static uint32_t nSonApp = 0, nSonDisp = 0;
static float bruitSynth(uint32_t& s) { s = s * 1664525u + 1013904223u; return ((int32_t)(s >> 8) & 0xFFFF) / 32768.0f - 1.0f; }
static uint8_t* fabriquerPop(bool apparait, uint32_t& n) {
  n = apparait ? 8000 : 7200;      // 0,5 s et 0,45 s
  uint8_t* o = (uint8_t*)(psramFound() ? ps_malloc(n) : malloc(n)); if (!o) { n = 0; return nullptr; }
  float* x = (float*)malloc(n * sizeof(float)); if (!x) { free(o); n = 0; return nullptr; }
  uint32_t seed = apparait ? 20240611u : 91827364u, seed2 = seed ^ 0x5bd1e995u;
  float bp = 0, lp = 0, bas = 0, pic = 0.001f;
  const float tc = apparait ? 0.34f : 0.03f, dur = n / 16000.0f;
  for (uint32_t i = 0; i < n; i++) {
    float t = i / 16000.0f, v = 0;
    // 1) souffle : bruit passe-bande dont la fréquence balaie (monte en apparaissant, descend en disparaissant)
    float amp, fc;
    if (apparait) { float u = t / tc; if (u > 1) u = 1; amp = (t < tc) ? 0.9f * powf(u, 1.5f) : 0.9f * expf(-(t - tc) * 40.0f); fc = 450.0f + 2200.0f * powf(u, 1.3f); }
    else { float u = (t - tc) / (dur - tc); if (u < 0) u = 0; amp = (t < tc) ? 0.0f : 0.9f * expf(-(t - tc) * 7.0f); fc = 2400.0f - 2050.0f * powf(u, 0.7f); }
    float f = 2.0f * sinf(3.14159f * fc / 16000.0f), nz = bruitSynth(seed);
    float hp = nz - lp - 0.45f * bp; bp += f * hp; lp += f * bp;
    v += bp * amp * 2.2f;
    // 2) claquement : petit « pop » sourd + craquement bref à l'instant tc
    if (t >= tc) {
      float d = t - tc;
      v += 0.75f * sinf(6.2832f * 170.0f * d) * expf(-d * 38.0f);
      v += 0.35f * sinf(6.2832f * 900.0f * d) * expf(-d * 170.0f);
      v += 0.8f * bruitSynth(seed2) * expf(-d * 420.0f);
    }
    // 3) battements d'ailes : bruit grave haché à 16-18 Hz
    float b = bruitSynth(seed2); bas += 0.12f * (b - bas);
    float fl = apparait ? 16.0f : 18.0f, ph = 6.2832f * fl * t, hach = 0.5f + 0.5f * sinf(ph); hach = hach * hach;
    float ea = apparait ? ((t > 0.04f) ? expf(-fabsf(t - 0.3f) * 6.0f) : 0.0f) : ((t > 0.05f) ? expf(-(t - 0.05f) * 6.0f) : 0.0f);
    v += bas * hach * ea * 3.2f;
    x[i] = v; float a = fabsf(v); if (a > pic) pic = a;
  }
  float g = 0.92f / pic;
  for (uint32_t i = 0; i < n; i++) {
    float v = tanhf(x[i] * g * 2.4f) / tanhf(2.4f); if (i > n - 400) v *= (float)(n - i) / 400.0f;      // compression douce (plus de niveau perçu) + fondu final
    int k = (int)(128.0f + 127.0f * v); o[i] = (uint8_t)(k < 0 ? 0 : (k > 255 ? 255 : k));
  }
  free(x); return o;
}
static const uint8_t* sonPop(bool apparait, uint32_t& n) {
  uint8_t*& b = apparait ? sonApp : sonDisp; uint32_t& nb = apparait ? nSonApp : nSonDisp;
  if (!b) b = fabriquerPop(apparait, nb);
  n = nb; return b;
}

static void son(int id, int vol = 200) {
  if (id < 0 || id >= SN_NB || R.volume == 0) return;
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
  if (id == SN_POP_APP || id == SN_POP_DISP) { uint32_t nn = 0; const uint8_t* s = sonPop(id == SN_POP_APP, nn); if (s && nn) { d = s; n = nn; rate = 16000; } }      // nouveaux sons faits par le code
  if (!d || !n) return;
  uint32_t m = millis(); if (m - dernierSon[id] < 150) return; dernierSon[id] = m;
  if (!M5.Speaker.isRunning()) {
    if (lectureEnCours) return;      // la Matinale demarre : on ne touche pas au haut-parleur
    M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume);
  }
  if (!tTestSon) M5.Speaker.setVolume(R.volume);
  static uint8_t canal = 0; canal = (canal + 1) & 3;
  M5.Speaker.setChannelVolume(canal, vol * 5 / 4 > 255 ? 255 : vol * 5 / 4);
  M5.Speaker.playRaw(d, n, rate, false, 1, canal, true);
}
static String diagSon; static uint32_t tDiagSon = 0;      // résultat du dernier test, affiché dans le panneau de réglages
static void testerSon() {      // bip fort puis cri : pour vérifier que le haut-parleur répond
  bool ouvert_ = M5.Speaker.isRunning();
  if (!lectureEnCours && !ouvert_) { M5.Mic.end(); ouvert_ = M5.Speaker.begin(); }
  diagSon = String(ouvert_ ? "Haut-parleur actif, volume " : "Haut-parleur ABSENT, volume ") + String((int)R.volume); if (lectureEnCours) diagSon = "Matinale en cours : coupe-la d'abord"; tDiagSon = millis() + 5000; panSale = true;
  M5.Speaker.setVolume(255); tTestSon = millis() + 1800; if (!tTestSon) tTestSon = 1;
  M5.Speaker.tone(1000.0f, 160);
  dernierSon[SN_CROA1] = 0; son(SN_CROA1, 255);
}
static void sonCri() { static const int ids[7] = {SN_CROA1, SN_CROA2, SN_CROA3, SN_CROASSE1, SN_CROASSE2, SN_ALARME, SN_TETON}; int n = sonsReels ? 7 : 5; son(ids[random(n)], 190); }

// ---------- effets ----------
static void plumes(float x, float y, int n) {
  if (OP.q == 0) n = (n + 1) / 2;
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
  hx = dv * s * (0.8f + 1.8f * sinf(pitch)); hy = s * (4.9f + 0.4f * sinf(pitch));
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
struct Rep { float cx, cy, d, s, ct, st, sx = 1.0f, sy = 1.0f, sn = 0.0f, ro = 0.0f, cro = 1.0f, sro = 0.0f;
  // projection 3D légère : (u, v, w) = (avant, bas, côté) ; sn = sinus du demi-tour (0 = de profil), ro = roulis ; sn = 0 et ro = 0 redonnent exactement le dessin de profil
  void q(float u, float v, float w, float& X, float& Y) const {
    float uu = u * ct + v * st, vv = -u * st + v * ct, v2 = vv, w2 = w;
    if (ro != 0.0f) { v2 = vv * cro - w * sro; w2 = w * cro + vv * sro; }
    float x = s * sx * (uu * d - w2 * sn), y = s * sy * v2;
    if (sn > 0.001f) { float k = 1.0f + 0.012f * (uu * sn + w2 * d) * sn; x *= k; y *= k; }
    X = cx + x; Y = cy + y;
  }
  void p(float u, float v, float& X, float& Y) const { q(u, v, 0.0f, X, Y); }
  void q0(float u, float v, float w, float& X, float& Y) const {      // 3.1.32 : idem q sans roulis = position de reference du corps (le corps lui-meme ne roule pas)
    float uu = u * ct + v * st, vv = -u * st + v * ct, x = s * sx * (uu * d - w * sn), y = s * sy * vv;
    if (sn > 0.001f) { float k = 1.0f + 0.012f * (uu * sn + w * d) * sn; x *= k; y *= k; }
    X = cx + x; Y = cy + y;
  }
  void soude(float u, float v, float w, float& dx, float& dy) const {      // decalage a ajouter aux points roules d'une aile pour que sa racine (u, v, w) reste collee a l'epaule du corps non roule
    dx = 0.0f; dy = 0.0f; if (ro == 0.0f) return; float ax, ay, bx, by; q(u, v, w, ax, ay); q0(u, v, w, bx, by); dx = bx - ax; dy = by - ay;
  }
  void pw(float u, float v, float wf, float& X, float& Y) const { float uu = u * ct + v * st, vv = -u * st + v * ct; X = cx + s * sx * uu * wf; Y = cy + s * sy * vv; }      // largeur propre (corps, poitrine)
};
static void polyS(M5Canvas& g, const float* p, int n, uint16_t c) {      // éventail depuis le premier point (pour les formes en étoile)
  for (int i = 1; i < n - 1; i++) tri(g, p[0], p[1], p[i * 2], p[i * 2 + 1], p[i * 2 + 2], p[i * 2 + 3], c);
}
static const float EPAULE = 4.2f;      // 3.1.32 : ecart lateral de l'epaule (aile ouverte et aile repliee partent du meme point : pas de saut au decollage)
static void dirAile(float e, float sw, float& du, float& dv) {
  float er = e * 0.0174533f; float a = -sw * cosf(er), b = -sinf(er); float l = sqrtf(a * a + b * b); if (l < 0.001f) l = 1; du = a / l; dv = b / l;
}
static bool detG = false; static uint16_t detC = 0;      // 3.1.38 : gros plan : l'aile ouverte reçoit un reflet clair le long de chaque grande plume
static void aile(M5Canvas& g, const Rep& R_, float su, float sv, const Aile& a, float wl, uint16_t col, float wsgn) {
  // panneau de l'avant-bras + 6 grandes plumes qui s'écartent en éventail ; wsgn = +1 aile du côté P, -1 aile du côté Q (le côté s'écarte quand le corps pivote)
  if (wl < 0.04f) return;
  const bool plat = (R_.sn < 0.001f && R_.ro == 0.0f);
  float sdx = 0.0f, sdy = 0.0f; if (!plat) R_.soude(su, sv, wsgn * EPAULE, sdx, sdy);      // 3.1.32 : racine soudee a l'epaule meme quand le corps roule (virage, retournement)
  auto PW = [&](float u, float v, float& X, float& Y) {
    if (plat) { R_.p(u, v, X, Y); return; }
    float du_ = u - su, dv_ = v - sv; R_.q(u, v, wsgn * (EPAULE + 0.30f * sqrtf(du_ * du_ + dv_ * dv_) + 0.55f * fabsf(du_)), X, Y); X += sdx; Y += sdy;
  };
  float dau, dav, dhu, dhv; dirAile(a.ea, a.sa, dau, dav); dirAile(a.eh, a.sh, dhu, dhv);
  float la = 17.0f * wl, lh = 23.0f * wl;
  float tau = -dav, tav = dau; if (tau > 0.05f || (fabsf(tau) <= 0.05f && tav < 0)) { tau = -tau; tav = -tav; }
  float thu = -dhv, thv = dhu; if (thu > 0.05f || (fabsf(thu) <= 0.05f && thv < 0)) { thu = -thu; thv = -thv; }
  float Wu = su + dau * la, Wv = sv + dav * la;
  float q[10];
  PW(su, sv, q[0], q[1]); PW(Wu - tau * 1.3f, Wv - tav * 1.3f, q[2], q[3]); PW(Wu + tau * 9.0f * wl, Wv + tav * 9.0f * wl, q[4], q[5]);
  PW(su + dau * la * 0.45f + tau * 10.0f * wl, sv + dav * la * 0.45f + tav * 10.0f * wl, q[6], q[7]); PW(su + tau * 6.0f * wl, sv + tav * 6.0f * wl, q[8], q[9]);
  polyS(g, q, 5, col);
  { float tx_, ty_;      // pointes déchirées au bord de fuite
    PW(Wu + tau * 14.0f * wl - dhu * 2.0f, Wv + tav * 14.0f * wl - dhv * 2.0f, tx_, ty_); tri(g, q[4], q[5], q[6], q[7], tx_, ty_, col);
    PW(su + dau * la * 0.45f + tau * 15.0f * wl, sv + dav * la * 0.45f + tav * 15.0f * wl, tx_, ty_); tri(g, q[6], q[7], q[8], q[9], tx_, ty_, col); }
  for (int k = 0; k < (OP.q == 0 ? (mnE() ? 3 : (dgE() ? 4 : 5)) : (mnE() ? 4 : (dgE() ? 5 : 7))); k++) {
    float phi = k * 0.14f + ((k & 1) ? 0.05f : 0.0f), cph = cosf(phi), sph = sinf(phi);
    float du = dhu * cph + thu * sph, dv = dhv * cph + thv * sph, L = lh * (1.0f - 0.045f * k) * ((k & 1) ? 0.80f : 1.04f);
    float ru = Wu + dhu * 0.7f * k + tau * 0.6f * k, rv = Wv + dhv * 0.7f * k + tav * 0.6f * k, w = 2.4f * wl + 0.7f;
    float x1, y1, x2, y2, x3, y3;
    PW(ru - thu * w, rv - thv * w, x1, y1); PW(ru + thu * w, rv + thv * w, x2, y2); PW(ru + du * L, rv + dv * L, x3, y3);
    tri(g, x1, y1, x2, y2, x3, y3, col);
    if (detG) ligne(g, (x1 + x2) * 0.5f, (y1 + y2) * 0.5f, x3, y3, detC);
  }
}
static float aileDef = 0.0f;      // 3.1.41 : 0 aile saine, 0,7 aile blessee, 1 aile gravement blessee (une aile qui pend en vol)
static void deuxAiles(M5Canvas& g, const Rep& R_, const Aile& pose, float wl, uint16_t col) {      // l'aile proche et l'aile lointaine échangent leurs rôles pendant le demi-tour
  float u01 = 0.5f + 0.5f * R_.d;
  Aile a2 = pose; a2.ea += 14.0f - 60.0f * aileDef; a2.eh += 10.0f - 50.0f * aileDef;
  Aile pP = mixA(a2, pose, u01), pQ = mixA(pose, a2, u01);
  float bPu = lerpf(3.2f, 1.6f, u01), bPv = lerpf(-3.8f, -3.2f, u01), bQu = lerpf(1.6f, 3.2f, u01), bQv = lerpf(-3.2f, -3.8f, u01);
  float wP = wl * lerpf(0.92f, 1.0f, u01), wQ = wl * lerpf(1.0f, 0.92f, u01);
  if (R_.d >= 0) { aile(g, R_, bQu, bQv, pQ, wQ, col, -1.0f); aile(g, R_, bPu, bPv, pP, wP, col, 1.0f); }
  else { aile(g, R_, bPu, bPv, pP, wP, col, 1.0f); aile(g, R_, bQu, bQv, pQ, wQ, col, -1.0f); }
}

static float tCs[15][14], tSn[15][14]; static bool tTrigOk = false;      // cosinus et sinus des sommets du corps et de la poitrine (6 à 14 sommets), calculés une seule fois : mêmes valeurs qu'avant, sans recalcul à chaque image
static void initTrig() { for (int n = 6; n <= 14; n++) for (int i = 0; i < n; i++) { const float a = i * (6.2832f / (float)n); tCs[n][i] = cosf(a); tSn[n][i] = sinf(a); } tTrigOk = true; }
static void dessinerCorbeau(M5Canvas& g, const Corbeau& c) {
  if (!tTrigOk) initTrig();
  { const float sc = c.s * zs; lodG = (OP.q == 2 || c.duvet > 0.02f || sc >= 0.79f) ? 0 : (sc < 0.60f ? 2 : 1); }      // taille a l'ecran : moins de details quand le corbeau est petit
  const uint16_t NOIR = C(4, 3, 6);
  Rep R_; R_.cx = c.x; R_.cy = c.y + c.bob; R_.d = c.dv; R_.s = c.s; R_.ct = cosf(c.pitch); R_.st = sinf(c.pitch); { float sqv = c.sq > 0.2f ? c.sq : 1.0f; R_.sy = sqv; R_.sx = 1.0f / sqrtf(sqv); }
  R_.sn = sqrtf(mxf(0.0f, 1.0f - c.dv * c.dv)); if (c.ro != 0.0f) { R_.ro = c.ro; R_.cro = cosf(c.ro); R_.sro = sinf(c.ro); }
  const float s = c.s, dv = c.dv, sn = R_.sn;
  const int ci0 = (int)(&c - cr); const bool pD = natPatte(ci0); const float def = natAile(ci0) ? (natSante(ci0) >= 2 ? 1.0f : 0.7f) : 0.0f;      // 3.1.41 : blessure a la patte / a l'aile
  // 3.1.38 : GROS PLAN (corbeau vu de pres : visite a la vitre, zoom, oiseau pose dans la paume) : volume en trois tons, plumes en couches, reflets ; la lumiere vient de la lune (en haut a droite) ; coupe par l'allegement automatique (dgE) et jamais pour un oiseau de taille normale
  const bool gros = !dgE() && c.duvet < 0.02f && (s * zs >= 1.6f || (c.perch == 14 && c.etat != E_VOL));
  const bool rg = (OP.hor != 0);
  const uint16_t gM = rg ? C(26, 8, 12) : C(14, 17, 32), gH = rg ? C(58, 14, 20) : C(34, 44, 76), gH2 = rg ? C(104, 30, 34) : C(70, 90, 140);
  float q[32];
  auto P = [&](float u, float v, float& X, float& Y) { R_.p(u, v, X, Y); };
  // pattes : le pied reste posé sur le support, la hanche suit le corps (les orteils agrippent la branche) ; de face, les deux pattes s'écartent
  if (c.pied) {
    float hx, hy; hx = R_.cx + dv * s * (0.8f + 1.8f * sinf(c.pitch)); hy = R_.cy + s * (4.9f + 0.4f * sinf(c.pitch));
    for (int k = 0; k < 2; k++) {
      float sk = k ? 1.0f : -1.0f;
      float hipx = hx + sk * 1.3f * s * dv - sk * 1.8f * s * sn, hipy = hy - 0.4f;
      float fxk = c.fx + sk * 2.0f * s * dv - sk * 2.4f * s * sn, fyk = c.fy + c.fp * (fxk - c.fx);
      if (pD && k == 0) { const float kx_ = hipx + dv * 1.6f * s, ky_ = hipy + 2.6f * s; ligne(g, hipx, hipy, kx_, ky_, NOIR); ligne(g, kx_, ky_, hipx - dv * 0.4f * s, hipy + 3.8f * s, NOIR); continue; }      // patte blessee : repliee contre le ventre
      if (k == 1 && c.grat > 0.02f) {      // il se gratte : une patte monte vers la tête
        float sx_, sy_; P(7.0f, -0.5f, sx_, sy_); float a = pas(c.grat), wob = sinf(c.tAct * 26.0f) * 0.8f * a;
        fxk = lerpf(fxk, sx_ + wob, a); fyk = lerpf(fyk, sy_ + wob * 0.6f, a);
      }
      tri(g, hipx - 1.1f, hipy, hipx + 1.1f, hipy, fxk, fyk, NOIR);
      ligne(g, hipx, hipy, fxk, fyk, NOIR);
      ligne(g, fxk, fyk, fxk + dv * 3.4f * s, fyk + 0.3f, NOIR); ligne(g, fxk, fyk, fxk + dv * 2.4f * s, fyk - 0.7f, NOIR); ligne(g, fxk, fyk, fxk - dv * 1.8f * s, fyk + 0.3f, NOIR);
    }
  } else if (c.fan > 0.3f) {      // approche d'un atterrissage : les pattes pendent vers l'avant
    float hx, hy; hx = R_.cx + dv * s * (0.8f + 1.8f * sinf(c.pitch)); hy = R_.cy + s * (4.9f + 0.4f * sinf(c.pitch)); float e = c.fan;
    for (int k = 0; k < 2; k++) { float sk = k ? 1.0f : -1.0f; ligne(g, hx + sk * 1.3f * s * dv - sk * 1.8f * s * sn, hy, hx + dv * (3.0f + sk * 1.5f) * s - sk * 2.4f * s * sn, hy + 6.0f * s * e, NOIR); }
  }
  { float sp = hypotf(c.vx, c.vy);      // lignes de vitesse (parallèles à la trajectoire) et image fantôme des ailes : flou de mouvement
    if (OP.q >= 1 && c.wl > 0.5f && c.etat == E_VOL && sp > 95.0f) {
      float ux = -c.vx / sp, uy = -c.vy / sp, qx = -uy, qy = ux;
      for (int i = 0; i < 4; i++) { float o = (i - 1.5f) * 6.0f * s, l0 = 14.0f * s + i * 2.5f * s, l1 = l0 + (12.0f + 6.0f * (i % 2) + 0.05f * sp) * s; ligne(g, R_.cx + ux * l0 + qx * o, R_.cy + uy * l0 + qy * o, R_.cx + ux * l1 + qx * o, R_.cy + uy * l1 + qy * o, (i % 2 == 0) ? C(196, 74, 62) : C(130, 40, 40)); }
    }
    if (OP.q >= 1 && c.wl > 0.5f && c.etat == E_VOL) { aileDef = def; deuxAiles(g, R_, c.poseP, c.wl, C(60, 9, 14)); aileDef = 0.0f; } }
  // ailes ouvertes (vol)
  if (c.wl > 0.04f) { detG = gros; detC = gH; aileDef = def; deuxAiles(g, R_, c.pose, c.wl, NOIR); aileDef = 0.0f; detG = false; }
  // queue : plumes pointues qui pendent (perché) ou se rassemblent vers l'arrière (vol), et une plume dominante relevée ; en éventail latéral dans les virages
  { float wag = (c.ruf <= 0.01f) ? 0.0f : 0.10f * c.ruf * sinf(c.tAct * 38.0f);
    float perch = 1.0f - lim(c.wl / 0.6f, 0.0f, 1.0f), droop = lerpf(0.04f, c.tq, perch), spread = lerpf(0.07f + 0.30f * c.fan, 0.24f + 0.2f * c.fan, perch);
    const float bu = -8.0f, bv = 0.8f; float q4[8];
    P(-9.0f, -2.6f, q4[0], q4[1]); P(-9.0f, 3.8f, q4[2], q4[3]); P(-12.0f, 5.0f + 5.0f * perch, q4[4], q4[5]); P(-12.0f, -1.0f, q4[6], q4[7]); poly(g, q4, 4, NOIR);
    for (int k = 0; k < 5; k++) {
      if (mnE() && (k == 1 || k == 3)) continue;      // allègement maximal : trois plumes de queue au lieu de cinq
      float th = droop + (k - 2) * spread + wag, L = (23.0f - fabsf((float)(k - 2)) + ((k % 2 == 0) ? 4.0f : -3.0f)) * lerpf(1.0f, 1.05f, perch);
      float dx_ = -cosf(th), dy_ = sinf(th), px_ = sinf(th), py_ = cosf(th), w = 3.0f - 0.6f * (k % 2), x1, y1, x2, y2, x3, y3;
      P(bu + px_ * w, bv + py_ * w, x1, y1); P(bu - px_ * w, bv - py_ * w, x2, y2); R_.q(bu + dx_ * L, bv + dy_ * L, (k - 2) * (0.9f + 3.4f * c.fan), x3, y3);
      tri(g, x1, y1, x2, y2, x3, y3, NOIR);
    }
    float th = lerpf(0.0f, -(c.pitch + 0.2f), perch) + wag * 0.5f, Lp = lerpf(26.0f, c.plume, perch), rx = bu - 1.0f, ry = bv - 1.4f;
    for (int seg = 0; seg < 2; seg++) {      // plume dominante : relevée vers l'arrière, avec une courbure
      float dx_ = -cosf(th), dy_ = sinf(th), px_ = sinf(th), py_ = cosf(th), ln = Lp * (seg == 0 ? 0.55f : 0.45f), w0 = seg == 0 ? 3.2f : 2.0f, w1 = seg == 0 ? 2.0f : 0.0f;
      float ex = rx + dx_ * ln, ey = ry + dy_ * ln, a1x, a1y, a2x, a2y, b1x, b1y, b2x, b2y;
      P(rx + px_ * w0, ry + py_ * w0, a1x, a1y); P(rx - px_ * w0, ry - py_ * w0, a2x, a2y); P(ex + px_ * w1, ey + py_ * w1, b1x, b1y); P(ex - px_ * w1, ey - py_ * w1, b2x, b2y);
      tri(g, a1x, a1y, a2x, a2y, b1x, b1y, NOIR); tri(g, a2x, a2y, b2x, b2y, b1x, b1y, NOIR);
      rx = ex; ry = ey; th -= 0.16f;
    } }
  // corps : la respiration gonfle la poitrine, le secouement hérisse les plumes ; il s'élargit quand il pivote (de face, on voit sa largeur)
  float br = 1.0f + 0.05f * sinf(c.resp * 2.7f) + 0.09f * c.ruf * sinf(c.tAct * 46.0f);
  const float wfb = sqrtf(dv * dv + 0.72f * sn * sn), wfc = sqrtf(dv * dv + sn * sn);      // retournement épais : de face le corps garde au moins 85 % de sa largeur de profil (jamais une lamelle)
  const int NB = mnE() ? 8 : (dgE() ? 10 : 14), NC = mnE() ? 6 : (dgE() ? 8 : 10);      // allègement : moins de sommets pour le corps et la poitrine quand l'image est en retard (jamais en temps normal)
  for (int i = 0; i < NB; i++) { R_.pw(tCs[NB][i] * 11.8f * (1.0f + 0.02f * (br - 1.0f) * 5.0f), tSn[NB][i] * 5.9f * br, wfb, q[i * 2], q[i * 2 + 1]); }
  poly(g, q, NB, NOIR);
  float ch[20];      // poitrine bombée : centre porté par la direction, rayon par la largeur
  { float ucc = 6.6f * R_.ct + 1.4f * R_.st, vcc = -6.6f * R_.st + 1.4f * R_.ct;
    for (int i = 0; i < NC; i++) {
      float ru = tCs[NC][i] * 5.4f * (0.5f + 0.5f * br) * (1.0f + 0.30f * sn), rv = tSn[NC][i] * 5.2f * br * (1.0f + 0.12f * sn);
      float ruu = ru * R_.ct + rv * R_.st, rvv = -ru * R_.st + rv * R_.ct;
      ch[i * 2] = R_.cx + s * R_.sx * (ucc * dv + ruu * wfc); ch[i * 2 + 1] = R_.cy + s * R_.sy * (vcc + rvv);
    } }
  poly(g, ch, NC, NOIR);
  if (sn > 0.10f) {      // de face : épaules rondes et poitrail large (volume du corps, pas une feuille)
    for (int e = 0; e < 2; e++) { float es = e ? -1.0f : 1.0f, X1, Y1; R_.q(2.2f, -1.4f, es * 5.8f, X1, Y1); disque(g, X1, Y1, (2.6f + 3.2f * sn) * s, NOIR); }
    float X2, Y2; R_.q(4.6f, 2.4f, 0.0f, X2, Y2); disque(g, X2, Y2, (2.4f + 4.0f * sn) * s, NOIR);
  }
  if (c.duvet > 0.02f) {      // petit corbeau : plumes duveteuses autour du corps
    const int ND = qDeg ? 6 : 10;
    for (int i = 0; i < ND; i++) {
      float a = i * (6.28f / (float)ND) + 0.2f, ex = 3.5f * c.duvet + 1.2f * sinf(i * 2.3f), x1, y1, x2, y2, x3, y3;
      R_.pw(cosf(a) * 11.0f, sinf(a) * 5.2f, wfb, x1, y1); R_.pw(cosf(a + 0.2f) * 11.0f, sinf(a + 0.2f) * 5.2f, wfb, x2, y2); R_.pw(cosf(a + 0.1f) * (12.5f + ex), sinf(a + 0.1f) * (6.5f + ex * 0.8f), wfb, x3, y3);
      tri(g, x1, y1, x2, y2, x3, y3, NOIR);
    }
  }
  const float apf = 1.0f - lim(c.wl / 0.6f, 0.0f, 1.0f), alev = lim(0.9f / (s * zs), 1.0f, 2.2f), ayp = -5.7f - 1.6f * alev;      // 3.1.32 : aile repliee plaquee au dos, bord superieur releve au-dessus de la ligne du dos (au moins ~1 px a l'ecran, meme tres petit)
  if (c.wl < 0.6f) {      // aile repliée sur le flanc : couvertures + primaires pointues qui pendent vers le bas (les deux côtés quand il pivote)
    const float wp[10] = {6.0f, -4.6f, -1.5f, ayp, -14.5f, -3.4f, -15.5f, 2.0f, -2.0f, 3.8f};
    const float lointain = (dv >= 0) ? -1.0f : 1.0f;
    for (int cote = 0; cote < 2; cote++) {
      float ws = cote ? lointain : -lointain; if (cote && sn < 0.05f) continue;
      float wz = ws * EPAULE; if (sn < 0.001f && c.ro == 0.0f) wz = 0.0f;
      float fdx, fdy; R_.soude(wp[0], wp[1], wz, fdx, fdy);
      for (int i = 0; i < 5; i++) { R_.q(wp[i * 2], wp[i * 2 + 1], wz, q[i * 2], q[i * 2 + 1]); q[i * 2] += fdx; q[i * 2 + 1] += fdy; }
      poly(g, q, 5, NOIR);
      for (int k = 0; k < (mnE() ? 1 : (dgE() ? 2 : 4)); k++) {
        float u0 = 2.0f - 3.8f * k, v0 = 3.0f, lg = (9.0f + ((k % 2 == 0) ? 4.0f : 0.0f)) * c.wd * (1.0f + 0.35f * def), x1, y1, x2, y2, x3, y3;
        R_.q(u0 + 2.0f, v0 - 1.4f, wz, x1, y1); R_.q(u0 - 2.0f, v0 + 0.8f, wz, x2, y2); R_.q(u0 - lg, v0 + 1.0f + ((k % 2) ? 2.0f : -1.0f) + 3.2f * def, wz, x3, y3); x1 += fdx; y1 += fdy; x2 += fdx; y2 += fdy; x3 += fdx; y3 += fdy;
        tri(g, x1, y1, x2, y2, x3, y3, NOIR);
      }
    }
  }
  // cou et tête : la tête tourne (angle du cou ha, angle du bec hb)
  float hu = 8.2f + 4.8f * cosf(c.ha), hv = -2.0f + 4.8f * sinf(c.ha), cb = cosf(c.hb), sb = sinf(c.hb);
  auto PH = [&](float p, float qq, float& X, float& Y) { P(hu + p * cb - qq * sb, hv + p * sb + qq * cb, X, Y); };
  auto PHw = [&](float p, float qq, float w, float& X, float& Y) { R_.q(hu + p * cb - qq * sb, hv + p * sb + qq * cb, w, X, Y); };
  float n1x, n1y, n2x, n2y, hhx, hhy; P(3.5f, -5.2f, n1x, n1y); P(7.5f, 3.8f, n2x, n2y); P(hu, hv, hhx, hhy);
  tri(g, n1x, n1y, n2x, n2y, hhx, hhy, NOIR);
  disque(g, hhx, hhy, 5.0f * (1.0f + 0.14f * sn) * s, NOIR);
  if (sn > 0.10f) { float nx_, ny_; P(hu - 2.0f, hv + 3.4f, nx_, ny_); disque(g, nx_, ny_, (2.8f + 2.6f * sn) * s, NOIR); }      // cou épais vu de face
  { float a1x, a1y, a2x, a2y, a3x, a3y; PH(-3.8f, 1.5f, a1x, a1y); PH(0.8f, 4.0f, a2x, a2y); PH(-4.6f, 6.0f, a3x, a3y); tri(g, a1x, a1y, a2x, a2y, a3x, a3y, NOIR); }      // gorge hirsute
  { float o = c.bec, Ax, Ay, Bx, By, Mx, My, Tx, Ty, Lx, Ly, Ux, Uy;      // bec lourd, qui s'ouvre quand il croasse
    PH(2.4f, -2.8f, Ax, Ay); PH(2.6f, 3.3f, Bx, By); PH(2.8f, 0.2f, Mx, My); PH(13.5f, 1.0f - 3.4f * o, Tx, Ty); PH(11.8f, 2.2f + 3.6f * o, Lx, Ly); PH(7.6f, -3.0f, Ux, Uy);
    tri(g, Ax, Ay, Mx, My, Tx, Ty, NOIR); tri(g, Mx, My, Bx, By, Lx, Ly, NOIR); tri(g, Ax, Ay, Ux, Uy, Tx, Ty, NOIR);
    if (sn > 0.05f) { float slx, sly, srx, sry; PHw(2.8f, 0.2f, -2.4f, slx, sly); PHw(2.8f, 0.2f, 2.4f, srx, sry); tri(g, slx, sly, srx, sry, Tx, Ty, NOIR); } }      // de face : la largeur du bec
  // objet porté dans le bec
  if (c.objet) {
    float X, Y; PH(11.0f, 2.6f, X, Y);
    float tw = (sinf(millis() * 0.0081f) > 0.6f) ? 1.0f : 0.0f;
    dessinerObjet(g, c.objet - 1, X + dv * 1.5f, Y + 2.0f, 3.6f * s + 0.8f, dv > 0 ? 0.4f : 2.7f, tw);
  }
  if (gros) {      // volume en trois tons : ton de base (noir), ton moyen sur le corps, ton clair serre côté lune ; plumes en couches sur le flanc, reflet sur l'aile, la tête et le bec
    const float lx = 0.55f * s, ly = -0.83f * s; float bq[28], cxm = 0.0f, cym = 0.0f;
    for (int i = 0; i < NB; i++) { R_.pw(tCs[NB][i] * 11.8f * (1.0f + 0.02f * (br - 1.0f) * 5.0f), tSn[NB][i] * 5.9f * br, wfb, bq[i * 2], bq[i * 2 + 1]); cxm += bq[i * 2]; cym += bq[i * 2 + 1]; }
    cxm /= (float)NB; cym /= (float)NB;
    for (int t3 = 0; t3 < 2; t3++) {      // deux couches : large (ton moyen) puis serrée (ton clair)
      const float kk = t3 ? 0.60f : 0.88f, sh = t3 ? 2.4f : 1.0f; float hq[28];
      for (int i = 0; i < NB; i++) { hq[i * 2] = cxm + (bq[i * 2] - cxm) * kk + lx * sh; hq[i * 2 + 1] = cym + (bq[i * 2 + 1] - cym) * kk + ly * sh * 0.8f; }
      poly(g, hq, NB, t3 ? gH : gM);
    }
    { float cx2 = 0.0f, cy2 = 0.0f, hc[20]; for (int i = 0; i < NC; i++) { cx2 += ch[i * 2]; cy2 += ch[i * 2 + 1]; } cx2 /= (float)NC; cy2 /= (float)NC;
      for (int i = 0; i < NC; i++) { hc[i * 2] = cx2 + (ch[i * 2] - cx2) * 0.58f + lx * 1.5f; hc[i * 2 + 1] = cy2 + (ch[i * 2 + 1] - cy2) * 0.58f + ly * 1.2f; }
      poly(g, hc, NC, gM); }      // poitrine
    for (int j = 0; j < 3; j++) for (int i = 0; i < 4; i++) {      // plumes en couches : petits chevrons décalés une rangée sur deux
      const float u0 = -7.0f + 4.4f * i + ((j & 1) ? 2.2f : 0.0f), v0 = -2.6f + 2.4f * j; float a1x, a1y, a2x, a2y, a3x, a3y;
      P(u0 - 1.6f, v0 - 0.7f, a1x, a1y); P(u0, v0 + 0.9f, a2x, a2y); P(u0 + 1.6f, v0 - 0.7f, a3x, a3y); ligne(g, a1x, a1y, a2x, a2y, gH); ligne(g, a2x, a2y, a3x, a3y, gH);
    }
    if (c.wl < 0.6f) { float x1, y1, x2, y2, x3, y3; P(6.0f, -3.4f, x1, y1); P(-1.5f, ayp + 1.3f, x2, y2); P(-14.5f, -2.2f, x3, y3); ligne(g, x1, y1, x2, y2, gH2); ligne(g, x2, y2, x3, y3, gH); }      // reflet sur le bord de l'aile repliée
    disque(g, hhx + lx * 0.9f, hhy + ly * 0.8f, 3.1f * s, gM); disque(g, hhx + lx * 1.5f, hhy + ly * 1.3f, 1.7f * s, gH);      // tête
    { float b0x, b0y, b1x, b1y; PH(3.4f, -2.2f, b0x, b0y); PH(12.2f, -2.8f * c.bec, b1x, b1y); ligne(g, b0x, b0y, b1x, b1y, gH2); PH(4.8f, -0.9f, b0x, b0y); disque(g, b0x, b0y, 0.4f * s + 0.2f, C(0, 0, 0)); }      // reflet sur le bec et narine
    for (int i = 0; i < 3; i++) { float n0x, n0y, n1x, n1y; PH(-2.0f - i * 1.2f, 2.2f + i * 1.4f, n0x, n0y); PH(-5.6f - i * 1.0f, 3.6f + i * 1.8f, n1x, n1y); ligne(g, n0x, n0y, n1x, n1y, gH); }      // plumes du cou
  }
  // liseré rouge sombre côté lune : du volume ; le ventre s'éclaire quand le corbeau se montre de face
  { const uint16_t RIMC = (OP.hor == 2) ? C(70, 12, 18) : ((OP.hor == 1) ? C(88, 16, 22) : C(104, 20, 26)); float x1, y1, x2, y2;
    const float rB = lerpf(-6.2f, -4.6f + 0.667f * (ayp + 4.6f), apf), rC = lerpf(-5.0f, ayp + 0.577f * (-3.4f - ayp), apf), rBu = lerpf(0.0f, 1.0f, apf), rDu = lerpf(-14.0f, -14.5f, apf), rD = lerpf(-3.0f, -3.4f, apf);      // le liseré suit le bord releve de l'aile repliee
    const float bd[3][4] = {{8, -5.4f, rBu, rB}, {rBu, rB, -9, rC}, {-9, rC, rDu, rD}};
    if (!qMin) for (int i = 0; i < 3; i++) { P(bd[i][0], bd[i][1], x1, y1); P(bd[i][2], bd[i][3], x2, y2); ligne(g, x1, y1, x2, y2, RIMC); }
    const float hd[4][4] = {{-3.2f, -4.6f, 0, -5.2f}, {0, -5.2f, 3.4f, -4.4f}, {4.2f, -3.4f, 7.6f, -3.1f}, {7.6f, -3.1f, 13, 0.6f}};
    if (!qMin) for (int i = 0; i < 4; i++) { PH(hd[i][0], hd[i][1], x1, y1); PH(hd[i][2], hd[i][3], x2, y2); ligne(g, x1, y1, x2, y2, RIMC); }
    if (sn > 0.25f && !qMin) {
      float kk = lim((sn - 0.25f) / 0.5f, 0.0f, 1.0f);
      for (int e = 0; e < 2; e++) { float es = e ? -1.0f : 1.0f, X1, Y1, rr = (2.6f + 3.2f * sn) * s; R_.q(2.2f, -1.4f, es * 5.8f, X1, Y1); ligne(g, X1 - es * rr * 0.9f, Y1 - rr * 0.35f, X1 - es * rr * 0.25f, Y1 - rr * 0.95f, RIMC); ligne(g, X1 - es * rr * 0.25f, Y1 - rr * 0.95f, X1 + es * rr * 0.5f, Y1 - rr * 0.9f, RIMC); }      // liseré sur le dessus des épaules
      uint16_t BC = CF(104.0f * (0.5f + 0.5f * kk), 20.0f * kk + 8.0f, 26.0f * kk + 6.0f);
      for (int i = 0; i < NC - 1; i++) if (ch[i * 2] >= R_.cx - 0.5f && ch[i * 2 + 2] >= R_.cx - 0.5f) ligne(g, ch[i * 2], ch[i * 2 + 1], ch[i * 2 + 2], ch[i * 2 + 3], BC);
      float b0x, b0y, b1x, b1y, b2x, b2y; R_.pw(-7.0f, 5.7f, wfb, b0x, b0y); R_.pw(0.0f, 6.1f * br, wfb, b1x, b1y); R_.pw(7.0f, 5.0f, wfb, b2x, b2y);
      ligne(g, b0x, b0y, b1x, b1y, BC); ligne(g, b1x, b1y, b2x, b2y, BC);
    } }
  // yeux rouges qui brillent : un par côté, visible selon l'orientation ; les deux apparaissent à mi-rotation
  { const float ek = (s > 1.2f) ? powf(1.2f / s, 0.5f) : 1.0f;      // l'œil reste petit quand le corbeau est très grand (gros plan)
    const uint16_t cA = c.rare ? C(150, 100, 10) : oeilAnn, cC = c.rare ? C(255, 215, 70) : oeilCoeur;
    const float pro = (dv >= 0) ? 1.0f : -1.0f;
    const int ci_ = (int)(&c - cr); const float ye = (ci_ >= 0 && ci_ < NCB && ci_ == TD.k) ? TD.yeux : 0.0f;      // 3.1.39 : l'oiseau caressé / serré doucement dans la paume ferme les yeux
    if (c.clin < 0.5f && ye < 0.7f) {
      for (int e = 0; e < 2; e++) {
        float es = e ? -1.0f : 1.0f, vis = 0.85f * dv * es + 0.5f * sn; if (vis < -0.05f) continue;
        float kk = lim((vis + 0.05f) / 0.45f, 0.0f, 1.0f), rr = (0.55f + 0.45f * kk) * ek * (1.0f - 0.45f * ye), ex, ey; PHw(1.6f, -1.0f, es * 3.2f, ex, ey);
        if (OP.hor && OP.q >= 1) { float lu = lueurOeil * (1.0f + 0.05f * sinf(c.resp * 5.3f)) * ((OP.hor == 2) ? 1.0f : 0.75f); disque(g, ex, ey, (3.9f * s + 1.6f) * rr * lu, C(34, 3, 7)); disque(g, ex, ey, (2.7f * s + 1.0f) * rr * lu, C(78, 6, 11)); }      // horreur : l'œil est presque la seule lumière, il éclaire la tête autour de lui
        if (gros) disque(g, ex, ey, (2.4f * s + 0.9f) * rr, C(2, 1, 4));      // orbite sombre autour de l'œil
        disque(g, ex, ey, (1.7f * s + 0.6f) * rr, cA); disque(g, ex, ey, (1.1f * s + 0.2f) * rr, cC);
        if (gros) disque(g, ex - 0.35f * s, ey - 0.45f * s, (0.30f * s + 0.12f) * rr, C(255, 242, 232));      // reflet de lumière dans l'œil
      }
    } else { float ex, ey; PHw(1.6f, -1.0f, pro * 3.2f, ex, ey); ligne(g, ex - 1.5f, ey, ex + 1.5f, ey, C(70, 8, 12)); }
    if (c.flash > 0.01f) { float ex, ey; PHw(1.6f, -1.0f, pro * 3.2f, ex, ey); disque(g, ex, ey, 3.0f + 7.0f * c.flash, CF(255, 60 + 120 * c.flash, 40 + 90 * c.flash)); disque(g, ex, ey, 1.8f, C(255, 240, 225)); } }
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
  auto P = [&](float u, float v, float& X, float& Y) { v *= fv; X = m.x + (u * c - v * sn) * S; Y = m.y + (u * sn + v * c) * S; };      // fv : le petit corbeau ne vole jamais sur le dos
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


// ---------- la main tendue, la visée du lancer, l'élastique de la fronde ----------
// ---------- la main : manga stylisée, contours nets, ombrage, manche sombre qui sort du bas ----------
enum { PS_REPOS = 0, PS_OUVRE, PS_PINCE, PS_CROISE, PS_POINTE, PS_POING, PS_VAGUE, PS_ENVELOPPE };
static const float MP[8][5][2] = {      // [pose][doigt : pouce, index, majeur, annulaire, auriculaire][écart, flexion] ; flexion négative = relevé
  {{0.10f, 0.10f}, {0.0f, 0.05f}, {0.0f, 0.0f}, {-0.10f, -0.12f}, {-0.26f, -0.18f}},      // repos : annulaire et auriculaire légèrement relevés
  {{0.35f, -0.10f}, {0.10f, -0.05f}, {0.0f, -0.05f}, {-0.18f, -0.15f}, {-0.40f, -0.20f}},      // ouverte
  {{-0.30f, 0.70f}, {0.05f, -0.65f}, {0.0f, 0.20f}, {-0.10f, 0.12f}, {-0.22f, 0.05f}},      // pince
  {{0.10f, 0.40f}, {-0.26f, 0.05f}, {0.22f, 0.0f}, {-0.05f, 0.60f}, {-0.10f, 0.60f}},      // doigts croisés
  {{0.10f, 0.75f}, {0.05f, 0.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}},      // index pointé
  {{0.0f, 0.85f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}},      // poing
  {{0.10f, 0.10f}, {0.0f, 0.05f}, {0.0f, 0.0f}, {-0.10f, -0.12f}, {-0.26f, -0.18f}},      // vague : les doigts ondulent (calculée)
  {{0.0f, 0.62f}, {-0.04f, 0.55f}, {0.0f, 0.62f}, {-0.05f, 0.58f}, {-0.10f, 0.52f}}};      // enveloppe : les doigts se courbent doucement autour de l'oiseau, sans l'écraser
static const float MPAUME[5][2] = {{0.0f, 0.18f}, {0.03f, -0.30f}, {0.0f, -0.36f}, {-0.03f, -0.32f}, {-0.07f, -0.26f}};      // paume vers le haut : [doigt][écart, flexion] ; flexion négative = doigts relevés en coupe, pouce dressé
static const float DG[5][7] = {      // u0, v0, angle de base, 3 phalanges, largeur
  {0.0f, 7.4f, 1.00f, 4.6f, 3.6f, 3.2f, 3.6f}, {7.0f, 5.2f, 0.12f, 6.2f, 4.4f, 3.6f, 3.0f}, {7.0f, 1.9f, 0.02f, 6.8f, 4.8f, 3.8f, 3.1f},
  {7.0f, -1.5f, -0.08f, 6.2f, 4.4f, 3.5f, 2.9f}, {7.0f, -4.6f, -0.20f, 4.6f, 3.4f, 3.0f, 2.6f}};
static void capsule(M5Canvas& g, float ax, float ay, float bx, float by, float ra, float rb, uint16_t col) {
  float dx = bx - ax, dy = by - ay, l = sqrtf(dx * dx + dy * dy); if (l < 0.01f) l = 1.0f; float nx = -dy / l, ny = dx / l;
  tri(g, ax + nx * ra, ay + ny * ra, ax - nx * ra, ay - ny * ra, bx + nx * rb, by + ny * rb, col);
  tri(g, ax - nx * ra, ay - ny * ra, bx - nx * rb, by - ny * rb, bx + nx * rb, by + ny * rb, col);
  disque(g, ax, ay, ra, col); disque(g, bx, by, rb, col);
}
static void anneauPaume(M5Canvas& g, float cx, float cy, float rx, float ry, uint16_t col, int n) {      // un anneau aplati (ondulation sur la paume) tracé en n segments
  float px = cx + rx, py = cy;
  for (int i = 1; i <= n; i++) { const float a = 6.2832f * i / n, x = cx + rx * cosf(a), y = cy + ry * sinf(a); ligne(g, px, py, x, y, col); px = x; py = y; }
}
static void dessinerMain(M5Canvas& g) {
  if (!H.actif && H.ext < 0.02f) return;
  const uint16_t PEAU = C(238, 196, 166), OMBRE = C(196, 132, 116), CONTOUR = C(40, 12, 16), ONGLE = C(250, 214, 196), RIM = C(120, 24, 30), PLI = C(60, 10, 16);
  const uint16_t MAN = C(58, 20, 28), MAN2 = C(36, 11, 18), MANR = C(150, 42, 48), BAND = C(92, 26, 34);
  const RepM R_ = repereMain(); const float S = R_.S;
  auto L = [&](float u, float v, float& X, float& Y) { ptMain(R_, u, v, X, Y); };
  float wx, wy; L(-8.5f, 0.0f, wx, wy);      // poignet
  float ddx = wx - R_.ex, ddy = wy - R_.ey, ll = sqrtf(ddx * ddx + ddy * ddy); if (ll < 1.0f) ll = 1.0f;
  const float ax = ddx / ll, ay = ddy / ll, nx = -ay, ny = ax, sd = (ny >= 0.0f) ? 1.0f : -1.0f;      // sd : le côté bas (ombre)
  const float FL = 13.0f * S, w0 = 26.0f * S, w1 = 8.4f * S, wH = 4.9f * S, cH = 5.7f * S;      // 3.1.42 : manche en perspective : trapeze tres large en bas (pres du spectateur), etroit a la manchette puis au poignet
  const float c0x = wx - ax * FL, c0y = wy - ay * FL;      // bord de la manche (manchette)
  // manche : du bord de l'écran jusqu'à la manchette, avec plis, côté bas plus sombre et liseré clair en haut
  tri(g, R_.ex + nx * w0, R_.ey + ny * w0, R_.ex - nx * w0, R_.ey - ny * w0, c0x + nx * w1, c0y + ny * w1, MAN); tri(g, R_.ex - nx * w0, R_.ey - ny * w0, c0x - nx * w1, c0y - ny * w1, c0x + nx * w1, c0y + ny * w1, MAN);
  tri(g, R_.ex, R_.ey, R_.ex + sd * nx * w0, R_.ey + sd * ny * w0, c0x + sd * nx * w1, c0y + sd * ny * w1, MAN2); tri(g, R_.ex, R_.ey, c0x + sd * nx * w1, c0y + sd * ny * w1, c0x, c0y, MAN2);
  ligne(g, R_.ex - sd * nx * w0, R_.ey - sd * ny * w0, c0x - sd * nx * w1, c0y - sd * ny * w1, MANR);
  ligne(g, R_.ex + sd * nx * w0, R_.ey + sd * ny * w0, c0x + sd * nx * w1, c0y + sd * ny * w1, PLI);
  { static const float PF[3] = {-0.55f, 0.05f, 0.55f};
    for (int k = 0; k < 3; k++) ligne(g, c0x + nx * w1 * PF[k], c0y + ny * w1 * PF[k], R_.ex + nx * w0 * PF[k] * 0.85f - ax * 6.0f, R_.ey + ny * w0 * PF[k] * 0.85f - ay * 6.0f, (k == 1) ? PLI : MAN2); }
  { const float bl = 3.2f * S, b1 = w1 + 1.2f * S;      // manchette : un bord épais autour du poignet
    float b0x = c0x - ax * bl, b0y = c0y - ay * bl;
    tri(g, c0x + nx * w1, c0y + ny * w1, c0x - nx * w1, c0y - ny * w1, b0x + nx * b1, b0y + ny * b1, BAND); tri(g, c0x - nx * w1, c0y - ny * w1, b0x - nx * b1, b0y - ny * b1, b0x + nx * b1, b0y + ny * b1, BAND);
    ligne(g, b0x - sd * nx * b1, b0y - sd * ny * b1, b0x + sd * nx * b1, b0y + sd * ny * b1, MANR); ligne(g, c0x - sd * nx * w1, c0y - sd * ny * w1, b0x - sd * nx * b1, b0y - sd * ny * b1, MANR); }
  // avant-bras nu : de la manchette au poignet (contour net, peau, ombre sous le bras)
  { const float bl = 3.2f * S, f0x = c0x - ax * bl * 0.2f, f0y = c0y - ay * bl * 0.2f;
    for (int pass = 0; pass < 2; pass++) {
      float gr = pass ? 0.0f : 1.4f, cx_ = f0x, cy_ = f0y;
      uint16_t col = pass ? PEAU : CONTOUR;
      tri(g, wx + nx * (wH + gr), wy + ny * (wH + gr), wx - nx * (wH + gr), wy - ny * (wH + gr), cx_ + nx * (cH + gr), cy_ + ny * (cH + gr), col);
      tri(g, wx - nx * (wH + gr), wy - ny * (wH + gr), cx_ - nx * (cH + gr), cy_ - ny * (cH + gr), cx_ + nx * (cH + gr), cy_ + ny * (cH + gr), col);
    }
    tri(g, wx, wy, wx + sd * nx * wH, wy + sd * ny * wH, f0x + sd * nx * cH, f0y + sd * ny * cH, OMBRE); tri(g, wx, wy, f0x + sd * nx * cH, f0y + sd * ny * cH, f0x, f0y, OMBRE);
    ligne(g, wx - sd * nx * wH * 0.5f, wy - sd * ny * wH * 0.5f, f0x - sd * nx * cH * 0.5f, f0y - sd * ny * cH * 0.5f, C(214, 160, 138));      // reflet sur le dessus du bras
    disque(g, wx, wy, wH + 1.4f, CONTOUR); disque(g, wx, wy, wH, PEAU); }      // le poignet
  // paume : contour net, peau, ombrage. pal = 0 : main de face ; pal = 1 : paume tournée vers le haut, vue de profil (aplatie, creusée, doigts relevés en coupe, pouce dressé)
  const float pal = lim(H.pal, 0.0f, 1.0f), palS = pas(pal), vk = lerpf(1.0f, PAUME_VK, palS), fvk = lerpf(1.0f, 0.78f, palS);
  const uint16_t PAUMEC = C(247, 210, 186), PAUMEO = C(212, 146, 130), PEAUX = (palS > 0.5f) ? PAUMEC : PEAU;
  auto Lv = [&](float u, float v, float& X, float& Y) { ptMain(R_, u, v * vk, X, Y); };
  { static const float palm[12] = {-8, -4.8f, 7, -7.4f, 7.4f, 7.0f, 1.5f, 10.0f, -5, 8.4f, -8, 4.8f}; float cu = 0, cv_ = 0; for (int i = 0; i < 6; i++) { cu += palm[i * 2]; cv_ += palm[i * 2 + 1]; } cu /= 6; cv_ /= 6;
    float cx0, cy0; Lv(cu, cv_, cx0, cy0);
    float bx_, by_; ptPaume(R_, bx_, by_); const float ccx = lerpf(cx0, bx_, palS), ccy = lerpf(cy0, by_, palS);      // le creux de la paume : c'est exactement là que les pattes se posent
    if (palS > 0.3f) {      // la tranche de la main : on la voit sous la paume quand elle est tournée vers le haut
      const float ep = 2.3f * S * pas((palS - 0.3f) / 0.7f);
      for (int pass = 0; pass < 2; pass++) {
        float grow = pass ? 1.0f : 1.25f, px[6], py[6]; uint16_t col = pass ? OMBRE : CONTOUR;
        for (int i = 0; i < 6; i++) { Lv(cu + (palm[i * 2] - cu) * grow, cv_ + (palm[i * 2 + 1] - cv_) * grow, px[i], py[i]); py[i] += ep; }
        for (int i = 0; i < 6; i++) { int j = (i + 1) % 6; tri(g, ccx, ccy + ep, px[i], py[i], px[j], py[j], col); }
      }
    }
    for (int pass = 0; pass < 2; pass++) {
      float grow = pass ? 1.0f : 1.25f, px[6], py[6]; uint16_t col = pass ? PEAUX : CONTOUR;
      for (int i = 0; i < 6; i++) Lv(cu + (palm[i * 2] - cu) * grow, cv_ + (palm[i * 2 + 1] - cv_) * grow, px[i], py[i]);
      for (int i = 0; i < 6; i++) { int j = (i + 1) % 6; tri(g, ccx, ccy, px[i], py[i], px[j], py[j], col); }
    }
    float s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y; Lv(-8, -4.8f, s0x, s0y); Lv(7, -7.4f, s1x, s1y); Lv(7, -3.2f, s2x, s2y); Lv(-8, 0.4f, s3x, s3y);
    const uint16_t SH = (palS > 0.5f) ? PAUMEO : OMBRE;
    tri(g, s0x, s0y, s1x, s1y, s2x, s2y, SH); tri(g, s0x, s0y, s2x, s2y, s3x, s3y, SH);
    if (palS < 0.5f) {
      static const float pl[3][4] = {{1, 5.6f, 5.5f, 5.9f}, {1, 1.6f, 5.5f, 1.9f}, {1, -2.2f, 5.5f, -2.0f}};
      for (int i = 0; i < 3; i++) { float a1, b1, a2, b2; Lv(pl[i][0], pl[i][1], a1, b1); Lv(pl[i][2], pl[i][3], a2, b2); ligne(g, a1, b1, a2, b2, OMBRE); }      // plis des phalanges
      { float a1, b1, a2, b2; Lv(-6.0f, 2.0f, a1, b1); Lv(-1.5f, 4.4f, a2, b2); ligne(g, a1, b1, a2, b2, OMBRE); }      // pli de la paume
    } else {      // la paume : deux plis en arc
      static const float pc[2][4][2] = {{{-6.0f, 6.0f}, {-3.0f, 4.4f}, {0.0f, 3.4f}, {3.4f, 3.0f}}, {{-6.5f, 0.0f}, {-2.5f, -1.4f}, {1.5f, -2.2f}, {5.0f, -2.4f}}};
      for (int q = 0; q < 2; q++) for (int k = 0; k < 3; k++) { float a1, b1, a2, b2; Lv(pc[q][k][0], pc[q][k][1], a1, b1); Lv(pc[q][k + 1][0], pc[q][k + 1][1], a2, b2); ligne(g, a1, b1, a2, b2, PAUMEO); }
    }
  }
  if (H.pal > 0.5f && H.halo > 0.05f) {      // effet d'offrande : deux ondulations sombres-rouges qui s'étalent depuis la paume (deux anneaux de lignes, rien de plein écran)
    float hx, hy; ptPaume(R_, hx, hy); const int ns = (OP.q == 0) ? 10 : 14; const float hk = H.halo * pas((H.pal - 0.5f) * 2.0f);
    for (int k = 0; k < 2; k++) {
      const float ph = fmodf(tAnim * 0.55f + k * 0.5f, 1.0f), rx = 6.0f + 24.0f * ph, fa = (1.0f - ph) * hk;
      if (fa < 0.12f) continue;
      anneauPaume(g, hx, hy + 1.0f, rx, rx * 0.34f, CF(40.0f + 190.0f * fa, 6.0f + 54.0f * fa, 10.0f + 44.0f * fa), ns);
    }
  }
  // doigts (l'index passe par-dessus le majeur quand ils se croisent) ; paume vers le haut : ils se relèvent en coupe et le pouce se dresse
  int ordre[5] = {4, 3, 2, 1, 0}; if (H.sp[1] < -0.2f && H.sp[2] > 0.15f) { ordre[0] = 4; ordre[1] = 3; ordre[2] = 0; ordre[3] = 2; ordre[4] = 1; }
  for (int oi = 0; oi < 5; oi++) {
    int d = ordre[oi]; const float* f = DG[d]; float a = f[2] + H.sp[d] - ((d == 0) ? 0.78f * palS : 0.0f), fl = H.cu[d], px[4], py[4], rs[4], cu = f[0], cv_ = f[1] * fvk;
    L(cu, cv_, px[0], py[0]); rs[0] = f[6] * 0.5f * S;
    static const float KF[3] = {0.8f, 1.0f, 0.8f};
    for (int k = 0; k < 3; k++) { a += fl * KF[k] * (d == 0 ? -1.0f : 1.0f); float c25 = cosf(fl * 0.25f); cu += cosf(a) * f[3 + k] * c25; cv_ += sinf(a) * f[3 + k] * c25; L(cu, cv_, px[k + 1], py[k + 1]); rs[k + 1] = f[6] * 0.5f * S * (0.9f - 0.18f * (k + 1)); }
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < 3; i++) capsule(g, px[i], py[i], px[i + 1], py[i + 1], rs[i] + (pass ? 0.0f : 1.3f), rs[i + 1] + (pass ? 0.0f : 1.3f), pass ? PEAUX : CONTOUR);
    for (int i = 0; i < 3; i++) ligne(g, px[i] + 0.5f, py[i] + 0.9f, px[i + 1] + 0.5f, py[i + 1] + 0.9f, (palS > 0.5f) ? PAUMEO : OMBRE);
    disque(g, px[3], py[3], mxf(1.0f, rs[3] * 0.55f), ONGLE);
  }
  // objets posés dans la paume
  for (int i = 0; i < 3; i++) { ObjM& o = H.om[i]; if (!o.actif) continue; float ox, oy; if (palS > 0.5f) { ptPaume(R_, ox, oy); ox += o.u * 0.8f; oy -= 2.5f; } else { L(o.u * 0.8f, -2.0f, ox, oy); oy -= 5.0f; } dessinerObjet(g, o.type, ox, oy, 4.2f, o.u * 0.2f, 0.0f); }
  // braises qui montent de la paume (l'effet d'offrande)
  for (int q = 0; q < 6; q++) { const Braise& b = BRAIS[q]; if (!b.on) continue; const float u = lim(b.t / b.dur, 0.0f, 1.0f); disque(g, b.x, b.y, (u < 0.7f) ? 1.2f : 0.8f, CF(255.0f - 135.0f * u, 150.0f - 130.0f * u, 110.0f - 80.0f * u)); }
}
static void dessinerVisee(M5Canvas& g) {
  if (iconDrag) { dessinerObjet(g, visType >= 0 ? (int)visType : 0, VS.x, VS.y, 4.6f, 0.0f, 0.0f); return; }      // 3.1.42 : seulement l'icone que l'on glisse vers la paume
  float dx = VS.x - VS.x0, dy = VS.y - VS.y0, l = sqrtf(dx * dx + dy * dy);
  if (l > 6) for (int i = 0; i < (int)l; i += 8) disque(g, VS.x0 + dx * i / l, VS.y0 + dy * i / l, 1.0f, C(255, 200, 160));
  dessinerObjet(g, visType >= 0 ? (int)visType : (int)lanceTyp(choixL), VS.x, VS.y, 4.6f, 0.0f, 0.0f);
  float vx = D.vx, vy = D.vy; if (hypotf(vx, vy) < 200) { vx = dx * 7.0f; vy = dy * 7.0f; }
  for (int i = 1; i <= 5; i++) { float t = i * 0.12f; disque(g, VS.x + vx * 0.9f * t, VS.y + vy * 0.9f * t + 150.0f * t * t, 1.2f, C(255, 236, 200)); }
}
static void dessinerFronde(M5Canvas& g) {
  Corbeau& c = cr[FR.c];
  ligne(g, FR.ax, FR.ay + 14, FR.ax, FR.ay + 4, C(150, 90, 40)); ligne(g, FR.ax, FR.ay + 4, FR.ax - 8, FR.ay - 4, C(150, 90, 40)); ligne(g, FR.ax, FR.ay + 4, FR.ax + 8, FR.ay - 4, C(150, 90, 40));
  ligne(g, FR.ax - 8, FR.ay - 4, c.x, c.y + 2, C(200, 130, 60)); ligne(g, FR.ax + 8, FR.ay - 4, c.x, c.y + 2, C(200, 130, 60));
}


// ---------- l'œuf magique sur la branche ----------
static void dessinerOeuf(M5Canvas& g) {
  float amp = (OE.etat == 1) ? 1.4f * sinf(OE.tc * 38.0f) * (OE.tc + 0.3f) : 0.0f, x = OE.x + amp, y = OE.y - 5.4f;
  dessinerObjet(g, T_OEUF, x, y, 4.8f, amp * 0.08f, 0.0f);
  float pulse = 0.5f + 0.5f * sinf(tAnim * 3.0f);
  if (pulse > 0.78f) { disque(g, x - 1.7f, y - 2.4f, 1.0f, C(255, 190, 200)); }      // reflet qui brille par moments
  if (OE.etat == 1) {      // fissures qui s'ouvrent
    uint16_t cf = C(255, 150, 110); float p = OE.tc;
    ligne(g, x, y - 5.5f, x - 1.6f, y - 2.6f, cf); if (p > 0.25f) ligne(g, x - 1.6f, y - 2.6f, x + 0.8f, y - 0.6f, cf);
    if (p > 0.5f) { ligne(g, x + 0.8f, y - 0.6f, x - 1.2f, y + 2.0f, cf); ligne(g, x, y - 5.5f, x + 2.2f, y - 3.0f, cf); }
    if (p > 0.75f) { ligne(g, x - 1.2f, y + 2.0f, x + 1.0f, y + 4.2f, cf); ligne(g, x + 2.2f, y - 3.0f, x + 3.4f, y - 0.4f, cf); }
  }
}

// ---------- visite d'un corbeau : il vient vers l'écran, regarde, donne un coup de bec dans la vitre, repart ----------
static const float SILL_B = 176.0f;      // dessus de la barre de métal (pixels écran) : le corbeau s'arrête là, devant la vitre
static void centreBarre(float s, float pitch, float dv, float mir, float& cx, float& cy) {      // centre du corps quand les pattes sont sur la barre
  float hx, hy; hancheC(s, pitch, dv, hx, hy); cx = 160.0f - mir * 16.0f - hx; cy = SILL_B - JAMBE * s - hy;
}
static void poserGP() {      // une seule trajectoire continue : il saute de sa branche, grossit en battant des ailes, se pose sur la barre de métal (ou reste en l'air), frappe du bec, repart en rapetissant
  Corbeau& c = vcb; const float mir = (float)GP.mir, HS = GP.jeune ? 2.4f : 2.8f, T0 = 0.30f, t = GP.t, AIR = 30.0f;
  const bool barre = (OP.barre != 0);
  float LXf = 0, LYf = 0; centreBarre(HS, c.pr, mir * 0.5f, mir, LXf, LYf);
  const float HX = 160.0f - mir * 12.0f, HY = 112.0f, tx = barre ? LXf : HX, ty = barre ? LYf - AIR : HY;      // fin de l'approche
  float x = tx, y = ty, s = HS, dvv = mir * (barre ? 0.5f : 0.45f), pit = 0.42f, pk = 0.0f, fan = 0.55f, wl = 1.0f, ha = -0.30f, hbO = 0.19f; bool pied = false, accroupi = false, pose = false;
  float fxp = GP.fx0, fyp = GP.fy0;
  if (GP.ph == 1) {
    if (t < T0) {      // il se ramasse sur sa branche, les ailes commencent à s'ouvrir
      float e = pas(t / T0); x = GP.sx; y = GP.sy + 2.6f * GP.s0 * e; s = GP.s0; dvv = GP.dv0; pit = lerpf(GP.p0, 0.55f, e); fan = 0.0f; wl = pas((t - 0.10f) / 0.20f); pied = (t < T0 - 0.04f); accroupi = true;
      ha = lerpf(GP.ha0, -0.30f, e);
    } else {
      float u = lim((t - T0) / (GP.dur - T0), 0.0f, 1.0f), v = 0.7f * u + 0.3f * pas(u), e = pas(u), k = 1.0f - GP.s0 / HS;
      s = GP.s0 / (1.0f - k * v); x = lerpf(GP.sx, tx, e) - 20.0f * sinf(3.1416f * u); y = lerpf(GP.sy, ty, e) - 24.0f * sinf(3.1416f * u);
      dvv = lerpf(GP.dv0, mir * 0.55f, pas(u / 0.6f)); pit = lerpf(lerpf(0.55f, 0.12f, pas(u / 0.25f)), 0.42f, pas((u - 0.55f) / 0.45f)); fan = lerpf(0.0f, 0.55f, pas((u - 0.5f) / 0.5f));
    }
  } else if (GP.ph == 2) {      // il descend sur la barre, ailes ouvertes en frein puis repliées, les pattes se referment sur le métal
    float u = lim(t / GP.dur, 0.0f, 1.0f), e = pas(u);
    x = LXf; y = LYf - AIR * (1.0f - e); dvv = mir * lerpf(0.55f, 0.5f, u); pit = lerpf(0.42f, c.pr, e); fan = 0.55f * (1.0f - e); wl = 1.0f - pas((u - 0.3f) / 0.7f); pied = (u > 0.7f); pose = true;
    ha = lerpf(-0.30f, c.ha0, e); hbO = lerpf(0.19f, 0.49f, e); fxp = 160.0f - mir * 16.0f; fyp = SILL_B;
  } else if (GP.ph == 5) {
    float u = lim(t / GP.dur, 0.0f, 1.0f), v2 = powf(u, 1.5f), dir = (GP.ex >= GP.x0) ? 1.0f : -1.0f, k = GP.s0 / 0.15f - 1.0f;
    s = GP.s0 / (1.0f + k * v2); x = lerpf(GP.x0, GP.ex, u * u); y = lerpf(GP.y0, GP.ey, u * u) - 16.0f * sinf(3.1416f * u);
    dvv = lerpf(GP.dv0, dir, pas(u / 0.45f)); pit = lerpf(GP.p0, 0.08f, pas(u / 0.35f)); fan = lerpf(0.55f, 0.1f, pas(u / 0.5f)); if (GP.perche) wl = pas(t / 0.25f);
  } else if (barre) {      // posé sur la barre : il regarde, penche la tête, cligne, appelle, puis frappe la vitre du bec
    float sc = 1.0f; dvv = mir * 0.5f; wl = 0.0f; fan = 0.0f; pied = true; pose = true; hbO = 0.49f + 0.12f * sinf(t * 1.3f); fxp = 160.0f - mir * 16.0f; fyp = SILL_B;
    if (GP.ph == 4) {
      float cyc = 0.9f, tc = t - GP.nf * cyc;
      if (tc < 0.34f) { float e = pas(tc / 0.34f); sc = 1.0f - 0.05f * e; pk = -0.7f * e; }
      else if (tc < 0.42f) { float u = (tc - 0.34f) / 0.08f; sc = lerpf(0.95f, 1.22f, u); pk = lerpf(-0.7f, 1.0f, u); dvv = mir * lerpf(0.5f, 0.38f, u); }
      else { float u = pas((tc - 0.42f) / 0.48f); sc = lerpf(1.22f, 1.0f, u); pk = lerpf(1.0f, 0.0f, u); dvv = mir * lerpf(0.38f, 0.5f, u); }
      GP.sc = sc; GP.pk = pk;
    }
    s = HS * sc; pit = c.pr + 0.02f * sinf(t * 1.1f); centreBarre(s, pit, dvv, mir, x, y); ha = c.ha0 + 0.55f * (GP.th + 0.2f);
  } else {      // sans barre : il reste en l'air devant la vitre, ailes ouvertes
    y += 2.5f * sinf(t * 3.1f);
    if (GP.ph == 3) { float q = lim(t / GP.dur, 0.0f, 1.0f); dvv = mir * lerpf(0.55f * cosf(1.6f * 3.1416f * q), 0.45f, pas((q - 0.75f) / 0.25f)); }
    if (GP.ph == 4) {
      float cyc = 0.9f, tc = t - GP.nf * cyc, sc = 1.0f;
      if (tc < 0.34f) { float e = pas(tc / 0.34f); sc = 1.0f - 0.05f * e; pk = -0.7f * e; }
      else if (tc < 0.42f) { float u = (tc - 0.34f) / 0.08f; sc = lerpf(0.95f, 1.22f, u); pk = lerpf(-0.7f, 1.0f, u); dvv = mir * lerpf(0.45f, 0.30f, u); }
      else { float u = pas((tc - 0.42f) / 0.48f); sc = lerpf(1.22f, 1.0f, u); pk = lerpf(1.0f, 0.0f, u); dvv = mir * lerpf(0.30f, 0.45f, u); }
      s = HS * sc; GP.sc = sc; GP.pk = pk;
    }
  }
  GP.px = x; GP.py = y; GP.ps = s; GP.dvv = dvv; GP.pit = pit;
  const bool perch = (GP.ph == 3 || GP.ph == 4) && barre;
  float cp = (accroupi || perch) ? 0.0f : coup(c.phase), cpl = (accroupi || perch) ? 0.0f : coup(fmodf(c.phase + 0.93f, 1.0f));
  c.etat = E_VOL; c.visible = true; c.flash = 0; c.objet = 0; c.duvet = 0; c.hum = 0; c.pied = pied; c.fx = fxp; c.fy = fyp; c.fp = 0; c.sq = 1.0f; c.ro = (accroupi || perch || GP.ph == 2) ? 0.0f : 0.10f * sinf(t * 2.2f);
  c.s = s; c.x = x; c.y = y; c.dv = dvv; c.d = (dvv >= 0.0f) ? 1 : -1; c.pitch = pit; c.fan = fan; c.wl = wl;
  if (perch) { c.pose = A_PLANE; c.poseP = A_PLANE; c.bob = 0.3f * s * sinf(t * 2.7f); }
  else if (pose) { c.pose = mixA(mixA(A_HAUT, A_BAS, cp), A_FLARE, pas(lim(t / GP.dur, 0.0f, 1.0f))); c.poseP = c.pose; c.bob = 0.0f; }
  else { c.poseP = mixA(A_HAUT, A_BAS, cpl); c.pose = mixA(A_HAUT, A_BAS, cp); c.bob = accroupi ? 0.0f : -2.4f * s * (cp - 0.4f); }
  c.ha = ha + (perch ? 0.0f : 0.5f * GP.th) + 0.30f * pk; c.hb = c.ha + hbO + 0.25f * pk; c.bec = (GP.ph == 4) ? lim(-pk * 0.3f, 0.0f, 0.3f) : lim(GP.ouv * 2.6f, 0.0f, 1.0f); c.clin = GP.cl;
}
static void barreMetal(M5Canvas& g, float sl) {      // barre de métal très noire, un peu angulaire, avec un léger liseré rouge sombre : elle glisse depuis le bas de l'écran
  if (sl <= 0.01f) return;
  const float oy = (1.0f - sl) * 40.0f, Y = SILL_B + oy, B = mnf(Y + 40.0f, 205.0f);
  const uint16_t NOIR = C(3, 2, 5), FAC = C(15, 4, 9), RIM = C(110, 22, 28), RIM2 = C(54, 9, 15);
  static const float P[10][2] = {{-6, 10}, {12, 3}, {30, 0}, {104, 0}, {116, -4}, {204, -4}, {216, 0}, {290, 0}, {308, 3}, {326, 10}};      // profil : (x, hauteur du dessus par rapport à SILL_B)
  for (int i = 0; i < 9; i++) {
    float xa = P[i][0], ya = Y + P[i][1], xb = P[i + 1][0], yb = Y + P[i + 1][1];
    tri(g, xa, ya, xb, yb, xb, B, NOIR); tri(g, xa, ya, xb, B, xa, B, NOIR);
    tri(g, xa, ya, xb, yb, xb, yb + 5.0f, FAC); tri(g, xa, ya, xb, yb + 5.0f, xa, ya + 5.0f, FAC);      // chanfrein clair sous l'arête
    ligne(g, xa, ya, xb, yb, RIM); ligne(g, xa, ya + 1.5f, xb, yb + 1.5f, RIM2);
  }
  for (int i = 1; i < 9; i++) if (P[i][1] != P[i - 1][1]) ligne(g, P[i][0], Y + P[i][1], P[i][0], Y + P[i][1] + 9.0f, RIM2);      // arêtes verticales aux angles
  for (int x = 44; x < 300; x += 52) { ligne(g, (float)x, Y + 10.0f, (float)x, Y + 16.0f, RIM2); disque(g, (float)x, Y + 8.0f, 1.2f, RIM2); }      // fentes et rivets
}
static void dessinerGP(M5Canvas& g) {      // dessiné avec les autres éléments (boîtes englobantes) : seule la zone du corbeau est redessinée et envoyée à l'écran
  if (!GP.ph) return;
  const float szs = zs, sztx = ztx, szty = zty;      // la visite se dessine en pixels écran (zoom 1), le monde reprend ensuite sa place
  float shk = (GP.choc > 0.2f) ? GP.choc : 0.0f;
  if (GP.ph == 1) { float u = lim(GP.t / GP.dur, 0.0f, 1.0f); shk += 1.2f * u * u * u; } else if (GP.ph <= 4) shk += 0.4f;      // la caméra tremble de plus en plus près
  zs = 1.0f; ztx = shk * sinf(GP.t * 71.0f); zty = shk * 0.7f * cosf(GP.t * 57.0f);
  if (OP.barre) { float sl = (GP.ph == 1) ? pas((GP.t - 0.5f) / 0.7f) : ((GP.ph <= 4) ? 1.0f : 1.0f - pas(GP.t / 0.5f)); barreMetal(g, sl); }
  if (OP.barre && !qDeg && (GP.ph == 3 || GP.ph == 4)) {      // 3.1.38 : le dessus de la barre accroche la lumière de la lune (côté droit) et le corbeau y porte son ombre (vers la gauche)
    const float fxs = 160.0f - GP.mir * 16.0f, hs = vcb.s * 0.36f;
    ligne(g, 118.0f, SILL_B - 3.0f, 202.0f, SILL_B - 3.0f, C(58, 18, 24)); ligne(g, 118.0f, SILL_B - 2.0f, 202.0f, SILL_B - 2.0f, C(36, 11, 17)); ligne(g, 160.0f, SILL_B - 3.0f, 202.0f, SILL_B - 3.0f, C(92, 30, 36));
    ell(g, fxs - 9.0f * hs, SILL_B - 2.4f, 30.0f * hs, 2.3f, C(1, 0, 2));
  }
  dessinerCorbeau(g, vcb);
  { float f = (GP.ph == 5) ? 1.0f - pas(GP.t / (GP.dur * 0.5f)) : 1.0f;      // fissures dans la vitre : de plus en plus grandes à chaque coup
    if (f > 0.02f) for (int k = 0; k < GP.fis && k < 3; k++) {
      uint16_t cl = CF(225.0f * f, 200.0f * f, 200.0f * f);
      for (int i = 0; i < 6; i++) {
        float a = k * 0.7f + i * 1.047f + ((GP.gr >> (i * 3)) & 7) * 0.08f, ln = 12.0f + 9.0f * k + ((GP.gr >> (i * 2 + k)) & 3) * 5.0f, ca = cosf(a), sa = sinf(a);
        float mx = GP.ix[k] + ca * ln * 0.55f, my = GP.iy[k] + sa * ln * 0.55f, b = a + ((i & 1) ? 0.5f : -0.5f);
        ligne(g, GP.ix[k], GP.iy[k], mx, my, cl); ligne(g, mx, my, mx + cosf(b) * ln * 0.45f, my + sinf(b) * ln * 0.45f, cl);
      }
    }
    if (GP.fis > 0 && GP.tChoc < 0.35f) {      // choc : éclair blanc, puis anneaux autour du point d'impact
      int k = (GP.fis > 3 ? 3 : GP.fis) - 1; float u = GP.tChoc / 0.35f, rr = 8.0f + 44.0f * u; uint16_t cl = CF(235 * (1 - u), 205 * (1 - u), 205 * (1 - u));
      if (GP.tChoc < 0.09f) { float q = 1.0f - GP.tChoc / 0.09f; disque(g, GP.ix[k], GP.iy[k], 6.0f + 14.0f * q, CF(255, 150 + 90 * q, 140 + 90 * q)); disque(g, GP.ix[k], GP.iy[k], 3.0f + 5.0f * q, C(255, 250, 245)); }
      for (int r = 0; r < 2; r++) { float rad = r ? rr * 0.62f : rr, px = GP.ix[k] + rad, py = GP.iy[k];
        for (int i = 1; i <= 18; i++) { float a = i * 0.34907f, nx = GP.ix[k] + cosf(a) * rad, ny = GP.iy[k] + sinf(a) * rad; ligne(g, px, py, nx, ny, cl); px = nx; py = ny; } }
    } }
  zs = szs; ztx = sztx; zty = szty;
}
// ---------- logique ----------
static float perchY(int i) { return i < 4 ? ytop(PERCH[i][0]) + 0.4f : PERCH[i][1]; }
static float perchP(int i) { static const float PT_[2] = {-0.33f, 0.43f}; return i < 4 ? (ytop(PERCH[i][0] + 4) - ytop(PERCH[i][0] - 4)) / 8.0f : PT_[i - 4]; }
static bool perchOccupe(int i) { if (V.ph != V_RIEN && (i == 1 || i == 2)) return true; for (int k = 0; k < NCB; k++) if (cr[k].perch == i) return true; return false; }
static int perchLibre() {
  int n = 0, l[NPE]; for (int i = 0; i < NPE; i++) if (!perchOccupe(i)) l[n++] = i;
  return n ? l[random(n)] : -1;
}
static int nbDehors() { int n = 0; for (int k = 0; k < NCB; k++) if (cr[k].etat == E_DEHORS || (cr[k].etat == E_VOL && cr[k].but == B_SORTIR) || cr[k].but == B_MONDE) n++; return n; }

static int perchPour(const Corbeau& c);      // famille.h : perchoir attitré selon le rang
static float rangPoids(int k);               // famille.h : les mieux classés sont servis d'abord
static void posture(Corbeau& c) {      // posture perchée : tronc redressé (45 à 65 degrés), poitrine en avant, queue qui pend, tête dégagée
  switch (random(4)) {
    case 0: c.pr = 0.85f; c.tq = 0.20f; c.plume = 34; c.wd = 1.1f; break;
    case 1: c.pr = 1.00f; c.tq = 0.12f; c.plume = 30; c.wd = 0.9f; break;
    case 2: c.pr = 0.78f; c.tq = 0.28f; c.plume = 38; c.wd = 1.2f; break;
    default: c.pr = 1.10f; c.tq = 0.10f; c.plume = 32; c.wd = 1.0f; break;
  }
  c.pr += rnd(-0.04f, 0.04f); c.ha0 = c.pr - 0.35f + rnd(-0.08f, 0.08f);
}
static void initCorbeau(Corbeau& c, float s) {
  memset(&c, 0, sizeof(c)); c.s = s; c.d = 1; c.dv = 1; c.visible = true; c.perch = -1; c.item = -1;
  c.pj = -1; c.pCal = rnd01(); c.pBav = rnd01(); c.pJou = rnd01(); c.pCra = rnd01(); posture(c); c.pitch = c.pitchC = c.pr; c.ha = c.haC = c.ha0; c.hb = c.hbC = c.ha0 + 0.49f;
  c.pose = c.poseC = c.poseP = A_PLANE; c.sq = 1.0f; c.tClin = rnd(1.0f, 4.0f); c.resp = rnd(0, 6.28f); c.act = A_IMMO; c.dAct = rnd(1.0f, 5.0f);
}
static void placer(Corbeau& c, int i) {      // posé sur la branche : les pattes sur le dessus, le corps déduit
  c.perch = i; c.fx = PERCH[i][0]; c.fy = perchY(i); c.fp = perchP(i); c.pied = true; c.vx = c.vy = 0; c.etat = E_PERCHE; c.but = B_RIEN; c.t = 0; c.tfin = rnd(4, 11);
  c.visible = true; c.wl = c.wlC = 0; c.fan = c.fanC = 0; c.hop = 0; c.bob = 0; c.sub = 0; c.dv = (float)c.d; c.pitch = c.pitchC = c.pr; c.pv = 0; c.ox = c.oy = 0;
  c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.0f, 5.0f); c.grat = c.gratC = 0; c.ha = c.haC = c.ha0; c.hb = c.hbC = c.ha0 + 0.49f; c.hav = c.hbv = 0;
  float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy); c.x = c.fx - hx; c.y = c.fy - JAMBE * c.s - hy;
}
static void decoller(Corbeau& c, float tx, float ty, uint8_t but) {      // pour B_SORTIR tx,ty = point à atteindre ; sinon = appui des pattes visé
  c.etat = E_DECOLLE; c.t = 0; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.sub = 0; c.act = A_REPOS; c.tAct = 0;
  c.d = (tx >= c.x) ? 1 : -1; c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = 0.9f;
  if (natAile((int)(&c - cr))) { c.vitesse *= 0.72f; c.hz *= 0.84f; }      // 3.1.41 : aile blessee : plus lent, battements plus lents
}
static void entrer(Corbeau& c, float tx, float ty, uint8_t but) {      // arrive par un bord de l'écran
  bool gauche = (tx < 160) ? (rnd01() < 0.7f) : (rnd01() < 0.3f);
  c.x = gauche ? bordG() : bordD(); c.y = rnd(18, 110); c.d = gauche ? 1 : -1; c.dv = (float)c.d; c.vx = c.d * 90.0f; c.vy = 0; c.pied = false;
  c.etat = E_VOL; c.t = 1.5f; c.tx = tx; c.ty = ty; c.but = but; c.aWp = false; c.visible = true; c.sub = 0; c.act = A_REPOS;
  c.vitesse = rnd(82, 108); c.hz = rnd(2.7f, 3.0f); c.restBat = 4 + random(3); c.planeT = 0; c.phase = rnd01(); c.wl = c.wlC = 1; c.fan = c.fanC = 0; c.pitch = c.pitchC = 0.1f; c.pose = c.poseC = A_BAS;
}
static void sortir(Corbeau& c) {
  if (c.etat != E_DEHORS && mo::partir(c)) return;      // monde.h : la sortie devient une excursion (maison, mare, champ)
  c.perch = -1; c.item = -1;
  float tx = (rnd01() < 0.5f) ? bordG() - 16.0f : bordD() + 16.0f;
  if (c.x < 100) tx = bordG() - 16.0f; else if (c.x > 220) tx = bordD() + 16.0f;
  decoller(c, tx, rnd(8, 90), B_SORTIR);
}
static void versPerchoir(Corbeau& c) {
  int i = perchPour(c);
  if (i < 0) { if (nbDehors() < 3) sortir(c); else { c.t = 0; if (c.etat == E_SOL) { c.but = B_RIEN; c.sub = 2; } } return; }
  c.perch = i; c.item = -1; decoller(c, PERCH[i][0], perchY(i), B_PERCHOIR);
}
static void apresSol(Corbeau& c) {
  if (rnd01() < 0.3f && nbDehors() < 2) sortir(c); else versPerchoir(c);
}

static void attribuer(int iO, int force = -1);
static void lacher(int type, float x, float y0 = -6.0f) {       // une nourriture tombe du ciel (ou de l'endroit touché)
  int idx = -1; for (int i = 0; i < NOB; i++) if (!ob[i].actif) { idx = i; break; }
  if (idx < 0) { float vieux = -1; for (int i = 0; i < NOB; i++) if (ob[i].t > vieux && ob[i].repos) { vieux = ob[i].t; idx = i; } if (idx < 0) return; }
  if (!stockPrend(type)) return;      // 3.1.38 : viande et noix sont rares (stock qui se renouvelle)
  Obj& o = ob[idx]; o.actif = true; o.type = (uint8_t)type; o.x = lim(x, 12, 308); o.y = (y0 < 0.0f) ? -6.0f : mnf(y0, solHaut(o.x) - 3.2f); o.vy = (y0 < 0.0f) ? 20.0f : 0.0f; o.t = 0; o.repos = false; o.reserve = -1;
  for (int k = 0; k < NCB; k++) if (cr[k].item == idx) { cr[k].item = -1; if (cr[k].but == B_NOUR) cr[k].but = B_RIEN; }
  son(SN_CROC, 110);
  for (int k = 0; k < NCB; k++) {      // les corbeaux perchés tournent la tête vers la nourriture tout de suite
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible) continue;
    c.d = (o.x >= c.x) ? 1 : -1; c.look = lim(atan2f(solHaut(o.x) - c.y, fabsf(o.x - c.x) + 8.0f) * 0.7f, -0.3f, 0.8f); c.tLook = 1.6f; c.clin = 0; c.tClin = rnd(1.5f, 3.0f);
  }
  { int fk = cibleK; cibleK = -1; attribuer(idx, fk); if (ob[idx].reserve < 0 && gRassasie && fk < 0) infoMsg("Ils n'ont pas faim", "Ils reviendront dès qu'ils auront faim"); } mo::compet(idx);      // et l'un d'eux (celui qu'on a désigné par son nom, sinon le plus proche) part sans attendre ; s'il n'y a qu'un gros morceau, un rival vient le disputer
}

static void attribuer(int iO, int force) {       // un corbeau libre vient chercher la nourriture (3.1.38 : seulement s'il a faim ; son plat prefere attire davantage)
  Obj& o = ob[iO]; int best = -1; float bd = 1e9f; gRassasie = false;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    bool ok = (c.etat == E_PERCHE && c.objet == 0 && c.perch < 10) || c.etat == E_DEHORS || (c.etat == E_SOL && c.but == B_RIEN);
    if (!ok || c.but == B_NOUR || c.rapporte) continue;
    const float ev = mo::envie(k, o.type); if (ev <= 0.0f && k != force) { gRassasie = true; continue; }
    float px = (c.etat == E_DEHORS) ? 160.0f + (k % 2 ? 200.0f : -200.0f) : c.x; float dd = fabsf(px - o.x) + rnd(0, 25) + rangPoids(k) - ((ev > 1.2f) ? 45.0f : 0.0f);
    if (dd < bd) { bd = dd; best = k; }
  }
  if (force >= 0 && force < NCB) { const Corbeau& cf = cr[force]; if (((cf.etat == E_PERCHE && cf.objet == 0 && cf.perch < 10) || cf.etat == E_DEHORS || (cf.etat == E_SOL && cf.but == B_RIEN)) && cf.but != B_NOUR && !cf.rapporte) best = force; }
  if (best < 0) return;
  Corbeau& c = cr[best]; float x0 = (c.etat == E_DEHORS) ? ((best % 2) ? bordD() : bordG()) : c.x;
  int dsg = (o.x >= x0) ? 1 : -1;
  float gx = lim(o.x - dsg * 19.0f * c.s, 10, 310), gy = solHaut(gx);
  c.perch = -1; c.item = iO; o.reserve = best; c.objet = 0;
  if (c.etat == E_PERCHE || c.etat == E_SOL) { c.sub = 0; decoller(c, gx, gy, B_NOUR); }
  else { entrer(c, gx, gy, B_NOUR); }
  c.item = iO;
}

static void braise() {      // une braise sombre-rouge monte de la paume (4 au plus en qualité Fluide)
  const int nmax = (OP.q == 0) ? 4 : 6; RepM r_ = repereMain(); float px, py; ptPaume(r_, px, py);
  for (int q = 0; q < nmax; q++) if (!BRAIS[q].on) { Braise& b = BRAIS[q]; b.on = true; b.x = px + rnd(-9.0f, 9.0f); b.y = py - 2.0f; b.vx = rnd(-8.0f, 8.0f); b.vy = -rnd(14.0f, 32.0f); b.t = 0.0f; b.dur = rnd(0.9f, 1.5f); break; }
}
static void mainRecoit() {      // le corbeau pose ses pattes dans la paume : elle cède un peu, une gerbe de braises
  H.dipV += 46.0f; H.halo = 1.0f; for (int i = 0; i < 4; i++) braise();
}
static void atterrir(Corbeau& c) {
  if (c.but == B_VISITEUR && c.perch >= 10) { ptVis(c.perch - 10, c.fx, c.fy); c.fp = 0; }
  else if (c.but == B_PERCHOIR && c.perch >= 0) { c.fx = PERCH[c.perch][0]; c.fy = perchY(c.perch); c.fp = perchP(c.perch); }
  else { c.fx = c.tx; c.fy = c.ty; c.fp = (c.but == B_PERCHOIR && c.fx > 66 && c.fx < 236 && fabsf(c.fy - ytop(c.fx)) < 4.0f) ? (ytop(c.fx + 4) - ytop(c.fx - 4)) / 8.0f : 0.0f; }
  float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy);
  c.ox = c.x - (c.fx - hx); c.oy = c.y - (c.fy - JAMBE * c.s - hy);      // l'écart restant se résorbe en douceur
  c.pied = true; c.etat = E_ATTERRIT; c.t = 0; c.vx = c.vy = 0; son(SN_ATTERRIT, 170);
  if (c.but == B_VISITEUR && c.perch == 14) mainRecoit();
}

// ----- moments de calme d'un corbeau perché : immobile, regarde autour, lisse ses plumes, se gratte, se secoue -----
// ----- moments de la vie d'un corbeau perché : immobile, regarde, lisse, gratte, se secoue, s'étire, appelle, dort, descend picorer ou se poudrer -----
static void descendre(Corbeau& c, uint8_t but) {      // il descend au sol pour picorer ou se poudrer
  float gx = rnd(100.0f, 300.0f); c.perch = -1; decoller(c, gx, solHaut(gx), but);
}
static void prochainActe(Corbeau& c) {
  uint8_t prec = c.act; c.tAct = 0; bool lib = gCalme <= 0;
  if ((prec == A_LISSE_DOS || prec == A_LISSE_POIT) && random(100) < 45) { c.act = A_SECOUE; c.dAct = 0.9f; return; }
  bool sol = (c.perch >= 0 && c.perch < 4 && V.ph == V_RIEN && !appelOn && nbDehors() < 2 && c.duvet < 0.5f && !natPatte((int)(&c - cr)));      // patte blessee : ne descend plus chasser au sol
  float w[11];      // poids de chaque activité, selon la personnalité (calme, bavard, joueur, craintif) et l'humeur
  w[0] = 0; w[1] = 18 + 30 * c.pCal; w[2] = 14 + 14 * c.pJou + 10 * c.pCra + 20 * c.hum; w[3] = lib ? 14 + 18 * c.pCal : 0; w[4] = lib ? 8 : 0; w[5] = lib ? 5 : 0;
  w[6] = lib ? 6 + 6 * c.pJou : 0; w[7] = (OP.amb == 2) ? 30 + 30 * c.pCal : 0; w[8] = 4 + 14 * c.pBav + 10 * c.hum; w[9] = sol ? 6 + 14 * c.pJou : 0; w[10] = sol ? 3 + 6 * c.pJou : 0;
  if (cbAbsent || pr::calme()) { w[1] *= 2.5f; w[2] *= 0.15f; w[3] = w[4] = w[5] = w[6] = w[8] = w[9] = w[10] = 0; if (w[7] < 35) w[7] = 35; }      // presence : absence = presque immobile, parfois endormi
  float tot = 0; for (int i = 1; i <= 10; i++) tot += w[i];
  float r = rnd01() * tot; int k = 1; for (; k < 10; k++) { if (r < w[k]) break; r -= w[k]; }
  switch (k) {
    case 1: c.act = A_IMMO; c.dAct = rnd(3, 8); break;
    case 2: c.act = A_REGARDE; c.dAct = rnd(3, 6); c.sub = -1; break;
    case 3: c.act = random(2) ? A_LISSE_DOS : A_LISSE_POIT; c.dAct = rnd(3.0f, 4.5f); gCalme = c.dAct + rnd(2, 5); break;
    case 4: c.act = A_GRATTE; c.dAct = rnd(1.9f, 2.6f); gCalme = c.dAct + rnd(2, 5); break;
    case 5: c.act = A_SECOUE; c.dAct = 0.9f; gCalme = c.dAct + rnd(1, 3); break;
    case 6: c.act = A_ETIRE; c.dAct = 1.8f; gCalme = c.dAct + rnd(1, 3); break;
    case 7: c.act = A_DORT; c.dAct = rnd(8, 16); break;
    case 8: c.act = A_APPEL; c.dAct = 1.4f; break;
    case 9: c.act = A_REPOS; c.dAct = 1.0f; descendre(c, B_FOURRAGE); break;
    default: c.act = A_REPOS; c.dAct = 1.0f; descendre(c, B_BAIN); break;
  }
}
static void acteur(Corbeau& c, float dt) {
  c.tAct += dt;
  if (tAlerte > 0 && c.act != A_DORT) { c.haC = c.ha0 - 0.5f; c.hbC = c.haC + 0.49f; c.pitchC = c.pr + 0.1f; c.d = (int8_t)dirAlerte; c.clin = 0; return; }      // alerte : tous regardent dans la même direction
  if (c.tAct >= c.dAct) prochainActe(c);
  float a = c.tAct, dA = c.dAct;
  float e = pas(a / 0.6f) * (1.0f - pas((a - (dA - 0.6f)) / 0.6f));
  switch (c.act) {
    case A_IMMO: c.haC = c.ha0 + 0.015f * sinf(c.resp * 0.5f); c.hbC = c.haC + 0.49f; break;
    case A_REGARDE: {
      static const float AN[4] = {-0.5f, 0.1f, -0.25f, 0.3f};
      int k = ((int)(a / 1.3f) + (int)(c.s * 10)) & 3;
      if (k != c.sub) { c.sub = k; if (k == 2 && random(100) < 45) { c.d = -c.d; c.surs = 0.22f; } }      // il se retourne d'un petit saut
      c.haC = c.ha0 + AN[k]; c.hbC = c.haC + 0.49f; c.pitchC = c.pr + 0.05f * sinf(a * 3.0f);
    } break;
    case A_LISSE_DOS: c.haC = lerpf(c.ha0, -2.55f, e); c.hbC = lerpf(c.ha0 + 0.49f, 3.15f, e) + 0.28f * e * sinf(a * 17.0f); c.pitchC = c.pr + 0.05f * e * sinf(a * 8.5f); c.bob += 0.4f * c.s * e * sinf(a * 17.0f); break;
    case A_LISSE_POIT: c.haC = lerpf(c.ha0, 1.95f, e); c.hbC = lerpf(c.ha0 + 0.49f, 2.2f, e) + 0.25f * e * sinf(a * 15.0f); c.pitchC = c.pr - 0.10f * e; break;
    case A_GRATTE: { float g = pas(a / 0.35f) * (1.0f - pas((a - (dA - 0.35f)) / 0.35f)); c.gratC = g; c.haC = lerpf(c.ha0, -0.15f, g); c.hbC = lerpf(c.ha0 + 0.49f, 0.4f, g); c.pitchC = c.pr - 0.07f * g; } break;
    case A_SECOUE: c.ruf = sinf(3.1416f * a / dA); c.pitchC = c.pr + 0.06f * c.ruf * sinf(a * 38.0f); c.haC = c.ha0; c.hbC = c.ha0 + 0.49f; break;
    case A_ETIRE: {      // il s'étire : ailes qui s'ouvrent, tête levée
      float g = pas(a / 0.5f) * (1.0f - pas((a - (dA - 0.5f)) / 0.5f));
      c.wlC = 0.85f * g; c.poseC = A_PLANE; c.pitchC = c.pr - 0.15f * g; c.haC = c.ha0 - 0.3f * g; c.hbC = c.haC + 0.49f;
    } break;
    case A_DORT: c.haC = -2.55f; c.hbC = 3.2f; c.clin = 1.0f; c.pitchC = c.pr + 0.1f; break;      // il dort, la tête sous l'aile
    case A_APPEL: c.haC = c.ha0 - 0.5f; c.hbC = c.haC + 0.49f; c.bec = 0.5f + 0.5f * sinf(a * 14.0f); if (a - dt <= 0.0f) sonCri(); break;      // il appelle
    default: c.haC = c.ha0 + 0.07f * sinf(c.resp * 0.8f + c.s * 7.0f); c.hbC = c.haC + 0.49f; break;
  }
}

static void relacherMain(int k, float x, float y);
static void retourLibre(Corbeau& c) {      // le corbeau en vol cherche un perchoir libre, sinon il sort et reviendra
  int i = perchPour(c);
  if (i >= 0) { c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); c.aWp = false; }
  else { c.perch = -1; c.but = B_SORTIR; c.tx = (c.x < 160) ? bordG() - 16.0f : bordD() + 16.0f; c.ty = rnd(8, 90); c.aWp = false; c.fuite = true; }
}
static void naissance(float x, float y, bool rare);
static void minisEnvol(float x, float y, int n);
static bool petitGrand();      // famille.h : le petit est grand (ou la famille coupée) : un nouvel œuf peut le remplacer
static void designerParent(int k);
static bool oeufAF();      // famille.h : effectif réel (un petit ne naît que d'un œuf attrapé)
static bool placeNaiss();      // famille.h : le corbeau k sera le parent attitré du prochain petit
static void oeufAttrape(Corbeau& c) {      // l'œuf lancé est attrapé en vol : il ne casse pas, il éclot en nouveau corbeau (ou en petits corbeaux s'il n'y a plus de place)
  Proj& p = proj[c.pj]; ralenti(0.35f); p.actif = false; c.flash = 1.0f; c.bec = 1; c.surs = 0.3f;
  son(SN_CROC, 230); sonCri(); plumes(c.x, c.y, 10); eclat(c.x, c.y, 0.3f, 1.5f); vib(50);
  nbOeufs++; nbOeufsSale = true; tOeufsEcr = millis(); oeufsSale = true;
  int i = perchLibre();
  if ((oeufAF() ? placeNaiss() : (cr[PC].etat == E_OFF || petitGrand())) && i >= 0) { designerParent((int)(&c - cr)); naissance(PERCH[i][0], perchY(i), rnd01() < 0.2f); }      // celui qui attrape l'œuf devient le parent attitré du petit
  else { if (!oeufAF()) minisEnvol(c.x, c.y - 4.0f, 7); son(SN_POP_DISP, 190); }
  c.pj = -1; retourLibre(c);
}
static void attraper(Corbeau& c) {      // il attrape l'objet lancé, puis le rapporte sur une branche ou le dépose par terre
  if (proj[c.pj].type == T_OEUF) { oeufAttrape(c); return; }
  Proj& p = proj[c.pj]; ralenti(0.2f); c.objet = p.type + 1; p.actif = false; c.flash = 0.7f; c.bec = 1; son(SN_CROC, 210); plumes(c.x, c.y, 3);
  int i = perchLibre();
  if (rnd01() < 0.5f && i >= 0) { c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); }
  else { float gx = lim(p.ox, 24, 296); c.perch = -1; c.but = B_OBJET; c.tx = gx; c.ty = solHaut(gx); }
  c.aWp = false; c.pj = -1;
}

static uint8_t natSante(int k); static bool natMort(int k); static void natNourri(int k); static void natBagarre(int w, int l, int src); static bool natImmobile(const Corbeau& c); static void natSauver(bool force); static void natPromu(int k); static int natSlotLibre(); static void natHeritage(int p);      // nature.h (3.1.34)
#include "monde.h"
#include "famille.h"
#include "nature.h"

static void majCorbeau(Corbeau& c, int idx, float dt) {
  c.t += dt; c.resp += dt;
  c.tClin -= dt; if (c.tClin <= 0) { c.clin = 1; c.tClin = rnd(2.2f, 5.5f); }
  if (c.clin > 0) { c.clin -= dt * 4.5f; if (c.clin < 0) c.clin = 0; }
  if (c.bec > 0) { c.bec -= dt * 3.0f; if (c.bec < 0) c.bec = 0; }
  if (c.flash > 0) { c.flash -= dt * 4.0f; if (c.flash < 0) c.flash = 0; }
  if (c.tLook > 0) c.tLook -= dt;
  c.hum = mxf(0.0f, c.hum - dt * 0.05f);
  c.bob = 0; c.hop = 0; c.ruf = 0;
  c.haC = c.ha0; c.hbC = c.ha0 + 0.49f; c.gratC = 0; c.wlC = 0; c.fanC = 0; c.poseC = A_PLANE; c.pitchC = c.pr;
  if (c.surs > 0) { c.surs -= dt; if (c.surs < 0) c.surs = 0; if (c.etat == E_PERCHE || c.etat == E_SOL) c.bob = -3.2f * c.s * sinf(c.surs / 0.30f * 3.1416f); }

  switch (c.etat) {
    case E_PERCHE: {
      c.pied = true; c.vx = c.vy = 0;
      if (c.perch >= 10) { if (c.perch == 14 ? (!H.actif || c.t >= c.tfin) : V.ph == V_RIEN) { if (c.perch == 14 && H.actif) { H.tRec = delaiPaume(); H.dipV += 35.0f; H.halo = mxf(H.halo, 0.5f); } c.perch = -1; versPerchoir(c); break; } ptVis(c.perch - 10, c.fx, c.fy); c.fp = 0; }      // posé sur le visiteur : il en suit le balancement
      c.bob += 0.30f * c.s * sinf(c.resp * 2.7f);                   // respiration
      c.pitchC = c.pr + 0.015f * sinf(c.resp * 0.9f);               // léger balancement
      acteur(c, dt);
      if (c.perch == 14 && idx == mgK && mgT > 0.5f) { const float ph_ = fmodf(mgT * 2.4f, 1.0f), dp_ = pas(sinf(ph_ * 3.1416f)); c.haC = c.ha0 + 0.65f * dp_; c.hbC = c.haC + 0.49f; c.pitchC = c.pr - 0.2f * dp_; c.bec = (dp_ > 0.6f) ? 0.5f : 0.0f; }      // 3.1.42 : il becquete la nourriture posee dans la paume
      if (c.objet && c.t > 1.2f && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && mo::tresorDepart(c, (int)(&c - cr))) break;      // 3.1.30 : un objet brillant va toujours a la tour
      if (c.objet && c.t > 9.0f) { c.objet = 0; eclat(c.x + c.dv * 18 * c.s, c.y - 2, 0.3f, 0.7f); }
      if (c.perch < 10 && c.t * actFreq >= c.tfin && gAction <= 0 && c.act <= A_REGARDE && !fam::immobile(c)) {
        int r = random(100);
        if (r < 40) { versPerchoir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 62 && nbDehors() < 2 && !fam::enService(c)) { sortir(c); gAction = rnd(1.5f, 3.0f); }
        else if (r < 80) { c.d = -c.d; c.surs = 0.22f; c.t = 0; c.tfin = rnd(3, 9); }
        else { c.bec = 1; if (c.duvet > 0.5f) son(SN_GAZ, 200); else sonCri(); c.t = 0; c.tfin = rnd(4, 11); }
      }
    } break;
    case E_DECOLLE: {
      const bool aB = natAile(idx); float u = c.t / (aB ? 0.26f : 0.11f); c.poseC = A_HAUT; c.wlC = 1; c.pitchC = 0.55f; c.haC = -0.55f; c.hbC = -0.06f;
      c.bob = 2.6f * c.s * pas(u > 1 ? 1 : u);                // il s'accroupit avant de sauter
      if (c.t < 0.02f) son(SN_AILES, 200);
      if (c.t >= (aB ? 0.26f : 0.11f)) {      // aile blessee : decolle plus lentement, saut plus faible
        c.etat = E_VOL; c.t = 0; c.vx = c.d * 46.0f; c.vy = aB ? -62.0f : -105.0f; c.bob = 0; c.pied = false;
        if (c.gerbe) { c.gerbe = false; c.vx = rnd(-120, 120); c.vy = -rnd(110, 170); c.d = (c.vx >= 0) ? 1 : -1; c.dv = (float)c.d; }      // envol en gerbe c.perch = (c.but == B_PERCHOIR || c.but == B_VISITEUR) ? c.perch : -1;
        float dist = hypotf(c.tx - c.x, c.ty - c.y);
        if (c.but == B_SORTIR) { c.wx = c.x + c.d * 70.0f; c.wy = mnf(c.y - 50.0f, 60.0f) - rnd(0, 20); c.aWp = true; }
        else if (dist > 110 && c.but != B_ORBITE && c.but != B_ATTRAPE && c.but != B_POURSUITE) { c.wx = (c.x + c.tx) * 0.5f + rnd(-20, 20); c.wy = mnf(c.y, c.ty) - rnd(14, 50); c.aWp = true; }
      }
    } break;
    case E_VOL: {
      c.wlC = 1; c.pied = false;
      if (c.tOrb > 0) { c.tOrb -= dt; if (c.tOrb <= 0 && (c.but == B_ORBITE || c.but == B_POURSUITE)) { c.pj = -1; retourLibre(c); } }
      float gx, gy; bool orb = (c.but == B_ORBITE);
      if (orb) { c.ang += c.om * dt; float lead = (c.om > 0 ? 0.35f : -0.35f); float ox = c.ocg ? orbCx : c.ocx, oy = c.ocg ? orbCy : c.ocy; gx = ox + c.ray * cosf(c.ang + lead); gy = oy + c.ray * 0.5f * sinf(c.ang + lead); }
      else if (c.but == B_POURSUITE) { Corbeau& tc = cr[c.pj >= 0 ? c.pj : 0]; gx = tc.x; gy = tc.y; }
      else if (c.but == B_ATTRAPE) {
        if (c.pj >= 0 && proj[c.pj].actif) { gx = proj[c.pj].x + proj[c.pj].vx * 0.15f; gy = proj[c.pj].y + proj[c.pj].vy * 0.15f; }
        else { retourLibre(c); if (c.but == B_PERCHOIR) cible(c, c.tx, c.ty, (float)c.d, gx, gy); else { gx = c.tx; gy = c.ty; } }
      }
      else if (c.aWp) { gx = c.wx; gy = c.wy; } else if (c.but == B_SORTIR) { gx = c.tx; gy = c.ty; }
      else { if (c.but == B_VISITEUR && c.perch >= 10) ptVis(c.perch - 10, c.tx, c.ty); cible(c, c.tx, c.ty, (float)c.d, gx, gy); }
      float dx = gx - c.x, dy = gy - c.y, dist = sqrtf(dx * dx + dy * dy); if (dist < 0.001f) dist = 0.001f;
      if (c.aWp && dist < 18) { c.aWp = false; break; }
      bool fin = !c.aWp && !orb && c.but != B_ATTRAPE && c.but != B_POURSUITE, proche = fin && dist < ((c.but == B_VISITEUR && c.perch == 14) ? 54.0f : 38.0f) && c.but != B_SORTIR;
      float sp = fin ? ((c.but == B_VISITEUR && c.perch == 14) ? lim(dist * 1.9f + 12.0f, 12.0f, c.vitesse) : lim(dist * 2.4f + 16.0f, 16.0f, c.vitesse)) : c.vitesse;      // vers la paume : il ralentit plus longtemps
      if (c.but == B_SORTIR) sp = c.vitesse * 1.05f;
      if (orb) sp = 125.0f;
      if (c.but == B_ATTRAPE) sp = (c.pj >= 0 && proj[c.pj].type == T_OEUF) ? 275.0f : 190.0f;      // pour un œuf, ils foncent plus vite : on peut l'attraper de justesse
      if (c.but == B_POURSUITE) sp = 150.0f;
      float k = mnf(1.0f, dt * (c.but == B_ATTRAPE ? 7.0f : (c.but == B_POURSUITE ? 5.0f : 3.0f)));
      c.vx += (dx / dist * sp - c.vx) * k; c.vy += (dy / dist * sp - c.vy) * k;
      if (natAile(idx) && !orb) c.vy += 16.0f * dt;      // aile blessee : vole bas, perd de la hauteur
      { float tvx = dx / dist * sp, ref = (fabsf(tvx) > 14.0f) ? tvx : c.vx;      // sens voulu : le corps pivote dès que la direction change
        if (proche) { if (fabsf(dx) > 8.0f) c.d = (dx > 0) ? 1 : -1; } else if (fabsf(ref) > 14.0f) c.d = (ref > 0) ? 1 : -1; }
      { float lo = -(14.0f + 300.0f * mxf(0.0f, -c.dv)), hi = 14.0f + 300.0f * mxf(0.0f, c.dv);      // jamais de marche arrière : la vitesse contre le regard attend la fin du demi-tour
        if (c.vx < lo) c.vx = lo; else if (c.vx > hi) c.vx = hi; }
      c.x += c.vx * dt; c.y += c.vy * dt;
      c.pitchC = proche ? 0.62f : lim(-c.vy * 0.006f + 0.1f, -0.4f, 0.5f);
      { float hc = proche ? 0.0f : 0.55f * (c.pitchC - 0.1f); c.haC = -0.30f + hc; c.hbC = 0.19f + hc; }      // la tête reste presque à l'horizontale quand le corps s'incline
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
        if (p0 > 0.9f && c.phase < 0.1f) son(SN_AILES, natAile(idx) ? 55 : 120);      // battement affaibli si l'aile est blessee
        float cp = coup(c.phase), ph2 = c.phase - 0.10f; if (ph2 < 0) ph2 += 1.0f; float cpH = coup(ph2);
        c.poseC = mixA(A_HAUT, A_BAS, cp); c.poseC.eh = lerpf(A_HAUT.eh, A_BAS.eh, cpH); c.poseC.sh = lerpf(A_HAUT.sh, A_BAS.sh, cpH);      // la main (primaires) suit le poignet avec un léger retard
        float rec = (c.phase > 0.45f) ? sinf(3.1416f * lim((c.phase - 0.45f) / 0.5f, 0.0f, 1.0f)) : 0.0f; c.wlC = 1.0f - 0.18f * rec;      // remontée : ailes à demi repliées
        c.bob = -2.4f * c.s * (cp - 0.4f);
      }
      if (natAile(idx)) { c.poseC.ea *= 0.68f; c.poseC.eh *= 0.68f; c.wlC *= 0.92f; }      // battements amples reduits
      if (c.but == B_SORTIR) { if (c.x < bordG() + 2.0f || c.x > bordD() - 2.0f) { c.etat = E_DEHORS; c.t = 0; c.visible = false; c.tfin = c.fuite ? rnd(2.0f, 4.5f) : (c.rapporte ? rnd(3.5f, 6.0f) : rnd(6, 16)); c.fuite = false; c.wl = c.wlC = 0; } }
      else if (c.but == B_ATTRAPE && c.pj >= 0 && proj[c.pj].actif && hypotf(proj[c.pj].x - c.x, proj[c.pj].y - c.y) < (proj[c.pj].type == T_OEUF ? 14.0f : 11.0f)) attraper(c);
      else if (fin && (dist < 2.5f || (dist < 7.0f && sqrtf(c.vx * c.vx + c.vy * c.vy) < 20.0f))) atterrir(c);
    } break;
    case E_ATTERRIT: {
      if (c.perch == 14 && c.but == B_VISITEUR) ptVis(4, c.fx, c.fy);      // les pattes suivent la paume qui cède
      float u = c.t / 0.34f; c.pied = true;
      c.wlC = (c.t > 0.06f) ? 0.0f : 1.0f; c.poseC = A_FLARE; c.fanC = (u < 0.55f) ? 1.0f : 0.0f;
      c.bob = 1.5f * c.s * sinf(lim(u, 0, 1) * 3.1416f);
      if (c.t >= 0.34f) {
        c.t = 0; c.wlC = 0; c.act = A_IMMO; c.tAct = 0; c.dAct = rnd(1.5f, 4.0f);
        if (c.but == B_PERCHOIR) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(5, 13); c.ox = c.oy = 0; posture(c); }
        else if (c.but == B_VISITEUR) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(5, 13); c.ox = c.oy = 0; c.d = (c.perch == 14) ? (H.gauche ? 1 : -1) : ((c.perch == 10) ? -1 : 1); if (c.perch == 14) posture(c); }
        else if (c.but == B_NOUR || c.but == B_OBJET || c.but == B_FOURRAGE || c.but == B_BAIN) { c.etat = E_SOL; c.sub = 0; c.peck = 0; c.om = 0; }
        else if (c.but == B_MONDE) { mo::arrivee(c); }
        else if (c.but == B_FAM) { fam::arrive(c, idx); }
        else { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; }
      }
    } break;
    case E_SOL: {
      c.pied = true; c.pitchC = c.pr * 0.45f; c.haC = c.pitchC - 0.35f; c.hbC = c.haC + 0.49f; if (c.but == B_MONDE) mo::sol(c, idx, dt); else if (c.but == B_FAM) fam::sol(c, idx, dt);
      if (c.but == B_NOUR && c.sub <= 1) {
        Obj* o = (c.item >= 0 && c.item < NOB && ob[c.item].actif && ob[c.item].reserve == idx) ? &ob[c.item] : nullptr;
        if (!o) { c.but = B_RIEN; c.item = -1; apresSol(c); break; }
        if (!o->repos) { c.d = (o->x >= c.x) ? 1 : -1; c.haC = -0.15f; c.hbC = 0.34f; break; }      // la nourriture tombe encore : il attend
        float gx = o->x - c.d * 19.0f * c.s;
        if (c.sub == 0) {
          float dxm = gx - c.x; c.d = (o->x >= c.x) ? 1 : -1;
          if (fabsf(dxm) < 1.8f) { c.sub = 1; c.peck = 0; }
          else { { const bool pB = natPatte(idx); c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), (pB ? 16.0f : 34.0f) * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f)); c.hop = -fabsf(sinf(c.t * (pB ? 6.0f : 11.0f))) * (pB ? 1.4f : 2.6f) * c.s; } c.bob = c.hop; c.pitchC = c.pr * 0.6f; c.haC = -0.2f; c.hbC = 0.3f; }
        } else if (c.sub == 1) {
          c.peck += dt; float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f));
          c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
          if (c.peck > 0.24f && o->actif && o->reserve == idx && mo::peutPrendre(c.item)) {      // il prend la nourriture
            son(SN_CROC, 200); if (o->type < T_BRILLANT && fam::detourner(c, idx)) { o->actif = false; c.item = -1; break; }      // le parent (ou un aidant) garde le morceau pour le petit
            mo::mange(c, idx, o->type);
            if (o->type >= T_BRILLANT) { c.objet = o->type + 1; o->actif = false; c.item = -1; c.sub = 2; c.peck = 0; c.t = 0; c.flash = 0.5f; }
            else { o->actif = false; c.item = -1; c.sub = 3; c.peck = 0; }
          }
        }
      }
      if (c.but == B_NOUR && c.sub == 3) {       // il picore ce qu'il a pris
        c.peck += dt; float dip = fabsf(sinf(c.peck / 0.40f * 3.1416f)); c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
        if (c.peck > 0.9f && c.peck - dt <= 0.9f) { son(SN_GAZ, 150); }
        if (c.peck > 1.8f) { c.but = B_RIEN; apresSol(c); }
      } else if (c.but == B_NOUR && c.sub == 2) { c.peck += dt; if (c.peck > 0.7f) { c.but = B_RIEN; if (!mo::tresorDepart(c, idx)) apresSol(c); } }
      else if (c.but == B_OBJET) {
        if (c.sub == 0) { if (!mo::tresorDepart(c, idx)) { c.sub = 1; c.peck = 0; c.d = (c.x < 160) ? 1 : -1; } }      // 3.1.30 : brillant = tour
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
      if (c.but == B_FOURRAGE) {      // il marche et sautille, puis picore, trois fois de suite
        if (c.sub == 0) { c.tx = lim(c.x + rnd(-50, 50), 18, 302); c.sub = 1; c.d = (c.tx >= c.x) ? 1 : -1; }
        else if (c.sub == 1) {
          float dxm = c.tx - c.x;
          if (fabsf(dxm) < 1.8f) { c.sub = 2; c.peck = 0; }
          else { c.x += (dxm > 0 ? 1 : -1) * mnf(fabsf(dxm), 36.0f * dt) * mxf(0.0f, c.dv * (dxm > 0 ? 1.0f : -1.0f)); c.hop = -fabsf(sinf(c.t * 10.0f)) * 2.4f * c.s; c.bob = c.hop; c.pitchC = c.pr * 0.55f; c.haC = c.pitchC - 0.2f; c.hbC = c.haC + 0.5f; }
        } else if (c.sub == 2) {
          c.peck += dt; float dip = fabsf(sinf(c.peck / 0.42f * 3.1416f)); c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
          if (c.peck > 1.26f) { c.om += 1; if (c.om >= 3) { c.but = B_RIEN; c.om = 0; apresSol(c); } else c.sub = 0; }
        }
      } else if (c.but == B_BAIN) {      // bain de poussière : accroupi, les ailes qui battent, les plumes qui volent
        c.peck += dt; float fl = sinf(c.peck * 24.0f);
        c.pitchC = 0.05f; c.bob = 1.4f * c.s + 0.6f * fl; c.wlC = 0.55f + 0.2f * fl; c.poseC = mixA(A_PLANE, A_HAUT, 0.5f + 0.5f * fl); c.haC = -0.2f; c.hbC = 0.3f;
        if (((int)(c.peck * 7.0f)) != ((int)((c.peck - dt) * 7.0f))) plumes(c.x, c.y + 5.0f * c.s, 1);
        if (c.peck > 2.8f) { c.but = B_RIEN; c.wlC = 0; apresSol(c); }
      }
      if (c.but == B_RIEN && c.sub == 2 && c.etat == E_SOL && c.t > 0.5f) { c.t = 0; apresSol(c); }
    } break;
    case E_TENU: {      // attrapé par le doigt : il bat des ailes et suit le doigt
      c.pied = false; c.wlC = 1; c.fanC = 0; c.phase += 6.0f * dt; if (c.phase >= 1) c.phase -= 1;
      float cp = coup(c.phase); c.poseC = mixA(A_HAUT, A_BAS, cp); c.bob = -1.5f * c.s * (cp - 0.4f);
      c.pitchC = 0.95f; c.haC = -0.30f; c.hbC = 0.19f;
      float k = mnf(1.0f, dt * 16.0f); c.x += (c.tx - c.x) * k; c.y += (c.ty - c.y) * k;
      if (!D.bas && c.t > 0.4f) relacherMain(idx, c.x, c.y);      // sécurité : le doigt est parti
    } break;
    case E_TIR: {      // projeté par la fronde : trajectoire en arc, puis retour vers un perchoir
      c.pied = false; c.wlC = 0.45f; c.poseC = A_PLANE; c.fanC = 0; c.bob = 0;
      c.vy += 330.0f * dt; c.x += c.vx * dt; c.y += c.vy * dt; if (fabsf(c.vx) > 12.0f) c.d = (c.vx >= 0) ? 1 : -1;
      c.pitchC = lim(-atan2f(c.vy, fabsf(c.vx)) * 0.9f, -1.2f, 1.2f); c.haC = -0.30f; c.hbC = 0.19f;
      float sol = solHaut(c.x) - 10.0f;
      if (c.t > 1.5f || c.x < bordG() || c.x > bordD() || c.y < -40 || (c.y > sol && c.vy > 0)) {
        if (c.y > sol) c.y = sol;
        c.etat = E_VOL; c.t = 0; c.vx *= 0.6f; c.vy = mnf(c.vy, 0.0f) - 20.0f; c.wlC = 1; retourLibre(c);
      }
    } break;
    case E_DEHORS: {
      c.visible = false; c.wl = c.wlC = 0;
      if (c.t >= c.tfin) {
        if (c.rapporte) {           // il revient avec un objet qu'il dépose par terre
          c.rapporte = false; c.objet = 7 + random(4);
          float gx = rnd(110, 300); c.perch = -1; entrer(c, gx, solHaut(gx), B_OBJET);
        } else {
          int i = perchPour(c);
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

  fam::pose(c, idx, dt);      // famille.h : rôles (couple, nid, poussins, protection)
  // la tête suit un regard imposé (toucher, nourriture) ; tous les mouvements sont lissés
  { float sqT = 1.0f;      // squash & stretch : écrasé avant le saut, étiré au départ, écrasé à l'atterrissage
    if (c.etat == E_DECOLLE) sqT = lerpf(1.0f, 0.86f, pas(c.t / 0.11f));
    else if (c.etat == E_VOL && c.t < 0.25f && c.vy < -20.0f) sqT = 1.0f + 0.14f * (1.0f - pas(c.t / 0.25f));
    else if (c.etat == E_ATTERRIT) sqT = 1.0f - 0.12f * sinf(lim(c.t / 0.34f, 0.0f, 1.0f) * 3.1416f);
    c.sq += (sqT - c.sq) * mnf(1.0f, dt * 22.0f);
    if (fabsf(c.dv - (float)c.d) > 0.35f) c.fanC = mxf(c.fanC, (c.etat == E_VOL) ? 0.9f : 0.7f); }      // il pivote : les plumes de la queue s'ouvrent
  if (c.tLook > 0 && (c.etat == E_PERCHE || (c.etat == E_SOL && c.but != B_NOUR))) { c.haC = c.look; c.hbC = c.look + 0.49f; }
  ressort(c.ha, c.hav, c.haC, 11.0f, dt); ressort(c.hb, c.hbv, c.hbC, 12.0f, dt); ressort(c.pitch, c.pv, c.pitchC, 9.0f, dt);
  { float thT = (c.d > 0) ? 0.0f : 3.14159f, th = acosf(lim(c.dv, -1.0f, 1.0f));      // demi-tour : un angle (0 à 180 degrés), la largeur vue de côté est son cosinus
    float stp = ((c.etat == E_VOL || c.etat == E_DECOLLE) ? 11.5f : 9.0f) * (0.45f + 0.55f * sinf(th)) * dt;
    if (fabsf(thT - th) <= stp) th = thT; else th += (thT > th) ? stp : -stp;
    c.dv = cosf(th);
    float sn_ = sinf(th), roT = 0.0f;
    if (c.etat == E_VOL || c.etat == E_DECOLLE) { roT = lim(c.vy * 0.0025f, -0.3f, 0.3f); if (sn_ > 0.02f) roT += ((c.d > 0) ? -0.40f : 0.40f) * sn_; }
    ressort(c.ro, c.rov, roT, 10.0f, dt); if (roT == 0.0f && fabsf(c.ro) < 0.002f && fabsf(c.rov) < 0.02f) { c.ro = 0.0f; c.rov = 0.0f; } }
  c.wl += (c.wlC - c.wl) * mnf(1.0f, dt * ((c.etat == E_VOL || c.etat == E_DECOLLE || c.etat == E_TENU) ? 18.0f : 10.0f));
  c.fan += (c.fanC - c.fan) * mnf(1.0f, dt * 8.0f);
  c.grat += (c.gratC - c.grat) * mnf(1.0f, dt * 12.0f);
  c.poseP = c.pose;
  { float kp = mnf(1.0f, dt * ((c.etat == E_VOL || c.etat == E_TENU) ? 30.0f : 14.0f));
    float kh = (c.etat == E_VOL || c.etat == E_TENU) ? kp * 0.7f : kp;      // la main suit le poignet : un peu plus lentement
    c.pose.ea += (c.poseC.ea - c.pose.ea) * kp; c.pose.eh += (c.poseC.eh - c.pose.eh) * kh; c.pose.sa += (c.poseC.sa - c.pose.sa) * kp; c.pose.sh += (c.poseC.sh - c.pose.sh) * kh; }
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
  if (fam::enService(c)) return;      // un parent qui a des œufs ou des petits reste près du nid
  float cy = orbCy;
  c.item = -1; c.rapporte = false; c.fuite = false; c.sub = 0; c.objet = 0;
  switch (c.etat) {
    case E_DEHORS: entrer(c, orbCx, cy, B_ORBITE); break;
    case E_PERCHE: case E_SOL: case E_ATTERRIT: c.perch = -1; decoller(c, orbCx, cy, B_ORBITE); break;
    case E_OFF: return;
    case E_POOF: return;      // il revient d'abord ; il sera recruté ensuite
    default: c.perch = -1; c.but = B_ORBITE; c.aWp = false; break;      // déjà en vol
  }
  c.ang = atan2f((c.y - cy) / 0.5f, c.x - orbCx); c.om = (random(2) ? 1.0f : -1.0f) * rnd(1.6f, 2.3f); c.ray = rnd(62, 92); c.ocg = true; c.tOrb = 0;
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
  ralenti(0.25f); V.flinch = 1.0f; float y0 = visPiedsY();
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
  memset(&V, 0, sizeof(V)); V.ph = V_NUEE; gAction = 4.0f; orbCx = VX; orbCy = visPiedsY() - 42.0f;
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
  ralenti(0.35f); vib(90); eclat(VX, cy, 0.35f, 3.2f); plumes(VX, cy, 40); son(SN_POP_APP, 230); son(SN_PLUMES, 160); sonCri();
  for (int i = 0; i < NMINI; i++) if (mini[i].actif && mini[i].mode == 1) { Mini& m = mini[i]; float a = atan2f(m.y - cy, m.x - VX), v = rnd(60, 140); m.mode = 2; m.vx = cosf(a) * v; m.vy = sinf(a) * v - 20; m.a = atan2f(m.vy, m.vx); m.vie = 1.0f; }
}
static void finVisite() {      // il se disperse en corbeaux et en plumes, en un clin d'œil
  if (V.ph == V_RIEN) return;
  float cy = visPiedsY() - 35.0f;
  if (V.ph == V_PRESENT) { eclat(VX, cy, 0.3f, 2.6f); plumes(VX, cy, 36); minisEnvol(VX, cy, 14); son(SN_POP_DISP, 230); son(SN_PLUMES, 160); sonCri(); }
  for (int i = 0; i < NMINI; i++) if (mini[i].actif && mini[i].mode == 1) mini[i].actif = false;
  V.ph = V_RIEN; V.arm = V.armC = 0; gVisite = delaiVisite(false);
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    if (c.perch >= 10 && (c.etat == E_PERCHE || c.etat == E_ATTERRIT)) { c.perch = -1; versPerchoir(c); }
    else if (c.but == B_ORBITE || c.but == B_VISITEUR) {
      int i = perchLibre();
      if (i >= 0 && random(100) < 65) { c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); c.aWp = false; }
      else { c.perch = -1; c.but = B_SORTIR; c.tx = (c.x < 160) ? bordG() - 16.0f : bordD() + 16.0f; c.ty = rnd(8, 90); c.aWp = false; c.fuite = true; }
    }
  }
}
static void majVisiteur(float dt) {
  if (V.ph == V_RIEN) return;
  orbCx = VX; orbCy = visPiedsY() - 42.0f;
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
      m.vy += m.w * 0.75f * dt; m.r -= m.vx * dt; if (m.r < 8) m.r = 8;      // m.vy = angle sur l'orbite
      m.x = VX + m.r * cosf(m.vy); m.y = cy + 0.55f * m.r * sinf(m.vy);
      { float sg = (m.w > 0) ? 1.0f : -1.0f; m.a = atan2f(0.55f * cosf(m.vy) * sg, -sinf(m.vy) * sg); }      // cap = tangente à l'ellipse
    } else {
      m.x += m.vx * dt; m.y += m.vy * dt; m.vx *= (1.0f - 0.8f * dt); m.vy = m.vy * (1.0f - 0.8f * dt) - 10.0f * dt; m.vie -= dt / 0.9f; m.a = atan2f(m.vy, m.vx);
      if (m.vie <= 0) m.actif = false;
    }
  }
}

static void secousse() {
  vib(60); gAction = 2.5f;
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
  if (c.act == A_DORT) { c.act = A_IMMO; c.dAct = rnd(2.0f, 4.0f); c.tAct = 0; }
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

// ---------- modes d'interaction : main, lancer, appel / effrayer, fronde ----------
static void disperserOrbite(Corbeau& c) {      // la nuée se défait : retour aux perchoirs, ou sortie puis retour
  if (c.perch >= 10 && (c.etat == E_PERCHE || c.etat == E_ATTERRIT)) { c.perch = -1; versPerchoir(c); }
  else if (c.but == B_ORBITE || c.but == B_VISITEUR) {
    int i = perchLibre();
    if (i >= 0 && random(100) < 65) { c.perch = i; c.but = B_PERCHOIR; c.tx = PERCH[i][0]; c.ty = perchY(i); c.aWp = false; }
    else { c.perch = -1; c.but = B_SORTIR; c.tx = (c.x < 160) ? bordG() - 16.0f : bordD() + 16.0f; c.ty = rnd(8, 90); c.aWp = false; c.fuite = true; }
  }
}
static void chasser(Corbeau& c) {      // le corbeau fuit ; il reviendra
  if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_TENU || c.etat == E_TIR || c.etat == E_OFF) return;
  { const int ki_ = (int)(&c - cr); if (ki_ >= 0 && ki_ < NCB && natSante(ki_) >= 2 && c.etat == E_PERCHE) { c.surs = 0.3f; c.bec = 1.0f; return; } }      // un blesse grave ne vole plus : il se fige
  eclat(c.x, c.y - 4.0f, 0.12f, 0.6f);      // éclat de choc au départ rapide
  c.item = -1; c.rapporte = false; c.fuite = true; c.flash = 0.8f; c.bec = 1; c.pj = -1;
  if (c.etat == E_PERCHE || c.etat == E_SOL || c.etat == E_ATTERRIT) { c.perch = -1; sortir(c); c.fuite = true; c.gerbe = true; }
  else { c.perch = -1; c.but = B_SORTIR; c.tx = (c.x < 160) ? bordG() - 16.0f : bordD() + 16.0f; c.ty = rnd(8, 90); c.aWp = false; }
}
static void effrayer(float x, float y) {      // tous les corbeaux proches s'envolent en nuée
  for (int k = 0; k < NCB; k++) if (hypotf(cr[k].x - x, cr[k].y - y) < 170) chasser(cr[k]);
  minisEnvol(x, y, 12); plumes(x, y, 10); eclat(x, y, 0.2f, 1.0f); sonCri(); son(SN_PLUMES, 170); gAction = 3.0f;
}
static void appelDebut(float x, float y) {      // ils viennent tourner autour du doigt
  if (V.ph != V_RIEN || appelOn) return;
  appelOn = true; orbCx = x; orbCy = y; gAction = 4.0f;
  for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if (c.etat == E_POOF || c.etat == E_TENU || c.etat == E_TIR || c.etat == E_OFF) continue; mettreEnOrbite(c); c.ray = rnd(24, 46); }
  sonCri();
}
static void appelFin() {
  if (!appelOn) return;
  appelOn = false;
  for (int k = 0; k < NCB; k++) if (cr[k].but == B_ORBITE) disperserOrbite(cr[k]);
}
static bool appuiProche(float x, float y, float& sx, float& sy) {      // point d'appui (branche ou sol) proche d'un point
  if (x >= 66 && x <= 236 && fabsf(y - (ytop(x) - 7)) < 28) { sx = x; sy = ytop(x) + 0.4f; return true; }
  for (int i = 4; i < NPE; i++) if (!perchOccupe(i) && hypotf(x - PERCH[i][0], y - (PERCH[i][1] - 7)) < 30) { sx = PERCH[i][0]; sy = perchY(i); return true; }
  if (y > 168) { sx = lim(x, 14, 306); sy = solHaut(sx); return true; }
  return false;
}
static int plusProche(float x, float y, bool libreSeulement) {
  int best = -1; float bd = 1e9f;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; if (c.etat == E_POOF || c.etat == E_TENU || c.etat == E_TIR || c.etat == E_ATTERRIT || c.etat == E_OFF) continue;
    if (libreSeulement && (c.objet || c.perch >= 10 || (c.etat == E_SOL && c.but != B_RIEN) || (c.etat == E_VOL && c.but != B_ORBITE && c.but != B_PERCHOIR && c.but != B_RIEN))) continue;
    float d = (c.etat == E_DEHORS) ? 400.0f : hypotf(c.x - x, c.y - y); if (d < bd) { bd = d; best = k; }
  }
  return best;
}
static void appeler(float x, float y) {      // un toucher : le corbeau le plus proche vient se poser là
  int b = plusProche(x, y, true); if (b < 0) return;
  float sx, sy; if (!appuiProche(x, y, sx, sy)) { sx = lim(x, 14, 306); sy = solHaut(sx); }
  Corbeau& c = cr[b]; c.item = -1; c.rapporte = false; c.flash = 0.6f; c.bec = 1;
  if (c.etat == E_DEHORS) { c.perch = -1; entrer(c, sx, sy, B_PERCHOIR); }
  else if (c.etat == E_PERCHE || c.etat == E_SOL) { c.perch = -1; decoller(c, sx, sy, B_PERCHOIR); }
  else { c.perch = -1; c.but = B_PERCHOIR; c.tx = sx; c.ty = sy; c.aWp = false; }
  sonCri();
}
static void rendreObjets();
static void envoyerMain();
static void tendreMain(float x, float y) {      // la main sort à une place fixe : le bras ne bouge plus ensuite (le toucher n'a plus d'effet sur lui)
  if (V.ph != V_RIEN || H.actif) return;
  (void)x; (void)y; const float FX = 160.0f, FY = 104.0f;      // 3.1.42 : la main est CENTREE (milieu de l'ecran), le bras sort du bas-milieu
  H.actif = true; H.x = H.tx = FX; H.y = H.ty = FY; H.ext = 0; H.gauche = false; H.pose = PS_REPOS; H.tPose = 0; H.vit = 1.0f; H.tRec = OP.paume ? ((OP.pvis > 0) ? rnd(1.8f, 3.2f) : 1e9f) : 0.9f; H.pal = H.palV = H.dip = H.dipV = H.halo = 0.0f; memset(BRAIS, 0, sizeof(BRAIS));
  H.vx = H.vy = H.tilt = H.tiltV = H.resp = 0; H.exs = FX + 16.0f; H.ouv = H.ouvV = H.esp = H.espV = H.clos = H.closV = H.circ = 0; H.ctrl = false; H.lx = FX; H.ly = FY; H.tHold = 0; H.lok = false;
  for (int i = 0; i < 5; i++) { H.sp[i] = MP[PS_REPOS][i][0]; H.cu[i] = MP[PS_REPOS][i][1]; H.spv[i] = H.cuv[i] = 0; }
  for (int i = 0; i < 3; i++) H.om[i].actif = false;
  son(SN_GAZ, 140);
}
static void retirerMain() { H.actif = false; rendreObjets(); TD = Tendre{0, 0, 0, 0, 0, 0, 0, 10.0f, 0, 0, 0, 0, -1, false}; }
static void saisir(int k, float x, float y, bool fronde) {      // on attrape un corbeau du bout du doigt
  Corbeau& c = cr[k]; if (c.etat == E_DEHORS || c.etat == E_POOF || c.etat == E_TIR) return;
  c.perch = -1; c.item = -1; c.rapporte = false; c.but = B_RIEN; c.etat = E_TENU; c.pied = false; c.vx = c.vy = 0; c.tx = x; c.ty = y; c.wx = c.x; c.wy = c.y;
  c.sub = fronde ? 1 : 0; c.act = A_REPOS; c.flash = 1.0f; c.bec = 1; c.pj = -1; c.t = 0; sonCri();
  if (fronde) { FR.on = true; FR.c = (int8_t)k; FR.ax = c.x; FR.ay = c.y; frTendu = false; }
}
static void relacherMain(int k, float x, float y) {      // on lâche : il se pose sur la branche ou le sol le plus proche, sinon il repart en vol
  Corbeau& c = cr[k]; if (c.etat != E_TENU) return;
  c.etat = E_VOL; c.t = 0; c.pied = false; c.wlC = 1; c.vx = rnd(-20, 20); c.vy = -30; c.sub = 0; c.aWp = false;
  float sx, sy;
  if (appuiProche(x, y, sx, sy)) { c.perch = -1; c.but = B_PERCHOIR; c.tx = sx; c.ty = sy; } else retourLibre(c);
}
static void tirer(int k) {      // fronde : le corbeau est projeté, puis revient se percher
  Corbeau& c = cr[k]; if (c.etat != E_TENU) return;
  float vx = (FR.ax - c.x) * 7.0f, vy = (FR.ay - c.y) * 7.0f, sp = hypotf(vx, vy);
  if (sp < 70.0f) { FR.on = false; relacherMain(k, c.x, c.y); return; }
  if (sp > 460.0f) { vx *= 460.0f / sp; vy *= 460.0f / sp; }
  c.etat = E_TIR; c.t = 0; c.vx = vx; c.vy = vy; c.d = (vx >= 0) ? 1 : -1; c.sub = 0; FR.on = false; son(SN_AILES, 230); son(SN_PLUMES, 150);
}
static void poursuivre(int idx) {      // les deux corbeaux les plus proches de la trajectoire foncent sur l'objet lancé
  Proj& p = proj[idx];
  float vit = hypotf(p.vx, p.vy), gr = 300.0f * (1.0f - lim((vit - 250.0f) / 500.0f, 0.0f, 0.75f));
  float tx = p.x + p.vx * 0.35f, ty = p.y + p.vy * 0.35f + 0.5f * gr * 0.1225f;
  int c1 = -1, c2 = -1; float d1 = 1e9f, d2 = 1e9f;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k];
    bool ok = c.objet == 0 && c.perch < 10 && ((c.etat == E_PERCHE) || (c.etat == E_SOL && c.but == B_RIEN) || (c.etat == E_VOL && (c.but == B_ORBITE || c.but == B_RIEN || c.but == B_PERCHOIR)));
    if (!ok) continue;
    if (cibleK != k && mo::envie(k, p.type) <= 0.0f) continue;      // 3.1.38 : un corbeau rassasie ne se precipite pas sur une nourriture
    float d = hypotf(c.x - tx, c.y - ty);
    if (d < d1) { d2 = d1; c2 = c1; d1 = d; c1 = k; } else if (d < d2) { d2 = d; c2 = k; }
  }
  { int fk = cibleK; cibleK = -1;      // l'objet lancé est destiné au corbeau désigné par son nom : il fonce, un autre peut le disputer
    if (fk >= 0 && fk < NCB) { const Corbeau& cf = cr[fk]; bool ok = cf.objet == 0 && cf.perch < 10 && ((cf.etat == E_PERCHE) || (cf.etat == E_SOL && cf.but == B_RIEN) || (cf.etat == E_VOL && (cf.but == B_ORBITE || cf.but == B_RIEN || cf.but == B_PERCHOIR))); if (ok) { if (c1 != fk) c2 = c1; c1 = fk; } } }
  int ch[2] = {c1, c2};
  for (int j = 0; j < 2; j++) {
    if (ch[j] < 0) continue; Corbeau& c = cr[ch[j]]; c.pj = (int8_t)idx; c.perch = -1;
    if (c.etat == E_PERCHE || c.etat == E_SOL) decoller(c, tx, ty, B_ATTRAPE); else { c.but = B_ATTRAPE; c.aWp = false; }
  }
}
static void lancerType(uint8_t type, float x, float y, float vx, float vy) {      // on lance un objet ou une nourriture (direction et force du geste)
  if (!stockPrend(type)) return;
  int idx = -1; for (int i = 0; i < NPROJ; i++) if (!proj[i].actif) { idx = i; break; } if (idx < 0) idx = 0;
  float sp = hypotf(vx, vy); if (sp < 130.0f) { vx *= 130.0f / (sp + 0.01f); vy *= 130.0f / (sp + 0.01f); } if (sp > 520.0f) { vx *= 520.0f / sp; vy *= 520.0f / sp; }
  Proj& p = proj[idx]; p.actif = true; p.type = type; p.x = x; p.y = y; p.vx = vx; p.vy = vy; p.rot = 0; p.t = 0; p.ox = x;
  p.vr = (p.type == T_SHURIKEN ? 24.0f : 10.0f) * (random(2) ? 1.0f : -1.0f); p.dern = -1;
  son(SN_PLUMES, 150); poursuivre(idx);      // même pour un œuf : les corbeaux essaient de l'attraper avant qu'il explose
}
static void lancer(float x, float y, float vx, float vy) { lancerType(lanceTyp(choixL), x, y, vx, vy); }
static void balayage(float x0, float y0, float x1, float y1) {      // un geste de balayage : les corbeaux près du trajet s'envolent
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy + 0.01f; int n = 0;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; float u = lim(((c.x - x0) * dx + (c.y - y0) * dy) / l2, 0.0f, 1.0f);
    if (hypotf(c.x - (x0 + dx * u), c.y - (y0 + dy * u)) < 55.0f) { chasser(c); n++; }
  }
  if (n) { minisEnvol((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, 6); plumes((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, 6); sonCri(); son(SN_PLUMES, 150); }
}

// ---------- œufs magiques ----------
static void naissance(float x, float y, bool rare) {      // un petit corbeau duveteux sort de l'œuf ; il grandira un peu avec le temps
  Corbeau& c = cr[PC]; initCorbeau(c, 0.62f * tailleF());
  c.rare = rare; c.duvet = 1.0f; agePous = 0; c.d = random(2) ? 1 : -1; c.dv = (float)c.d;
  c.perch = -1; c.fx = x; c.fy = y; c.fp = (ytop(x + 4) - ytop(x - 4)) / 8.0f; c.pied = true; c.etat = E_PERCHE; c.visible = true; c.tfin = rnd(8, 14); c.t = 0;
  c.flash = 0.8f; c.bec = 1; son(SN_GAZ, 230); fam::appliquer(c, PC); fam::debutPetit(rare);
}
static void craquer() {      // un toucher : l'œuf tremble et se fissure
  if (!OE.actif || OE.etat != 0) return;
  OE.etat = 1; OE.tc = 0; son(SN_CROC, 230); vib(40);
}
static void eclore() {      // il s'ouvre : naissance, ou explosion de plumes avec une surprise
  float x = OE.x, y = OE.y - 5.0f; OE.actif = false; OE.etat = 0;
  nbOeufs++; nbOeufsSale = true; tOeufsEcr = millis(); oeufsSale = true; ralenti(0.3f);
  int r = random(100); if (fam::oeufCouve()) r = (random(100) < 15) ? 90 : 10;      // un œuf couvé éclot toujours en petit corbeau
  eclat(x, y, 0.3f, 1.6f); plumes(x, y, r < 50 ? 8 : 24); vib(80);
  if ((r < 50 || r >= 85) && (fam::afOn() ? fam::placeLibre() : (cr[PC].etat == E_OFF || fam::grand()))) { fam::pNext = fam::oeufCouve() ? fam::cvK : fam::pondeur; naissance(OE.x, OE.y, r >= 85); }      // celui qui a pondu (et surveillé) l'œuf devient le parent attitré      // petit corbeau (parfois un rare, à l'œil doré)
  else {
    son(SN_POP_DISP, 230); son(SN_PLUMES, 170); if (!fam::afOn()) minisEnvol(x, y, 8);
    static const uint8_t RARE[5] = {T_BRILLANT, T_BAGUE, T_SHURIKEN, T_PARCHEMIN, T_KUNAI};
    int j = -1; for (int q = 0; q < NOB; q++) if (!ob[q].actif) { j = q; break; }
    if (j >= 0) { Obj& o = ob[j]; o.actif = true; o.type = RARE[random(5)]; o.x = lim(x, 12, 308); o.y = y; o.vy = -90; o.t = 0; o.repos = false; o.reserve = -1; }      // objet rare qui retombe
  }
  fam::apresEclosion();
}
static void oeufExplose(float x, float y) {      // un œuf lancé explose à l'impact en nuée de plumes et de petits corbeaux
  nbOeufs++; nbOeufsSale = true; tOeufsEcr = millis(); oeufsSale = true; ralenti(0.3f);
  eclat(x, y - 4, 0.3f, 2.2f); plumes(x, y - 4, 26); if (!fam::afOn()) minisEnvol(x, y - 6, 12); son(SN_POP_DISP, 230); son(SN_PLUMES, 180); sonCri(); vib(90);
  for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if (c.visible && hypotf(c.x - x, c.y - y) < 60) { c.flash = 1.0f; c.surs = 0.3f; } }
}
static void pondre() {      // un corbeau perché se secoue : il va pondre
  if (OE.actif || OE.ponte >= 0) return;
  int n = 0, l[NAD];
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 4 && c.objet == 0 && c.act <= A_REGARDE) l[n++] = k; }
  if (!n) return;
  int k = l[random(n)]; Corbeau& c = cr[k]; c.act = A_SECOUE; c.tAct = 0; c.dAct = 1.2f; c.bec = 1; OE.ponte = (int8_t)k; OE.tp = 1.1f; sonCri();
}
static void oeufMaintenant() {      // bouton « œuf » : un œuf apparaît tout de suite (un corbeau perché pond en une demi-seconde, sinon il apparaît dans un éclat)
  if (V.ph != V_RIEN || GP.ph) return;
  if (OE.actif || OE.ponte >= 0) { son(SN_POP_APP, 110); return; }      // il y en a déjà un
  pondre();
  if (OE.ponte >= 0) OE.tp = 0.45f;
  else {
    float x = rnd(96.0f, 216.0f); OE.actif = true; OE.x = x; OE.y = ytop(x) + 0.4f; OE.t = 0; OE.tc = 0; OE.etat = 0;
    son(SN_POP_APP, 170); plumes(x, OE.y - 4, 5); eclat(x, OE.y - 5, 0.25f, 0.8f); vib(60);
  }
  gOeuf = rnd(60.0f, 120.0f) * multFreq(OP.oe) * (1.0f + 0.35f * (float)natSurplus());
}
static void majOeuf(float dt) {
  gOeuf -= dt;
  if (gOeuf <= 0) {      // un œuf toutes les 60 à 120 s (réglage « Œufs pondus ») tant qu'il n'y en a pas déjà un ; si personne ne peut pondre ou s'il y en a un, on réessaie bientôt
    gOeuf = rnd(60.0f, 120.0f) * multFreq(OP.oe) * (1.0f + 0.35f * (float)natSurplus());
    if (OP.oe > 0) { if (V.ph == V_RIEN && !OE.actif && OE.ponte < 0 && !panneau) { pondre(); if (OE.ponte < 0) gOeuf = rnd(4.0f, 8.0f); } else gOeuf = rnd(6.0f, 10.0f); }
  }
  if (OE.ponte >= 0) {
    OE.tp -= dt;
    if (OE.tp <= 0) {
      Corbeau& c = cr[OE.ponte]; fam::pondeur = OE.ponte; OE.ponte = -1;
      if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 4) {      // l'œuf apparaît sur la branche, derrière lui
        float x = lim(c.fx - c.dv * 17.0f, 76.0f, 232.0f);
        OE.actif = true; OE.x = x; OE.y = ytop(x) + 0.4f; OE.t = 0; OE.tc = 0; OE.etat = 0; son(SN_POP_APP, 170); plumes(x, OE.y - 4, 5); eclat(x, OE.y - 5, 0.25f, 0.8f); vib(60);
      }
    }
  }
  if (OE.actif) { OE.t += dt; if (fam::afOn() && OE.etat == 0 && OE.t > 180.0f) { OE.actif = false; son(SN_POP_DISP, 120); plumes(OE.x, OE.y - 4, 4); }      // l'œuf pas attrapé refroidit au bout de trois minutes
    else if (OE.etat == 0 && !fam::afOn() && OE.t > 45.0f) craquer(); if (OE.etat == 1) { OE.tc += dt; if (OE.tc > 1.0f) eclore(); } }
  if (cr[PC].etat != E_OFF) { agePous += dt; float s_, d_; fam::tailleVie(agePous, s_, d_); cr[PC].s = s_ * tailleF(); cr[PC].duvet = d_; }      // poussin, jeune, moyen, adulte : taille et duvet selon l'âge
}

// ---------- vie sociale : s'appeler, alerte, chamaille, vol en groupe ----------
static int perchesLibres(int* l) { int n = 0; for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.objet == 0 && c.act <= A_REGARDE) l[n++] = k; } return n; }
static void appelMutuel() {      // deux corbeaux se font face et s'appellent à tour de rôle
  int l[NAD]; int n = perchesLibres(l); if (n < 2) return;
  int a = l[random(n)], b; do { b = l[random(n)]; } while (b == a);
  cr[a].d = (cr[b].x >= cr[a].x) ? 1 : -1; cr[b].d = -cr[a].d; cr[a].hum = cr[b].hum = 1.0f;
  AP.a = (int8_t)a; AP.b = (int8_t)b; AP.n = 5; AP.t = 0.2f;
}
static void alerte() {      // un danger : tous restent immobiles, la tête levée, regardant dans la même direction
  dirAlerte = random(2) ? 1 : -1; tAlerte = rnd(4.0f, 6.5f); alerteFuite = (random(100) < 40);
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (c.etat == E_PERCHE) { if (c.act == A_DORT) { c.act = A_IMMO; c.dAct = 2.0f; c.tAct = 0; } c.hum = 1.0f; } }
  for (int k = 0; k < NAD; k++) if (cr[k].etat == E_PERCHE) { cr[k].bec = 1; break; }
  sonCri();
}
static void orbiteLocale(Corbeau& c, float cx, float cy, float sens, float duree) {      // vol en cercle autour d'un point (groupe ou poursuite)
  c.perch = -1; c.ocg = false; c.ocx = cx; c.ocy = cy; c.ray = rnd(40.0f, 70.0f); c.om = sens * rnd(1.4f, 1.9f); c.tOrb = duree;
  decoller(c, cx, cy, B_ORBITE); c.ang = atan2f((c.y - cy) / 0.5f, c.x - cx);
}
static void chamaille() {      // l'un en poursuit un autre en vol, puis ils reviennent
  int l[NAD]; int n = perchesLibres(l); if (n < 2) return;
  int a = l[random(n)], b; do { b = l[random(n)]; } while (b == a);
  float cx = rnd(110.0f, 230.0f), cy = rnd(55.0f, 90.0f);
  orbiteLocale(cr[b], cx, cy, random(2) ? 1.0f : -1.0f, rnd(5.0f, 7.0f));
  Corbeau& c = cr[a]; c.perch = -1; c.pj = (int8_t)b; c.tOrb = rnd(5.5f, 7.5f); decoller(c, cx, cy, B_POURSUITE);
  cr[a].hum = cr[b].hum = 1.0f; sonCri();
}
static void volGroupe() {      // plusieurs corbeaux décollent ensemble et tournent en groupe avant de revenir
  int l[NAD]; int n = perchesLibres(l); if (n < 3) return;
  float cx = rnd(110.0f, 230.0f), cy = rnd(55.0f, 85.0f), sens = random(2) ? 1.0f : -1.0f, duree = rnd(7.0f, 10.0f);
  int m = n < 4 ? n : 4;
  for (int i = 0; i < m; i++) { int j = i + random(n - i); int t = l[i]; l[i] = l[j]; l[j] = t; orbiteLocale(cr[l[i]], cx, cy, sens, duree); }
  sonCri(); son(SN_AILES, 200);
}
static void majSocial(float dt) {
  if (tAlerte > 0) {
    tAlerte -= dt;
    if (tAlerte <= 0 && alerteFuite) { alerteFuite = false; int n = 0; for (int k = 0; k < NAD && n < 2; k++) if (cr[k].etat == E_PERCHE && cr[k].perch < 10 && random(100) < 50) { chasser(cr[k]); n++; } }
  }
  if (AP.n > 0) {
    AP.t -= dt;
    if (AP.t <= 0) {
      Corbeau& c = cr[(AP.n % 2) ? AP.a : AP.b];
      if (c.etat == E_PERCHE) { c.bec = 1; c.flash = 0.5f; c.look = c.ha0 - 0.3f; c.tLook = 0.9f; if (random(2)) sonCri(); else son(SN_CROASSE1, 190); }
      AP.n--; AP.t = rnd(0.7f, 1.2f);
    }
  }
  gSocial -= dt * actFreq;      // activité : calme = rares événements surtout paisibles ; agité = alertes, poursuites et vols en groupe fréquents
  if (gSocial <= 0) {
    gSocial = rnd(12.0f, 26.0f);
    if (V.ph == V_RIEN && !appelOn && !panneau) {
      static const uint8_t PA[5] = {44, 32, 24, 20, 16}, PL[5] = {24, 22, 20, 20, 18}, PC_[5] = {6, 12, 18, 24, 28}, PV[5] = {4, 10, 18, 26, 32};
      int a = (OP.act < 1 ? 1 : (OP.act > 5 ? 5 : OP.act)) - 1, r = random(100), s1 = PA[a], s2 = s1 + PL[a], s3 = s2 + PC_[a], s4 = s3 + PV[a];
      if (r < s1) appelMutuel(); else if (r < s2) alerte(); else if (r < s3) chamaille(); else if (r < s4) volGroupe();
    }
  }
}

// ---------- visite d'un corbeau devant l'écran : déroulement ----------
static void departGP() {
  GP.perche = ((GP.ph == 3 || GP.ph == 4) && OP.barre);
  GP.x0 = GP.px; GP.y0 = GP.py; GP.s0 = GP.ps; GP.dv0 = GP.dvv; GP.p0 = GP.pit; GP.ph = 5; GP.t = 0; GP.dur = 1.9f; GP.ouv = 0;
  float dir = random(2) ? 1.0f : -1.0f; GP.ex = 160.0f + dir * rnd(120.0f, 170.0f); GP.ey = rnd(70.0f, 105.0f); son(SN_AILES, 230);
}
static void lancerVisite() {
  if (GP.ph || V.ph != V_RIEN || panneau || appelOn || H.actif || cbAbsent) return;
  int n = 0, l[NAD];
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (c.etat == E_PERCHE && c.perch < 10 && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f) l[n++] = k; }
  if (!n) return;
  int k = l[random(n)]; Corbeau& c = cr[k];
  vcb = c;      // le visiteur est ce corbeau-là : il part de sa branche, à sa taille et dans sa posture
  memset(&GP, 0, sizeof(GP)); GP.ph = 1; GP.dur = 2.3f; GP.k = (int8_t)k; GP.mir = (c.dv >= 0.0f) ? 1 : -1; GP.jeune = (c.s < 0.8f || rnd01() < 0.25f);
  GP.sx = TX(c.x); GP.sy = TY(c.y + c.bob); GP.s0 = c.s * zs; GP.dv0 = c.dv; GP.p0 = c.pitch; GP.fx0 = TX(c.fx); GP.fy0 = TY(c.fy); GP.ha0 = c.ha; GP.hb0 = c.hb;
  GP.sc = 1.0f; GP.tChoc = 1.0f; GP.gr = (uint32_t)random(0x7fffffff);
  c.etat = E_DEHORS; c.visible = false; c.t = 0; c.tfin = 1e6f; c.perch = -1; c.but = B_RIEN;      // il quitte la scène le temps de la visite
  vcb.etat = E_VOL; poserGP(); son(SN_AILES, 240);
}
static void majGP(float dt) {
  gVisGP -= dt;
  if (!GP.ph) {
    if (gVisGP <= 0) { float rep = (millis() - tToucher) / 1000.0f; bool ok = rep > 20.0f || random(100) < 20; gVisGP = ((rep > 20.0f) ? rnd(40.0f, 90.0f) : rnd(110.0f, 190.0f)) * multFreq(OP.gp); if (ok && OP.gp > 0 && !mo::vueFixe()) lancerVisite(); }      // réglage « Visites au gros plan »
    return;
  }
  GP.t += dt; vcb.phase += ((GP.ph == 1) ? lerpf(2.6f, 1.8f, lim(GP.t / GP.dur, 0.0f, 1.0f)) : (GP.ph == 5 ? 2.6f : 1.5f)) * dt; if (vcb.phase >= 1) vcb.phase -= 1;      // grands battements qui ralentissent à l'approche
  GP.choc *= expf(-dt * 12.0f); GP.tChoc += dt;
  switch (GP.ph) {
    case 1: if (GP.t >= GP.dur) { if (OP.barre) { GP.ph = 2; GP.t = 0; GP.dur = 0.5f; son(SN_AILES, 200); } else { GP.ph = 3; GP.t = 0; GP.dur = rnd(1.1f, 1.5f); GP.n = (int8_t)(1 + random(3)); GP.nf = 0; GP.tCl = rnd(0.8f, 1.6f); GP.sc = 1.0f; GP.tChoc = 1.0f; son(SN_AILES, 220); } } break;
    case 2: if (GP.t >= GP.dur) { GP.ph = 3; GP.t = 0; GP.dur = rnd(1.0f, 1.5f); GP.n = (int8_t)(1 + random(3)); GP.nf = 0; GP.tCl = rnd(0.8f, 1.6f); GP.sc = 1.0f; GP.tChoc = 1.0f; son(SN_ATTERRIT, 230); } break;      // posé sur la barre
    case 3:      // il s'arrête net devant la vitre, ailes ouvertes, et tourne de face puis de trois quarts
      GP.th = -0.2f + 0.2f * sinf(GP.t * 4.0f);
      GP.tCl -= dt; if (GP.tCl <= 0) { GP.cl = 1.0f; GP.tCl = rnd(1.5f, 3.0f); } if (GP.cl > 0) GP.cl = mxf(0.0f, GP.cl - dt * 6.0f);
      if (!GP.cri && GP.t > 0.4f && random(1000) < 10) { GP.cri = true; GP.tCri = 0; sonCri(); }
      if (GP.cri) { GP.tCri += dt; GP.ouv = 0.38f * sinf(3.1416f * lim(GP.tCri / 0.6f, 0.0f, 1.0f)); if (GP.tCri > 0.6f) { GP.cri = false; GP.ouv = 0; } }
      if (GP.t >= GP.dur) { GP.ph = 4; GP.t = 0; GP.nf = 0; GP.impact = false; GP.ouv = 0; GP.cri = false; GP.th *= 0.3f; }
      break;
    case 4: {      // coups de bec dans la vitre : recul, élan, choc avec fissures et tremblement
      float cyc = 0.9f, tc = GP.t - GP.nf * cyc;
      GP.th += (-0.1f - GP.th) * mnf(1.0f, dt * 6.0f);
      if (tc >= cyc) { GP.nf++; GP.impact = false; tc -= cyc; if (GP.nf >= GP.n) { departGP(); break; } }
      if (tc >= 0.42f && !GP.impact) {
        GP.impact = true; GP.choc = 5.0f; GP.tChoc = 0; son(SN_CROC, 255); son(SN_PLUMES, 110); vib(40);
        if (GP.fis < 3) { poserGP(); float ex, ey; posOeil(vcb, ex, ey); GP.ix[GP.fis] = ex + vcb.dv * 7.0f * vcb.s; GP.iy[GP.fis] = ey + 2.6f * vcb.s; GP.fis++; }
      }
    } break;
    default:
      if (GP.t >= GP.dur) { GP.ph = 0; Corbeau& c = cr[GP.k]; c.etat = E_DEHORS; c.visible = false; c.t = 0; c.tfin = 0.4f; }      // il revient dans la scène
      break;
  }
  if (GP.ph) poserGP();
}

static void feuille() {      // une feuille ou une plume se détache de l'arbre et tombe en dérivant avec le vent
  int idx = -1; for (int i = 0; i < NPART; i++) if (!pl[i].actif) { idx = i; break; } if (idx < 0) return;
  Plume& p = pl[idx]; p.x = rnd(10.0f, 110.0f); p.y = rnd(25.0f, 95.0f); p.vx = rnd(-6.0f, 6.0f); p.vy = rnd(4.0f, 14.0f); p.rot = rnd(0.0f, 6.28f); p.vr = rnd(-3.0f, 3.0f); p.dur = rnd(4.5f, 6.5f); p.vie = p.dur; p.actif = true;
}
// ---------- main : poses, reconnaissance de formes tracées au doigt, objets dans la paume ----------
static void rendreObjets() {      // les objets posés dans la paume retombent
  for (int i = 0; i < 3; i++) {
    ObjM& o = H.om[i]; if (!o.actif) continue; o.actif = false;
    int j = -1; for (int q = 0; q < NOB; q++) if (!ob[q].actif) { j = q; break; }
    if (j >= 0) { Obj& b = ob[j]; b.actif = true; b.type = o.type; b.x = lim(H.x + o.u, 12, 308); b.y = H.y - 4.0f; b.vy = 0; b.t = 0; b.repos = false; b.reserve = -1; }
  }
}
static bool mainOccupee() { for (int k = 0; k < NCB; k++) if (cr[k].perch == 14) return true; return false; }
static int nourrMain() { if (!H.actif) return -1; for (int i = 0; i < 3; i++) if (H.om[i].actif && H.om[i].type < T_BRILLANT) return i; return -1; }      // 3.1.42 : indice de la nourriture posee sur la paume (-1 = aucune)
static bool surPaume(float x, float y) { const RepM r = repereMain(); float px, py; ptPaume(r, px, py); return fabsf(x - px) < 40.0f && fabsf(y - (py - 6.0f)) < 30.0f; }
static void mainPoser(int type) {      // 3.1.42 : la nourriture choisie est posee sur la paume ouverte ; un corbeau PEUT venir la chercher (faim, caractere, confiance : jamais obligatoire), sinon elle disparait apres un moment
  if (!H.actif || H.pal < 0.5f) return;
  if (type < 0 || type >= T_BRILLANT) { infoMsg("Pas de la nourriture", "Choisis une nourriture dans la barre"); return; }
  if (nourrMain() >= 0) { infoMsg("Il y a déjà de la nourriture", "Elle est sur la paume"); return; }
  int j = -1; for (int q = 0; q < 3; q++) if (!H.om[q].actif) { j = q; break; }
  if (j < 0) { infoMsg("La paume est pleine", nullptr); return; }
  if (!stockPrend(type)) return;
  ObjM& o = H.om[j]; o.actif = true; o.type = (uint8_t)type; o.u = 0.0f; o.t = 0.0f;
  float px, py; { const RepM r = repereMain(); ptPaume(r, px, py); }
  son(SN_POP_APP, 120); eclat(px, py - 6.0f, 0.15f, 0.6f); H.dipV += 26.0f; H.halo = 1.0f; H.tRec = rnd(1.2f, 2.6f);
  infoMsg("Posée sur la main", "Un corbeau viendra peut-être la chercher");
}
static int choisirVisiteurMain() {      // le plus apprivoisé et curieux vient ; un parent qui protège un petit ne vient pas (sauf s'il est le seul corbeau)
  int best = -1; float bs = -1e9f; const int im_ = nourrMain(), tn = (im_ >= 0) ? (int)H.om[im_].type : -1;
  for (int pass = 0; pass < 2 && best < 0; pass++) {
    if (pass == 1 && OP.nb > 1) break;
    for (int k = 0; k < NCB; k++) {
      Corbeau& c = cr[k]; if (k == PC || c.etat == E_POOF || c.etat == E_TENU || c.etat == E_TIR || c.etat == E_ATTERRIT || c.etat == E_OFF) continue;
      if (c.objet || c.perch >= 10 || (c.etat == E_SOL && c.but != B_RIEN) || (c.etat == E_VOL && c.but != B_ORBITE && c.but != B_PERCHOIR && c.but != B_RIEN)) continue;
      if (pass == 0 && fam::enService(c) && !(tn >= 0 && natSante(k) > 0 && !(k < 2 && fam::precieux()))) continue;      // un blesse affame peut venir chercher la nourriture posee sur la main
      if (tn >= 0) {      // 3.1.42 : nourriture sur la main : il vient selon sa faim, son plat prefere, son caractere (confiance) ; jamais obligatoire
        const float ev = mo::envie(k, tn); if (ev <= 0.0f) continue;
        const float hun = 1.0f - lim(mo::en[k], 0.0f, 1.0f), tr = (fam::pret && k < fam::NM) ? (0.5f * fam::S.soc[k] + 0.3f * fam::S.cur[k] + 0.2f * fam::S.aud[k]) / 255.0f : 0.5f;
        if (rnd01() > lim(0.12f + 0.55f * hun + 0.45f * tr - 0.35f * c.pCra + ((ev > 1.2f) ? 0.15f : 0.0f), 0.05f, 0.95f)) continue;
      }
      float sc = c.pJou + 0.6f * c.pBav + 0.3f * c.pCal - 0.6f * c.pCra + rnd(0.0f, 0.25f) - 0.0015f * ((c.etat == E_DEHORS) ? 300.0f : hypotf(c.x - H.x, c.y - H.y));
      if (sc > bs) { bs = sc; best = k; }
    }
  }
  return best;
}
static void envoyerMain() {      // un corbeau vient se poser sur la main (un seul à la fois : le suivant n'arrive qu'après le départ du premier)
  if (mainOccupee()) return;
  int b = OP.paume ? choisirVisiteurMain() : plusProche(H.x, H.y, true); if (b < 0) return;
  Corbeau& c = cr[b]; c.item = -1; c.rapporte = false; c.perch = 14;
  float px, py; ptVis(4, px, py);
  if (c.etat == E_DEHORS) entrer(c, px, py, B_VISITEUR);
  else if (c.etat == E_PERCHE || c.etat == E_SOL) decoller(c, px, py, B_VISITEUR);
  else { c.but = B_VISITEUR; c.aWp = false; }
}
static void traceAjoute(float x, float y) {
  if (TR.n > 0) { float d = hypotf(x - TR.x[TR.n - 1], y - TR.y[TR.n - 1]); if (d < 5.0f) return; TR.len += d; }
  if (TR.n >= 48) { for (int i = 0; i < 24; i++) { TR.x[i] = TR.x[i * 2]; TR.y[i] = TR.y[i * 2]; } TR.n = 24; }
  TR.x[TR.n] = x; TR.y[TR.n] = y; TR.n++;
}
static int reconnaitre() {      // reconnaissance simple et tolérante : 1 cercle, 2 triangle, 4 ligne, 5 spirale, 6 zigzag (la croix = deux lignes qui se croisent)
  const int N = 20; float rx[N], ry[N];
  if (TR.n < 5 || TR.len < 40.0f) return -1;
  // rééchantillonnage régulier le long du tracé
  float pas_ = TR.len / (N - 1), cum = 0, suiv = pas_; rx[0] = TR.x[0]; ry[0] = TR.y[0]; int m = 1;
  for (int i = 1; i < TR.n && m < N; i++) {
    float dx = TR.x[i] - TR.x[i - 1], dy = TR.y[i] - TR.y[i - 1], lg = hypotf(dx, dy); if (lg < 0.0001f) continue;
    while (m < N && cum + lg >= suiv) { float u = (suiv - cum) / lg; rx[m] = TR.x[i - 1] + dx * u; ry[m] = TR.y[i - 1] + dy * u; m++; suiv += pas_; }
    cum += lg;
  }
  while (m < N) { rx[m] = TR.x[TR.n - 1]; ry[m] = TR.y[TR.n - 1]; m++; }
  float dc = hypotf(TR.x[TR.n - 1] - TR.x[0], TR.y[TR.n - 1] - TR.y[0]), droit = dc / TR.len;
  float T = 0, A = 0; int corners = 0, flips = 0; float derniere = 0; bool dans = false; float somme = 0;
  for (int i = 1; i < N - 1; i++) {
    float a1 = atan2f(ry[i] - ry[i - 1], rx[i] - rx[i - 1]), a2 = atan2f(ry[i + 1] - ry[i], rx[i + 1] - rx[i]), th = a2 - a1;
    while (th > 3.1416f) th -= 6.2832f; while (th < -3.1416f) th += 6.2832f;
    T += th; A += fabsf(th);
    if (fabsf(th) > 0.8f || (dans && fabsf(th) > 0.45f && th * somme > 0)) { dans = true; somme += th; }
    else if (dans) { dans = false; if (fabsf(somme) > 1.0f) { corners++; if (corners > 1 && somme * derniere < 0) flips++; derniere = somme; } somme = 0; }
  }
  if (dans && fabsf(somme) > 1.0f) { corners++; if (corners > 1 && somme * derniere < 0) flips++; }
  // spirale : beaucoup de tours, rayon qui change
  float cx = 0, cy = 0; for (int i = 0; i < N; i++) { cx += rx[i]; cy += ry[i]; } cx /= N; cy /= N;
  float r0 = hypotf(rx[1] - cx, ry[1] - cy) + 0.1f, r1 = hypotf(rx[N - 2] - cx, ry[N - 2] - cy) + 0.1f;
  if (fabsf(T) > 7.0f && (r1 < 0.6f * r0 || r1 > 1.7f * r0)) return PS_POING;
  bool ferme = dc < 0.32f * TR.len;
  if (ferme) {
    if (corners >= 3 && corners <= 4 && fabsf(T) > 4.2f) return PS_PINCE;
    if (corners <= 1 && fabsf(T) > 4.2f && fabsf(T) < 9.0f) return PS_ENVELOPPE;      // un cercle : la main se referme doucement sur l'oiseau
    return -1;
  }
  if (droit > 0.82f) return PS_POINTE;
  if (corners >= 2 && flips >= 1) return PS_VAGUE;
  if (fabsf(T) > 3.4f && corners <= 1) return PS_OUVRE;
  if (droit > 0.55f) return PS_POINTE;
  return -1;
}
static void traceFin() {
  float dur = (millis() - TR.t0) / 1000.0f;
  if (TR.len < 18.0f && dur < 0.5f) return;      // un simple toucher (sur la main ou ailleurs) ne la déplace pas et ne la retire pas : on la retire avec le bouton Main ou en changeant de mode
  int p = reconnaitre(); if (p < 0) return;
  if (p == PS_POINTE) {      // deux lignes qui se croisent : doigts croisés
    float ang = atan2f(TR.y[TR.n - 1] - TR.y[0], TR.x[TR.n - 1] - TR.x[0]);
    if (millis() - tLigne < 1200) { float d = fabsf(ang - aLigne); while (d > 3.1416f) d = 6.2832f - d; if (d > 3.1416f - 0.6f) d = 3.1416f - d; if (d > 0.7f) p = PS_CROISE; }
    tLigne = millis(); aLigne = ang;
  }
  H.pose = (uint8_t)p; H.tPose = 0; H.vit = lim(TR.len / mxf(0.15f, dur) / 250.0f, 0.7f, 1.8f);
}
// ---------- tendresse : l'oiseau posé dans la paume (3.1.39) ----------
// Caresse : un toucher ou un glisser du doigt sur l'oiseau = il se penche, ferme les yeux, les plumes se lissent, petit son doux, coeur discret. Serrer : toucher long sur la main (0,7 s) ou le cercle existant = les doigts se ferment légèrement (jamais écrasé, il reste visible), yeux mi-clos. Câlins : de temps en temps il frotte la tête sur les doigts, yeux fermés. Coût : quelques ressorts, au plus 3 traits et un coeur de 3 formes, seulement quand un oiseau est dans la paume ; coupé en décor minimal.
static int oiseauPaume() { for (int k = 0; k < NCB; k++) if (cr[k].perch == 14 && cr[k].etat == E_PERCHE && cr[k].visible) return k; return -1; }
static void serrerDoux() { TD.sqT = 3.2f; son(SN_GAZ, 55); }
static void majTendresse(float dt) {
  const int k = (OP.tend && H.actif && H.pal > 0.6f) ? oiseauPaume() : -1;
  TD.k = (int8_t)k;
  if (k >= 0) {      // cercle ou toucher long sur la main : serrer doucement (au lieu du geste qui fait partir l'oiseau)
    if (H.tHold > 0.0f) { serrerDoux(); H.tHold = 0.0f; H.circ = 0.0f; }
    if (H.ctrl && D.bas && D.type == 6 && TR.len <= 16.0f && millis() - TR.t0 > 700u && TD.sqT <= 0.0f && !TD.sqFait) { serrerDoux(); TD.sqFait = true; }
  } else { TD.sqT = 0.0f; TD.calT = 0.0f; TD.carHold = 0.0f; }
  if (!D.bas) TD.sqFait = false;
  if (TD.carHold > 0.0f) TD.carHold -= dt;
  if (TD.sqT > 0.0f) TD.sqT -= dt;
  if (TD.calT > 0.0f) TD.calT -= dt;
  const float carT = (k >= 0 && ((D.bas && D.type == 8) || TD.carHold > 0.0f)) ? 1.0f : 0.0f;
  TD.car += (carT - TD.car) * mnf(1.0f, dt * 7.0f);
  TD.serre += (((TD.sqT > 0.0f && k >= 0) ? 1.0f : 0.0f) - TD.serre) * mnf(1.0f, dt * 3.5f);
  TD.cal += (((TD.calT > 0.0f && k >= 0) ? 1.0f : 0.0f) - TD.cal) * mnf(1.0f, dt * 5.0f);
  const float yt = (TD.car > 0.35f || TD.cal > 0.4f) ? 1.0f : ((TD.serre > 0.3f) ? 0.5f : 0.0f);
  TD.yeux += (yt - TD.yeux) * mnf(1.0f, dt * 6.0f);
  if (TD.coeurT > 0.0f) TD.coeurT -= dt;
  if (k < 0) return;
  Corbeau& c = cr[k];
  const bool actif = TD.car > 0.3f || TD.serre > 0.3f || TD.cal > 0.3f;
  if (actif && c.tfin - c.t < 3.0f) c.tfin = c.t + 3.0f;      // il reste tant qu'on s'occupe de lui
  if (TD.car > 0.3f) { c.look = 0.72f; c.tLook = 0.3f; }      // il se penche vers le doigt
  else if (TD.cal > 0.3f) { c.look = 0.50f + 0.28f * sinf(tAnim * 6.5f); c.tLook = 0.3f; }      // il frotte la tête sur les doigts
  else if (TD.serre > 0.3f) { c.look = 0.40f; c.tLook = 0.3f; }
  TD.tSon -= dt;
  if (actif && TD.tSon <= 0.0f) {
    TD.tSon = (TD.car > 0.3f) ? rnd(1.5f, 2.3f) : rnd(2.4f, 3.4f); son(SN_GAZ, 55);
    if ((TD.car > 0.3f || TD.cal > 0.3f) && !qMin) { TD.coeurT = 1.5f; TD.coeurX = c.x + (float)c.d * 5.0f * c.s; TD.coeurY = c.y - 15.0f * c.s; }
  }
  if (!actif && !D.bas) {      // câlins de temps en temps (réglage Câlins : rares / normaux / fréquents)
    TD.tCal -= dt;
    if (TD.tCal <= 0.0f) { TD.tCal = (OP.calin == 0) ? rnd(34.0f, 55.0f) : ((OP.calin == 2) ? rnd(6.0f, 12.0f) : rnd(14.0f, 26.0f)); if (c.t > 2.0f && H.pal > 0.85f) { TD.calT = 2.6f; TD.tSon = 0.0f; } }
  }
}
static void tendreSur(M5Canvas& g, int k) {      // par-dessus l'oiseau : plumes qui se lissent sous le doigt, petit coeur discret
  if (k != TD.k || TD.k < 0) return;
  const Corbeau& c = cr[k];
  if (TD.car > 0.3f && !dgE()) for (int i = 0; i < 3; i++) {
    const float u = fmodf(tAnim * 2.2f + i * 0.34f, 1.0f), px = c.x + (u - 0.5f) * 15.0f * c.s, py = c.y + c.bob - (3.6f + 0.9f * sinf(u * 3.1416f)) * c.s;
    ligne(g, px, py, px - (float)c.d * 4.0f * c.s, py + 1.2f * c.s, (OP.hor != 0) ? C(120, 40, 44) : C(76, 92, 140));
  }
  if (TD.coeurT > 0.0f && !qMin) {
    const float u = 1.0f - TD.coeurT / 1.5f, r = 0.4f + 1.9f * sinf(u * 3.1416f), hx = TD.coeurX + 2.0f * sinf(u * 5.0f), hy = TD.coeurY - 13.0f * u; const uint16_t cc = C(236, 80, 110);
    disque(g, hx - r * 0.55f, hy, r * 0.62f, cc); disque(g, hx + r * 0.55f, hy, r * 0.62f, cc); tri(g, hx - r * 1.12f, hy + r * 0.2f, hx + r * 1.12f, hy + r * 0.2f, hx, hy + r * 1.75f, cc);
  }
}
static void majMainNourr(float dt) {      // 3.1.42 : un corbeau pose dans la paume mange la nourriture qui s'y trouve : petits coups de bec (bruit de bec), puis il mange ; interaction tendre (petit coeur)
  const int im = nourrMain(), k = (H.actif && H.pal > 0.6f) ? oiseauPaume() : -1;
  if (im < 0 || k < 0) { mgK = -1; mgT = 0.0f; return; }
  Corbeau& c = cr[k]; ObjM& o = H.om[im];
  if (mgK != k) { mgK = (int8_t)k; mgT = 0.0f; }
  if (c.t < 0.6f || (k < NAD && mo::envie(k, o.type) <= 0.0f)) return;      // il vient de se poser, ou il n'a pas faim
  const int n0 = (int)(mgT * 2.4f); mgT += dt; const int n1 = (int)(mgT * 2.4f);
  if (c.tfin - c.t < 2.5f) c.tfin = c.t + 2.5f;      // il reste tant qu'il mange
  if (mgT > 0.5f && n1 != n0 && n1 <= 3) son(SN_CROC, 80);      // bruit de bec dans la main
  if (mgT >= 0.5f + 3.0f / 2.4f) {
    mo::mange(c, k, o.type); o.actif = false; mgK = -1; mgT = 0.0f; c.flash = 0.5f; c.hum = 1.0f; c.tfin = c.t + rnd(2.0f, 3.5f);
    son(SN_GAZ, 70); H.dipV += 12.0f; if (!qMin) { TD.coeurT = 1.5f; TD.coeurX = c.x + (float)c.d * 5.0f * c.s; TD.coeurY = c.y - 15.0f * c.s; }
  }
}
static void majMainPose(float dt) {
  if (!H.actif && H.ext <= 0.0f) return;
  H.x = H.tx = 160.0f; H.y = H.ty = 104.0f; H.vx = H.vy = 0.0f;      // le bras reste immobile
  H.resp = sinf(tAnim * 1.9f);
  H.circ *= expf(-dt * 0.7f); if (H.tHold > 0) H.tHold -= dt;
  majTendresse(dt);
  { const bool c_ = H.ctrl; const float sv = lim(hypotf(D.vx, D.vy) / 500.0f, 0.0f, 1.0f), w = (c_ ? 9.0f + 8.0f * sv : 5.0f);      // un trait rapide = mouvement vif ; au relâché, retour doux
    float gx = c_ ? lim((D.x - D.x0) / 55.0f, -1.0f, 1.0f) : 0.0f, gy = c_ ? lim((D.y - D.y0) / 55.0f, -1.0f, 1.0f) : 0.0f;
    ressort(H.tilt, H.tiltV, 0.5f * gx, w * 0.8f, dt);      // trait horizontal : le poignet pivote (vers la droite = les doigts s'abaissent)
    ressort(H.ouv, H.ouvV, -gy, w, dt);      // trait vers le haut : les doigts s'ouvrent ; vers le bas : le poing se ferme
    ressort(H.esp, H.espV, gx, w, dt);       // trait vers la droite : les doigts s'écartent ; vers la gauche : ils se rapprochent
    ressort(H.clos, H.closV, (H.tHold > 0) ? 1.0f : 0.0f, 7.0f, dt); }      // un cercle : la main se referme doucement sur l'oiseau
  if (!(H.tilt == H.tilt) || fabsf(H.tilt) > 1.5f) { H.tilt = H.tiltV = 0.0f; }
  if (!(H.ouv == H.ouv) || !(H.esp == H.esp) || !(H.clos == H.clos)) { H.ouv = H.ouvV = H.esp = H.espV = H.clos = H.closV = 0.0f; }
  {      // main ouverte paume vers le haut : la main se tourne tant que l'utilisateur ne la pilote pas ; halo et braises quand elle s'offre
    const bool geste = (H.ctrl && TR.len > 16.0f) || H.pose != PS_REPOS || H.clos > 0.15f || fabsf(H.ouv) > 0.3f;
    ressort(H.pal, H.palV, (OP.paume && H.actif && !geste) ? 1.0f : 0.0f, 7.0f, dt);
    if (!(H.pal == H.pal) || !(H.palV == H.palV)) { H.pal = H.palV = 0.0f; }
    H.pal = lim(H.pal, 0.0f, 1.0f);
    H.dipV += ((0.0f - H.dip) * 380.0f - 14.0f * H.dipV) * dt; H.dip += H.dipV * dt;      // la paume cède puis reprend sa place (ressort sous-amorti)
    if (!(H.dip == H.dip) || !(H.dipV == H.dipV)) { H.dip = H.dipV = 0.0f; }
    H.dip = lim(H.dip, -2.0f, 4.0f);
    H.halo += (((H.pal > 0.6f) ? (mainOccupee() ? 1.0f : 0.3f) : 0.0f) - H.halo) * mnf(1.0f, dt * 2.5f);
    for (int q = 0; q < 6; q++) { Braise& b = BRAIS[q]; if (!b.on) continue; b.t += dt; b.x += (b.vx + 5.0f * sinf(b.t * 5.0f + q)) * dt; b.y += b.vy * dt; if (b.t >= b.dur || !(b.x == b.x) || !(b.y == b.y)) b.on = false; }
    if (H.actif && H.pal > 0.85f && H.halo > 0.55f) { tBraise -= dt; if (tBraise <= 0.0f) { tBraise = rnd(0.16f, 0.32f) * ((OP.q == 0) ? 1.6f : 1.0f); braise(); } }
    if (OP.paume && H.actif && geste && H.pal < 0.8f) {      // un geste de l'utilisateur : le corbeau qui venait ou qui était dans la paume s'en va
      for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if (c.perch != 14) continue;
        if (c.etat == E_PERCHE) c.tfin = 0.0f;
        else if (c.etat == E_VOL || c.etat == E_DECOLLE) { c.perch = -1; retourLibre(c); H.tRec = delaiPaume(); } }
    }
  }
  for (int i = 0; i < 5; i++) {
    float ts, tc;
    if (H.pose == PS_VAGUE) { ts = MP[PS_REPOS][i][0]; tc = 0.55f * (0.5f + 0.5f * sinf(tAnim * 8.0f * H.vit - i * 0.9f)); }
    else { ts = MP[H.pose][i][0]; tc = MP[H.pose][i][1]; }
    if (H.pose == PS_REPOS || H.pose == PS_VAGUE) {      // gestes en direct, par-dessus la pose
      if (H.ouv > 0) { ts += (MP[PS_OUVRE][i][0] - ts) * H.ouv; tc += (MP[PS_OUVRE][i][1] - tc) * H.ouv; }
      else { ts += (MP[PS_POING][i][0] - ts) * -H.ouv; tc += (MP[PS_POING][i][1] * 0.92f - tc) * -H.ouv; }
      ts += (i == 0) ? 0.10f * H.esp : 0.12f * H.esp * (2 - i);
      ts += (MP[PS_ENVELOPPE][i][0] - ts) * H.clos; tc += (MP[PS_ENVELOPPE][i][1] - tc) * H.clos;
    }
    if (H.pal > 0.001f) { const float pp = pas(H.pal); ts += (MPAUME[i][0] - ts) * pp; tc += (MPAUME[i][1] - tc) * pp; }      // paume vers le haut : doigts relevés en coupe, pouce dressé
    if (TD.serre > 0.01f || TD.cal > 0.01f) { const float sq_ = TD.serre * 0.85f; ts += (MP[PS_ENVELOPPE][i][0] - ts) * sq_; tc += (MP[PS_ENVELOPPE][i][1] * 0.80f - tc) * sq_; tc += TD.cal * 0.06f * sinf(tAnim * 9.0f + i * 0.7f); }      // 3.1.39 : les doigts se referment légèrement (jamais le poing), ou frottent doucement
    tc += 0.035f * sinf(tAnim * 1.7f + i * 0.6f) * (H.ctrl ? 0.4f : 1.0f);      // respiration au repos
    tc += lim(-H.tiltV * 0.25f, -0.3f, 0.3f) * (0.6f + 0.12f * i);              // les doigts traînent derrière le poignet quand il pivote
    float w = (17.0f - 1.6f * i) * mnf(1.4f, H.vit), z = 0.62f;                   // un ressort par doigt, un peu sous-amorti : l'inertie se voit
    H.spv[i] += ((ts - H.sp[i]) * w * w - 2.0f * z * w * H.spv[i]) * dt; H.sp[i] += H.spv[i] * dt;
    H.cuv[i] += ((tc - H.cu[i]) * w * w - 2.0f * z * w * H.cuv[i]) * dt; H.cu[i] += H.cuv[i] * dt;
    if (!(H.cu[i] == H.cu[i]) || !(H.sp[i] == H.sp[i])) { H.cu[i] = H.sp[i] = H.cuv[i] = H.spv[i] = 0.0f; }      // garde-fou : jamais de valeur aberrante
    H.cu[i] = lim(H.cu[i], -0.5f, 1.25f); H.sp[i] = lim(H.sp[i], -0.7f, 0.7f);
  }
  if (H.pose != PS_REPOS) { H.tPose += dt; if (H.tPose > 1.9f / H.vit) { H.pose = PS_REPOS; H.tPose = 0; H.vit = 1.0f; } }
  for (int i = 0; i < 3; i++) { ObjM& o = H.om[i]; if (o.actif) { o.t += dt; if (o.t > (o.type < T_BRILLANT ? 25.0f : 10.0f)) { ObjM c = o; o.actif = false; int j = -1; if (c.type < T_BRILLANT) { float px_, py_; const RepM r_ = repereMain(); ptPaume(r_, px_, py_); eclat(px_, py_ - 6.0f, 0.2f, 0.5f); } else for (int q = 0; q < NOB; q++) if (!ob[q].actif) { j = q; break; } if (j >= 0) { Obj& b = ob[j]; b.actif = true; b.type = c.type; b.x = lim(H.x + c.u, 12, 308); b.y = H.y - 4.0f; b.vy = 0; b.t = 0; b.repos = false; b.reserve = -1; } } } }
  if (H.actif) {
    int nb = 0, k1 = -1; for (int k = 0; k < NCB; k++) if (cr[k].perch == 14) { nb++; if (k1 < 0) k1 = k; }
    if (nb > 1) for (int k = k1 + 1; k < NCB; k++) { Corbeau& c = cr[k]; if (c.perch != 14) continue; c.perch = -1; if (c.etat == E_PERCHE) versPerchoir(c); else retourLibre(c); }      // jamais deux oiseaux sur la main
    H.tRec -= dt;
    if (OP.paume) {
      if (H.ctrl && D.bas && D.type == 6 && TR.len <= 16.0f && millis() - TR.t0 > 600 && OP.pvis > 0 && H.tRec > 0.5f) H.tRec = 0.5f;      // le doigt posé et maintenu : la main s'offre, un corbeau vient
      if (H.tRec <= 0) { const bool nr_ = nourrMain() >= 0; H.tRec = nr_ ? rnd(2.2f, 4.0f) : 1.0f; if (nb == 0 && H.pal > 0.9f && (OP.pvis > 0 || nr_)) envoyerMain(); }      // 3.1.42 : de la nourriture sur la main : un corbeau peut venir meme si les visites sont sur Jamais
    } else if (H.tRec <= 0) { H.tRec = 1.2f; if (nb == 0) envoyerMain(); }
  }
}

// ---------- physique simple : chocs entre corbeaux, objets lancés, corbeau projeté par la fronde ----------
static float tChocCb = 0.0f;
static void impulsion(Corbeau& c, float jx, float jy) {      // coup reçu : en vol balistique la vitesse change, sinon la vitesse de choc s'ajoute puis s'amortit
  if (c.etat == E_TIR) { c.vx += jx; c.vy += jy; } else { c.kx = lim(c.kx + jx, -260.0f, 260.0f); c.ky = lim(c.ky + jy, -260.0f, 260.0f); }
}
static void choc(float x, float y, Corbeau* a, Corbeau* b, float force) {      // plumes, éclat, cri : le choc se voit et s'entend (pas plus de deux par seconde)
  if (tChocCb > 0) return; tChocCb = 0.45f;
  nat::choc(a, b, force);
  plumes(x, y, force > 120.0f ? 5 : 3); eclat(x, y, 0.12f, 0.6f); son(SN_PLUMES, 130);
  if (a) { a->flash = 0.6f; a->bec = 1; } if (b) { b->flash = 0.6f; b->bec = 1; }
  if (random(100) < 65) sonCri(); vib(30);
}
static bool frondeChasse(Corbeau& t, Corbeau& p, float v) {      // 3.1.43 : le corbeau projete par la fronde touche un autre corbeau : il le CHASSE (poursuite en vol) ; l'autre fuit en cercles ou riposte selon son caractere ; ni mort ni blessure (tFrImm coupe les blessures de collision)
  const int it = (int)(&t - cr), ip = (int)(&p - cr);
  if (it < 0 || ip < 0 || it >= NCB || ip >= NCB || it == ip || it == PC || ip == PC || t.duvet > 0.5f || p.duvet > 0.5f) return false;
  if (!(p.etat == E_PERCHE || p.etat == E_SOL || p.etat == E_VOL) || p.perch >= 10 || p.objet != 0 || p.but == B_POURSUITE || p.but == B_FAM || fam::enService(p) || natSante(ip) > 0) return false;      // jamais un parent de nid occupe, un blesse, le petit, un oiseau dans la paume
  choc(p.x, p.y, &t, &p, v); p.surs = 0.35f; tFrImm = 14.0f;
  t.etat = E_VOL; t.t = 0; t.perch = -1; t.item = -1; t.but = B_POURSUITE; t.pj = (int8_t)ip; t.tOrb = rnd(5.0f, 7.0f); t.aWp = false; t.vx *= 0.35f; t.vy = -45.0f; t.wlC = 1; t.pied = false; t.hum = 1.0f; t.bec = 1.0f; t.fuite = false;
  const float aud = (ip < NAD && fam::pret) ? fam::S.aud[ip] / 255.0f : 0.4f, fo = (ip < NAD && fam::pret) ? fam::S.frc[ip] / 255.0f : 0.4f;
  if (rnd01() < 0.10f + 0.45f * (0.6f * aud + 0.4f * fo)) {      // riposte : il se retourne et poursuit aussi
    if (p.etat != E_VOL) { p.perch = -1; p.item = -1; decoller(p, t.x, t.y, B_POURSUITE); } else { p.but = B_POURSUITE; p.aWp = false; }
    p.pj = (int8_t)it; p.tOrb = rnd(3.0f, 4.5f); p.hum = 1.0f; p.bec = 1.0f;
  } else {      // il fuit en cercles, poursuivi
    const float cx = lim(p.x + rnd(-40.0f, 40.0f), 70.0f, 250.0f), cy = rnd(55.0f, 95.0f), sens = random(2) ? 1.0f : -1.0f;
    if (p.etat == E_VOL) { p.but = B_ORBITE; p.ocg = false; p.ocx = cx; p.ocy = cy; p.ray = rnd(40.0f, 70.0f); p.om = sens * rnd(1.4f, 1.9f); p.tOrb = rnd(5.0f, 7.0f); p.aWp = false; p.ang = atan2f((p.y - cy) / 0.5f, p.x - cx); }
    else orbiteLocale(p, cx, cy, sens, rnd(5.0f, 7.0f));
    p.hum = 1.0f;
  }
  son(SN_CROASSE2, 190); sonCri(); return true;
}
static void contacts(float dt) {
  tChocCb -= dt; if (tFrImm > 0.0f) tFrImm -= dt;
  for (int k = 0; k < NCB; k++) {      // inertie et amortissement de la vitesse de choc
    Corbeau& c = cr[k]; if (c.kx == 0.0f && c.ky == 0.0f) continue;
    if (c.etat == E_VOL || c.etat == E_DECOLLE) { float kxe = c.kx; if (kxe * c.dv < 0.0f) kxe *= 0.25f; c.x += kxe * dt; c.y += c.ky * dt; }
    float a = expf(-dt * 3.2f); c.kx *= a; c.ky *= a; if (fabsf(c.kx) + fabsf(c.ky) < 3.0f) c.kx = c.ky = 0.0f;
  }
  for (int i = 0; i < NCB; i++) for (int j = i + 1; j < NCB; j++) {
    Corbeau& a = cr[i]; Corbeau& b = cr[j];
    if (!a.visible || !b.visible) continue;
    bool aa = (a.etat == E_VOL || a.etat == E_TIR), bb = (b.etat == E_VOL || b.etat == E_TIR);
    float dx = b.x - a.x, dy = b.y - a.y, d = hypotf(dx, dy), r = 7.5f * (a.s + b.s);
    if (d < 0.01f || d >= r + 8.0f) continue;
    float nx = dx / d, ny = dy / d;
    if (aa && bb) {      // deux corbeaux en vol : ils se heurtent, rebondissent (masse ~ taille au carré), perdent quelques plumes
      if (a.but == B_ORBITE && b.but == B_ORBITE) continue;
      if (d >= r) continue;
      if ((a.etat == E_TIR) != (b.etat == E_TIR)) { Corbeau& t = (a.etat == E_TIR) ? a : b; Corbeau& p = (a.etat == E_TIR) ? b : a; const float v = hypotf(t.vx, t.vy); if (v > 90.0f && frondeChasse(t, p, v)) continue; }      // 3.1.43 : projete sur un corbeau en vol
      float ov = r - d; a.x -= nx * ov * 0.5f; a.y -= ny * ov * 0.5f; b.x += nx * ov * 0.5f; b.y += ny * ov * 0.5f;
      float rv = (a.vx - b.vx) * nx + (a.vy - b.vy) * ny;      // > 0 : ils se rapprochent
      if (rv > 20.0f) {
        float ma = a.s * a.s, mb = b.s * b.s, jj = 1.6f * rv / (1.0f / ma + 1.0f / mb);
        impulsion(a, -jj / ma * nx, -jj / ma * ny); impulsion(b, jj / mb * nx, jj / mb * ny);
        if (rv > 40.0f) choc((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, &a, &b, rv);
      }
    } else if ((a.etat == E_TIR && (b.etat == E_PERCHE || b.etat == E_SOL)) || (b.etat == E_TIR && (a.etat == E_PERCHE || a.etat == E_SOL))) {      // le corbeau tiré à la fronde percute un corbeau posé
      Corbeau& t = (a.etat == E_TIR) ? a : b; Corbeau& p = (a.etat == E_TIR) ? b : a;
      float v = hypotf(t.vx, t.vy); if (d >= r || v < 110.0f) continue;
      if (frondeChasse(t, p, v)) continue;      // 3.1.43 : projete sur un corbeau pose : il le chasse (sinon, comportement d'avant)
      choc(p.x, p.y, &t, &p, v); p.surs = 0.35f;
      if (p.etat == E_PERCHE && p.perch < 10 && p.objet == 0 && random(100) < 70) chasser(p);
      t.vx *= -0.35f; t.vy = -fabsf(t.vy) * 0.4f - 40.0f;
    } else if ((a.etat == E_ATTERRIT && b.etat == E_PERCHE) || (b.etat == E_ATTERRIT && a.etat == E_PERCHE)) {      // un corbeau se pose tout près : l'autre sursaute doucement
      Corbeau& p = (a.etat == E_PERCHE) ? a : b; if (p.surs <= 0.0f && d < r) { p.surs = 0.30f; plumes(p.x, p.y - 2.0f, 1); }
    }
  }
}
static void majGlobal(float dt) {
  contacts(dt); mo::maj(dt);
  gAction -= dt * actFreq; gPoof -= dt * actFreq; gObjet -= dt * actFreq; gAttrib -= dt; gCalme -= dt;
  for (int i = 0; i < 2; i++) if (stK[i] < STMAX[i]) { const int a_ = (int)stK[i]; stK[i] = mnf(STMAX[i], stK[i] + dt / STREG[i]); if ((int)stK[i] != a_) stSale = true; }      // 3.1.38 : le stock de viande et de noix se renouvelle
  if (OP.hor == 2) {      // horreur : lueur des yeux instable (très discret), de rares battements de lumière et éclairs rouges lointains
    float n = sinf(tAnim * 9.0f) * sinf(tAnim * 2.3f + 1.0f) + 0.5f * sinf(tAnim * 23.0f); lueurOeil = 1.0f + 0.07f * n;
    if (battT > 0.0f) { battT -= dt; float u = 1.0f - lim(battT / 0.95f, 0.0f, 1.0f); lueurOeil += 0.9f * (powf(sinf(3.1416f * lim(u * 2.0f, 0.0f, 1.0f)), 2.0f) + 0.7f * powf(sinf(3.1416f * lim(u * 2.0f - 1.0f, 0.0f, 1.0f)), 2.0f)); }
    else { gBatt -= dt; if (gBatt <= 0.0f) { gBatt = rnd(45.0f, 110.0f); battT = 0.95f; } }
    if (eclT > 0.0f) eclT -= dt; else { gEcl -= dt; if (gEcl <= 0.0f && !panneau) { gEcl = rnd(40.0f, 120.0f); eclT = 0.42f; eclX = rnd(110.0f, 250.0f); eclGraine = (uint32_t)random(1000000) + 1; } }
  } else { lueurOeil = 1.0f; eclT = 0.0f; }
  gFeuille -= dt; if (gFeuille <= 0) { gFeuille = rnd(2.0f, 4.5f) * (OP.q == 0 ? 2.0f : 1.0f); feuille(); }
  majVisiteur(dt); majMinis(dt); majOeuf(dt); majSocial(dt); majGP(dt); majMainPose(dt); majMainNourr(dt); fam::maj(dt); nat::maj(dt);
  if (H.actif) H.ext += (1.0f - H.ext) * mnf(1.0f, dt * 5.0f); else if (H.ext > 0) { H.ext -= dt * 4.0f; if (H.ext < 0) H.ext = 0; }
  for (int i = 0; i < NPROJ; i++) {      // objets lancés : trajectoire, puis ils retombent et deviennent de l'objet au sol
    Proj& p = proj[i]; if (!p.actif) continue;
    p.t += dt; float vit = hypotf(p.vx, p.vy), gr = 300.0f * (1.0f - lim((vit - 250.0f) / 500.0f, 0.0f, 0.75f));
    p.vy += gr * dt; p.x += p.vx * dt; p.y += p.vy * dt; p.rot += p.vr * dt;
    if (H.actif && p.type != T_OEUF && surPaume(p.x, p.y + 4.0f)) {      // l'objet lancé atterrit dans la paume
      int jm = -1; for (int q = 0; q < 3; q++) if (!H.om[q].actif) { jm = q; break; }
      if (jm >= 0) { H.om[jm].actif = true; H.om[jm].type = p.type; H.om[jm].u = rnd(-5.0f, 5.0f); H.om[jm].t = 0; p.actif = false; son(SN_CROC, 160); eclat(p.x, p.y, 0.15f, 0.7f); continue; }
    }
    for (int k = 0; k < NCB; k++) {      // un objet lancé qui heurte un corbeau (qui n'essaie pas de l'attraper) : il rebondit, le corbeau est bousculé
      Corbeau& c = cr[k]; if (!c.visible || p.dern == k || p.t < 0.06f) continue;
      if (!(c.etat == E_VOL || c.etat == E_PERCHE || c.etat == E_SOL) || c.perch >= 10 || (c.but == B_ATTRAPE && c.pj == i)) continue;
      if (hypotf(p.x - c.x, p.y - c.y) > 7.0f * c.s + 3.0f) continue;
      p.dern = (int8_t)k; float v = hypotf(p.vx, p.vy);
      impulsion(c, p.vx * 0.3f, p.vy * 0.3f); choc(c.x, c.y, nullptr, &c, v); c.surs = 0.3f;
      if (c.etat == E_PERCHE && v > 160.0f && c.objet == 0 && random(100) < 55) chasser(c);
      p.vx *= -0.45f; p.vy = -fabsf(p.vy) * 0.3f - 30.0f; p.vr = -p.vr; break;
    }
    float sol = solHaut(p.x) - 3.2f;
    if (p.y >= sol && p.vy > 0 && p.type == T_OEUF) { oeufExplose(p.x, sol); p.actif = false; }
    else if (p.y >= sol && p.vy > 0) {
      int j = -1; for (int q = 0; q < NOB; q++) if (!ob[q].actif) { j = q; break; }
      if (j >= 0) { Obj& o = ob[j]; o.actif = true; o.type = p.type; o.x = lim(p.x, 12, 308); o.y = sol; o.vy = 0; o.t = 0; o.repos = true; o.reserve = -1; }
      p.actif = false; son(SN_CROC, 90);
    } else if (p.x < bordG() - 6.0f || p.x > bordD() + 6.0f || p.y > 260 || p.t > 6.0f) p.actif = false;
  }
  if (appelOn) { static float tRec = 0; tRec -= dt; if (tRec <= 0) { tRec = 0.8f; for (int k = 0; k < NCB; k++) { Corbeau& c = cr[k]; if ((c.etat == E_PERCHE && c.perch < 10 && c.objet == 0) || c.etat == E_DEHORS) { mettreEnOrbite(c); c.ray = rnd(24, 46); } } } }
  gVisite -= dt; if (gVisite <= 0 && V.ph == V_RIEN && !cbAbsent && !mo::vueFixe()) { if (nbDehors() < 3) invoquer(); else gVisite = 10.0f; }
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
    } else { o.t += dt; if (o.t > (o.reserve >= 0 ? 40.0f : (o.type < T_BRILLANT ? 36.0f : 14.0f))) { o.actif = false; for (int k = 0; k < NCB; k++) if (cr[k].item == i) { cr[k].item = -1; } } }
  }
  for (int i = 0; i < NPART; i++) {
    Plume& p = pl[i]; if (!p.actif) continue;
    p.vie -= dt; if (p.vie <= 0) { p.actif = false; continue; }
    p.x += p.vx * dt; p.y += p.vy * dt; p.vx = p.vx * (1.0f - 1.6f * dt) + 16.0f * sinf(tAnim * 0.6f + p.y * 0.02f) * dt; p.vy = p.vy * (1.0f - 1.6f * dt) + 34.0f * dt; p.rot += p.vr * dt;
  }
  for (int i = 0; i < NFX; i++) { Eclat& e = fx[i]; if (!e.actif) continue; e.t += dt; if (e.t >= e.dur) e.actif = false; }
}

// ---------- dessin de la scène ----------
static void dessinerEclair(M5Canvas& g) {      // éclair rouge lointain, très bref (deux éclats)
  const float u = eclT; bool vif = (u > 0.26f && u < 0.34f) || (u > 0.08f && u < 0.15f); bool pale = (u > 0.34f) || (u > 0.15f && u <= 0.26f);
  if (!vif && !pale) return;
  uint32_t sd = eclGraine; float x = eclX, y = -4.0f;
  for (int i = 0; i < 7; i++) {
    sd = sd * 1103515245UL + 12345UL; float nx = x + ((int)((sd >> 9) % 15) - 7) * 1.3f, ny = y + 11.0f + (float)((sd >> 14) % 6);
    ligne(g, x - 1, y, nx - 1, ny, vif ? C(120, 14, 20) : C(60, 7, 12)); ligne(g, x + 1, y, nx + 1, ny, vif ? C(120, 14, 20) : C(60, 7, 12));
    ligne(g, x, y, nx, ny, vif ? C(255, 150, 130) : C(170, 40, 40));
    if (i == 3 && vif) { float bx = nx + 14.0f, by = ny + 12.0f; ligne(g, nx, ny, bx, by, C(210, 60, 50)); }
    x = nx; y = ny;
  }
}
static void dessinerFiche(M5Canvas& g);
static const int E0 = NCB + NOB + 14;      // premiers rectangles supplémentaires : plumes (2), petits corbeaux (3), éclats (1), puis 6 morceaux du décor animé du monde
static const int EG_PL = NCB + NOB, EG_FX = NCB + NOB + 1, EG_MI = NCB + NOB + 4, EG_DYN = E0 + 6;
static bool chg[NENT], dr[NENT]; static uint32_t hsh[NENT], hshAnc[NENT];      // chg : l'élément a changé depuis l'image précédente ; dr : il faut le dessiner ; hsh : empreinte de son dessin
static inline bool hachable(int e) { return e < NCB + NOB || e == NCB + NOB + 3 || e == NCB + NOB + 5 || e == NCB + NOB + 7 || e == NCB + NOB + 9 || e == NCB + NOB + 10 || e == NCB + NOB + 2 || e >= EG_DYN; }      // éléments dont deux images identiques sont reconnues par l'empreinte (les autres sont toujours redessinés)
static inline Rect rVide() { Rect r; r.x0 = 9997; r.y0 = 9997; r.x1 = -9996; r.y1 = -9996; return r; }
static inline int aireR(const Rect& r) { return (r.x1 - r.x0) * (r.y1 - r.y0); }
static inline Rect unionR(const Rect& a, const Rect& b) { Rect r; r.x0 = a.x0 < b.x0 ? a.x0 : b.x0; r.y0 = a.y0 < b.y0 ? a.y0 : b.y0; r.x1 = a.x1 > b.x1 ? a.x1 : b.x1; r.y1 = a.y1 > b.y1 ? a.y1 : b.y1; return r; }
static void grouper(Rect* t, int n, int s0, int e0, int K) {      // regroupe n petits rectangles en au plus K (les plus proches d'abord) : le premier va dans nouv[s0], les suivants dans nouv[e0], nouv[e0 + 1]...
  if (n > 16) { for (int i = 1; i < n; i++) t[0] = unionR(t[0], t[i]); n = 1; }
  while (n > K) {
    int bi = 0, bj = 1, bc = 0x7fffffff;
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) { const int c = aireR(unionR(t[i], t[j])) - aireR(t[i]) - aireR(t[j]); if (c < bc) { bc = c; bi = i; bj = j; } }
    t[bi] = unionR(t[bi], t[bj]); t[bj] = t[--n];
  }
  nouv[s0] = n > 0 ? t[0] : rVide();
  for (int k = 1; k < K; k++) nouv[e0 + k - 1] = k < n ? t[k] : rVide();
}
static inline bool horsChamp(const Corbeau& c) { const float x = TX(c.x), y = TY(c.y + c.bob), m = 70.0f * c.s * zs + 12.0f; return x < -m || x > 320.0f + m || y < -m || y > 240.0f + m; }      // corbeau entierement hors de l'ecran (vues zoomees) : ni calcul ni dessin
static void dessinerPlume(M5Canvas& g, const Plume& p) {
  float cs = cosf(p.rot), sn = sinf(p.rot), L = 3.6f * (0.5f + p.vie / p.dur * 0.5f), w = 1.3f;
  tri(g, p.x - cs * L, p.y - sn * L, p.x + cs * L, p.y + sn * L, p.x - sn * w, p.y + cs * w, C(5, 3, 6));
  if (!qDeg) tri(g, p.x - cs * L, p.y - sn * L, p.x + cs * L, p.y + sn * L, p.x + sn * w, p.y - cs * w, C(5, 3, 6));      // allègement : une seule moitié de plume (3 pixels de large, invisible)
}
static void dessinerEclatFx(M5Canvas& g, const Eclat& f) {
  float u = f.t / f.dur, a = 1.0f - u, r = (4.0f + 14.0f * u) * f.taille;
  disque(g, f.x, f.y, r, C((uint8_t)(150 * a + 20), (uint8_t)(10 * a), (uint8_t)(12 * a)));
  disque(g, f.x, f.y, r * 0.6f, C(255, (uint8_t)(60 * a + 20), (uint8_t)(40 * a)));
  disque(g, f.x, f.y, r * 0.25f, C(255, 230, 210));
}
static void dessinerEntites(M5Canvas& g, bool sech) {
  sec = sech;
  // sech : premiere passe, rien n'est dessine : boites englobantes, empreintes ET enregistrement des primitives ; seconde passe : seuls les elements marques dr sont dessines, en rejouant l'enregistrement (sans recalculer)
  if (sech) { cmdN = 0; cmdOvf = false; recOn = false; }
  auto veut = [&](int e) { return sech || dr[e]; };
  auto fin = [&](int e) { if (sech) { nouv[e] = bbRect(); hsh[e] = hAcc; } };
  auto rec = [&](int e, auto&& fn) {
    if (sech) { cmdD[e] = cmdN; recOn = true; bbReset(); fn(); recOn = false; cmdF[e] = cmdN; nouv[e] = bbRect(); hsh[e] = hAcc; }
    else if (dr[e]) { if (cmdOvf) fn(); else rejouer(g, cmdD[e], cmdF[e]); }
  };
  if (mo::camOn()) {      // decor anime de la vue large / du suivi (lanterne, brume, halos, eau), en morceaux separes
    for (int k = 0; k < 8; k++) rec(EG_DYN + k, [&] { mo::dynPart(k); });
  } else if (sech) for (int k = 0; k < 8; k++) nouv[EG_DYN + k] = rVide();
  // objets
  for (int i = 0; i < NOB; i++) {
    rec(NCB + i, [&] {
      Obj& o = ob[i];
      if (o.actif) {
        float tw = (o.type >= T_BRILLANT && o.repos) ? ((sinf(millis() * 0.0075f + i) > 0.55f) ? 1.0f : 0.0f) : 0.0f;
        dessinerObjet(g, o.type, o.x, o.y, o.type >= T_BAGUE ? 5.5f : 4.6f, o.repos ? 0.0f : o.y * 0.12f, tw);
      }
    });
  }
  rec(NCB + NOB + 3, [&] { if (V.ph == V_PRESENT) dessinerVisiteur(g); });
  rec(NCB + NOB + 5, [&] { dessinerMain(g); });
  rec(NCB + NOB + 7, [&] { if (OE.actif) dessinerOeuf(g); });      // le visiteur, avant les corbeaux : ils se posent devant lui
  rec(NCB + NOB + 13, [&] { mo::dessinBetes(g); });      // les petites betes, au sol, derriere les corbeaux
  rec(NCB + NOB + 9, [&] { fam::nid(g); });      // le nid, derriere les corbeaux
  { const uint32_t u_ = micros(); for (int k = 0; k < NCB; k++) rec(k, [&] { if (cr[k].visible && !horsChamp(cr[k])) { dessinerCorbeau(g, cr[k]); fam::sur(g, k); nat::sur(g, k); tendreSur(g, k); } }); uCorb += micros() - u_; }
  rec(NCB + NOB + 10, [&] { fam::devant(g); });      // les brindilles du bord, devant le parent assis
  // petits corbeaux de la nuee : un rectangle par groupe proche (au plus quatre)
  if (sech) {
    Rect t[NMINI]; int n = 0; cmdD[SL_MI] = cmdN; recOn = true;
    for (int i = 0; i < NMINI; i++) if (mini[i].actif) { bbReset(); dessinerMini(g, mini[i]); t[n++] = bbRect(); }
    recOn = false; cmdF[SL_MI] = cmdN; grouper(t, n, EG_MI, E0 + 2, 4);
  } else if (cmdOvf) { for (int i = 0; i < NMINI; i++) if (mini[i].actif) dessinerMini(g, mini[i]); }
  else rejouer(g, cmdD[SL_MI], cmdF[SL_MI]);
  rec(NCB + NOB + 6, [&] {
    for (int i = 0; i < NPROJ; i++) { Proj& p = proj[i]; if (p.actif) dessinerObjet(g, p.type, p.x, p.y, p.type >= T_BAGUE ? 5.5f : 4.6f, p.rot, 0.0f); }
    if (VS.on) dessinerVisee(g);
    if (FR.on && cr[FR.c].etat == E_TENU) dessinerFronde(g);
  });
  // plumes : un rectangle par groupe proche (au plus trois)
  if (sech) {
    Rect t[NPART]; int n = 0; cmdD[SL_PL] = cmdN; recOn = true;
    for (int i = 0; i < NPART; i++) if (pl[i].actif) { bbReset(); dessinerPlume(g, pl[i]); t[n++] = bbRect(); }
    recOn = false; cmdF[SL_PL] = cmdN; grouper(t, n, EG_PL, E0, 3);
  } else if (cmdOvf) { for (int i = 0; i < NPART; i++) if (pl[i].actif) dessinerPlume(g, pl[i]); }
  else rejouer(g, cmdD[SL_PL], cmdF[SL_PL]);
  // eclats rouges de l'oeil : au plus deux rectangles
  if (sech) {
    Rect t[NFX]; int n = 0; cmdD[SL_FX] = cmdN; recOn = true;
    for (int i = 0; i < NFX; i++) if (fx[i].actif) { bbReset(); dessinerEclatFx(g, fx[i]); t[n++] = bbRect(); }
    recOn = false; cmdF[SL_FX] = cmdN; grouper(t, n, EG_FX, E0 + 5, 2);
  } else if (cmdOvf) { for (int i = 0; i < NFX; i++) if (fx[i].actif) dessinerEclatFx(g, fx[i]); }
  else rejouer(g, cmdD[SL_FX], cmdF[SL_FX]);
  rec(NCB + NOB + 2, [&] { nat::dessinerSteles(g); });      // les steles des disparus (au pied de l'arbre, discretes)
  rec(NCB + NOB + 8, [&] { if (OP.hor == 2 && eclT > 0.0f && !qMin) dessinerEclair(g); });
  rec(E0 + 14, [&] { nat::dessinerPred(g); });
  rec(E0 + 15, [&] { nat::dessinerSang(g); });      // 3.1.41 : les gouttes de sang (Realiste), tres petites et breves      // le predateur (rapace, hibou, renard) : une silhouette de plus, par-dessus les corbeaux
  rec(NCB + NOB + 11, [&] { dessinerGP(g); });      // la visite d'un corbeau au gros plan : redessinee par zones, comme le reste
  if (veut(NCB + NOB + 12)) { bbReset(); if (OP.fiche || infoT) dessinerFiche(g); fin(NCB + NOB + 12); }      // la fiche flottante d'un corbeau (double toucher)
  sec = false;
}

static void ajoutRect(Rect r) {
  if (r.x0 < 0) r.x0 = 0; if (r.y0 < 0) r.y0 = 0; if (r.x1 > 320) r.x1 = 320; if (r.y1 > H_SCENE) r.y1 = H_SCENE;
  if (r.x1 <= r.x0 || r.y1 <= r.y0) return;
  if (nListe < 96) liste[nListe++] = r;
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
  uint8_t* d = (uint8_t*)cv.getBuffer(); const uint8_t* s = (const uint8_t*)fondAct->getBuffer(); if (!d || !s) return;
  for (int y = r.y0; y < r.y1; y++) memcpy(d + ((size_t)y * 320 + r.x0) * bpp, s + ((size_t)y * 320 + r.x0) * bpp, (size_t)(r.x1 - r.x0) * bpp);
}
static void pousser(const Rect& r) {
  M5.Display.setClipRect(r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0); cv.pushSprite(0, 0); M5.Display.clearClipRect();
}

// ---------- panneau de réglages (par-dessus la scène : grosses tuiles, fermeture facile) ----------
static void construireFondLarge();
static void syncUI();
static void appliquerNb() {      // nombre de corbeaux visibles (1 à 8) ; l'emplacement 8 est celui du petit corbeau
  nAct = OP.nb; nat::oublier(nAct);      // le reglage est un maximum : les emplacements au-dela sont oublies ; les disparus restent absents
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k];
    if (k >= nAct || natMort(k)) { if (c.etat != E_OFF) { c.etat = E_OFF; c.visible = false; c.perch = -1; c.but = B_RIEN; c.objet = 0; c.pj = -1; c.item = -1; c.rapporte = false; c.fuite = false; c.aWp = false; } }
    else if (c.etat == E_OFF) { c.etat = E_DEHORS; c.visible = false; c.t = 0; c.tfin = rnd(0.8f, 2.5f); c.but = B_RIEN; c.perch = -1; c.wl = c.wlC = 0; c.fan = c.fanC = 0; }
  }
  for (int i = 0; i < NOB; i++) if (ob[i].actif && ob[i].reserve >= 0) ob[i].reserve = -1;
}
static void appliquerTaille(float ancien) {      // la taille change tout de suite, sans toucher aux proportions entre corbeaux ni à l'arbre
  const float r = tailleF() / ancien; if (r < 0.01f || fabsf(r - 1.0f) < 0.001f) return;
  for (int k = 0; k < NCB; k++) cr[k].s *= r;
  for (int k = 0; k < NAD; k++) { mo::sBase[k] *= r; mo::rt[k].sB *= r; }
}
static void tuile(M5Canvas& g, int x, int y, int w, int h, const char* lab, const char* val) {
  g.fillRoundRect(x, y, w, h, 8, C(44, 9, 18)); g.drawRoundRect(x, y, w, h, 8, C(160, 40, 48));
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_center); g.drawString(lab, x + w / 2, y + 2);
  g.setFont(&fonts::lgfxJapanGothic_16); g.setTextColor(C(255, 236, 225)); g.setTextDatum(middle_center); g.drawString(val, x + w / 2, y + h - 10); g.setTextDatum(top_left);
}
static uint8_t oeufMsg = 0; static uint32_t oeufMsgT = 0;      // 3.1.40 : message de la tuile Faire pondre un oeuf (1 : deja un, 2 : impossible)
static uint8_t natMsg = 0; static uint32_t natMsgT = 0;      // 3.1.36 : message de la page Envoyer un predateur
static uint8_t panPage = 0;      // 0 : réglages généraux ; 1 : fréquence des événements
static void segBarre(M5Canvas& g, int x, int y, int w, int h, int n, int sel, const char* const* lab) {      // barre tactile à n segments : le segment choisi est allumé
  g.setFont(&fonts::lgfxJapanGothic_16); g.setTextDatum(middle_center);
  for (int i = 0; i < n; i++) {
    int x0 = x + (w * i) / n, x1 = x + (w * (i + 1)) / n; bool on = (i == sel);
    g.fillRoundRect(x0 + 1, y, x1 - x0 - 2, h, 6, on ? C(170, 34, 40) : C(44, 9, 18)); g.drawRoundRect(x0 + 1, y, x1 - x0 - 2, h, 6, on ? C(255, 150, 120) : C(120, 30, 40));
    g.setTextColor(on ? C(255, 250, 240) : C(205, 145, 145)); g.drawString(lab[i], (x0 + x1) / 2, y + h / 2);
  }
  g.setTextDatum(top_left);
}
static void dessinerPanneau() {
  M5Canvas& g = cv;
  g.fillScreen(C(14, 2, 6));
  g.fillRect(0, 0, 320, 26, C(60, 10, 20)); g.drawFastHLine(0, 26, 320, C(214, 60, 52));
  g.setFont(&fonts::lgfxJapanGothic_16); g.setTextDatum(middle_left); g.setTextColor(C(255, 236, 225));
  if (millis() < tDiagSon) { g.setTextColor(C(150, 255, 170)); g.setFont(&fonts::lgfxJapanGothic_12); g.drawString(diagSon, 6, 13); g.setFont(&fonts::lgfxJapanGothic_16); g.setTextColor(C(255, 236, 225)); } else g.drawString(panPage == 13 ? "Nature : réalisme et capacité" : panPage == 12 ? "Fiche de la colonie" : panPage == 11 ? "Envoyer un prédateur" : panPage == 10 ? "Nature : prédateurs et vie sauvage" : panPage == 9 ? "Vie, nid et couple" : panPage == 8 ? "Caméra automatique" : panPage == 7 ? "Fluidité : images/s et écran" : panPage == 6 ? "Main, vue et interface" : panPage == 5 ? "Famille : parent attitré et petits" : panPage == 4 ? "Fonctions : barre, fiche, oeuf" : panPage == 3 ? "Monde : décor et échanges" : (panPage == 2 ? "Image : lumière et fluidité" : (panPage ? "Fréquence des événements" : "Réglages des corbeaux")), 8, 13);
  g.fillRoundRect(240, 2, 76, 22, 8, C(110, 24, 30)); g.drawRoundRect(240, 2, 76, 22, 8, C(255, 120, 90)); g.setTextDatum(middle_center); g.drawString("FERMER", 278, 13);
  if (panPage == 0) {
    // volume : - barre + et TEST SON
    g.fillRoundRect(4, 30, 312, 32, 8, C(44, 9, 18)); g.drawRoundRect(4, 30, 312, 32, 8, C(160, 40, 48));
    g.setTextDatum(middle_left); g.setTextColor(C(255, 236, 225)); g.drawString("Vol.", 10, 46);
    g.fillRoundRect(44, 33, 44, 26, 6, C(90, 18, 26)); g.drawRoundRect(44, 33, 44, 26, 6, C(255, 120, 90)); g.setTextDatum(middle_center); g.drawString("-", 66, 46);
    g.fillRoundRect(92, 41, 92, 10, 4, C(60, 12, 20)); g.fillRoundRect(92, 41, 1 + (int)(91.0f * R.volume / 255.0f), 10, 4, C(80, 230, 130));
    g.fillRoundRect(190, 33, 44, 26, 6, C(90, 18, 26)); g.drawRoundRect(190, 33, 44, 26, 6, C(255, 120, 90)); g.drawString("+", 212, 46);
    g.fillRoundRect(240, 33, 72, 26, 6, C(20, 90, 50)); g.drawRoundRect(240, 33, 72, 26, 6, C(120, 255, 160)); g.drawString("TEST SON", 276, 46);
    const char* SEC[3] = {"Faible", "Normale", "Forte"}; const char* AMB[3] = {"Lune + étoiles", "Brume rouge", "Nuit sombre"};
    char nb[8]; snprintf(nb, sizeof(nb), "%d", (int)nbMax);
    tuile(g, 4, 65, 153, 32, "Sons", sonsReels ? "Réels" : "Synthétiques"); tuile(g, 163, 65, 153, 32, "Corbeaux", nb);
    tuile(g, 4, 100, 153, 32, "Secousse", SEC[OP.sec]); tuile(g, 163, 100, 153, 32, "Vibration", OP.vib ? "Oui" : "Non");
    tuile(g, 4, 135, 153, 32, "Ambiance", AMB[OP.amb]); tuile(g, 163, 135, 153, 32, "Lumière", OP.hor == 2 ? "Horreur" : (OP.hor == 1 ? "Sombre" : "Clair"));
    tuile(g, 4, 170, 153, 32, "Colonie", "Fiche >"); tuile(g, 163, 170, 153, 32, "Bouton Visiteur", btnVis ? "Affiché" : "Masqué");
  } else if (panPage == 7) {      // fluidité : compteur d'images/s, cadence maximale, allègement automatique, écran rapide (clés cbfps, cbcad, cbdeg, cbecr)
    static const char* const LM3[3] = {"Non", "Images/s", "Détails"}; static const char* const LC3[3] = {"20", "25", "30"}; static const char* const LO2[2] = {"Non", "Oui"}; static const char* const LE3[3] = {"Standard", "Essai 80 MHz", "Garder"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Compteur d'images/s : jamais affiché à l'écran", 6, 31); g.drawString("Il reste lisible dans /api/etat (fps, fpsmin, msimage, msmax).", 6, 49);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Cadence maximale (images par seconde)", 6, 73); segBarre(g, 4, 87, 312, 24, 3, OP.cad, LC3);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Allègement automatique des détails si l'image ralentit", 6, 115); segBarre(g, 4, 129, 312, 24, 2, OP.deg, LO2);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(ecrEssaiFin ? C(255, 205, 130) : C(205, 145, 145)); g.drawString(ecrEssaiFin ? "Essai en cours : touche Garder si l'image est nette (10 s)" : "Écran rapide 80 MHz (essai de 10 s, à garder si l'image est nette)", 6, 157); segBarre(g, 4, 171, 312, 24, 3, ecr80 ? (OP.ecr ? 2 : 1) : 0, LE3);
  } else if (panPage == 8) {      // caméra : vues spontanées (clé cbcam)
    static const char* const LO[2] = {"Non", "Oui"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Caméra automatique (vue d'ensemble pendant les échanges)", 6, 31); segBarre(g, 4, 45, 312, 24, 2, OP.cauto, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Non : la vue choisie reste, sans gros plan ni visiteur spontanés.", 6, 77);
    g.drawString("Le bouton Vue ou la page Main et vue changent de vue.", 6, 93); g.drawString("En Suivi, touche un corbeau pour le suivre.", 6, 109);
  } else if (panPage == 12) {      // 3.1.40 : fiche de la colonie (compteurs deja existants, rien de nouveau en memoire)
    int nAd = 0, nBl = 0; for (int k = 0; k < NAD; k++) if (cr[k].etat != E_OFF) nAd++; for (int k = 0; k < NCB; k++) if (nat::sante(k) > 0) nBl++;
    char v1[24], v2[24], v3[24], v4[24], v5[24], v6[24], v7[24]; snprintf(v1, sizeof(v1), "%d / %d", nAd, (int)nbMax); snprintf(v2, sizeof(v2), "%u", (unsigned)nbOeufs); snprintf(v3, sizeof(v3), "%u", (unsigned)nat::SV.nMort); snprintf(v4, sizeof(v4), "%d", nBl);
    snprintf(v5, sizeof(v5), "%u", (unsigned)nat::SV.nBless); snprintf(v6, sizeof(v6), "%u tour, %u mare", (unsigned)mo::cTour, (unsigned)mo::cMare); snprintf(v7, sizeof(v7), "%u", (unsigned)nat::SV.nPred);
    tuile(g, 4, 32, 153, 32, "Adultes / maximum", v1); tuile(g, 163, 32, 153, 32, "Petit corbeau", cr[PC].etat != E_OFF ? "Présent" : "Absent");
    tuile(g, 4, 67, 153, 32, "Oeufs éclos", v2); tuile(g, 163, 67, 153, 32, "Stèles", v3);
    tuile(g, 4, 102, 153, 32, "Blessés maintenant", v4); tuile(g, 163, 102, 153, 32, "Blessures en tout", v5);
    tuile(g, 4, 137, 153, 32, "Trésors cachés", v6); tuile(g, 163, 137, 153, 32, "Attaques de prédateurs", v7);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Faire pondre un oeuf : Réglages, page Fonctions.", 6, 176); g.drawString("Chiffres gardés depuis le début de la colonie.", 6, 190);
  } else if (panPage == 13) {      // 3.1.41 : realisme de la nature et capacite du territoire (cles cbnrea, cbncap)
    static const char* const LRE[2] = {"Sobre", "Réaliste"}; static const char* const LCA3[6] = {"3", "4", "5", "6", "7", "8"};
    char sb[80]; snprintf(sb, sizeof(sb), "Capacité du territoire : %d oiseaux (là : %d)", (int)nat::RN.capa, nat::pretN ? nat::vivTot() : 0);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Réalisme : Sobre = pas de sang, blessures légères", 6, 31); segBarre(g, 4, 45, 312, 24, 2, nat::RN.real, LRE);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString(sb, 6, 73); segBarre(g, 4, 87, 312, 24, 6, (int)nat::RN.capa - 3, LCA3);
    g.drawString("Au-dessus : plus de prédateurs, plus de bagarres, moins d'oeufs.", 6, 119); g.drawString("En dessous : les prédateurs sont plus rares.", 6, 135);
    g.drawString("Jamais le dernier corbeau ni le couple d'un nid occupé.", 6, 151); g.drawString("Mode Protecteur (page précédente) : effets atténués.", 6, 167);
  } else if (panPage == 11) {      // 3.1.36 : envoyer un predateur tout de suite (rapace, hibou, renard), pour le voir et le tester
    static const char* const LN[3] = {"Rapace", "Hibou", "Renard"}; static const char* const LS[3] = {"du ciel", "silencieux", "au sol"}; static const char* const LMO[3] = {"Jamais", "Rare", "Réaliste"};
    static const char* const LM[5] = {"", "Un prédateur est déjà là.", "Pas de corbeau visible pour celui-là.", "Impossible maintenant (visite, absence...).", "Allègement automatique : prédateur coupé."};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Touche pour l'envoyer tout de suite (essai) :", 6, 31);
    for (int i = 0; i < 3; i++) {
      const int bx = 4 + i * 106;
      g.fillRoundRect(bx, 48, 100, 64, 10, C(120, 28, 34)); g.drawRoundRect(bx, 48, 100, 64, 10, C(255, 120, 90));
      g.setTextDatum(middle_center); g.setFont(&fonts::lgfxJapanGothic_16); g.setTextColor(C(255, 250, 240)); g.drawString(LN[i], bx + 50, 48 + 24);
      g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(255, 190, 170)); g.drawString(LS[i], bx + 50, 48 + 47);
    }
    g.setTextDatum(top_left); g.setFont(&fonts::lgfxJapanGothic_12);
    if (millis() < natMsgT && natMsg >= 1 && natMsg <= 4) { g.setTextColor(C(255, 220, 120)); g.drawString(LM[natMsg], 6, 118); } else { g.setTextColor(C(150, 255, 170)); g.drawString("Le panneau se ferme, il arrive.", 6, 118); }
    g.setTextColor(C(205, 145, 145));
    g.drawString("Cible : le plus faible. Alarme : les audacieux foncent.", 6, 138);
    g.drawString("Repousse-le : toucher, main, fronde, nourriture.", 6, 154);
    { char sb[64]; snprintf(sb, sizeof(sb), "Mort : %s (jamais le dernier corbeau)", LMO[nat::RN.mort > 2 ? 0 : nat::RN.mort]); g.drawString(sb, 6, 170); }
    g.drawString(nat::RN.pred == 0 ? "Prédateurs sur Jamais : démo, blessé léger au plus." : "Hibou et renard viennent même de jour ici.", 6, 186);
  } else if (panPage == 10) {      // nature : predateurs, blessures, mort, mode (cles cbnpre, cbnble, cbnmor, cbnmod)
    static const char* const LPR[3] = {"Jamais", "Rares", "Fréquents"}; static const char* const LBL[2] = {"Non", "Oui"}; static const char* const LMO[3] = {"Jamais", "Rare", "Réaliste"}; static const char* const LMD[2] = {"Nature", "Protecteur"};
    char sb[64]; snprintf(sb, sizeof(sb), "Mode : Nature ou Protecteur   (stèles : %u)", (unsigned)nat::SV.nMort);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Prédateurs : rapace, hibou la nuit, renard", 6, 31); segBarre(g, 4, 45, 312, 24, 3, nat::RN.pred, LPR);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Blessures : boiter, guérir, cicatrices", 6, 73); segBarre(g, 4, 87, 312, 24, 2, nat::RN.bles, LBL);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Mort (jamais le dernier ; stèle, oeuf de secours)", 6, 115); segBarre(g, 4, 129, 312, 24, 3, nat::RN.mort, LMO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString(sb, 6, 157); segBarre(g, 4, 171, 312, 24, 2, nat::RN.mode, LMD);
  } else if (panPage == 9) {      // vie, nid et couple (clés cbafnai, cbafnui, cbafcpl, cbafnid)
    static const char* const LN[2] = {"Oeuf attrapé", "Libre (comme avant)"}; static const char* const LU[2] = {"Au nid", "Sur l'arbre"}; static const char* const LC[2] = {"Non", "Oui"}; static const char* const LB[3] = {"Lente", "Normale", "Rapide"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Naissance d'un petit (N corbeaux = N exactement)", 6, 31); segBarre(g, 4, 45, 312, 24, 2, fam::AFR.naiss, LN);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Nuit : le couple dort au nid, les autres hors de la grosse branche", 6, 73); segBarre(g, 4, 87, 312, 24, 2, fam::AFR.nuit, LU);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Couple : parade, nourrir, couver à tour de rôle, rester ensemble", 6, 115); segBarre(g, 4, 129, 312, 24, 2, fam::AFR.couple, LC);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Construction du nid (brindilles apportées une à une)", 6, 157); segBarre(g, 4, 171, 312, 24, 3, fam::AFR.nidv, LB);
  } else if (panPage == 6) {      // main et vue : main ouverte paume vers le haut, visites dans la paume, recul de la vue (clés cbpaume, cbpvis, cbrecul)
    static const char* const LO[2] = {"Non", "Oui"}; static const char* const LPV[4] = {"Jamais", "Rare", "Normal", "Souvent"}; static const char* const LRC[3] = {"Normal", "Arbre", "Mare"}; static const char* const LLI[3] = {"Bâtiments", "Large", "Nid"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    static const char* const LCA[3] = {"Rares", "Normaux", "Fréquents"}; static const char* const LV6[6] = {"Normal", "Arbre", "Mare", "Bâtim.", "Large", "Nid"};
    (void)LRC; (void)LLI;
    g.drawString("Main ouverte, paume vers le haut (mode Main)", 6, 28); segBarre(g, 4, 40, 312, 20, 2, OP.paume, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Visites d'un corbeau dans la paume", 6, 62); segBarre(g, 4, 74, 312, 20, 4, OP.pvis, LPV);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Tendresse (caresser, serrer)", 6, 96); g.drawString("Câlins sur les doigts", 112, 96); segBarre(g, 4, 108, 100, 20, 2, OP.tend, LO); segBarre(g, 108, 108, 208, 20, 3, OP.calin, LCA);      // 3.1.39
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Vue (elle reste fixe) : normale, arbre, mare, bâtiments, large, nid", 6, 130); segBarre(g, 4, 142, 312, 20, 6, (OP.recul <= 5) ? OP.recul : -1, LV6);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Interface : boutons du haut, du bas et nourriture", 6, 164); { static const char* const LUI[2] = {"Ancienne", "Nouvelle"}; segBarre(g, 4, 176, 312, 20, 2, OP.ui, LUI); }      // 3.1.44
  } else if (panPage == 5) {      // famille : parent attitré, entraide, protection, croissance du petit (clés cbfam, cbfaid, cbfpro, cbfvit)
    static const char* const LF[2] = {"Désactivée", "Activée"}; static const char* const LA[3] = {"Non", "Selon caractère", "Tous aident"}; static const char* const LP[3] = {"Douce", "Normale", "Forte"}; static const char* const LV[3] = {"Lente", "Normale", "Rapide"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Famille (parent attitré, couple, nid, petits)", 6, 31); segBarre(g, 4, 45, 312, 24, 2, fam::RG.on ? 1 : 0, LF);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Les autres corbeaux aident à nourrir et à garder le petit", 6, 73); segBarre(g, 4, 87, 312, 24, 3, fam::RG.aide, LA);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Protection du petit (ailes ouvertes, alarme, poursuite)", 6, 115); segBarre(g, 4, 129, 312, 24, 3, fam::RG.prot, LP);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Vitesse de vie (poussin, jeune, moyen, adulte, vieux)", 6, 157); segBarre(g, 4, 171, 312, 24, 3, fam::RG.vit, LV);
  } else if (panPage == 4) {      // fonctions : barre de metal, fiche, nourriture, taille, oeuf a la demande (3.1.40)
    static const char* const LO[2] = {"Non", "Oui"}; static const char* const LT3[3] = {"Petit", "Normal", "Grand"}; static const char* const LPO[1] = {"Faire pondre un oeuf"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Barre de métal pour l'approche du corbeau (gros plan)", 6, 28); segBarre(g, 4, 41, 312, 20, 2, OP.barre, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Fiche et nom au double toucher sur un corbeau", 6, 63); segBarre(g, 4, 76, 312, 20, 2, OP.fiche, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Nourriture : toucher = poser, glisser = lancer", 6, 98); segBarre(g, 4, 111, 312, 20, 2, OP.lance, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Taille des corbeaux (Normal : un peu plus gros qu'avant)", 6, 133); segBarre(g, 4, 146, 312, 20, 3, OP.taille, LT3);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor((millis() < oeufMsgT && oeufMsg) ? C(255, 220, 120) : C(205, 145, 145));
    g.drawString((millis() < oeufMsgT && oeufMsg == 1) ? "Il y a déjà un oeuf." : ((millis() < oeufMsgT && oeufMsg == 2) ? "Impossible maintenant (visite en cours)." : "Un oeuf apparaît tout de suite (le panneau se ferme)"), 6, 168); segBarre(g, 4, 181, 312, 20, 1, 0, LPO);
  } else if (panPage == 3) {      // monde : décor (tour en ruine / simple) et échanges à longue distance (monde.h)
    static const char* const LD[2] = {"Simple", "Tour en ruine"}; static const char* const LO[2] = {"Non", "Oui"}; static const char* const LDC[3] = {"Complet", "Allégé", "Minimal"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Décor du monde (tour, portique, stèles, brume)", 6, 28); segBarre(g, 4, 40, 312, 16, 2, mo::RGM.tour ? 1 : 0, LD);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Échanges à longue distance (appels, poursuites, vols)", 6, 56); segBarre(g, 4, 68, 312, 16, 2, mo::RGM.lng ? 1 : 0, LO);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Décor : arrière-plan complet, allégé ou minimal", 6, 84); segBarre(g, 4, 96, 312, 16, 3, OP.dec, LDC);
    { static const char* const LB[3] = {"Non", "Peu", "Beaucoup"}; g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Petites bêtes à attraper (escargots, lézards, insectes)", 6, 112); segBarre(g, 4, 124, 312, 16, 3, mo::RGM.bet, LB); }
    { static const char* const LT[3] = {"Non", "Rares", "Fréquents"}; char tb[72]; snprintf(tb, sizeof(tb), "Trésors cachés (tour, mare) : cache %u tour, %u mare", (unsigned)mo::cTour, (unsigned)mo::cMare); g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString(tb, 6, 140); segBarre(g, 4, 152, 312, 16, 3, mo::RGM.tres, LT); }
    { static const char* const LX[3] = {"Rares", "Normales", "Fréquentes"}; g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Excursions lointaines (tour, stèles, portique)", 6, 168); segBarre(g, 4, 180, 312, 16, 3, mo::RGM.exc, LX); }
  } else if (panPage == 2) {
    static const char* const LL[3] = {"Clair", "Sombre", "Horreur"}; static const char* const LQ[3] = {"Fluide", "Normal", "Riche"}; static const char* const LM[2] = {"Non", "Oui"}; static const char* const LM3p[3] = {"Non", "Images/s", "Détails"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Lumière de la scène (Sombre par défaut)", 6, 31); segBarre(g, 4, 45, 312, 24, 3, OP.hor, LL);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Qualité des effets (Fluide = effets réduits, allège seul)", 6, 73); segBarre(g, 4, 87, 312, 24, 3, OP.q, LQ);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Ralentissement au repos (sur pile seulement)", 6, 115); { static const char* const LR[3] = {"Jamais", "Après 5 min", "Après 15 min"}; segBarre(g, 4, 129, 312, 24, 3, reposNiv, LR); }
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Présence : animations réduites quand personne n'est là", 6, 157); segBarre(g, 4, 171, 312, 24, 2, presenceOn ? 1 : 0, LM);
  } else {
    static const char* const L5[5] = {"1", "2", "3", "4", "5"}; static const char* const L4[4] = {"Jamais", "Rare", "Normale", "Souvent"}; static const char* const L3[3] = {"Jamais", "Rare", "Souvent"};
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.setTextDatum(top_left);
    g.drawString("Activité des corbeaux  (1 calme, 5 agité)", 6, 31); segBarre(g, 4, 45, 312, 24, 5, OP.act - 1, L5);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Visites d'un corbeau au gros plan", 6, 73); segBarre(g, 4, 87, 312, 24, 4, OP.gp, L4);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Visiteur mystérieux", 6, 115); segBarre(g, 4, 129, 312, 24, 3, OP.vis, L3);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(205, 145, 145)); g.drawString("Oeufs pondus", 6, 157); segBarre(g, 4, 171, 312, 24, 4, OP.oe, L4);
  }
  g.fillRoundRect(4, 206, 152, 30, 8, C(70, 16, 28)); g.drawRoundRect(4, 206, 152, 30, 8, C(214, 90, 90));
  g.fillRoundRect(164, 206, 152, 30, 8, C(110, 24, 30)); g.drawRoundRect(164, 206, 152, 30, 8, C(255, 120, 90));
  g.setFont(&fonts::lgfxJapanGothic_16); g.setTextDatum(middle_center); g.setTextColor(C(255, 236, 225));
  g.drawString(panPage == 13 ? "Envoyer >" : panPage == 12 ? "< Réglages" : panPage == 11 ? "< Réglages" : panPage == 10 ? "Suite >" : panPage == 9 ? "Nature >" : panPage == 8 ? "Vie et nid >" : panPage == 7 ? "Caméra >" : panPage == 6 ? "Fluidité >" : panPage == 5 ? "Main et vue >" : panPage == 4 ? "Famille >" : (panPage == 3 ? "Fonctions >" : (panPage == 2 ? "Monde >" : (panPage ? "Image >" : "Fréquence >"))), 80, 221); g.drawString("FERMER", 240, 221); g.setTextDatum(top_left);
  cv.pushSprite(0, 0);
}
static void ouvrirPanneau() {
  if (appelOn) appelFin(); if (H.actif) retirerMain(); VS.on = false; FR.on = false; D.bas = false; tpCible = 0;
  panneau = true; panSale = true; panPage = 0;
}
static void fermerPanneau() {
  panneau = false; plein = true; tDernier = millis();
  if (ambChange) { ambChange = false; ldMode = 1; ldPas = 0; ldAff = false; }      // 3.1.32 : le decor est reconstruit par etapes, sous un ecran de chargement, au lieu de geler l'image
}
static int natEnvoyer(int kind) {      // 3.1.36 : bouton Envoyer un predateur et /api/reglages?predateur= : ferme le panneau puis declenche ; 0 fait, 1 deja un, 2 pas de cible, 3 impossible, 4 allegement
  const int r = nat::peut(kind); if (r) return r;
  if (panneau) fermerPanneau();
  return nat::envoyer(kind);
}
static uint8_t panSuiv(uint8_t p) { return p == 13 ? 11 : (p == 10 ? 13 : (p >= 12 ? 0 : (uint8_t)((p + 1) % 12))); }      // 3.1.41 : la page 13 (suite de Nature) vient apres la page 10
static void touchPanneau(int x, int y) {
  if (y < 28 && x >= 236) { fermerPanneau(); return; }
  if (y >= 206) { if (x >= 160) fermerPanneau(); else { panPage = panSuiv(panPage); panSale = true; } return; }
  if (panPage == 12) return;      // 3.1.40 : fiche de la colonie : lecture seule
  if (panPage == 11) {      // envoyer un predateur : trois grosses touches (rapace, hibou, renard)
    if (y >= 44 && y < 116) {
      const int kd = (x < 108) ? 1 : (x < 214 ? 2 : 3);
      const int r = natEnvoyer(kd);
      if (OP.vib) vibrer(r ? 20 : 40);
      if (r) { natMsg = (uint8_t)r; natMsgT = millis() + 4000; panSale = true; }
    }
    return;
  }
  if (panPage == 13) {      // 3.1.41 : realisme (cle cbnrea) et capacite du territoire (cle cbncap)
    if (y >= 42 && y < 72) { nat::RN.real = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbnrea", nat::RN.real); }
    else if (y >= 84 && y < 114) { nat::RN.capa = (uint8_t)(3 + constrain((x - 4) * 6 / 312, 0, 5)); prefs.putUChar("cbncap", nat::RN.capa); nat::gPred = nat::delaiPred(); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 10) {      // nature (cles cbnpre, cbnble, cbnmor, cbnmod)
    if (y >= 42 && y < 72) { nat::RN.pred = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbnpre", nat::RN.pred); nat::gPred = nat::delaiPred(); }
    else if (y >= 84 && y < 114) { nat::RN.bles = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbnble", nat::RN.bles); }
    else if (y >= 126 && y < 156) { nat::RN.mort = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbnmor", nat::RN.mort); }
    else if (y >= 168 && y < 198) { nat::RN.mode = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbnmod", nat::RN.mode); nat::gPred = nat::delaiPred(); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 9) {      // vie, nid et couple (clés cbafnai, cbafnui, cbafcpl, cbafnid)
    if (y >= 42 && y < 72) { fam::AFR.naiss = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbafnai", fam::AFR.naiss); }
    else if (y >= 84 && y < 114) { fam::AFR.nuit = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbafnui", fam::AFR.nuit); }
    else if (y >= 126 && y < 156) { fam::AFR.couple = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbafcpl", fam::AFR.couple); fam::calculerFoyers(); }
    else if (y >= 168 && y < 198) { fam::AFR.nidv = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbafnid", fam::AFR.nidv); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 8) {      // caméra automatique (clé cbcam)
    if (y >= 42 && y < 72) { OP.cauto = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbcam", OP.cauto); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 7) {      // fluidité (clés cbfps, cbcad, cbdeg, cbecr)
    if (y >= 42 && y < 72) return;      // (plus de réglage : le compteur n'est jamais affiché)
    else if (y >= 84 && y < 114) { OP.cad = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbcad", OP.cad); }
    else if (y >= 126 && y < 156) { OP.deg = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbdeg", OP.deg); qDeg = qMin = false; }
    else if (y >= 168 && y < 198) {
      const int i = constrain((x - 4) * 3 / 312, 0, 2);
      if (i == 0) { vitesseEcran(false); OP.ecr = 0; prefs.putUChar("cbecr", 0); ecrEssaiFin = 0; }
      else if (i == 1) { vitesseEcran(true); if (!OP.ecr) ecrEssaiFin = millis() + 10000UL; }      // essai : retour automatique à 40 MHz dans 10 s si rien n'est gardé (écran illisible = rien à faire)
      else if (ecr80 && ecrEssaiFin) { OP.ecr = 1; prefs.putUChar("cbecr", 1); ecrEssaiFin = 0; }
      else return;
    }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 6) {      // main et vue (clés cbpaume, cbpvis, cbrecul)
    if (y >= 38 && y < 64) { OP.paume = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbpaume", OP.paume); }
    else if (y >= 72 && y < 98) { OP.pvis = (uint8_t)constrain((x - 4) * 4 / 312, 0, 3); prefs.putUChar("cbpvis", OP.pvis); }
    else if (y >= 106 && y < 132) { if (x < 106) { OP.tend = (((x - 4) * 2 / 100) >= 1) ? 1 : 0; prefs.putUChar("cbtend", OP.tend); } else { OP.calin = (uint8_t)constrain((x - 108) * 3 / 208, 0, 2); prefs.putUChar("cbcalin", OP.calin); } }
    else if (y >= 140 && y < 166) { mo::reglerRecul((uint8_t)constrain((x - 4) * 6 / 312, 0, 5)); }
    else if (y >= 174 && y < 200) {      // 3.1.44 : interface des boutons : nouvelle (4 modes, pilules) ou ancienne (5 modes, boutons d'avant) ; le fond est refait a la fermeture du panneau
      const uint8_t nu = (x >= 160) ? 1 : 0;
      if (nu != OP.ui) { if (appelOn) appelFin(); OP.ui = nu; prefs.putUChar("cbui", OP.ui); choixL = 0; appelSub = 0; if (nu && modeCb == 4) { modeCb = 1; if (!H.actif) tendreMain(200.0f, 140.0f); } ambChange = true; }
    }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 5) {      // famille (clés cbfam, cbfaid, cbfpro, cbfvit)
    if (y >= 42 && y < 72) { fam::RG.on = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbfam", fam::RG.on); }
    else if (y >= 84 && y < 114) { fam::RG.aide = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbfaid", fam::RG.aide); }
    else if (y >= 126 && y < 156) { fam::RG.prot = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbfpro", fam::RG.prot); }
    else if (y >= 168 && y < 198) { fam::RG.vit = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbfvit", fam::RG.vit); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 4) {      // fonctions : barre de metal, fiche, nourriture, taille (cles cbbarre, cbfiche, cblance, cbtaille) ; 5e rangee : faire pondre un oeuf (3.1.40, aucune cle)
    const int r4 = (y - 28) / 35, o4 = (y - 28) % 35;
    if (y < 28 || y >= 203 || o4 < 11 || r4 > 4) return;
    if (r4 == 0) { OP.barre = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbbarre", OP.barre); }
    else if (r4 == 1) { OP.fiche = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbfiche", OP.fiche); }
    else if (r4 == 2) { OP.lance = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cblance", OP.lance); }
    else if (r4 == 3) { float av = tailleF(); OP.taille = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbtaille", OP.taille); appliquerTaille(av); }
    else {      // faire pondre un oeuf tout de suite : le panneau se ferme et l'oeuf apparait (comme l'ancienne pastille du haut)
      if (OE.actif || OE.ponte >= 0) { oeufMsg = 1; oeufMsgT = millis() + 4000; }
      else if (V.ph != V_RIEN || GP.ph) { oeufMsg = 2; oeufMsgT = millis() + 4000; }
      else { oeufMsg = 0; if (OP.vib) vibrer(40); fermerPanneau(); oeufMaintenant(); return; }
    }
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 3) {      // monde : décor et échanges lointains (clés cbmtour, cblng)
    if (y >= 38 && y < 58) { mo::RGM.tour = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cbmtour", mo::RGM.tour); mo::decorChange(); ambChange = true; }
    else if (y >= 66 && y < 86) { mo::RGM.lng = (((x - 4) * 2 / 312) >= 1) ? 1 : 0; prefs.putUChar("cblng", mo::RGM.lng); }
    else if (y >= 94 && y < 114) { OP.dec = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("cbdec", OP.dec); ambChange = true; }      // le decor est refait a la fermeture du panneau
    else if (y >= 122 && y < 142) { int v = (x - 4) * 3 / 312; if (v < 0) v = 0; if (v > 2) v = 2; mo::RGM.bet = (uint8_t)v; prefs.putUChar("cbbet", mo::RGM.bet); mo::majNbBetes(); }
    else if (y >= 150 && y < 170) { int v = (x - 4) * 3 / 312; if (v < 0) v = 0; if (v > 2) v = 2; mo::RGM.tres = (uint8_t)v; prefs.putUChar("cbtres", mo::RGM.tres); }
    else if (y >= 178 && y < 198) { int v = (x - 4) * 3 / 312; if (v < 0) v = 0; if (v > 2) v = 2; mo::RGM.exc = (uint8_t)v; prefs.putUChar("cbexc", mo::RGM.exc); }      // excursions lointaines : rares / normales / frequentes      // tresors : non / rares / frequents
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 2) {      // image : lumière, qualité, mesures
    if (y >= 42 && y < 72) { int i = constrain((x - 4) * 3 / 312, 0, 2); OP.hor = (uint8_t)i; prefs.putUChar("cbniv", OP.hor); appliquerAmb(); ambChange = true; }
    else if (y >= 84 && y < 114) { int i = constrain((x - 4) * 3 / 312, 0, 2); OP.q = (uint8_t)i; prefs.putUChar("cbqual", OP.q); qDeg = qMin = false; }
    else if (y >= 126 && y < 156) { reposNiv = (uint8_t)constrain((x - 4) * 3 / 312, 0, 2); prefs.putUChar("repos", reposNiv); }
    else if (y >= 168 && y < 198) { presenceOn = ((x - 4) * 2 / 312) >= 1; prefs.putBool("pres", presenceOn); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (panPage == 1) {      // fréquence : barres tactiles
    if (y >= 42 && y < 72) { int i = constrain((x - 4) * 5 / 312, 0, 4); OP.act = (uint8_t)(i + 1); prefs.putUChar("cbact", OP.act); appliquerAct(); }
    else if (y >= 84 && y < 114) { int i = constrain((x - 4) * 4 / 312, 0, 3); OP.gp = (uint8_t)i; prefs.putUChar("cbgp", OP.gp); gVisGP = rnd(30.0f, 60.0f) * multFreq(OP.gp); }
    else if (y >= 126 && y < 156) { int i = constrain((x - 4) * 3 / 312, 0, 2); OP.vis = (uint8_t)i; prefs.putUChar("cbvis", OP.vis); gVisite = delaiVisite(true); }
    else if (y >= 168 && y < 198) { int i = constrain((x - 4) * 4 / 312, 0, 3); OP.oe = (uint8_t)i; prefs.putUChar("cboefq", OP.oe); gOeuf = rnd(40.0f, 70.0f) * multFreq(OP.oe); }
    else return;
    if (OP.vib) vibrer(40);
    panSale = true; return;
  }
  if (y >= 30 && y < 62) {
    if (x >= 40 && x < 90) { R.volume = constrain((int)R.volume - 24, 0, 255); M5.Speaker.setVolume(R.volume); sauverReglages(); panSale = true; sonCri(); }
    else if (x >= 238) { testerSon(); }
    else if (x >= 90 && x < 186) { R.volume = (uint8_t)constrain((x - 92) * 255 / 91, 0, 255); if (x >= 178) R.volume = 255; M5.Speaker.setVolume(R.volume); sauverReglages(); panSale = true; sonCri(); }      // toucher la barre : volume direct, jusqu'au maximum
    else if (x >= 186) { R.volume = constrain((int)R.volume + 24, 0, 255); M5.Speaker.setVolume(R.volume); sauverReglages(); panSale = true; sonCri(); }
    return;
  }
  if (y < 65 || y >= 202) return;
  int r = (y - 65) / 35, c = (x < 160) ? 0 : 1; if (y - 65 - r * 35 > 32) return;
  int i = r * 2 + c;
  switch (i) {
    case 0: if (deuxJeux()) { sonsReels = !sonsReels; prefs.putUChar("cbsons", sonsReels ? 1 : 0); } sonCri(); break;
    case 1: nbMax = (uint8_t)(nbMax >= 8 ? 1 : nbMax + 1); OP.nb = nbMax; prefs.putUChar("cbnb", nbMax); prefs.putUChar("cbeff", nbMax); appliquerNb(); break;
    case 2: OP.sec = (uint8_t)((OP.sec + 1) % 3); prefs.putUChar("cbsec", OP.sec); appliquerSec(); if (OP.vib) vibrer(70); break;
    case 3: OP.vib = OP.vib ? 0 : 1; prefs.putUChar("cbvib", OP.vib); if (OP.vib) vibrer(90); break;
    case 4: OP.amb = (uint8_t)((OP.amb + 1) % 3); prefs.putUChar("cbamb", OP.amb); appliquerAmb(); ambChange = true; break;
    case 5: OP.hor = (uint8_t)((OP.hor + 1) % 3); prefs.putUChar("cbniv", OP.hor); appliquerAmb(); ambChange = true; break;
    case 6: panPage = 12; break;      // 3.1.40 : fiche de la colonie ;      // (compteur d'images/s : jamais affiché)
    default: btnVis = !btnVis; prefs.putUChar("cbbvis", btnVis ? 1 : 0); ambChange = true; break;      // 3.1.37 : affiche ou masque le bouton Visiteur du haut de l'écran (le fond est refait à la fermeture)
  }
  panSale = true;
}

// ---------- vue large (dézoom) : le monde en entier, avec plus d'espace autour ----------
static void syncUI() {      // les bandeaux de boutons (haut et bas) sont identiques dans les deux fonds (et dans le paysage en cache de la caméra)
  if (camCache && cOk && fondOk) majBordsCam(*camC);
  if (!fondLOk || !fondOk) return;
  memcpy(fondL.getBuffer(), fond.getBuffer(), (size_t)24 * 320 * bpp);
  memcpy((uint8_t*)fondL.getBuffer() + (size_t)206 * 320 * bpp, (const uint8_t*)fond.getBuffer() + (size_t)206 * 320 * bpp, (size_t)34 * 320 * bpp);
}
static void cielLarge(M5Canvas& g) {      // ciel, lune, étoiles, collines et sol de la vue large (même monde, échelle réduite)
  for (int sy = 0; sy < 240; sy++) { float r, gg, b; cielRGB((sy - zty) / zs, r, gg, b); g.drawFastHLine(0, sy, 320, CF(r, gg, b)); }
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
  for (int i = 0; i < (ambEt * 2 * (3 - OP.dec)) / 3; i++) {
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sx = 2 + (int)((seed >> 8) % 316);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int sy = 2 + (int)((seed >> 8) % 140);
    if (hypotf(sx - mxs, sy - mys) < (LUNE_R + 24) * zs) continue;
    int v = (int)((seed >> 4) % 100);
    float sk = (OP.hor == 2) ? 0.50f : ((OP.hor == 1) ? 0.75f : 1.0f); g.drawPixel(sx, sy, v < 70 ? CF(150 * sk, 62 * sk, 74 * sk) : CF(205 * sk, 112 * sk, 120 * sk));      // horreur : étoiles rares et ternes
  }
  bkN = 0; brumeD(g, 110, 100, 100, 3, 0.35f); brumeD(g, 210, 128, 110, 3, 0.35f); brumeD(g, 90, 150, 100, 3, 0.35f);
  if (OP.hor) {      // horreur : nuages noirs en lambeaux devant la lune, longues brumes noires basses
    for (int i = 0; i < (OP.hor == 2 ? 4 : 2); i++) {
      float oy = -9.0f + i * 7.0f + 1.5f * sinf(i * 2.1f), ox = 16.0f * sinf(i * 2.3f + 0.6f), rx = (30.0f + 9.0f * ((i * 5) % 3)) * zs, ry = (2.2f + 0.7f * (i % 3)) * zs;
      g.fillEllipse((int)(mxs + ox * zs), (int)(mys + oy * zs), (int)rx, (int)(ry + 0.5f), CF(15 + 4 * (i % 2), 1, 3));
      g.fillEllipse((int)(mxs + (ox + 14.0f) * zs), (int)(mys + (oy + 1.5f) * zs), (int)(rx * 0.5f), (int)(ry * 0.8f + 0.5f), CF(11, 0, 2));
    }
    brumeD(g, 70, 200, 150, 8, 0.85f); brumeD(g, 250, 204, 150, 7, 0.85f); brumeD(g, 160, 198, 120, 6, 0.80f);
  }
  float gy = solHaut(160.0f) * zs + zty;
  seed = 777;      // grain léger dans le ciel : plus de côté « papier lisse »
  for (int i = 0; i < (2600 >> OP.dec); i++) {      // grain du ciel : complet, moitie, quart
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gx = (int)((seed >> 8) % 320);
    seed = (seed * 1103515245UL + 12345UL) & 0x7fffffffUL; int gyy = (int)((seed >> 8) % (uint32_t)((int)gy - 6 > 1 ? (int)gy - 6 : 1));
    if (hypotf(gx - mxs, gyy - mys) < (LUNE_R + 30) * zs) continue;
    float r, gg, b; cielRGB((gyy - zty) / zs, r, gg, b); bool cl = (seed >> 3) & 1;
    g.drawPixel(gx, gyy, CF(r * (cl ? 1.12f : 0.88f) + (cl ? 7 : 0), gg * (cl ? 1.12f : 0.88f), b * (cl ? 1.12f : 0.88f)));
  }
  { static const float LT[10][4] = {{282, 78, 0, 0.30f}, {334, 56, 3, 0.42f}, {238, 46, 2, 0.50f}, {8, 62, 4, 0.45f}, {-62, 72, 1, 0.50f}, {392, 66, 1, 0.52f}, {-30, 44, 3, 0.55f}, {372, 58, 4, 0.50f}, {430, 52, 2, 0.58f}, {-98, 56, 0, 0.58f}};
    for (int i = 0; i < 10; i++) {      // arbres lointains, tous différents de l'arbre du premier plan ; les derniers ne se voient que dans la vue large
      float sx = LT[i][0] * zs + ztx, sy = 197.0f * zs + zty; if (sx < -30 || sx > 350) continue;
      if (!decOk(i)) continue;      // decor allege : un arbre lointain sur trois en moins ; minimal : un sur deux
      arbreLoin(g, (int)LT[i][2], sx, sy, LT[i][1] * zs * 1.3f, LT[i][3]);
    }
  }
  for (int x = 0; x < 320; x++) {
    float yl1 = gy - 24.0f + 6.0f * sinf(x / 40.0f + 1.0f) + 4.0f * sinf(x / 17.0f); g.drawFastVLine(x, (int)yl1, 240 - (int)yl1, CF(66 * ambCol, 9 * ambCol, 15 * ambCol));
    float yl2 = gy - 12.0f + 5.0f * sinf(x / 55.0f + 2.0f) + 3.0f * sinf(x / 23.0f); g.drawFastVLine(x, (int)yl2, 240 - (int)yl2, CF(46 * ambCol, 6 * ambCol, 11 * ambCol));
    float ys = solHaut((x - ztx) / zs) * zs + zty; g.drawFastVLine(x, (int)ys, 240 - (int)ys, C(5, 2, 4));
  }
}
static void construireFondLarge() {
  VueId vid_;
  zs = 0.6f; ztx = zax * (1 - zs); zty = zay * (1 - zs);
  cielLarge(fondL);
  arbre(fondL);                                                       // l'arbre en entier (les arbres lointains, tous différents, sont dans cielLarge)
  vignette(fondL);
  syncUI();
}
static bool creerLarge() {
  if (fondLOk) return true;
  if (ESP.getFreePsram() < 400000UL && ESP.getFreeHeap() < 300000UL) return false;
  fondL.setColorDepth(cv.getColorDepth()); fondL.setPsram(true);
  if (!fondL.createSprite(320, 240)) return false;
  fondLOk = true; construireFondLarge(); return true;
}
static int ordreOctets = -1;      // 1 : octet fort en premier, 0 : inversé (détecté par un essai)
static inline uint16_t mix565(uint16_t a, uint16_t b, uint32_t al) {      // al : poids de b, de 0 à 32
  uint32_t A = (a | ((uint32_t)a << 16)) & 0x07E0F81FUL, B = (b | ((uint32_t)b << 16)) & 0x07E0F81FUL;
  uint32_t R = ((((B - A) * al) >> 5) + A) & 0x07E0F81FUL; return (uint16_t)(R | (R >> 16));
}
static void composerTransition() {      // pendant le zoom : fondu rapide entre la vue normale et la vue large (au lieu d'un zoom point par point), la taille des corbeaux suit le zoom
  const float p = pas(zoomU); const uint32_t al = (uint32_t)(p * 32.0f + 0.5f);
  if (ordreOctets < 0) { cv.drawPixel(0, 0, C(248, 0, 0)); ordreOctets = (((const uint8_t*)cv.getBuffer())[0] == 0xF8) ? 1 : 0; cv.drawPixel(0, 0, C(0, 0, 0)); }
  if (bpp != 2 || !fondLOk || al == 0 || al >= 32) { memcpy(cv.getBuffer(), (al >= 16 && fondLOk ? fondL : fond).getBuffer(), (size_t)320 * 240 * bpp); return; }
  const uint8_t* a = (const uint8_t*)fond.getBuffer(); const uint8_t* b = (const uint8_t*)fondL.getBuffer(); uint8_t* d = (uint8_t*)cv.getBuffer();
  memcpy(d, a, (size_t)24 * 640);
  const size_t o0 = (size_t)24 * 640, n = (size_t)(H_SCENE - 24) * 320;
  for (size_t i = 0; i < n; i++) {
    const uint8_t* pa = a + o0 + i * 2; const uint8_t* pb = b + o0 + i * 2; uint8_t* pd = d + o0 + i * 2;
    uint16_t va = ordreOctets ? (uint16_t)((pa[0] << 8) | pa[1]) : (uint16_t)((pa[1] << 8) | pa[0]), vb = ordreOctets ? (uint16_t)((pb[0] << 8) | pb[1]) : (uint16_t)((pb[1] << 8) | pb[0]);
    uint16_t v = (va == vb) ? va : mix565(va, vb, al);
    if (ordreOctets) { pd[0] = (uint8_t)(v >> 8); pd[1] = (uint8_t)(v & 255); } else { pd[1] = (uint8_t)(v >> 8); pd[0] = (uint8_t)(v & 255); }
  }
  memcpy(d + (size_t)H_SCENE * 640, a + (size_t)H_SCENE * 640, (size_t)(240 - H_SCENE) * 640);
}
static bool pinceOn = false, pinceFait = false; static float pinceD0 = 0;
static void pincer() {      // pincement à deux doigts : se pincer = dézoom, s'écarter = zoom
  if (M5.Touch.getCount() >= 2) {
    auto a = M5.Touch.getDetail(0), b = M5.Touch.getDetail(1); float d = hypotf((float)(a.x - b.x), (float)(a.y - b.y));
    if (!pinceOn) { pinceOn = true; pinceFait = false; pinceD0 = d; D.bas = false; VS.on = false; if (appelOn) appelFin(); }
    else if (!pinceFait && pinceD0 > 24.0f) {
      if (d < pinceD0 * 0.72f) { mo::reculer(+1); pinceFait = true; }
      else if (d > pinceD0 * 1.35f) { mo::reculer(-1); pinceFait = true; }
    }
  } else pinceOn = false;
}

// ---------- ouverture / fermeture ----------
// ---------- noms et fiches : chaque corbeau a un nom (mémorisé) ; un double toucher affiche sa fiche ----------
static char nomC[NCB][9];
static const char* const NOMS[24] = {"Ombrin", "Kraal", "Nyxel", "Skarn", "Velrik", "Morvane", "Cendre", "Brisk", "Tarvok", "Ylmar", "Nocte", "Gravel", "Ruska", "Doral", "Vesper", "Hexel", "Korvan", "Dusko", "Mirsa", "Talvek", "Orsel", "Brann", "Zorel", "Pelka"};
static void chargerNoms() {      // noms originaux de 5 à 8 lettres, tous différents ; tirés une fois puis relus de la mémoire (clé cbnoms)
  bool ok = (prefs.getBytesLength("cbnoms") == sizeof(nomC));
  if (ok) { prefs.getBytes("cbnoms", nomC, sizeof(nomC)); for (int k = 0; k < NCB; k++) { nomC[k][8] = 0; if (!nomC[k][0]) ok = false; } }
  if (!ok) {
    bool pris[24]; for (int i = 0; i < 24; i++) pris[i] = false;
    for (int k = 0; k < NCB; k++) { int i; do { i = random(24); } while (pris[i]); pris[i] = true; memset(nomC[k], 0, 9); strncpy(nomC[k], NOMS[i], 8); }
    prefs.putBytes("cbnoms", nomC, sizeof(nomC));
  }
}
struct FicheT { int8_t k; uint32_t t; };
static FicheT FI = {-1, 0};
static int8_t tapK = -1; static uint32_t tapT = 0;      // dernier toucher sur un corbeau (pour reconnaître le double toucher)
static bool pendOn = false; static int8_t pendK = -1; static float pendX = 0, pendY = 0; static uint32_t pendT = 0;      // réaction au toucher simple, retardée de 0,28 s pour laisser passer un double toucher
static void reactionRetardee() { if (cibleK >= 0 && millis() - cibleT > 20000) cibleK = -1; if (pendOn && millis() - pendT >= 280) { pendOn = false; if (pendK >= 0 && pendK < NCB && cr[pendK].visible) reagirToucher(cr[pendK], pendX, pendY); } }
static int fbx = 0, fby = 0, fbh = 80, fbb = 60; static const int FBW = 150, FBH = 80;      // position de la fiche affichée (pour les boutons)
static void dessinerFiche(M5Canvas& g) {
  if (FI.k >= 0 && (millis() - FI.t > 6000 || !cr[FI.k].visible)) FI.k = -1;
  if (infoT && millis() - infoT > infoDur) infoT = 0;
  if (infoT) {      // 3.1.38 : bulle d'information au-dessus de la barre du bas (nom et effet d'une nourriture, messages)
    const int iw = 292, ih = 34, ix = 14, iy = H_SCENE - ih - 3;
    bbPt((float)ix, (float)iy); bbPt((float)(ix + iw), (float)(iy + ih));
    if (!sec) { g.fillRoundRect(ix, iy, iw, ih, 8, C(10, 2, 6)); g.drawRoundRect(ix, iy, iw, ih, 8, C(170, 34, 40)); g.setTextDatum(top_left); g.setFont(&fonts::lgfxJapanGothic_16); g.setTextColor(C(255, 236, 225)); g.drawString(infoL1, ix + 8, iy + 2); g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(230, 170, 160)); g.drawString(infoL2, ix + 8, iy + 19); }
  }
  if (cibleK >= 0 && !cr[cibleK].visible) cibleK = -1;
  if (cibleK >= 0 && cibleK != FI.k) {      // pastille : « → Nom » au-dessus du corbeau désigné
    Corbeau& c = cr[cibleK]; float px = TX(c.x), py = TY(c.y + c.bob); int w = 60, x0 = (int)lim(px - w * 0.5f, 2.0f, 318.0f - w), y0 = (int)lim(py - 30.0f * c.s * zs - 20.0f, 28.0f, 180.0f);
    bbPt((float)x0, (float)y0); bbPt((float)(x0 + w), (float)(y0 + 16));
    if (!sec) { g.fillRoundRect(x0, y0, w, 16, 6, C(60, 10, 14)); g.drawRoundRect(x0, y0, w, 16, 6, C(255, 150, 120)); g.setFont(&fonts::lgfxJapanGothic_12); g.setTextDatum(middle_center); g.setTextColor(C(255, 236, 225)); char b[16]; snprintf(b, sizeof(b), ">%s", nomC[cibleK]); g.drawString(b, x0 + w / 2, y0 + 8); g.setTextDatum(top_left); }
  }
  if (FI.k < 0) return;
  const int k = FI.k; Corbeau& c = cr[k];
  static const char* const TRT[5] = {"Audacieux", "Calme", "Curieux", "Sociable", "Costaud"};
  char l1[12], l2[32], l3[40], l4[48]; snprintf(l1, sizeof(l1), "%s", nomC[k]);
  const float en_ = (k < NAD) ? mo::en[k] : 0.5f, tj_ = (k < NAD) ? mo::tJoie[k] : 0.0f, th_ = (k < NAD) ? mo::tHer[k] : 0.0f;      // le petit (emplacement 8) n'a pas de jauges
  int pct = (int)(lim(en_, 0.0f, 1.0f) * 100.0f + 0.5f);
  const char* hum = (tj_ > 0.0f) ? "Joyeux" : ((th_ > 0.0f) ? "Gonflé" : (en_ > 0.85f ? "Rassasié" : (en_ < 0.15f ? "Affamé" : "Calme")));
  snprintf(l3, sizeof(l3), "%s, énergie %d %%", hum, pct);
  if (fam::pret && k < fam::NM) {
    int m = 0, best = 0; const uint8_t v[5] = {fam::S.aud[k], fam::S.cal[k], fam::S.cur[k], fam::S.soc[k], fam::S.frc[k]}; for (int i = 1; i < 5; i++) if (v[i] > v[m]) m = i;
    { static const char* const GOUT[6] = {"graines", "viande", "noix", "pain", "baies", ""}; const int fv_ = mo::favori(k); snprintf(l2, sizeof(l2), "%s, aime %s", TRT[m], (fv_ >= 0 && fv_ < 5) ? GOUT[fv_] : "?"); }
    int j2 = -1; for (int j = 0; j < fam::NM; j++) if (j != k) { int a = fam::lien(k, j); if (j2 < 0 || abs(a) > abs(best)) { j2 = j; best = a; } }
    const char* lk = "sans lien fort";
    if (j2 >= 0 && best >= 80) { lk = "couple avec"; } else if (j2 >= 0 && best >= 40) { lk = "ami de"; } else if (j2 >= 0 && best <= -40) { lk = "rival de"; }
    if (j2 >= 0 && (best >= 40 || best <= -40)) snprintf(l4, sizeof(l4), "Rang %d, %s %s", (int)fam::S.rang[k] + 1, lk, nomC[j2]); else snprintf(l4, sizeof(l4), "Rang %d, %s", (int)fam::S.rang[k] + 1, lk);
  } else { snprintf(l2, sizeof(l2), "Trait : ?"); snprintf(l4, sizeof(l4), "Colonie en formation"); }
  char l5[40]; l5[0] = 0; fam::texteLien(k, l5, sizeof(l5), nomC);
  char l6[40]; l6[0] = 0; nat::texteSante(k, l6, sizeof(l6));
  fbh = FBH + 4 + (l5[0] ? 12 : 0) + (l6[0] ? 12 : 0); fbb = fbh - 20;
  float px = TX(c.x), py = TY(c.y + c.bob);
  int x0 = (int)lim(px - FBW * 0.5f, 2.0f, 318.0f - FBW), y0 = (int)lim(py - 30.0f * c.s * zs - fbh - 2.0f, 28.0f, (float)(H_SCENE - fbh - 2));
  fbx = x0; fby = y0;
  bbPt((float)x0, (float)y0); bbPt((float)(x0 + FBW), (float)(y0 + fbh)); if (sec) return;
  g.fillRoundRect(x0, y0, FBW, fbh, 6, C(10, 2, 6)); g.drawRoundRect(x0, y0, FBW, fbh, 6, C(170, 34, 40));
  g.setTextDatum(top_left); g.setFont(&fonts::lgfxJapanGothic_16); g.setTextColor(C(255, 236, 225)); g.drawString(l1, x0 + 6, y0 + 3);
  g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(230, 170, 160)); g.drawString(l2, x0 + 6, y0 + 22); g.drawString(l3, x0 + 6, y0 + 34); g.drawString(l4, x0 + 6, y0 + 50);
  if (k < NAD) { const int gw = FBW - 12, gf = (int)(lim(en_, 0.0f, 1.0f) * gw + 0.5f); g.fillRect(x0 + 6, y0 + 47, gw, 3, C(44, 12, 16)); g.fillRect(x0 + 6, y0 + 47, gf, 3, pct < 25 ? C(230, 60, 50) : (pct < 60 ? C(230, 150, 60) : C(120, 200, 110))); }      // 3.1.38 : jauge de faim / d'énergie
  if (l5[0]) { g.setTextColor(C(255, 205, 130)); g.drawString(l5, x0 + 6, y0 + 62); }
  if (l6[0]) { g.setTextColor(C(255, 170, 150)); g.drawString(l6, x0 + 6, y0 + 62 + (l5[0] ? 12 : 0)); }
  g.fillRoundRect(x0 + 6, y0 + fbb, 66, 16, 6, C(50, 100, 60)); g.fillRoundRect(x0 + 78, y0 + fbb, 66, 16, 6, C(100, 50, 40));      // boutons : Lui donner / Lui lancer
  g.setTextDatum(middle_center); g.setTextColor(C(255, 250, 240)); g.drawString("Lui donner", x0 + 39, y0 + fbb + 8); g.drawString("Lui lancer", x0 + 111, y0 + fbb + 8); g.setTextDatum(top_left);
}
// ---------- ouverture et fermeture PROGRESSIVES (3.1.32) ----------
// Avant : ouvrir() faisait tout dans UNE image (decor, paysage de 411 Ko cuit en PSRAM, reglages, colonie, haut-parleur) : plus d'une demi-seconde (estime) d'ecran fige quand on touche « Corbeaux ».
// Maintenant : un ecran de chargement leger s'affiche tout de suite, puis chaque etape (une par tour de boucle) fait un morceau ; le fond et le paysage gardes en memoire a la fermeture sont reutilises
// quand les reglages visuels n'ont pas change (reouverture bien plus rapide). La fermeture est faite APRES l'affichage de l'ecran suivant, en quatre etapes.
static const int LD_FIN0 = 14, LD_FIN1 = 11;      // nombre d'etapes : ouverture complete / reconstruction du decor
static inline uint32_t hmS(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t sigFond() {      // tout ce dont depend le fond (ciel, arbre, decor, vignettage)
  uint32_t h = 2166136261u;
  h = hmS(h, OP.hor); h = hmS(h, OP.amb); h = hmS(h, OP.dec); h = hmS(h, (uint32_t)mo::RGM.tour); h = hmS(h, (uint32_t)(int)(ambVig * 1000.0f)); h = hmS(h, (uint32_t)ambEt); h = hmS(h, (uint32_t)bpp); h = hmS(h, btnVis ? 1u : 0u); h = hmS(h, (uint32_t)OP.ui);
  return h;
}
static void chargerEcran(bool premier) {      // ecran de chargement : seule la barre est renvoyee a l'ecran apres la premiere image
  const int fin = ldMode ? LD_FIN1 : LD_FIN0; const int w = (ldPas <= 0) ? 0 : (ldPas >= fin ? 194 : 194 * ldPas / fin);
  M5Canvas& g = cv;
  if (premier) {
    g.fillScreen(C(10, 0, 4)); g.setFont(&fonts::lgfxJapanGothic_16); g.setTextDatum(middle_center); g.setTextColor(C(255, 150, 140));
    g.drawString(ldMode ? "Mise à jour du décor" : "Les corbeaux", 160, 98);
    g.setFont(&fonts::lgfxJapanGothic_12); g.setTextColor(C(170, 100, 100)); g.drawString("Chargement...", 160, 160); g.setTextDatum(top_left);
    g.drawRoundRect(60, 120, 200, 14, 7, C(110, 24, 30));
  }
  if (w > 0) g.fillRoundRect(63, 123, w, 8, 4, C(214, 60, 52));
  M5.Display.startWrite();
  if (premier) g.pushSprite(0, 0); else { M5.Display.setClipRect(56, 116, 208, 24); g.pushSprite(0, 0); M5.Display.clearClipRect(); }
  M5.Display.endWrite();
}
static void chargerPas() {
  if (!ldAff) { ldAff = true; chargerEcran(true); return; }      // premiere image : l'ecran de chargement, rien d'autre
  const int fin = ldMode ? LD_FIN1 : LD_FIN0;
  switch (ldPas) {
    case 0: {
      if (ldMode == 0) {
        plein = true; chargerSons(); chargerOeil(); chargerOptions();
        bpp = cv.getColorDepth() / 8; if (bpp < 1) bpp = 1;
        if (!fondGarde && ESP.getFreePsram() < 300000UL && ESP.getFreeHeap() < 260000UL) { ldPas = -1; return; }      // fondOk reste faux : message « pas assez de memoire »
        if (!fondGarde) { if (fond.getBuffer()) fond.deleteSprite(); fond.setColorDepth(cv.getColorDepth()); fond.setPsram(true); if (!fond.createSprite(320, 240)) { ldPas = -1; return; } }
        if (!cmdB) { cmdB = (Cmd*)ps_malloc(sizeof(Cmd) * CMDMAX); cmdMax = cmdB ? CMDMAX : 0; }      // 64 Ko en PSRAM ; sans lui le dessin recalcule tout comme avant
        zs = 1.0f; ztx = 0.0f; zty = 0.0f; fondLOk = false; fondAct = &fond;
      }
      mo::chargerMon();
      ldFondSaut = fondGarde && fondSig == sigFond();      // le fond garde est encore le bon : ciel, arbre, decor et vignettage sautes
    } break;
    case 1: { if (!ldFondSaut) { fondGarde = false; VueId v_; zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); cielLarge(fond); } } break;      // le fond va etre redessine : l'ancien n'est plus garde (si on part en cours de route, il est rendu)
    case 2: { if (!ldFondSaut) { VueId v_; zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); arbre(fond); } } break;
    case 3: { if (!ldFondSaut) { VueId v_; zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); mo::decorNormal(fond); vignette(fond); } } break;
    case 4: mo::bakePas(0); break;
    case 5: mo::bakePas(1); break;
    case 6: mo::bakePas(2); break;
    case 7: mo::bakePas(3); break;
    case 8: mo::bakePas(4); break;
    case 9: mo::bakePas(5); break;
    case 10: {
      menuCb(fond); boutonSons(fond); boutonVisiteur(fond); boutonVue(fond); barreCb(fond, choix, -1);
      cOk = false; nEnCours = false; fondSig = sigFond(); fondGarde = true;      // le paysage en cache de la camera n'est plus valable
      if (ldMode) { if (fondLOk) construireFondLarge(); }
      else fondOk = true;
    } break;
    case 11: {
  memset(cr, 0, sizeof(cr)); memset(ob, 0, sizeof(ob)); memset(pl, 0, sizeof(pl)); memset(fx, 0, sizeof(fx)); memset(anc, 0, sizeof(anc)); memset(dernierSon, 0, sizeof(dernierSon));
  static const float tailles[NCB] = {1.42f, 1.26f, 1.19f, 1.34f, 1.23f, 1.30f, 1.28f, 1.21f, 0.62f};      // adulte posé ≈ 1/6 de la hauteur de l'arbre (~31 unités pour ~185) ; le petit est plus petit
  for (int k = 0; k < NCB; k++) initCorbeau(cr[k], tailles[k] * tailleF());
  cr[0].d = -1; cr[1].d = 1; cr[2].d = 1; placer(cr[0], 3); placer(cr[1], 1); placer(cr[2], 4);
  for (int k = 0; k < 3; k++) cr[k].tfin = rnd(3, 8);
  nat::init();      // nature.h : sante, cicatrices, steles, reglages (avant fam::init pour que les rangs tiennent compte des disparus)
  fam::init();      // famille.h : la colonie (traits, rôles, rangs, perchoirs attitrés) est relue de la mémoire
  chargerNoms(); FI.k = -1; tapK = -1; pendOn = false; visType = -1;
  gCalme = 0; memset(&V, 0, sizeof(V)); memset(mini, 0, sizeof(mini)); memset(proj, 0, sizeof(proj)); memset(&H, 0, sizeof(H)); memset(BRAIS, 0, sizeof(BRAIS)); tBraise = 0.0f; memset(&VS, 0, sizeof(VS)); memset(&FR, 0, sizeof(FR)); memset(&D, 0, sizeof(D));
  appelOn = false; modeCb = 0; choixL = 0; appelSub = 0; tpCible = 0; gVisite = delaiVisite(true); tAnim = millis() * 0.001f;
  for (int k = 3; k < NAD; k++) { cr[k].etat = E_DEHORS; cr[k].visible = false; cr[k].t = 0; cr[k].tfin = (k == 3) ? 1.2f : 3.0f + 3.0f * (k - 3); }
  cr[PC].etat = E_OFF; cr[PC].visible = false;
    } break;
    case 12: {
  zoomC = 0.0f; zoomU = 0.0f; pinceOn = false; mo::reinit(); fam::apresMonde();
  if (zoomC > 0.5f) { if (creerLarge()) { zs = 0.6f; ztx = zax * (1 - zs); zty = zay * (1 - zs); fondAct = &fondL; } else { zoomC = zoomU = 0.0f; } }
  if (zoomC < 0.5f) { zs = ZN; ztx = zax * (1 - zs); zty = zay * (1 - zs); }
  memset(&GP, 0, sizeof(GP)); gVisGP = rnd(50.0f, 90.0f) * multFreq(OP.gp); tToucher = millis(); gEcl = rnd(30.0f, 70.0f); gBatt = rnd(35.0f, 80.0f); eclT = 0.0f; battT = 0.0f;
  memset(&OE, 0, sizeof(OE)); OE.ponte = -1; gOeuf = rnd(20.0f, 45.0f) * multFreq(OP.oe); agePous = 0; panneau = false; panSale = true; ambChange = false; oeufsSale = false; appliquerNb(); fam::restaurerY();
  gAction = 3.0f; gPoof = rnd(14, 24); gObjet = rnd(24, 40); gAttrib = 0; choix = 0; presse = -1; stK[0] = STMAX[0]; stK[1] = STMAX[1]; stSale = false; infoT = 0; barI = -1; barInfo = false; tDernier = millis(); tSec = millis(); nImg = 0; fps = 0; msDessin = 0; volVu = false;
  fpsHN = 0; fpsMoy10 = 0; fpsMin = 0; msMax = 0.0f; maxIntervalle = 0; tPrecImg = 0; qDeg = qMin = false; tQ = tBon = millis();      // compteur d'images/s et allègement : on repart de zéro
  vitesseEcran(OP.ecr != 0);      // écran rapide (80 MHz) seulement s'il a été essayé puis gardé dans Réglages > Fluidité
  if (prefs.getUChar("cbvol1", 0) == 0) { if (R.volume < 200) { R.volume = 200; sauverReglages(); } prefs.putUChar("cbvol1", 1); }      // volume de départ plus fort, une seule fois
      barreCb(fond, choix, -1);      // la barre du fond montre le mode de depart (nourrir)
    } break;
    case 13: {
  if (!lectureEnCours && !M5.Speaker.isRunning()) { M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume); }
  Serial.printf("[corbeau] ouvert : heap=%u psram=%u\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
    } break;
    default: break;
  }
  ldPas++;
  while (ldPas < fin && ((ldPas >= 1 && ldPas <= 3 && ldFondSaut) || (ldPas >= 5 && ldPas <= 9 && mo::bakeSaut))) ldPas++;      // etapes inutiles : sautees dans le meme tour
  if (ldPas >= fin) { ldPas = -1; plein = true; tDernier = millis(); tSec = millis(); tPrecImg = 0; tToucher = millis(); return; }
  chargerEcran(false);
}
static void quitterDebut() { if (!ouvert || qPas >= 0) return; ldPas = -1; qPas = 0; }      // la fermeture se fait ensuite par etapes (quitterPas), apres l'affichage de l'ecran suivant
static void quitterPas() {
  switch (qPas) {
    case 0: {
      if (nbOeufsSale) { prefs.putUInt("cboeufs", nbOeufs); nbOeufsSale = false; }
      fam::sauver(true);      // ecritures en memoire de la colonie : une etape a elles seules
    } break;
    case 1: {
      libererCamCache(); if (cmdB) { free(cmdB); cmdB = nullptr; cmdMax = 0; } nat::ferme(); fam::pret = false;
      qGarder = (fondGarde && mo::gardeMonde() && ESP.getFreePsram() >= 600000UL);      // le fond et le paysage restent en memoire (reouverture rapide) s'il reste assez de PSRAM
      mo::fermerMonde(qGarder); ouvert = false; seuilSecousse = 2.2f; zs = 1.0f; ztx = 0.0f; zty = 0.0f; fondAct = &fond;
    } break;
    case 2: {
      vitesseEcran(false); ecrEssaiFin = 0;      // les autres pages gardent la vitesse d'écran d'usine (40 MHz)
      if (fondLOk) { fondL.deleteSprite(); fondLOk = false; }
      if (!qGarder) { if (fond.getBuffer()) fond.deleteSprite(); fondGarde = false; }
      fondOk = false; tQuit = millis();
    } break;
    default: {
      if (!lectureEnCours && M5.Speaker.isRunning()) { M5.Speaker.stop(); M5.Speaker.end(); M5.Mic.begin(); }     // la musique en cours (Matinale) n'est jamais touchée
      Serial.printf("[corbeau] fermé (garde=%d) : heap=%u psram=%u\n", (int)qGarder, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
    } break;
  }
  if (++qPas > 3) qPas = -1;
}
static void quitter() { quitterDebut(); while (qPas >= 0) quitterPas(); }
static void libererCache() {      // rend la memoire du fond et du paysage gardes
  if (ouvert || qPas >= 0 || ldPas >= 0) return;
  if (fond.getBuffer()) fond.deleteSprite();
  fondGarde = false; mo::libererSprites(); mo::tentee = false;
}
static void surveillerCache(bool force) {      // appele toutes les 2 s hors du mode Corbeaux : memoire rare, mise a jour en cours ou plus de 15 min sans revenir = on rend la memoire
  if (ouvert || qPas >= 0 || ldPas >= 0) return;
  if (!fondGarde && !mo::gardeMonde()) return;
  if (force || ESP.getFreePsram() < 450000UL || (uint32_t)(millis() - tQuit) > 900000UL) libererCache();
}

// ---------- présence : absence = vie minimale et peu coûteuse ; retour de l'utilisateur = la scène s'éveille, et ce qui s'est passé « hors écran » est rattrapé ----------
static void eveiller(uint32_t secs) {
  int n = 0, k0 = -1;
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || c.perch < 0 || c.perch >= 10 || c.objet) continue;
    if (c.act == A_DORT) { c.act = A_IMMO; c.dAct = 1.2f; c.tAct = 0; }
    c.hum = 1.0f; c.flash = 0.45f; c.look = c.ha0 - 0.3f; c.tLook = 0.9f + 0.15f * n; if (k0 < 0) k0 = k; n++;      // la tête se lève, les yeux s'allument l'un après l'autre
  }
  if (k0 >= 0) { Corbeau& c = cr[k0]; c.bec = 1; sonCri(); plumes(c.x, c.y - 3.0f * c.s, 4); eclat(c.x + c.dv * 8.0f * c.s, c.y - 4.0f * c.s, 0.3f, 0.8f); }
  gAction = 2.5f;
  float s = (float)(secs > 600 ? 600 : secs);      // rattrapage : un œuf a pu être pondu, le petit a grandi, des feuilles sont tombées
  gOeuf -= s * 0.15f; agePous += s * 0.4f; fam::rattrapageY((uint32_t)s);
  if (secs > 60) for (int i = 0; i < 3; i++) feuille();
  ralenti(0.25f);
}
static void presenceMaj() {
  const uint32_t der_ = ((int32_t)(pr::derInter - derniereVraie) > 0) ? pr::derInter : derniereVraie;      // dernier geste, bruit fort ou changement d'orientation
  const bool a = presenceOn && ((uint32_t)(millis() - der_) > pr::delaiAbs) && !GP.ph && V.ph == V_RIEN && !H.actif && !appelOn;
  if (a == cbAbsent) return;
  if (a) { cbAbsent = true; cbDepuis = millis(); appliquerAct(); }
  else { cbAbsent = false; appliquerAct(); eveiller((millis() - cbDepuis) / 1000UL); }
}
// ---------- presence (presence.h) : objets laisses par l'absence ; la scene (yeux, cri, rattrapage) est dans eveiller() ci-dessus ----------
static void arriveeCb() {      // rattrapage : des objets ont ete laisses au sol pendant l'absence
  static const uint8_t TR[5] = {T_BAGUE, T_BRILLANT, T_PARCHEMIN, T_GRAINE, T_KUNAI};
  for (int i = 0; i < pr::nbTraces; i++) {
    int j = -1; for (int q = 0; q < NOB; q++) if (!ob[q].actif) { j = q; break; }
    if (j >= 0) { Obj& o = ob[j]; o.actif = true; o.type = TR[random(5)]; o.x = rnd(40.0f, 280.0f); o.y = solHaut(o.x) - 3.2f; o.vy = 0; o.t = 0; o.repos = true; o.reserve = -1; }
  }
}
static bool occupe() { return GP.ph || V.ph != V_RIEN || H.actif || appelOn; }      // un evenement est en cours : l'utilisateur est la
static void presenceCb() {
  static uint8_t etPrec = pr::PRESENT;
  if (pr::etat == pr::ENTREE && etPrec != pr::ENTREE && pr::vraieArrivee) arriveeCb();
  etPrec = pr::etat;
}
// ---------- allégement automatique : si l'image est trop lente, les détails coûteux sont réduits (discrètement), puis rétablis dès que le temps repasse sous le seuil ----------
static void allegement() {
  if (!OP.deg || OP.q == 2 || cbAbsent) { qDeg = qMin = false; return; }      // Riche ou interrupteur coupé : jamais d'allègement ; en absence tout est déjà minimal
  const uint32_t now = millis(); if (now - tQ < 1200u) return;      // pas de va-et-vient : un changement de niveau au plus toutes les 1,2 s
  const float per = (float)periodeMs(); int n = qMin ? 2 : (qDeg ? 1 : 0);
  if (emaTot > per * 1.25f + 4.0f) { if (n < 2) { n++; tQ = now; } tBon = now; }      // vraiment trop lent : niveau 2
  else if (emaTot > per * 0.92f) { if (n < 1) { n = 1; tQ = now; } tBon = now; }      // en retard : niveau 1
  else if (emaTot < per * 0.62f) { if (n > 0 && now - tBon > 2500u) { n--; tQ = now; tBon = now; } }      // confortable depuis 2,5 s : on rétablit un niveau
  else tBon = now;
  qDeg = n >= 1; qMin = n >= 2;
}
// ---------- une image ----------
static void dessiner() {
  while (qPas >= 0) quitterPas();      // une fermeture differee est encore en cours : on la termine avant de rouvrir
  if (!ouvert) { ouvert = true; fondOk = false; ldMode = 0; ldPas = 0; ldAff = false; }      // ouverture progressive : ecran de chargement tout de suite, puis une etape par tour de boucle
  if (ldPas >= 0) { chargerPas(); return; }
  if (!fondOk) {
    cv.fillScreen(C(10, 0, 4)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 150, 140));
    cv.drawString("Pas assez de mémoire", 160, 110); cv.drawString("pour les corbeaux", 160, 134); cv.setTextDatum(top_left); cv.pushSprite(0, 0); return;
  }
  if (tTestSon && millis() > tTestSon) { tTestSon = 0; M5.Speaker.setVolume(R.volume); }
  if (ecrEssaiFin && (int32_t)(millis() - ecrEssaiFin) > 0) { ecrEssaiFin = 0; if (!OP.ecr) vitesseEcran(false); plein = true; panSale = true; }      // essai de l'écran rapide terminé sans être gardé : retour à la vitesse d'usine
  if (panneau) { if (panSale) { panSale = false; dessinerPanneau(); } return; }
  if (oeufsSale) { oeufsSale = false; }
  uint32_t m0 = millis(), u0 = micros(); uCorb = 0;
  float dt = (m0 - tDernier) / 1000.0f; bool longue = dt > 0.4f; tDernier = m0; if (dt > 0.05f) dt = 0.05f; if (dt < 0.001f) dt = 0.001f;
  if (slowT > 0) { slowT -= dt; dt *= 0.4f; }      // ralenti court sur les moments forts
  if (longue) plein = true;
  pincer();
  if (zoomC > 0.5f && !fondLOk) { if (!creerLarge()) zoomC = 0.0f; }
  bool anim = fabsf(zoomU - zoomC) > 0.004f;
  if (anim) { zoomU += (zoomC - zoomU) * mnf(1.0f, dt * 6.0f); if (fabsf(zoomU - zoomC) <= 0.004f) { zoomU = zoomC; anim = false; } plein = true; }
  else if (zoomU != zoomC) { zoomU = zoomC; plein = true; }
  { float zn = lerpf(ZN, 0.6f, pas(zoomU)); if (zn != zs && !mo::camOn()) { zs = zn; ztx = zax * (1 - zs); zty = zay * (1 - zs); plein = true; } }
  fondAct = (zoomU > 0.5f && fondLOk) ? &fondL : &fond;
  bool camUse = false;      // vrai : le fond de cette image est le paysage en cache de la caméra
  if (mo::camPilote(dt)) {      // monde.h : vue d'ensemble ou suivi d'un oiseau (zs, ztx, zty = ce que la caméra voudrait)
    const bool pleinAvant = plein; fondAct = &fond; plein = true;      // caméra rapide ou pas de cache : le paysage est recomposé à chaque image, comme avant
    if (creerCamCache()) {
      const float zD = zs, xD = ztx, yD = zty;
      const float vit = fmaxf(fabsf(xD - cPX), fmaxf(fabsf(yD - cPY), fabsf(zD - cPZ) * 160.0f)); cPZ = zD; cPX = xD; cPY = yD;      // vitesse de la caméra en pixels par image
      if (camRapide) { if (vit < 0.7f) camRapide = false; } else if (vit > 1.3f) camRapide = true;
      if (camRapide) { cOk = false; nEnCours = false; }
      else {
        bool flip = false;
        if (!cOk) {      // caméra presque immobile et pas encore de paysage en cache : une composition complète, une seule fois
          mo::composerPar(*camC, 0, H_SCENE, mo::cz, mo::cxx, mo::cyy); majBordsCam(*camC);
          cAZ = zD; cAX = xD; cAY = yD; cOk = true; nEnCours = false; flip = true;
        } else {
          if (!nEnCours && (fabsf(zD - cAZ) * 160.0f >= 1.0f || fabsf(xD - cAX) >= 1.0f || fabsf(yD - cAY) >= 1.0f)) { nEnCours = true; nBande = 0; nCz = mo::cz; nCx = mo::cxx; nCy = mo::cyy; }      // elle a bougé d'un pixel : nouveau paysage, composé en trois bandes
          if (nEnCours) {
            static const int BD[4] = {0, 70, 140, H_SCENE};
            mo::composerPar(*camN, BD[nBande], BD[nBande + 1], nCz, nCx, nCy);
            if (++nBande >= 3) { majBordsCam(*camN); M5Canvas* t = camC; camC = camN; camN = t; cAZ = nCz; cAX = 160.0f - nCx * nCz; cAY = 128.0f - nCy * nCz; nEnCours = false; flip = true; }
          }
        }
        zs = cAZ; ztx = cAX; zty = cAY; fondAct = camC; camUse = true;
        plein = pleinAvant || flip;      // image pleine seulement quand le nouveau paysage est posé (ou si un autre événement l'exige) ; sinon zones modifiées seulement
      }
    }
  } else if (camCache) { cOk = false; nEnCours = false; camRapide = true; }
  // volume affiché par le programme sur l'écran : on redessine tout quand il disparaît
  if (millis() < finAffVolume) volVu = true; else if (volVu) { volVu = false; plein = true; }
  if (dernierSecouement != secVu) { if (secVu != 0 || dernierSecouement != 0) { if (millis() - dernierSecouement < 600) secousse(); } secVu = dernierSecouement; }
  tAnim = millis() * 0.001f;
  presenceMaj(); reactionRetardee();
  if (nbOeufsSale && millis() - tOeufsEcr > 6000u && !occupe()) { prefs.putUInt("cboeufs", nbOeufs); nbOeufsSale = false; }      // ecrit en memoire quelques secondes apres l'evenement, jamais pendant
  for (int k = 0; k < NCB; k++) majCorbeau(cr[k], k, dt);
  majGlobal(dt * pr::facteurMonde());      // presence : en absence les evenements du monde sont rares
  presenceCb();
  uint32_t u1 = micros();
  if (presse >= 0 && millis() - tPressee > 180 && barI < 0) { presse = -1; stSale = false; barreCb(fond, choix, -1); syncUI(); barreSale = true; }
  else if (stSale) { stSale = false; barreCb(fond, choix, presse); syncUI(); barreSale = true; }      // 3.1.38 : le compteur de viande / noix a change
  nListe = 0;
  dessinerEntites(cv, true);        // 1) boîtes englobantes (rien n'est dessiné)
  const int lvPr = pr::niveauCb();      // presence : lumiere du ciel (basse en absence, elle revient a l'arrivee)
  { static int lvPrec = 8; if (lvPr < 8 || lvPr != lvPrec) plein = true; lvPrec = lvPr; }
  if (plein) {
    if (mo::camOn()) { if (camUse) memcpy(cv.getBuffer(), camC->getBuffer(), (size_t)320 * 240 * bpp); else mo::composer(); } else if (anim) composerTransition(); else memcpy(cv.getBuffer(), fondAct->getBuffer(), (size_t)320 * 240 * bpp);
    for (int e = 0; e < NENT; e++) dr[e] = true;      // image pleine : tout est redessiné
  } else {
    for (int e = 0; e < NENT; e++) {      // seuls les éléments qui ont changé (rectangle ou empreinte du dessin) sont effacés, redessinés et envoyés à l'écran
      const bool aN = nouv[e].x1 > nouv[e].x0, aA = anc[e].x1 > anc[e].x0;
      bool c;
      if (!aN && !aA) c = false;
      else if (!hachable(e)) c = true;
      else c = !(aN && aA && nouv[e].x0 == anc[e].x0 && nouv[e].y0 == anc[e].y0 && nouv[e].x1 == anc[e].x1 && nouv[e].y1 == anc[e].y1 && hsh[e] == hshAnc[e]);
      chg[e] = c;
      if (c) { if (aA) ajoutRect(anc[e]); if (aN) ajoutRect(nouv[e]); }
    }
    fusionner();
    for (int i = 0; i < nListe; i++) restaurer(liste[i]);
    for (int e = 0; e < NENT; e++) {      // un élément inchangé est quand même redessiné s'il touche une zone restaurée (l'ordre de superposition est conservé)
      bool d = chg[e];
      if (!d && nouv[e].x1 > nouv[e].x0) for (int i = 0; i < nListe; i++) { const Rect& r = liste[i]; if (nouv[e].x0 < r.x1 && r.x0 < nouv[e].x1 && nouv[e].y0 < r.y1 && r.y0 < nouv[e].y1) { d = true; break; } }
      dr[e] = d;
    }
  }
  uint32_t u2 = micros();
  dessinerEntites(cv, false);       // 2) dessin réel
  for (int e = 0; e < NENT; e++) { anc[e] = nouv[e]; hshAnc[e] = hsh[e]; }
  Rect barre; barre.x0 = 0; barre.y0 = H_SCENE; barre.x1 = 320; barre.y1 = 240;
  if (plein || barreSale) restaurer(barre);       // ce qui a pu déborder sur la barre est effacé
  uint32_t u3 = micros();
  if (lvPr < 8) pr::assombrir(cv, 0, H_SCENE, lvPr);      // presence : scene assombrie (jamais en pleine presence)
  M5.Display.startWrite();
  if (plein) { cv.pushSprite(0, 0); plein = false; }
  else { for (int i = 0; i < nListe; i++) pousser(liste[i]); if (barreSale) pousser(barre); }
  barreSale = false;
  M5.Display.endWrite();
  { uint32_t u4 = micros(); const float k = 0.12f;      // mesures lissées pour l'écran de contrôle et /api/etat
    emaMaj += ((u1 - u0) / 1000.0f - emaMaj) * k; emaCmp += ((u2 - u1) / 1000.0f - emaCmp) * k; emaEnt += ((u3 - u2) / 1000.0f - emaEnt) * k; emaPush += ((u4 - u3) / 1000.0f - emaPush) * k; emaCorb += (uCorb / 1000.0f - emaCorb) * k;
    emaTot += ((u4 - u0) / 1000.0f - emaTot) * k;
    allegement(); }
  nImg++; msDessin = (int)(millis() - m0);
  { const uint32_t itv = m0 - tPrecImg; if (tPrecImg && itv < 5000u && itv > maxIntervalle) maxIntervalle = itv; tPrecImg = m0; }      // pire intervalle entre deux images (pour msmax)
  if (millis() - tSec >= 1000) { fps = nImg; nImg = 0; tSec = millis();
    fpsHist[fpsHN % 5] = fps; fpsHN++; { const int n = fpsHN < 5 ? fpsHN : 5; int s = 0, mn = 9999; for (int i = 0; i < n; i++) { s += fpsHist[i]; if (fpsHist[i] < mn) mn = fpsHist[i]; } fpsMoy10 = s * 10 / n; fpsMin = mn; }      // moyenne glissante sur 5 s et plus bas récent
    msMax = (float)maxIntervalle; maxIntervalle = 0; memTas = (int)(ESP.getFreeHeap() / 1024); memPsram = (int)(ESP.getFreePsram() / 1024); if (debug) Serial.printf("[corbeau] %d i/s dessin=%d ms (logique %.1f fond %.1f dessin %.1f ecran %.1f dont corbeaux %.1f) Q%d%s heap=%u psram=%u\n", fps, msDessin, emaMaj, emaCmp, emaEnt, emaPush, emaCorb, (int)OP.q, qDeg ? "-" : "", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram()); }
}

// ---------- toucher ----------
// ---------- toucher : barre de modes, puis geste selon le mode ----------
static int trouverCorbeau(float x, float y, float marge) {
  int best = -1; float bd = 1e9f;
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; if (!c.visible || c.etat == E_DEHORS || c.etat == E_POOF) continue;
    float rx = (c.wl > 0.3f ? 30.0f : 20.0f) * c.s * marge, ry = (c.wl > 0.3f ? 26.0f : 16.0f) * c.s * marge;
    float dx = fabsf(x - c.x) / rx, dy = fabsf(y - (c.y + c.bob)) / ry;
    if (dx < 1 && dy < 1 && dx + dy < bd) { bd = dx + dy; best = k; }
  }
  return best;
}
static void infoNourr(int i) {      // 3.1.38 : nom et effet d'une nourriture (i >= 0) ou d'un objet du mode Lancer (i <= -2 : indice = -2 - i), au toucher long sur son icone
  static const char* const NN[6] = {"Graines", "Viande", "Noix", "Pain", "Baies", "Objet brillant"};
  static const char* const EN[6] = {"Un peu d'énergie. Préférées des calmes.", "Beaucoup d'énergie, plus fort. Costauds.", "Énergie moyenne, craque au bec. Curieux.", "Bonne énergie, se partage. Sociables.", "Peu d'énergie, mais de la joie. Audacieux.", "Un trésor : ils le cachent à la tour."};
  static const char* const NL[6] = {"Kunai", "Shuriken", "Bague", "Parchemin", "Objet brillant", "Oeuf"};
  static const char* const EL[6] = {"Ils l'attrapent en vol.", "Ils l'attrapent en vol.", "Un trésor à rapporter.", "Ils l'attrapent en vol.", "Un trésor à cacher.", "Attrapé il éclot, raté il éclate."};
  char a[40];
  if (i >= 0 && i < 6) { const int si = stIdx(i); if (si >= 0) snprintf(a, sizeof(a), "%s  (%d/%d, rare)", NN[i], (int)stK[si], (int)STMAX[si]); else snprintf(a, sizeof(a), "%s", NN[i]); infoMsg(a, EN[i], 3200); }
  else if (i <= -2 && -2 - i < 6) { const int j = -2 - i; infoMsg(NL[j], EL[j], 3200); }
}
static void touchBarreN(int x) {      // 3.1.44 : modes a gauche (4 colonnes de 48 px), nourriture ou objets a droite
  if (x < 198) {
    int m = (x - 4) / 48; if (m < 0) m = 0; if (m > 3) m = 3;
    if (m == modeCb && m == 1) { if (H.actif) retirerMain(); else tendreMain(200.0f, 140.0f); }      // re-toucher le mode Main : la main se retire ou ressort
    else if (m != modeCb) { if (appelOn) appelFin(); if (H.actif && m != 0 && m != 1 && m != 2) retirerMain(); VS.on = false; modeCb = (uint8_t)m; if (m == 1 && !H.actif) tendreMain(200.0f, 140.0f); son(SN_POP_APP, 90); }
  } else if (modeCb == 0 || modeCb == 2) {
    const int n = (modeCb == 0) ? 6 : 5; int i = ((x - 203) * n) / 114; if (i < 0) i = 0; if (i > n - 1) i = n - 1;
    if (modeCb == 0) choix = i; else choixL = (uint8_t)i;
    presse = i; tPressee = millis(); barI = (int8_t)i; barT0 = tPressee; barInfo = false;
  }
  barreCb(fond, choix, presse); syncUI(); barreSale = true;
}
static int hautTouche(int sx, int sy, int x, int y) {      // 3.1.44 : boutons du haut : 0 menu, 1 reglages, 2 visiteur, 3 vue ; zone tactile de 40 px de haut (au-dela de 24 px seulement si aucun corbeau n'est sous le doigt)
  if (sy >= 40) return -1;
  if (sy >= 24 && trouverCorbeau((float)x, (float)y, 1.0f / zs) >= 0) return -1;
  const int n = hautN(), w = (308 - 7 * (n - 1)) / n; int s = (sx - 3) / (w + 7); if (s < 0) s = 0; if (s > n - 1) s = n - 1;
  return (s == 0) ? 0 : ((s == n - 1) ? 3 : ((s == 1) ? 1 : 2));
}
static void hautAction(int k) {
  if (k == 0) { page = 6; bip(700, 40); }
  else if (k == 1) ouvrirPanneau();
  else if (k == 2) { if (V.ph == V_RIEN) invoquer(); else finVisite(); }
  else mo::basculer();
}
static void touchBarre(int x) {
  if (OP.ui) { touchBarreN(x); return; }
  if (x < 176) {
    int m = (int)lim((x - 4) / 34.0f, 0, 4);
    if (m == modeCb && m == 1) { if (H.actif) retirerMain(); else tendreMain(200.0f, 140.0f); }      // re-toucher le mode Main : la main se retire ou ressort
    else if (m != modeCb) { if (appelOn) appelFin(); if (H.actif && m != 0 && m != 1 && m != 2) retirerMain(); VS.on = false; modeCb = (uint8_t)m; if (m == 1 && !H.actif) tendreMain(200.0f, 140.0f); }      // choisir le mode Main : la main sort tout de suite
  } else if (modeCb == 0 || modeCb == 2) {
    int i = (int)lim((x - 180) / 22.0f, 0, 5);
    if (modeCb == 0) choix = i; else choixL = (uint8_t)i;      // nourriture choisie : on décide ensuite où elle va (toucher = poser, glisser = lancer)
    presse = i; tPressee = millis(); barI = (int8_t)i; barT0 = tPressee; barInfo = false;      // 3.1.38 : la nourriture tombe au relâchement d'un toucher court ; un toucher long (0,45 s) affiche son nom et son effet, sans la lancer
  } else if (modeCb == 3) appelSub = (x < 248) ? 0 : 1;
  barreCb(fond, choix, presse); syncUI(); barreSale = true;
}
static void appui(int x, int y) {
  uint32_t m = millis();
  const int sxp = x, syp = y;
  if (y < H_SCENE) { x = (int)((x - ztx) / zs); y = (int)((y - zty) / zs); }      // la scène est en coordonnées du monde
  D.bas = true; D.x0 = D.x = (float)x; D.y0 = D.y = (float)y; D.vx = D.vy = 0; D.t0 = D.tp = m; D.cible = -1; D.type = 0; D.fait = false; D.grab = false; tpCible = 0; tpFait = false;
  if (syp >= H_SCENE) { D.bas = false; touchBarre(sxp); return; }
  if (OP.ui) { const int hk = hautTouche(sxp, syp, x, y); if (hk >= 0) { D.bas = false; hautAction(hk); return; } }
  else {
  if (sxp < 50 && syp < 24) { D.bas = false; page = 6; bip(700, 40); return; }
  if (sxp >= 238 && sxp < 270 && syp < 24) { D.bas = false; mo::basculer(); return; }
  if (sxp >= 52 && sxp < 140 && syp < 24) {
    D.bas = false;
    ouvrirPanneau();
    return;
  }
  if (sxp >= 270 && syp < 20) { D.bas = false; return; }      // (le compteur d'images/s ne se bascule plus par un toucher : il se règle seulement dans Réglages > Fluidité)
  if (sxp >= 140 && sxp < 176 && syp < 24) { D.bas = false; return; }      // 3.1.40 : plus de pastille d'oeufs en haut (le nombre est dans la fiche de la colonie, l'oeuf a la demande dans Reglages > Fonctions) ;      // bouton œuf
  if (btnVis && sxp >= 176 && sxp < 236 && syp < 24) { D.bas = false; if (V.ph == V_RIEN) invoquer(); else finVisite(); return; }
  }
  if (V.ph == V_PRESENT && x >= VX - 26 && x < VX + (V.arm > 0.3f ? 40 : 26) && y >= visPiedsY() - 76 && y < visPiedsY() + 2) { tpCible = 1; tpT0 = m; return; }
  if (OE.actif && OE.etat == 0 && modeCb != 2 && modeCb != 4 && fabsf(x - OE.x) < 12 && fabsf(y - (OE.y - 5.0f)) < 13) { if (!fam::attraperOeuf()) craquer(); D.bas = false; return; }
  if (FI.k >= 0 && sxp >= fbx && sxp < fbx + FBW && syp >= fby && syp < fby + fbh) {      // toucher sur la fiche : les deux boutons, rien d'autre ne réagit
    const int k = FI.k;
    if (syp >= fby + fbb - 2 && sxp < fbx + 75) { cibleK = (int8_t)k; cibleT = m; FI.k = -1; Corbeau& cc = cr[k]; lacher(choix, lim(cc.x + cc.d * 14.0f * cc.s, 12.0f, 308.0f), cc.y - 10.0f); son(SN_POP_APP, 150); }      // il reçoit la nourriture choisie, à ses pieds
    else if (syp >= fby + fbb - 2) { cibleK = (int8_t)k; cibleT = m; FI.k = -1; son(SN_POP_APP, 150); }      // le prochain objet lancé ou la prochaine nourriture sera pour lui
    D.bas = false; return;
  }
  if (OP.fiche) {      // double toucher sur un corbeau : sa fiche (nom, trait, humeur, lien, rang) s'affiche quelques secondes
    int b2 = trouverCorbeau((float)x, (float)y, 1.3f / zs);
    if (b2 >= 0) { if (tapK == b2 && m - tapT < 350) { FI.k = (int8_t)b2; FI.t = m; tapK = -1; pendOn = false; son(SN_POP_APP, 110); D.bas = false; return; } tapK = (int8_t)b2; tapT = m; } else tapK = -1;
  }
  if (modeCb == 0 && H.actif && H.pal > 0.5f && surPaume((float)x, (float)y)) { mainPoser(choix); D.bas = false; return; }      // 3.1.42 : toucher la paume avec une nourriture choisie : elle se pose sur la main
  switch (modeCb) {
    case 0: {
      int b = trouverCorbeau((float)x, (float)y, 1.0f / zs);
      if (b >= 0) { mo::armer(b); mo::cibler(b); if (OP.fiche) { pendOn = true; pendK = (int8_t)b; pendX = (float)x; pendY = (float)y; pendT = m; { Corbeau& cq = cr[b]; cq.d = (x >= cq.x) ? 1 : -1; cq.look = lim(atan2f((float)y - cq.y, fabsf((float)x - cq.x) + 8.0f) * 0.7f, -0.3f, 0.8f); cq.tLook = 1.2f; if (cq.surs <= 0.0f) cq.surs = 0.16f; } } /* retour visible tout de suite : tete tournee vers le doigt, petit sursaut ; la reaction complete suit apres 0,28 s si ce n'est pas un double toucher */ else reagirToucher(cr[b], (float)x, (float)y); D.bas = false; return; }
      if (V.ph == V_RIEN && !GP.ph && hypotf((float)(sxp - 160), (float)(syp - 95)) < 42.0f) { tpCible = 3; tpT0 = m; tpX = (float)x; if (OP.lance) { D.type = 7; visType = (int8_t)choix; VS.on = true; VS.x0 = VS.x = (float)x; VS.y0 = VS.y = (float)y; } else D.bas = false; return; }      // toucher long au centre : un corbeau vient
      if (V.ph == V_RIEN && x >= 70 && x < 244 && fabsf(y - ytop((float)x)) < 14) { tpCible = 2; tpT0 = m; tpX = (float)x; if (OP.lance) { D.type = 7; visType = (int8_t)choix; VS.on = true; VS.x0 = VS.x = (float)x; VS.y0 = VS.y = (float)y; } else D.bas = false; return; }
      if (OP.lance) { D.type = 7; visType = (int8_t)choix; VS.on = true; VS.x0 = VS.x = (float)x; VS.y0 = VS.y = (float)y; } else { lacher(choix, (float)x); D.bas = false; }      // toucher = poser ici ; glisser = lancer
    } break;
    case 1: D.type = 1; D.cible = (int8_t)trouverCorbeau((float)x, (float)y, 1.5f / zs);
      if (!H.actif && D.cible < 0) tendreMain((float)x, (float)y);      // un appui sur le mode Main : la main sort tout de suite, à sa place fixe
      if (H.actif && OP.tend && TD.k >= 0 && D.cible == TD.k) { D.type = 8; D.grab = false; TD.carHold = 0.0f; break; }      // 3.1.39 : un toucher ou un glisser du doigt sur l'oiseau posé dans la paume = une caresse (pas un tracé de geste)
      if (OP.ui && H.actif) {      // 3.1.44 : la fronde est un geste du mode Main : toucher un corbeau (pas celui de la paume), le tirer en arriere, lacher
        const int bf = trouverCorbeau((float)x, (float)y, 1.0f / zs);
        if (bf >= 0 && cr[bf].perch != 14 && cr[bf].etat != E_TENU) { saisir(bf, (float)x, (float)y, true); if (cr[bf].etat == E_TENU) { D.type = 4; D.cible = (int8_t)bf; break; } }
      }
      if (H.actif) {      // un trait tracé n'importe où sur l'écran pilote le poignet et les doigts ; le bras ne bouge pas
        D.cible = -1; D.type = 6; TR.n = 0; TR.len = 0; TR.t0 = m; traceAjoute((float)x, (float)y);
        H.ctrl = true; H.circ = 0; H.lok = false; H.lx = (float)x; H.ly = (float)y;
      }
      break;
    case 2: D.type = 2; VS.on = true; VS.x0 = VS.x = (float)x; VS.y0 = VS.y = (float)y; break;
    case 3: if (OP.ui || appelSub == 0) { D.type = 3; appelDebut((float)x, (float)y); }      // 3.1.44 (interface nouvelle) : Voix = un toucher appelle, un balayage rapide les fait fuir (voir glisse)
       else { effrayer((float)x, (float)y); D.bas = false; } break;
    default: { int b = trouverCorbeau((float)x, (float)y, 1.6f / zs); if (b >= 0) { D.type = 4; D.cible = (int8_t)b; saisir(b, (float)x, (float)y, true); } else D.bas = false; } break;
  }
}
static float balX = 0.0f, balY = 0.0f;      // 3.1.44 : dernier point du balayage (mode Voix)
static void glisse(int x, int y) {
  if (!D.bas) return;
  x = (int)((x - ztx) / zs); y = (int)((y - zty) / zs);
  uint32_t m = millis(); float dt = (m - D.tp) / 1000.0f;
  if (dt > 0.004f) { D.vx = 0.6f * D.vx + 0.4f * (x - D.x) / dt; D.vy = 0.6f * D.vy + 0.4f * (y - D.y) / dt; D.tp = m; }
  D.x = (float)x; D.y = (float)y;
  float dist = hypotf(D.x - D.x0, D.y - D.y0);
  switch (D.type) {
    case 1:
      if (D.cible >= 0) {
        if (!D.grab && dist > 9) { D.grab = true; saisir(D.cible, D.x, D.y, false); }
        if (D.grab && cr[D.cible].etat == E_TENU) { cr[D.cible].tx = D.x; cr[D.cible].ty = D.y; }
      }
      break;
    case 6: {      // le trait pilote le poignet et les doigts (le bras reste immobile)
      traceAjoute(D.x, D.y);
      float sx = D.x - H.lx, sy = D.y - H.ly;
      if (hypotf(sx, sy) > 5.0f) {      // tour de cercle accumulé : assez de tours = la main se referme sur l'oiseau
        float a = atan2f(sy, sx);
        if (H.lok) { float th = a - H.la; while (th > 3.1416f) th -= 6.2832f; while (th < -3.1416f) th += 6.2832f; if (th * H.circ < 0 && fabsf(H.circ) > 0.5f) H.circ *= 0.4f; H.circ += th; if (fabsf(H.circ) > 2.6f) H.tHold = 1.3f; }
        H.la = a; H.lok = true; H.lx = D.x; H.ly = D.y;
      }
    } break;
    case 2: case 7: VS.x = D.x; VS.y = D.y; break;
    case 3:
      if (OP.ui && dist > 36.0f && hypotf(D.vx, D.vy) > 240.0f) { appelFin(); effrayer(D.x, D.y); D.type = 9; balX = D.x; balY = D.y; }      // 3.1.44 : balayage rapide = geste de repoussement : ils fuient
      else if (appelOn) { orbCx = D.x; orbCy = D.y; }
      break;
    case 9: if (hypotf(D.x - balX, D.y - balY) > 60.0f) { effrayer(D.x, D.y); balX = D.x; balY = D.y; } break;      // le balayage continue : chaque 60 px les corbeaux proches fuient encore
    case 4:
      if (D.cible >= 0 && cr[D.cible].etat == E_TENU) {
        float dx = D.x - FR.ax, dy = D.y - FR.ay, l = hypotf(dx, dy); if (l > 38.0f && !frTendu) { frTendu = true; son(SN_CROC, 75); }      // 3.1.43 : bruit de la fronde qui se tend
        if (l > 70) { dx *= 70 / l; dy *= 70 / l; }
        cr[D.cible].tx = FR.ax + dx; cr[D.cible].ty = FR.ay + dy;
      }
      break;
    default: break;
  }
}
static void relache() {
  if (!D.bas) return;
  D.bas = false; uint32_t m = millis(); float len = hypotf(D.x - D.x0, D.y - D.y0), sp = hypotf(D.vx, D.vy);
  switch (D.type) {
    case 1:
      if (D.cible >= 0) { if (D.grab) relacherMain(D.cible, D.x, D.y); else reagirToucher(cr[D.cible], D.x, D.y); }
      else if (len > 60 && sp > 280) balayage(D.x0, D.y0, D.x, D.y);
      else if (len < 14 && m - D.t0 < 500) appeler(D.x0, D.y0);
      break;
    case 7: {      // nourriture : un toucher la pose à cet endroit, un glisser la lance en arc
      VS.on = false; visType = -1;
      if (len < 14.0f) { lacher(choix, D.x0, D.y0); break; }
      float vx, vy; if (sp > 200) { vx = D.vx * 0.9f; vy = D.vy * 0.9f; } else { vx = (D.x - D.x0) * 7.0f; vy = (D.y - D.y0) * 7.0f; }
      lancerType((uint8_t)choix, D.x, D.y, vx, vy);
    } break;
    case 2: {
      VS.on = false; if (len < 10) break;
      float vx, vy; if (sp > 200) { vx = D.vx * 0.9f; vy = D.vy * 0.9f; } else { vx = (D.x - D.x0) * 7.0f; vy = (D.y - D.y0) * 7.0f; }
      lancer(D.x, D.y, vx, vy);
    } break;
    case 6: H.ctrl = false; traceFin(); break;
    case 8: TD.carHold = 1.0f; break;      // la caresse se prolonge un instant après que le doigt se lève
    case 3: appelFin(); break;
    case 4: if (D.cible >= 0) tirer(D.cible); FR.on = false; break;
    default: break;
  }
}
static void toucher(const m5::touch_detail_t& t) {
  if (!ouvert || !fondOk || ldPas >= 0) return;
  if (panneau) { if (t.wasPressed()) touchPanneau(t.x, t.y); return; }
  if (t.isPressed() || t.wasPressed()) tToucher = millis();
  if (GP.ph >= 3) { if (t.wasPressed() && GP.ph != 5) departGP(); return; }
  if (pinceOn) return;
  mo::appuiLong(t);
  if (tpCible && t.isPressed() && !tpFait && millis() - tpT0 > 600) { tpFait = true; if (D.type == 7) { D.type = 0; D.bas = false; VS.on = false; visType = -1; } if (tpCible == 1) bras(); else if (tpCible == 3) lancerVisite(); else invoquer(); }
  if (barI >= 0 && modeCb == 0 && H.actif && t.isPressed() && t.y < H_SCENE - 8) { iconDrag = true; dragX = ((float)t.x - ztx) / zs; dragY = ((float)t.y - zty) / zs; VS.on = true; VS.x = VS.x0 = dragX; VS.y = VS.y0 = dragY; visType = barI; barInfo = true; }      // 3.1.42 : on glisse l'icone de la barre vers la scene
  if (barI >= 0) {      // toucher sur une icone de la barre : toucher long = nom et effet ; toucher court = la nourriture tombe
    if (t.isPressed() && !barInfo && millis() - barT0 > 450u) { barInfo = true; infoNourr(modeCb == 0 ? barI : -2 - lanceVieux(barI)); }
    if (t.wasReleased() || !t.isPressed()) { const int bi = barI; const bool inf = barInfo, drg = iconDrag; barI = -1; barInfo = false; presse = -1; barreCb(fond, choix, -1); syncUI(); barreSale = true;
      if (drg) { iconDrag = false; VS.on = false; visType = -1; if (surPaume(dragX, dragY)) mainPoser(bi); else lacher(bi, lim(dragX, 12.0f, 308.0f)); }      // 3.1.42 : lache sur la paume = posee sur la main ; ailleurs = tombe la
      else if (!inf && modeCb == 0 && !OP.lance) lacher(bi, rnd(112, 300)); }
  }
  if (t.wasReleased()) { if (tpCible && !tpFait) { if (tpCible == 1) gerbe(); else if (D.type != 7) lacher(choix, tpX); } tpCible = 0; relache(); }
  if (t.wasPressed()) { if (!nat::toucher(t.x, t.y)) appui(t.x, t.y); }      // un toucher sur un predateur le fait fuir
  else if (t.isPressed()) glisse(t.x, t.y);
}

}  // namespace cb

static inline void corbeauDessiner() { cb::dessiner(); }
static inline void corbeauToucher(const m5::touch_detail_t& t) { cb::toucher(t); }
static inline void corbeauQuitter() { cb::quitter(); }
static inline bool corbeauChargement() { return cb::qPas < 0 && (!cb::ouvert || cb::ldPas >= 0); }      // l'ouverture progressive est en cours (ou va commencer) : la boucle la fait avancer sans attendre la cadence
static inline bool corbeauPret() { return cb::ouvert && cb::ldPas < 0 && cb::qPas < 0; }
static inline void corbeauQuitterDebut() { cb::quitterDebut(); }
static inline bool corbeauQuitterEnCours() { return cb::qPas >= 0; }
static inline void corbeauQuitterPas() { cb::quitterPas(); }
static inline void corbeauSurveillerCache(bool force) { cb::surveillerCache(force); }
static inline bool corbeauOccupe() { return cb::occupe(); }
static inline void corbeauSonVolume() { cb::son(cb::SN_CROC, 255); }
