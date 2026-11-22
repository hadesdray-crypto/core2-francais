#pragma once
// ======================= GROUPE COHÉRENT ET FAMILLE (famille.h) =======================
// Inclus par corbeau.h, dans l'espace de noms cb, juste avant majCorbeau (après monde.h).
// 3.1.0 : une colonie persistante (identités, rôles, rangs, liens, perchoirs attitrés, sorties et sommeil en groupe, réactions collectives).
// L'état de la colonie est mémorisé dans la mémoire NVS (clé « cbgrp », un seul bloc compact).
// monde.h (vie sociale, nourriture) garde ses interactions : ce fichier lui fournit les rangs, les liens (mémorisés) et la force de chacun, et les relit à chaque tranche.
static void chasser(Corbeau& c);
static void orbiteLocale(Corbeau& c, float cx, float cy, float sens, float duree);
namespace fam {

static const int NM = NCB;                    // membres de la colonie : six adultes et le jeune
enum { R_ALPHA = 0, R_ADULTE, R_JEUNE };
struct Sauv {
  uint8_t ver, fph;                           // version du bloc ; phase du cycle de famille (utilisée dès 3.1.1)
  uint8_t aud[NM], cal[NM], cur[NM], soc[NM], frc[NM];      // traits stables : audace, calme, curiosité, sociabilité, force
  uint8_t role[NM], rang[NM];                 // rôle (alpha, adulte, jeune) et rang (0 = dominant)
  int8_t lien[21];                            // liens entre paires (-100 rivaux, +100 amis)
  uint8_t brins, nEuf, nPou, gen, jeune;      // nid, œufs, poussins, générations, jeunes envolés (3.1.1 et 3.1.2)
  uint16_t age;                               // avancement de l'étape de la famille, en secondes
};
static Sauv S;
static bool pret = false, sale = false;
static int8_t foyer[NM];                      // perchoir attitré de chaque membre (-1 = aucun)
static float tDepl = 0.0f, tSauv = 0.0f, tTick = 0.0f, chg = 0.0f, gGrp = 40.0f, gRepos = 150.0f, repos = 0.0f, tRappel = -1.0f, tMob = 0.0f, tAlPrec = 0.0f;
static float eco[NM];                         // délai avant que le membre réponde au cri collectif
static bool vuV = false, vuH = false, vuP = false, rallie[NM];
static uint32_t vuSec = 0;
static int8_t sent = -1;                      // sentinelle du sommeil collectif
static int8_t lastM[NAD][NAD];                // dernier lien vu dans monde.h (-3 à +3) : pour répercuter ses changements dans la colonie
struct Suit { int8_t k, dest; float t; };
static Suit pend[2];

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
static void calculerFoyers() {
  static const int8_t HR[6] = {2, 3, 4, 1, 0, 5};      // du dominant au dominé : milieu de la branche, puis le perchoir haut, puis les bouts
  for (int k = 0; k < NM; k++) foyer[k] = (S.rang[k] < 6) ? HR[S.rang[k]] : (int8_t)-1;
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
  if (!pret || !sale) return;
  if (!force && tSauv < 90.0f) return;
  prefs.putBytes("cbgrp", &S, sizeof(S)); sale = false; tSauv = 0.0f; chg = 0.0f;
}

static bool libre(int k) {      // perché sur l'arbre, sans rien à faire de particulier : disponible pour une sortie ou un rôle collectif
  if (k < 0 || k >= NAD) return false;
  const Corbeau& c = cr[k];
  return c.etat == E_PERCHE && c.visible && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && c.objet == 0 && c.act <= A_REGARDE && c.duvet < 0.5f && !mo::rt[k].away;
}
static bool calme() {
  if (V.ph != V_RIEN || GP.ph || appelOn || H.actif || panneau || tAlerte > 0.0f) return false;
  for (int i = 0; i < NOB; i++) if (ob[i].actif) return false;
  return true;
}

// ---------- perchoirs attitrés : le plus fort a le meilleur, il déloge un dominé qui s'est mis sur sa place ----------
static int perchPour(const Corbeau& c) {
  int k = (int)(&c - cr), h = (pret && k >= 0 && k < NM) ? foyer[k] : -1;
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
  sent = (int8_t)best;
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
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible || c.perch < 0 || c.perch >= 10) continue;
    int hm = foyer[k];
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
  if (repos <= 0.0f) {
    gGrp -= h * actFreq;
    if (gGrp <= 0.0f) { gGrp = rnd(45.0f, 90.0f); if (calme() && !mo::camOn()) sortieGroupe(); }
  }
  // sommeil collectif
  if (repos > 0.0f) { repos -= h; if (repos <= 0.0f) finRepos(false); else reposTick(); }
  else { gRepos -= h; if (gRepos <= 0.0f) { gRepos = rnd(30.0f, 50.0f); if (calme() && mo::nbAilleurs() == 0 && !GP.ph) debutRepos(); } }
  sauver(false);
}

static void init() {
  bool ok = false;
  if (prefs.getBytesLength("cbgrp") == sizeof(Sauv)) { prefs.getBytes("cbgrp", &S, sizeof(S)); ok = valide(); }
  sale = false; if (!ok) { generer(); sale = true; }
  pret = true; calculerFoyers(); chg = 0.0f; tSauv = 0.0f;
  for (int k = 0; k < NM; k++) { appliquer(cr[k], k); eco[k] = 0.0f; rallie[k] = false; }
  cr[0].s = 1.46f; cr[1].s = 1.34f;      // le couple dominant : le mâle un peu plus grand
  for (int k = 0; k < 3; k++) if (foyer[k] >= 0 && cr[k].etat == E_PERCHE) placer(cr[k], foyer[k]);      // les trois premiers sont déjà à leur place
  pend[0].k = pend[1].k = -1; vuV = vuH = vuP = false; vuSec = dernierSecouement; tAlPrec = 0.0f; tMob = 10.0f; tDepl = 0.0f; tRappel = -1.0f;
  gGrp = rnd(35.0f, 60.0f); gRepos = rnd(70.0f, 110.0f); repos = 0.0f; sent = -1;
  if (sale) sauver(true);
}
static void maj(float dt) {
  if (!pret) return;
  tDepl -= dt; tSauv += dt; tMob -= dt;
  for (int i = 0; i < 2; i++) if (pend[i].k >= 0) { pend[i].t -= dt; if (pend[i].t <= 0.0f) { suivreChef(pend[i]); pend[i].k = -1; } }
  for (int k = 0; k < NM; k++) if (eco[k] > 0.0f) { eco[k] -= dt; if (eco[k] <= 0.0f) chante(k); }
  if (tRappel > 0.0f) { tRappel -= dt; if (tRappel <= 0.0f) rappel(); }
  tTick += dt; if (tTick < 0.25f) return;
  float h = tTick; tTick = 0.0f; tick(h);
}
static void pose(Corbeau& c, int idx, float dt) { (void)c; (void)idx; (void)dt; }      // rôles de la famille (3.1.1 et 3.1.2)
static bool enService(const Corbeau& c) { (void)c; return false; }

}  // namespace fam

static int perchPour(const Corbeau& c) { return fam::perchPour(c); }
static float rangPoids(int k) { return (fam::pret && k >= 0 && k < NCB) ? 12.0f * fam::S.rang[k] : 0.0f; }
