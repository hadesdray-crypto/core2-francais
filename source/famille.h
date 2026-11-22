#pragma once
// ======================= GROUPE COHÉRENT ET FAMILLE (famille.h) =======================
// Inclus par corbeau.h, dans l'espace de noms cb, juste avant majCorbeau (après monde.h).
// 3.1.0 : une colonie persistante (identités, rôles, rangs, liens, perchoirs attitrés, sorties et sommeil en groupe, réactions collectives).
// L'état de la colonie est mémorisé dans la mémoire NVS (clé « cbgrp », un seul bloc compact).
// monde.h (vie sociale, nourriture) garde ses interactions : ce fichier lui fournit les rangs, les liens (mémorisés) et la force de chacun, et les relit à chaque tranche.
static void chasser(Corbeau& c);
static void orbiteLocale(Corbeau& c, float cx, float cy, float sens, float duree);
static void appliquerNb();
namespace fam {

static const int NM = NCB;                    // membres de la colonie : six adultes et le jeune
enum { R_ALPHA = 0, R_ADULTE, R_JEUNE };
enum { F_SOLO = 0, F_PARADE, F_NID, F_PONTE, F_COUVE, F_POUSSINS, F_JEUNES, F_VIDE };      // étapes du cycle de famille
struct Sauv {
  uint8_t ver, fph;                           // version du bloc ; phase du cycle de famille (utilisée dès 3.1.1)
  uint8_t aud[NM], cal[NM], cur[NM], soc[NM], frc[NM];      // traits stables : audace, calme, curiosité, sociabilité, force
  uint8_t role[NM], rang[NM];                 // rôle (alpha, adulte, jeune) et rang (0 = dominant)
  int8_t lien[NM * (NM - 1) / 2];              // liens entre paires (-100 rivaux, +100 amis)
  uint8_t brins, nEuf, nPou, gen, jeune;      // nid, œufs, poussins, générations, jeunes envolés (3.1.1 et 3.1.2)
  uint16_t age;                               // avancement de l'étape de la famille, en secondes
};
static Sauv S;
static bool pret = false, sale = false;
static int8_t foyer[NM];                      // perchoir attitré de chaque membre (-1 = aucun)
static float ageF = 0.0f, tBrin = 6.0f, tEuf = 15.0f, layT = 0.0f;
static float tDepl = 0.0f, tSauv = 0.0f, tTick = 0.0f, chg = 0.0f, gGrp = 40.0f, gRepos = 150.0f, repos = 0.0f, tRappel = -1.0f, tMob = 0.0f, tAlPrec = 0.0f;
static float eco[NM];                         // délai avant que le membre réponde au cri collectif
static bool vuV = false, vuH = false, vuP = false, rallie[NM];
static uint32_t vuSec = 0;
static int8_t sent = -1;                      // sentinelle du sommeil collectif
static int8_t lastM[NAD][NAD];                // dernier lien vu dans monde.h (-3 à +3) : pour répercuter ses changements dans la colonie
struct Suit { int8_t k, dest; float t; };
static Suit pend[2];
// ---- BLOC Y (3.1.11) : parent attitré du petit corbeau, entraide, protection, croissance (la logique est dans la section « BLOC Y » plus bas) ----
struct Pet { uint8_t ver, stade, par, rare, nour, res; uint16_t age; };      // le petit de l'œuf, mémorisé (clé cbpet) : stade 0 aucun, 1 poussin, 2 jeune, 3 grand ; par : son parent attitré (255 = aucun) ; nour : repas reçus ; age : secondes
static Pet PE;
struct Rg { uint8_t on, aide, prot, vit; };      // réglages « Famille » (clés cbfam, cbfaid, cbfpro, cbfvit) : activée ; entraide 0 non, 1 selon le caractère, 2 tous ; protection 0 douce, 1 normale, 2 forte ; croissance 0 lente, 1 normale, 2 rapide
static Rg RG = {1, 1, 1, 1};
static bool saleP = false, restau = false;
static int8_t pNext = -1, pondeur = -1, hostY = -1, hostCible = 0;      // parent désigné pour le prochain petit ; celui qui a pondu l'œuf de la branche ; corbeau qui menace le petit ; 0 : il menace le petit, 1 : son parent
static const float Y_JEUNE = 110.0f, Y_GRAND = 300.0f;
// ---- BLOC AF (3.1.19) : effectif réel, cycle de vie, nid construit, vrai couple (la logique est dans la section « BLOC AF » plus bas) ----
// Effectif = N (réglage Corbeaux) : un petit ne naît que d'un œuf attrapé (puis couvé) ou d'une ponte du couple, jamais en cachette ; devenu adulte il rejoint la colonie (N + 1, mémorisé).
struct Afr { uint8_t naiss, nuit, couple, nidv; };      // réglages « Vie, nid et couple » (clés cbafnai, cbafnui, cbafcpl, cbafnid) : naissance 0 œuf attrapé (défaut) 1 libre comme avant ; nuit 0 au nid 1 sur l'arbre ; couple 0 non 1 oui ; construction du nid 0 lente 1 normale 2 rapide
static Afr AFR = {0, 0, 1, 1};
struct Adu { uint8_t ver, k; uint16_t age; };      // le dernier petit devenu adulte : son emplacement dans la colonie et son âge en secondes de vie (clé cbafne)
static Adu ADU;
static float ageAdu = 0.0f, tSauvAdu = 0.0f; static bool saleAdu = false;
static const float Y_MOYEN = 200.0f, Y_VIEUX = 1500.0f;      // âges de vie (vitesse normale, secondes) : poussin 0, jeune 110, moyen 200, adulte 300, vieux 1500
static int8_t cvK = -1; static bool cvOn = false; static float tCv = 0.0f, tCvAbs = 0.0f;      // l'œuf attrapé : le corbeau qui le couve, couvaison en cours, temps couvé, temps sans couveur
static void sauverAdu(bool force) {
  if (!saleAdu) return; if (!force && tSauvAdu < 90.0f) return;
  ADU.age = (uint16_t)mnf(ageAdu, 65000.0f); prefs.putBytes("cbafne", &ADU, sizeof(ADU)); saleAdu = false; tSauvAdu = 0.0f;
}
static float nidMul() { static const float M[3] = {1.8f, 1.0f, 0.55f}; return M[AFR.nidv > 2 ? 1 : AFR.nidv]; }      // rythme de construction du nid
static int noeufs() { return AFR.naiss == 0 ? 1 : 3; }      // œufs d'une ponte : un seul (un petit visible, jamais caché), trois comme avant en mode libre
static bool afOn() { return pret && RG.on && AFR.naiss == 0; }      // effectif réel : plus de naissance sans œuf attrapé
static const char* nomVie(int v) { static const char* const N[5] = {"Poussin", "Jeune", "Moyen", "Adulte", "Vieux"}; return N[v < 0 ? 0 : (v > 4 ? 4 : v)]; }
static int vieDe(float age) { return age < Y_JEUNE ? 0 : (age < Y_MOYEN ? 1 : (age < Y_GRAND ? 2 : (age < Y_VIEUX ? 3 : 4))); }
static bool vieux(int k) { return pret && ADU.ver == 1 && k == (int)ADU.k && ageAdu >= Y_VIEUX; }
static void tailleVie(float age, float& s, float& duvet) {      // taille et duvet selon l'âge : poussin très duveteux, jeune, moyen presque emplumé, adulte
  if (age < Y_JEUNE) { float u = age / Y_JEUNE; s = lerpf(0.62f, 0.74f, u); duvet = lerpf(1.0f, 0.8f, u); }
  else if (age < Y_MOYEN) { float u = (age - Y_JEUNE) / (Y_MOYEN - Y_JEUNE); s = lerpf(0.74f, 0.94f, u); duvet = lerpf(0.8f, 0.42f, u); }
  else if (age < Y_GRAND) { float u = (age - Y_MOYEN) / (Y_GRAND - Y_MOYEN); s = lerpf(0.94f, 1.16f, u); duvet = lerpf(0.42f, 0.08f, u); }
  else { s = 1.22f; duvet = 0.0f; }
}
static int foyerNuit(int k);
static void nuitCouple();
static bool placeLibre();      // âge (secondes) où le poussin devient jeune, puis grand
static float faimP = 0.4f, tSoin = 8.0f, tAide = 20.0f, tMenP = 0.0f, tBat = 15.0f, tHost = 90.0f, tSauvP = 0.0f, tCriY = 0.0f, tChaY = 0.0f, tCriP = 4.0f, tEssai = 40.0f, tSurv = 5.0f, tProche = 20.0f;
static bool rage[NM];
static float tmis[NM];
struct MenY { bool on; uint8_t kind; int8_t qui; float x, y, tOff; };      // menace près du petit : 1 visiteur, 2 main, 3 corbeau hostile en vol, 4 objet lancé, 5 hostile qui vient le menacer
static MenY ym;
static bool soinOn() { return pret && RG.on && PE.stade >= 1 && PE.stade <= 2 && cr[PC].etat != E_OFF && cr[PC].visible; }
static bool grand() { return !RG.on || (PE.stade != 1 && PE.stade != 2); }      // le petit est grand (ou la famille coupée) : un nouvel œuf peut le remplacer
static float yInt(int k) {      // intensité de la défense : réglage Protection x audace du corbeau (un aidant protège moins fort que le parent)
  static const float PR[3] = {0.55f, 0.8f, 1.0f};
  float a = (k >= 0 && k < NM) ? S.aud[k] / 255.0f : 0.5f;
  return PR[RG.prot > 2 ? 1 : RG.prot] * (0.55f + 0.45f * a) * ((k == PE.par) ? 1.0f : 0.8f);
}
static float coteY(float off) {      // un point de la grosse branche, à côté du petit
  float cx = cr[PC].fx, x = cx + off; if (x < 84.0f || x > 226.0f) x = cx - off; return lim(x, 84.0f, 226.0f);
}
static void sauverY(bool force) {
  if (!pret) return;
  if (force && PE.stade >= 1 && PE.stade <= 2) saleP = true;
  if (!saleP || (!force && tSauvP < 90.0f)) return;
  PE.age = (uint16_t)mnf(agePous, 60000.0f); prefs.putBytes("cbpet", &PE, sizeof(PE)); saleP = false; tSauvP = 0.0f;
}

static int8_t& lien(int a, int b) { if (a > b) { int t = a; a = b; b = t; } return S.lien[a * NM - a * (a + 3) / 2 + b - 1]; }
static void changer(int a, int b, int d) {
  int8_t& l = lien(a, b); int v = (int)l + d; if (v > 100) v = 100; if (v < -100) v = -100;
  if (v != l) { chg += fabsf((float)(v - l)); l = (int8_t)v; if (chg >= 12.0f) sale = true; }
}

static void calculerRangs() {
  int sc[NM];
  for (int k = 0; k < NM; k++) sc[k] = S.frc[k] * 2 + S.aud[k] + (S.role[k] == R_ALPHA ? 2000 : 0) + (k == 0 ? 1000 : 0) - (S.role[k] == R_JEUNE ? 1000 : 0);
  for (int k = 0; k < NM; k++) { int n = 0; for (int j = 0; j < NM; j++) if (sc[j] > sc[k] || (sc[j] == sc[k] && j < k)) n++; S.rang[k] = (uint8_t)n; }
}
static void echanger(int k, int perch) { for (int j = 0; j < NM; j++) if (j != k && foyer[j] == perch) { foyer[j] = foyer[k]; break; } foyer[k] = (int8_t)perch; }
static void calculerFoyers() {
  static const int8_t HR[6] = {2, 3, 4, 1, 0, 5};      // du dominant au dominé : milieu de la branche, puis le perchoir haut, puis les bouts
  for (int k = 0; k < NM; k++) foyer[k] = (S.rang[k] < 6) ? HR[S.rang[k]] : (int8_t)-1;
  if (S.fph >= F_NID) { if (AFR.couple) { echanger(0, 1); echanger(1, 2); } else { echanger(0, 4); echanger(1, 2); } }      // le couple : côte à côte sur la grosse branche, près du nid (réglage Couple) ; sinon le mâle veille au-dessus du nid
}
static void generer() {
  memset(&S, 0, sizeof(S)); S.ver = 1;
  for (int k = 0; k < NM; k++) {
    S.aud[k] = (uint8_t)(50 + random(190)); S.cal[k] = (uint8_t)(40 + random(200)); S.cur[k] = (uint8_t)(40 + random(200)); S.soc[k] = (uint8_t)(40 + random(200)); S.frc[k] = (uint8_t)(40 + random(200));
    S.role[k] = (k < 2) ? R_ALPHA : (k < NM - 1 ? R_ADULTE : R_JEUNE);
  }
  S.aud[0] = (uint8_t)(210 + random(40)); S.frc[0] = (uint8_t)(215 + random(40)); S.soc[0] = (uint8_t)(120 + random(60)); S.cal[0] = (uint8_t)(150 + random(80));
  S.aud[1] = (uint8_t)(170 + random(40)); S.frc[1] = (uint8_t)(175 + random(40)); S.soc[1] = (uint8_t)(170 + random(60)); S.cal[1] = (uint8_t)(140 + random(80));
  S.aud[NM - 1] = (uint8_t)(60 + random(80)); S.frc[NM - 1] = (uint8_t)(30 + random(60)); S.cur[NM - 1] = (uint8_t)(190 + random(60)); S.soc[NM - 1] = (uint8_t)(140 + random(80));
  calculerRangs();
  for (int a = 0; a < NM; a++) for (int b = a + 1; b < NM; b++) lien(a, b) = (int8_t)(((int)S.soc[a] + S.soc[b]) / 16 - 15 + (int)random(21) - 10);
  lien(0, 1) = 35;
}
static bool valide() {
  if (S.ver != 1 || S.rang[NM - 1] != NM - 1) return false;
  bool vu[NM]; for (int k = 0; k < NM; k++) vu[k] = false;
  for (int k = 0; k < NM; k++) { if (S.rang[k] >= NM || vu[S.rang[k]] || S.role[k] > R_JEUNE) return false; vu[S.rang[k]] = true; }
  return true;
}
static void appliquer(Corbeau& c, int k) {      // les traits de la colonie deviennent la personnalité du corbeau
  if (k < 0 || k >= NM) return;
  c.pCal = S.cal[k] / 255.0f; c.pBav = S.soc[k] / 255.0f; c.pJou = S.cur[k] / 255.0f; c.pCra = 1.0f - S.aud[k] / 255.0f;
}
static void sauver(bool force) {
  sauverY(force); sauverAdu(force);
  if (!pret || !sale) return;
  if (!force && tSauv < 90.0f) return;
  S.age = (uint16_t)mnf(ageF, 60000.0f);
  prefs.putBytes("cbgrp", &S, sizeof(S)); sale = false; tSauv = 0.0f; chg = 0.0f;
}

static void sauverTard() { sale = true; if (tSauv < 87.0f) tSauv = 87.0f; }      // 3.1.26 : la sauvegarde des grands moments du nid est ecrite quelques secondes plus tard (jamais en pleine animation) ; quitter() l'ecrit tout de suite

static bool libre(int k) {      // perché sur l'arbre, sans rien à faire de particulier : disponible pour une sortie ou un rôle collectif
  if (k < 0 || k >= NAD) return false;
  const Corbeau& c = cr[k];
  if (k < 2 && S.fph >= F_NID) return false;      // le couple a mieux à faire
  if (k == PE.par && soinOn()) return false;      // le parent attitré veille sur son petit
  return c.etat == E_PERCHE && c.visible && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && !mo::rt[k].away;
}
static bool calme() {
  if (V.ph != V_RIEN || GP.ph || appelOn || H.actif || panneau || tAlerte > 0.0f) return false;
  for (int i = 0; i < NOB; i++) if (ob[i].actif) return false;
  return true;
}

// ---------- perchoirs attitrés : le plus fort a le meilleur, il déloge un dominé qui s'est mis sur sa place ----------
static int perchPour(const Corbeau& c) {
  int k = (int)(&c - cr), h = (pret && k >= 0 && k < NM) ? (repos > 0.0f ? foyerNuit(k) : foyer[k]) : -1;
  if (h >= 0) {
    if (!perchOccupe(h)) return h;
    int j = -1; for (int q = 0; q < NM; q++) if (cr[q].perch == h) j = q;
    if (j >= 0 && j != k && S.rang[j] > S.rang[k] && cr[j].etat == E_PERCHE && cr[j].objet == 0 && tDepl <= 0.0f && random(100) < 70) { tDepl = 9.0f; chasser(cr[j]); return h; }
  }
  int l[NPE], n = 0;      // sinon : un perchoir libre qui n'est le foyer de personne (de présent), à défaut n'importe lequel
  for (int i = 0; i < NPE; i++) {
    if (perchOccupe(i)) continue;
    bool fo = false; for (int q = 0; q < NM; q++) if (q != k && foyer[q] == i && cr[q].etat != E_OFF) fo = true;
    if (!fo) l[n++] = i;
  }
  if (n) return l[random(n)];
  return perchLibre();
}

// ---------- réactions collectives ----------
static void chante(int k) {
  Corbeau& c = cr[k]; if (c.etat != E_PERCHE || c.act == A_DORT) return;
  c.bec = 1.0f; c.flash = 0.4f; sonCri();
}
static void choeur(float x, float y, int nCris, bool tourner) {      // alarme : tous tournent la tête vers l'événement, les mieux classés répondent en chaîne
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible || c.perch >= 10) continue;
    if (tourner) { c.d = (x >= c.x) ? 1 : -1; c.look = lim(atan2f(y - c.y, fabsf(x - c.x) + 8.0f) * 0.7f, -0.4f, 0.8f); c.tLook = 1.8f; c.hum = 1.0f; }
    if (S.rang[k] < nCris && eco[k] <= 0.0f) eco[k] = 0.25f + 0.5f * S.rang[k] + rnd(0.0f, 0.2f);
  }
}
static void mobbing(float x, float y) {      // les plus audacieux foncent sur l'intrus et tournent autour de lui
  if (tMob > 0.0f) return; tMob = 30.0f;
  int m = 0;
  for (int k = 0; k < NAD && m < 2; k++) {
    if (!libre(k) || S.aud[k] < 150) continue;
    orbiteLocale(cr[k], x, y - 14.0f, random(2) ? 1.0f : -1.0f, 3.2f); cr[k].hum = 1.0f; m++;
  }
  if (m) sonCri();
}
static void rappel() {      // après une secousse : le groupe se reforme, les mieux classés d'abord
  for (int k = 0; k < NAD; k++) { Corbeau& c = cr[k]; if (c.etat == E_DEHORS && !c.rapporte) { float t = c.t + 0.8f + 0.8f * S.rang[k]; if (t < c.tfin) c.tfin = t; } }
}

// ---------- sorties en groupe : un meneur, d'autres le suivent vers la même destination ----------
static void suivreChef(const Suit& p) {
  if (!libre(p.k) || !mo::pret) return;
  if (p.dest == mo::D_MARE && mo::spotsPris(mo::D_MARE, 0) && mo::spotsPris(mo::D_MARE, 1)) return;
  mo::excursion(cr[p.k], p.k, p.dest);
}
static void sortieGroupe() {
  if (!mo::pret || mo::nbAilleurs() > 0) return;
  int l[NAD], n = 0; for (int k = 0; k < NAD; k++) if (libre(k)) l[n++] = k;
  if (n < 2) return;
  int ch = l[0]; for (int i = 1; i < n; i++) if (S.rang[l[i]] < S.rang[ch]) ch = l[i];
  int d = mo::choisirDest(ch); if (d < 0) return;
  if (!mo::excursion(cr[ch], ch, d)) return;
  cr[ch].bec = 1.0f; sonCri();
  int m = 0;
  for (int i = 0; i < n && m < 2; i++) {
    int k = l[i]; if (k == ch) continue;
    if (random(256) < 70 + S.soc[k] / 2 + lien(ch, k)) { pend[m].k = (int8_t)k; pend[m].dest = (int8_t)d; pend[m].t = 0.6f + 0.7f * m; m++; }
  }
}

// ---------- sommeil collectif : à des places attitrées, un sentinelle reste éveillé ----------
static void debutRepos() {
  repos = rnd(24.0f, 32.0f); sent = -1; int best = -1;
  for (int k = 0; k < NAD; k++) { rallie[k] = false; if (cr[k].etat == E_OFF) continue; if (best < 0 || S.aud[k] > S.aud[best]) best = k; }
  sent = (int8_t)best; nuitCouple();
}
static void finRepos(bool brusque) {
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || c.act != A_DORT) continue;
    if (brusque) { c.act = A_IMMO; c.dAct = 1.5f + 0.3f * S.rang[k]; } else { c.act = A_ETIRE; c.dAct = 1.8f; }
    c.tAct = 0.0f;
  }
  if (!brusque) { Corbeau& a = cr[0]; if (a.etat == E_PERCHE) { a.bec = 1.0f; sonCri(); } }
  repos = 0.0f; gRepos = rnd(160.0f, 260.0f); sale = true;
}
static void reposTick() {
  if (!calme()) { finRepos(true); return; }
  mo::gSoc = mxf(mo::gSoc, 3.0f); gSocial = mxf(gSocial, 3.0f); gPoof = mxf(gPoof, 3.0f); gAction = mxf(gAction, 2.0f); gObjet = mxf(gObjet, 3.0f); mo::gTrip = mxf(mo::gTrip, 3.0f);
  for (int k = 0; k < NAD; k++) {
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible) continue;
    if (c.but == B_FAM) { if (k != sent && c.act != A_DORT && c.act <= A_REGARDE) { c.act = A_DORT; c.tAct = 0.0f; c.dAct = repos + 1.0f; } continue; }      // au nid : elle dort sur place
    if (c.perch < 0 || c.perch >= 10) continue;
    int hm = foyerNuit(k);
    if (hm >= 0 && c.perch != hm && !rallie[k] && c.act != A_DORT && c.objet == 0) { rallie[k] = true; c.perch = -1; versPerchoir(c); continue; }      // chacun rejoint sa place
    if (k == sent) { if (c.act == A_DORT) { c.act = A_REGARDE; c.dAct = 4.0f; c.tAct = 0.0f; } continue; }
    if (c.act != A_DORT && c.act <= A_REGARDE && (hm < 0 || c.perch == hm || rallie[k])) { c.act = A_DORT; c.tAct = 0.0f; c.dAct = repos + 1.0f; }
  }
}

// ---------- liens : monde.h les fait évoluer (-3 à +3), la colonie les mémorise (-100 à +100) ----------
static int versM(int v) { return v >= 0 ? (v + 17) / 34 : -((-v + 17) / 34); }
static void lierMonde() {
  for (int a = 0; a < NAD; a++) for (int b = a + 1; b < NAD; b++) {
    int cur = mo::lien[a][b];
    if (cur != lastM[a][b]) { changer(a, b, (cur - lastM[a][b]) * 34); lastM[a][b] = lastM[b][a] = (int8_t)cur; }
    int want = versM(lien(a, b)); if (want > 3) want = 3; if (want < -3) want = -3;
    if (want != cur) { mo::lien[a][b] = mo::lien[b][a] = (int8_t)want; lastM[a][b] = lastM[b][a] = (int8_t)want; }
  }
}
static void apresMonde() {      // appelé juste après mo::reinit() : les rangs, les liens et la force mémorisés remplacent ceux tirés au hasard
  for (int k = 0; k < NAD; k++) { mo::rang[k] = (int8_t)S.rang[k]; mo::forceK[k] = (int8_t)(S.frc[k] / 100); }
  for (int a = 0; a < NAD; a++) for (int b = 0; b < NAD; b++) { int v = (a == b) ? 0 : versM(lien(a, b)); if (v > 3) v = 3; if (v < -3) v = -3; mo::lien[a][b] = lastM[a][b] = (int8_t)v; }
}

// ---------- famille (3.1.1 et 3.1.2) : couple, nid, œufs, couvaison, poussins, protection, croissance ----------
// Cycle accéléré : parade (~40 s), nid (~2 à 3 min), ponte (~1 min), couvaison (~2,5 min), poussins (~3 min), jeunes au bord du nid (~1 min), nid vide (~2 min), puis une nouvelle ponte.
// L'étape, le nid, les œufs, les poussins et leur âge sont mémorisés (S.fph, S.brins, S.nEuf, S.nPou, S.age, S.gen, S.jeune).
static const float NX = 88.0f, NY = 106.0f;      // le nid : sur une fourche au bout de la petite branche du tronc, devant le ciel
static const float CUPX = NX + 3.0f, RIMX = NX + 14.0f;      // où la femelle couve ; où se pose un parent qui apporte quelque chose ou qui monte la garde
static const int NB_NID = 16;                    // brindilles d'un nid fini
static const float T_SOLO = 45.0f, T_COUVE = 150.0f, T_POUS = 200.0f, T_JEUN = 70.0f, T_VIDE = 120.0f;
enum { M_RIEN = 0, M_VERS_BRIN, M_PIQUE, M_VERS_NID, M_DEPOSE, M_COUPLE, M_VERS_COUVE, M_NID, M_GARDE, M_NUIT, M_RELAIS, M_OEUF, M_Y_VERS, M_Y_DONNE, M_Y_GARDE, M_Y_VEILLE, M_Y_MENACE, M_Y_ESSAI };      // M_Y_* : missions du parent attitré, des aidants et du petit (bloc Y)
struct Mis { uint8_t ph, kind, porte; float t, def, tz, dur; };      // mission d'un parent (0 = le mâle, 1 = la femelle) ; kind : 1 brindille, 2 nourriture ; porte : ce qu'il tient au bec ; t avance image par image (pose) ; def : posture de défense (0 à 1)
static Mis mis[NM];
struct Par { bool on, go; uint8_t step; float t, sx; bool calin; };      // calin : la parade de couple recommence en cours de cycle (sans changer l'étape de la famille)
static Par par;
struct Men { bool on; uint8_t kind; int8_t qui; float x, y, tOff; };      // menace sur le nid : 1 visiteur, 2 main, 3 corbeau étranger, 4 objet lancé
static Men men;
struct Rlv { uint8_t ph, qui; float t; };      // relais de couvaison à tour de rôle : 0 rien, 1 l'autre vient au bord du nid, 2 le couveur s'en va, 3 l'autre s'installe
static Rlv RLV;
static float tRlv = 60.0f, tCalin = 70.0f;
static float faim[3], beg[3], couv = 0.0f, tNour = 20.0f, tChasse = 0.0f, tMen = 0.0f, tCri = 0.0f; static uint8_t sui = 0, ordre = 0;

static bool nichee() { return S.fph >= F_PONTE && S.fph <= F_JEUNES; }
static bool precieux() { return nichee() && (S.nEuf > 0 || S.nPou > 0); }
static float croiss() {      // croissance des poussins : 0 à 0,55 poussin duveteux, 0,55 à 1 jeune emplumé
  if (S.fph == F_POUSSINS) return 0.55f * lim(ageF / T_POUS, 0.0f, 1.0f);
  if (S.fph == F_JEUNES) return 0.55f + 0.45f * lim(ageF / T_JEUN, 0.0f, 1.0f);
  return 0.0f;
}
static float mereX() { return croiss() >= 0.35f ? NX + 19.0f : CUPX; }      // grands, les poussins prennent la place : elle se tient au bord
static float mereY() { return croiss() >= 0.35f ? NY + 0.5f : NY - 4.5f; }
static bool parentLibre(int p) {
  const Corbeau& c = cr[p];
  return c.etat == E_PERCHE && c.visible && c.but == B_RIEN && c.perch >= 0 && c.perch < NPE && c.objet == 0 && c.act <= A_REGARDE && !mo::rt[p].away;
}
static bool auNid(int p) { return mis[p].ph == M_NID && cr[p].etat == E_PERCHE && cr[p].but == B_FAM; }
static void versPoint(int p, float x, float y, uint8_t ph) {
  Corbeau& c = cr[p]; c.perch = -1; c.item = -1; c.rapporte = false; c.fuite = false; c.pj = -1;
  mis[p].ph = ph; mis[p].t = 0.0f; mis[p].tz = 0.0f; mis[p].dur = 0.0f; tmis[p] = 0.0f;
  decoller(c, x, y, B_FAM);
}
static void arrive(Corbeau& c, int idx) {      // fin de l'atterrissage d'un parent sur un point de sa mission
  int p = (idx >= 0 && idx < NM) ? idx : -1;
  if (p < 0 || mis[p].ph == M_RIEN) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; return; }
  Mis& m = mis[p]; c.t = 0.0f; c.ox = c.oy = 0.0f; c.act = A_IMMO; c.tAct = 0.0f; c.dAct = rnd(1.5f, 3.0f); c.sub = 0; c.peck = 0.0f; m.t = 0.0f;
  switch (m.ph) {
    case M_VERS_BRIN: c.etat = E_SOL; m.ph = M_PIQUE; break;
    case M_VERS_NID: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (NX >= c.fx) ? 1 : -1; m.ph = M_DEPOSE; break;
    case M_VERS_COUVE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (croiss() >= 0.35f) ? -1 : 1; m.ph = M_NID; break;
    case M_COUPLE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (cr[1 - p].x >= c.x) ? 1 : -1; break;
    case M_GARDE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (men.x >= c.fx) ? 1 : -1; break;
    case M_NUIT: case M_RELAIS: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (NX >= c.fx) ? 1 : -1; break;      // la nuit au nid, ou au bord du nid pour le relais de couvaison
    case M_OEUF: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (OE.x >= c.fx) ? 1 : -1; break;      // il couve l'œuf attrapé
    case M_Y_VERS: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (cr[PC].x >= c.x) ? 1 : -1; m.ph = M_Y_DONNE; break;
    case M_Y_GARDE: case M_Y_VEILLE: case M_Y_MENACE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (((m.ph == M_Y_MENACE && hostCible == 1 && PE.par < NAD) ? cr[PE.par].x : cr[PC].x) >= c.x) ? 1 : -1; break;
    case M_Y_ESSAI: c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = rnd(3.0f, 6.0f); posture(c); m.ph = M_RIEN; break;
    default: c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; m.ph = M_RIEN; break;
  }
}
static void sol(Corbeau& c, int idx, float dt) {      // au sol : il ramasse une brindille ou de la nourriture (tête qui plonge trois fois), puis la porte au nid
  (void)dt; if (idx < 0 || idx >= NM) return;
  Mis& m = mis[idx]; if (m.ph != M_PIQUE) return;
  float dip = fabsf(sinf(m.t / 0.42f * 3.1416f));
  c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
  if (m.t > 1.3f) {
    if (m.kind == 3) { float tx = coteY(c.x < cr[PC].fx ? -17.0f : 17.0f); versPoint(idx, tx, ytop(tx) + 0.4f, M_Y_VERS); mis[idx].porte = 2; mis[idx].kind = 3; }      // ce qu'il a trouvé est pour le petit
    else { uint8_t k = (m.kind == 2) ? 2 : 1; versPoint(idx, RIMX, NY - 7.5f, M_VERS_NID); mis[idx].porte = k; }
  }
}
static void debutParade() {
  memset(&par, 0, sizeof(par)); par.on = true; S.fph = F_PARADE; ageF = 0.0f;
}
static void finMission(int p) {
  Corbeau& c = cr[p]; mis[p].ph = M_RIEN; mis[p].porte = 0; mis[p].kind = 0;
  if (c.but != B_FAM) return;
  if (p == 1 && nichee() && !auNid(0) && RLV.ph == 0) { versPoint(1, mereX(), mereY(), M_VERS_COUVE); return; }      // la femelle retourne sur le nid (sauf si le mâle couve à son tour)
  c.but = B_RIEN; c.tfin = rnd(2.0f, 5.0f); c.perch = -1; versPerchoir(c);
}
static void annulerParade() {
  if (par.calin) { par.on = false; par.calin = false; if (mis[1].ph == M_COUPLE) finMission(1); if (mis[0].ph == M_COUPLE) finMission(0); return; }
  par.on = false; S.fph = F_SOLO; ageF = T_SOLO - 15.0f;
  if (mis[1].ph == M_COUPLE) finMission(1);
  if (cr[0].etat == E_PERCHE) cr[0].tfin = rnd(3.0f, 6.0f);
}
static void paradeTick(float h) {
  if (!par.on) { if (!par.calin) S.fph = F_SOLO; return; }
  if (cr[0].etat == E_OFF || cr[1].etat == E_OFF || (par.step < 3 && !calme())) { annulerParade(); return; }
  mo::gSoc = mxf(mo::gSoc, 3.0f); gSocial = mxf(gSocial, 3.0f); gPoof = mxf(gPoof, 3.0f); gAction = mxf(gAction, 2.0f); gObjet = mxf(gObjet, 3.0f); mo::gTrip = mxf(mo::gTrip, 3.0f);
  par.t += h;
  switch (par.step) {
    case 0:      // rapprochement : la femelle vient se poser tout contre le mâle
      if (!par.go) {
        if (parentLibre(0) && parentLibre(1)) {
          if (par.calin) {      // parade en cours de cycle : ils se retrouvent tous deux sur la grosse branche, côte à côte
            float x0 = rnd(112.0f, 190.0f); par.sx = x0 + 19.0f; par.go = true; par.t = 0.0f;
            versPoint(0, x0, ytop(x0) + 0.4f, M_COUPLE); versPoint(1, x0 + 19.0f, ytop(x0 + 19.0f) + 0.4f, M_COUPLE);
          } else {
            float mx = cr[0].fx, tx = (mx + 19.0f <= 232.0f) ? mx + 19.0f : mx - 19.0f; par.sx = tx; par.go = true; par.t = 0.0f;
            versPoint(1, tx, ytop(tx) + 0.4f, M_COUPLE);
          }
        } else if (par.t > 8.0f) annulerParade();
      } else if (mis[1].ph == M_COUPLE && cr[1].etat == E_PERCHE && cr[1].but == B_FAM && (!par.calin || (mis[0].ph == M_COUPLE && cr[0].etat == E_PERCHE && cr[0].but == B_FAM))) { par.step = 1; par.t = 0.0f; }
      else if (par.t > 14.0f) annulerParade();
      break;
    case 1:      // lissage mutuel : elle lisse le mâle, puis il la lisse
    case 2:      // partage de nourriture : il lui passe un morceau de bec à bec
      for (int p = 0; p < 2; p++) { Corbeau& c = cr[p]; c.act = A_IMMO; c.tAct = 0.0f; c.dAct = 3.0f; c.tfin = 1e9f; }
      if (par.step == 1 && par.t > 9.0f) { par.step = 2; par.t = 0.0f; }
      else if (par.step == 2) {
        if (par.t > 2.2f && par.t - h <= 2.2f) cr[0].bec = 0.6f;
        if (par.t > 3.3f && par.t - h <= 3.3f) { cr[1].bec = 1.0f; son(SN_GAZ, 120); }
        if (par.t > 6.0f) {      // vol de parade : tous deux décollent et tournent côte à côte
          float cx = rnd(120.0f, 200.0f), cy = rnd(52.0f, 80.0f), sens = random(2) ? 1.0f : -1.0f, dur = 7.0f;
          mis[1].ph = M_RIEN; cr[1].but = B_RIEN; mis[0].ph = M_RIEN; cr[0].but = B_RIEN; cr[0].tfin = 4.0f; cr[1].tfin = 4.0f;
          orbiteLocale(cr[0], cx, cy, sens, dur); orbiteLocale(cr[1], cx, cy, sens, dur);
          cr[1].ray = cr[0].ray; cr[1].ang = cr[0].ang - 0.55f * sens; cr[1].om = cr[0].om;
          sonCri(); son(SN_AILES, 200); par.step = 3; par.t = 0.0f;
        }
      }
      break;
    default:      // retour aux perchoirs, puis le couple est formé : le nid commence
      if (par.t > 11.0f || (par.t > 4.0f && cr[0].etat == E_PERCHE && cr[1].etat == E_PERCHE)) {
        if (par.calin) { par.on = false; par.calin = false; changer(0, 1, 6); sale = true; }      // simple parade de couple : le cycle continue là où il en était
        else { par.on = false; S.fph = F_NID; ageF = 0.0f; tBrin = rnd(5.0f, 8.0f) * nidMul(); changer(0, 1, 100); calculerFoyers(); sauverTard(); }
      }
      break;
  }
}

// ---------- menace sur le nid : le visiteur, la main, un corbeau étranger ou un objet lancé près du nid ----------
static void detecterMenace(float h) {
  bool on = false; uint8_t kind = 0; int8_t qui = -1; float x = NX, y = NY;
  if (precieux()) {
    if (V.ph != V_RIEN) { on = true; kind = 1; x = VX; y = 92.0f; }
    else if (H.actif && hypotf(H.x - NX, H.y - NY) < 130.0f) { on = true; kind = 2; x = H.x; y = H.y; }
    else {
      for (int i = 0; i < NPROJ && !on; i++) if (proj[i].actif && hypotf(proj[i].x - NX, proj[i].y - NY) < 70.0f) { on = true; kind = 4; x = proj[i].x; y = proj[i].y; }
      for (int k = 2; k < NM && !on; k++) {
        const Corbeau& c = cr[k]; if (!c.visible || c.but == B_ORBITE || (k == NM - 1 && S.fph >= F_JEUNES)) continue;
        if ((c.etat == E_VOL || c.etat == E_DECOLLE || c.etat == E_ATTERRIT) && hypotf(c.x - NX, c.y - NY) < 34.0f) { on = true; kind = 3; qui = (int8_t)k; x = c.x; y = c.y; }
      }
    }
  }
  tMen -= h; tChasse -= h; tCri -= h;
  if (on) {
    bool debut = !men.on; men.on = true; men.kind = kind; men.qui = qui; men.x = x; men.y = y; men.tOff = 0.0f;
    if (debut && tMen <= 0.0f) { tMen = 12.0f; tAlerte = mxf(tAlerte, 3.5f); dirAlerte = (x >= NX) ? 1 : -1; alerteFuite = false; sale = true; }      // alarme : tout le groupe se fige et regarde l'intrus
    if (tCri <= 0.0f) { tCri = 2.2f; for (int p = 0; p < 2; p++) if (mis[p].ph == M_NID || mis[p].ph == M_GARDE || p == 0) { cr[p].bec = 1.0f; } son(SN_ALARME, 170); }
  } else if (men.on) { men.tOff += h; if (men.tOff > 3.0f) men.on = false; }
}
static void protegerNid() {      // le mâle s'interpose ailes ouvertes ou chasse l'étranger ; il reprend sa branche quand tout est calme
  Corbeau& c = cr[0];
  if (men.on) {
    if (men.kind == 3 && men.qui >= 0 && tChasse <= 0.0f && parentLibre(0) && mis[0].ph == M_RIEN) {      // il fonce sur l'étranger et le chasse
      tChasse = 25.0f; Corbeau& v = cr[men.qui];
      c.perch = -1; c.pj = men.qui; c.tOrb = rnd(3.5f, 5.0f); c.hum = 1.0f; decoller(c, v.x, v.y, B_POURSUITE); sonCri(); chasser(v);
    } else if (men.kind != 3 && mis[0].ph == M_RIEN && parentLibre(0)) versPoint(0, RIMX, NY - 7.5f, M_GARDE);      // il s'interpose devant le nid
  } else if (mis[0].ph == M_GARDE) finMission(0);
}
static void nourrir(int p) {      // ce qu'un parent a rapporté : un morceau pour la femelle qui couve, ou pour les poussins
  if (S.fph == F_COUVE || S.fph == F_PONTE) { if (p == 0) { cr[1].bec = 1.0f; cr[1].flash = 0.3f; son(SN_GAZ, 110); } else if (auNid(0)) { cr[0].bec = 1.0f; cr[0].flash = 0.3f; son(SN_GAZ, 110); } return; }      // le mâle nourrit la femelle qui couve, et la femelle nourrit le mâle quand il couve à son tour
  for (int j = 0; j < 2 && S.nPou > 0; j++) { int i = sui % S.nPou; faim[i] = 0.0f; beg[i] = 1.0f; sui++; }
  son(SN_GAZ, 140);
}
static void eclosion() {      // les œufs s'ouvrent : des poussins duveteux au bec grand ouvert
  S.nPou = (uint8_t)((S.nEuf > 1 && random(100) < 25) ? S.nEuf - 1 : S.nEuf); if (S.nPou < 1) S.nPou = 1;
  S.nEuf = 0; S.fph = F_POUSSINS; ageF = 0.0f; tNour = rnd(6.0f, 10.0f);
  for (int i = 0; i < 3; i++) { faim[i] = 0.5f; beg[i] = 1.0f; }
  eclat(NX, NY - 8.0f, 0.3f, 1.2f); plumes(NX, NY - 8.0f, 6); son(SN_GAZ, 180); son(SN_PLUMES, 120); sauverTard();
}
static void envol() {      // un jeune quitte le nid : le premier rejoint le groupe quand la place est libre, les autres s'en vont
  if (S.nPou == 0) return;
  S.nPou--; S.jeune++; sale = true;
  const bool nait = (AFR.naiss == 0) ? placeLibre() : ((cr[PC].etat == E_OFF || grand()) && random(100) < 70);      // effectif réel : le petit du nid devient un corbeau visible quand l'emplacement du petit est libre
  if (nait) { pNext = 1; naissance(NX + 2.0f, NY - 5.0f, false); agePous = 140.0f; if (PE.stade == 1) { PE.stade = 2; PE.age = 140; saleP = true; } }
  else minisEnvol(NX, NY - 12.0f, 1);
  son(SN_AILES, 170); sonCri(); plumes(NX, NY - 10.0f, 6);
}
// ======================= BLOC AF, partie 1 (3.1.19) : effectif réel, nuit au nid, couple =======================
static bool placeLibre() { return RG.on && cr[PC].etat == E_OFF; }      // l'emplacement du petit est libre : un petit peut naître
static bool reproOk() {      // une nouvelle ponte : seulement quand l'enfant précédent est devenu adulte (et si l'emplacement du petit est libre)
  if (AFR.naiss != 0) return true;
  return RG.on && cr[PC].etat == E_OFF && (PE.stade == 0 || PE.stade >= 3);
}
static bool nidPret() { return pret && RG.on && S.fph >= F_NID && S.brins >= 8 && cr[0].etat != E_OFF && cr[1].etat != E_OFF; }
static bool nuitNid() { return AFR.nuit == 0 && nidPret() && nAct >= 2; }
static int foyerNuit(int k) {      // place de nuit : le couple au nid ; les autres d'abord sur les deux branches secondaires (haute, gauche), puis sur la grosse branche, toujours les mêmes d'une nuit à l'autre (selon le rang)
  if (k < 0 || k >= NM) return -1;
  if (!nuitNid() || k < 2 || k >= NAD) return foyer[k];
  int r = 0; for (int j = 2; j < NAD; j++) if (j != k && cr[j].etat != E_OFF && S.rang[j] < S.rang[k]) r++;
  static const int8_t MB[4] = {2, 3, 1, 0};
  if (r == 0) return 4;
  if (r == 1) return 5;
  return MB[(r - 2) & 3];
}
static void nuitCouple() {      // la nuit tombe : la femelle s'installe dans la coupe du nid, le mâle monte la garde sur le bord (si le nid est assez fait) ; ils y restent jusqu'au réveil
  if (!nuitNid() || men.on || par.on) return;
  if (mis[1].ph == M_RIEN && parentLibre(1)) versPoint(1, CUPX, NY - 4.5f, M_NUIT);
  if (mis[0].ph == M_RIEN && parentLibre(0)) versPoint(0, RIMX, NY - 7.5f, M_NUIT);
}
static bool coupleLibre() { return mis[0].ph == M_RIEN && mis[1].ph == M_RIEN && parentLibre(0) && parentLibre(1) && calme() && !men.on && !par.on && repos <= 0.0f && RLV.ph == 0; }
static void relaisTick(float h) {      // couver à tour de rôle : l'autre vient au bord du nid, le couveur lui cède la place et part manger, puis ils échangent de nouveau
  if (!AFR.couple || S.fph != F_COUVE) {
    if (RLV.ph || auNid(0)) { RLV.ph = 0; if (mis[0].ph == M_NID || mis[0].ph == M_RELAIS) finMission(0); if (mis[1].ph == M_RELAIS) finMission(1); }
    return;
  }
  const int a = RLV.qui, b = 1 - a;
  switch (RLV.ph) {
    case 0: {
      int q = auNid(0) ? 0 : 1, o = 1 - q; const bool fin = ageF > T_COUVE - 40.0f;
      tRlv -= h;
      if (tRlv > 0.0f && !(q == 0 && fin)) break;
      if (q == 1 && (fin || ageF < 25.0f)) { tRlv = 6.0f; break; }
      if (auNid(q) && mis[o].ph == M_RIEN && parentLibre(o) && calme() && !men.on && !par.on && repos <= 0.0f) { RLV.qui = (uint8_t)q; versPoint(o, RIMX, NY - 7.5f, M_RELAIS); RLV.ph = 1; RLV.t = 0.0f; }
      else tRlv = rnd(5.0f, 9.0f);
    } break;
    case 1:      // l'autre vient au bord du nid : un petit cri, un morceau de bec à bec, le couveur se lève
      RLV.t += h;
      if (mis[b].ph == M_RELAIS && cr[b].etat == E_PERCHE && cr[b].but == B_FAM && mis[b].t > 1.0f && auNid(a)) {
        cr[b].bec = 0.6f; cr[a].bec = 0.8f; son(SN_GAZ, 110);
        mis[a].ph = M_RIEN; mis[a].porte = 0; cr[a].but = B_RIEN; cr[a].tfin = rnd(2.0f, 4.0f); cr[a].perch = -1; versPerchoir(cr[a]);
        RLV.ph = 2; RLV.t = 0.0f;
      } else if (RLV.t > 14.0f || !auNid(a)) { if (mis[b].ph == M_RELAIS) finMission(b); RLV.ph = 0; tRlv = rnd(20.0f, 30.0f); }
      break;
    case 2:      // l'autre s'installe dans la coupe
      RLV.t += h;
      if (RLV.t > 0.9f) { versPoint(b, mereX(), mereY(), M_VERS_COUVE); RLV.ph = 3; RLV.t = 0.0f; }
      break;
    default:
      RLV.t += h;
      if (auNid(b)) { RLV.ph = 0; tRlv = rnd(40.0f, 65.0f); }
      else if (RLV.t > 14.0f) { RLV.ph = 0; tRlv = 15.0f; }
      break;
  }
}
static void calinTick(float h) {      // de temps en temps le couple se retrouve : lissage mutuel, partage d'un morceau, vol de parade côte à côte
  if (!AFR.couple || par.on) return;
  if (S.fph != F_NID && S.fph != F_VIDE) return;
  tCalin -= h; if (tCalin > 0.0f) return;
  tCalin = rnd(75.0f, 130.0f);
  if (coupleLibre() && !precieux()) { memset(&par, 0, sizeof(par)); par.on = true; par.calin = true; }
}
static void afPilote(float h) {
  for (int p = 0; p < 2; p++) if (mis[p].ph == M_NUIT && repos <= 0.0f && mis[p].t > 1.5f) finMission(p);      // le jour se lève : ils quittent le nid
  relaisTick(h);
  calinTick(h);
}
static void pilote(float h) {      // une tranche de 0,25 s : les missions des deux parents selon l'étape du cycle
  for (int p = 0; p < 2; p++) { Mis& m = mis[p]; if (m.ph != M_RIEN && (cr[p].etat == E_OFF || cr[p].but != B_FAM)) { m.ph = M_RIEN; m.porte = 0; m.kind = 0; } }
  detecterMenace(h);
  if (S.fph == F_PARADE || (par.on && par.calin)) { paradeTick(h); return; }
  ageF += h;
  afPilote(h);
  for (int p = 0; p < 2; p++) {      // dépôt de ce que le parent a rapporté : une brindille s'ajoute au nid, de la nourriture va au bec de la femelle ou des poussins
    Mis& m = mis[p]; if (m.ph != M_DEPOSE || cr[p].etat != E_PERCHE) continue;
    if (m.porte == 1 && m.t > 0.6f) { m.porte = 0; if (S.brins < NB_NID) S.brins++; sale = true; son(SN_CROC, 70); }
    if (m.porte == 2 && m.t > 0.7f) { m.porte = 0; nourrir(p); }
    if (m.t > 1.4f) { finMission(p); tBrin = rnd(3.0f, 7.0f) * nidMul(); }
  }
  if (nichee()) {
    protegerNid();
    if (mis[1].ph == M_RIEN && cr[1].etat != E_OFF && parentLibre(1) && calme() && !men.on && RLV.ph == 0 && !auNid(0) && !(S.fph >= F_COUVE && cvOn)) versPoint(1, mereX(), mereY(), M_VERS_COUVE);      // la femelle rejoint le nid
    else if (auNid(1) && fabsf(cr[1].fx - mereX()) > 2.5f && calme() && !men.on) versPoint(1, mereX(), mereY(), M_VERS_COUVE);      // elle change de place
    if (S.fph >= F_COUVE && calme() && !men.on) {      // le mâle (puis la femelle, quand les petits ont grandi) va chercher à manger
      tNour -= h;
      if (tNour <= 0.0f) {
        int p = 0;
        if (S.fph >= F_POUSSINS && croiss() >= 0.3f && ordre) p = 1;
        bool ok;
        if (auNid(0)) { p = 1; ok = (mis[1].ph == M_RIEN && parentLibre(1) && RLV.ph == 0); }      // le mâle couve à son tour : la femelle va chercher à manger
        else ok = (mis[0].ph == M_RIEN || mis[0].ph == M_GARDE) && (mis[1].ph == M_RIEN || mis[1].ph == M_NID) && (p == 1 ? auNid(1) : parentLibre(0));
        if (ok) {
          float gx = rnd(110.0f, 300.0f); versPoint(p, gx, solHaut(gx), M_VERS_BRIN); mis[p].kind = 2; mis[p].porte = 0; ordre ^= 1;
          tNour = (S.fph == F_COUVE) ? rnd(26.0f, 38.0f) : rnd(10.0f, 16.0f);
        } else tNour = 1.0f;
      }
    }
  }
  switch (S.fph) {
    case F_SOLO:
      if (RG.on && ageF > T_SOLO && calme() && parentLibre(0) && parentLibre(1) && mo::nbAilleurs() == 0) debutParade();
      break;
    case F_NID:
      tBrin -= h;
      if (tBrin <= 0.0f && calme()) {      // les deux parents peuvent aller chercher des brindilles en même temps (par terre, au loin), une par voyage
        int en = 0; for (int q = 0; q < 2; q++) if (mis[q].kind == 1 && (mis[q].ph == M_VERS_BRIN || mis[q].ph == M_PIQUE || mis[q].ph == M_VERS_NID || mis[q].ph == M_DEPOSE)) en++;
        if (S.brins + en < NB_NID && en < 2) {
          int p = random(2); if (!parentLibre(p) || mis[p].ph != M_RIEN) p = 1 - p;
          if (parentLibre(p) && mis[p].ph == M_RIEN) { float gx = rnd(110.0f, 300.0f); versPoint(p, gx, solHaut(gx), M_VERS_BRIN); mis[p].kind = 1; mis[p].porte = 0; tBrin = rnd(2.5f, 5.0f) * nidMul(); } else tBrin = 1.0f;
        } else tBrin = 1.0f;
      }
      if (S.brins >= NB_NID && mis[0].ph == M_RIEN && mis[1].ph == M_RIEN && reproOk()) { S.fph = F_PONTE; ageF = 0.0f; tEuf = rnd(8.0f, 12.0f); S.nEuf = 0; S.nPou = 0; sauverTard(); }
      break;
    case F_PONTE: case F_COUVE:
      if (S.fph == F_PONTE && auNid(1)) {
        tEuf -= h; if (tEuf <= 0.0f && layT <= 0.0f && S.nEuf < noeufs()) { layT = 1.5f; tEuf = rnd(16.0f, 24.0f); }
      }
      if (S.fph == F_PONTE && S.nEuf >= noeufs()) { S.fph = F_COUVE; ageF = 0.0f; tNour = rnd(14.0f, 22.0f); sauverTard(); }
      if (S.fph == F_COUVE && ageF > T_COUVE && auNid(1)) eclosion();
      break;
    case F_POUSSINS:
      if (ageF > T_POUS) { S.fph = F_JEUNES; ageF = 0.0f; sauverTard(); }
      break;
    case F_JEUNES:
      if (S.nPou > 0 && ageF > T_JEUN * 0.55f + 9.0f * (S.jeune % 3)) { if (AFR.naiss != 0 || placeLibre() || ageF > T_JEUN + 400.0f) envol(); }      // l'un après l'autre ; il attend que l'emplacement du petit soit libre
      if (S.nPou == 0) { S.fph = F_VIDE; ageF = 0.0f; S.gen++; sauverTard(); finMission(1); }
      break;
    case F_VIDE:
      if (RG.on && ageF > T_VIDE && reproOk()) { S.fph = F_PONTE; ageF = 0.0f; tEuf = rnd(8.0f, 12.0f); S.nEuf = 0; sauverTard(); }
      break;
    default: break;
  }
}

// ---------- dessin du nid (derrière les corbeaux), des brindilles du devant, des poussins, des morceaux portés au bec ----------
static uint32_t hsh(uint32_t i) { i *= 2654435761u; i ^= i >> 15; i *= 2246822519u; i ^= i >> 13; return i; }
static float h01(int i, int s) { return (hsh((uint32_t)(i * 7 + s * 131 + 17)) & 1023u) / 1023.0f; }
static void brin(M5Canvas& g, int i) {      // un brin de la coupe : plus il est haut, plus la coupe est large
  float u = (float)i / NB_NID, w = 9.0f + 7.0f * u, cx = NX + (h01(i, 1) - 0.5f) * 2.0f * (w - 3.0f);
  float a = (h01(i, 2) - 0.5f) * 1.3f, L = 6.5f + 5.0f * h01(i, 3), yy = NY - 9.5f * u - 0.8f * h01(i, 4);
  uint16_t col = (i % 3 == 0) ? C(84, 34, 20) : ((i % 3 == 1) ? C(56, 22, 14) : C(104, 47, 26));
  float dx = cosf(a) * L * 0.5f, dy = sinf(a) * L * 0.5f;
  ligne(g, cx - dx, yy - dy, cx + dx, yy + dy, col); ligne(g, cx - dx, yy - dy + 1.0f, cx + dx, yy + dy + 1.0f, col);
}
static void fourche(M5Canvas& g) {      // la fourche choisie : un support noir au bout de la petite branche, deux dents qui tiennent la coupe
  const uint16_t nr = C(5, 2, 4);
  mo::epais(g, 74.0f, 109.0f, 110.0f, 109.5f, 4.6f, nr); mo::epais(g, 86.0f, 109.0f, 103.0f, 100.0f, 2.8f, nr); mo::epais(g, 80.0f, 109.5f, 96.0f, 117.0f, 2.6f, nr);
  disque(g, 110.0f, 109.5f, 2.0f, nr);
}
static void poussin(M5Canvas& g, int i, int n, float gr, float lev) {      // un poussin : duvet gris qui devient plumes noires, bec rouge grand ouvert quand il réclame
  const float sz = lerpf(0.62f, 1.15f, gr), x = NX + (i - (n - 1) * 0.5f) * 6.4f - 1.5f, by = NY - 5.0f, w = 4.2f * sz, hh = 3.6f * sz;
  const bool plu = gr >= 0.55f;
  const uint16_t col = plu ? C(14, 8, 12) : C(46, 39, 43), clair = plu ? C(70, 30, 34) : C(92, 82, 86);
  if (plu) {      // jeune : petites ailes qui battent de temps en temps
    float bat = (fmodf(tAnim * 0.37f + h01(i, 9), 1.0f) < 0.18f) ? sinf(tAnim * 26.0f) : 0.0f;
    tri(g, x - w * 0.6f, by - hh * 0.7f, x - w - 4.5f * sz, by - hh * 0.9f - 3.0f * sz * (0.4f + 0.6f * bat), x - w * 0.2f, by - hh * 0.2f, col);
    tri(g, x + w * 0.6f, by - hh * 0.7f, x + w + 4.5f * sz, by - hh * 0.9f - 3.0f * sz * (0.4f + 0.6f * bat), x + w * 0.2f, by - hh * 0.2f, col);
  }
  ell(g, x, by - hh * 0.4f, w, hh, col);
  if (!plu) for (int j = 0; j < 6; j++) { float a = -2.7f + j * 0.45f; ligne(g, x + cosf(a) * w * 0.8f, by - hh * 0.4f + sinf(a) * hh * 0.8f, x + cosf(a) * (w + 1.8f * sz), by - hh * 0.4f + sinf(a) * (hh + 1.8f * sz), clair); }      // duvet hérissé
  else ligne(g, x - w * 0.7f, by - hh * 0.9f, x + w * 0.7f, by - hh * 0.9f, clair);      // liseré sur le dos
  const float hy = by - hh * 0.9f - 2.4f * sz - 5.0f * lev * sz, hx = x;
  ligne(g, x, by - hh * 0.8f, hx, hy + 1.5f * sz, col); ligne(g, x + 0.8f, by - hh * 0.8f, hx + 0.8f, hy + 1.5f * sz, col);      // le cou
  ell(g, hx, hy, 2.8f * sz, 2.5f * sz, col);
  if (lev > 0.35f) {      // bec ouvert : la bouche rouge
    float ob = lev * 2.2f * sz;
    tri(g, hx - 1.5f * sz - ob, hy - 5.4f * sz, hx + 1.5f * sz + ob, hy - 5.4f * sz, hx, hy - 0.8f * sz, C(214, 58, 62));
    ligne(g, hx - 0.6f * sz, hy - 1.4f * sz, hx - 1.5f * sz - ob, hy - 5.4f * sz, C(176, 142, 88)); ligne(g, hx + 0.6f * sz, hy - 1.4f * sz, hx + 1.5f * sz + ob, hy - 5.4f * sz, C(176, 142, 88));
  } else { tri(g, hx - 1.2f * sz, hy - 1.0f * sz, hx + 1.2f * sz, hy - 1.0f * sz, hx, hy - 3.4f * sz, C(176, 142, 88)); }
  if (gr < 0.3f) ligne(g, hx - 1.9f * sz, hy - 0.1f, hx - 0.6f * sz, hy - 0.1f, C(10, 5, 8));      // yeux encore fermés
  else disque(g, hx - 1.2f * sz, hy - 0.2f, 0.7f, C(255, 70, 50));
}
static void nid(M5Canvas& g) {
  if (!pret || S.fph < F_NID) return;
  fourche(g);
  if (S.brins == 0) return;
  int n = (S.brins > NB_NID) ? NB_NID : S.brins;
  ligne(g, NX - 16.0f, NY + 2.0f, NX + 16.0f, NY - 0.2f, C(44, 18, 11)); ligne(g, NX - 15.0f, NY + 3.2f, NX + 15.0f, NY + 1.2f, C(32, 13, 9));
  for (int i = 0; i < n; i += 2) brin(g, i);
  if (n >= 8) ell(g, NX, NY - 8.2f, 11.0f, 2.3f, C(9, 3, 4));      // l'intérieur de la coupe
  for (int e = 0; e < S.nEuf && e < 4; e++) {      // les œufs, pâles et tachetés
    float x = NX + (e - (S.nEuf - 1) * 0.5f) * 5.6f, y = NY - 6.8f;
    ell(g, x, y, 2.8f, 3.5f, C(112, 116, 104)); ell(g, x - 0.8f, y - 1.1f, 1.1f, 1.4f, C(168, 170, 150)); disque(g, x + 1.0f, y + 1.2f, 0.5f, C(56, 52, 44));
  }
  if (S.nPou > 0 && (S.fph == F_POUSSINS || S.fph == F_JEUNES)) {
    const float gr = croiss();
    for (int i = 0; i < S.nPou && i < 3; i++) poussin(g, i, S.nPou > 3 ? 3 : S.nPou, gr, (0.28f + 0.72f * beg[i]) * (1.0f - 0.85f * couv));
  }
}
static void devant(M5Canvas& g) {
  if (!pret || S.fph < F_NID || S.brins == 0) return;
  int n = (S.brins > NB_NID) ? NB_NID : S.brins;
  for (int i = 1; i < n; i += 2) brin(g, i);
  if (n >= NB_NID) { ligne(g, NX - 15.5f, NY - 9.6f, NX - 3.0f, NY - 7.8f, C(124, 55, 32)); ligne(g, NX + 3.0f, NY - 7.8f, NX + 15.5f, NY - 9.6f, C(124, 55, 32)); }
}
static void boutBec(const Corbeau& c, float& X, float& Y, float& dx, float& dy) {      // bout du bec et direction du bec à l'écran
  float ct = cosf(c.pitch), st = sinf(c.pitch);
  float hu = 8.2f + 4.8f * cosf(c.ha), hv = -2.0f + 4.8f * sinf(c.ha), cb = cosf(c.hb), sb = sinf(c.hb);
  float u = hu + 10.0f * cb, v = hv + 10.0f * sb;
  X = c.x + c.dv * c.s * (u * ct + v * st); Y = c.y + c.bob + c.s * (-u * st + v * ct);
  dx = c.dv * (cb * ct + sb * st); dy = -cb * st + sb * ct;
}
static void vieuxPlumes(M5Canvas& g, const Corbeau& c) {      // un vieux corbeau : plumes grises et blanches à la tête et sur le dos
  const float ct = cosf(c.pitch), st = sinf(c.pitch), s = c.s, cx = c.x, cy = c.y + c.bob;
  const uint16_t g1 = C(168, 166, 176), g2 = C(104, 102, 112);
  const float hu = 8.2f + 4.8f * cosf(c.ha), hv = -2.0f + 4.8f * sinf(c.ha);
  for (int i = 0; i < 4; i++) {
    float a = -2.4f + 0.55f * i, u1 = hu + cosf(a) * 2.6f, v1 = hv + sinf(a) * 2.6f, u2 = hu + cosf(a) * 5.0f, v2 = hv + sinf(a) * 5.0f;
    ligne(g, cx + c.dv * s * (u1 * ct + v1 * st), cy + s * (-u1 * st + v1 * ct), cx + c.dv * s * (u2 * ct + v2 * st), cy + s * (-u2 * st + v2 * ct), g1);
  }
  for (int i = 0; i < 5; i++) {
    float u1 = -3.0f + 2.6f * i, v1 = -4.2f, u2 = u1 - 3.2f, v2 = -7.4f - 0.5f * (float)(i & 1);
    ligne(g, cx + c.dv * s * (u1 * ct + v1 * st), cy + s * (-u1 * st + v1 * ct), cx + c.dv * s * (u2 * ct + v2 * st), cy + s * (-u2 * st + v2 * ct), (i & 1) ? g2 : g1);
  }
}
static void sur(M5Canvas& g, int k) {      // ce que le corbeau k porte au bec : une brindille, un morceau de nourriture, un morceau offert
  if (!pret || k < 0 || k >= NM || !cr[k].visible) return;
  const Corbeau& c = cr[k]; float X, Y, dx, dy; boutBec(c, X, Y, dx, dy); const float s = c.s;
  if (vieux(k)) vieuxPlumes(g, c);
  if (mis[k].porte == 1) {
    float cs = cosf(0.35f), sn = sinf(0.35f), px = -dy, py = dx, tx = dx * cs + px * sn, ty = dy * cs + py * sn, cx = X - dx * 3.0f * s, cy = Y - dy * 3.0f * s;
    ligne(g, cx - tx * 6.0f * s, cy - ty * 6.0f * s, cx + tx * 9.0f * s, cy + ty * 9.0f * s, C(96, 44, 22)); ligne(g, cx - tx * 6.0f * s, cy - ty * 6.0f * s + 0.8f, cx + tx * 9.0f * s, cy + ty * 9.0f * s + 0.8f, C(60, 26, 14));
  } else if (mis[k].porte == 2) {
    float mx = X - dx * 1.5f * s, my = Y - dy * 1.5f * s;
    if (mis[k].ph == M_Y_DONNE && k != PC) { float X2, Y2, d2, e2; boutBec(cr[PC], X2, Y2, d2, e2); float u = lim((mis[k].t - 0.25f) / 0.45f, 0.0f, 1.0f); mx = lerpf(mx, X2, u); my = lerpf(my, Y2, u); }      // le morceau passe du bec du parent à celui du petit
    disque(g, mx, my, 1.5f * s, C(200, 54, 60)); disque(g, mx - 0.4f, my - 0.6f, 0.6f * s, C(255, 170, 170));
  }
  if (k < 2 && par.on && par.step == 2) {
    float u = lim((par.t - 2.2f) / 1.1f, 0.0f, 1.0f); bool tient = (k == 0) ? (par.t < 3.3f) : (par.t >= 3.3f && par.t < 4.6f);
    if (tient) { float mx = X, my = Y; if (u > 0.0f && u < 1.0f) { float X2, Y2, d2, e2; boutBec(cr[1 - k], X2, Y2, d2, e2); mx = lerpf(X, X2, (k == 0) ? u : 1.0f - u); my = lerpf(Y, Y2, (k == 0) ? u : 1.0f - u); }
      disque(g, mx, my - 0.5f * s, 1.3f * s, C(200, 54, 60)); disque(g, mx - 0.4f * s, my - 0.9f * s, 0.5f * s, C(255, 170, 170)); }
  }
}
static void pose(Corbeau& c, int idx, float dt) {      // attitudes des parents : lissage, partage, dépôt, couvaison, défense du nid
  if (!pret || idx < 0 || idx >= NM) return;
  Mis& m = mis[idx]; m.t += dt;
  const bool dort = (c.act == A_DORT);
  if (idx < 2 && par.on && (par.step == 1 || par.step == 2) && c.etat == E_PERCHE) {
    const Corbeau& o = cr[1 - idx]; c.d = (o.x >= c.x) ? 1 : -1; const float T = par.t;
    if (par.step == 1) {
      bool actif = (idx == 1) == (T < 4.5f);      // la femelle lisse d'abord, puis le mâle
      if (actif) { c.haC = c.ha0 + 0.12f + 0.20f * sinf(T * 8.0f); c.hbC = c.haC + 0.49f; if (((int)(T * 3.0f)) != ((int)((T - dt) * 3.0f))) c.bec = 0.45f; }
      else { c.haC = c.ha0 + 0.30f; c.hbC = c.haC + 0.49f; c.clin = 1.0f; }
    } else {
      if (idx == 0) { c.haC = c.ha0 - 0.02f; c.hbC = c.haC + 0.45f; c.pitchC = c.pr + 0.08f; }
      else { c.haC = c.ha0 - 0.30f; c.hbC = c.haC + 0.45f; if (T < 3.3f) c.bec = mxf(c.bec, 0.5f * (0.5f + 0.5f * sinf(T * 9.0f))); }
    }
    return;
  }
  const bool surNid = (c.etat == E_PERCHE && c.but == B_FAM && (m.ph == M_NID || m.ph == M_GARDE));
  float tg = (men.on && surNid) ? 1.0f : 0.0f, dx0 = men.x;
  if (soinOn() && c.etat == E_PERCHE) {      // défense du petit : le parent (ou un aidant) ailes ouvertes face à l'intrus ; le corbeau hostile fait sa parade de menace
    if (c.but == B_FAM && m.ph == M_Y_MENACE && m.t > 0.4f) { tg = 0.85f; dx0 = (hostCible == 1 && PE.par < NAD) ? cr[PE.par].x : cr[PC].x; }
    else if (ym.on && ((c.but == B_FAM && m.ph == M_Y_GARDE) || (idx == PE.par && ym.kind == 5 && hostCible == 1 && c.but != B_FAM))) { tg = yInt(idx); dx0 = ym.x; }
  }
  m.def += (tg - m.def) * mnf(1.0f, dt * 6.0f);
  if (m.ph == M_DEPOSE && c.etat == E_PERCHE) {      // il penche la tête dans le nid et y glisse la brindille, ou la nourriture
    float dip = sinf(3.1416f * lim(m.t / 1.4f, 0.0f, 1.0f)); c.haC = lerpf(c.ha0, 0.75f, dip); c.hbC = c.haC + 0.5f; c.pitchC = c.pr - 0.2f * dip;
  } else if ((m.ph == M_NID || m.ph == M_OEUF || (m.ph == M_NUIT && idx == 1)) && c.etat == E_PERCHE) {      // elle couve (ou il couve l'œuf attrapé, ou elle dort dans la coupe) : corps bas dans la coupe, de temps en temps elle retourne les œufs du bec
    c.pitchC = c.pr * (croiss() >= 0.35f ? 0.88f : 0.62f); c.bob += (croiss() >= 0.35f ? 0.6f : 1.6f) * c.s;
    if (!dort) {
      float ph = fmodf(tAnim * 0.083f + idx * 0.37f, 1.0f);
      if (ph < 0.12f) { float dip = sinf(3.1416f * ph / 0.12f); c.haC = lerpf(c.ha0, 0.95f, dip); c.hbC = c.haC + 0.5f; }
      if (layT > 0.0f) { float e = sinf(3.1416f * lim((1.5f - layT) / 1.5f, 0.0f, 1.0f)); c.ruf = 0.6f * e; c.pitchC -= 0.12f * e; c.bec = mxf(c.bec, 0.4f * e); }
    }
  }
  if (vieux(idx) && c.etat == E_PERCHE && !dort) { c.pitchC += 0.08f; c.bob += 0.35f * c.s; }      // un vieux corbeau : le dos voûté
  if (idx == PC && soinOn()) {      // le petit : réclame (tête levée, bec ouvert) quand il a faim, se fait tout petit quand on menace
    float bg = lim((faimP - 0.5f) * 2.2f, 0.0f, 1.0f);
    if (ym.on) { c.pitchC = c.pr * 0.82f; c.bob += 0.8f * c.s; }
    else if (bg > 0.02f && !dort && c.etat == E_PERCHE) { c.haC = c.ha0 - 0.22f * bg; c.hbC = c.haC + 0.49f; c.bec = mxf(c.bec, bg * (0.45f + 0.55f * sinf(tAnim * 12.0f))); c.pitchC = c.pr - 0.08f * bg; }
  }
  if (m.ph == M_Y_DONNE && c.etat == E_PERCHE && idx < NAD) {      // il penche la tête vers le bec du petit et lui passe le morceau
    float dip = sinf(3.1416f * lim(m.t / 1.6f, 0.0f, 1.0f)); c.haC = lerpf(c.ha0, 0.5f, dip); c.hbC = c.haC + 0.45f; c.pitchC = c.pr - 0.12f * dip;
  }
  if (m.def > 0.02f && !dort) {      // défense : ailes à demi ouvertes arquées vers l'avant, plumes de la gorge hérissées, bec ouvert, face à l'intrus
    c.d = (dx0 >= c.x) ? 1 : -1; c.wlC = 0.9f * m.def; c.poseC = mixA(A_PLANE, A_FLARE, 0.55f * m.def); c.fanC = m.def; c.ruf = m.def;
    c.pitchC = c.pr - 0.18f * m.def; c.haC = c.ha0 - 0.12f * m.def; c.hbC = c.haC + 0.45f; c.bec = mxf(c.bec, 0.55f * m.def * (0.5f + 0.5f * sinf(tAnim * 12.0f)));
  }
}
static bool enService(const Corbeau& c) {      // un parent qui a des œufs ou des petits ne rejoint ni la nuée du visiteur ni l'appel
  int k = (int)(&c - cr); return pret && k >= 0 && ((k < 2 && nichee()) || ((k == PE.par || k == PC) && soinOn()) || (cvOn && k == (int)cvK));
}
static void majFam(float dt) {      // à chaque image : poussins qui réclament, couverts la nuit, ponte
  if (layT > 0.0f) { layT -= dt; if (layT <= 0.0f && S.nEuf < noeufs()) { S.nEuf++; sale = true; son(SN_POP_APP, 80); plumes(NX, NY - 6.0f, 2); } }      // l'œuf apparaît dans le nid
  const bool petits = (S.nPou > 0 && (S.fph == F_POUSSINS || S.fph == F_JEUNES));
  float tc = 0.0f;
  if (petits && auNid(1) && cr[1].act == A_DORT) tc = 1.0f;      // la nuit, elle les couvre
  else if (petits && auNid(1) && croiss() < 0.35f && !men.on) tc = 0.55f;      // tout petits : surtout sous elle
  couv += (tc - couv) * mnf(1.0f, dt * 3.0f);
  if (!petits) return;
  const bool visite = (mis[0].ph == M_DEPOSE && mis[0].porte == 2) || (mis[1].ph == M_DEPOSE && mis[1].porte == 2);
  for (int i = 0; i < 3 && i < S.nPou; i++) {
    faim[i] += dt / (16.0f + 6.0f * h01(i, 5)); if (faim[i] > 1.0f) faim[i] = 1.0f;
    float t = (faim[i] > 0.55f || visite) ? 1.0f : 0.0f; if (men.on) t = 0.0f;
    if (t > beg[i] && beg[i] < 0.2f && tCri <= 0.0f && random(100) < 8) { son(SN_GAZ, 95); tCri = 1.5f; }
    beg[i] += (t - beg[i]) * mnf(1.0f, dt * 5.0f);
  }
}

// ======================= BLOC Y (3.1.11) : parent attitré, entraide, protection, nourriture rare =======================
// Le petit corbeau de l'œuf (cr[PC]) a un parent attitré (PE.par), mémorisé (clé cbpet). Il le nourrit, le garde, veilleY sur lui la nuit et l'encourage à voler jusqu'à ce qu'il soit grand (stade 3).
// Les autres corbeaux aident (aident à nourrir et à garder), ignorent ou menacent le petit selon leur caractère et leur lien avec le parent.
// Les missions réutilisent celles du nid (mis[], versPoint, arrive, pose) : phases M_Y_*. Tout est coupé par le réglage « Famille » (RG.on).
static bool paisible() { return V.ph == V_RIEN && !GP.ph && !appelOn && !panneau && tAlerte <= 0.0f && !H.actif; }
static bool libreY(int k) { return k >= 0 && k < NAD && k != PC && parentLibre(k) && mis[k].ph == M_RIEN && !(k < 2 && (precieux() || par.on)); }
static int attitudeY(int k) {      // 0 aide le petit, 1 l'ignore, 2 le menace : sociabilité, calme, force, lien avec le parent (amis et couple : plus volontiers) et audace
  if (k < 0 || k >= NAD || k == PE.par || RG.aide == 0 || cr[k].etat == E_OFF) return 1;
  if (RG.aide == 2) return 0;
  int P = (PE.par < NAD) ? (int)PE.par : -1, l = (P >= 0) ? (int)lien(P, k) : 0;
  int b = (int)S.soc[k] + (int)S.cal[k] / 2 + (255 - (int)S.frc[k]) / 4 + 2 * l;
  if (b >= 290) return 0;
  if (b < 190 && S.aud[k] >= 130) return 2;
  return 1;
}
static void rageSet(int k, bool on) {      // un parent dont le petit a faim se bat plus fort pour un morceau : sa force monte d'un cran tant que dure la faim
  if (k < 0 || k >= NAD || rage[k] == on) return;
  rage[k] = on; int v = (int)mo::forceK[k] + (on ? 1 : -1); if (v < 0) v = 0; if (v > 4) v = 4; mo::forceK[k] = (int8_t)v;
}
static int meilleurParent() {      // adoption : à défaut de parent désigné (ou s'il n'est plus là), l'adulte le plus bienveillant
  int b = -1, sc = -1;
  for (int k = 0; k < NAD; k++) { if (cr[k].etat == E_OFF) continue; int v = (int)S.soc[k] + (int)S.cal[k] / 2 + (k < 2 ? 40 : 0) + (int)random(30); if (v > sc) { sc = v; b = k; } }
  return b;
}
static void poserPetit(float x) {      // le petit est posé sur la grosse branche (c'est là que ses parents viennent le nourrir)
  Corbeau& c = cr[PC]; x = lim(x, 84.0f, 226.0f);
  c.perch = -1; c.fx = x; c.fy = ytop(x) + 0.4f; c.fp = (ytop(x + 4.0f) - ytop(x - 4.0f)) / 8.0f; c.pied = true; c.vx = c.vy = 0.0f; c.etat = E_PERCHE; c.but = B_RIEN; c.sub = 0; c.ox = c.oy = 0.0f;
  c.pitch = c.pitchC = c.pr; c.pv = 0.0f; c.dv = (float)c.d; c.visible = true;
  float hx, hy; hancheC(c.s, c.pitch, c.dv, hx, hy); c.x = c.fx - hx; c.y = c.fy - JAMBE * c.s - hy;
}
static float xPetitLibre() {      // un point de la branche entre deux perchoirs, le plus loin des autres corbeaux
  static const float XS[3] = {116.0f, 160.0f, 203.0f}; int best = 1; float bd = -1.0f;
  for (int i = 0; i < 3; i++) { float d = 1e9f; for (int k = 0; k < NAD; k++) if (cr[k].etat == E_PERCHE && cr[k].visible) { float q = fabsf(cr[k].fx - XS[i]); if (q < d) d = q; } if (d > bd) { bd = d; best = i; } }
  return XS[best];
}
static void debutPetit(bool rare) {      // appelé par naissance() : le petit a un parent attitré (celui qui a attrapé l'œuf, l'a pondu ou l'a élevé dans le nid), sinon un adoptant
  if (!pret || restau) return;
  int p = pNext; pNext = -1;
  if (!RG.on) { PE.stade = 0; return; }
  if (p < 0 || p >= NAD || cr[p].etat == E_OFF) p = meilleurParent();
  PE.ver = 1; PE.stade = 1; PE.par = (p >= 0) ? (uint8_t)p : 255; PE.rare = rare ? 1 : 0; PE.nour = 0; PE.age = 0; PE.res = 0; saleP = true;
  faimP = 0.35f; tSoin = 8.0f; ym.on = false; hostY = -1;
  poserPetit(xPetitLibre());
  if (p >= 0) changer(p, PC, 40);
  sauverY(true);
}
static void restaurerY() {      // à l'ouverture du mode : le petit (ou le jeune devenu grand, s'il y a de la place) revient, avec son âge et son parent
  if (!pret || !RG.on || PE.stade == 0 || cr[PC].etat != E_OFF) return;
  if (PE.stade == 3) {      // le petit adulte est déjà dans la colonie (promu, N + 1 mémorisé) : il n'est jamais recréé en plus
    if (AFR.naiss == 0) { if (PE.res != 1) { memset(&PE, 0, sizeof(PE)); saleP = true; sauverY(true); } return; }
    if (nAct >= NAD) return;
  }
  restau = true; naissance(120.0f, 140.0f, PE.rare != 0); restau = false;
  agePous = (PE.stade == 3) ? (Y_GRAND + 5.0f) : (float)PE.age;
  poserPetit(xPetitLibre()); faimP = 0.45f; tSoin = 10.0f;
  if (PE.par < NAD && cr[PE.par].etat == E_OFF) PE.par = 255;
}
static bool immobile(const Corbeau& c) { return (int)(&c - cr) == PC && soinOn() && c.etat == E_PERCHE; }      // le petit ne s'éloigne pas de la branche de ses parents
static void rattrapageY(uint32_t secs) { if (soinOn() && secs > 40UL) faimP = mxf(faimP, 0.7f); if (ADU.ver == 1) { ageAdu = mnf(ageAdu + (float)secs * 0.3f, 65000.0f); saleAdu = true; } }      // au retour de l'utilisateur : le petit réclame
static int prix(int t) { return t == T_VIANDE ? 5 : (t == T_PAIN ? 4 : (t == T_NOIX ? 3 : (t == T_BAIES ? 2 : 1))); }
static bool defier(int P, int iO) {      // il y a peu de nourriture : le parent défie celui qui tient le morceau (la bagarre est celle du bloc J : ailes ouvertes, cris, choc, le plus fort l'emporte)
  Obj& o = ob[iO]; int W = o.reserve; Corbeau& c = cr[P]; mo::Rt& r = mo::rt[P];
  if (!o.actif || W < 0 || W >= NAD || W == P || r.away || mo::nbAilleurs() >= 3) return false;
  if (!(cr[W].but == B_NOUR && cr[W].etat == E_SOL && cr[W].item == iO)) return false;
  int sw = (cr[W].x < o.x) ? -1 : 1; float tx = lim(o.x - sw * 24.0f * c.s, 12.0f, 308.0f), ty = solHaut(tx);
  r.sB = c.s; r.hops = 0; r.away = true; r.dest = mo::D_DISPUTE; r.spot = 0; r.ph = 0; r.t = 0.0f; r.tx = tx; r.n = (uint8_t)iO; r.dur = 0.0f; r.tw = 0.0f;
  c.perch = -1; c.item = -1; c.rapporte = false; decoller(c, tx, ty, B_MONDE);
  tBat = 22.0f; c.hum = 1.0f; son(SN_CROASSE2, 190);
  return true;
}
static bool chercher(int k) {      // k va chercher un morceau pour le petit : la meilleure nourriture posée (ou la disputer s'il y en a peu), sinon il cherche par terre
  Corbeau& c = cr[k]; int iO = -1, n = 0, sc = 0;
  for (int i = 0; i < NOB; i++) if (ob[i].actif && ob[i].repos && ob[i].type < T_BRILLANT) { n++; int v = prix(ob[i].type); if (v > sc) { sc = v; iO = i; } }
  if (iO < 0) { float gx = rnd(110.0f, 300.0f); versPoint(k, gx, solHaut(gx), M_VERS_BRIN); mis[k].kind = 3; mis[k].porte = 0; return true; }
  Obj& o = ob[iO]; int W = o.reserve;
  if (W < 0 || W == k) {
    int sg = (o.x >= c.x) ? 1 : -1; float gx = lim(o.x - sg * 19.0f * c.s, 10.0f, 310.0f);
    c.perch = -1; c.item = iO; o.reserve = (int8_t)k; c.objet = 0; c.rapporte = false; decoller(c, gx, solHaut(gx), B_NOUR); c.item = iO; return true;
  }
  if (n <= 2 && tBat <= 0.0f && k == PE.par) return defier(k, iO);
  return false;
}
static bool detourner(Corbeau& c, int idx) {      // appelé quand un corbeau va manger : s'il est le parent (ou un aidant) et que le petit a faim, il garde le morceau pour le petit
  if (!soinOn() || idx < 0 || idx >= NAD || mis[idx].ph != M_RIEN || faimP < 0.28f || (idx < 2 && (precieux() || par.on))) return false;
  if (idx != PE.par && (attitudeY(idx) != 0 || faimP < 0.55f || random(100) < 40)) return false;
  float tx = coteY(c.x < cr[PC].fx ? -17.0f : 17.0f);
  versPoint(idx, tx, ytop(tx) + 0.4f, M_Y_VERS); mis[idx].porte = 2; mis[idx].kind = 3; c.hum = 1.0f; son(SN_GAZ, 120);
  return true;
}
static void donner(int k) {      // le morceau passe du bec du parent au bec du petit
  Corbeau& pc = cr[PC]; mis[k].porte = 0; pc.bec = 1.0f; pc.flash = 0.35f; cr[k].bec = 0.6f; son(SN_GAZ, 150);
  faimP -= (k == PE.par) ? 0.85f : 0.6f; if (faimP < 0.0f) faimP = 0.0f;
  if (PE.nour < 250) PE.nour++; saleP = true;
  changer(k, PC, (k == PE.par) ? 2 : 3); if (k != PE.par && PE.par < NAD) changer(PE.par, k, 2);
  tSoin = rnd(10.0f, 16.0f);
}
static void veilleY(int k, float dur) {      // il vient se poser à côté du petit (la nuit : jusqu'au réveil du groupe)
  float tx = coteY(cr[k].x < cr[PC].fx ? -17.0f : 17.0f);
  versPoint(k, tx, ytop(tx) + 0.4f, M_Y_VEILLE); mis[k].dur = dur;
}
static void essai(int P) {      // le jeune apprend à voler : un saut ailes battantes le long de la branche, son parent l'encourage du bec
  Corbeau& pc = cr[PC]; float amp = 14.0f + 40.0f * lim((agePous - Y_JEUNE) / (Y_GRAND - Y_JEUNE), 0.0f, 1.0f);
  float x = pc.fx + (random(2) ? amp : -amp); if (x < 90.0f || x > 220.0f) x = pc.fx - (x - pc.fx);
  x = lim(x, 90.0f, 220.0f);
  versPoint(PC, x, ytop(x) + 0.4f, M_Y_ESSAI); tEssai = rnd(35.0f, 60.0f);
  if (P >= 0 && P < NAD && cr[P].etat == E_PERCHE) { cr[P].d = (x >= cr[P].x) ? 1 : -1; cr[P].bec = 1.0f; cr[P].look = -0.1f; cr[P].tLook = 1.5f; }
  son(SN_AILES, 120);
}
// ======================= BLOC AF, partie 2 (3.1.19) : œuf attrapé et couvé, petit devenu adulte =======================
static bool libreOeuf(int k) { return k >= 0 && k < NAD && parentLibre(k) && mis[k].ph == M_RIEN && !(k < 2 && (precieux() || par.on || RLV.ph)) && !(k == (int)PE.par && soinOn()); }
static int choisirCouveur() {      // le corbeau qui a pondu l'œuf (s'il est libre), sinon le plus proche
  if (pondeur >= 0 && libreOeuf(pondeur)) return pondeur;
  int b = -1; float bd = 1e9f;
  for (int k = 0; k < NAD; k++) { if (!libreOeuf(k)) continue; float d = fabsf(cr[k].x - OE.x); if (d < bd) { bd = d; b = k; } }
  return b;
}
static bool oeufCouve() { return cvOn; }
static void annulerCouve() {
  if (cvK >= 0 && mis[cvK].ph == M_OEUF) finMission(cvK);
  cvOn = false; cvK = -1; tCv = 0.0f; tCvAbs = 0.0f; if (OE.actif && OE.etat == 2) OE.etat = 0;
}
static bool attraperOeuf() {      // un toucher sur l'œuf posé : il est attrapé et couvé par le pondeur (ou le plus proche) ; refusé tant qu'il n'y a pas de place pour un petit de plus
  if (!afOn() || !OE.actif || OE.etat != 0) return false;
  if (!placeLibre()) { son(SN_POP_APP, 90); return true; }
  int k = choisirCouveur(); if (k < 0) { son(SN_POP_APP, 90); return true; }
  cvK = (int8_t)k; cvOn = true; tCv = 0.0f; tCvAbs = 0.0f; OE.etat = 2; OE.t = 0.0f;
  versPoint(k, OE.x, ytop(OE.x) + 0.4f, M_OEUF); cr[k].bec = 1.0f; son(SN_CROC, 200);
  return true;
}
static float dureeCouve() { static const float VF[3] = {1.5f, 1.0f, 0.55f}; return 24.0f * VF[RG.vit > 2 ? 1 : RG.vit]; }      // couvaison de l'œuf attrapé (selon la vitesse de vie)
static void couveTick(float h) {      // l'œuf attrapé est couvé ; si le couveur est dérangé, un autre prend le relais ou l'œuf attend
  if (!cvOn) return;
  if (!OE.actif || OE.etat == 0) { annulerCouve(); return; }
  if (OE.etat == 1) return;      // il craque : eclore() (corbeau.h) s'en charge
  const int k = cvK; const bool assis = k >= 0 && cr[k].etat == E_PERCHE && cr[k].but == B_FAM && mis[k].ph == M_OEUF && mis[k].t > 1.0f;
  if (assis) {
    tCvAbs = 0.0f; tCv += h;
    if (tCv > dureeCouve()) { OE.etat = 1; OE.tc = 0.0f; son(SN_CROC, 230); cr[k].bec = 1.0f; }
    return;
  }
  const bool enRoute = k >= 0 && cr[k].etat != E_OFF && cr[k].but == B_FAM && mis[k].ph == M_OEUF;
  if (enRoute) { tCvAbs = 0.0f; return; }
  tCvAbs += h;
  if (tCvAbs > 1.5f) {
    if (k >= 0 && mis[k].ph == M_OEUF) mis[k].ph = M_RIEN;
    int n = choisirCouveur();
    if (n >= 0) { cvK = (int8_t)n; versPoint(n, OE.x, ytop(OE.x) + 0.4f, M_OEUF); tCvAbs = 0.0f; }
    else if (tCvAbs > 40.0f) annulerCouve();
  }
}
static void apresEclosion() {      // appelé par eclore() : le couveur quitte l'œuf, le petit est né
  if (!cvOn) return;
  int k = cvK; cvOn = false; cvK = -1; tCv = 0.0f; tCvAbs = 0.0f;
  if (k >= 0 && mis[k].ph == M_OEUF) finMission(k);
}
static void promouvoir() {      // le petit est adulte : il prend sa place dans la colonie (N + 1, mémorisé) ; si la colonie est pleine il reste sur l'emplacement du petit
  if (AFR.naiss != 0 || !RG.on || nAct >= NAD || nAct < 1 || cr[PC].etat == E_OFF) return;
  const int k = nAct; Corbeau& c = cr[PC];
  cr[k] = c; Corbeau& n = cr[k];
  mo::rt[k] = mo::rt[PC]; mo::sBase[k] = c.s; mo::rt[k].sB = c.s;
  mis[k] = mis[PC]; memset(&mis[PC], 0, sizeof(Mis)); tmis[k] = 0.0f; tmis[PC] = 0.0f;
  n.duvet = 0.0f;
  c.etat = E_OFF; c.visible = false; c.perch = -1; c.but = B_RIEN; c.objet = 0; c.item = -1; c.pj = -1; c.duvet = 0.0f;
  OP.nb = (uint8_t)(k + 1); nAct = OP.nb; prefs.putUChar("cbnb", OP.nb);
  appliquer(n, k); PE.res = 1; saleP = true;
  ADU.ver = 1; ADU.k = (uint8_t)k; ageAdu = Y_GRAND; saleAdu = true; sauverAdu(true);
  if (n.etat == E_PERCHE && n.but == B_RIEN) { n.perch = -1; n.tfin = rnd(1.0f, 2.5f); versPerchoir(n); }      // il rejoint son perchoir
}
static void devenuGrand() {      // il est grand : il quitte le giron et rejoint la colonie (l'état est mémorisé)
  PE.stade = 3; saleP = true; Corbeau& c = cr[PC]; int P = PE.par;
  c.bec = 1.0f; c.flash = 0.6f; son(SN_AILES, 170); son(SN_GAZ, 120); plumes(c.x, c.y - 4.0f, 8); eclat(c.x, c.y - 6.0f, 0.3f, 1.2f);
  if (P >= 0 && P < NAD) { if (cr[P].etat == E_PERCHE) { cr[P].bec = 1.0f; cr[P].d = (c.x >= cr[P].x) ? 1 : -1; sonCri(); } changer(P, PC, 10); }
  rageSet(P, false); promouvoir(); sauverY(true);
}
static void finHost(int k) {      // le menaçant s'en va (ou a été chassé) : le lien avec le parent se refroidit
  if (PE.par < NAD && k >= 0 && k < NAD) changer(PE.par, k, -7);
  hostY = -1; hostCible = 0; tHost = rnd(80.0f, 140.0f);
}
static void missionsY(float h) {      // avancement des missions Y (chaque phase se termine ou est annulée si le parent, le petit ou le réglage change)
  for (int k = 0; k < NM; k++) {
    Mis& m = mis[k]; if (m.ph < M_Y_VERS) continue;
    Corbeau& c = cr[k]; tmis[k] += h;
    if (c.etat == E_OFF || c.but != B_FAM || !soinOn() || tmis[k] > 45.0f) {
      bool etait = (m.ph == M_Y_MENACE); m.ph = M_RIEN; m.porte = 0; m.kind = 0; tmis[k] = 0.0f;
      if (c.etat != E_OFF && c.but == B_FAM) { if (k == PC) { c.but = B_RIEN; c.tfin = 4.0f; } else finMission(k); }
      if (etait && hostY == k) finHost(k);
      if (hostY == k && !etait) hostY = -1;
      continue;
    }
    switch (m.ph) {
      case M_Y_DONNE: if (c.etat == E_PERCHE) { if (m.porte == 2 && m.t > 0.8f) donner(k); if (m.t > 1.7f) finMission(k); } break;
      case M_Y_GARDE: if (!ym.on) { m.tz += h; if (m.tz > 1.5f) finMission(k); } else m.tz = 0.0f; break;
      case M_Y_VEILLE: if (c.etat == E_PERCHE) { if (m.dur < 1e8f) { if (m.t > m.dur) finMission(k); } else if (repos <= 0.0f) finMission(k); } break;
      case M_Y_MENACE: if (c.etat == E_PERCHE && m.t > 3.8f) { finHost(k); finMission(k); } break;
      default: break;
    }
  }
}
static void detecterMenaceY(float h) {      // une menace près du petit : visiteur, main, objet lancé, corbeau hostile en vol ou qui vient le menacer
  const Corbeau& pc = cr[PC]; bool on = false; uint8_t kind = 0; int8_t qui = -1; float x = pc.x, y = pc.y;
  if (V.ph != V_RIEN) { on = true; kind = 1; x = VX; y = 92.0f; }
  else if (H.actif && hypotf(H.x - pc.x, H.y - pc.y) < 120.0f) { on = true; kind = 2; x = H.x; y = H.y; }
  else if (hostY >= 0 && mis[hostY].ph == M_Y_MENACE && cr[hostY].etat == E_PERCHE && mis[hostY].t > 0.4f) { on = true; kind = 5; qui = (int8_t)hostY; x = cr[hostY].x; y = cr[hostY].y; }
  else {
    for (int i = 0; i < NPROJ && !on; i++) if (proj[i].actif && hypotf(proj[i].x - pc.x, proj[i].y - pc.y) < 60.0f) { on = true; kind = 4; x = proj[i].x; y = proj[i].y; }
    for (int k = 0; k < NAD && !on; k++) {
      const Corbeau& c = cr[k]; if (k == PE.par || !c.visible || c.but == B_ORBITE || c.but == B_FAM || attitudeY(k) != 2) continue;
      if ((c.etat == E_VOL || c.etat == E_DECOLLE || c.etat == E_ATTERRIT) && hypotf(c.x - pc.x, c.y - pc.y) < 34.0f) { on = true; kind = 3; qui = (int8_t)k; x = c.x; y = c.y; }
    }
  }
  tMenP -= h;
  if (on) {
    bool debut = !ym.on; ym.on = true; ym.kind = kind; ym.qui = qui; ym.x = x; ym.y = y; ym.tOff = 0.0f;
    if (debut && tMenP <= 0.0f) { tMenP = 12.0f; if (!men.on && kind <= 2) { tAlerte = mxf(tAlerte, 3.0f); dirAlerte = (x >= pc.x) ? 1 : -1; alerteFuite = false; } }
    if (tCriY <= 0.0f) { tCriY = 2.4f; if (PE.par < NAD) cr[PE.par].bec = 1.0f; son(SN_ALARME, 160); }
  } else if (ym.on) { ym.tOff += h; if (ym.tOff > 3.0f) ym.on = false; }
}
static void protegerPetit(int P) {      // le parent s'interpose ailes ouvertes (le caractère règle l'intensité) et chasse l'intrus ; les aidants le rejoignent
  Corbeau& c = cr[P];
  if (libreY(P) && !(ym.kind == 5 && hostCible == 1)) { float tx = coteY(c.x < cr[PC].fx ? -15.0f : 15.0f); versPoint(P, tx, ytop(tx) + 0.4f, M_Y_GARDE); }
  if (ym.kind >= 3 && ym.qui >= 0 && ym.qui < NAD && tChaY <= 0.0f && (RG.prot == 2 || (RG.prot == 1 && S.aud[P] > 100)) && (c.etat == E_PERCHE || c.etat == E_SOL)) {
    Corbeau& v = cr[ym.qui]; tChaY = 25.0f; mis[P].ph = M_RIEN; mis[P].porte = 0;
    c.perch = -1; c.pj = ym.qui; c.tOrb = rnd(3.5f, 5.0f); c.hum = 1.0f; decoller(c, v.x, v.y, B_POURSUITE); sonCri(); chasser(v);
  }
  int ng = 0; for (int k = 0; k < NAD; k++) if (mis[k].ph == M_Y_GARDE) ng++;
  for (int k = 0; k < NAD && ng < 2; k++) if (k != P && attitudeY(k) == 0 && libreY(k) && random(100) < 30) { float tx = coteY(cr[k].x < cr[PC].fx ? 14.0f : -14.0f); versPoint(k, tx, ytop(tx) + 0.4f, M_Y_GARDE); ng++; }
}
static void soinTick(float h) {      // une tranche de 0,25 s : faim du petit, repas, veilleY, protection, entraide, croissance
  if (!pret) return;
  tBat -= h; tSoin -= h; tAide -= h; tCriY -= h; tChaY -= h; tHost -= h * actFreq; tProche -= h; tCriP -= h; tEssai -= h * actFreq; tSurv -= h; tSauvP += h;
  if (!OE.actif && OE.ponte < 0) pondeur = -1;
  if (RG.on && OE.actif && pondeur >= 0 && pondeur < NAD && tSurv <= 0.0f) {      // celui qui a pondu surveille l'œuf : il devient le parent attitré s'il l'ouvre
    tSurv = rnd(5.0f, 9.0f); Corbeau& c = cr[pondeur];
    if (c.etat == E_PERCHE && c.act <= A_REGARDE && c.but == B_RIEN) { c.d = (OE.x >= c.x) ? 1 : -1; c.look = lim(atan2f(OE.y - c.y, fabsf(OE.x - c.x) + 8.0f) * 0.7f, -0.4f, 0.8f); c.tLook = 2.0f; if (OE.etat == 1) c.bec = 0.6f; }
  }
  if (RG.on && PE.stade >= 1 && PE.stade <= 2 && cr[PC].etat != E_OFF) {      // croissance : le réglage accélère ou ralentit l'âge (corbeau.h l'avance d'une seconde par seconde)
    static const float VF[3] = {0.6f, 1.0f, 1.8f}; agePous += h * (VF[RG.vit > 2 ? 1 : RG.vit] - 1.0f);
    if (PE.stade == 1 && agePous >= Y_JEUNE) { PE.stade = 2; saleP = true; cr[PC].bec = 1.0f; son(SN_GAZ, 140); sauverY(true); }
    else if (PE.stade == 2 && agePous >= Y_GRAND) devenuGrand();
  }
  missionsY(h);
  if (!soinOn()) { if (ym.on) ym.on = false; if (hostY >= 0 && mis[hostY].ph < M_Y_VERS) hostY = -1; faimP = 0.4f; sauverY(false); return; }
  const bool absent = cbAbsent; Corbeau& pc = cr[PC]; int P = PE.par;
  if (P >= NAD || cr[P].etat == E_OFF) {      // adoption : le parent n'est plus là
    int n = meilleurParent(); if (n < 0) return;
    PE.par = (uint8_t)n; P = n; saleP = true; changer(n, PC, 25);
  }
  faimP += h / (PE.stade == 1 ? 26.0f : 42.0f) * (absent ? 0.25f : 1.0f); if (faimP > 1.0f) faimP = 1.0f; if (absent && faimP > 0.8f) faimP = 0.8f;
  rageSet(P, faimP > 0.5f);
  if (!absent && faimP > 0.6f && tCriP <= 0.0f && !ym.on && pc.act != A_DORT) { tCriP = rnd(5.0f, 9.0f); pc.bec = 1.0f; son(SN_GAZ, 100); }      // il réclame
  if (repos > 0.0f && pc.etat == E_PERCHE && pc.act != A_DORT && pc.act <= A_REGARDE) { pc.act = A_DORT; pc.tAct = 0.0f; pc.dAct = repos + 1.0f; }      // il dort avec les autres
  detecterMenaceY(h);
  sauverY(false);
  if (absent) return;
  if (ym.on) { protegerPetit(P); return; }
  if (!paisible()) return;
  if (faimP > 0.55f && tSoin <= 0.0f && libreY(P)) { tSoin = chercher(P) ? 14.0f : 4.0f; }
  else if (faimP > 0.75f && tAide <= 0.0f && RG.aide > 0) {      // le parent est pris ou tarde : un aidant va chercher à manger pour le petit
    tAide = rnd(18.0f, 32.0f);
    if (!libreY(P) || random(100) < 35) for (int k = 0; k < NAD; k++) if (attitudeY(k) == 0 && libreY(k) && cr[k].duvet < 0.5f) { chercher(k); break; }
  }
  else if (repos > 0.0f && libreY(P)) veilleY(P, 1e9f);
  else if (libreY(P) && tProche <= 0.0f && fabsf(cr[P].x - pc.x) > 50.0f) { tProche = rnd(25.0f, 45.0f); veilleY(P, rnd(20.0f, 40.0f)); }
  else if (PE.stade == 2 && tEssai <= 0.0f && pc.etat == E_PERCHE && pc.act <= A_REGARDE && mis[PC].ph == M_RIEN && !(P < 2 && precieux())) essai(P);
  else if (RG.aide == 1 && hostY < 0 && tHost <= 0.0f && mis[PC].ph == M_RIEN) {      // un corbeau hostile vient menacer le petit (ou son parent) : le parent défend
    tHost = rnd(80.0f, 140.0f); int k = -1, d0 = random(NAD);
    for (int j = 0; j < NAD && k < 0; j++) { int i = (d0 + j) % NAD; if (attitudeY(i) == 2 && libreY(i) && cr[i].duvet < 0.5f) k = i; }
    if (k >= 0) {
      hostCible = (random(100) < 35 && P < NAD && cr[P].etat == E_PERCHE && cr[P].perch >= 0 && cr[P].perch < 4) ? 1 : 0;
      float bx = (hostCible == 1) ? cr[P].fx : cr[PC].fx, tx = lim(bx + ((bx < 150.0f) ? 21.0f : -21.0f), 84.0f, 226.0f);
      versPoint(k, tx, ytop(tx) + 0.4f, M_Y_MENACE); hostY = (int8_t)k; cr[k].bec = 1.0f; sonCri();
    }
  }
}
static void texteLien(int k, char* b, int n, const char (*noms)[9]) {      // pour la fiche d'un corbeau (double toucher) : son lien avec le petit (parent, aidant, hostile, petit)
  if (n < 4) return; b[0] = 0;
  if (!pret || !RG.on || k < 0 || k >= NCB) return;
  const int P = (PE.par < NAD) ? (int)PE.par : -1;
  if (ADU.ver == 1 && k == (int)ADU.k && cr[k].etat != E_OFF) {      // le dernier petit devenu adulte : son âge de vie
    if (PE.res == 1 && P >= 0) snprintf(b, n, "%s, élevé par %s", nomVie(vieDe(ageAdu)), noms[P]); else snprintf(b, n, "%s", nomVie(vieDe(ageAdu)));
    return;
  }
  if (PE.stade == 0 || cr[PC].etat == E_OFF) return;
  if (k == PC) { const char* st = nomVie(vieDe(agePous)); if (P >= 0) snprintf(b, n, "%s %s %s", st, PE.stade == 3 ? "par" : "de", noms[P]); else snprintf(b, n, "%s", st); }
  else if (k == P) snprintf(b, n, "%s %s", PE.stade < 3 ? "Parent de" : "A élevé", noms[PC]);
  else if (k < NAD && PE.stade < 3) { int a = attitudeY(k); snprintf(b, n, "%s %s", a == 0 ? "Aide" : (a == 2 ? "Menace" : "Ignore"), noms[PC]); }
}

// ---------- une tranche de 0,25 s ----------
static void tick(float h) {
  bool v = (V.ph != V_RIEN);
  if (v && !vuV) choeur(VX, 90.0f, 3, true);
  vuV = v;
  bool hh = H.actif;
  if (hh && !vuH) { choeur(H.x, H.y, 2, true); mobbing(H.x, H.y); }
  vuH = hh;
  bool p = false; float px = 160.0f, py = 80.0f; for (int i = 0; i < NPROJ; i++) if (proj[i].actif) { p = true; px = proj[i].x; py = proj[i].y; }
  if (p && !vuP) choeur(px, py, 0, true);
  vuP = p;
  if (dernierSecouement != vuSec) { vuSec = dernierSecouement; if (millis() - dernierSecouement < 2000) { tRappel = 3.5f; choeur(160.0f, 40.0f, 3, false); if (repos > 0.0f) finRepos(true); } }
  if (tAlerte > 0.0f && tAlPrec <= 0.0f) choeur(160.0f, 80.0f, 3, false);
  tAlPrec = tAlerte;
  // liens : voisins qui restent côte à côte = amitié qui grandit ; appels entre eux ; poursuites = rivalité
  for (int a = 0; a < NAD; a++) {
    Corbeau& ca = cr[a]; if (ca.etat != E_PERCHE || !ca.visible || ca.perch < 0 || ca.perch >= 10) continue;
    for (int b = a + 1; b < NAD; b++) {
      Corbeau& cb2 = cr[b]; if (cb2.etat != E_PERCHE || !cb2.visible || cb2.perch < 0 || cb2.perch >= 10) continue;
      if (fabsf(ca.x - cb2.x) < 38.0f && fabsf(ca.y - cb2.y) < 22.0f && random(100) < 5 + (S.soc[a] + S.soc[b]) / 64) changer(a, b, 1);
    }
  }
  if (AP.n > 0) changer(AP.a, AP.b, 2);
  for (int k = 0; k < NAD; k++) if (cr[k].but == B_POURSUITE && cr[k].pj >= 0 && cr[k].pj < NAD && cr[k].etat == E_VOL) changer(k, cr[k].pj, -2);
  lierMonde();
  // sorties en groupe
  pilote(h);
  soinTick(h);
  couveTick(h);
  if (ADU.ver == 1 && ADU.k < NAD && cr[ADU.k].etat != E_OFF) { static const float VF[3] = {0.6f, 1.0f, 1.8f}; ageAdu = mnf(ageAdu + h * VF[RG.vit > 2 ? 1 : RG.vit], 65000.0f); saleAdu = true; tSauvAdu += h; }      // l'âge du dernier petit devenu adulte (jusqu'à vieux)
  if (repos <= 0.0f && !par.on) {
    gGrp -= h * actFreq;
    if (gGrp <= 0.0f) { gGrp = rnd(45.0f, 90.0f); if (calme() && !mo::camOn()) sortieGroupe(); }
  }
  // sommeil collectif
  if (repos > 0.0f) { repos -= h; if (repos <= 0.0f) finRepos(false); else reposTick(); }
  else { gRepos -= h; if (gRepos <= 0.0f) { gRepos = rnd(30.0f, 50.0f); if (calme() && mo::nbAilleurs() == 0 && !GP.ph && !par.on && !men.on) debutRepos(); } }
  sauver(false);
}

static void init() {
  bool ok = false;
  if (prefs.getBytesLength("cbgrp") == sizeof(Sauv)) { prefs.getBytes("cbgrp", &S, sizeof(S)); ok = valide(); }
  sale = false; if (!ok) { generer(); sale = true; }
  pret = true; calculerFoyers(); chg = 0.0f; tSauv = 0.0f;
  for (int k = 0; k < NM; k++) { appliquer(cr[k], k); eco[k] = 0.0f; rallie[k] = false; }
  cr[0].s = 1.46f * tailleF(); cr[1].s = 1.34f * tailleF();      // le couple dominant : le mâle un peu plus grand
  for (int k = 0; k < 3; k++) if (foyer[k] >= 0 && cr[k].etat == E_PERCHE) placer(cr[k], foyer[k]);      // les trois premiers sont déjà à leur place
  memset(mis, 0, sizeof(mis)); memset(&par, 0, sizeof(par)); memset(&men, 0, sizeof(men)); layT = 0.0f; couv = 0.0f; sui = 0; ordre = 0; tChasse = tMen = tCri = 0.0f; tNour = rnd(10.0f, 18.0f);
  for (int i = 0; i < 3; i++) { faim[i] = 0.5f; beg[i] = 0.0f; }
  if (S.fph == F_PARADE) S.fph = F_SOLO; ageF = S.age; tBrin = rnd(4.0f, 7.0f); tEuf = rnd(8.0f, 14.0f);
  pend[0].k = pend[1].k = -1; vuV = vuH = vuP = false; vuSec = dernierSecouement; tAlPrec = 0.0f; tMob = 10.0f; tDepl = 0.0f; tRappel = -1.0f;
  gGrp = rnd(35.0f, 60.0f); gRepos = rnd(70.0f, 110.0f); repos = 0.0f; sent = -1;
  RG.on = prefs.getUChar("cbfam", 1) ? 1 : 0; RG.aide = prefs.getUChar("cbfaid", 1); if (RG.aide > 2) RG.aide = 1; RG.prot = prefs.getUChar("cbfpro", 1); if (RG.prot > 2) RG.prot = 1; RG.vit = prefs.getUChar("cbfvit", 1); if (RG.vit > 2) RG.vit = 1;
  memset(&PE, 0, sizeof(PE)); if (prefs.getBytesLength("cbpet") == sizeof(Pet)) { prefs.getBytes("cbpet", &PE, sizeof(PE)); if (PE.ver != 1 || PE.stade > 3) memset(&PE, 0, sizeof(PE)); }
  AFR.naiss = prefs.getUChar("cbafnai", 0); if (AFR.naiss > 1) AFR.naiss = 0; AFR.nuit = prefs.getUChar("cbafnui", 0); if (AFR.nuit > 1) AFR.nuit = 0;
  AFR.couple = prefs.getUChar("cbafcpl", 1) ? 1 : 0; AFR.nidv = prefs.getUChar("cbafnid", 1); if (AFR.nidv > 2) AFR.nidv = 1;
  memset(&ADU, 0, sizeof(ADU)); ageAdu = 0.0f; saleAdu = false; tSauvAdu = 0.0f;
  if (prefs.getBytesLength("cbafne") == sizeof(Adu)) { prefs.getBytes("cbafne", &ADU, sizeof(ADU)); if (ADU.ver != 1 || ADU.k >= NAD) memset(&ADU, 0, sizeof(ADU)); else ageAdu = (float)ADU.age; }
  memset(&RLV, 0, sizeof(RLV)); tRlv = rnd(40.0f, 70.0f); tCalin = rnd(60.0f, 100.0f); cvOn = false; cvK = -1; tCv = tCvAbs = 0.0f;
  saleP = false; restau = false; pNext = pondeur = hostY = -1; hostCible = 0; memset(&ym, 0, sizeof(ym)); memset(rage, 0, sizeof(rage)); memset(tmis, 0, sizeof(tmis));
  faimP = 0.4f; tSoin = 8.0f; tAide = 20.0f; tMenP = 0.0f; tBat = 15.0f; tHost = rnd(60.0f, 110.0f); tSauvP = 0.0f; tCriY = tChaY = 0.0f; tCriP = 4.0f; tEssai = rnd(25.0f, 45.0f); tSurv = 5.0f; tProche = 15.0f;
  if (sale) sauver(true);
}
static void maj(float dt) {
  if (!pret || (nAct < 2 && !soinOn() && !cvOn)) return;      // seul, un corbeau n'a ni couple ni famille : comportements solitaires (sauf le soin du petit : adoption seule)
  tDepl -= dt; tSauv += dt; tMob -= dt;
  majFam(dt);
  for (int i = 0; i < 2; i++) if (pend[i].k >= 0) { pend[i].t -= dt; if (pend[i].t <= 0.0f) { suivreChef(pend[i]); pend[i].k = -1; } }
  for (int k = 0; k < NM; k++) if (eco[k] > 0.0f) { eco[k] -= dt; if (eco[k] <= 0.0f) chante(k); }
  if (tRappel > 0.0f) { tRappel -= dt; if (tRappel <= 0.0f) rappel(); }
  tTick += dt; if (tTick < 0.25f) return;
  float h = tTick; tTick = 0.0f; tick(h);
}

}  // namespace fam

static int perchPour(const Corbeau& c) { return fam::perchPour(c); }
static bool petitGrand() { return fam::grand(); }
static void designerParent(int k) { fam::pNext = (int8_t)k; }
static bool oeufAF() { return fam::afOn(); }
static bool placeNaiss() { return fam::placeLibre(); }
static float rangPoids(int k) { return (fam::pret && k >= 0 && k < NCB) ? 12.0f * fam::S.rang[k] - ((fam::soinOn() && k == fam::PE.par && fam::faimP > 0.3f) ? 90.0f : 0.0f) : 0.0f; }      // le parent attitré d'un petit qui a faim est servi le premier
