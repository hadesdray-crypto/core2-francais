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
enum { F_SOLO = 0, F_PARADE, F_NID, F_PONTE, F_COUVE, F_POUSSINS, F_JEUNES, F_VIDE };      // étapes du cycle de famille
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
static float ageF = 0.0f, tBrin = 6.0f, tEuf = 15.0f, layT = 0.0f;
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
static void echanger(int k, int perch) { for (int j = 0; j < NM; j++) if (j != k && foyer[j] == perch) { foyer[j] = foyer[k]; break; } foyer[k] = (int8_t)perch; }
static void calculerFoyers() {
  static const int8_t HR[6] = {2, 3, 4, 1, 0, 5};      // du dominant au dominé : milieu de la branche, puis le perchoir haut, puis les bouts
  for (int k = 0; k < NM; k++) foyer[k] = (S.rang[k] < 6) ? HR[S.rang[k]] : (int8_t)-1;
  if (S.fph >= F_NID) { echanger(0, 4); echanger(1, 2); }      // le couple : le mâle veille au-dessus du nid, la femelle sur la grosse branche
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
  S.age = (uint16_t)mnf(ageF, 60000.0f);
  prefs.putBytes("cbgrp", &S, sizeof(S)); sale = false; tSauv = 0.0f; chg = 0.0f;
}

static bool libre(int k) {      // perché sur l'arbre, sans rien à faire de particulier : disponible pour une sortie ou un rôle collectif
  if (k < 0 || k >= NAD) return false;
  const Corbeau& c = cr[k];
  if (k < 2 && S.fph >= F_NID) return false;      // le couple a mieux à faire
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
    Corbeau& c = cr[k]; if (c.etat != E_PERCHE || !c.visible) continue;
    if (c.but == B_FAM) { if (k != sent && c.act != A_DORT && c.act <= A_REGARDE) { c.act = A_DORT; c.tAct = 0.0f; c.dAct = repos + 1.0f; } continue; }      // au nid : elle dort sur place
    if (c.perch < 0 || c.perch >= 10) continue;
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

// ---------- famille (3.1.1) : le couple dominant se forme, bâtit un nid sur la fourche, la femelle y pond ----------
// Cycle accéléré : parade (~40 s), nid (~2 à 3 min), ponte (~1 min). L'étape et le nid sont mémorisés (S.fph, S.brins, S.nEuf, S.age).
static const float NX = 88.0f, NY = 106.0f;      // le nid : sur une fourche au bout de la petite branche du tronc, devant le ciel
static const int NB_NID = 16;                    // brindilles d'un nid fini
static const float T_SOLO = 45.0f;
enum { M_RIEN = 0, M_VERS_BRIN, M_PIQUE, M_VERS_NID, M_DEPOSE, M_COUPLE, M_VERS_COUVE, M_NID };
struct Mis { uint8_t ph; bool porte; float t; };      // mission d'un parent (0 = le mâle, 1 = la femelle) ; t avance image par image (pose)
static Mis mis[2];
struct Par { bool on, go; uint8_t step; float t, sx; };
static Par par;

static bool parentLibre(int p) {
  const Corbeau& c = cr[p];
  return c.etat == E_PERCHE && c.visible && c.but == B_RIEN && c.perch >= 0 && c.perch < NPE && c.objet == 0 && c.act <= A_REGARDE && !mo::rt[p].away;
}
static bool auNid(int p) { return mis[p].ph == M_NID && cr[p].etat == E_PERCHE && cr[p].but == B_FAM; }
static void versPoint(int p, float x, float y, uint8_t ph) {
  Corbeau& c = cr[p]; c.perch = -1; c.item = -1; c.rapporte = false; c.fuite = false; c.pj = -1;
  mis[p].ph = ph; mis[p].t = 0.0f;
  decoller(c, x, y, B_FAM);
}
static void arrive(Corbeau& c, int idx) {      // fin de l'atterrissage d'un parent sur un point de sa mission
  int p = (idx >= 0 && idx < 2) ? idx : -1;
  if (p < 0 || mis[p].ph == M_RIEN) { c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; return; }
  Mis& m = mis[p]; c.t = 0.0f; c.ox = c.oy = 0.0f; c.act = A_IMMO; c.tAct = 0.0f; c.dAct = rnd(1.5f, 3.0f); c.sub = 0; c.peck = 0.0f; m.t = 0.0f;
  switch (m.ph) {
    case M_VERS_BRIN: c.etat = E_SOL; m.ph = M_PIQUE; break;
    case M_VERS_NID: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (NX >= c.fx) ? 1 : -1; m.ph = M_DEPOSE; break;
    case M_VERS_COUVE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = 1; m.ph = M_NID; break;
    case M_COUPLE: c.etat = E_PERCHE; c.tfin = 1e9f; posture(c); c.d = (cr[1 - p].x >= c.x) ? 1 : -1; break;
    default: c.etat = E_PERCHE; c.but = B_RIEN; c.tfin = 4; m.ph = M_RIEN; break;
  }
}
static void sol(Corbeau& c, int idx, float dt) {      // au sol : il ramasse une brindille (tête qui plonge trois fois), puis la porte au nid
  (void)dt; if (idx < 0 || idx >= 2) return;
  Mis& m = mis[idx]; if (m.ph != M_PIQUE) return;
  float dip = fabsf(sinf(m.t / 0.42f * 3.1416f));
  c.haC = lerpf(-0.3f, 0.55f, dip); c.hbC = lerpf(0.2f, 0.8f, dip); c.pitchC = c.pr * 0.45f - 0.30f * dip;
  if (m.t > 1.3f) { m.porte = true; versPoint(idx, NX + 12.0f, NY - 7.5f, M_VERS_NID); }
}
static void debutParade() {
  memset(&par, 0, sizeof(par)); par.on = true; S.fph = F_PARADE; ageF = 0.0f;
}
static void finMission(int p) { Corbeau& c = cr[p]; mis[p].ph = M_RIEN; mis[p].porte = false; if (c.but == B_FAM) { c.but = B_RIEN; c.tfin = rnd(2.0f, 5.0f); c.perch = -1; versPerchoir(c); } }
static void annulerParade() {
  par.on = false; S.fph = F_SOLO; ageF = T_SOLO - 15.0f;
  if (mis[1].ph == M_COUPLE) finMission(1);
  if (cr[0].etat == E_PERCHE) cr[0].tfin = rnd(3.0f, 6.0f);
}
static void paradeTick(float h) {
  if (!par.on) { S.fph = F_SOLO; return; }
  if (cr[0].etat == E_OFF || cr[1].etat == E_OFF || (par.step < 3 && !calme())) { annulerParade(); return; }
  mo::gSoc = mxf(mo::gSoc, 3.0f); gSocial = mxf(gSocial, 3.0f); gPoof = mxf(gPoof, 3.0f); gAction = mxf(gAction, 2.0f); gObjet = mxf(gObjet, 3.0f); mo::gTrip = mxf(mo::gTrip, 3.0f);
  par.t += h;
  switch (par.step) {
    case 0:      // rapprochement : la femelle vient se poser tout contre le mâle
      if (!par.go) {
        if (parentLibre(0) && parentLibre(1)) {
          float mx = cr[0].fx, tx = (mx + 19.0f <= 232.0f) ? mx + 19.0f : mx - 19.0f; par.sx = tx; par.go = true; par.t = 0.0f;
          versPoint(1, tx, ytop(tx) + 0.4f, M_COUPLE);
        } else if (par.t > 8.0f) annulerParade();
      } else if (mis[1].ph == M_COUPLE && cr[1].etat == E_PERCHE && cr[1].but == B_FAM) { par.step = 1; par.t = 0.0f; }
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
          mis[1].ph = M_RIEN; cr[1].but = B_RIEN; cr[0].tfin = 4.0f; cr[1].tfin = 4.0f;
          orbiteLocale(cr[0], cx, cy, sens, dur); orbiteLocale(cr[1], cx, cy, sens, dur);
          cr[1].ray = cr[0].ray; cr[1].ang = cr[0].ang - 0.55f * sens; cr[1].om = cr[0].om;
          sonCri(); son(SN_AILES, 200); par.step = 3; par.t = 0.0f;
        }
      }
      break;
    default:      // retour aux perchoirs, puis le couple est formé : le nid commence
      if (par.t > 11.0f || (par.t > 4.0f && cr[0].etat == E_PERCHE && cr[1].etat == E_PERCHE)) {
        par.on = false; S.fph = F_NID; ageF = 0.0f; tBrin = rnd(5.0f, 8.0f); changer(0, 1, 100); calculerFoyers(); sale = true; sauver(true);
      }
      break;
  }
}
static void pilote(float h) {      // une tranche de 0,25 s : les missions des deux parents selon l'étape du cycle
  for (int p = 0; p < 2; p++) { Mis& m = mis[p]; if (m.ph != M_RIEN && (cr[p].etat == E_OFF || cr[p].but != B_FAM)) { m.ph = M_RIEN; m.porte = false; } }
  if (S.fph == F_PARADE) { paradeTick(h); return; }
  ageF += h;
  for (int p = 0; p < 2; p++) {      // dépôt de la brindille : elle s'ajoute au nid, puis le parent repart
    Mis& m = mis[p]; if (m.ph != M_DEPOSE || cr[p].etat != E_PERCHE) continue;
    if (m.t > 0.6f && m.porte) { m.porte = false; if (S.brins < NB_NID) S.brins++; sale = true; son(SN_CROC, 70); }
    if (m.t > 1.3f) { finMission(p); tBrin = rnd(3.0f, 7.0f); }
  }
  switch (S.fph) {
    case F_SOLO:
      if (ageF > T_SOLO && calme() && parentLibre(0) && parentLibre(1) && mo::nbAilleurs() == 0) debutParade();
      break;
    case F_NID:
      tBrin -= h;
      if (tBrin <= 0.0f && S.brins < NB_NID && calme() && mis[0].ph == M_RIEN && mis[1].ph == M_RIEN) {
        int p = random(2); if (!parentLibre(p)) p = 1 - p;
        if (parentLibre(p)) { float gx = rnd(110.0f, 300.0f); versPoint(p, gx, solHaut(gx), M_VERS_BRIN); mis[p].porte = false; tBrin = 1e6f; } else tBrin = 1.0f;
      }
      if (S.brins >= NB_NID && mis[0].ph == M_RIEN && mis[1].ph == M_RIEN) { S.fph = F_PONTE; ageF = 0.0f; tEuf = rnd(8.0f, 12.0f); S.nEuf = 0; sale = true; sauver(true); }
      break;
    case F_PONTE: case F_COUVE:
      if (mis[1].ph == M_RIEN && cr[1].etat != E_OFF && parentLibre(1) && calme()) versPoint(1, NX, NY - 4.5f, M_VERS_COUVE);      // la femelle rejoint le nid
      if (S.fph == F_PONTE && auNid(1)) {
        tEuf -= h; if (tEuf <= 0.0f && layT <= 0.0f && S.nEuf < 3) { layT = 1.5f; tEuf = rnd(16.0f, 24.0f); }
      }
      if (S.fph == F_PONTE && S.nEuf >= 3) { S.fph = F_COUVE; ageF = 0.0f; sale = true; sauver(true); }
      break;
    default: break;
  }
}

// ---------- dessin du nid (derrière les corbeaux), des brindilles du devant, des morceaux portés au bec ----------
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
  mo::epais(g, 74.0f, 109.0f, 102.0f, 109.5f, 4.6f, nr); mo::epais(g, 86.0f, 109.0f, 103.0f, 100.0f, 2.8f, nr); mo::epais(g, 80.0f, 109.5f, 96.0f, 117.0f, 2.6f, nr);
  disque(g, 102.0f, 109.5f, 2.0f, nr);
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
    float x = NX + (e - 1) * 5.6f, y = NY - 6.8f;
    ell(g, x, y, 2.8f, 3.5f, C(112, 116, 104)); ell(g, x - 0.8f, y - 1.1f, 1.1f, 1.4f, C(168, 170, 150)); disque(g, x + 1.0f, y + 1.2f, 0.5f, C(56, 52, 44));
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
static void sur(M5Canvas& g, int k) {      // ce que le corbeau k porte au bec : une brindille, un morceau offert
  if (!pret || k < 0 || k >= 2 || !cr[k].visible) return;
  const Corbeau& c = cr[k]; float X, Y, dx, dy; boutBec(c, X, Y, dx, dy); const float s = c.s;
  if (mis[k].porte) {
    float cs = cosf(0.35f), sn = sinf(0.35f), px = -dy, py = dx, tx = dx * cs + px * sn, ty = dy * cs + py * sn, cx = X - dx * 3.0f * s, cy = Y - dy * 3.0f * s;
    ligne(g, cx - tx * 6.0f * s, cy - ty * 6.0f * s, cx + tx * 9.0f * s, cy + ty * 9.0f * s, C(96, 44, 22)); ligne(g, cx - tx * 6.0f * s, cy - ty * 6.0f * s + 0.8f, cx + tx * 9.0f * s, cy + ty * 9.0f * s + 0.8f, C(60, 26, 14));
  }
  if (par.on && par.step == 2) {
    float u = lim((par.t - 2.2f) / 1.1f, 0.0f, 1.0f); bool tient = (k == 0) ? (par.t < 3.3f) : (par.t >= 3.3f && par.t < 4.6f);
    if (tient) { float mx = X, my = Y; if (u > 0.0f && u < 1.0f) { float X2, Y2, d2, e2; boutBec(cr[1 - k], X2, Y2, d2, e2); mx = lerpf(X, X2, (k == 0) ? u : 1.0f - u); my = lerpf(Y, Y2, (k == 0) ? u : 1.0f - u); }
      disque(g, mx, my - 0.5f * s, 1.3f * s, C(200, 54, 60)); disque(g, mx - 0.4f * s, my - 0.9f * s, 0.5f * s, C(255, 170, 170)); }
  }
}
static void pose(Corbeau& c, int idx, float dt) {      // attitudes des parents : lissage, partage, dépôt d'une brindille, couvaison
  if (!pret || idx < 0 || idx >= 2) return;
  Mis& m = mis[idx]; m.t += dt;
  const bool dort = (c.act == A_DORT);
  if (par.on && (par.step == 1 || par.step == 2) && c.etat == E_PERCHE) {
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
  if (m.ph == M_DEPOSE && c.etat == E_PERCHE) {      // il penche la tête dans le nid et y glisse la brindille
    float dip = sinf(3.1416f * lim(m.t / 1.3f, 0.0f, 1.0f)); c.haC = lerpf(c.ha0, 0.75f, dip); c.hbC = c.haC + 0.5f; c.pitchC = c.pr - 0.2f * dip;
  } else if (m.ph == M_NID && c.etat == E_PERCHE) {      // elle couve : corps bas dans la coupe, de temps en temps elle retourne les œufs du bec
    c.pitchC = c.pr * 0.62f; c.bob += 1.6f * c.s;
    if (!dort) {
      float ph = fmodf(tAnim * 0.083f + idx * 0.37f, 1.0f);
      if (ph < 0.12f) { float dip = sinf(3.1416f * ph / 0.12f); c.haC = lerpf(c.ha0, 0.95f, dip); c.hbC = c.haC + 0.5f; }
      if (layT > 0.0f) { float e = sinf(3.1416f * lim((1.5f - layT) / 1.5f, 0.0f, 1.0f)); c.ruf = 0.6f * e; c.pitchC -= 0.12f * e; c.bec = mxf(c.bec, 0.4f * e); }
    }
  }
}
static bool enService(const Corbeau& c) { (void)c; return false; }

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
  if (repos <= 0.0f && !par.on) {
    gGrp -= h * actFreq;
    if (gGrp <= 0.0f) { gGrp = rnd(45.0f, 90.0f); if (calme() && !mo::camOn()) sortieGroupe(); }
  }
  // sommeil collectif
  if (repos > 0.0f) { repos -= h; if (repos <= 0.0f) finRepos(false); else reposTick(); }
  else { gRepos -= h; if (gRepos <= 0.0f) { gRepos = rnd(30.0f, 50.0f); if (calme() && mo::nbAilleurs() == 0 && !GP.ph && !par.on) debutRepos(); } }
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
  memset(mis, 0, sizeof(mis)); memset(&par, 0, sizeof(par)); layT = 0.0f; if (S.fph == F_PARADE) S.fph = F_SOLO; ageF = S.age; tBrin = rnd(4.0f, 7.0f); tEuf = rnd(8.0f, 14.0f);
  pend[0].k = pend[1].k = -1; vuV = vuH = vuP = false; vuSec = dernierSecouement; tAlPrec = 0.0f; tMob = 10.0f; tDepl = 0.0f; tRappel = -1.0f;
  gGrp = rnd(35.0f, 60.0f); gRepos = rnd(70.0f, 110.0f); repos = 0.0f; sent = -1;
  if (sale) sauver(true);
}
static void maj(float dt) {
  if (!pret) return;
  tDepl -= dt; tSauv += dt; tMob -= dt;
  if (layT > 0.0f) { layT -= dt; if (layT <= 0.0f && S.nEuf < 3) { S.nEuf++; sale = true; son(SN_POP_APP, 80); plumes(NX, NY - 6.0f, 2); } }      // l'œuf apparaît dans le nid
  for (int i = 0; i < 2; i++) if (pend[i].k >= 0) { pend[i].t -= dt; if (pend[i].t <= 0.0f) { suivreChef(pend[i]); pend[i].k = -1; } }
  for (int k = 0; k < NM; k++) if (eco[k] > 0.0f) { eco[k] -= dt; if (eco[k] <= 0.0f) chante(k); }
  if (tRappel > 0.0f) { tRappel -= dt; if (tRappel <= 0.0f) rappel(); }
  tTick += dt; if (tTick < 0.25f) return;
  float h = tTick; tTick = 0.0f; tick(h);
}

}  // namespace fam

static int perchPour(const Corbeau& c) { return fam::perchPour(c); }
static float rangPoids(int k) { return (fam::pret && k >= 0 && k < NCB) ? 12.0f * fam::S.rang[k] : 0.0f; }
