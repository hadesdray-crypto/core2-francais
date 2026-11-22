#pragma once
// ======================= NATURE (nature.h) : predateurs, blessures, mort, accidents, selection naturelle =======================
// 3.1.34 (moteur AP2). Inclus par corbeau.h, dans l'espace de noms cb, juste apres famille.h.
// Un seul predateur a la fois (rapace de jour, hibou la nuit pendant le sommeil du groupe, renard au sol) : une silhouette de plus, ~12 formes, coupee en decor minimal.
// Chaque corbeau a une sante (sain, blesse leger : ne part plus en excursion, blesse grave : ne vole plus) et une cicatrice memorisees (cle cbnat).
// La mort est reglable (Jamais / Rare / Realiste) avec des garde-fous (jamais le dernier, parent attitre du petit, couple du nid) ; un mort laisse une stele ; un oeuf de secours revient si la colonie est sous le nombre regle.
// Reglages (page « Nature ») : cles cbnpre (predateurs), cbnble (blessures), cbnmor (mort), cbnmod (mode Nature / Protecteur).
namespace nat {

struct Rg { uint8_t pred, bles, mort, mode; };      // pred 0 jamais, 1 rares, 2 frequents ; bles 0 non, 1 oui ; mort 0 jamais, 1 rare, 2 realiste ; mode 0 nature, 1 protecteur
static Rg RN = {1, 1, 1, 0};
struct Sv { uint8_t ver; uint8_t sant[NCB]; uint8_t cic[NCB]; uint8_t mort[NCB]; uint16_t nMort, nPred, nBless; };      // sant 0 sain, 1 leger, 2 grave ; cic : cicatrice ; mort : emplacement vide (le corbeau est mort) ; nMort : nombre de steles
static Sv SV;
static bool pretN = false, sale = false;
static float tSauvN = 0.0f, tTk = 0.0f;
static float tSan[NCB], tFaim[NCB], tExil[NCB], tExilCd[NCB];
enum { P_RIEN = 0, P_RAPACE, P_HIBOU, P_RENARD };
struct Pd { uint8_t kind, ph; int8_t cible; bool rep, porte, fait, aboie; float t, x, y, vx, vy, sx, sy, hx, hy, tx, ty, dir, det; int8_t mob[2]; };
static Pd PD;
static float gPred = 90.0f, tAcc = 180.0f, tSec = -1.0f, tVieux = 0.0f, tCol = 0.0f, tDeuil = -1.0f, dX = 0.0f, dY = 0.0f, tOld = -1.0f; static int8_t oldK = -1; static bool vuRepos = false;

static uint8_t sante(int k) { return (k >= 0 && k < NCB && pretN) ? SV.sant[k] : 0; }
static bool estMort(int k) { return k >= 0 && k < NAD && SV.mort[k] != 0; }
static int vivAd() { int n = 0; for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (!SV.mort[k]) n++; return n; }      // adultes vivants (emplacements en service, moins les morts)
static int vivTot() { return vivAd() + ((cr[PC].etat != E_OFF) ? 1 : 0); }
static bool peutMourir(int k) {      // garde-fous : jamais le dernier, jamais le couple d'un nid qui a des oeufs ou des petits, jamais si le reglage est Jamais
  if (!pretN || !fam::pret || RN.mort == 0 || k < 0 || k >= NCB) return false;
  if (cr[k].etat == E_OFF || (k < NAD && (SV.mort[k] || k >= (int)OP.nb))) return false;
  if (vivTot() < (RN.mort == 1 ? 3 : 2)) return false;      // Rare : au moins trois vivants avant ; Realiste : au moins deux ; il en reste toujours un
  if (k < NAD && vivAd() <= 1) return false;
  if (k < 2 && fam::precieux()) return false;
  return true;
}
static void blesser(int k, int niv) {
  if (!pretN || !RN.bles || k < 0 || k >= NCB || niv < 1 || cr[k].etat == E_OFF) return;
  if (k < NAD && SV.mort[k]) return;
  uint8_t& s = SV.sant[k];
  if (niv > (int)s) s = (uint8_t)niv; else if (niv == (int)s && s == 1 && random(100) < 35) s = 2;
  if (niv >= 2 || random(100) < 55) SV.cic[k] = 1;
  tSan[k] = 0.0f; SV.nBless++; sale = true;
  Corbeau& c = cr[k]; c.flash = 0.8f; c.bec = 1.0f; c.surs = 0.35f;
}
static void coupleRompu() {      // l'un des deux parents n'est plus la : le cycle de la famille recommence a zero
  fam::par.on = false; fam::par.calin = false; fam::men.on = false; fam::RLV.ph = 0;
  if (fam::S.fph >= fam::F_PARADE) { fam::S.fph = fam::F_SOLO; fam::ageF = 0.0f; fam::S.nEuf = 0; fam::S.nPou = 0; }
  fam::tCalin = rnd(60.0f, 100.0f);
  for (int p = 0; p < 2; p++) if (cr[p].etat != E_OFF && !SV.mort[p]) fam::finMission(p);
  for (int p = 0; p < 2; p++) if (cr[p].etat == E_OFF || SV.mort[p]) { memset(&fam::mis[p], 0, sizeof(fam::Mis)); }
  fam::sauverTard();
}
static float steleX(int i) { return 128.0f + 15.0f * (float)(i % 6); }
static void refRangs() {
  fam::calculerRangs(); fam::calculerFoyers();
  for (int j = 0; j < NAD; j++) { mo::rang[j] = (int8_t)fam::S.rang[j]; mo::forceK[j] = (int8_t)(fam::S.frc[j] / 100); }
}
static void tuer(int k, int cause) {      // cause : 1 rapace, 2 bagarre, 3 vieillesse, 4 hibou, 5 renard, 6 accident
  if (k < 0 || k >= NCB) return; Corbeau& c = cr[k]; const float x = c.x, y = c.y;
  for (int j = 0; j < NCB; j++) { Corbeau& o = cr[j]; if (j != k && o.pj == k && o.but == B_POURSUITE) { o.pj = -1; if (o.etat == E_VOL) retourLibre(o); } }
  if (k < NAD) { mo::finR(mo::rt[k]); }
  for (int i = 0; i < NOB; i++) if (ob[i].reserve == k) ob[i].reserve = -1;
  if (cibleK == k) cibleK = -1; if (FR.on && FR.c == k) FR.on = false;
  if (fam::cvOn && fam::cvK == k) fam::annulerCouve();
  if (fam::pondeur == k) fam::pondeur = -1; if (fam::pNext == k) fam::pNext = -1; if (fam::hostY == k) fam::hostY = -1;
  for (int i = 0; i < 2; i++) if (fam::pend[i].k == k) fam::pend[i].k = -1;
  memset(&fam::mis[k], 0, sizeof(fam::Mis)); fam::rage[k] = false; fam::tmis[k] = 0.0f;
  if (k == PC) { memset(&fam::PE, 0, sizeof(fam::PE)); fam::saleP = true; fam::sauverY(true); fam::ym.on = false; }
  else if (fam::PE.par == k) { fam::PE.par = 255; fam::saleP = true; }
  if (k < NAD && fam::ADU.ver == 1 && fam::ADU.k == k) { fam::ADU.ver = 0; fam::saleAdu = true; fam::sauverAdu(true); }
  c.etat = E_OFF; c.visible = false; c.perch = -1; c.but = B_RIEN; c.objet = 0; c.item = -1; c.pj = -1; c.rapporte = false; c.fuite = false; c.aWp = false; c.act = A_REPOS;
  if (k < NAD) { SV.mort[k] = 1; }
  SV.sant[k] = 0; SV.cic[k] = 0; tSan[k] = 0.0f; tFaim[k] = 0.0f; tExil[k] = 0.0f;
  SV.nMort++; sale = true;
  if (cause != 3) { plumes(x, y - 3.0f, 10); eclat(x, y - 5.0f, 0.35f, 1.3f); son(SN_ALARME, 180); } else { eclat(x, y - 5.0f, 0.5f, 0.7f); }
  if (k < NAD) {
    if (k < 2) coupleRompu();
    refRangs(); fam::sale = true;
    if (vivAd() < (int)OP.nb && tSec < 0.0f) tSec = rnd(100.0f, 160.0f);      // oeuf de secours : la colonie est sous le nombre regle
  }
  tDeuil = 1.4f; dX = steleX((int)SV.nMort - 1); dY = solHaut(dX) - 14.0f;
  tSauvN = 80.0f;
}
static void deuil() {      // les autres se rassemblent un instant pres de la stele
  fam::choeur(dX, dY, 2, true);
  int m = 0;
  for (int k = 0; k < NAD && m < 3; k++) {
    if (!fam::libre(k) || random(100) < 35) continue;
    orbiteLocale(cr[k], dX, dY, random(2) ? 1.0f : -1.0f, rnd(4.0f, 6.0f)); cr[k].hum = 1.0f; m++;
  }
  if (m) sonCri();
}
static void oublier(int n) {      // le reglage Corbeaux change : les emplacements au-dela n'existent plus (le reglage reste un maximum, pas une valeur forcee)
  if (!pretN) return;
  for (int k = n; k < NAD; k++) { SV.mort[k] = 0; SV.sant[k] = 0; SV.cic[k] = 0; }
  int vivants = 0; for (int k = 0; k < NAD && k < n; k++) if (!SV.mort[k]) vivants++;
  if (vivants < 1) for (int k = 0; k < NAD; k++) SV.mort[k] = 0;
  sale = true;
}
static int slotLibre() { for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (SV.mort[k]) return k; return -1; }
static void promu(int k) {      // le petit devient adulte dans l'emplacement k : il garde les traits herites
  uint8_t* T[5] = {fam::S.aud, fam::S.cal, fam::S.cur, fam::S.soc, fam::S.frc};
  for (int t = 0; t < 5; t++) T[t][k] = T[t][PC];
  if (k >= 2) fam::S.role[k] = fam::R_ADULTE;
  SV.mort[k] = 0; SV.sant[k] = SV.sant[PC]; SV.cic[k] = SV.cic[PC]; tSan[k] = tSan[PC]; SV.sant[PC] = 0; SV.cic[PC] = 0; tSan[PC] = 0.0f;
  for (int j = 0; j < fam::NM; j++) if (j != k) fam::lien(k, j) = (int8_t)(((int)fam::S.soc[k] + fam::S.soc[j]) / 16 - 15);
  fam::calculerRangs(); fam::calculerFoyers(); fam::apresMonde();
  fam::sale = true; sale = true;
}
static void heritage(int p) {      // selection naturelle : le petit herite des traits de ses parents (moyenne) avec une petite mutation
  if (!pretN) return;
  int a = (p >= 0 && p < NAD && cr[p].etat != E_OFF && !SV.mort[p]) ? p : -1, b = -1;
  if (a < 0) { for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (!SV.mort[k] && cr[k].etat != E_OFF) { a = k; break; } }
  if (a < 0) return;
  if (a < 2) { int o = 1 - a; if (cr[o].etat != E_OFF && !SV.mort[o]) b = o; }
  if (b < 0) { int bj = -1; int bl = -999; for (int j = 0; j < NAD && j < (int)OP.nb; j++) if (j != a && !SV.mort[j] && cr[j].etat != E_OFF) { int l = fam::lien(a, j); if (l > bl) { bl = l; bj = j; } } b = bj >= 0 ? bj : a; }
  uint8_t* T[5] = {fam::S.aud, fam::S.cal, fam::S.cur, fam::S.soc, fam::S.frc};
  for (int t = 0; t < 5; t++) {
    int v = ((int)T[t][a] + (int)T[t][b]) / 2 + (int)random(-14, 15);
    if (random(100) < 8) v += (int)random(-30, 31);
    if (v < 25) v = 25; if (v > 250) v = 250; T[t][PC] = (uint8_t)v;
  }
  fam::appliquer(cr[PC], PC); fam::sale = true; sale = true;
}

// ---------- predateurs ----------
static bool predOk() { return pretN && fam::pret && ouvert && ldPas < 0 && RN.pred > 0 && decLvl() < 2 && !panneau && V.ph == V_RIEN && !GP.ph && !cbAbsent && vivTot() >= 1; }
static float delaiPred() { float d = (RN.pred == 2) ? rnd(70.0f, 140.0f) : rnd(170.0f, 320.0f); return (RN.mode == 1) ? d * 2.3f : d; }
static bool cibleOk(int k, int kind) {
  if (k < 0 || k >= NCB) return false; const Corbeau& c = cr[k];
  if (c.etat == E_OFF || !c.visible || (k < NAD && (SV.mort[k] || k >= (int)OP.nb))) return false;
  if (!mo::vis(c.x - 12.0f, c.x + 12.0f)) return false;
  if (kind == P_RENARD) return k < NAD && c.etat == E_SOL && c.objet == 0;
  if (c.etat != E_PERCHE || c.but == B_FAM || c.objet != 0) return false;
  if (k != PC && (c.perch < 0 || c.perch >= 10)) return false;
  if (kind == P_HIBOU) return c.act == A_DORT;
  return true;
}
static float score(int k, int kind) {
  const Corbeau& c = cr[k]; float s = 1.0f + rnd(0.0f, 0.5f);
  const float cal = fam::S.cal[k] / 255.0f, fo = fam::S.frc[k] / 255.0f;
  bool seul = true;
  for (int j = 0; j < NCB; j++) if (j != k && cr[j].visible && cr[j].etat != E_OFF && fabsf(cr[j].x - c.x) < 48.0f && fabsf(cr[j].y - c.y) < 40.0f) { seul = false; break; }
  if (seul) s += 0.9f;
  if (k == PC && fam::PE.stade >= 1 && fam::PE.stade <= 2) s += 1.2f;
  s += 0.9f * (float)SV.sant[k]; if (k < NAD && fam::vieux(k)) s += 0.8f;
  s += 0.6f * (1.0f - fo); s -= 0.4f * cal;
  if (kind == P_HIBOU) { int hm = fam::foyerNuit(k); if (hm >= 0 && c.perch != hm) s += 0.6f; if (c.perch >= 0 && c.perch < 4) s += 0.4f; if (k == (int)fam::sent) s -= 5.0f; }
  return s;
}
static int cibleChoisie(int kind) {
  int bk = -1; float bs = -1e9f;
  for (int k = 0; k < NCB; k++) { if (!cibleOk(k, kind)) continue; float s = score(k, kind); if (s > bs) { bs = s; bk = k; } }
  return bk;
}
static void sortirPred(bool apres) {      // le predateur s'en va (apres un coup, ou sans coup)
  if (PD.ph >= 2) return;
  PD.ph = 2; PD.t = 0.0f; PD.fait = true; (void)apres;
  if (PD.kind == P_RENARD) PD.dir = (PD.x >= PD.tx) ? 1.0f : -1.0f;
}
static void finPred() { memset(&PD, 0, sizeof(PD)); PD.mob[0] = PD.mob[1] = -1; gPred = delaiPred(); }
static float protection(int k) {      // ce que fait le joueur pour proteger la cible : main, fronde, nourriture ou objet lance, appel
  const Corbeau& c = cr[k]; float p = 0.0f;
  if (H.actif && hypotf(H.x - c.x, H.y - c.y) < 90.0f) p += 0.55f;
  for (int i = 0; i < NPROJ; i++) if (proj[i].actif && hypotf(proj[i].x - c.x, proj[i].y - c.y) < 90.0f) p += 0.5f;
  if (FR.on) p += 0.4f; if (appelOn) p += 0.3f;
  for (int i = 0; i < 2; i++) { int m = PD.mob[i]; if (m >= 0 && m < NAD && cr[m].etat != E_OFF && !SV.mort[m]) p += 0.28f; }
  if (RN.mode == 1) p *= 1.4f;
  return p;
}
static float esquive(int k) {      // les prudents survivent mieux : prudence, calme, etat de sante, sommeil
  const Corbeau& c = cr[k]; const float aud = fam::S.aud[k] / 255.0f, cal = fam::S.cal[k] / 255.0f;
  float e = 0.18f + 0.30f * (1.0f - aud) + 0.25f * cal;
  if (c.act == A_DORT) e *= 0.4f;
  e -= 0.15f * (float)SV.sant[k]; if (k < NAD && fam::vieux(k)) e -= 0.10f;
  if (RN.mode == 1) e += 0.35f;
  return lim(e, 0.05f, 0.85f);
}
static void ratee() {      // le coup echoue : des plumes, un cri, il s'en va
  const int k = PD.cible; if (k >= 0 && k < NCB) { Corbeau& c = cr[k]; plumes(c.x, c.y - 3.0f, 5); eclat(c.x, c.y - 4.0f, 0.2f, 0.8f); c.surs = 0.35f; c.bec = 1.0f; if (k != PC && c.etat == E_PERCHE && SV.sant[k] < 2 && random(100) < 40) chasser(c); }
  son(SN_ALARME, 180); sonCri();
  if (PD.mob[0] >= 0 && random(100) < 22) { int m = PD.mob[random(2) && PD.mob[1] >= 0 ? 1 : 0]; blesser(m, 1); }
  sortirPred(true);
}
static void touche(int k) {      // le coup porte : blessure, ou (rarement, garde-fous) la mort
  Corbeau& c = cr[k];
  float pl = (RN.mort == 2) ? 0.34f : 0.13f;
  if (SV.sant[k] > 0) pl *= 1.7f; if (k == PC) pl *= 0.7f; if (PD.kind == P_RENARD) pl *= 0.55f; if (RN.mode == 1) pl *= 0.35f;
  if (k < NAD && fam::vieux(k)) pl *= 1.5f;
  const bool letal = peutMourir(k) && rnd01() < pl;
  SV.nPred++; sale = true; son(SN_PLUMES, 170); vib(40);
  if (letal) { PD.porte = true; tuer(k, PD.kind == P_HIBOU ? 4 : (PD.kind == P_RENARD ? 5 : 1)); }
  else {
    const int g = (PD.kind == P_HIBOU) ? 50 : (PD.kind == P_RENARD ? 30 : 40);
    plumes(c.x, c.y - 3.0f, 9); eclat(c.x, c.y - 4.0f, 0.25f, 1.1f); son(SN_ALARME, 190);
    blesser(k, random(100) < g ? 2 : 1);
    if (k != PC && SV.sant[k] < 2 && c.etat == E_PERCHE) chasser(c);
  }
  sortirPred(true);
}
static void frappe() {      // fin du piquer / du bond : resolution (protection du joueur, des autres, du parent ; esquive ; blessure ou mort)
  if (PD.fait) return; PD.fait = true;
  const int k = PD.cible;
  if (!cibleOk(k, PD.kind) && !(PD.kind == P_RENARD && k >= 0 && k < NCB && cr[k].etat != E_OFF && cr[k].visible && cr[k].etat != E_VOL)) { ratee(); return; }
  const float prot = protection(k);
  if (k == PC) {      // le petit : son parent s'interpose d'abord (combat), il n'est atteint qu'en cas de defaite
    int pk = fam::PE.par; if (pk < 0 || pk >= NAD || cr[pk].etat != E_PERCHE || SV.mort[pk]) { pk = -1; for (int j = 0; j < NAD && j < (int)OP.nb; j++) if (!SV.mort[j] && cr[j].etat == E_PERCHE && cr[j].but == B_RIEN) { pk = j; break; } }
    if (pk >= 0) {
      Corbeau& pc = cr[pk]; pc.bec = 1.0f; pc.flash = 0.8f; pc.wlC = 1.0f; plumes((pc.x + PD.x) * 0.5f, (pc.y + PD.y) * 0.5f, 6); son(SN_CROASSE2, 200);
      const float win = 0.40f + 0.40f * fam::S.frc[pk] / 255.0f + 0.15f * fam::S.aud[pk] / 255.0f + 0.5f * prot + ((RN.mode == 1) ? 0.2f : 0.0f);
      if (rnd01() < win) { if (random(100) < 30) blesser(pk, 1); fam::changer(pk, PC, 6); ratee(); return; }
      blesser(pk, 1);
    }
  }
  if (rnd01() < esquive(k) + prot * 0.6f) { ratee(); return; }
  touche(k);
}
static void reactions(float px, float py) {      // l'alarme : les audacieux foncent, les craintifs se figent, les autres regardent
  int m = 0; PD.mob[0] = PD.mob[1] = -1;
  tAlerte = mxf(tAlerte, 3.0f); dirAlerte = (px >= 160.0f) ? 1 : -1; alerteFuite = false;
  for (int k = 0; k < NAD && k < (int)OP.nb; k++) {
    Corbeau& c = cr[k]; if (k == PD.cible || SV.mort[k] || c.etat != E_PERCHE || !c.visible || c.perch < 0 || c.perch >= 10 || c.but != B_RIEN) continue;
    const int aud = fam::S.aud[k];
    if (aud >= 150 && m < 2 && fam::libre(k)) { orbiteLocale(c, px, py + 8.0f, random(2) ? 1.0f : -1.0f, 4.5f); c.hum = 1.0f; c.bec = 1.0f; PD.mob[m++] = (int8_t)k; }
    else if (aud < 110) { c.act = A_IMMO; c.tAct = 0.0f; c.dAct = 3.0f; c.surs = 0.3f; c.hum = 1.0f; }
    else c.hum = 1.0f;
  }
  son(SN_ALARME, 200); sonCri();
}
static bool lancer(int kind) {
  const int k = cibleChoisie(kind); if (k < 0) return false;
  memset(&PD, 0, sizeof(PD)); PD.kind = (uint8_t)kind; PD.cible = (int8_t)k; PD.mob[0] = PD.mob[1] = -1; PD.ph = 0;
  const Corbeau& c = cr[k]; PD.tx = c.x; PD.ty = c.y; PD.dir = random(2) ? 1.0f : -1.0f;
  if (kind == P_RAPACE) {
    PD.sx = c.x - PD.dir * 130.0f; PD.sy = -26.0f; PD.hx = c.x - PD.dir * 56.0f; PD.hy = mxf(c.y - 70.0f, 16.0f);
    PD.x = PD.sx; PD.y = PD.sy; reactions(PD.hx, PD.hy);
  } else if (kind == P_HIBOU) {
    PD.sx = (PD.dir > 0.0f) ? bordG() - 26.0f : bordD() + 26.0f; PD.sy = c.y - 36.0f; PD.dir = (PD.sx < c.x) ? 1.0f : -1.0f;
    PD.hx = c.x - PD.dir * 22.0f; PD.hy = c.y - 28.0f; PD.x = PD.sx; PD.y = PD.sy;
  } else {
    PD.dir = (c.x < 160.0f) ? 1.0f : -1.0f; PD.sx = (PD.dir > 0.0f) ? bordG() - 20.0f : bordD() + 20.0f;
    PD.x = PD.sx; PD.y = solHaut(PD.sx) + 0.4f;
  }
  return true;
}
static void declencher() {
  bool ok = false;
  if (fam::repos > 0.0f) { if (fam::repos > 8.0f) ok = lancer(P_HIBOU); if (!ok) gPred = rnd(20.0f, 40.0f); return; }      // le groupe dort : seul le hibou rode
  if (random(100) < 40) ok = lancer(P_RENARD);
  if (!ok) ok = lancer(P_RAPACE);
  if (!ok) gPred = rnd(20.0f, 40.0f);
}
static void repousser() {      // le joueur touche le predateur : il s'enfuit tout de suite
  PD.rep = true; sortirPred(false); son(SN_ALARME, 160); eclat(PD.x, PD.y, 0.2f, 1.0f); vib(40);
}
static bool toucher(int x, int y) {
  if (!PD.kind || PD.ph >= 2) return false;
  const float r = 34.0f * zs + 6.0f;
  if (fabsf((float)x - TX(PD.x)) > r || fabsf((float)y - TY(PD.y)) > r) return false;
  repousser(); return true;
}
static void pdMaj(float dt) {
  Pd& p = PD; if (!p.kind || dt <= 0.0f) return;
  p.t += dt; const float ox = p.x, oy = p.y; const int k = p.cible;
  if (p.ph < 2 && !predOk()) sortirPred(false);
  if (p.ph == 0) {
    if (cibleOk(k, p.kind)) { p.tx = cr[k].x; p.ty = cr[k].y; } else sortirPred(false);
  } else if (p.ph == 1 && k >= 0 && k < NCB && cr[k].etat != E_OFF) { p.tx = cr[k].x; p.ty = cr[k].y; }
  if (p.kind == P_RAPACE) {
    if (p.ph == 0) { const float u = lim(p.t / 2.1f, 0.0f, 1.0f), e = pas(u); p.x = lerpf(p.sx, p.hx, e); p.y = lerpf(p.sy, p.hy, e) + 4.0f * sinf(u * 9.0f) * (1.0f - u); if (p.t >= 2.1f) { p.ph = 1; p.t = 0.0f; p.sx = p.x; p.sy = p.y; p.hx = p.x; p.hy = p.y; } }
    else if (p.ph == 1) { const float u = lim(p.t / 0.62f, 0.0f, 1.0f), e = u * u; p.x = lerpf(p.hx, p.tx, e); p.y = lerpf(p.hy, p.ty - 2.0f, e); if (u >= 1.0f) frappe(); }
    else { p.vx += p.dir * 200.0f * dt; p.vy -= 180.0f * dt; if (p.t < 0.05f) { p.vx = p.dir * 40.0f; p.vy = -30.0f; } p.x += p.vx * dt; p.y += p.vy * dt; if (p.y < -60.0f || p.t > 2.6f) { finPred(); return; } }
  } else if (p.kind == P_HIBOU) {
    if (p.ph == 0) {
      const float u = lim(p.t / 3.4f, 0.0f, 1.0f), e = pas(u); p.x = lerpf(p.sx, p.hx, e); p.y = lerpf(p.sy, p.hy, e) + 3.0f * sinf(p.t * 4.0f);
      p.det += dt;
      if (p.det >= 0.5f) {      // la sentinelle peut le voir arriver : tout le groupe se reveille et il renonce
        p.det -= 0.5f; const int s = (int)fam::sent;
        float pd = 0.10f + ((s >= 0 && s < NAD) ? 0.22f * fam::S.cal[s] / 255.0f : 0.0f); if (RN.mode == 1) pd *= 1.8f;
        if (fam::repos <= 0.0f) sortirPred(false);
        else if (s >= 0 && s < NAD && cr[s].etat != E_OFF && rnd01() < pd) { fam::finRepos(true); fam::choeur(p.x, p.y, 3, true); son(SN_ALARME, 210); sonCri(); sortirPred(false); }
      }
      if (p.ph == 0 && p.t >= 3.4f) { p.ph = 1; p.t = 0.0f; p.hx = p.x; p.hy = p.y; }
    } else if (p.ph == 1) { const float u = lim(p.t / 0.5f, 0.0f, 1.0f), e = u * u; p.x = lerpf(p.hx, p.tx, e); p.y = lerpf(p.hy, p.ty - 4.0f, e); if (u >= 1.0f) frappe(); }
    else { p.vx += p.dir * 90.0f * dt; p.vy -= 110.0f * dt; if (p.t < 0.05f) { p.vx = p.dir * 30.0f; p.vy = -20.0f; } p.x += p.vx * dt; p.y += p.vy * dt; if (p.y < -60.0f || p.t > 2.6f) { finPred(); return; } }
  } else {      // renard
    if (p.ph == 0) {
      const float d = p.tx - p.x; p.dir = (d >= 0.0f) ? 1.0f : -1.0f; p.x += p.dir * 40.0f * dt; p.y = solHaut(p.x) + 0.4f;
      p.det += dt;
      if (fabsf(d) < 150.0f && !p.aboie) { p.aboie = true; fam::choeur(p.x, p.y, 3, true); tAlerte = mxf(tAlerte, 2.5f); dirAlerte = (p.x >= 160.0f) ? 1 : -1; alerteFuite = false; son(SN_ALARME, 170); }
      if (p.det >= 0.5f) {
        p.det -= 0.5f;
        if (fabsf(d) < 110.0f) { const float pd = 0.12f + 0.25f * fam::S.cur[k] / 255.0f + 0.10f * (1.0f - fam::S.aud[k] / 255.0f); if (rnd01() < pd * (RN.mode == 1 ? 1.8f : 1.0f)) { Corbeau& c = cr[k]; if (SV.sant[k] < 2) chasser(c); sortirPred(false); } }
      }
      if (p.ph == 0 && (fabsf(d) < 26.0f || p.t > 16.0f)) { p.ph = 1; p.t = 0.0f; p.sx = p.x; }
    } else if (p.ph == 1) { const float u = lim(p.t / 0.38f, 0.0f, 1.0f); p.x = lerpf(p.sx, p.tx, u); p.y = solHaut(p.x) + 0.4f - 9.0f * sinf(u * 3.1416f); if (u >= 1.0f) frappe(); }
    else { p.x += p.dir * 120.0f * dt; p.y = solHaut(p.x) + 0.4f; if (p.x < bordG() - 40.0f || p.x > bordD() + 40.0f || p.t > 3.2f) { finPred(); return; } }
  }
  p.vx = (p.x - ox) / dt; p.vy = (p.y - oy) / dt;
}

// ---------- accidents, vieillesse, combats ----------
static void chutePoussin() {      // un poussin tombe du nid : avec la mort reglable il est perdu (une stele), sinon les parents le remettent
  plumes(fam::NX, fam::NY - 4.0f, 6); son(SN_ALARME, 190); sonCri(); eclat(fam::NX, fam::NY - 6.0f, 0.25f, 0.9f);
  for (int p = 0; p < 2; p++) if (cr[p].etat != E_OFF) cr[p].bec = 1.0f;
  if (RN.mort > 0 && fam::S.nPou >= 2) { fam::S.nPou--; fam::sauverTard(); SV.nMort++; sale = true; tDeuil = 2.0f; dX = steleX((int)SV.nMort - 1); dY = solHaut(dX) - 14.0f; }
}
static void accident() {
  if (!pretN || panneau || V.ph != V_RIEN || GP.ph || (!RN.bles && RN.mort == 0)) return;
  if ((fam::S.fph == fam::F_POUSSINS || fam::S.fph == fam::F_JEUNES) && fam::S.nPou >= 2 && random(100) < 40) { chutePoussin(); return; }
  if (RN.bles && random(100) < 60) {      // un faux pas : une patte prise, une blessure legere
    int l[NAD], n = 0; for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (!SV.mort[k] && !(k < 2 && fam::precieux()) && cr[k].etat == E_PERCHE && cr[k].visible && cr[k].perch >= 0 && cr[k].perch < 10 && SV.sant[k] == 0) l[n++] = k;
    if (n) { int k = l[random(n)]; Corbeau& c = cr[k]; plumes(c.x, c.y - 2.0f, 3); son(SN_PLUMES, 130); c.surs = 0.4f; blesser(k, 1); }
  }
}
static void vieillesse() {      // un vieux corbeau peut s'eteindre tranquillement (tres rare)
  if (RN.mort == 0 || fam::ADU.ver != 1 || fam::ADU.k >= NAD || oldK >= 0) return;
  const int k = (int)fam::ADU.k; if (cr[k].etat == E_OFF || SV.mort[k] || fam::ageAdu < fam::Y_VIEUX + 150.0f) return;
  if (!peutMourir(k)) return;
  if (random(1000) < (RN.mort == 1 ? 12 : 50)) { oldK = (int8_t)k; tOld = 9.0f; Corbeau& c = cr[k]; c.act = A_DORT; c.tAct = 0.0f; c.dAct = 12.0f; }
}
static void bagarre(int w, int l, int src) {      // une bagarre peut blesser le perdant, le chasser du groupe, changer les rangs ; jusqu'a la mort (Realiste, garde-fous)
  if (!pretN || !fam::pret || w < 0 || l < 0 || w >= NAD || l >= NAD || w == l) return;
  uint8_t& fw = fam::S.frc[w]; uint8_t& fl = fam::S.frc[l];
  if (fw < 250) fw = (uint8_t)(fw + 3); if (fl > 25) fl = (uint8_t)(fl - 4);
  fam::changer(w, l, -15); refRangs(); fam::sale = true;
  const float k = (src == 0) ? 1.0f : 0.55f;
  if (RN.bles && rnd01() < (0.34f + 0.2f * ((int)fw - (int)fl) / 255.0f) * k) blesser(l, random(100) < 25 ? 2 : 1);
  if (rnd01() < 0.45f * k) { tExil[l] = rnd(50.0f, 100.0f); tExilCd[l] = rnd(8.0f, 14.0f); }
  const bool mortel = (RN.mort == 2 && SV.sant[l] >= 1 && random(100) < 9) || (RN.mort == 1 && SV.sant[l] == 2 && random(100) < 4);
  if (mortel && src == 0 && peutMourir(l)) tuer(l, 2);
}
static void choc(Corbeau* a, Corbeau* b, float force) {      // collision en vol : peu frequente, blessure legere du plus faible
  if (!pretN || !RN.bles || !a || !b || force < 60.0f || tCol > 0.0f) return;
  const int ia = (int)(a - cr), ib = (int)(b - cr); if (ia < 0 || ib < 0 || ia >= NAD || ib >= NAD) return;
  if (a->etat != E_VOL || b->etat != E_VOL || random(100) >= 12) return;
  tCol = 90.0f; blesser(fam::S.frc[ia] <= fam::S.frc[ib] ? ia : ib, 1);
}
static void nourri(int k) {      // le joueur (ou la chasse) nourrit un corbeau : il guerit plus vite
  if (!pretN || k < 0 || k >= NCB) return;
  if (SV.sant[k]) { tSan[k] += 60.0f; cr[k].flash = 0.5f; }
  tFaim[k] = 0.0f;
}
static bool oeufSecours() {
  if (OE.actif || OE.ponte >= 0 || panneau || V.ph != V_RIEN || GP.ph || cr[PC].etat != E_OFF) return false;
  const float x = rnd(96.0f, 216.0f); OE.actif = true; OE.x = x; OE.y = ytop(x) + 0.4f; OE.t = 0; OE.tc = 0; OE.etat = 0;
  son(SN_POP_APP, 170); plumes(x, OE.y - 4.0f, 5); eclat(x, OE.y - 5.0f, 0.25f, 0.8f); vib(60);
  return true;
}
static void sauver(bool force) {
  if (!pretN || !sale) return;
  if (!force && tSauvN < 90.0f) return;
  SV.ver = 1; prefs.putBytes("cbnat", &SV, sizeof(SV)); sale = false; tSauvN = 0.0f;
}
static void init() {
  RN.pred = prefs.getUChar("cbnpre", 1); if (RN.pred > 2) RN.pred = 1;
  RN.bles = prefs.getUChar("cbnble", 1) ? 1 : 0;
  RN.mort = prefs.getUChar("cbnmor", 1); if (RN.mort > 2) RN.mort = 1;
  RN.mode = prefs.getUChar("cbnmod", 0) ? 1 : 0;
  memset(&SV, 0, sizeof(SV));
  if (prefs.getBytesLength("cbnat") == sizeof(Sv)) { prefs.getBytes("cbnat", &SV, sizeof(SV)); if (SV.ver != 1) memset(&SV, 0, sizeof(SV)); }
  for (int k = 0; k < NCB; k++) { if (SV.sant[k] > 2) SV.sant[k] = 0; SV.cic[k] = SV.cic[k] ? 1 : 0; SV.mort[k] = SV.mort[k] ? 1 : 0; tSan[k] = tFaim[k] = tExil[k] = tExilCd[k] = 0.0f; }
  SV.mort[PC] = 0;
  { int vivants = 0; for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (!SV.mort[k]) vivants++; if (vivants < 1) for (int k = 0; k < NAD; k++) SV.mort[k] = 0; }
  memset(&PD, 0, sizeof(PD)); PD.mob[0] = PD.mob[1] = -1;
  gPred = (RN.pred == 2) ? rnd(40.0f, 80.0f) : rnd(70.0f, 130.0f); tAcc = rnd(120.0f, 240.0f); tVieux = 0.0f; tCol = 0.0f; tDeuil = -1.0f; tOld = -1.0f; oldK = -1; vuRepos = false;
  { int vivants = 0; for (int k = 0; k < NAD && k < (int)OP.nb; k++) if (!SV.mort[k]) vivants++; tSec = (vivants < (int)OP.nb) ? rnd(45.0f, 90.0f) : -1.0f; }
  sale = false; tSauvN = 0.0f; tTk = 0.0f; pretN = true;
}
static void tick(float h) {
  for (int k = 0; k < NCB; k++) {
    Corbeau& c = cr[k]; if (c.etat == E_OFF || (k < NAD && SV.mort[k])) continue;
    if (SV.sant[k]) {      // guerison par le repos (plus lente en vol) et la nourriture
      float v = (c.etat == E_PERCHE || c.etat == E_SOL) ? 1.0f : 0.4f; if (RN.mort == 2) v *= 0.8f;
      tSan[k] += h * v;
      if (tSan[k] >= (SV.sant[k] == 2 ? 210.0f : 140.0f)) { SV.sant[k] = (uint8_t)(SV.sant[k] - 1); tSan[k] = 0.0f; sale = true; if (c.visible) { c.flash = 0.5f; eclat(c.x, c.y - 6.0f * c.s, 0.25f, 0.5f); } }
    }
    if (k < NAD && RN.bles) {      // faiblesse par la faim prolongee
      if (mo::en[k] < 0.10f) { tFaim[k] += h; if (tFaim[k] > 160.0f) { tFaim[k] = 0.0f; blesser(k, 1); } } else tFaim[k] = mxf(0.0f, tFaim[k] - 2.0f * h);
    }
    if (tExil[k] > 0.0f) {      // chasse du groupe : il est re-chasse de temps en temps
      tExil[k] -= h; tExilCd[k] -= h;
      if (tExilCd[k] <= 0.0f && c.etat == E_PERCHE && c.perch >= 0 && c.perch < 10 && c.but == B_RIEN && SV.sant[k] < 2 && c.objet == 0 && V.ph == V_RIEN && !GP.ph) { chasser(c); tExilCd[k] = rnd(14.0f, 24.0f); }
    }
  }
  if (!PD.kind) {
    gPred -= h;
    if (gPred <= 0.0f) { gPred = delaiPred(); if (predOk() && !appelOn) declencher(); }
    else if (fam::repos > 16.0f && !vuRepos) { vuRepos = true; if (predOk() && !appelOn && random(100) < (RN.pred == 2 ? 40 : 20)) { if (lancer(P_HIBOU)) gPred = delaiPred(); } }
  }
  if (fam::repos <= 0.0f) vuRepos = false;
  tAcc -= h; if (tAcc <= 0.0f) { tAcc = rnd(170.0f, 340.0f) / (RN.pred == 2 ? 1.4f : 1.0f); accident(); }
  tCol -= h;
  tVieux += h; if (tVieux >= 30.0f) { tVieux = 0.0f; vieillesse(); }
  if (oldK >= 0) { tOld -= h; if (tOld <= 0.0f) { const int k = oldK; oldK = -1; tOld = -1.0f; if (peutMourir(k)) tuer(k, 3); } }
  if (tDeuil > 0.0f) { tDeuil -= h; if (tDeuil <= 0.0f) deuil(); }
  if (tSec > 0.0f) {
    tSec -= h;
    if (tSec <= 0.0f) { tSec = -1.0f; if (vivAd() < (int)OP.nb) tSec = oeufSecours() ? rnd(80.0f, 120.0f) : rnd(15.0f, 30.0f); }
  }
  tSauvN += h; sauver(false);
}
static void maj(float dt) {
  if (!pretN || !fam::pret) return;
  if (PD.kind) pdMaj(dt);
  tTk += dt; if (tTk < 0.5f) return;
  const float h = tTk; tTk = 0.0f; tick(h);
}
static void ferme() { sauver(true); memset(&PD, 0, sizeof(PD)); PD.mob[0] = PD.mob[1] = -1; pretN = false; }
static void texteSante(int k, char* b, int n) {      // pour la fiche d'un corbeau (double toucher)
  if (n < 4) return; b[0] = 0; if (!pretN || k < 0 || k >= NCB) return;
  const uint8_t sa = SV.sant[k]; const bool ci = SV.cic[k] != 0;
  if (sa == 2) snprintf(b, n, "Blessé grave : repos"); else if (sa == 1) snprintf(b, n, "Blessé léger : boite"); else if (ci) snprintf(b, n, "Cicatrice à l'aile");
}

// ---------- dessin ----------
struct Fr { float x, y, fx, fy, nx, ny, s; };
static inline void pt(const Fr& f, float u, float v, float& X, float& Y) { X = f.x + (f.fx * u + f.nx * v) * f.s; Y = f.y + (f.fy * u + f.ny * v) * f.s; }
static void tri3(M5Canvas& g, const Fr& f, float u0, float v0, float u1, float v1, float u2, float v2, uint16_t c) {
  float a, b, a1, b1, a2, b2; pt(f, u0, v0, a, b); pt(f, u1, v1, a1, b1); pt(f, u2, v2, a2, b2); tri(g, a, b, a1, b1, a2, b2, c);
}
static void dessinerRapace(M5Canvas& g) {
  float sp = hypotf(PD.vx, PD.vy), fx = PD.dir, fy = 0.0f; if (sp > 6.0f) { fx = PD.vx / sp; fy = PD.vy / sp; }
  const float sg = (fx >= 0.0f) ? 1.0f : -1.0f; Fr f = {PD.x, PD.y, fx, fy, -fy * sg, fx * sg, 1.1f};
  const float o = (PD.ph == 0) ? 0.85f + 0.12f * sinf(PD.t * 7.0f) : ((PD.ph == 1) ? 0.12f : 0.7f), sw = 6.0f + 19.0f * o;
  const uint16_t cA = C(44, 26, 30), cB = C(76, 46, 46), cC = C(58, 34, 38);
  for (int s = -1; s <= 1; s += 2) {      // deux ailes : une grande, une plus claire derriere
    tri3(g, f, 3.0f, 2.5f * s, -6.0f, (2.5f + sw) * s, -8.0f, 2.0f * s, cA);
    if (!dgE()) tri3(g, f, 3.0f, 2.5f * s, -8.0f, 2.0f * s, -14.0f, (2.0f + sw * 0.5f) * s, cB);
  }
  tri3(g, f, -10.0f, 0.0f, -19.0f, -3.5f, -19.0f, 3.5f, cA);      // queue
  tri3(g, f, 9.0f, 0.0f, -4.0f, -4.0f, -4.0f, 4.0f, cC); tri3(g, f, -4.0f, -4.0f, -4.0f, 4.0f, -11.0f, 0.0f, cC);      // corps
  { float hx, hy; pt(f, 9.0f, -0.3f, hx, hy); disque(g, hx, hy, 3.0f * f.s, cC); }
  tri3(g, f, 11.5f, -1.5f, 11.5f, 1.0f, 15.5f, 2.2f, C(150, 120, 50));      // bec crochu
  { float ex, ey; pt(f, 10.6f, -1.3f, ex, ey); disque(g, ex, ey, 1.0f, C(255, 205, 60)); }      // l'oeil
  if (PD.ph == 1 && !dgE()) { float a, b, a1, b1; pt(f, 2.0f, 3.0f, a, b); pt(f, 6.5f, 7.5f, a1, b1); ligne(g, a, b, a1, b1, C(150, 120, 50)); pt(f, 0.0f, 3.0f, a, b); pt(f, 4.5f, 8.0f, a1, b1); ligne(g, a, b, a1, b1, C(150, 120, 50)); }
  if (PD.porte) { float a, b; pt(f, 1.0f, 6.0f, a, b); ell(g, a, b + 2.0f, 3.2f, 1.9f, C(8, 5, 10)); }
}
static void dessinerHibou(M5Canvas& g) {
  const float x = PD.x, y = PD.y, s = 1.1f, fl = sinf(PD.t * 6.0f) * 4.0f; const bool plane = (PD.ph < 2) ? (PD.ph == 0) : true;
  const uint16_t cB = C(86, 64, 52), cF = C(128, 104, 86), cA = C(70, 50, 44);
  if (plane) {      // ailes ecartees, un coup d'aile lent
    const float w = (PD.ph == 1) ? 17.0f : 26.0f;
    tri(g, x - 4.0f * s, y + 0.5f * s, x - 4.0f * s, y + 7.0f * s, x - w * s, y + (3.0f + fl) * s, cA); tri(g, x + 4.0f * s, y + 0.5f * s, x + 4.0f * s, y + 7.0f * s, x + w * s, y + (3.0f + fl) * s, cA);
    if (!dgE()) { tri(g, x - 4.0f * s, y + 2.0f * s, x - w * 0.7f * s, y + (6.0f + fl) * s, x - 4.0f * s, y + 8.0f * s, cB); tri(g, x + 4.0f * s, y + 2.0f * s, x + w * 0.7f * s, y + (6.0f + fl) * s, x + 4.0f * s, y + 8.0f * s, cB); }
  }
  ell(g, x, y + 4.0f * s, 5.4f * s, 7.6f * s, cB);      // corps
  ell(g, x, y - 5.0f * s, 5.4f * s, 4.4f * s, cB);      // tete
  tri(g, x - 4.4f * s, y - 7.5f * s, x - 2.2f * s, y - 8.4f * s, x - 4.8f * s, y - 12.6f * s, cB); tri(g, x + 4.4f * s, y - 7.5f * s, x + 2.2f * s, y - 8.4f * s, x + 4.8f * s, y - 12.6f * s, cB);      // aigrettes
  ell(g, x, y - 4.8f * s, 4.2f * s, 3.4f * s, cF);      // disque facial
  disque(g, x - 2.2f * s, y - 5.2f * s, 1.9f * s, C(255, 206, 70)); disque(g, x + 2.2f * s, y - 5.2f * s, 1.9f * s, C(255, 206, 70));
  disque(g, x - 2.2f * s, y - 5.2f * s, 0.8f * s, C(14, 8, 8)); disque(g, x + 2.2f * s, y - 5.2f * s, 0.8f * s, C(14, 8, 8));
  tri(g, x - 0.8f * s, y - 3.8f * s, x + 0.8f * s, y - 3.8f * s, x, y - 1.8f * s, C(170, 130, 50));      // bec
  if (PD.porte) ell(g, x, y + 10.0f * s, 3.2f * s, 1.9f * s, C(8, 5, 10));
}
static void dessinerRenard(M5Canvas& g) {
  const float d = PD.dir, s = 1.15f, y0 = PD.y; const float cr_ = (PD.ph == 0) ? 1.0f : 0.0f, gait = sinf(PD.t * ((PD.ph == 2) ? 16.0f : 6.0f)) * ((PD.ph == 1) ? 0.0f : 1.6f);
  const float by = y0 - (7.4f - 2.0f * cr_) * s, x = PD.x;
  const uint16_t cB = C(122, 52, 26), cD = C(58, 28, 20), cP = C(205, 190, 170);
  ligne(g, x + d * 5.0f * s, by + 1.5f * s, x + d * (5.0f + gait) * s, y0, cD); ligne(g, x - d * 5.0f * s, by + 1.5f * s, x - d * (5.0f + gait) * s, y0, cD);
  if (!dgE()) { ligne(g, x + d * 3.0f * s, by + 1.5f * s, x + d * (3.0f - gait) * s, y0, cD); ligne(g, x - d * 3.0f * s, by + 1.5f * s, x - d * (3.0f - gait) * s, y0, cD); }
  ell(g, x, by, 8.0f * s, 3.4f * s, cB);      // corps
  tri(g, x - d * 6.0f * s, by - 1.8f * s, x - d * 19.0f * s, by + (1.2f + gait * 0.4f) * s, x - d * 6.0f * s, by + 3.4f * s, cB);      // queue touffue
  tri(g, x - d * 15.0f * s, by + (0.6f + gait * 0.3f) * s, x - d * 19.5f * s, by + (1.2f + gait * 0.4f) * s, x - d * 14.5f * s, by + 2.4f * s, cP);      // bout blanc
  disque(g, x + d * 9.0f * s, by - 0.5f * s, 2.9f * s, cB); tri(g, x + d * 10.0f * s, by - 1.6f * s, x + d * 16.0f * s, by + 1.0f * s, x + d * 10.0f * s, by + 2.6f * s, cB);      // tete et museau
  tri(g, x + d * 8.0f * s, by - 2.2f * s, x + d * 9.8f * s, by - 2.8f * s, x + d * 8.6f * s, by - 7.0f * s, cB); tri(g, x + d * 10.2f * s, by - 2.4f * s, x + d * 11.8f * s, by - 2.0f * s, x + d * 11.4f * s, by - 6.6f * s, cB);      // oreilles
  disque(g, x + d * 10.4f * s, by - 0.9f * s, 0.8f * s, C(245, 205, 60));
  if (PD.porte) ell(g, x + d * 15.0f * s, by + 2.0f * s, 2.6f * s, 1.5f * s, C(8, 5, 10));
}
static void dessinerPred(M5Canvas& g) {
  if (!PD.kind) return;
  if (PD.kind == P_RAPACE) dessinerRapace(g); else if (PD.kind == P_HIBOU) dessinerHibou(g); else dessinerRenard(g);
}
static void dessinerSteles(M5Canvas& g) {      // les steles des disparus : petites pierres au pied de l'arbre (jusqu'a six), discretes
  if (!pretN || SV.nMort == 0 || decLvl() >= 2) return;
  const int n = SV.nMort > 6 ? 6 : (int)SV.nMort;
  for (int i = 0; i < n; i++) {
    const float x = steleX(i); if (!mo::vis(x - 9.0f, x + 9.0f)) continue;
    const float y0 = solHaut(x) + 0.6f, w = 2.9f, h = 7.5f + 1.8f * (float)(i % 3), lean = (float)((i * 37) % 5 - 2) * 0.35f;
    ell(g, x, y0, 6.4f, 1.3f, C(24, 15, 20));
    tri(g, x - w + lean, y0 - 0.4f, x + w + lean * 0.2f, y0 - 0.4f, x - w + lean * 1.4f, y0 - h, C(64, 58, 70)); tri(g, x + w + lean * 0.2f, y0 - 0.4f, x + w + lean * 1.4f, y0 - h, x - w + lean * 1.4f, y0 - h, C(64, 58, 70));
    ell(g, x + lean * 1.4f, y0 - h, w, 2.5f, C(76, 70, 84));
    if (!dgE()) { ligne(g, x + lean * 1.2f, y0 - h + 1.6f, x + lean * 1.2f, y0 - h + 6.0f, C(44, 38, 50)); ligne(g, x + 4.5f, y0 - 0.8f, x + 9.0f, y0 - 1.5f, C(10, 7, 12)); }
  }
}
static void sur(M5Canvas& g, int k) {      // un corbeau blesse ou marque : pansement clair sur l'aile, plume cassee (cicatrice)
  if (!pretN || k < 0 || k >= NCB) return;
  const uint8_t sa = SV.sant[k]; const bool ci = SV.cic[k] != 0; if (!sa && !ci) return;
  const Corbeau& c = cr[k]; if (!c.visible) return;
  const float s = c.s, d = (c.dv >= 0.0f) ? 1.0f : -1.0f, bx = c.x - d * 3.0f * s, by = c.y + c.bob - 3.4f * s;
  if (ci) { ligne(g, bx, by, bx - d * 4.6f * s, by - 2.8f * s, C(150, 146, 152)); if (!dgE()) ligne(g, bx - d * 1.0f * s, by + 0.8f * s, bx - d * 5.0f * s, by - 0.4f * s, C(112, 108, 118)); }
  if (sa) tri(g, bx + d * 1.0f * s, by + 0.8f * s, bx + d * 4.4f * s, by + 0.2f * s, bx + d * 2.2f * s, by + 3.6f * s, (sa == 2) ? C(238, 196, 120) : C(214, 206, 178));
}

}  // namespace nat

static uint8_t natSante(int k) { return nat::sante(k); }
static bool natMort(int k) { return nat::pretN && nat::estMort(k); }
static void natNourri(int k) { nat::nourri(k); }
static void natBagarre(int w, int l, int src) { nat::bagarre(w, l, src); }
static bool natImmobile(const Corbeau& c) { return nat::sante((int)(&c - cr)) >= 2; }
static void natSauver(bool force) { nat::sauver(force); }
static void natPromu(int k) { nat::promu(k); }
static int natSlotLibre() { return nat::pretN ? nat::slotLibre() : -1; }
static void natHeritage(int p) { nat::heritage(p); }
