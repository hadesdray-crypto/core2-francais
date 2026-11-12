// Core2 français — programme du M5Stack Core2 v1.1, entièrement en français.
// Personnage : « Bulle », l'âme du boîtier. Naît œuf, grandit, vit dans une boîte en 3D.
// Modules activables, protection contre les plantages, mise à jour sans câble avec retour arrière.
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <Update.h>
#include <HTTPUpdate.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include "page_web.h"
#include "types.h"
#define NB_PERSOS 2      // personnages disponibles (4 emplacements prévus en mémoire)

#define VERSION_PROGRAMME "2.1.0"
#define SOURCE_MAJ "https://raw.githubusercontent.com/hadesdray-crypto/core2-francais/main/version.json"
#define DEBUT_BIN "https://raw.githubusercontent.com/hadesdray-crypto/core2-francais/"

// ---------- Modules ----------
enum Module { M_METEO = 0, M_WIFI, M_SON, M_BATTERIE, M_HEURE, M_GRAVITE, M_MESSAGES, M_NB };
struct Reseau { String nom; int dbm; bool ferme; int canal; };
struct Touche { int x, y, w, h; String lib; int code; };      // code > 0 : caractère ; < 0 : action
static const char* NOMS_MODULES[M_NB] = {"Météo", "Wi-Fi", "Son", "Batterie", "Heure et sommeil", "Gravité", "Messages"};
static const char* CLES_MODULES[M_NB] = {"meteo", "wifi", "son", "batterie", "heure", "gravite", "messages"};
RTC_NOINIT_ATTR int dernierModule;      // module en cours au moment d'un plantage
RTC_NOINIT_ATTR uint32_t marqueRtc;

// ---------- Réglages (mémoire permanente) ----------
struct Reglages {
  bool actif[M_NB] = {true, true, true, true, true, true, true};
  String ssid, motDePasse, ssid2, mdp2, nom = "Bulle";
  float lat = 48.85f, lon = 2.35f;     // Paris par défaut ; réglable depuis la page web ou l'appli
  int luminosite = 160, volume = 120;
  int perso = 0; String reveil = ""; bool objets = true;
} R;
Preferences prefs;

// ---------- Réseaux connus : le Core2 se souvient de 8 réseaux et prend celui qu'il voit le mieux ----------
#define NB_CONNUS 8
String connuN[NB_CONNUS], connuM[NB_CONNUS];
int nbConnus() { int n = 0; for (int i = 0; i < NB_CONNUS; i++) if (connuN[i].length()) n++; return n; }
int indexConnu(const String& n) { for (int i = 0; i < NB_CONNUS; i++) if (connuN[i].length() && connuN[i] == n) return i; return -1; }
void sauverConnus() { for (int i = 0; i < NB_CONNUS; i++) { prefs.putString(("cn" + String(i)).c_str(), connuN[i]); prefs.putString(("cm" + String(i)).c_str(), connuM[i]); } }
void ajouterConnu(const String& n, const String& m) {     // le dernier utilisé passe en tête
  if (!n.length()) return;
  int j = indexConnu(n); if (j < 0) j = NB_CONNUS - 1;
  for (int i = j; i > 0; i--) { connuN[i] = connuN[i - 1]; connuM[i] = connuM[i - 1]; }
  connuN[0] = n; connuM[0] = m; sauverConnus();
}
void oublierConnu(const String& n) { int j = indexConnu(n); if (j < 0) return; for (int i = j; i < NB_CONNUS - 1; i++) { connuN[i] = connuN[i + 1]; connuM[i] = connuM[i + 1]; } connuN[NB_CONNUS - 1] = ""; connuM[NB_CONNUS - 1] = ""; sauverConnus(); }
void chargerConnus() {
  for (int i = 0; i < NB_CONNUS; i++) { connuN[i] = prefs.getString(("cn" + String(i)).c_str(), ""); connuM[i] = prefs.getString(("cm" + String(i)).c_str(), ""); }
  if (R.ssid2.length() && indexConnu(R.ssid2) < 0) ajouterConnu(R.ssid2, R.mdp2);     // reprise des anciens réglages
  if (R.ssid.length() && indexConnu(R.ssid) < 0) ajouterConnu(R.ssid, R.motDePasse);
}
void chargerReglages() {
  prefs.begin("core2fran", false);
  for (int i = 0; i < M_NB; i++) R.actif[i] = prefs.getBool(CLES_MODULES[i], true);
  R.ssid = prefs.getString("ssid", "");
  R.motDePasse = prefs.getString("mdp", "");
  R.ssid2 = prefs.getString("ssid2", ""); R.mdp2 = prefs.getString("mdp2", "");
  R.nom = prefs.getString("nom", "Bulle");
  R.lat = prefs.getFloat("lat", 48.85f);
  R.lon = prefs.getFloat("lon", 2.35f);
  R.luminosite = prefs.getInt("lum", 160);
  R.volume = prefs.getInt("vol", 120);
  R.perso = constrain(prefs.getInt("perso", 0), 0, NB_PERSOS - 1); R.reveil = prefs.getString("reveil", ""); R.objets = prefs.getBool("objets", true);
  chargerConnus();
}
void sauverReglages() {
  if (R.ssid.length()) { int j = indexConnu(R.ssid); if (j < 0 || connuM[j] != R.motDePasse) ajouterConnu(R.ssid, R.motDePasse); }
  if (R.ssid2.length()) { int j = indexConnu(R.ssid2); if (j < 0 || connuM[j] != R.mdp2) ajouterConnu(R.ssid2, R.mdp2); }
  for (int i = 0; i < M_NB; i++) prefs.putBool(CLES_MODULES[i], R.actif[i]);
  prefs.putString("ssid", R.ssid); prefs.putString("mdp", R.motDePasse); prefs.putString("nom", R.nom);
  prefs.putString("ssid2", R.ssid2); prefs.putString("mdp2", R.mdp2);
  prefs.putFloat("lat", R.lat); prefs.putFloat("lon", R.lon);
  prefs.putInt("lum", R.luminosite); prefs.putInt("vol", R.volume);
  prefs.putInt("perso", R.perso); prefs.putString("reveil", R.reveil); prefs.putBool("objets", R.objets);
}

// ---------- Personnage ----------
static const char* NOMS_PHASES[] = {"Œuf", "Bébé", "Enfant", "Ado", "Adulte", "Ancien"};
// Durées de chaque phase, en minutes d'éveil.
static const uint32_t FIN_PHASE_MIN[] = {10, 360, 2880, 10080, 43200, 0xFFFFFFFF};

struct Perso {
  uint32_t ageMin = 0;      // minutes d'éveil
  float faim = 20, joie = 70, energie = 80;
  Attitude attitude = NORMALE; uint32_t finAttitude = 0;
  float x = 0, y = 0, z = 0.3f, vx = 0, vy = 0;   // position dans la boîte (-1..1), z = profondeur (0 devant, 1 au fond)
  bool porte = false;
  uint32_t clignement = 0;
} P;
float cibleX[4] = {0, 0.63f, -0.75f, 0}, cibleY[4] = {0, 0.70f, 0.76f, 0.15f};   // 1 = lit, 2 = gamelle (position dans la boîte)
int cible = 0; uint32_t finRepas = 0; extern uint32_t debutCharge;

Phase phase() { Phase p = OEUF; for (int i = 0; i < 6; i++) { if (P.ageMin < FIN_PHASE_MIN[i]) { p = (Phase)i; break; } } return p; }
void attitude(Attitude a, uint32_t ms) { P.attitude = a; P.finAttitude = millis() + ms; }


// ---------- Fiches des personnages : tout est réglable et gardé en mémoire ----------
static const char* NOMS_EV[EV_NB] = {"Branché sur le courant", "Caresse", "Secousse", "Gros bruit", "Pluie", "Froid", "Chaud", "Faim", "Nuit", "Wi-Fi coupé", "Tête en bas"};
Fiche FI[4];
#define FA (FI[constrain(R.perso, 0, NB_PERSOS - 1)])
static void setHum(Fiche& f, int a, int r, int g, int b) { f.cHum[a][0] = r; f.cHum[a][1] = g; f.cHum[a][2] = b; }
void fichePardefaut(Fiche& f, int idx) {
  memset(&f, 0, sizeof(f)); f.version = 2; f.forme = (idx == 1) ? 1 : 0; f.humeurCouleur = true;
  strncpy(f.nom, idx == 1 ? "Enclume" : (idx == 0 ? "Bulle" : "Libre"), 15);
  if (idx == 1) {
    f.r = 40; f.g = 130; f.b = 255; f.couleurAuto = false;
    const uint8_t ca[5] = {10, 10, 10, 90, 10}; memcpy(f.carac, ca, 5);
    f.aime[EV_COURANT] = -1; f.aime[EV_SECOUSSE] = -1; f.aime[EV_BRUIT] = -1; f.aime[EV_PLUIE] = -1; f.aime[EV_FAIM] = -1;
    f.poids = 95; f.milieu = 0; f.gravite = 140; f.saut = 0; f.rebond = 12; f.mode0g = 1;
    setHum(f, ENERVE, 255, 120, 20); setHum(f, TRISTE, 120, 60, 70); setHum(f, FROID, 130, 150, 200); setHum(f, CHAUD, 255, 70, 40);
  } else {
    f.couleurAuto = true;
    const uint8_t ca[5] = {40, 60, 30, 20, 20}; memcpy(f.carac, ca, 5);
    f.poids = 30; f.milieu = 0; f.gravite = 100; f.saut = 100; f.rebond = 33; f.mode0g = 0;
    setHum(f, TRISTE, 110, 120, 170); setHum(f, FROID, 150, 200, 255); setHum(f, CHAUD, 255, 90, 60); setHum(f, ENERVE, 255, 70, 40);
  }
}
void chargerFiches() {
  for (int i = 0; i < 4; i++) {
    char k[8]; snprintf(k, sizeof(k), "fiche%d", i);
    fichePardefaut(FI[i], i);
    if (prefs.getBytesLength(k) == sizeof(Fiche)) { Fiche t; prefs.getBytes(k, &t, sizeof(t)); if (t.version == 1 || t.version == 2) { if (t.version == 1 && i == 1 && t.r == 190 && t.g == 45 && t.b == 38) { t.r = 40; t.g = 130; t.b = 255; } t.version = 2; FI[i] = t; } }
    FI[i].nom[15] = 0;
  }
}
void sauverFiche(int i) { char k[8]; snprintf(k, sizeof(k), "fiche%d", i); prefs.putBytes(k, &FI[i], sizeof(Fiche)); }
int dominant() { int b = 0; for (int i = 1; i < 5; i++) if (FA.carac[i] > FA.carac[b]) b = i; return b; }

// ---------- Petits gestes et paroles ----------
static const char* NOMS_CARACT[5] = {"Curieux", "Câlin", "Farceur", "Bougon", "Timide"};
enum Geste { G_AUCUN = 0, G_REGARDE, G_BAILLE, G_ETIRE, G_SAUT, G_CACHE, G_POINGS, G_EPAULES, G_CROISE, G_ASSIS, G_GRATTE };
Geste geste = G_AUCUN; uint32_t debutGeste = 0, finGeste = 0, prochainGeste = 20000, finDanse = 0, dernierBipDanse = 0;
String parole; uint32_t finParole = 0;
uint32_t points = 0;      // points gagnés aux jeux (gardés en mémoire)
void dire(const char* t, uint32_t ms = 3500) { parole = String(t); finParole = millis() + ms; }
// Phrases : 0 faim, 1 caresse, 2 il grandit, 3 plus de Wi-Fi, 4 réveil, 5 tête en bas, 6 branché, 7 énervé
static const char* PHRASES[5][8] = {
  {"J'ai un petit creux...", "Oh, c'est quoi ça ?", "Je grandis, super !", "Où est passé le Wi-Fi ?", "Debout, belle journée !", "Le monde est à l'envers !", "Mmh, de l'énergie !", "Hé, c'est quoi ça ?!"},
  {"Un câlin et à manger ?", "Encore, j'adore...", "Je grandis grâce à toi !", "Le Wi-Fi est parti... snif", "Bonjour toi, bien dormi ?", "Aide-moi, je suis à l'envers", "Merci, ça fait du bien", "Pas gentil, ça..."},
  {"Mon ventre fait un concert !", "Hihi, ça chatouille !", "Regarde, je deviens énorme !", "Quelqu'un a volé le Wi-Fi !", "Debout les marmottes !", "Je fais le poirier !", "Plein les piles !", "Ah non, pas ça !"},
  {"J'ai faim. Encore.", "Bon... d'accord.", "Pff, déjà plus grand.", "Même le Wi-Fi me lâche.", "Déjà ? Cinq minutes.", "Remets-moi à l'endroit.", "Enfin, du courant.", "GRRR ! LAISSE-MOI !"},
  {"Euh... j'ai un peu faim...", "Oh... merci...", "J'ai grandi... pardon ?", "Le Wi-Fi... il est parti...", "Euh... bonjour... déjà ?", "Je... je suis à l'envers...", "Oh, du courant, merci !", "S-stop... s'il te plaît"}};
const char* phrase(int type) { return PHRASES[dominant()][type]; }
bool forcerReaction = false;     // pour montrer une réaction dans l'aperçu des réglages
static const uint16_t REFROIDI[EV_NB] = {4000, 1500, 2500, 3000, 20000, 20000, 20000, 20000, 30000, 30000, 5000};
// Il réagit selon ce qu'il aime ou déteste : content ou, s'il déteste, énervé (ou triste s'il n'est pas bougon).
void reagir(int ev) {
  static uint32_t dernier[EV_NB]; uint32_t m = millis();
  if (ev < 0 || ev >= EV_NB) return;
  if (!forcerReaction && m - dernier[ev] < REFROIDI[ev]) return; dernier[ev] = m;
  int v = FA.aime[ev]; if (!v) return;
  if (!forcerReaction && phase() == OEUF) return;
  if (v > 0) { P.joie = min(100.0f, P.joie + 4); attitude(CONTENT, 3500); }
  else {
    P.joie = max(0.0f, P.joie - 4);
    if (FA.carac[3] >= 35) { attitude(ENERVE, 3500); dire(phrase(7), 3000); } else attitude(TRISTE, 3500);
  }
}

// ---------- Etat du monde ----------
struct Monde {
  int batterie = 100; bool charge = false;
  bool wifiOk = false; int rssi = 0;
  float temperature = 20; int codeMeteo = -1; uint32_t derniereMeteo = 0; bool meteoValide = false;
  float niveauSon = 0; uint32_t picSon = 0;
  bool nuit = false; bool heureValide = false;
  String message; uint32_t finMessage = 0;
  bool gravite = true;
} W;

WebServer serveur(80);
bool modeAcces = false;     // le Core2 crée son propre Wi-Fi quand aucun réseau n'est réglé
M5Canvas cv(&M5.Display);
String infoDemarrage;       // message affiché au démarrage (module désactivé après plantage, retour arrière…)
uint32_t debutDemarrage, tPage = 0;
int page = 0;               // 0 = accueil, 1 = réglages
uint32_t dernierBip = 0;
// ---------- Veille de l'écran, pile, effets ----------
bool veille = false, brancheUsb = false;
uint32_t derniereActivite = 0, dernierTap = 0;
float pileLissee = 0, pctPile = -1;
uint32_t finEcrase = 0, finVol = 0, prochainVol = 20000, finVibre = 0, finCaresse = 0;
String versionDispo, urlBinDispo, infoMaj; bool demandeMaj = false; volatile uint32_t prochaineVerif = 30000; uint32_t finGriffe = 0;
int essaiReseau = 0;
void activite() { derniereActivite = millis(); }

// ---------- Protection contre les plantages ----------
// Chaque module est exécuté entouré de debut()/fin() : si le Core2 plante, on sait lequel.
void debutModule(int m) { dernierModule = m; }
void finModule() { dernierModule = -1; }

void gererDemarrageSecurise() {
  esp_reset_reason_t r = esp_reset_reason();
  if (marqueRtc != 0xC02F5A11u) { marqueRtc = 0xC02F5A11u; dernierModule = -1; }
  bool plantage = (r == ESP_RST_PANIC || r == ESP_RST_TASK_WDT || r == ESP_RST_INT_WDT || r == ESP_RST_WDT);
  int essais = prefs.getInt("essais", 0);
  if (plantage) {
    prefs.putInt("essais", ++essais);
    if (dernierModule >= 0 && dernierModule < M_NB) {
      prefs.putBool(CLES_MODULES[dernierModule], false);
      infoDemarrage = String("Le module « ") + NOMS_MODULES[dernierModule] + " » a planté : désactivé.";
    } else infoDemarrage = "Redémarrage après un plantage.";
  } else if (r == ESP_RST_POWERON || r == ESP_RST_SW) {
    // démarrage normal : on compte quand même, remis à zéro après 45 s stables
    prefs.putInt("essais", ++essais);
  }
  dernierModule = -1;
  // Retour arrière : 4 démarrages ratés de suite → on repart sur l'ancienne version.
  if (essais >= 4) {
    const esp_partition_t* run = esp_ota_get_running_partition();
    const esp_partition_t* autre = esp_ota_get_next_update_partition(run);
    uint8_t magique = 0;
    if (autre && esp_partition_read(autre, 0, &magique, 1) == ESP_OK && magique == 0xE9) {
      prefs.putInt("essais", 0);
      esp_ota_set_boot_partition(autre);
      delay(200); esp_restart();
    } else {
      prefs.putInt("essais", 0);
      infoDemarrage = "Pas d'ancienne version à retrouver.";
    }
  }
}
void validerSiStable() {
  static bool fait = false;
  if (!fait && millis() - debutDemarrage > 45000) {
    fait = true; prefs.putInt("essais", 0);
    esp_ota_mark_app_valid_cancel_rollback();
  }
}

// ---------- Sauvegarde du personnage ----------
void sauverPerso() {
  prefs.putUInt("age", P.ageMin); prefs.putFloat("faim", P.faim); prefs.putFloat("joie", P.joie); prefs.putFloat("ener", P.energie);
}
void chargerPerso() {
  P.ageMin = prefs.getUInt("age", 0); P.faim = prefs.getFloat("faim", 20); P.joie = prefs.getFloat("joie", 70); P.energie = prefs.getFloat("ener", 80); points = prefs.getUInt("pts", 0);
}

// ---------- Sons ----------
void bip(int freq, int ms) {
  if (!R.actif[M_SON]) return;
  if (millis() - dernierBip < 150) return; dernierBip = millis();
  M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume);
  M5.Speaker.tone(freq, ms); delay(ms + 10);
  M5.Speaker.end(); M5.Mic.begin();
}

// Petits sons du personnage : une suite de notes très courte pour chaque événement.
enum { S_MANGER = 0, S_CARESSE, S_CHOC, S_SECOUE, S_PILE, S_WIFI_OK, S_WIFI_KO, S_REVEIL, S_BRANCHE, S_VOL, S_GRAVITE, S_NB };
void jouer(int s) {
  if (!R.actif[M_SON] || R.volume == 0 || s < 0 || s >= S_NB) return;
  static uint32_t dernier[S_NB];
  if (millis() - dernier[s] < 400) return; dernier[s] = millis();
  static const int16_t N[S_NB][6] = {
    {660, 60, 880, 60, 990, 80}, {520, 50, 620, 50, 740, 70}, {140, 60, 0, 0, 0, 0}, {400, 50, 300, 50, 220, 70},
    {500, 90, 350, 120, 0, 0}, {700, 50, 1000, 70, 0, 0}, {600, 60, 400, 90, 0, 0}, {800, 40, 1100, 60, 0, 0},
    {660, 60, 990, 80, 0, 0}, {900, 30, 1200, 40, 0, 0}, {300, 50, 500, 50, 0, 0} };
  M5.Mic.end(); M5.Speaker.begin(); M5.Speaker.setVolume(R.volume);
  for (int i = 0; i < 3; i++) { int f = N[s][i * 2], d = N[s][i * 2 + 1]; if (!f) break; M5.Speaker.tone(f, d); delay(d + 8); }
  M5.Speaker.end(); M5.Mic.begin(); dernierBip = millis();
}
void vibrer(int ms) { M5.Power.setVibration(150); finVibre = millis() + ms; }

// ---------- Pages d'outils ----------
// Pages : 0 accueil, 1 réglages, 2-4 réglage Wi-Fi, 5 matériel, 6 menu,
// 7 radar Wi-Fi, 8 analyseur réseau, 9 détection de mouvement, 10 météo, 11 horloge, 12 niveau, 13 sonomètre, 14 lampe.
int pageAvant = 0;
int lampeNiveau = 255;
volatile uint32_t sniffType[3] = {0, 0, 0};
volatile uint32_t sniffCanal[15] = {0};
volatile uint32_t sniffTotal = 0;
volatile long sRssiSum = 0, sRssiSq = 0; volatile int sRssiN = 0;
bool detection = false; float mvtNiveau = 0, mvtBase = 5.0f; bool mouvement = false;
uint8_t histMvt[300]; int histMvtPos = 0; uint32_t detDerniere = 0; int canalDet = 1;
uint32_t nbClicPwr = 0, nbSommeilClic = 0, nbSommeilTimeout = 0, nbTouch = 0, dbgM = 0, dbgAct = 0;
bool sniffActif = false; uint32_t debutSniff = 0, dernierHop = 0; int canalSniff = 1;
uint32_t sniffParSeconde = 0, sniffDernierTotal = 0, sniffDerniereSec = 0;
void IRAM_ATTR rappelSniff(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type == WIFI_PKT_MGMT) sniffType[0]++;
  else if (type == WIFI_PKT_CTRL) sniffType[1]++;
  else if (type == WIFI_PKT_DATA) sniffType[2]++;
  else return;
  sniffTotal++;
  wifi_promiscuous_pkt_t* p = (wifi_promiscuous_pkt_t*)buf;
  int c = p->rx_ctrl.channel; if (c >= 1 && c <= 14) sniffCanal[c]++;
  int rs = p->rx_ctrl.rssi; sRssiSum += rs; sRssiSq += (long)rs * rs; sRssiN++;
}
void demarrerSniff() {
  for (int i = 0; i < 3; i++) sniffType[i] = 0; for (int i = 0; i < 15; i++) sniffCanal[i] = 0;
  sniffTotal = 0; sniffDernierTotal = 0; sniffParSeconde = 0;
  WiFi.disconnect(); delay(20);
  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&rappelSniff);
  canalSniff = 1; esp_wifi_set_channel(canalSniff, WIFI_SECOND_CHAN_NONE);
  sniffActif = true; debutSniff = millis(); dernierHop = millis();
}
void arreterSniff() {
  esp_wifi_set_promiscuous(false); sniffActif = false;
  if (nbConnus()) WiFi.begin(connuN[0].c_str(), connuM[0].c_str());
}
// Détection de mouvement par Wi-Fi : on mesure les variations de la force des ondes
// autour du module. Quand quelqu'un bouge, les ondes sont perturbées : l'agitation monte.
void demarrerDetection() {
  sRssiSum = 0; sRssiSq = 0; sRssiN = 0; mvtNiveau = 0; mvtBase = 5.0f; mouvement = false;
  for (int i = 0; i < (int)sizeof(histMvt); i++) histMvt[i] = 0; histMvtPos = 0;
  int ch = WiFi.channel(); if (ch < 1 || ch > 13) ch = 1; canalDet = ch;
  WiFi.disconnect(); delay(20);
  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&rappelSniff);
  esp_wifi_set_channel(canalDet, WIFI_SECOND_CHAN_NONE);
  detection = true; detDerniere = millis();
}
void arreterDetection() {
  esp_wifi_set_promiscuous(false); detection = false;
  if (nbConnus()) WiFi.begin(connuN[0].c_str(), connuM[0].c_str());
}
uint32_t chronoDebut = 0; bool chronoMarche = false; uint32_t chronoFige = 0;
uint8_t histSon[220]; int histSonPos = 0;
float distanceWifi(int dbm) { return powf(10.0f, (-45.0f - dbm) / 22.0f); }

// ---------- Modules : lecture du monde ----------
int pourcentTension(float mv) {
  static const int T[][2] = {{4150,100},{4050,90},{3950,80},{3870,70},{3800,60},{3750,50},{3700,40},{3650,30},{3600,20},{3500,10},{3400,5},{3300,2},{3200,0}};
  int n = sizeof(T) / sizeof(T[0]);
  if (mv >= T[0][0]) return 100;
  for (int i = 0; i < n - 1; i++) if (mv <= T[i][0] && mv > T[i+1][0]) return T[i+1][1] + (int)((T[i][1] - T[i+1][1]) * (mv - T[i+1][0]) / (float)(T[i][0] - T[i+1][0]));
  return 0;
}
// Pourcentage de la pile.
// Sur pile : la tension fait foi. Branché : la tension monte artificiellement, donc on compte le courant qui entre (pile de 500 mAh).
int phaseCharge = 0;      // 0 pas de charge, 1 démarrage doux, 2 à fond, 3 ralentit avant la fin, 4 pleine (la puce coupe)
void moduleBatterie() {
  debutModule(M_BATTERIE);
  static uint32_t tPrec = 0; static int dernierSauve = -100;
  uint32_t m = millis();
  float mv = M5.Power.getBatteryVoltage(), ma = M5.Power.getBatteryCurrent();
  bool avant = W.charge;
  W.charge = (M5.Power.isCharging() == M5.Power.is_charging);
  uint8_t etatPuce = M5.In_I2C.readRegister8(0x34, 0x01, 400000) & 0x07;
  bool finie = brancheUsb && etatPuce == 4;
  phaseCharge = !brancheUsb ? 0 : (etatPuce == 4 ? 4 : (etatPuce == 3 ? 3 : (etatPuce == 2 ? 2 : (etatPuce == 1 ? 1 : (W.charge ? 2 : 0)))));
  if (mv > 2500 && mv < 4500) {
    pileLissee = (pileLissee == 0) ? mv : pileLissee * 0.7f + mv * 0.3f;
    float parTension = pourcentTension(pileLissee);
    if (pctPile < 0) {            // premier passage après l'allumage
      int memo = prefs.getInt("pile", -1), puce = M5.Power.getBatteryLevel();
      if (!brancheUsb) pctPile = parTension;
      else if (memo >= 0) pctPile = min((float)memo + 3, parTension);
      else { float corrige = pourcentTension(mv - max(0.0f, ma) * 1.2f); pctPile = min(corrige, (puce >= 0) ? (float)puce : corrige); }   // branché sans repère : on retire la hausse de tension due au courant de charge (estimation prudente)
    } else if (brancheUsb) {
      if (ma > 0 && tPrec) pctPile += ma * ((m - tPrec) / 3600000.0f) / 500.0f * 100.0f;
      if (pctPile > 99) pctPile = 99;
    } else {
      pctPile = pctPile * 0.8f + parTension * 0.2f;
    }
    if (finie) pctPile = 100;
    W.batterie = constrain((int)(pctPile + 0.5f), 0, 100);
    if (abs(W.batterie - dernierSauve) >= 5) { dernierSauve = W.batterie; prefs.putInt("pile", W.batterie); }
  }
  tPrec = m;
  if (W.charge && !avant) { debutCharge = millis(); attitude(SURPRIS, 700); if (!veille) jouer(S_BRANCHE); }
  finModule();
}

void moduleWifi() {
  debutModule(M_WIFI);
  bool avant = W.wifiOk;
  W.wifiOk = (WiFi.status() == WL_CONNECTED);
  W.rssi = W.wifiOk ? WiFi.RSSI() : 0;
  if (R.actif[M_WIFI] && !veille && millis() > 8000 && avant != W.wifiOk) jouer(W.wifiOk ? S_WIFI_OK : S_WIFI_KO);
  finModule();
}

void moduleSon() {
  debutModule(M_SON);
  static int16_t tampon[256];
  if (!M5.Mic.isEnabled()) { M5.Mic.begin(); }
  if (M5.Mic.record(tampon, 256, 16000)) {
    long somme = 0; for (int i = 0; i < 256; i++) somme += abs(tampon[i]);
    float niveau = somme / 256.0f;
    W.niveauSon = W.niveauSon * 0.7f + niveau * 0.3f;
    if (niveau > 6000 && millis() - W.picSon > 2000) { W.picSon = millis(); attitude(PEUR, 2500); reagir(EV_BRUIT); P.joie = max(0.0f, P.joie - 3); W.message = String("@#$%!"); W.finMessage = millis() + 2500; finGriffe = millis() + 1500; }
    else if (niveau > 1500 && millis() - W.picSon > 1500) { W.picSon = millis(); attitude(ECOUTE, 2500); }
  }
  finModule();
}

void moduleHeure() {
  debutModule(M_HEURE);
  struct tm t;
  if (getLocalTime(&t, 0)) {
    W.heureValide = true;
    W.nuit = (t.tm_hour >= 22 || t.tm_hour < 7);
    if (R.reveil.length() == 5) {      // réveil du matin : il danse
      static int jourFait = -1;
      if (t.tm_yday != jourFait && t.tm_hour == R.reveil.substring(0, 2).toInt() && t.tm_min == R.reveil.substring(3, 5).toInt()) {
        jourFait = t.tm_yday; if (veille) reveillerEcran(); activite();
        attitude(CONTENT, 12000); finDanse = millis() + 12000; dire(phrase(4), 12000);
      }
    }
  }
  finModule();
}

// La météo est demandée à Open-Meteo (gratuit, sans clé), dans une tâche à part pour ne jamais bloquer l'écran.
bool plusRecente(const String& a, const String& b) {
  int x[3] = {0, 0, 0}, y[3] = {0, 0, 0};
  sscanf(a.c_str(), "%d.%d.%d", &x[0], &x[1], &x[2]); sscanf(b.c_str(), "%d.%d.%d", &y[0], &y[1], &y[2]);
  for (int i = 0; i < 3; i++) if (x[i] != y[i]) return x[i] > y[i];
  return false;
}
// Regarde sur Internet s'il existe une version plus récente du programme.
bool verifierVersion() {
  WiFiClientSecure client; client.setInsecure(); client.setTimeout(8);
  HTTPClient http; http.setTimeout(8000); bool ok = false;
  if (http.begin(client, SOURCE_MAJ)) {
    if (http.GET() == 200) {
      JsonDocument doc;
      if (!deserializeJson(doc, http.getString())) {
        String v = doc["version"] | "", b = doc["bin"] | "";
        ok = v.length() > 0;
        if (ok && plusRecente(v, VERSION_PROGRAMME) && b.startsWith(DEBUT_BIN)) { urlBinDispo = b; versionDispo = v; infoMaj = ""; }
        else if (ok) { versionDispo = ""; urlBinDispo = ""; infoMaj = "Programme à jour (vérifié sur Internet)"; }
      }
    }
    http.end();
  }
  if (!ok) infoMaj = "Vérification impossible pour l'instant";
  return ok;
}
void tacheMeteo(void*) {
  for (;;) {
    if (W.wifiOk && millis() > prochaineVerif) { bool ok = verifierVersion(); prochaineVerif = millis() + (ok ? 6UL * 3600UL * 1000UL : 10UL * 60UL * 1000UL); }
    if (R.actif[M_METEO] && W.wifiOk && (!W.meteoValide || millis() - W.derniereMeteo > 30UL * 60UL * 1000UL)) {
      WiFiClientSecure client; client.setInsecure(); client.setTimeout(8);
      HTTPClient http; http.setTimeout(8000);
      String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(R.lat, 3) + "&longitude=" + String(R.lon, 3) + "&current=temperature_2m,weather_code";
      if (http.begin(client, url)) {
        int code = http.GET();
        if (code == 200) {
          JsonDocument doc;
          if (!deserializeJson(doc, http.getString())) {
            W.temperature = doc["current"]["temperature_2m"] | 20.0f;
            W.codeMeteo = doc["current"]["weather_code"] | -1;
            W.meteoValide = true; W.derniereMeteo = millis();
          }
        }
        http.end();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

// ---------- Vie du personnage ----------
void vieDuPerso() {
  static uint32_t dernier = 0, derniereMinute = 0;
  uint32_t m = millis();
  if (m - dernier < 100) return; dernier = m;
  // Une minute d'éveil : il vieillit (sauf quand il dort la nuit).
  if (m - derniereMinute >= 60000) {
    derniereMinute = m;
    if (phase() == OEUF || !(R.actif[M_HEURE] && W.nuit)) P.ageMin++;
    P.faim = min(100.0f, P.faim + 0.6f * (1.0f + FA.carac[3] / 300.0f));
    if (P.faim > 80) P.joie = max(0.0f, P.joie - (FA.carac[1] > 70 ? 0.5f : 1.0f));
    if (W.charge) { P.joie = min(100.0f, P.joie + 3); P.energie = min(100.0f, P.energie + 5); }   // le courant lui fait du bien
    P.energie = (R.actif[M_HEURE] && W.nuit) ? min(100.0f, P.energie + 3) : max(0.0f, P.energie - 0.3f);
    if (P.ageMin % 5 == 0) sauverPerso();
  }
  if (P.attitude != NORMALE && m > P.finAttitude) P.attitude = NORMALE;
  if (P.attitude == NORMALE) {
    if (R.actif[M_HEURE] && W.nuit && phase() != OEUF) P.attitude = SOMMEIL;
    else if (W.batterie < 15 && !brancheUsb) P.attitude = TRISTE;
    else if (P.faim > 75) P.attitude = AFFAME;
    else if (R.actif[M_METEO] && W.meteoValide && ((W.codeMeteo >= 71 && W.codeMeteo <= 77) || W.codeMeteo == 85 || W.codeMeteo == 86)) P.attitude = FROID;   // neige
    else if (R.actif[M_METEO] && W.meteoValide && W.codeMeteo >= 51) P.attitude = PLUIE;                                                      // pluie, averses, orage
    else if (R.actif[M_METEO] && W.meteoValide && W.temperature < 5) P.attitude = FROID;
    else if (R.actif[M_METEO] && W.meteoValide && W.temperature > 28) P.attitude = CHAUD;
    else if (R.actif[M_WIFI] && !W.wifiOk && R.ssid.length() && millis() > 60000) P.attitude = TRISTE;
    else if (P.joie > 75) P.attitude = CONTENT;
  }
  {   // il dit ce qu'il ressent
    static Attitude prec = NORMALE; static int phasePrec = -1;
    if (P.attitude != prec) {
      if (P.attitude == AFFAME) { dire(phrase(0)); reagir(EV_FAIM); }
      else if (P.attitude == TRISTE && R.actif[M_WIFI] && !W.wifiOk && R.ssid.length() && W.batterie >= 15) { dire(phrase(3), 4500); reagir(EV_WIFI); }
      else if (P.attitude == PLUIE) reagir(EV_PLUIE);
      else if (P.attitude == FROID) reagir(EV_FROID);
      else if (P.attitude == CHAUD) reagir(EV_CHAUD);
      prec = P.attitude;
    }
    if (phasePrec >= 0 && (int)phase() != phasePrec && phase() != OEUF) dire(phrase(2), 5000);
    phasePrec = (int)phase();
    { static bool nuitPrec = false; if (W.nuit && !nuitPrec) reagir(EV_NUIT); nuitPrec = W.nuit; }
  }
  if (millis() < finCaresse && !veille) { static uint32_t dv = 0; if (m - dv > 350) { dv = m; vibrer(35); } }   // il ronronne
  {   // petits gestes tout seuls, selon son caractère
    if (geste != G_AUCUN && m > finGeste) geste = G_AUCUN;
    if (geste == G_AUCUN && m > prochainGeste && phase() != OEUF && !veille && !P.porte && m > finVol && m > finDanse
        && (P.attitude == NORMALE || P.attitude == CONTENT) && page == 0) {
      if (FA.forme == 1) {        // Enclume a ses propres gestes : il cogne ses poings, roule des épaules, croise les bras, s'assoit, se gratte la tête
        static const uint16_t D1[5] = {1600, 2000, 3500, 6000, 2200};
        int w1[5] = {FA.carac[3] + 10, FA.carac[0] / 2 + 10, FA.carac[3] / 2 + FA.carac[4] / 2 + 5, (P.energie < 50 ? 40 : 5) + FA.carac[1] / 3, FA.carac[0] + 5};
        int tot1 = 0; for (int i = 0; i < 5; i++) tot1 += w1[i];
        int ry1 = constrain(20000 - FA.carac[0] * 80 + FA.carac[3] * 60, 8000, 40000);
        int tir = random(max(1, tot1)), g1 = 0; for (int i = 0; i < 5; i++) { if (tir < w1[i]) { g1 = i; break; } tir -= w1[i]; }
        geste = (Geste)(G_POINGS + g1); debutGeste = m; finGeste = m + D1[g1]; prochainGeste = m + ry1 + random(ry1);
        if (geste == G_POINGS && !veille) jouer(S_CHOC);
      } else {
      static const uint16_t DUREE[6] = {0, 2500, 2000, 1800, 900, 2200};
      int w[5] = {FA.carac[0] + 5, FA.carac[3] / 2 + FA.carac[1] / 2 + 5, FA.carac[1] + 5, (FA.poids < 70) ? FA.carac[2] + 2 : 0, FA.carac[4]};
      int tot = 0; for (int i = 0; i < 5; i++) tot += w[i];
      int rythme = constrain(22000 - FA.carac[2] * 120 + FA.carac[3] * 100, 8000, 40000);
      int tirage = random(max(1, tot)), g = 1; for (int i = 0; i < 5; i++) { if (tirage < w[i]) { g = i + 1; break; } tirage -= w[i]; }
      geste = (Geste)g; debutGeste = m; finGeste = m + DUREE[g];
      prochainGeste = m + rythme + random(rythme);
      if (geste == G_SAUT && !veille) jouer(S_VOL);
      }
    }
  }
  cible = 0;
  if (phase() != OEUF && W.charge) cible = 3;
  else if (phase() != OEUF) { if (P.attitude == SOMMEIL) cible = 1; else if (millis() < finRepas || P.attitude == AFFAME) cible = 2; }
}

// ---------- Mouvement dans la boîte : poids, inertie, milieu, chocs ----------
float ax = 0, ay = 0, az = 1, axPrec = 0, ayPrec = 0, azPrec = 1;
uint32_t dernierSecouement = 0;
float gSceneX = 0, gSceneY = 0;      // gravité et mouvement de la main vus par les objets de la pièce
float sqY = 0, sqYv = 0, sqX = 0, sqXv = 0, tremble = 0, gxL = 0, gyL = 1;   // écrasement (ressorts), tremblement, gravité filtrée
bool modeApercu = false;
float masse() { return 0.5f + FA.poids / 50.0f; }

// ---------- Vue de la pièce, miroir, marche au sol ----------
int vue = 0, rotEcran = 0;          // vue 0 = de face (debout), 1 = d'en haut (à plat) ; rotEcran 0, 90 ou -90 (en hauteur)
bool changeVue = false;
uint32_t miroirFin = 0; int miroirX = 160, miroirY = 100; bool contreVitre = false; float regardX = 0, regardY = 0;   // il vient contre la vitre et suit le doigt
float pasPhase = 0; bool enMarche = false; uint32_t poussiere = 0, prochaineMarche = 0; float marcheX = 0, marcheY = 0.3f;
float gzL = 0;
int pageRetourPerso = 6, retourSous = 15;
float cibleSolX[4] = {0, 0.46f, -0.54f, 0}, cibleSolY[4] = {0, 0.72f, 0.72f, 0.1f};   // lit, gamelle, prise : sur le sol, au fond
float tailleAge() { Phase ph = modeApercu ? ADULTE : phase(); return (ph == BEBE) ? 0.6f : (ph == ENFANT) ? 0.8f : (ph == ADO) ? 0.95f : (ph == ADULTE) ? 1.1f : 1.0f; }

// Un choc contre une paroi : le corps s'écrase en ressort, ça fait « boum » et le boîtier vibre.
// Un léger s'écrase beaucoup, un lourd s'écrase peu mais secoue et fait vibrer plus fort.
void choc(float v, bool vert) {
  float ms = masse();
  if (v < 0.9f - 0.4f * FA.poids / 100.0f) return;
  float k = max(0.6f, v * (3.5f - ms * 0.8f));
  if (vert) sqYv += k; else sqXv += k;
  tremble = max(tremble, min(5.0f, v * ms * 1.6f));
  if (!veille) { jouer(S_CHOC); vibrer((int)constrain(40 + v * ms * 45, 40, 260)); }
}

// Enclume ne vole jamais : il marche sur le sol de la pièce, vers le fond ou vers l'avant. Il glisse seulement si on penche beaucoup.
void physiqueSol(float dt, bool pesanteur, float linx) {
  uint32_t m = millis(); float ms = masse();
  float sx = -gxL, sz = (vue == 1 && rotEcran == 0) ? gyL : gzL;       // la pente du sol
  float pente = sqrtf(sx * sx + sz * sz), seuil = 0.2f + 0.25f * ms / 2.4f;
  float ax2 = 0, az2 = 0;
  if (pente > seuil) { float k = (pente - seuil) / pente * 5.0f * (pesanteur ? FA.gravite / 100.0f : 0.4f); ax2 += sx * k; az2 += sz * k; if (pente > seuil + 0.35f && P.attitude == NORMALE) attitude(SURPRIS, 900); }
  if (fabsf(linx) > 0.1f) ax2 += -linx * 5.0f / sqrtf(ms);             // la main qui bouge : il garde son inertie
  bool miroir = m < miroirFin;
  float tx = marcheX, ty = marcheY; bool bouge = true;
  if (miroir) { float d0 = 1.0f; tx = constrain((miroirX - 160) / (108.0f * 0.82f) * 0.78f * d0, -0.7f, 0.7f); ty = -0.74f; }
  else if (cible) { tx = cibleSolX[cible]; ty = cibleSolY[cible]; }
  else if (m > prochaineMarche) { marcheX = random(-70, 71) / 100.0f; marcheY = random(-60, 71) / 100.0f; prochaineMarche = m + 7000 + random(9000) - FA.carac[0] * 40; }
  if ((P.attitude == SOMMEIL && !cible) || geste == G_ASSIS || geste == G_POINGS || geste == G_CROISE) bouge = false;
  float dx = tx - P.x, dy = ty - P.y, d = sqrtf(dx * dx + dy * dy);
  float vmax = 0.28f / sqrtf(ms) * (P.attitude == ENERVE ? 1.8f : 1.0f) * (miroir ? 1.6f : 1.0f);
  if (bouge && d > 0.04f) { ax2 += (dx / d * vmax - P.vx) * 3.0f; az2 += (dy / d * vmax * 0.8f - P.vy) * 3.0f; }
  else { ax2 += -P.vx * 4.0f; az2 += -P.vy * 4.0f; }
  P.vx += ax2 * dt; P.vy += az2 * dt;
  P.x += P.vx * dt; P.y += P.vy * dt;
  float v = fabsf(P.vx) + fabsf(P.vy); enMarche = v > 0.04f;
  if (enMarche) { int avant = (int)(pasPhase / 3.14159f); pasPhase += v * dt * 22.0f; if ((int)(pasPhase / 3.14159f) != avant) { poussiere = m; tremble = max(tremble, 0.4f + 0.5f * ms / 2.4f); } }
  const float lim = 0.78f;
  if (P.x > lim) { P.x = lim; choc(fabsf(P.vx) * 3, false); P.vx = 0; } if (P.x < -lim) { P.x = -lim; choc(fabsf(P.vx) * 3, false); P.vx = 0; }
  if (P.y > lim) { P.y = lim; P.vy = 0; } if (P.y < -lim) { P.y = -lim; P.vy = 0; }
  contreVitre = miroir && P.y < -0.68f;
  P.z = 0.35f;
}
void physique(float dt) {
  static float zLisse = 0.35f;
  uint32_t m = millis();
  bool pesanteur = R.actif[M_GRAVITE] && W.gravite;
  float ms = masse(), dens = FA.milieu / 100.0f;
  float linx = 0, liny = 0;
  if (M5.Imu.isEnabled()) {
    axPrec = ax; ayPrec = ay; azPrec = az;
    M5.Imu.getAccel(&ax, &ay, &az);
    {   // comment le Core2 est posé : debout, à plat sur la table, ou en hauteur
      static float ox = 0, oy = 1, oz = 0; static int cand = 0; static uint32_t tCand = 0;
      float a0 = 1.0f - expf(-dt / 0.4f); ox += (ax - ox) * a0; oy += (ay - oy) * a0; oz += (az - oz) * a0;
      int c = 0;
      if (fabsf(oz) > 0.85f) c = 1; else if (ox > 0.75f && fabsf(oz) < 0.6f) c = 2; else if (ox < -0.75f && fabsf(oz) < 0.6f) c = 3;
      if (c != cand) { cand = c; tCand = m; }
      else if (m - tCand > 900) { int nv = (c == 1) ? 1 : 0, nr = (c == 2) ? -90 : (c == 3 ? 90 : 0); if (nv != vue || nr != rotEcran) { vue = nv; rotEcran = nr; changeVue = true; } }
    }
    if (rotEcran) { float rx = ax, ry = ay; if (rotEcran == -90) { ax = -ry; ay = rx; } else { ax = ry; ay = -rx; } }   // en hauteur : la gravité tourne avec l'image
    if (changeVue) { changeVue = false; axPrec = ax; ayPrec = ay; azPrec = az; }
    float sec = fabs(ax - axPrec) + fabs(ay - ayPrec) + fabs(az - azPrec);
    { static uint32_t debutRenverse = 0; static bool renverseDit = false;      // retourné la tête en bas
      if (ay < -0.7f && fabsf(az) < 0.6f) {
        if (!debutRenverse) debutRenverse = m;
        if (m - debutRenverse > 1200 && !renverseDit) { renverseDit = true; attitude(SURPRIS, 3000); dire(phrase(5), 3500); reagir(EV_RENVERSE); }
      } else { debutRenverse = 0; renverseDit = false; }
    }
    if (sec > 2.2f && m - dernierSecouement > 1200) {
      dernierSecouement = m; activite();
      attitude(ETOURDI, 3500); P.joie = max(0.0f, P.joie - 4);
      float force = (pesanteur ? 25.0f : 60.0f) * ms;     // avec la gravité il est projeté bien plus fort ; un lourd bouge moins
      P.vx += (random(-100, 100) / force); P.vy += (random(-100, 100) / force);
      finVol = 0; jouer(S_SECOUE); W.message = String("@#$%!"); W.finMessage = millis() + 2500; finGriffe = millis() + 1500;
      reagir(EV_SECOUSSE);
    }
    float a = 1.0f - expf(-dt / 0.35f);                // la gravité est la partie lente du signal, le reste est le mouvement de la main
    gxL += (ax - gxL) * a; gyL += (ay - gyL) * a; gzL += (az - gzL) * a;
    linx = ax - gxL; liny = ay - gyL;
    float gf0 = pesanteur ? FA.gravite / 100.0f : (FA.mode0g == 1 ? 0.12f : (FA.mode0g == 2 ? 0.22f : 0.0f));
    gSceneX = -(gxL * 7.0f * gf0 + linx * 8.0f); gSceneY = gyL * 7.0f * gf0 + liny * 8.0f;
  }
  sqYv += (-220.0f * sqY - 11.0f * sqYv) * dt; sqY = constrain(sqY + sqYv * dt, -0.3f, 0.45f);
  sqXv += (-220.0f * sqX - 11.0f * sqXv) * dt; sqX = constrain(sqX + sqXv * dt, -0.3f, 0.45f);
  tremble *= expf(-dt * 7.0f);
  if (P.porte) { P.vx = P.vy = 0; return; }
  if (FA.forme == 1 && phase() != OEUF) { physiqueSol(dt, pesanteur, linx); return; }
  contreVitre = false;
  if (m < miroirFin && phase() != OEUF) {      // Bulle vient tout contre la vitre, face au doigt
    float tx = constrain((miroirX - 160) / 110.0f, -0.7f, 0.7f), ty = constrain((miroirY - 100) / 70.0f, -0.6f, 0.5f);
    P.vx += ((tx - P.x) * 6.0f - P.vx * 3.5f) * dt; P.vy += ((ty - P.y) * 6.0f - P.vy * 3.5f) * dt;
    P.x += P.vx * dt; P.y += P.vy * dt; zLisse += (-0.25f - zLisse) * min(1.0f, dt * 2.5f); P.z = zLisse; finVol = 0;
    contreVitre = P.z < -0.18f; return;
  }
  if (cible && !(pesanteur && (fabs(ax) > 0.45f || fabs(ay) > 0.45f))) {   // il va au lit ou à la gamelle
    P.z = 0.5f;
    float lent = 1.0f / sqrtf(ms);
    P.vx += (cibleX[cible] - P.x) * 3.0f * lent * dt; P.vy += (cibleY[cible] - P.y) * 3.0f * lent * dt;
    P.vx *= 0.9f; P.vy *= 0.9f; P.x += P.vx * dt * 10.0f; P.y += P.vy * dt * 10.0f;
    return;
  }
  float gfac = 0, drag = 0.3f + 6.0f * dens, reb = FA.rebond * 0.0106f;
  bool tombe = pesanteur;
  if (pesanteur) gfac = FA.gravite / 100.0f;
  else if (FA.mode0g == 1) { gfac = 0.12f; drag += 2.0f; tombe = true; }       // chute lente, comme sous l'eau
  else if (FA.mode0g == 2) { gfac = 0.22f; reb *= 1.4f; tombe = true; }        // comme sur la Lune
  reb = constrain(reb, 0.0f, 0.95f);
  if (tombe) {
    float flot = constrain(1.0f - dens * 0.9f * (1.0f - ms / 2.5f), 0.05f, 1.0f);   // un léger flotte dans un liquide, un lourd coule
    float k = 11.0f / sqrtf(ms);                                                      // la main qui bouge : un lourd garde son inertie
    float lx = fabsf(linx) > 0.03f * ms ? linx : 0, ly = fabsf(liny) > 0.03f * ms ? liny : 0;
    float gx = -(gxL * 7.0f * gfac * flot + lx * k), gy = gyL * 7.0f * gfac * flot + ly * k;
    bool pente = (fabsf(gxL) + fabsf(gyL)) > 0.25f;
    float S = (FA.saut / 100.0f) * ((!pesanteur && FA.mode0g == 2) ? 1.8f : 1.0f);
    // Un être qui vole : de temps en temps il décolle, plane un peu, puis retombe.
    if (S > 0.01f && pente && phase() != OEUF && m > prochainVol && (P.attitude == NORMALE || P.attitude == CONTENT) && P.energie > 15) {
      finVol = m + 1300 + random(1100); prochainVol = finVol + 9000 + random(14000);
      if (!veille) jouer(S_VOL);
    }
    if (m < finVol) { P.vx += (-gx * 1.5f * S + sinf(m / 300.0f) * 0.8f * S) * dt; P.vy += -gy * 1.5f * S * dt; P.vx *= 0.96f; P.vy *= 0.96f; }
    else { P.vx += gx * dt; P.vy += gy * dt; }
    float f = expf(-drag * dt); P.vx *= f; P.vy *= f;
  } else {
    // Sans gravité : il est en lévitation, il flotte doucement et se balade.
    float tx = 0.45f * sinf(m / 5300.0f), ty = 0.22f * sinf(m / 3700.0f) + 0.06f * sinf(m / 700.0f);
    P.vx += (tx - P.x) * 1.4f * dt; P.vy += (ty - P.y) * 1.4f * dt;
    float f = expf(-(1.2f + 6.0f * dens) * dt); P.vx *= f; P.vy *= f; finVol = 0;
  }
  P.x += P.vx * dt; P.y += P.vy * dt;
  const float lim = 0.78f;
  float fr = constrain(0.95f - 0.12f * ms, 0.5f, 0.9f);       // frottement contre la paroi
  if (P.x > lim)  { P.x = lim;  choc(fabsf(P.vx), false); P.vx = -P.vx * reb; if (fabsf(P.vx) < 0.15f) P.vx = 0; P.vy *= fr; }
  if (P.x < -lim) { P.x = -lim; choc(fabsf(P.vx), false); P.vx = -P.vx * reb; if (fabsf(P.vx) < 0.15f) P.vx = 0; P.vy *= fr; }
  if (P.y > lim)  { P.y = lim;  choc(fabsf(P.vy), true);  P.vy = -P.vy * reb; if (fabsf(P.vy) < 0.15f) P.vy = 0; P.vx *= fr; }
  if (P.y < -lim) { P.y = -lim; choc(fabsf(P.vy), true);  P.vy = -P.vy * reb; if (fabsf(P.vy) < 0.15f) P.vy = 0; P.vx *= fr; }
  { float zc = tombe ? 0.35f : 0.35f + 0.25f * sinf(m / 4000.0f); zLisse += (zc - zLisse) * min(1.0f, dt * 2.0f); P.z = zLisse; }
}

// ---------- Dessin ----------
uint16_t C(uint8_t r, uint8_t g, uint8_t b) { return cv.color565(r, g, b); }

// ---------- Décor : ville néon, lit et gamelle ----------
static const int LIT_X = 188, LIT_W = 52, GAM_X = 74, GAM_W = 44;   // position à l'écran, au pied du mur du fond

void dessinerBoite() {
  const int L = 320, H = 200;           // zone de la boîte (le bas est pour les boutons)
  int ix = 52, iy = 36;                 // fond de la boîte
  uint16_t cyan = C(0, 230, 255), rose = C(255, 40, 170), mur = C(14, 12, 34), murCote = C(18, 14, 44), sol = C(22, 16, 46);
  cv.fillScreen(C(6, 6, 16));
  // fond : la ville la nuit
  bool jour = R.actif[M_HEURE] && W.heureValide && !W.nuit;      // le décor suit l'heure : jour ou nuit
  cv.fillRect(ix, iy, L - 2 * ix, H - 2 * iy, jour ? C(30, 70, 130) : C(10, 8, 28));
  if (jour) { int sx = ix + 190, sy = iy + 22; cv.fillCircle(sx, sy, 9, C(255, 225, 90)); for (int k = 0; k < 8; k++) { float a = k * 0.785f + millis() / 4000.0f; cv.drawLine(sx + (int)(cosf(a) * 12), sy + (int)(sinf(a) * 12), sx + (int)(cosf(a) * 17), sy + (int)(sinf(a) * 17), C(255, 225, 90)); } }
  else { int sx = ix + 190, sy = iy + 22; cv.fillCircle(sx, sy, 8, C(225, 225, 255)); cv.fillCircle(sx + 4, sy - 2, 7, C(10, 8, 28)); }
  static const uint8_t haut[9] = {50, 78, 40, 92, 62, 84, 46, 70, 56};
  for (int i = 0; i < 9; i++) {
    int bx = ix + 4 + i * 23, bh = haut[i], by = H - iy - bh;
    cv.fillRect(bx, by, 20, bh, jour ? C(40, 52, 96) : C(16, 14, 40)); cv.drawRect(bx, by, 20, bh, jour ? C(90, 110, 170) : C(44, 32, 96));
    for (int f = 0; f < 6; f++) {
      int wx = bx + 3 + (f % 3) * 6, wy = by + 5 + (f / 3) * 10 + ((i * 7 + f * 3) % 4) * 6;
      if (wy < H - iy - 6 && (jour ? ((i * 5 + f * 3) % 5 == 0) : (((i * 5 + f * 3 + (int)(millis() / 3000)) % 3) != 0))) cv.fillRect(wx, wy, 3, 4, ((i + f) % 2) ? C(255, 220, 90) : cyan);
    }
  }
  cv.drawRect(ix + 150, iy + 8, 40, 12, rose); cv.drawFastHLine(ix + 154, iy + 14, 32, rose);     // enseigne néon
  if (R.actif[M_METEO] && W.meteoValide) {      // le temps qu'il fait dehors, visible d'un coup d'œil derrière lui
    int cm = W.codeMeteo, x0 = ix + 1, y0 = iy + 1, lw = L - 2 * ix - 2, lh = H - 2 * iy - 2; uint32_t tm = millis();
    bool nuages = (cm >= 1 && cm <= 3) || cm >= 45, pluie = (cm >= 51 && cm <= 67) || (cm >= 80 && cm <= 82) || cm >= 95, neige = (cm >= 71 && cm <= 77) || cm == 85 || cm == 86, brouillard = (cm == 45 || cm == 48);
    if (nuages) { uint16_t cn = jour ? C(200, 205, 225) : C(70, 70, 100); int n = (cm >= 3) ? 4 : 2; for (int k = 0; k < n; k++) { int cx = x0 + (int)((k * 67 + tm / 120) % (lw + 40)) - 20, cy = y0 + 12 + (k % 2) * 14; cv.fillCircle(cx, cy, 8, cn); cv.fillCircle(cx + 9, cy - 3, 10, cn); cv.fillCircle(cx + 19, cy, 8, cn); } }
    if (pluie) for (int k = 0; k < 18; k++) { int rx = x0 + (k * 37 + tm / 9) % lw, ry = y0 + (k * 23 + tm / 4) % lh; cv.drawLine(rx, ry, rx - 2, ry + 6, C(130, 170, 255)); }
    if (neige) for (int k = 0; k < 16; k++) { int rx = x0 + (k * 41 + (int)(6 * sinf(tm / 700.0f + k))) % lw, ry = y0 + (k * 29 + tm / 25) % lh; cv.fillCircle(rx, ry, 1, C(240, 245, 255)); }
    if (brouillard) for (int k = 0; k < 4; k++) cv.drawFastHLine(x0, y0 + 30 + k * 22, lw, C(150, 150, 170));
    if (cm >= 95 && (tm % 4000) < 140) cv.fillRect(x0, y0, lw, lh, C(220, 220, 255));                 // éclair d'orage
    cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_right); cv.setTextColor(C(255, 255, 255)); cv.drawString(String((int)roundf(W.temperature)) + " °C", L - ix - 4, y0 + 2); cv.setTextDatum(top_left);
  }
  cv.fillTriangle(0, 0, L, 0, L - ix, iy, mur); cv.fillTriangle(0, 0, ix, iy, L - ix, iy, mur);               // plafond
  cv.fillTriangle(0, H, L, H, L - ix, H - iy, sol); cv.fillTriangle(0, H, ix, H - iy, L - ix, H - iy, sol); // sol
  cv.fillTriangle(0, 0, 0, H, ix, H - iy, murCote); cv.fillTriangle(0, 0, ix, iy, ix, H - iy, murCote);       // mur gauche
  cv.fillTriangle(L, 0, L, H, L - ix, H - iy, murCote); cv.fillTriangle(L, 0, L - ix, iy, L - ix, H - iy, murCote);
  // grille du sol
  for (int k = 0; k <= 8; k++) cv.drawLine(k * L / 8, H, ix + k * (L - 2 * ix) / 8, H - iy, C(70, 30, 120));
  for (int j = 1; j <= 3; j++) { float t = j * j / 9.0f; int y = H - (int)(iy * t), xl = (int)(ix * t); cv.drawFastHLine(xl, y, L - 2 * xl, C(70, 30, 120)); }
  // arêtes néon
  cv.drawRect(ix, iy, L - 2 * ix, H - 2 * iy, cyan);
  cv.drawLine(0, 0, ix, iy, rose); cv.drawLine(L, 0, L - ix, iy, rose);
  cv.drawLine(0, H, ix, H - iy, rose); cv.drawLine(L, H, L - ix, H - iy, rose);
  // le lit (capsule de sommeil), à droite
  cv.fillRoundRect(LIT_X, 146, LIT_W, 18, 5, C(36, 28, 76)); cv.drawRoundRect(LIT_X, 146, LIT_W, 18, 5, rose);
  cv.fillRoundRect(LIT_X + 4, 140, 16, 9, 4, C(200, 200, 255)); cv.drawFastHLine(LIT_X + 4, 162, LIT_W - 8, rose);
  // la gamelle (distributeur de granulés), à gauche
  cv.fillRect(GAM_X + 10, 126, GAM_W - 20, 22, C(30, 28, 60)); cv.drawRect(GAM_X + 10, 126, GAM_W - 20, 22, cyan);
  cv.fillCircle(GAM_X + GAM_W / 2, 132, 2, ((millis() / 600) % 2) ? C(80, 255, 140) : C(20, 80, 50));
  cv.fillRoundRect(GAM_X, 150, GAM_W, 14, 5, C(30, 28, 60)); cv.drawRoundRect(GAM_X, 150, GAM_W, 14, 5, cyan);
  for (int g = 0; g < 4; g++) cv.fillCircle(GAM_X + 9 + g * 9, 150, 2, C(80, 255, 140));
}


// Vue d'en haut (Core2 posé à plat) : on voit le sol de la pièce, le lit et la gamelle par-dessus.
void dessinerBoiteDessus() {
  uint16_t cyan = C(0, 230, 255), rose = C(255, 40, 170), murC = C(18, 14, 44);
  cv.fillScreen(C(6, 6, 16));
  cv.fillRect(4, 2, 312, 196, murC);
  cv.fillRect(14, 10, 292, 182, C(22, 16, 46));
  for (int k = 0; k <= 8; k++) cv.drawFastVLine(14 + k * 292 / 8, 10, 182, C(70, 30, 120));
  for (int k = 0; k <= 5; k++) cv.drawFastHLine(14, 10 + k * 182 / 5, 292, C(70, 30, 120));
  cv.drawRect(4, 2, 312, 196, rose); cv.drawRect(14, 10, 292, 182, cyan);
  bool sol = FA.forme == 1;
  int lx = 160 + (int)((sol ? cibleSolX[1] : cibleX[1]) * 130), ly = 100 + (int)((sol ? cibleSolY[1] : cibleY[1]) * 80);
  int gx = 160 + (int)((sol ? cibleSolX[2] : cibleX[2]) * 130), gy = 100 + (int)((sol ? cibleSolY[2] : cibleY[2]) * 80);
  ly = min(ly, 174); gy = min(gy, 176);
  cv.fillRoundRect(lx - 26, ly - 15, 52, 30, 6, C(36, 28, 76)); cv.drawRoundRect(lx - 26, ly - 15, 52, 30, 6, rose);
  cv.fillRoundRect(lx + 10, ly - 11, 12, 22, 4, C(200, 200, 255)); cv.drawFastVLine(lx - 4, ly - 13, 26, rose);
  cv.fillCircle(gx, gy, 13, C(30, 28, 60)); cv.drawCircle(gx, gy, 13, cyan); cv.drawCircle(gx, gy, 9, cyan);
  for (int g = 0; g < 4; g++) cv.fillCircle(gx - 4 + (g % 2) * 8, gy - 4 + (g / 2) * 8, 2, C(80, 255, 140));
}
void positionEcran(int& px, int& py, float& echelle) {
  const int L = 320, H = 200;
  bool sol = (FA.forme == 1 && phase() != OEUF);
  if (vue == 1 && rotEcran == 0) { echelle = 0.8f; px = 160 + (int)(P.x * 130); py = 100 + (int)(P.y * 80); if (sol) py -= (int)(0.45f * 34 * echelle * tailleAge()); return; }
  if (sol) {        // sur le sol : plus il est au fond, plus il est petit et haut
    float d = constrain((P.y + 0.78f) / 1.56f, 0.0f, 1.0f); echelle = 1.0f - 0.38f * d;
    float demi = 160 - 52 * d; px = 160 + (int)(P.x / 0.78f * demi * 0.82f);
    py = 197 - (int)(d * 33) - (int)(0.9f * 34.0f * echelle * tailleAge()); if (P.porte) py -= (int)(22 * echelle); return;
  }
  echelle = 1.0f - 0.45f * P.z;
  px = L / 2 + (int)(P.x * 110 * echelle);
  py = H / 2 + (int)(P.y * 70 * echelle);
}

// ---------- Animation de charge : il se branche, il prend le courant, il se sent bien ----------
uint32_t debutCharge = 0;
void effetChargeAvant(int px, int py, float e) {
  uint32_t t = millis();
  for (int k = 0; k < 3; k++) {
    int rr = (int)(40 * e) + k * 9 + (int)(4 * sinf(t / 200.0f + k));
    cv.drawEllipse(px, py, rr, (int)(rr * 0.9f), (k % 2) ? C(0, 230, 255) : C(255, 230, 60));
  }
}
void effetChargeApres(int px, int py, float e) {
  uint32_t t = millis();
  for (int i = 0; i < 10; i++) {          // le courant arrive vers lui
    float ph = fmodf(t / (phaseCharge == 2 ? 500.0f : 1600.0f) + i / 10.0f, 1.0f), a = i * 0.63f + t / 1500.0f, d = (1.0f - ph) * 78.0f;
    if (phaseCharge != 2 && (i % 2)) continue;          // en fin de charge, moins de courant arrive
    int x = px + (int)(cosf(a) * d), y = py + (int)(sinf(a) * d * 0.7f);
    cv.fillCircle(x, y, 2, (i % 2) ? C(255, 230, 60) : C(0, 230, 255));
  }
  int bx = px, by = py - (int)(52 * e) - (int)(4 * sinf(t / 180.0f));            // éclair au-dessus de la tête
  cv.fillTriangle(bx + 3, by - 9, bx - 5, by + 1, bx + 1, by + 1, C(255, 230, 60)); cv.fillTriangle(bx - 1, by, bx + 6, by, bx - 3, by + 11, C(255, 230, 60));
  if (t - debutCharge < 1300) {           // à l'instant du branchement : décharges depuis les coins
    for (int c = 0; c < 2; c++) {
      int sx = c ? 316 : 4, sy = 40, x0 = sx, y0 = sy;
      for (int s = 1; s <= 6; s++) { int x1 = sx + (px - sx) * s / 6 + (int)(random(-9, 9)), y1 = sy + (py - sy) * s / 6 + (int)(random(-9, 9)); if (s == 6) { x1 = px; y1 = py; } cv.drawLine(x0, y0, x1, y1, C(255, 255, 255)); x0 = x1; y0 = y1; }
    }
  }
}

void dessinerOeuf(int px, int py, float e) {
  uint32_t t = millis();
  int sec = (t / 400) % 2;
  int rx = (int)(26 * e), ry = (int)(34 * e);
  int dx = (P.attitude == ETOURDI || (P.ageMin >= 8)) ? (sec ? 2 : -2) : 0;   // il remue avant d'éclore
  uint16_t neon = C(0, 230, 255), rose = C(255, 40, 170);
  cv.fillEllipse(px + dx, py + 4, rx + 6, 6, C(40, 10, 60));
  cv.fillEllipse(px + dx, py, rx + 2, ry + 2, neon);
  cv.fillEllipse(px + dx, py, rx, ry, C(14, 16, 36));
  cv.drawLine(px + dx - rx / 2, py - ry / 2, px + dx - rx / 2, py, neon); cv.drawLine(px + dx - rx / 2, py, px + dx - rx / 4, py + ry / 3, neon);
  cv.drawLine(px + dx + rx / 2, py - ry / 3, px + dx + rx / 2, py + ry / 3, neon);
  cv.fillCircle(px + dx, py, max(3, (int)(6 * e + 2 * sinf(t / 300.0f))), rose);
  if (P.ageMin >= 6) {   // fissures lumineuses
    cv.drawLine(px + dx - 6, py - 6, px + dx, py + 2, rose); cv.drawLine(px + dx, py + 2, px + dx + 7, py - 4, rose);
    cv.drawLine(px + dx + 7, py - 4, px + dx + 12, py + 6, rose);
  }
}


// ---------- Outils de dessin communs ----------
float progGeste() { return constrain((millis() - debutGeste) / (float)max(1UL, (unsigned long)(finGeste - debutGeste)), 0.0f, 1.0f); }
uint16_t couleurHumeur(Attitude a, uint16_t def) {
  int i = (int)a; if (i < 0 || i >= 13) return def;
  const uint8_t* c = FA.cHum[i]; if (!c[0] && !c[1] && !c[2]) return def;
  return C(c[0], c[1], c[2]);
}
void epais(int x0, int y0, int x1, int y1, int w, uint16_t c) {      // trait épais (deux triangles)
  float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy); if (l < 1) return;
  float nx = -dy / l * w / 2.0f, ny = dx / l * w / 2.0f;
  cv.fillTriangle((int)(x0 + nx), (int)(y0 + ny), (int)(x0 - nx), (int)(y0 - ny), (int)(x1 + nx), (int)(y1 + ny), c);
  cv.fillTriangle((int)(x0 - nx), (int)(y0 - ny), (int)(x1 - nx), (int)(y1 - ny), (int)(x1 + nx), (int)(y1 + ny), c);
}
// Quand il s'énerve : de l'énergie jaillit vers le haut (blanc, puis orange, puis rouge).
void jetsEnergie(int cx, int yBase, float haut, int n, float ecartX) {
  uint32_t t = millis();
  for (int v = 0; v < n; v++) {
    int vx = cx + (int)((v - (n - 1) / 2.0f) * ecartX);
    for (int j = 0; j < 7; j++) {
      float ph = fmodf(t / 650.0f + j / 7.0f + v * 0.31f, 1.0f);
      int y = yBase - (int)(ph * (haut + 26)), x = vx + (int)(sinf(ph * 9.0f + v) * 4);
      uint16_t col = ph < 0.3f ? C(255, 255, 220) : (ph < 0.65f ? C(255, 170, 40) : C(220, 50, 30));
      cv.fillCircle(x, y, 1 + (int)((1.0f - ph) * 4), col);
    }
  }
}

// ---------- Personnage n°2 : Enclume (un bloc d'acier bleu, gros bras, poings carrés, il marche au sol) ----------
void dessinerOeufEnclume(int px, int py, float e) {        // son œuf à lui : un lingot d'acier riveté
  uint32_t t = millis(); int sec = (t / 350) % 2;
  int w = (int)(30 * e), h = (int)(36 * e), dx = (P.ageMin >= 8) ? (sec ? 2 : -2) : 0;
  uint16_t acier = C(70, 90, 130), bleu = C(40, 140, 255), clair = C(150, 225, 255);
  cv.fillEllipse(px + dx, py + h + 2, w + 6, 5, C(40, 10, 60));
  cv.fillRoundRect(px + dx - w, py - h, w * 2, h * 2, (int)(10 * e), acier); cv.drawRoundRect(px + dx - w, py - h, w * 2, h * 2, (int)(10 * e), bleu);
  for (int i = -1; i <= 1; i += 2) for (int j = -1; j <= 1; j += 2) cv.fillRect(px + dx + i * (w - 6) - 2, py + j * (h - 6) - 2, 4, 4, clair);   // rivets
  cv.drawFastHLine(px + dx - w + 4, py, w * 2 - 8, bleu);
  int l = 3 + (int)(3 * (1 + sinf(t / 300.0f)));
  cv.fillRect(px + dx - l, py - h / 2 - l / 2, l * 2, l, clair);                          // lumière qui bat dedans
  if (P.ageMin >= 6) { cv.drawLine(px + dx - 8, py - 10, px + dx - 2, py + 4, clair); cv.drawLine(px + dx - 2, py + 4, px + dx + 9, py - 6, clair); }   // il se fend
}
void dessinerEnclume(int px, int py, float e) {
  Phase ph = modeApercu ? ADULTE : phase();
  float u = 34.0f * e * tailleAge(); uint32_t t = millis();
  uint16_t neon = FA.couleurAuto ? C(40, 130, 255) : C(FA.r, FA.g, FA.b);
  if (FA.humeurCouleur) neon = couleurHumeur(P.attitude, neon);
  if (ph == ANCIEN) neon = ((neon >> 1) & 0x7BEF) + C(60, 60, 60);                        // l'ancien grisonne
  uint16_t mid = (neon >> 1) & 0x7BEF, dark = (neon >> 2) & 0x39E7, noir = C(12, 10, 22), blanc = C(255, 255, 255), pale = C(120, 120, 170);
  uint16_t bleu = C(40, 140, 255), bleuVif = C(150, 225, 255);
  bool enerve = (P.attitude == ENERVE), charge = W.charge;
  bool vitre = !modeApercu && contreVitre && millis() < miroirFin;
  bool assis = (geste == G_ASSIS) || (P.attitude == SOMMEIL);
  bool danse = !modeApercu && millis() < finDanse;
  bool marche = enMarche && !modeApercu && !assis;
  int nbEvents = (ph == BEBE) ? 0 : (ph == ENFANT) ? 1 : (ph == ADO) ? 2 : 3;
  if (enerve) { px += random(-2, 3); py += random(-1, 2); }
  if (P.attitude == FROID) px += ((t / 60) % 2) ? 1 : -1;                                  // il grelotte
  float pg = progGeste();
  float hs = constrain(1.0f - 0.6f * sqY + 0.25f * sqX, 0.55f, 1.3f) * (1.0f + 0.02f * sinf(t / 800.0f));
  float ws = constrain(1.0f + 0.45f * sqY - 0.5f * sqX, 0.75f, 1.35f);
  if (ph == BEBE) { hs *= 0.85f; ws *= 1.1f; }                                              // bébé : tout rond, tout trapu
  if (ph == ANCIEN) hs *= 0.92f;                                                            // l'ancien se tasse
  int yPied = py + (int)(0.9f * u);
  int feetH = max(3, (int)(0.2f * u));
  int hb = (int)(1.55f * u * hs);
  int yBot = yPied - (assis ? 2 : feetH);
  int yTop = yBot - hb;
  int W = (int)(0.95f * u * ws);
  int epa = (geste == G_EPAULES) ? (int)(0.12f * u * sinf(pg * 12.566f)) : 0;
  cv.fillEllipse(px, yPied + 2, (int)(W * 1.4f), max(3, (int)(6 * e)), C(40, 10, 60));   // ombre au sol
  if (charge) {                                   // en charge : une lumière bleue sort de son dos et de sa tête
    int cy0 = yTop + hb / 3;
    for (int i = 0; i < 9; i++) {
      float fc = (phaseCharge == 2) ? 1.0f : 0.45f;           // à fond : grands rayons ; en fin de charge : ils raccourcissent
      float a = 3.14159f + i * 0.3927f, l = u * fc * (0.6f + 0.4f * sinf(t / (phaseCharge == 2 ? 70.0f : 220.0f) + i * 1.7f)), ca = cosf(a), sa = sinf(a);
      epais(px + (int)(ca * W * 0.8f), cy0 + (int)(sa * hb * 0.55f), px + (int)(ca * (W * 0.8f + l)), cy0 + (int)(sa * (hb * 0.55f + l)), max(2, (int)(0.12f * u)), (i % 2) ? bleu : bleuVif);
    }
    for (int j = 0; j < 8; j++) {
      float q = fmodf(t / (phaseCharge == 2 ? 400.0f : 1300.0f) + j / 8.0f, 1.0f); int x = px + (int)((j - 3.5f) * W / 4.0f) + (int)(sinf(q * 8 + j) * 3), y = yTop - (int)(q * u * 1.4f), s = 2 + (int)((1 - q) * 4);
      cv.fillRect(x - s / 2, y - s / 2, s, s, q < 0.5f ? C(200, 240, 255) : bleu);
    }
  }
  float ps = sinf(pasPhase);                       // les pieds : un pas lourd après l'autre
  for (int s = -1; s <= 1; s += 2) {
    int fw = (int)(0.7f * u * ws), fx = px + s * (int)(0.5f * u * ws) + (assis ? s * (int)(0.25f * u) : 0);
    int up = (marche && ((s < 0) == (ps > 0))) ? (int)(4 * e * fabsf(ps)) : 0;
    if (danse && (int)((t / 300) % 2) == (s < 0 ? 0 : 1)) up = (int)(5 * e);
    cv.fillRoundRect(fx - fw / 2, yPied - feetH - up, fw, feetH, 3, mid); cv.drawRoundRect(fx - fw / 2, yPied - feetH - up, fw, feetH, 3, neon);
  }
  int rc = max(4, (int)((ph == BEBE ? 0.45f : 0.22f) * u));                               // le bloc d'un seul tenant
  cv.fillRoundRect(px - W, yTop, W * 2, hb, rc, mid);
  cv.drawRoundRect(px - W, yTop, W * 2, hb, rc, neon); cv.drawRoundRect(px - W + 1, yTop + 1, W * 2 - 2, hb - 2, rc, neon);
  if (ph != BEBE) { int ce = yTop + hb * 3 / 4; cv.drawFastHLine(px - W + 3, ce, W * 2 - 6, dark); cv.drawFastHLine(px - W + 3, ce + 1, W * 2 - 6, dark); cv.fillRect(px - 3, ce - 3, 6, 7, neon); }
  if (ph == ADO) for (int i = -1; i <= 1; i += 2) cv.fillTriangle(px + i * W, yTop + 4, px + i * (W + (int)(0.25f * u)), yTop - (int)(0.1f * u), px + i * (W - (int)(0.2f * u)), yTop, neon);   // ado : épaulettes en pointe
  int vy = yTop - max(3, (int)(0.16f * u)), vh = max(3, (int)(0.16f * u)), vw = max(6, (int)(0.3f * u));   // les évents (1 enfant, 2 ado, 3 adulte)
  for (int i = 0; i < nbEvents; i++) {
    int vx = px + (int)((i - (nbEvents - 1) / 2.0f) * W * 0.55f);
    cv.fillRect(vx - vw / 2, vy, vw, vh + 1, dark); cv.drawRect(vx - vw / 2, vy, vw, vh + 1, neon);
    if (charge) cv.fillRect(vx - vw / 2 + 2, vy + 1, vw - 4, vh - 1, ((t / 150 + i) % 2) ? bleuVif : bleu);
  }
  int fx = px - (int)(W * 0.74f), fw = (int)(W * 1.48f), fy = yTop + (int)(0.14f * u), fh = (int)(0.62f * u * hs);   // l'écran du visage
  cv.fillRoundRect(fx, fy, fw, fh, 3, noir); cv.drawRoundRect(fx, fy, fw, fh, 3, neon);
  int ey = fy + fh * 2 / 5 + (int)(regardY * 2 * e), ex = (int)(W * 0.38f), eo = max(3, (int)((ph == BEBE ? 0.2f : 0.14f) * u));
  int rg = (int)(regardX * eo * 0.8f) + ((geste == G_GRATTE) ? (int)(eo * 0.6f) : 0);
  bool clign = (t % 4200) < 140;
  switch (P.attitude) {
    case SOMMEIL:
      cv.drawFastHLine(px - ex - eo, ey, eo * 2, pale); cv.drawFastHLine(px + ex - eo, ey, eo * 2, pale);
      cv.setTextColor(C(220, 220, 255)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.drawString("z z", px + W / 2, yTop - 26); break;
    case ENERVE:
      cv.fillRect(px - ex - eo, ey - 1, eo * 2, eo + 1, neon); cv.fillRect(px + ex - eo, ey - 1, eo * 2, eo + 1, neon);
      epais(px - ex - eo - 2, ey - eo - 3, px - ex + eo + 2, ey - 3, 3, neon); epais(px + ex + eo + 2, ey - eo - 3, px + ex - eo - 2, ey - 3, 3, neon); break;
    case SURPRIS: case PEUR:
      cv.drawRect(px - ex - eo - 1, ey - eo - 1, eo * 2 + 3, eo * 2 + 3, neon); cv.drawRect(px + ex - eo - 1, ey - eo - 1, eo * 2 + 3, eo * 2 + 3, neon);
      cv.fillRect(px - ex - 1, ey - 1, 3, 3, blanc); cv.fillRect(px + ex - 1, ey - 1, 3, 3, blanc); break;
    case ETOURDI:
      for (int s = -1; s <= 1; s += 2) { int cx = px + s * ex; cv.drawLine(cx - 4, ey - 4, cx + 4, ey + 4, neon); cv.drawLine(cx - 4, ey + 4, cx + 4, ey - 4, neon); } break;
    default:
      if (charge) {                                // les yeux s'allument
        for (int s = -1; s <= 1; s += 2) { int cx = px + s * ex + rg; cv.drawCircle(cx, ey, eo + 6, bleu); cv.drawCircle(cx, ey, eo + 3, bleuVif); cv.fillRect(cx - eo, ey - eo, eo * 2, eo * 2 + 1, bleuVif); cv.fillRect(cx - eo / 2, ey - eo / 2, eo, eo, blanc); }
      } else if (P.attitude == CONTENT) { for (int s = -1; s <= 1; s += 2) { int cx = px + s * ex; cv.drawLine(cx - eo, ey + 3, cx, ey - 3, neon); cv.drawLine(cx, ey - 3, cx + eo, ey + 3, neon); } }
      else if (clign) { cv.drawFastHLine(px - ex - eo, ey, eo * 2, neon); cv.drawFastHLine(px + ex - eo, ey, eo * 2, neon); }
      else {
        cv.fillRect(px - ex - eo + rg, ey - eo, eo * 2, eo * 2 + 2, neon); cv.fillRect(px + ex - eo + rg, ey - eo, eo * 2, eo * 2 + 2, neon);
        cv.fillRect(px - ex - 1 + rg, ey - 1, 3, 3, blanc); cv.fillRect(px + ex - 1 + rg, ey - 1, 3, 3, blanc);
        if (P.attitude == TRISTE) { epais(px - ex - eo - 2, ey - eo - 2, px - ex + eo + 2, ey - eo - 6, 2, neon); epais(px + ex + eo + 2, ey - eo - 2, px + ex - eo - 2, ey - eo - 6, 2, neon); }
      }
  }
  if (ph == ANCIEN) { epais(px - ex - eo - 2, ey - eo - 5, px - ex + eo + 2, ey - eo - 4, 3, C(230, 230, 240)); epais(px + ex - eo - 2, ey - eo - 4, px + ex + eo + 2, ey - eo - 5, 3, C(230, 230, 240)); }   // gros sourcils blancs
  int my = fy + fh - max(5, fh / 5);
  if (ph == ANCIEN) { cv.fillRect(px - 10, my - 3, 20, 4, C(230, 230, 240)); }                                                       // moustache
  else if (millis() < finRepas || P.attitude == AFFAME) cv.fillRect(px - 6, my - 1, 12, 6 + (int)((t / 150) % 2) * 3, neon);
  else switch (P.attitude) {
    case ENERVE: for (int i = 0; i < 4; i++) cv.drawLine(px - 10 + i * 5, my + ((i % 2) ? 0 : 4), px - 5 + i * 5, my + ((i % 2) ? 4 : 0), blanc); break;
    case SOMMEIL: cv.drawFastHLine(px - 5, my + 2, 10, pale); break;
    case TRISTE: cv.drawFastHLine(px - 6, my + 4, 12, neon); cv.drawFastHLine(px - 4, my + 3, 8, neon); break;
    case SURPRIS: case PEUR: cv.fillRect(px - 4, my - 2, 8, 9, neon); break;
    default: if (charge || P.attitude == CONTENT) cv.fillArc(px, my - 4, 6, 11, 20, 160, neon); else cv.drawFastHLine(px - 6, my + 2, 12, neon);
  }
  if (P.attitude == CHAUD) cv.fillRect(px - ex - eo - 3, ey - 4, ex * 2 + eo * 2 + 6, 9, C(255, 40, 170));   // lunettes de soleil
  // les bras : très épais, avec des poings carrés énormes
  int aw = max(5, (int)((ph == BEBE ? 0.35f : 0.5f) * u)), fs = max(8, (int)((ph == BEBE ? 0.55f : 0.82f) * u));
  int chest = yTop + hb / 2;
  for (int s = -1; s <= 1; s += 2) {
    int sx = px + s * (W - 2), sy = yTop + (int)(0.34f * u) + epa;
    int fxp = px + s * (W + (int)(0.4f * u)), fyp = yBot - (int)(0.2f * u);
    if (marche) fyp -= (int)(0.15f * u * sinf(pasPhase) * s); else fyp += (int)(2 * sinf(t / 700.0f + s));
    if (assis) { fyp = yPied - fs / 2; fxp = px + s * (W + (int)(0.45f * u)); }
    if (vitre) { fxp = px + s * (int)(W * 0.72f); fyp = yTop + hb / 3; }
    else if (enerve) { bool haut = (int)((t / 180) % 2) == (s < 0 ? 0 : 1); fxp = px + s * (W + (int)(0.25f * u)); fyp = haut ? yTop - (int)(0.15f * u) : chest; }
    else if (danse) { bool haut = (int)((t / 300) % 2) == (s < 0 ? 0 : 1); fxp = px + s * (W + (int)(0.3f * u)); fyp = haut ? yTop - (int)(0.2f * u) : chest; }
    else if (charge) { fxp = px + s * (W + (int)(0.2f * u)); fyp = yTop - (int)(0.05f * u) - (int)(4 * e * fabsf(sinf(t / 250.0f))); }
    else if (geste == G_POINGS) { float k = (pg < 0.5f) ? pg * 2.0f : (pg > 0.75f ? (1.0f - pg) * 4.0f : 1.0f); fxp = px + s * (int)((W + 0.4f * u) * (1.0f - k) + fs * 0.55f * k); fyp = chest - (int)(0.1f * u * k); }
    else if (geste == G_CROISE) { fxp = px - s * (int)(W * 0.45f); fyp = chest + (int)(0.1f * u) + s * 3; }
    else if (geste == G_GRATTE && s > 0) { fxp = px + (int)(W * 0.45f) + (int)(4 * e * sinf(t / 70.0f)); fyp = yTop - (int)(0.12f * u); }
    else if (geste == G_CACHE) { fxp = px + s * ex; fyp = ey; }
    else if (P.attitude == PLUIE && s > 0) { fxp = px + W + (int)(0.15f * u); fyp = yTop - (int)(0.1f * u); }
    epais(sx, sy, fxp, fyp, aw, neon); epais(sx, sy, fxp, fyp, max(2, aw - 4), mid);
    int f2 = vitre ? fs * 7 / 5 : fs;                                                      // contre la vitre : les mains s'aplatissent
    if (vitre) cv.drawRect(fxp - f2 / 2 - 3, fyp - f2 / 2 - 3, f2 + 6, f2 + 6, C(150, 190, 230));
    cv.fillRect(fxp - f2 / 2, fyp - f2 / 2, f2, f2, neon); cv.fillRect(fxp - f2 / 2 + 2, fyp - f2 / 2 + 2, f2 - 4, f2 - 4, mid);
    for (int k = 1; k < 4; k++) cv.drawFastVLine(fxp - f2 / 2 + k * f2 / 4, fyp - f2 / 2 + 2, vitre ? f2 - 4 : f2 / 3, dark);
    if (vitre) cv.drawRect(fxp - f2 / 2, fyp - f2 / 2, f2, f2, blanc);
  }
  if (geste == G_POINGS && pg > 0.45f && pg < 0.62f) { for (int k = 0; k < 6; k++) { float a = k * 1.047f; cv.drawLine(px, chest, px + (int)(cosf(a) * u * 0.6f), chest + (int)(sinf(a) * u * 0.6f), C(255, 240, 160)); } }   // les poings se cognent
  if (P.attitude == PLUIE) { int ux = px + W + (int)(0.15f * u), uy = yTop - (int)(0.1f * u); cv.drawLine(ux, uy, ux, uy - 18, C(120, 120, 120)); cv.fillArc(ux, uy - 18, 0, (int)(1.1f * u), 180, 360, C(230, 190, 60));
    if (!modeApercu) for (int i = 0; i < 8; i++) cv.drawLine(20 + (i * 41 + millis() / 8) % 280, 40 + (i * 29 + millis() / 5) % 120, 18 + (i * 41 + millis() / 8) % 280, 48 + (i * 29 + millis() / 5) % 120, C(130, 170, 255)); }
  dessinerChapeau(px, vy, (int)(W * 0.8f));
  if (enerve) {
    jetsEnergie(px, vy, 1.3f * u, max(1, nbEvents), W * 0.55f);
    int on = (int)((t / 12) % 30); cv.drawEllipse(px, yPied + 3, W + on, 6 + on / 4, C(255, 120, 20));          // onde de choc au sol
  }
  if (P.attitude == ETOURDI) { for (int i = 0; i < 3; i++) { float a = millis() / 150.0f + i * 2.1f; cv.fillCircle(px + (int)(cosf(a) * 24), yTop - 14 + (int)(sinf(a) * 5), 3, C(255, 230, 90)); } }
  if (!modeApercu && millis() - poussiere < 350) { int k = (millis() - poussiere) / 40; for (int s = -1; s <= 1; s += 2) cv.drawCircle(px + s * (W + k * 2), yPied - 2, 2 + k, C(110, 100, 140)); }   // poussière à chaque pas
}

void dessinerBulle(int px, int py, float e);
void dessinerPersonnage(int px, int py, float e) {
  if (phase() == OEUF && !modeApercu) { if (FA.forme == 1) dessinerOeufEnclume(px, py, e); else dessinerOeuf(px, py, e); }
  else if (FA.forme == 1) dessinerEnclume(px, py, e);
  else dessinerBulle(px, py, e);
}

void dessinerBulle(int px, int py, float e) {
  Phase ph = modeApercu ? ADULTE : phase();
  float taille = (ph == BEBE) ? 0.6f : (ph == ENFANT) ? 0.8f : (ph == ADO) ? 0.95f : (ph == ADULTE) ? 1.1f : 1.0f;
  int r = (int)(34 * e * taille);
  uint16_t neon = FA.couleurAuto ? ((ph == BEBE) ? C(255, 80, 200) : (ph == ENFANT) ? C(255, 170, 40) : (ph == ADO) ? C(0, 220, 255) : (ph == ADULTE) ? C(80, 255, 140) : C(190, 120, 255)) : C(FA.r, FA.g, FA.b);
  if (FA.humeurCouleur) neon = couleurHumeur(P.attitude, neon);
  uint32_t t = millis();
  float respire = 1.0f + 0.04f * sinf(t / 600.0f);
  float ecrase = (P.attitude == ETOURDI) ? 0.85f + 0.15f * sinf(t / 90.0f) : 1.0f;
  ecrase *= constrain(1.0f - 0.7f * sqY + 0.3f * sqX, 0.45f, 1.3f);      // il s'écrase en ressort au moment du choc
  if (geste == G_ETIRE) ecrase *= 1.0f + 0.2f * sinf(progGeste() * 3.14159f);
  float largeur = constrain(1.0f + 0.5f * sqY - 0.6f * sqX, 0.7f, 1.4f);
  int rx = (int)(r * respire * largeur), ry = (int)(r * 0.92f * ecrase);
  int coin = max(4, r / 3);
  cv.fillEllipse(px, py + ry + 2, rx, (int)(7 * e), C(40, 10, 60));              // ombre lumineuse
  cv.fillEllipse(px, py + ry + 2, rx - 6, (int)(4 * e), C(90, 20, 120));
  int ah = max(6, (int)(r * 0.5f));                                               // antenne qui clignote
  cv.drawLine(px, py - ry, px, py - ry - ah, neon);
  cv.fillCircle(px, py - ry - ah, max(2, r / 9), ((t / 500) % 2) ? C(255, 40, 170) : neon);
  cv.fillRoundRect(px - rx - 5, py - ry / 3, 7, max(8, ry / 2), 2, neon); cv.fillRoundRect(px + rx - 2, py - ry / 3, 7, max(8, ry / 2), 2, neon);   // oreillettes
  cv.fillRoundRect(px - rx / 2 - r / 5, py + ry - 3, r / 2, max(5, r / 5), 2, neon); cv.fillRoundRect(px + rx / 2 - r / 5, py + ry - 3, r / 2, max(5, r / 5), 2, neon);   // pieds
  cv.fillRoundRect(px - rx, py - ry, rx * 2, ry * 2, coin, neon);                // coque néon
  cv.fillRoundRect(px - rx + 3, py - ry + 3, rx * 2 - 6, ry * 2 - 6, coin, C(12, 12, 30));   // écran du visage
  // visage
  int yeuY = py - ry / 6 + (int)(regardY * 3 * e), ecart = rx / 2;
  bool clignote = (t % 3500) < 130;
  int ouvert = max(2, (int)(r * 0.17f));
  uint16_t pale = C(120, 120, 170);
  switch (P.attitude) {
    case SOMMEIL:
      cv.drawFastHLine(px - ecart - ouvert, yeuY, ouvert * 2, pale); cv.drawFastHLine(px + ecart - ouvert, yeuY, ouvert * 2, pale);
      cv.setTextColor(C(220, 220, 255)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.drawString("z z", px + rx / 2, py - ry - 10);
      break;
    case SURPRIS: case PEUR:
      cv.drawCircle(px - ecart, yeuY, ouvert + 3, neon); cv.drawCircle(px + ecart, yeuY, ouvert + 3, neon);
      cv.fillCircle(px - ecart, yeuY, 2, C(255, 255, 255)); cv.fillCircle(px + ecart, yeuY, 2, C(255, 255, 255));
      break;
    case ENERVE:
      cv.fillRoundRect(px - ecart - ouvert, yeuY - 1, ouvert * 2, ouvert + 2, 2, neon); cv.fillRoundRect(px + ecart - ouvert, yeuY - 1, ouvert * 2, ouvert + 2, 2, neon);
      epais(px - ecart - ouvert - 3, yeuY - ouvert - 5, px - ecart + ouvert + 2, yeuY - 3, 3, neon); epais(px + ecart + ouvert + 3, yeuY - ouvert - 5, px + ecart - ouvert - 2, yeuY - 3, 3, neon);
      break;
    case ETOURDI:
      cv.drawLine(px - ecart - 5, yeuY - 5, px - ecart + 5, yeuY + 5, neon); cv.drawLine(px - ecart - 5, yeuY + 5, px - ecart + 5, yeuY - 5, neon);
      cv.drawLine(px + ecart - 5, yeuY - 5, px + ecart + 5, yeuY + 5, neon); cv.drawLine(px + ecart - 5, yeuY + 5, px + ecart + 5, yeuY - 5, neon);
      break;
    default:
      if (W.charge && P.attitude != SURPRIS) {   // yeux heureux en ^ ^
        cv.drawLine(px - ecart - ouvert, yeuY + 3, px - ecart, yeuY - 3, neon); cv.drawLine(px - ecart, yeuY - 3, px - ecart + ouvert, yeuY + 3, neon);
        cv.drawLine(px + ecart - ouvert, yeuY + 3, px + ecart, yeuY - 3, neon); cv.drawLine(px + ecart, yeuY - 3, px + ecart + ouvert, yeuY + 3, neon);
      } else if (clignote || geste == G_BAILLE) { cv.drawFastHLine(px - ecart - ouvert, yeuY, ouvert * 2, neon); cv.drawFastHLine(px + ecart - ouvert, yeuY, ouvert * 2, neon); }
      else {
        int big = (P.attitude == ECOUTE) ? ouvert + 2 : ouvert;
        int reg = ((geste == G_REGARDE) ? (int)(ouvert * 0.9f * sinf((t - debutGeste) / 330.0f)) : 0) + (int)(regardX * ouvert * 0.9f);
        cv.fillRoundRect(px - ecart + reg - big, yeuY - big, big * 2, big * 2 + 3, 2, neon); cv.fillRoundRect(px + ecart + reg - big, yeuY - big, big * 2, big * 2 + 3, 2, neon);
        cv.fillRect(px - ecart - 1 + reg, yeuY - 2, 3, 3, C(255, 255, 255)); cv.fillRect(px + ecart - 1 + reg, yeuY - 2, 3, 3, C(255, 255, 255));
      }
  }
  int boucheY = py + ry / 3;
  if (geste == G_BAILLE) { int bo = 3 + (int)(6 * sinf(progGeste() * 3.14159f)); cv.fillEllipse(px, boucheY + 2, 6, bo, neon); }
  else if (millis() < finRepas) {   // il mange
    cv.fillEllipse(px, boucheY + 2, 7, 3 + ((t / 150) % 2) * 4, neon);
  } else switch (P.attitude) {
    case CONTENT: cv.fillArc(px, boucheY - 4, 6, 12, 20, 160, neon); break;
    case TRISTE: cv.fillArc(px, boucheY + 8, 6, 11, 200, 340, neon); break;
    case AFFAME: cv.fillEllipse(px, boucheY + 2, 7, 6, neon); cv.fillEllipse(px, boucheY + 4, 3, 2, C(255, 40, 170)); break;
    case SURPRIS: case PEUR: cv.fillEllipse(px, boucheY + 2, 5, 7, neon); break;
    case SOMMEIL: cv.drawFastHLine(px - 5, boucheY, 10, pale); break;
    case ENERVE: for (int i = 0; i < 4; i++) cv.drawLine(px - 10 + i * 5, boucheY + ((i % 2) ? 1 : 5), px - 5 + i * 5, boucheY + ((i % 2) ? 5 : 1), neon); break;
    default: if (W.charge) cv.fillArc(px, boucheY - 4, 6, 12, 20, 160, neon); else cv.fillArc(px, boucheY - 2, 5, 9, 40, 140, neon);
  }
  if (geste == G_CACHE) { cv.fillRoundRect(px - ecart - ouvert - 5, yeuY - ouvert - 4, ouvert * 2 + 10, ouvert * 2 + 10, 4, neon); cv.fillRoundRect(px + ecart - ouvert - 5, yeuY - ouvert - 4, ouvert * 2 + 10, ouvert * 2 + 10, 4, neon); }   // il se cache les yeux
  if (P.attitude == FROID) { cv.fillRect(px - rx, py + ry / 2, rx * 2, 7, C(230, 70, 70)); cv.fillRect(px + rx / 2, py + ry / 2, 7, 18, C(230, 70, 70)); }
  if (P.attitude == CHAUD) { cv.fillRect(px - ecart - ouvert - 3, yeuY - 4, ecart * 2 + ouvert * 2 + 6, 9, C(255, 40, 170)); cv.drawFastHLine(px - ecart - ouvert - 3, yeuY - 3, ecart * 2 + ouvert * 2 + 6, C(255, 255, 255)); }
  if (P.attitude == PLUIE) { cv.fillArc(px, py - ry - 18, 0, 36, 180, 360, C(230, 190, 60)); cv.drawLine(px, py - ry - 18, px, py - ry + 4, C(120, 120, 120));
    if (!modeApercu) for (int i = 0; i < 8; i++) cv.drawLine(20 + (i * 41 + millis() / 8) % 280, 40 + (i * 29 + millis() / 5) % 120, 18 + (i * 41 + millis() / 8) % 280, 48 + (i * 29 + millis() / 5) % 120, C(130, 170, 255));
    if (!modeApercu && W.codeMeteo >= 95 && (millis() % 5000) < 160) { cv.drawLine(250, 20, 238, 60, C(255, 255, 200)); cv.drawLine(238, 60, 252, 62, C(255, 255, 200)); cv.drawLine(252, 62, 240, 104, C(255, 255, 200)); } }   // orage : un éclair de temps en temps
  if (!modeApercu && P.attitude == FROID && W.codeMeteo >= 71 && W.codeMeteo <= 86) { for (int i = 0; i < 10; i++) cv.fillCircle(20 + (i * 37 + (int)(8 * sinf(millis() / 700.0f + i))) % 280, 30 + (i * 23 + millis() / 30) % 140, 2, C(240, 245, 255)); }   // neige
  if (P.attitude == ETOURDI) { for (int i = 0; i < 3; i++) { float a = millis() / 150.0f + i * 2.1f; cv.fillCircle(px + (int)(cosf(a) * 22), py - ry - 10 + (int)(sinf(a) * 5), 3, C(255, 230, 90)); } }
  dessinerChapeau(px, py - ry, rx);
  if (P.attitude == ENERVE) jetsEnergie(px, py - ry - ah, r * 0.9f, 2, r * 0.5f);   // l'énergie jaillit vers le haut
}

void dessinerBarre() {
  // boutons du bas : Nourrir | Gravité | Réglages
  cv.fillRect(0, 200, 320, 40, C(8, 8, 20)); cv.drawFastHLine(0, 200, 320, C(255, 40, 170));
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center);
  const char* lib[3] = {"Nourrir", "Gravité", "Menu"};
  for (int i = 0; i < 3; i++) {
    int bx = 4 + i * 106;
    if (i == 1) {      // bouton coupé en deux : la gravité à gauche, le personnage à droite
      cv.fillRoundRect(bx, 204, 50, 32, 8, C(18, 20, 44)); cv.drawRoundRect(bx, 204, 50, 32, 8, C(0, 230, 255));
      cv.fillRoundRect(bx + 52, 204, 50, 32, 8, C(34, 20, 56)); cv.drawRoundRect(bx + 52, 204, 50, 32, 8, C(255, 40, 170));
      cv.setTextColor(C(240, 240, 255)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Gravité", bx + 25, 213);
      cv.fillCircle(bx + 25, 228, 4, W.gravite ? C(80, 255, 140) : C(255, 70, 90));
      cv.setFont(&fonts::lgfxJapanGothic_16); cv.drawString("Perso", bx + 77, 220); continue;
    }
    cv.fillRoundRect(bx, 204, 102, 32, 8, C(18, 20, 44)); cv.drawRoundRect(bx, 204, 102, 32, 8, C(0, 230, 255));
    cv.setTextColor(C(240, 240, 255)); cv.drawString(lib[i], bx + 51, 220);
  }
  cv.setTextDatum(top_left);
}

void dessinerInfos() {
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(220, 225, 255));
  String h = String(NOMS_PHASES[phase()]);
  if (phase() != OEUF) h = R.nom + " · " + h;
  cv.setTextDatum(top_left); cv.drawString(h, 4, 3);
  // jauges simples
  int x = 4, y = 18;
  auto jauge = [&](const char* nom, float v, uint16_t c) { cv.setTextColor(C(180, 185, 220)); cv.drawString(nom, x, y); cv.fillRect(x + 50, y + 2, 50, 7, C(50, 50, 80)); cv.fillRect(x + 50, y + 2, (int)(v / 2), 7, c); y += 12; };
  if (phase() != OEUF) { jauge("Faim", P.faim, C(240, 140, 80)); jauge("Joie", P.joie, C(250, 220, 90)); jauge("Énergie", P.energie, C(90, 200, 255)); }
  // icônes d'état en haut à droite (texte)
  cv.setTextDatum(top_right); int ry = 3;
  {   // la batterie est toujours affichée
    int bx = 292, by = ry; uint16_t cb = (W.batterie < 15 && !brancheUsb) ? C(255, 110, 110) : C(190, 240, 190), jaune = C(255, 230, 60);
    cv.drawRect(bx, by, 22, 11, cb); cv.fillRect(bx + 22, by + 3, 2, 5, cb);
    cv.fillRect(bx + 2, by + 2, (int)(18 * constrain(W.batterie, 0, 100) / 100.0f), 7, cb);
    if (W.charge) { cv.fillTriangle(bx + 12, by + 1, bx + 7, by + 6, bx + 12, by + 6, jaune); cv.fillTriangle(bx + 10, by + 5, bx + 15, by + 5, bx + 10, by + 10, jaune); }
    cv.setTextColor(cb); cv.drawString(String(W.batterie) + " %", 288, ry); ry += 14;
    if (W.charge) { cv.setTextColor(jaune); cv.drawString("En charge", 316, ry); ry += 14; }
  }
  if (R.actif[M_WIFI]) { cv.setTextColor(W.wifiOk ? C(190, 240, 190) : C(255, 160, 120)); cv.drawString(modeAcces ? "Réseau du Core2" : (W.wifiOk ? "Wi-Fi " + String(W.rssi) + " dBm" : "Pas de Wi-Fi"), 316, ry); ry += 14; }
  if (R.actif[M_METEO] && W.meteoValide) { cv.setTextColor(C(200, 220, 255)); cv.drawString(String((int)W.temperature) + " °C", 316, ry); ry += 14; }
  cv.setTextDatum(top_left);
  // bulle de message (frigo)
  if (R.actif[M_MESSAGES] && W.message.length() && (W.finMessage == 0 || millis() < W.finMessage)) {
    cv.fillRoundRect(30, 150, 260, 44, 10, C(250, 248, 230)); cv.drawRoundRect(30, 150, 260, 44, 10, C(120, 100, 60));
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(40, 30, 20)); cv.setTextDatum(middle_center);
    cv.drawString(W.message.substring(0, 36), 160, 172); cv.setTextDatum(top_left);
  } else if (parole.length() && millis() < finParole && page == 0) {
    cv.fillRoundRect(16, 160, 288, 30, 12, C(225, 245, 255)); cv.drawRoundRect(16, 160, 288, 30, 12, C(0, 150, 200));
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(20, 40, 70)); cv.setTextDatum(middle_center);
    cv.drawString(parole.substring(0, 34), 160, 175); cv.setTextDatum(top_left);
  }
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_center);
  if (W.batterie < 15 && !brancheUsb) { cv.setTextColor(C(255, 110, 110)); cv.drawString("Pile faible : branche-moi", 160, 3); }
  else if (versionDispo.length()) { cv.setTextColor(C(120, 255, 150)); cv.drawString("Mise à jour " + versionDispo + " : Réglages > Matériel", 160, 3); }
  else if (R.actif[M_HEURE] && W.heureValide) { struct tm th; if (getLocalTime(&th, 0)) { char hb[8]; snprintf(hb, sizeof(hb), "%02d:%02d", th.tm_hour, th.tm_min); cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(0, 230, 255)); cv.drawString(hb, 160, 2); cv.setFont(&fonts::lgfxJapanGothic_12); } }
  cv.setTextDatum(top_left);
  if (infoDemarrage.length() && millis() - debutDemarrage < 12000) {
    cv.fillRoundRect(10, 92, 300, 38, 8, C(120, 40, 40)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(middle_center);
    cv.drawString(infoDemarrage.substring(0, 48), 160, 111); cv.setTextDatum(top_left);
  }
}

void pousserAccueil() {          // en hauteur, l'image tourne avec l'écran
  if (rotEcran == 0) { cv.pushSprite(0, 0); return; }
  M5.Display.fillRect(0, 0, 70, 240, TFT_BLACK); M5.Display.fillRect(250, 0, 70, 240, TFT_BLACK);
  cv.pushRotateZoom(&M5.Display, 160, 120, (float)rotEcran, 0.75f, 0.75f);
}
void dessinerAccueil() {
  bool sol = (FA.forme == 1 && phase() != OEUF), dessus = (vue == 1 && rotEcran == 0);
  if (dessus) dessinerBoiteDessus(); else dessinerBoite();
  int px, py; float e; positionEcran(px, py, e);
  uint32_t m = millis();
  bool miroir = m < miroirFin && phase() != OEUF;
  float rgx = miroir ? constrain((miroirX - px) / 90.0f, -1.0f, 1.0f) : 0, rgy = miroir ? constrain((miroirY - py) / 70.0f, -1.0f, 1.0f) : 0;
  regardX += (rgx - regardX) * 0.3f; regardY += (rgy - regardY) * 0.3f;
  if (tremble > 0.3f) { px += (int)(random(-100, 101) / 100.0f * tremble); py += (int)(random(-100, 101) / 100.0f * tremble); }
  if (m < finDanse) {
    if (sol) py -= (int)(4 * e * fabsf(sinf(m / 150.0f)));
    else { py -= (int)(12 * e * fabsf(sinf(m / 170.0f))); px += (int)(10 * e * sinf(m / 340.0f)); }
    if (!veille && m - dernierBipDanse > 600) { dernierBipDanse = m; jouer(S_REVEIL); }
  }
  else if (geste == G_SAUT && !sol) { float pr = constrain((m - debutGeste) / (float)max(1UL, (unsigned long)(finGeste - debutGeste)), 0.0f, 1.0f); py -= (int)(16 * e * sinf(pr * 3.14159f)); }
  if (W.charge && !sol) { py -= (int)(3 * fabsf(sinf(m / 250.0f))); effetChargeAvant(px, py, e); }
  bool pesanteur = R.actif[M_GRAVITE] && W.gravite;
  if (!sol && !pesanteur && !W.charge && !miroir) { int rr = (int)(30 * e) + (int)(3 * sinf(m / 400.0f)); cv.drawEllipse(px, py + (int)(46 * e), rr, 5, C(0, 230, 255)); cv.drawEllipse(px, py + (int)(46 * e), rr - 8, 3, C(255, 40, 170)); }   // anneau de lévitation
  if (!sol && m < finVol) { int fy = py + (int)(38 * e), l = 8 + (int)((m / 40) % 8); cv.fillTriangle(px - 14, fy, px - 6, fy, px - 10, fy + l, C(255, 200, 60)); cv.fillTriangle(px + 6, fy, px + 14, fy, px + 10, fy + l, C(255, 200, 60)); }   // petits réacteurs quand il vole
  dessinerObjets(false); dessinerPersonnage(px, py, e); dessinerObjets(true);
  if (miroir && contreVitre) {     // contre la vitre : sa buée, et pour Bulle ses mains à plat
    int bx = px, by = sol ? py - (int)(14 * e) : py + (int)(12 * e), rr = (int)(12 * e) + (int)(3 * sinf(m / 600.0f));
    for (int k = 0; k < 18; k++) { float a = k * 0.349f; cv.fillCircle(bx + (int)(cosf(a) * rr), by + (int)(sinf(a) * rr * 0.6f), 1, C(170, 190, 225)); }
    if (!sol) { int r = (int)(34 * e * 1.1f), sz = r / 2; for (int s2 = -1; s2 <= 1; s2 += 2) { int hx = px + s2 * (int)(r * 0.95f), hy = py - r / 6; cv.fillRect(hx - sz / 2, hy - sz / 2, sz, sz, C(150, 200, 255)); cv.drawRect(hx - sz / 2, hy - sz / 2, sz, sz, C(255, 255, 255)); for (int k = 1; k < 3; k++) cv.drawFastVLine(hx - sz / 2 + k * sz / 3, hy - sz / 2 + 1, sz - 2, C(90, 130, 200)); } }
  }
  if (m < finCaresse) {
    if (!sol) { for (int i = 0; i < 3; i++) { int hx = px - 22 + i * 22, hy = py - (int)(44 * e) - (int)((1500 - (finCaresse - m)) / 40) - (i % 2) * 6; cv.fillCircle(hx - 3, hy, 3, C(255, 60, 150)); cv.fillCircle(hx + 3, hy, 3, C(255, 60, 150)); cv.fillTriangle(hx - 6, hy + 1, hx + 6, hy + 1, hx, hy + 8, C(255, 60, 150)); } }   // petits cœurs
    else { float u = 34.0f * e * tailleAge(); int top = py - (int)(0.85f * u); for (int i = 0; i < 3; i++) { int k = (int)((1500 - (finCaresse - m)) / 30) + i * 7; int hx = px + (i - 1) * (int)(0.5f * u), hy = top - k; cv.fillRect(hx - 3 - k / 10, hy - 3 - k / 10, 6 + k / 5, 6 + k / 5, C(180, 220, 255)); } }   // lui, il fume de contentement par ses évents
  }
  if (W.charge && !sol) effetChargeApres(px, py, e);
  if (brancheUsb && phase() != OEUF) {     // au-dessus de lui : la pile et ce que fait la charge
    int bx = px - 13, by = py - (int)(78 * e) - (sol ? (int)(10 * e) : 0); uint16_t cb = phaseCharge == 4 ? C(80, 255, 140) : (phaseCharge == 2 ? C(255, 230, 60) : C(0, 200, 255));
    cv.drawRect(bx, by, 24, 11, cb); cv.fillRect(bx + 24, by + 3, 2, 5, cb);
    int niv = phaseCharge == 4 ? 20 : (int)(20 * constrain(W.batterie, 0, 100) / 100.0f); if (phaseCharge == 2 && (m / 300) % 2) niv = min(20, niv + 3);
    cv.fillRect(bx + 2, by + 2, niv, 7, cb);
    cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_center); cv.setTextColor(cb);
    cv.drawString(phaseCharge == 4 ? "Pleine" : (phaseCharge == 2 ? "Charge à fond" : (phaseCharge == 3 ? "Ralentit (fin)" : (phaseCharge == 1 ? "Démarrage doux" : "Branché"))), px, by - 14); cv.setTextDatum(top_left);
  }
  if (m < finRepas && !dessus) for (int i = 0; i < 3; i++) cv.fillCircle(GAM_X + GAM_W / 2 + (i - 1) * 8, 146 - (int)((m / 90 + i * 5) % 14), 2, C(80, 255, 140));
  if (P.attitude == ENERVE) { uint16_t cr = ((m / 120) % 2) ? C(255, 40, 30) : C(140, 20, 20); cv.drawRect(0, 0, 320, 200, cr); cv.drawRect(1, 1, 318, 198, cr); }   // il voit rouge
  dessinerInfos(); dessinerBarre();
  pousserAccueil();
}

// ---------- Page de réglages à l'écran ----------
static bool direct = false, changerDirect = false, eco = false;
void appliquerDirect() {
  if (direct || modeAcces) { WiFi.mode(WIFI_AP_STA); WiFi.softAP("Core2-francais", "core2fran"); }
  else { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); }
  esp_wifi_set_country_code("FR", true);      // canaux 1 à 13 comme en France (certains téléphones utilisent le 12 ou le 13)
  if (nbConnus()) WiFi.begin(connuN[0].c_str(), connuM[0].c_str());
  WiFi.setSleep(eco || veille || !brancheUsb);     // économie : le Wi-Fi dort entre deux échanges
}
static const char* LIB_COURTS[M_NB] = {"Météo", "Wi-Fi", "Son", "Batterie", "Heure", "Gravité", "Messages"};
void dessinerReglages() {
  cv.fillScreen(C(18, 20, 34)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left); cv.setTextColor(C(255, 255, 255));
  cv.drawString("Modules : touche pour activer ou couper", 8, 6);
  cv.setFont(&fonts::lgfxJapanGothic_16);
  for (int i = 0; i < M_NB; i++) {
    int bx = 6 + (i % 2) * 158, by = 24 + (i / 2) * 30;
    cv.fillRoundRect(bx, by, 152, 26, 8, R.actif[i] ? C(50, 130, 80) : C(120, 50, 60));
    cv.setTextDatum(middle_left); cv.setTextColor(C(255, 255, 255)); cv.drawString(String(LIB_COURTS[i]) + (R.actif[i] ? " : oui" : " : non"), bx + 8, by + 13);
  }
  cv.fillRoundRect(164, 114, 152, 26, 8, C(50, 100, 150)); cv.setTextDatum(middle_left); cv.setTextColor(C(255, 255, 255)); cv.drawString("Matériel, mise à jour", 170, 127);
  cv.setTextDatum(top_left); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(190, 195, 230));
  String ip = (modeAcces || WiFi.status() != WL_CONNECTED) ? "192.168.4.1" : WiFi.localIP().toString();
  cv.drawString("Adresse : http://" + ip, 8, 148);
  cv.drawString("Version " VERSION_PROGRAMME "   Code maj : " + prefs.getString("code", "666666"), 8, 164);
  cv.drawString(W.wifiOk ? "Wi-Fi connecté : " + WiFi.SSID() : ((direct || modeAcces) ? "Direct : Core2-francais" : "Pas de Wi-Fi"), 8, 180);
  cv.drawString("Pile " + String(W.batterie) + " %  " + String((int)M5.Power.getBatteryVoltage()) + " mV  " + String((int)M5.Power.getBatteryCurrent()) + " mA", 8, 192);
  cv.fillRoundRect(236, 146, 80, 26, 8, C(50, 100, 150)); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString("Wi-Fi...", 276, 159);
  cv.fillRoundRect(236, 174, 80, 26, 8, eco ? C(50, 130, 80) : C(90, 70, 110)); cv.drawString(eco ? "Éco : oui" : "Éco : non", 276, 187); cv.setTextDatum(top_left);
  cv.setFont(&fonts::lgfxJapanGothic_16);
  cv.fillRoundRect(4, 204, 102, 32, 8, C(52, 56, 96)); cv.setTextDatum(middle_center); cv.setTextColor(C(240, 240, 255)); cv.drawString("Retour", 55, 220);
  cv.fillRoundRect(112, 204, 98, 32, 8, (direct || modeAcces) ? C(50, 130, 80) : C(90, 70, 110)); cv.drawString((direct || modeAcces) ? "Direct : oui" : "Direct : non", 161, 220);
  cv.fillRoundRect(216, 204, 100, 32, 8, C(120, 60, 60)); cv.drawString("Redémarrer", 266, 220);
  cv.setTextDatum(top_left);
  cv.pushSprite(0, 0);
}


// ---------- Réglage du Wi-Fi directement sur l'écran du Core2 ----------
static Reseau listeR[24]; static int nbR = 0, decalR = 0;
static bool scanEnCours = false; static uint32_t debutScan = 0;
static String mdpSaisi, nomChoisi; static bool majKb = false, symbKb = false, voirMdp = true;
static uint32_t debutConn = 0; static int resultatConn = 0;      // 0 en cours, 1 réussi, 2 échec
static uint32_t derniereTouche = 0;
static int emplacement = 1;      // 1 = Wi-Fi de la maison, 2 = Wi-Fi du téléphone (partage de connexion)
static uint8_t scanEssais = 0;
void arreterReconnexion() { WiFi.setAutoReconnect(false); WiFi.disconnect(false, false); delay(150); }
void lancerScan() { if (WiFi.status() != WL_CONNECTED) arreterReconnexion(); if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA); WiFi.scanDelete(); WiFi.scanNetworks(true, true); scanEnCours = true; debutScan = millis(); nbR = 0; decalR = 0; scanEssais = 0; }
void lireScan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) { if (millis() - debutScan > 15000) { scanEnCours = false; WiFi.setAutoReconnect(true); } return; }
  if (n <= 0) { if (scanEssais < 3) { scanEssais++; WiFi.scanDelete(); delay(200); WiFi.scanNetworks(true, true); debutScan = millis(); return; } scanEnCours = false; WiFi.setAutoReconnect(true); return; }
  scanEnCours = false; WiFi.setAutoReconnect(true);
  nbR = 0;
  for (int i = 0; i < n; i++) {
    String nom = WiFi.SSID(i); if (!nom.length()) continue;
    int j = -1; for (int k = 0; k < nbR; k++) if (listeR[k].nom == nom) j = k;
    if (j >= 0) { if (WiFi.RSSI(i) > listeR[j].dbm) { listeR[j].dbm = WiFi.RSSI(i); listeR[j].canal = WiFi.channel(i); } continue; }
    if (nbR < 24) { listeR[nbR].nom = nom; listeR[nbR].dbm = WiFi.RSSI(i); listeR[nbR].ferme = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN); listeR[nbR].canal = WiFi.channel(i); nbR++; }
  }
  for (int a = 0; a < nbR; a++) for (int b = a + 1; b < nbR; b++) if (listeR[b].dbm > listeR[a].dbm) { Reseau t = listeR[a]; listeR[a] = listeR[b]; listeR[b] = t; }
  WiFi.scanDelete();
}
void validerWifi() {
  if (emplacement == 2) { R.ssid2 = nomChoisi; R.mdp2 = mdpSaisi; } else { R.ssid = nomChoisi; R.motDePasse = mdpSaisi; }
  sauverReglages(); ajouterConnu(nomChoisi, mdpSaisi);
  WiFi.setAutoReconnect(true); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); delay(100);
  WiFi.disconnect(); WiFi.begin(nomChoisi.c_str(), mdpSaisi.c_str());
  debutConn = millis(); resultatConn = 0; page = 4;
}
static int construireClavier(Touche* t) {
  int n = 0;
  const char* ligne1 = symbKb ? "@#$%&*-_+=" : "azertyuiop";
  const char* ligne2 = symbKb ? "!?.,;:/()'" : "qsdfghjklm";
  const char* ligne3 = symbKb ? "~^<>\"|" : "wxcvbn";
  const char* lignes[4] = {"1234567890", ligne1, ligne2, ligne3};
  for (int r = 0; r < 3; r++) {
    for (int i = 0; i < 10; i++) { char c = lignes[r][i]; if (r > 0 && !symbKb && majKb) c = toupper(c); t[n++] = {i * 32, 44 + r * 31, 32, 29, String(c), (int)c}; }
  }
  t[n++] = {0, 137, 48, 29, symbKb ? "" : (majKb ? "MAJ" : "maj"), -1};
  for (int i = 0; i < 6; i++) { char c = ligne3[i]; if (!symbKb && majKb) c = toupper(c); t[n++] = {48 + i * 32, 137, 32, 29, String(c), (int)c}; }
  t[n++] = {240, 137, 80, 29, "Effacer", -2};
  t[n++] = {0, 168, 100, 29, symbKb ? "abc" : "#@!", -3};
  t[n++] = {104, 168, 212, 29, "espace", -4};
  t[n++] = {4, 202, 96, 34, "Annuler", -5};
  t[n++] = {108, 202, 100, 34, voirMdp ? "Masquer" : "Voir", -6};
  t[n++] = {216, 202, 100, 34, "Valider", -7};
  return n;
}
void dessinerWifi() {
  cv.fillScreen(C(18, 20, 34)); cv.setTextDatum(top_left); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(255, 255, 255));
  if (page == 2) {
    if (scanEnCours) lireScan();
    cv.fillRoundRect(4, 1, 312, 19, 6, C(70, 76, 130));
    { static uint32_t tRech = 0; if (!scanEnCours && millis() - tRech > 15000) { tRech = millis(); if (nbR == 0 || millis() - debutScan > 15000) lancerScan(); } }
    cv.drawString(emplacement == 2 ? "Pour : le téléphone (touche ici pour changer)" : "Pour : la maison (touche ici pour changer)", 10, 4);
    if (scanEnCours) cv.drawString("Recherche des réseaux… (2,4 GHz seulement)", 8, 60);
    else if (!nbR) cv.drawString("Aucun réseau trouvé : touche « Chercher »", 8, 60);
    for (int i = 0; i < 5; i++) {
      int k = decalR + i; if (k >= nbR) break; int y = 22 + i * 34;
      bool connu = indexConnu(listeR[k].nom) >= 0, actuel = WiFi.status() == WL_CONNECTED && WiFi.SSID() == listeR[k].nom;
      cv.fillRoundRect(4, y, 312, 30, 8, actuel ? C(30, 90, 60) : (connu ? C(40, 70, 110) : C(40, 46, 86)));
      String n = listeR[k].nom; if (n.length() > 17) n = n.substring(0, 16) + "…";
      cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_left); cv.setTextColor(C(255, 255, 255)); cv.drawString(n, 10, y + 15);
      int pc = constrain(2 * (listeR[k].dbm + 100), 0, 100);
      cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(middle_right);
      cv.setTextColor(pc > 60 ? C(120, 255, 150) : (pc > 35 ? C(255, 220, 90) : C(255, 130, 110)));
      cv.drawString((actuel ? String("connecté  ") : (connu ? String("connu  ") : String(""))) + String(pc) + " %" + (listeR[k].ferme ? "" : "  ouvert"), 310, y + 15);
    }
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(240, 240, 255));
    cv.fillRoundRect(4, 204, 90, 32, 8, C(52, 56, 96)); cv.drawString("Retour", 49, 220);
    cv.fillRoundRect(98, 204, 62, 32, 8, C(52, 56, 96)); cv.drawString("Haut", 129, 220);
    cv.fillRoundRect(164, 204, 62, 32, 8, C(52, 56, 96)); cv.drawString("Bas", 195, 220);
    cv.fillRoundRect(230, 204, 86, 32, 8, C(50, 100, 150)); cv.drawString("Chercher", 273, 220);
  } else if (page == 3) {
    cv.drawString("Réseau : " + nomChoisi, 6, 3);
    cv.fillRoundRect(4, 17, 312, 24, 6, C(10, 12, 26));
    String aff = voirMdp ? mdpSaisi : String(); if (!voirMdp) for (size_t i = 0; i < mdpSaisi.length(); i++) aff += "*";
    if (aff.length() > 24) aff = "…" + aff.substring(aff.length() - 23);
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_left); cv.setTextColor(C(255, 255, 255)); cv.drawString(aff + ((millis() / 500) % 2 ? "_" : ""), 10, 29);
    Touche t[60]; int n = construireClavier(t);
    cv.setTextDatum(middle_center);
    for (int i = 0; i < n; i++) {
      uint16_t col = t[i].code > 0 ? C(52, 56, 96) : (t[i].code == -7 ? C(50, 130, 80) : (t[i].code == -5 ? C(120, 60, 60) : C(70, 76, 130)));
      cv.fillRoundRect(t[i].x + 1, t[i].y + 1, t[i].w - 2, t[i].h - 2, 5, col);
      cv.setFont(t[i].code > 0 ? &fonts::lgfxJapanGothic_16 : &fonts::lgfxJapanGothic_12); cv.setTextColor(C(240, 240, 255));
      cv.drawString(t[i].lib, t[i].x + t[i].w / 2, t[i].y + t[i].h / 2);
    }
  } else {
    if (resultatConn == 0) { if (WiFi.status() == WL_CONNECTED) resultatConn = 1; else if (millis() - debutConn > 25000) resultatConn = 2; }
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(top_left); cv.setTextColor(C(255, 255, 255));
    cv.drawString("Réseau : " + nomChoisi, 10, 20);
    if (resultatConn == 0) { cv.drawString("Connexion en cours" + String("...").substring(0, (millis() / 400) % 4), 10, 60); }
    else if (resultatConn == 1) { cv.setTextColor(C(120, 255, 150)); cv.drawString("Connecté !", 10, 60); cv.setTextColor(C(255, 255, 255)); cv.drawString("Adresse : " + WiFi.localIP().toString(), 10, 90); cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Les mises à jour par Wi-Fi sont possibles.", 10, 120); }
    else { cv.setTextColor(C(255, 130, 110)); cv.drawString(WiFi.status() == WL_NO_SSID_AVAIL ? "Réseau introuvable" : "Échec de la connexion", 10, 60); cv.setTextColor(C(255, 255, 255)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString(WiFi.status() == WL_NO_SSID_AVAIL ? "Partage du téléphone : mets-le en 2,4 GHz." : "Vérifie le mot de passe, puis réessaie.", 10, 90); cv.drawString("Le Core2 ne voit pas le 5 GHz.", 10, 108); }
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center);
    cv.fillRoundRect(4, 204, 140, 32, 8, C(52, 56, 96)); cv.drawString(resultatConn == 2 ? "Réessayer" : "Réglages", 74, 220);
    cv.fillRoundRect(176, 204, 140, 32, 8, C(52, 56, 96)); cv.drawString("Accueil", 246, 220);
  }
  cv.setTextDatum(top_left);
  cv.pushSprite(0, 0);
}
void gererTactileWifi(bool appui, int x, int y) {
  if (!appui) return;
  if (page == 2) {
    if (y >= 204) {
      if (x < 96) page = 6;
      else if (x < 162) { if (decalR > 0) decalR--; }
      else if (x < 228) { if (decalR + 5 < nbR) decalR++; }
      else { lancerScan(); }
      bip(700, 40); return;
    }
    if (y < 21) { emplacement = (emplacement == 2) ? 1 : 2; bip(700, 40); return; }
    if (y >= 22 && y < 192) {
      int k = decalR + (y - 22) / 34;
      if (k < nbR) { nomChoisi = listeR[k].nom; int j = indexConnu(nomChoisi); mdpSaisi = j >= 0 ? connuM[j] : String(""); majKb = false; symbKb = false; bip(800, 50); if (j >= 0 || !listeR[k].ferme) validerWifi(); else page = 3; }
    }
  } else if (page == 3) {
    Touche t[60]; int n = construireClavier(t);
    for (int i = 0; i < n; i++) {
      if (x >= t[i].x && x < t[i].x + t[i].w && y >= t[i].y && y < t[i].y + t[i].h) {
        int c = t[i].code; bip(900, 25);
        if (c > 0) { if (mdpSaisi.length() < 63) mdpSaisi += (char)c; if (majKb && !symbKb) majKb = false; }
        else if (c == -1) majKb = !majKb;
        else if (c == -2) { if (mdpSaisi.length()) mdpSaisi.remove(mdpSaisi.length() - 1); }
        else if (c == -3) symbKb = !symbKb;
        else if (c == -4) { if (mdpSaisi.length() < 63) mdpSaisi += ' '; }
        else if (c == -5) page = 2;
        else if (c == -6) voirMdp = !voirMdp;
        else if (c == -7) validerWifi();
        return;
      }
    }
  } else {
    if (y >= 204) { if (x < 160) { if (resultatConn == 2) { mdpSaisi = ""; page = 3; } else page = 2; } else page = 0; bip(700, 40); }
  }
}

// ---------- Page « Matériel » : tout ce que le Core2 mesure, test du son, mise à jour par Internet ----------
void dessinerMateriel() {
  cv.fillScreen(C(18, 20, 34)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left);
  float gx = 0, gy = 0, gz = 0; if (M5.Imu.isEnabled()) M5.Imu.getGyro(&gx, &gy, &gz);
  int y = 4;
  cv.setTextColor(C(0, 230, 255)); cv.drawString("Matériel du Core2 - version " VERSION_PROGRAMME, 8, y); y += 16;
  cv.setTextColor(C(255, 255, 255));
  String l;
  l = "Pile : " + String(W.batterie) + " %   " + String((int)M5.Power.getBatteryVoltage()) + " mV   " + String((int)M5.Power.getBatteryCurrent()) + " mA"; cv.drawString(l, 8, y); y += 15;
  l = brancheUsb ? "Courant : branché" : "Courant : sur pile"; if (W.charge) l += ", en charge"; cv.drawString(l, 8, y); y += 15;
  if (W.wifiOk) l = "Wi-Fi : " + WiFi.SSID() + "  (" + String(W.rssi) + " dBm)"; else l = "Wi-Fi : non connecté"; cv.drawString(l, 8, y); y += 15;
  if (W.wifiOk) l = "Adresse : " + WiFi.localIP().toString(); else l = (direct || modeAcces) ? "Adresse : 192.168.4.1 (Wi-Fi direct)" : "Adresse : aucune"; cv.drawString(l, 8, y); y += 15;
  l = "Maison : "; l += R.ssid.length() ? R.ssid : String("(aucun)"); cv.drawString(l, 8, y); y += 15;
  l = "Téléphone : "; l += R.ssid2.length() ? R.ssid2 : String("(aucun)"); cv.drawString(l, 8, y); y += 15;
  l = "Inclinaison : " + String(ax, 2) + "  " + String(ay, 2) + "  " + String(az, 2); cv.drawString(l, 8, y); y += 15;
  l = "Rotation : " + String((int)gx) + "  " + String((int)gy) + "  " + String((int)gz); cv.drawString(l, 8, y); y += 15;
  l = "Micro : niveau " + String((int)W.niveauSon); cv.drawString(l, 8, y); y += 15;
  l = "Mémoire libre : " + String(ESP.getFreeHeap() / 1024) + " ko"; cv.drawString(l, 8, y); y += 15;
  cv.setTextColor(C(120, 255, 150));
  if (versionDispo.length()) l = "Mise à jour disponible : " + versionDispo; else if (infoMaj.length()) l = infoMaj; else l = "Mise à jour : pas encore vérifié";
  cv.drawString(l, 8, y);
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(240, 240, 255));
  cv.fillRoundRect(4, 204, 90, 32, 8, C(52, 56, 96)); cv.drawString("Retour", 49, 220);
  cv.fillRoundRect(98, 204, 96, 32, 8, C(52, 56, 96)); cv.drawString("Test son", 146, 220);
  cv.fillRoundRect(198, 204, 118, 32, 8, C(50, 130, 80)); cv.drawString(versionDispo.length() ? "Mettre à jour" : "Vérifier", 257, 220);
  cv.setTextDatum(top_left);
  cv.pushSprite(0, 0);
}

// ---------- Cadre commun des pages d'outils ----------
void cadrePage(const char* titre) {
  cv.fillScreen(C(10, 12, 26));
  cv.fillRect(0, 0, 320, 22, C(24, 28, 60)); cv.drawFastHLine(0, 22, 320, C(0, 230, 255));
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_left); cv.setTextColor(C(0, 230, 255)); cv.drawString(titre, 8, 11);
  cv.setTextDatum(top_left);
}
void boutonBas(int x, int w, const char* lib, uint16_t coul) {
  cv.fillRoundRect(x, 206, w, 30, 8, coul); cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString(lib, x + w / 2, 221); cv.setTextDatum(top_left);
}
struct Tuile { const char* nom; int page; uint16_t coul; };
static const Tuile TUILES[] = {
  {"Personnage", 0, 0x2C6B}, {"Radar Wi-Fi", 7, 0x0A5F}, {"Analyseur", 8, 0x8A0F},
  {"Détection", 9, 0x021F}, {"Météo", 10, 0x0C7A}, {"Horloge", 11, 0x6A1F},
  {"Niveau", 12, 0x05EB}, {"Sonomètre", 13, 0xFB40}, {"Lampe", 14, 0x8420},
  {"Réglages", 1, 0x4A4F}, {"Matériel", 5, 0x3269}, {"Apparence", 15, 0xC1E8}, {"Jeux", 20, 0x2D6B}, {"Wi-Fi", 2, 0x03B5},
};
static const int NB_TUILES = sizeof(TUILES) / sizeof(TUILES[0]);
void dessinerMenu() {
  cadrePage("Menu — tout ce que je sais faire");
  for (int i = 0; i < NB_TUILES; i++) {
    int c = i % 3, r = i / 3; int x = 6 + c * 104, y = 28 + r * 40;
    cv.fillRoundRect(x, y, 100, 36, 8, TUILES[i].coul); cv.drawRoundRect(x, y, 100, 36, 8, C(120, 160, 220));
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString(TUILES[i].nom, x + 50, y + 18);
  }
  cv.setTextDatum(top_left); cv.pushSprite(0, 0);
}
void dessinerRadarWifi() {
  cadrePage("Radar Wi-Fi");
  if (scanEnCours) lireScan();
  int cx = 110, cy = 120, R0 = 86;
  for (int k = 1; k <= 3; k++) cv.drawCircle(cx, cy, R0 * k / 3, C(30, 70, 60));
  cv.drawFastHLine(cx - R0, cy, R0 * 2, C(30, 70, 60)); cv.drawFastVLine(cx, cy - R0, R0 * 2, C(30, 70, 60));
  float bal = (millis() % 3000) / 3000.0f * 6.283f;
  cv.drawLine(cx, cy, cx + (int)(cosf(bal) * R0), cy + (int)(sinf(bal) * R0), C(0, 230, 120));
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left);
  for (int i = 0; i < nbR && i < 12; i++) {
    float d = distanceWifi(listeR[i].dbm); float rr = constrain(d / 30.0f, 0.05f, 1.0f) * R0;
    float ang = (i * 2.399f);
    int x = cx + (int)(cosf(ang) * rr), y = cy + (int)(sinf(ang) * rr);
    uint16_t c = listeR[i].dbm > -60 ? C(120, 255, 150) : (listeR[i].dbm > -72 ? C(255, 220, 90) : C(255, 120, 110));
    cv.fillCircle(x, y, 3, c);
    String n = listeR[i].nom; if (n.length() > 8) n = n.substring(0, 8);
    int tx = 206, ty = 28 + i * 15; cv.setTextColor(c); cv.drawString(n, tx, ty);
    cv.setTextColor(C(150, 160, 200)); cv.drawString(String(d, d < 10 ? 1 : 0) + " m", 290, ty);
  }
  cv.setTextColor(C(150, 160, 200)); cv.drawString(String(nbR) + " réseaux — distance estimée", 6, 196);
  boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(214, 102, scanEnCours ? "..." : "Chercher", C(50, 100, 150));
  cv.pushSprite(0, 0);
}
void dessinerAnalyseur() {
  cadrePage("Analyseur réseau (observation)");
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left);
  if (!sniffActif) {
    cv.setTextColor(C(220, 225, 255));
    cv.drawString("Observe le trafic Wi-Fi autour (comme un analyseur", 8, 34);
    cv.drawString("de réseau). Le Wi-Fi normal se met en pause pendant", 8, 50);
    cv.drawString("l'analyse, puis se reconnecte à l'arrêt.", 8, 66);
    boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(176, 140, "Démarrer l'analyse", C(50, 130, 80));
    cv.pushSprite(0, 0); return;
  }
  uint32_t g = sniffType[0], co = sniffType[1], da = sniffType[2];
  cv.setTextColor(C(0, 230, 255)); cv.drawString("Canal écouté : " + String(canalSniff) + "   " + String(sniffParSeconde) + " trames/s", 8, 30);
  cv.setTextColor(C(255, 255, 255));
  cv.drawString("Gestion : " + String(g), 8, 50); cv.drawString("Contrôle : " + String(co), 8, 66); cv.drawString("Données : " + String(da), 8, 82);
  cv.drawString("Total : " + String(sniffTotal), 8, 98);
  cv.drawString("Activité par canal :", 8, 118);
  uint32_t mx = 1; for (int c = 1; c <= 13; c++) if (sniffCanal[c] > mx) mx = sniffCanal[c];
  for (int c = 1; c <= 13; c++) {
    int x = 8 + (c - 1) * 23, h = (int)(60.0f * sniffCanal[c] / mx); if (h < 1 && sniffCanal[c]) h = 1;
    cv.fillRect(x, 192 - h, 18, h, c == canalSniff ? C(0, 230, 120) : C(80, 120, 200));
    cv.setTextColor(C(150, 160, 200)); cv.drawString(String(c), x + 4, 194);
  }
  boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(176, 140, "Arrêter l'analyse", C(130, 60, 60));
  cv.pushSprite(0, 0);
}
void dessinerDetection() {
  cadrePage("Détection de mouvement (Wi-Fi)");
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left);
  if (!detection) {
    cv.setTextColor(C(220, 225, 255));
    cv.drawString("Repère un mouvement (une personne qui bouge) en", 8, 34);
    cv.drawString("mesurant les perturbations des ondes Wi-Fi autour.", 8, 50);
    cv.drawString("Le Wi-Fi se met en pause pendant la mesure.", 8, 66);
    cv.drawString("Ce n'est pas une image : c'est un détecteur.", 8, 82);
    boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(176, 140, "Démarrer", C(50, 130, 80));
    cv.pushSprite(0, 0); return;
  }
  cv.fillRoundRect(8, 28, 304, 38, 8, mouvement ? C(120, 30, 40) : C(18, 48, 34));
  cv.setFont(&fonts::lgfxJapanGothic_24); cv.setTextDatum(middle_center);
  cv.setTextColor(mouvement ? C(255, 120, 120) : C(120, 255, 150));
  cv.drawString(mouvement ? "Mouvement détecté" : "Calme", 160, 47);
  cv.setTextDatum(top_left); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(150, 160, 200));
  cv.drawString("Canal " + String(canalDet) + "   agitation " + String(mvtNiveau, 1) + "  (repos " + String(mvtBase, 1) + ")", 8, 72);
  int base = 192;
  cv.drawFastHLine(8, base, 300, C(40, 60, 80));
  for (int i = 0; i < (int)sizeof(histMvt); i++) {
    int idx = (histMvtPos + i) % (int)sizeof(histMvt); int h = histMvt[idx];
    cv.drawFastVLine(8 + i, base - h, h, h > 40 ? C(255, 120, 110) : C(0, 200, 140));
  }
  boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(176, 140, "Arrêter", C(130, 60, 60));
  cv.pushSprite(0, 0);
}
void dessinerMeteoPage() {
  cadrePage("Météo");
  cv.setTextDatum(top_left);
  if (!W.meteoValide) { cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(220, 225, 255)); cv.drawString(W.wifiOk ? "Récupération de la météo…" : "Pas de Wi-Fi : météo indisponible.", 10, 60); }
  else {
    const char* desc = "Ciel clair"; int c = W.codeMeteo;
    if (c == 0) desc = "Ciel dégagé"; else if (c <= 2) desc = "Peu nuageux"; else if (c == 3) desc = "Couvert";
    else if (c <= 48) desc = "Brouillard"; else if (c <= 57) desc = "Bruine"; else if (c <= 67) desc = "Pluie";
    else if (c <= 77) desc = "Neige"; else if (c <= 82) desc = "Averses"; else if (c <= 86) desc = "Averses de neige"; else desc = "Orage";
    cv.setFont(&fonts::lgfxJapanGothic_24); cv.setTextDatum(top_center); cv.setTextColor(C(0, 230, 255)); cv.drawString(String(W.temperature, 1) + " °C", 160, 50);
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(240, 240, 255)); cv.drawString(desc, 160, 92);
    cv.setTextDatum(top_left); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(150, 160, 200));
    cv.drawString("Lieu : " + String(R.lat, 2) + ", " + String(R.lon, 2) + "  (réglable dans Réglages)", 8, 150);
    cv.drawString("Mis à jour il y a " + String((millis() - W.derniereMeteo) / 60000) + " min", 8, 170);
  }
  boutonBas(4, 100, "Menu", C(52, 56, 96)); boutonBas(214, 102, "Actualiser", C(50, 100, 150));
  cv.pushSprite(0, 0);
}
void dessinerHorloge() {
  cadrePage("Horloge");
  struct tm t; bool ok = getLocalTime(&t, 0);
  cv.setTextDatum(top_center);
  if (ok) {
    char h[16]; snprintf(h, sizeof(h), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    cv.setFont(&fonts::lgfxJapanGothic_40); cv.setTextColor(C(0, 230, 255)); cv.drawString(h, 160, 40);
    static const char* J[] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
    char d[40]; snprintf(d, sizeof(d), "%s %02d/%02d/%04d", J[t.tm_wday], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(240, 240, 255)); cv.drawString(d, 160, 92);
  } else { cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 160, 120)); cv.drawString("Heure en attente du Wi-Fi…", 160, 60); }
  uint32_t ch = chronoMarche ? (millis() - chronoDebut) : chronoFige;
  char cbuf[20]; snprintf(cbuf, sizeof(cbuf), "%02lu:%02lu.%lu", (unsigned long)(ch / 60000), (unsigned long)((ch / 1000) % 60), (unsigned long)((ch / 100) % 10));
  cv.setFont(&fonts::lgfxJapanGothic_24); cv.setTextColor(C(255, 230, 90)); cv.drawString(String("Chrono ") + cbuf, 160, 128);
  cv.setTextDatum(top_left);
  boutonBas(4, 100, "Menu", C(52, 56, 96));
  boutonBas(112, 96, chronoMarche ? "Arrêter" : "Départ", chronoMarche ? C(130, 60, 60) : C(50, 130, 80));
  boutonBas(214, 102, "Remettre 0", C(52, 56, 96));
  cv.pushSprite(0, 0);
}
void dessinerNiveau() {
  cadrePage("Niveau à bulle");
  int cx = 160, cy = 120, R0 = 78;
  cv.drawCircle(cx, cy, R0, C(80, 120, 200)); cv.drawCircle(cx, cy, 14, C(80, 120, 200));
  cv.drawFastHLine(cx - R0, cy, R0 * 2, C(40, 60, 110)); cv.drawFastVLine(cx, cy - R0, R0 * 2, C(40, 60, 110));
  int bx = cx + (int)(constrain(ax, -1.0f, 1.0f) * R0), by = cy + (int)(constrain(ay, -1.0f, 1.0f) * R0);
  bool plat = fabs(ax) < 0.03f && fabs(ay) < 0.03f;
  cv.fillCircle(bx, by, 12, plat ? C(120, 255, 150) : C(0, 230, 255));
  float angle = atan2f(sqrtf(ax * ax + ay * ay), fabs(az)) * 57.3f;
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(top_left); cv.setTextColor(C(240, 240, 255));
  cv.drawString("Inclinaison : " + String(angle, 1) + " deg", 8, 30);
  cv.setTextColor(plat ? C(120, 255, 150) : C(150, 160, 200)); cv.drawString(plat ? "À plat" : "Penché", 8, 48);
  boutonBas(4, 100, "Menu", C(52, 56, 96));
  cv.pushSprite(0, 0);
}
void dessinerSonometre() {
  cadrePage("Sonomètre");
  histSon[histSonPos] = (uint8_t)constrain((int)(W.niveauSon / 40.0f), 0, 150); histSonPos = (histSonPos + 1) % 220;
  for (int i = 0; i < 220; i++) {
    int idx = (histSonPos + i) % 220; int h = histSon[idx]; if (h > 150) h = 150;
    uint16_t c = h > 110 ? C(255, 90, 90) : (h > 60 ? C(255, 220, 90) : C(120, 255, 150));
    cv.drawFastVLine(50 + i, 190 - h, h, c);
  }
  cv.drawFastHLine(50, 190, 220, C(60, 70, 120));
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(top_left); cv.setTextColor(C(0, 230, 255));
  cv.drawString("Niveau : " + String((int)W.niveauSon), 8, 30);
  cv.setTextColor(C(150, 160, 200)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Micro — bruit autour", 8, 52);
  boutonBas(4, 100, "Menu", C(52, 56, 96));
  cv.pushSprite(0, 0);
}
void dessinerLampe() {
  int niv = lampeNiveau;
  cv.fillScreen(C(niv, niv, niv > 240 ? niv - 10 : niv));
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_left); cv.setTextColor(C(40, 40, 40)); cv.drawString("Lampe — glisse pour régler", 8, 6);
  int bw = (int)(300.0f * niv / 255.0f); cv.fillRoundRect(10, 206, 300, 16, 6, C(180, 180, 180)); cv.fillRoundRect(10, 206, bw, 16, 6, C(90, 90, 90));
  cv.fillRoundRect(4, 180, 90, 20, 6, C(60, 60, 60)); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.drawString("Menu", 49, 190); cv.setTextDatum(top_left);
  cv.pushSprite(0, 0);
}

// ---------- Pages d'apparence : personnage, couleurs, corps, caractères, réactions (l'aperçu vivant est toujours visible à droite) ----------
static const uint8_t PAL[10][3] = {{0, 0, 0}, {220, 45, 40}, {255, 140, 30}, {255, 220, 60}, {70, 230, 120}, {0, 220, 255}, {70, 110, 255}, {170, 100, 255}, {255, 70, 170}, {235, 235, 255}};
static const char* NOMS_PAL[10] = {"Aucune", "Rouge", "Orange", "Jaune", "Vert", "Cyan", "Bleu", "Violet", "Rose", "Blanc"};
static const char* NOMS_MILIEU[4] = {"Air", "Eau", "Huile", "Miel"};
static const uint8_t VAL_MILIEU[4] = {0, 35, 60, 90}, VAL_POIDS[5] = {10, 30, 50, 70, 95}, VAL_GRAV[4] = {50, 100, 140, 200}, VAL_SAUT[4] = {0, 50, 100, 200}, VAL_REB[4] = {10, 33, 60, 90};
static const char* NOMS_0G[3] = {"Flotte", "Chute lente", "Lune"};
static const int HUMS[6] = {ENERVE, TRISTE, FROID, CHAUD, CONTENT, PEUR};
static const char* NOMS_HUM[6] = {"Énervé", "Triste", "Froid", "Chaud", "Content", "Peur"};
static int suivantVal(const uint8_t* tab, int n, int val) { int i = 0, d = 999; for (int k = 0; k < n; k++) { int dd = abs((int)tab[k] - val); if (dd < d) { d = dd; i = k; } } return tab[(i + 1) % n]; }
static int indexVal(const uint8_t* tab, int n, int val) { int i = 0, d = 999; for (int k = 0; k < n; k++) { int dd = abs((int)tab[k] - val); if (dd < d) { d = dd; i = k; } } return i; }
static int palIdx(const uint8_t* c) { for (int i = 1; i < 10; i++) if (PAL[i][0] == c[0] && PAL[i][1] == c[1] && PAL[i][2] == c[2]) return i; return 0; }

void dessinerApercu() {       // l'encadré toujours visible : le personnage vit pendant qu'on règle
  cv.fillRoundRect(200, 28, 116, 172, 8, C(10, 8, 28)); cv.drawRoundRect(200, 28, 116, 172, 8, C(0, 230, 255));
  cv.setClipRect(202, 30, 112, 168);
  modeApercu = true;
  dessinerPersonnage(258 + (int)(P.x * 14), 124 + (int)(P.y * 8), 0.62f);
  modeApercu = false;
  cv.clearClipRect();
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(190, 200, 240)); cv.setTextDatum(top_center); cv.drawString(FA.nom, 258, 32); cv.setTextDatum(top_left);
}
static void ligneRegl(int y, const char* lib, const String& val, const uint8_t* pas) {
  cv.fillRoundRect(4, y, 192, 23, 6, C(34, 38, 78)); cv.drawRoundRect(4, y, 192, 23, 6, C(0, 150, 200));
  cv.setTextDatum(middle_left); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(190, 200, 240)); cv.drawString(lib, 10, y + 12);
  cv.setTextDatum(middle_right); cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 255, 255));
  if (pas) { cv.fillRoundRect(172, y + 4, 20, 15, 3, (pas[0] || pas[1] || pas[2]) ? C(pas[0], pas[1], pas[2]) : C(60, 60, 80)); cv.drawRoundRect(172, y + 4, 20, 15, 3, C(255, 255, 255)); cv.drawString(val, 168, y + 12); }
  else cv.drawString(val, 190, y + 12);
  cv.setTextDatum(top_left);
}
void dessinerApparence() {
  cadrePage("Mon personnage");
  uint8_t cr[3] = {FA.r, FA.g, FA.b};
  ligneRegl(28, "Personnage", String(FA.nom), nullptr);
  ligneRegl(53, "Couleur", FA.couleurAuto ? String("Auto") : String(palIdx(cr) ? NOMS_PAL[palIdx(cr)] : "Autre"), FA.couleurAuto ? PAL[0] : cr);
  ligneRegl(78, "Couleur selon l'humeur", FA.humeurCouleur ? "Oui" : "Non", nullptr);
  ligneRegl(103, "Corps, poids, gravité", ">", nullptr);
  ligneRegl(128, "Caractères", ">", nullptr);
  ligneRegl(153, "Aime / déteste", ">", nullptr);
  ligneRegl(178, "Couleurs des humeurs", ">", nullptr);
  dessinerApercu(); boutonBas(4, 100, "Menu", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void dessinerReactions() {
  cadrePage("Aime ou déteste ?");
  cv.setFont(&fonts::lgfxJapanGothic_12);
  for (int i = 0; i < EV_NB; i++) {
    int y = 28 + i * 15; int a = FA.aime[i];
    cv.fillRoundRect(4, y, 192, 14, 4, a > 0 ? C(30, 100, 60) : (a < 0 ? C(120, 40, 50) : C(40, 44, 80)));
    cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(middle_left); cv.drawString(NOMS_EV[i], 8, y + 7);
    cv.setTextDatum(middle_right); cv.drawString(a > 0 ? "Aime" : (a < 0 ? "Déteste" : "Neutre"), 192, y + 7);
  }
  cv.setTextDatum(top_left); dessinerApercu(); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void dessinerCaracteres() {
  cadrePage("Caractères (0 à 100)");
  for (int i = 0; i < 5; i++) {
    int y = 34 + i * 34;
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_left); cv.setTextColor(C(240, 240, 255)); cv.drawString(NOMS_CARACT[i], 6, y + 10);
    cv.fillRoundRect(84, y, 106, 20, 5, C(30, 34, 70)); cv.fillRoundRect(84, y, max(4, (int)(106 * FA.carac[i] / 100)), 20, 5, C(0, 170, 220));
    cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString(String(FA.carac[i]), 137, y + 10);
  }
  cv.setTextDatum(top_left); dessinerApercu(); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void dessinerCouleursHumeur() {
  cadrePage("Couleur selon l'humeur");
  for (int i = 0; i < 6; i++) { const uint8_t* c = FA.cHum[HUMS[i]]; int pi = palIdx(c); bool aucune = !c[0] && !c[1] && !c[2]; ligneRegl(28 + i * 25, NOMS_HUM[i], aucune ? String("Aucune") : String(pi ? NOMS_PAL[pi] : "Autre"), c); }
  dessinerApercu(); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void dessinerCorps() {
  cadrePage("Corps : poids et gravité");
  ligneRegl(28, "Poids", String(FA.poids), nullptr);
  ligneRegl(53, "Milieu", NOMS_MILIEU[indexVal(VAL_MILIEU, 4, FA.milieu)], nullptr);
  ligneRegl(78, "Force de la gravité", String(FA.gravite) + " %", nullptr);
  ligneRegl(103, "Sans gravité", NOMS_0G[constrain(FA.mode0g, 0, 2)], nullptr);
  ligneRegl(128, "Saut", FA.saut == 0 ? String("Aucun") : String(FA.saut) + " %", nullptr);
  ligneRegl(153, "Rebond", String(FA.rebond), nullptr);
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(190, 200, 240)); cv.drawString("Incline le Core2 pour voir l'effet", 6, 182);
  dessinerApercu(); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void gererTactileApparence(const m5::touch_detail_t& t) {
  int x = t.x, y = t.y; bool clic = t.wasPressed();
  if (clic && page == 15) retourSous = 15;
  if (clic && y >= 206 && x < 104) { page = (page == 15) ? pageRetourPerso : retourSous; bip(700, 40); return; }
  Fiche& f = FA;
  if (page == 17) {
    if (t.isPressed() && x >= 80 && x <= 194) for (int i = 0; i < 5; i++) { int ry = 34 + i * 34; if (y >= ry - 4 && y < ry + 26) f.carac[i] = constrain((x - 84) * 100 / 106, 0, 100); }
    if (t.wasReleased()) sauverFiche(R.perso);
    return;
  }
  if (!clic || x >= 200) return;
  bool modif = false;
  if (page == 15) {
    int l = (y - 28) / 25;
    if (l == 0) { R.perso = (R.perso + 1) % NB_PERSOS; sauverReglages(); bip(880, 40); }
    else if (l == 1) { uint8_t cr[3] = {f.r, f.g, f.b}; int n = f.couleurAuto ? 1 : (palIdx(cr) + 1) % 10; if (!f.couleurAuto && palIdx(cr) == 0) n = 1; if (n == 0) f.couleurAuto = true; else { f.couleurAuto = false; f.r = PAL[n][0]; f.g = PAL[n][1]; f.b = PAL[n][2]; } modif = true; }
    else if (l == 2) { f.humeurCouleur = !f.humeurCouleur; modif = true; }
    else if (l == 3) { page = 19; bip(700, 40); }
    else if (l == 4) { page = 17; bip(700, 40); }
    else if (l == 5) { page = 16; bip(700, 40); }
    else if (l == 6) { page = 18; bip(700, 40); }
  } else if (page == 16) {
    int l = (y - 28) / 15;
    if (l >= 0 && l < EV_NB) { f.aime[l] = (f.aime[l] == 0) ? 1 : ((f.aime[l] > 0) ? -1 : 0); modif = true; forcerReaction = true; reagir(l); forcerReaction = false; }
  } else if (page == 18) {
    int l = (y - 28) / 25;
    if (l >= 0 && l < 6) { int a = HUMS[l]; int n = (palIdx(f.cHum[a]) + 1) % 10; f.cHum[a][0] = PAL[n][0]; f.cHum[a][1] = PAL[n][1]; f.cHum[a][2] = PAL[n][2]; modif = true; attitude((Attitude)a, 4000); }
  } else if (page == 19) {
    int l = (y - 28) / 25;
    if (l == 0) f.poids = suivantVal(VAL_POIDS, 5, f.poids);
    else if (l == 1) f.milieu = suivantVal(VAL_MILIEU, 4, f.milieu);
    else if (l == 2) f.gravite = suivantVal(VAL_GRAV, 4, f.gravite);
    else if (l == 3) f.mode0g = (f.mode0g + 1) % 3;
    else if (l == 4) f.saut = suivantVal(VAL_SAUT, 4, f.saut);
    else if (l == 5) f.rebond = suivantVal(VAL_REB, 4, f.rebond);
    if (l >= 0 && l < 6) modif = true;
  }
  if (modif) { sauverFiche(R.perso); bip(880, 40); }
}


// ---------- Objets dans la pièce : une balle légère et une caisse lourde (ils se poussent selon le poids) ----------
struct Objet { float x, y, vx, vy, m, reb, drag; int rayon; };
Objet objs[2] = {{0.35f, -0.3f, 0, 0, 0.4f, 0.75f, 0.25f, 9}, {-0.45f, 0.55f, 0, 0, 2.2f, 0.12f, 1.4f, 13}};
void objetsPhysique(float dt) {
  if (!R.objets || phase() == OEUF || veille) return;
  for (int i = 0; i < 2; i++) {
    Objet& o = objs[i];
    o.vx += gSceneX * dt; o.vy += gSceneY * dt;
    float f = expf(-o.drag * dt); o.vx *= f; o.vy *= f;
    o.x += o.vx * dt; o.y += o.vy * dt;
    const float lim = 0.85f;
    if (o.x > lim) { o.x = lim; o.vx = -o.vx * o.reb; } if (o.x < -lim) { o.x = -lim; o.vx = -o.vx * o.reb; }
    if (o.y > lim) { o.y = lim; o.vy = -o.vy * o.reb; } if (o.y < -lim) { o.y = -lim; o.vy = -o.vy * o.reb; }
    float dx = (o.x - P.x) * 110, dy = (o.y - P.y) * 70, d = sqrtf(dx * dx + dy * dy), rr = 26 + o.rayon;   // choc avec le personnage
    if (d < rr && d > 0.01f) {
      float nx = dx / d, ny = dy / d, ms = masse();
      float vn = (o.vx - P.vx) * 110 * nx + (o.vy - P.vy) * 70 * ny;
      if (vn < 0) {
        float j = -1.5f * vn / (1.0f / o.m + 1.0f / ms);
        o.vx += (j / o.m * nx) / 110; o.vy += (j / o.m * ny) / 70;
        if (!P.porte) { P.vx -= (j / ms * nx) / 110; P.vy -= (j / ms * ny) / 70; }
        if (-vn > 40) { jouer(S_CHOC); tremble = max(tremble, min(3.0f, -vn / 60.0f)); }
      }
      float push = rr - d; o.x += nx * push / 110 * 0.8f; o.y += ny * push / 70 * 0.8f;
    }
  }
}
void dessinerObjets(bool devant) {
  if (!R.objets || phase() == OEUF) return;
  for (int i = 0; i < 2; i++) {
    const Objet& o = objs[i]; if ((o.y > P.y) != devant) continue;
    int sx = 160 + (int)(o.x * 110 * 0.84f), sy = 100 + (int)(o.y * 70 * 0.84f), r = o.rayon;
    cv.fillEllipse(sx, sy + r, r, 3, C(40, 10, 60));
    if (i == 0) { cv.fillCircle(sx, sy, r - 1, C(0, 200, 255)); cv.drawCircle(sx, sy, r - 1, C(255, 255, 255)); cv.fillCircle(sx - 3, sy - 3, 2, C(255, 255, 255)); }
    else { cv.fillRect(sx - r, sy - r, r * 2, r * 2, C(70, 40, 120)); cv.drawRect(sx - r, sy - r, r * 2, r * 2, C(255, 40, 170)); cv.drawLine(sx - r, sy - r, sx + r, sy + r, C(255, 40, 170)); cv.drawLine(sx + r, sy - r, sx - r, sy + r, C(255, 40, 170)); }
  }
}

// ---------- Chapeaux (gagnés avec les points des jeux) ----------
static const char* NOMS_CHAPEAU[4] = {"Aucun", "Casquette", "Couronne", "Haut-de-forme"};
static const uint16_t SEUIL_CHAPEAU[4] = {0, 20, 60, 120};
void dessinerChapeau(int px, int yTop, int w) {
  int h = FA.chapeau;
  if (h == 0 && !modeApercu && R.actif[M_METEO] && W.meteoValide && W.temperature < 8) {     // il fait froid dehors : bonnet à pompon
    cv.fillArc(px, yTop + 2, 0, w * 2 / 3, 180, 360, C(220, 50, 60)); cv.fillRect(px - w * 2 / 3, yTop - 2, w * 4 / 3, 5, C(240, 240, 250));
    cv.fillCircle(px, yTop - w * 2 / 3, max(3, w / 6), C(240, 240, 250)); return;
  }
  if (h <= 0 || h > 3) return;
  uint16_t rose = C(255, 40, 170), or_ = C(255, 215, 80), nuit = C(30, 30, 56);
  if (h == 1) { cv.fillRoundRect(px - w * 2 / 3, yTop - w / 2, w * 4 / 3, w / 2 + 2, 4, rose); cv.fillRect(px - w * 2 / 3, yTop - 2, w * 2, 4, rose); }
  else if (h == 2) { cv.fillRect(px - w / 2, yTop - w / 3, w, w / 3 + 1, or_); for (int i = -1; i <= 1; i++) cv.fillTriangle(px + i * w / 2 - w / 6, yTop - w / 3, px + i * w / 2 + w / 6, yTop - w / 3, px + i * w / 2, yTop - w * 2 / 3, or_); }
  else { cv.fillRect(px - w / 2, yTop - w * 3 / 4, w, w * 3 / 4, nuit); cv.drawRect(px - w / 2, yTop - w * 3 / 4, w, w * 3 / 4, rose); cv.fillRect(px - w * 3 / 4, yTop - 3, w * 3 / 2, 4, nuit); cv.drawFastHLine(px - w / 2, yTop - w / 5, w, rose); }
}

// ---------- Jeux : Attraper, Mémoire, Pierre-feuille-ciseaux ----------
void gagner(int n) { points += n; prefs.putUInt("pts", points); }
struct Chute { float x, y, v; uint8_t type; bool actif; };
static Chute chutes[6];
static int jAScore = 0, jAVies = 3, jAEtat = 0; static uint32_t jAPrec = 0, jAProchain = 0; static float jAx = 160;
void jeuAttraperDepart() { memset(chutes, 0, sizeof(chutes)); jAScore = 0; jAVies = 3; jAEtat = 1; jAx = 160; jAPrec = millis(); jAProchain = millis() + 600; }
void dessinerJeuAttraper() {
  uint32_t m = millis(); float dt = constrain((m - jAPrec) / 1000.0f, 0.0f, 0.05f); jAPrec = m;
  cadrePage("Attraper les étincelles");
  if (jAEtat == 1) {
    auto t = M5.Touch.getDetail();
    if (t.isPressed() && t.y < 200) jAx += (t.x - jAx) * 0.35f; else jAx += (-gxL) * 180.0f * dt;     // le doigt, ou l'inclinaison du Core2
    jAx = constrain(jAx, 24.0f, 296.0f);
    if (m > jAProchain) {
      for (int i = 0; i < 6; i++) if (!chutes[i].actif) { chutes[i].x = random(20, 300); chutes[i].y = 30; chutes[i].v = 60 + jAScore * 3 + random(40); chutes[i].type = (random(100) < 25) ? 1 : 0; chutes[i].actif = true; break; }
      jAProchain = m + max(350, 900 - jAScore * 15);
    }
    for (int i = 0; i < 6; i++) if (chutes[i].actif) {
      Chute& c = chutes[i]; c.y += c.v * dt;
      if (c.y > 150 && c.y < 190 && fabsf(c.x - jAx) < 28) {
        if (c.type == 0) { jAScore++; attitude(CONTENT, 600); jouer(S_MANGER); } else { jAVies--; attitude(ETOURDI, 700); jouer(S_CHOC); vibrer(80); }
        c.actif = false;
      } else if (c.y > 200) { if (c.type == 0) jAVies--; c.actif = false; }
    }
    if (jAVies <= 0) { jAEtat = 2; gagner(jAScore); jouer(S_PILE); attitude(FA.carac[3] >= 60 ? ENERVE : TRISTE, 2500); }
  }
  for (int i = 0; i < 6; i++) if (chutes[i].actif) {
    const Chute& c = chutes[i]; int cx = (int)c.x, cy = (int)c.y;
    if (c.type == 0) { cv.fillCircle(cx, cy, 7, C(80, 255, 140)); cv.fillCircle(cx, cy, 3, C(255, 255, 255)); }
    else { cv.fillCircle(cx, cy, 8, C(255, 50, 60)); cv.drawLine(cx - 4, cy - 4, cx + 4, cy + 4, C(255, 255, 255)); cv.drawLine(cx + 4, cy - 4, cx - 4, cy + 4, C(255, 255, 255)); }
  }
  modeApercu = true; dessinerPersonnage((int)jAx, 168, 0.5f); modeApercu = false;
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(top_right); cv.drawString("Score " + String(jAScore), 314, 28);
  for (int i = 0; i < 3; i++) cv.fillRoundRect(8 + i * 18, 30, 14, 12, 3, i < jAVies ? C(255, 60, 120) : C(60, 40, 60));
  cv.setTextDatum(middle_center);
  if (jAEtat != 1) {
    cv.fillRoundRect(40, 70, 240, 60, 10, C(24, 28, 70)); cv.drawRoundRect(40, 70, 240, 60, 10, C(0, 230, 255));
    cv.drawString(jAEtat == 0 ? "Verts : attrape. Rouges : évite." : "Fini ! +" + String(jAScore) + " points", 160, 90); cv.drawString("Touche l'écran pour jouer", 160, 112);
  }
  cv.setTextDatum(top_left); boutonBas(4, 100, "Jeux", C(52, 56, 96)); cv.pushSprite(0, 0);
}

static uint8_t jMSeq[30]; static int jMLen = 0, jMPos = 0, jMEtat = 0, jMStep = 0, jMLum = -1; static uint32_t jMT = 0;
static const uint8_t COUL_M[4][3] = {{0, 220, 255}, {255, 70, 170}, {70, 230, 120}, {255, 220, 60}};
static const int FREQ_M[4] = {440, 554, 660, 880};
void jeuMemoireDepart() { jMLen = 1; jMSeq[0] = random(4); jMPos = 0; jMEtat = 1; jMT = millis() + 600; jMStep = 0; jMLum = -1; }
void dessinerJeuMemoire() {
  uint32_t m = millis();
  cadrePage("Mémoire : répète la suite");
  if (jMEtat == 1 && m >= jMT) {
    if (jMLum < 0) { jMLum = jMSeq[jMStep]; jMT = m + 450; bip(FREQ_M[jMLum], 130); }
    else { jMLum = -1; jMStep++; jMT = millis() + 200; if (jMStep >= jMLen) { jMEtat = 2; jMPos = 0; } }
  } else if (jMEtat == 2 && jMLum >= 0 && m > jMT) jMLum = -1;
  for (int i = 0; i < 4; i++) {
    int x = 12 + (i % 2) * 152, y = 30 + (i / 2) * 84; bool allume = (jMLum == i);
    uint16_t c = C(COUL_M[i][0], COUL_M[i][1], COUL_M[i][2]);
    cv.fillRoundRect(x, y, 144, 78, 10, allume ? c : ((c >> 2) & 0x39E7)); cv.drawRoundRect(x, y, 144, 78, 10, c);
  }
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(middle_center);
  String msg = jMEtat == 0 ? "Touche pour jouer" : (jMEtat == 1 ? "Regarde..." : (jMEtat == 2 ? "À toi ! (" + String(jMPos) + "/" + String(jMLen) + ")" : "Perdu ! +" + String(max(0, jMLen - 1)) + " points"));
  cv.drawString(msg, 214, 222);
  cv.setTextDatum(top_left); boutonBas(4, 100, "Jeux", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void toucheMemoire(int x, int y) {
  if (jMEtat == 0 || jMEtat == 3) { jeuMemoireDepart(); return; }
  if (jMEtat != 2 || y < 30 || y >= 198) return;
  int i = (x >= 164 ? 1 : 0) + (y >= 114 ? 2 : 0);
  jMLum = i; jMT = millis() + 220; bip(FREQ_M[i], 110);
  if (i == jMSeq[jMPos]) {
    jMPos++;
    if (jMPos >= jMLen) { if (jMLen >= 29) { jMEtat = 3; gagner(jMLen); attitude(CONTENT, 3000); return; } jMSeq[jMLen++] = random(4); jMEtat = 1; jMStep = 0; jMT = millis() + 800; jMLum = -1; attitude(CONTENT, 1000); }
  } else { jMEtat = 3; gagner(max(0, jMLen - 1)); attitude(FA.carac[3] >= 60 ? ENERVE : TRISTE, 2500); jouer(S_PILE); }
}

static int jPJoueur = -1, jPCore = -1, jPSerie = 0;
void dessinerMain(int x, int y, int type, uint16_t c) {       // mains carrées : pierre, feuille, ciseaux
  uint16_t fonce = (c >> 2) & 0x39E7;
  if (type == 0) { cv.fillRect(x - 15, y - 13, 30, 28, c); for (int i = 1; i < 4; i++) cv.drawFastVLine(x - 15 + i * 7, y - 13, 12, fonce); }
  else if (type == 1) { cv.fillRect(x - 15, y, 30, 16, c); for (int i = 0; i < 4; i++) cv.fillRect(x - 15 + i * 8, y - 22, 6, 24, c); cv.fillRect(x - 21, y + 2, 6, 10, c); }
  else { epais(x - 4, y + 8, x - 14, y - 20, 7, c); epais(x + 4, y + 8, x + 14, y - 20, 7, c); cv.fillRect(x - 13, y + 4, 26, 14, c); cv.fillRect(x + 8, y - 4, 10, 8, c); }
}
static const char* NOMS_MAIN[3] = {"Pierre", "Feuille", "Ciseaux"};
void dessinerJeuPFC() {
  cadrePage("Pierre, feuille, ciseaux");
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255));
  cv.drawString(FA.nom, 80, 36); cv.drawString("Toi", 240, 36);
  if (jPJoueur >= 0) {
    dessinerMain(80, 80, jPCore, C(255, 140, 60)); dessinerMain(240, 80, jPJoueur, C(0, 220, 255));
    int r = (jPJoueur - jPCore + 3) % 3;
    cv.setTextColor(r == 1 ? C(80, 255, 140) : (r == 2 ? C(255, 90, 90) : C(255, 230, 90)));
    cv.drawString(r == 1 ? "Gagné ! +2" : (r == 2 ? "Perdu..." : "Égalité +1"), 160, 118);
  } else cv.drawString("Choisis ta main", 160, 90);
  cv.setTextColor(C(190, 200, 240)); cv.drawString("Série : " + String(jPSerie), 160, 140);
  for (int i = 0; i < 3; i++) { int x = 6 + i * 104; cv.fillRoundRect(x, 152, 100, 50, 8, C(34, 38, 78)); cv.drawRoundRect(x, 152, 100, 50, 8, C(0, 150, 200)); dessinerMain(x + 50, 176, i, C(0, 220, 255)); }
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(255, 255, 255));
  for (int i = 0; i < 3; i++) cv.drawString(NOMS_MAIN[i], 56 + i * 104, 196);
  cv.setTextDatum(top_left); boutonBas(4, 100, "Jeux", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void touchePFC(int x, int y) {
  if (y < 152 || y >= 204) return;
  int i = constrain((x - 6) / 104, 0, 2); jPJoueur = i; jPCore = random(3);
  int r = (i - jPCore + 3) % 3; bip(660 + i * 100, 60);
  if (r == 1) { gagner(2); jPSerie++; attitude(CONTENT, 2500); } else if (r == 0) gagner(1); else { jPSerie = 0; attitude(FA.carac[3] >= 60 ? ENERVE : TRISTE, 2500); }
}

static const char* REVEILS[5] = {"", "06:30", "07:00", "07:30", "08:00"};
void dessinerMenuJeux() {
  cadrePage("Jeux et chapeaux");
  ligneRegl(28, "Attraper les étincelles", ">", nullptr);
  ligneRegl(53, "Mémoire (la suite)", ">", nullptr);
  ligneRegl(78, "Pierre, feuille, ciseaux", ">", nullptr);
  ligneRegl(103, "Chapeau", NOMS_CHAPEAU[constrain(FA.chapeau, 0, 3)], nullptr);
  ligneRegl(128, "Objets dans la pièce", R.objets ? "Oui" : "Non", nullptr);
  ligneRegl(153, "Réveil du matin", R.reveil.length() ? R.reveil : String("Non"), nullptr);
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 230, 90)); cv.drawString("Points : " + String(points), 8, 182);
  dessinerApercu(); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void gererTactileJeux(const m5::touch_detail_t& t) {
  int x = t.x, y = t.y; bool clic = t.wasPressed(); if (!clic) return;
  if (y >= 206 && x < 104) { page = (page == 20) ? pageRetourPerso : 20; bip(700, 40); return; }
  if (page == 20) {
    if (x >= 200) return;
    int l = (y - 28) / 25;
    if (l == 0) { page = 21; jAEtat = 0; bip(700, 40); }
    else if (l == 1) { page = 22; jMEtat = 0; bip(700, 40); }
    else if (l == 2) { page = 23; jPJoueur = -1; bip(700, 40); }
    else if (l == 3) { int h = FA.chapeau; for (int k = 0; k < 4; k++) { h = (h + 1) % 4; if (points >= SEUIL_CHAPEAU[h]) break; } FA.chapeau = h; sauverFiche(R.perso); bip(880, 40); }
    else if (l == 4) { R.objets = !R.objets; sauverReglages(); bip(880, 40); }
    else if (l == 5) { int k = 0; for (int i = 0; i < 5; i++) if (R.reveil == REVEILS[i]) k = i; R.reveil = REVEILS[(k + 1) % 5]; sauverReglages(); bip(880, 40); }
  }
  else if (page == 21) { if (jAEtat != 1) jeuAttraperDepart(); }
  else if (page == 22) toucheMemoire(x, y);
  else if (page == 23) touchePFC(x, y);
}

// ---------- Menu rapide du personnage (bouton « Perso » de l'accueil) ----------
void dessinerMenuPerso() {
  cadrePage("Mon personnage");
  static const char* L[8] = {"Personnage", "Apparence et couleur", "Caractère", "Ce qu'il aime", "Couleurs d'humeur", "Corps et gravité", "Jeux et chapeaux", "Exercices du cerveau"};
  for (int i = 0; i < 8; i++) ligneRegl(26 + i * 22, L[i], i == 0 ? String(FA.nom) : String(">"), nullptr);
  dessinerApercu(); boutonBas(4, 100, "Accueil", C(52, 56, 96)); cv.pushSprite(0, 0);
}

// ---------- Exercices du cerveau, avec le Docteur Synapse ----------
struct Cerveau { uint16_t jour; uint8_t ex[5]; uint8_t libre; uint16_t jours[14]; uint8_t notes[14]; };
Cerveau CB;
uint16_t jourActuel() { struct tm th; if (W.heureValide && getLocalTime(&th, 0)) return (uint16_t)((th.tm_year % 100) * 366 + th.tm_yday + 1); return 1; }
void chargerCerveau() { memset(&CB, 0, sizeof(CB)); if (prefs.getBytesLength("cerv") == sizeof(Cerveau)) prefs.getBytes("cerv", &CB, sizeof(CB)); }
int noteDuJour(const uint8_t* ex) { int n = 0, s = 0; for (int i = 0; i < 5; i++) if (ex[i]) { n++; s += ex[i]; } return n ? s / n : 0; }
void noteCerveau(int ex, int score) {
  uint16_t j = jourActuel();
  if (CB.jour != j) {                       // nouveau jour : la note d'hier part dans la courbe
    int nj = noteDuJour(CB.ex);
    if (CB.jour && nj) { for (int i = 0; i < 13; i++) { CB.jours[i] = CB.jours[i + 1]; CB.notes[i] = CB.notes[i + 1]; } CB.jours[13] = CB.jour; CB.notes[13] = nj; }
    CB.jour = j; memset(CB.ex, 0, 5);
  }
  if (ex >= 0 && ex < 5 && score > CB.ex[ex]) CB.ex[ex] = constrain(score, 1, 100);   // on garde le meilleur du jour
  prefs.putBytes("cerv", &CB, sizeof(CB));
}
void dessinerSynapse(int cx, int cy, float e, int humeur) {      // un petit cerveau rose, lunettes rondes et blouse blanche
  uint32_t t = millis(); cy += (int)(3 * sinf(t / 500.0f));
  uint16_t rose = C(255, 150, 200), rosef = C(200, 90, 150), blanc = C(255, 255, 255), noir = C(20, 20, 30);
  cv.fillTriangle(cx - (int)(26 * e), cy + (int)(46 * e), cx + (int)(26 * e), cy + (int)(46 * e), cx, cy + (int)(12 * e), blanc);
  cv.fillRect(cx - (int)(26 * e), cy + (int)(46 * e), (int)(52 * e), (int)(8 * e), blanc);
  cv.drawLine(cx, cy + (int)(14 * e), cx, cy + (int)(54 * e), C(160, 160, 190));
  static const int8_t lx[7] = {-14, -4, 8, 16, -10, 4, 14}, ly[7] = {-6, -14, -12, -2, 6, 8, 6};
  int r = (int)(13 * e);
  for (int i = 0; i < 7; i++) cv.fillCircle(cx + (int)(lx[i] * e), cy + (int)(ly[i] * e), r, rose);
  cv.drawLine(cx, cy - (int)(24 * e), cx, cy - (int)(10 * e), rosef);
  cv.drawArc(cx - (int)(12 * e), cy - (int)(14 * e), (int)(6 * e), (int)(5 * e), 200, 340, rosef); cv.drawArc(cx + (int)(12 * e), cy - (int)(14 * e), (int)(6 * e), (int)(5 * e), 200, 340, rosef);
  int ey = cy - (int)(2 * e);
  bool clign = (t % 3000) < 120;
  for (int s = -1; s <= 1; s += 2) {
    int x = cx + s * (int)(9 * e);
    cv.fillCircle(x, ey, (int)(6 * e), blanc); cv.drawCircle(x, ey, (int)(7 * e), noir);
    if (clign) cv.drawFastHLine(x - 3, ey, 6, noir); else cv.fillCircle(x + (int)(2 * sinf(t / 900.0f)), ey, max(1, (int)(2 * e)), noir);
  }
  cv.drawLine(cx - (int)(2 * e), ey, cx + (int)(2 * e), ey, noir);
  int my = cy + (int)(10 * e);
  if (humeur == 1) cv.fillArc(cx, my - 3, 3, 7, 20, 160, rosef); else if (humeur == 2) cv.drawCircle(cx, my + 1, 3, rosef); else cv.drawFastHLine(cx - 4, my + 1, 8, rosef);
}
static const char* NOMS_EX[5] = {"Calcul rapide", "Mémoire des chiffres", "Deux coups avant", "Couleurs piégées", "Réflexes"};
static const char* INTRO_EX[5][2] = {
  {"10 calculs, le plus vite possible.", "Tape la réponse, puis OK."},
  {"Retiens les chiffres qui s'affichent,", "puis tape-les dans l'ordre."},
  {"Touche « Pareil ! » si la lettre est", "la même que deux lettres avant."},
  {"Touche la couleur de l'encre,", "pas le mot écrit."},
  {"Attends que l'écran devienne vert,", "puis touche le plus vite possible."}};
void dessinerMenuCerveau() {
  cadrePage("Exercices du cerveau");
  bool auj = (CB.jour == jourActuel());
  for (int i = 0; i < 5; i++) ligneRegl(26 + i * 22, NOMS_EX[i], (auj && CB.ex[i]) ? String(CB.ex[i]) : String("à faire"), nullptr);
  int nj = auj ? noteDuJour(CB.ex) : 0;
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 230, 90)); cv.setTextDatum(top_left);
  cv.drawString(nj ? "Note du jour : " + String(nj) : String("Note du jour : -"), 8, 140);
  cv.drawRect(8, 160, 188, 42, C(60, 70, 120));
  for (int i = 0; i < 14; i++) { int v = CB.notes[i]; if (!v) continue; int h = max(2, v * 36 / 100); cv.fillRect(12 + i * 13, 199 - h, 10, h, C(0, 200, 255)); }
  if (nj) { int h = max(2, nj * 36 / 100); cv.drawRect(12 + 13 * 13 + 13 - 13, 199 - h, 10, h, C(255, 230, 90)); }
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(150, 160, 200)); cv.drawString("Les 14 derniers jours", 12, 162);
  cv.fillRoundRect(200, 26, 116, 176, 8, C(10, 8, 28)); cv.drawRoundRect(200, 26, 116, 176, 8, C(0, 230, 255));
  dessinerSynapse(258, 92, 1.0f, nj >= 70 ? 1 : 0);
  static const char* CONSEILS[5][2] = {{"Un peu chaque", "jour, c'est mieux."}, {"Va vite,", "mais reste juste."}, {"On progresse dans", "ce qu'on pratique."}, {"Bouge et dors", "bien aussi !"}, {"Essaie de battre", "ton score d'hier."}};
  int c = (millis() / 6000) % 5;
  cv.setTextDatum(top_center); cv.setTextColor(C(255, 255, 255)); cv.drawString("Docteur Synapse", 258, 30);
  cv.setTextColor(C(200, 220, 255)); cv.drawString(CONSEILS[c][0], 258, 164); cv.drawString(CONSEILS[c][1], 258, 180); cv.setTextDatum(top_left);
  boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
// état commun des exercices
static int cxEtat = 0, cxN = 0, cxErr = 0, cxScore = 0, cxMax = 0, cxEssai = 0, cxLen = 0, cxPos = 0, cxHit = 0, cxCibles = 0, cxFa = 0, cxMot = 0, cxEncre = 0, cxFlash = 0;
static uint32_t cxT0 = 0, cxT1 = 0, cxSomme = 0, cxFlashFin = 0, cxTemps[5];
static long cxRep = 0; static String cxSaisie, cxQuestion, cxDetail; static char cxSeq[32]; static bool cxRepondu = false;
static const char* MOTS_C[4] = {"ROUGE", "BLEU", "VERT", "JAUNE"};
static const uint8_t COUL_C[4][3] = {{230, 40, 40}, {40, 110, 255}, {40, 200, 80}, {250, 220, 40}};
void flashEx(bool bon) { cxFlash = bon ? 1 : 2; cxFlashFin = millis() + 300; bip(bon ? 1200 : 220, 70); }
void finExercice(int score, const String& detail) { cxScore = constrain(score, 1, 100); cxDetail = detail; cxEtat = 2; noteCerveau(page - 26, cxScore); bip(cxScore >= 60 ? 1320 : 440, 160); }
void nouveauCalcul() {
  int op = random(3); long a, b;
  if (op == 0) { a = random(2, 51); b = random(2, 50); cxRep = a + b; cxQuestion = String(a) + " + " + String(b); }
  else if (op == 1) { a = random(10, 100); b = random(1, a); cxRep = a - b; cxQuestion = String(a) + " - " + String(b); }
  else { a = random(2, 10); b = random(2, 10); cxRep = a * b; cxQuestion = String(a) + " × " + String(b); }
  cxSaisie = "";
}
void nouvelleSuite() { for (int i = 0; i < cxLen; i++) cxSeq[i] = '0' + random(10); cxSeq[cxLen] = 0; cxT1 = millis() + 700; cxSaisie = ""; cxN = 1; }
void nouvellesLettres() {
  const char* L = "BCDFGHJK";
  for (int i = 0; i < 24; i++) { if (i >= 2 && random(100) < 30) cxSeq[i] = cxSeq[i - 2]; else { char c; do { c = L[random(8)]; } while (i >= 2 && c == cxSeq[i - 2]); cxSeq[i] = c; } }
  cxPos = 0; cxT1 = millis() + 800; cxRepondu = false; cxHit = cxCibles = cxFa = 0;
}
void nouveauStroop() { cxMot = random(4); if (random(100) < 70) { do { cxEncre = random(4); } while (cxEncre == cxMot); } else cxEncre = cxMot; cxT1 = millis(); }
void departExercice() {
  cxEtat = 1; cxN = 0; cxErr = 0; cxT0 = millis(); cxSomme = 0; cxFlashFin = 0;
  if (page == 26) nouveauCalcul();
  else if (page == 27) { cxLen = 3; cxMax = 0; cxEssai = 0; nouvelleSuite(); }
  else if (page == 28) nouvellesLettres();
  else if (page == 29) nouveauStroop();
  else { cxEssai = 0; cxPos = 0; cxT1 = millis() + random(1500, 4000); }   // réflexes : 0 attendre, 1 vert, 2 trop tôt
}
static void dessinerPave() {
  cv.fillRoundRect(60, 80, 200, 30, 6, C(10, 12, 26)); cv.drawRoundRect(60, 80, 200, 30, 6, C(0, 230, 255));
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString(cxSaisie + (((millis() / 500) % 2) ? "_" : ""), 160, 95);
  static const char* T[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Effacer", "OK"};
  for (int i = 0; i < 12; i++) {
    int x = 2 + (i % 6) * 53, y = 116 + (i / 6) * 44;
    cv.fillRoundRect(x, y, 51, 40, 6, i == 11 ? C(50, 130, 80) : (i == 10 ? C(120, 60, 60) : C(52, 56, 96)));
    cv.setFont(i >= 10 ? &fonts::lgfxJapanGothic_12 : &fonts::lgfxJapanGothic_16); cv.drawString(T[i], x + 25, y + 20);
  }
  cv.setTextDatum(top_left);
}
static int touchePave(int x, int y) { if (y < 116 || y >= 204) return -1; int i = ((y - 116) / 44) * 6 + constrain((x - 2) / 53, 0, 5); if (i < 9) return i + 1; if (i == 9) return 0; return i; }
void dessinerExercice() {
  int ex = page - 26; uint32_t m = millis();
  cadrePage(NOMS_EX[ex]);
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(middle_center);
  if (cxEtat == 0) {
    dessinerSynapse(160, 70, 0.9f, 0);
    cv.setTextColor(C(255, 255, 255)); cv.drawString(INTRO_EX[ex][0], 160, 140); cv.drawString(INTRO_EX[ex][1], 160, 162);
    cv.setTextColor(C(120, 255, 150)); cv.drawString("Touche l'écran pour commencer", 160, 188);
  } else if (cxEtat == 2) {
    dessinerSynapse(250, 80, 0.9f, cxScore >= 60 ? 1 : 2);
    cv.setTextColor(C(255, 230, 90)); cv.setTextSize(2); cv.drawString(String(cxScore), 100, 70); cv.setTextSize(1);
    cv.setTextColor(C(255, 255, 255)); cv.drawString("sur 100", 100, 100); cv.drawString(cxDetail, 160, 150);
    cv.setTextColor(C(120, 255, 150)); cv.drawString("Touche l'écran pour rejouer", 160, 188);
  } else if (page == 26) {
    cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Calcul " + String(cxN + 1) + " / 10", 160, 32);
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextSize(2); cv.drawString(cxQuestion + " = ?", 160, 58); cv.setTextSize(1);
    dessinerPave();
  } else if (page == 27) {
    if (cxN == 1) {
      int32_t el = (int32_t)(m - cxT1); int i = el >= 0 ? el / 1100 : -1;
      cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Retiens " + String(cxLen) + " chiffres", 160, 32);
      if (i >= cxLen) { cxN = 2; cxSaisie = ""; }
      else if (i >= 0 && (el % 1100) < 850) { cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextSize(4); cv.setTextColor(C(0, 230, 255)); cv.drawString(String(cxSeq[i]), 160, 115); cv.setTextSize(1); }
    }
    if (cxN == 2) { cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Tape les " + String(cxLen) + " chiffres dans l'ordre", 160, 40); dessinerPave(); }
  } else if (page == 28) {
    if (m >= cxT1 + 2500) {                    // fin d'une lettre : on compte
      bool cible = cxPos >= 2 && cxSeq[cxPos] == cxSeq[cxPos - 2];
      if (cible) { cxCibles++; if (cxRepondu) cxHit++; } else if (cxRepondu) cxFa++;
      cxPos++; cxRepondu = false; cxT1 += 2500;
      if (cxPos >= 24) { int non = max(1, 24 - cxCibles); int s = (int)(100.0f * ((float)cxHit / max(1, cxCibles) - (float)cxFa / non)); finExercice(s, String(cxHit) + " sur " + String(cxCibles) + " trouvées, " + String(cxFa) + " en trop"); }
    }
    if (cxEtat == 1) {
      cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Lettre " + String(min(cxPos + 1, 24)) + " / 24", 160, 32);
      cv.fillRoundRect(118, 44, 84, 84, 10, C(24, 28, 60)); cv.drawRoundRect(118, 44, 84, 84, 10, C(0, 230, 255));
      if (m >= cxT1 && m - cxT1 < 2000) { cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextSize(4); cv.drawString(String(cxSeq[cxPos]), 160, 88); cv.setTextSize(1); }
      cv.fillRoundRect(60, 140, 200, 56, 12, cxRepondu ? C(40, 70, 50) : C(50, 130, 80)); cv.setFont(&fonts::lgfxJapanGothic_16); cv.drawString("Pareil !", 160, 168);
    }
  } else if (page == 29) {
    cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString(String(cxN + 1) + " / 20", 160, 32);
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextSize(3); cv.setTextColor(C(COUL_C[cxEncre][0], COUL_C[cxEncre][1], COUL_C[cxEncre][2])); cv.drawString(MOTS_C[cxMot], 160, 92); cv.setTextSize(1);
    for (int i = 0; i < 4; i++) { int x = 4 + i * 79; cv.fillRoundRect(x, 150, 75, 50, 8, C(COUL_C[i][0], COUL_C[i][1], COUL_C[i][2])); cv.drawRoundRect(x, 150, 75, 50, 8, C(255, 255, 255)); }
  } else {
    if (cxPos == 0 && m >= cxT1) { cxPos = 1; cxT0 = m; }
    if (cxPos == 2 && m >= cxT1) { cxPos = 0; cxT1 = m + random(1500, 4000); }
    uint16_t fond = cxPos == 1 ? C(30, 170, 70) : (cxPos == 2 ? C(200, 120, 30) : C(120, 30, 40));
    cv.fillRoundRect(8, 28, 304, 172, 12, fond);
    cv.setTextColor(C(255, 255, 255)); cv.drawString(cxPos == 1 ? "Touche !" : (cxPos == 2 ? "Trop tôt ! On recommence." : "Attends le vert..."), 160, 104);
    cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Essai " + String(min(cxEssai + 1, 5)) + " / 5" + (cxEssai ? "   dernier : " + String(cxTemps[cxEssai - 1]) + " ms" : String("")), 160, 180);
  }
  if (m < cxFlashFin) { uint16_t c = cxFlash == 1 ? C(80, 255, 140) : C(255, 60, 60); cv.drawRect(0, 23, 320, 181, c); cv.drawRect(1, 24, 318, 179, c); cv.drawRect(2, 25, 316, 177, c); }
  cv.setTextDatum(top_left); boutonBas(4, 100, "Retour", C(52, 56, 96)); cv.pushSprite(0, 0);
}
void toucheExercice(int x, int y) {
  if (y >= 206 && x < 104) { page = 25; cxEtat = 0; bip(700, 40); return; }
  if (cxEtat != 1) { if (y < 204) departExercice(); return; }
  uint32_t m = millis();
  if (page == 26 || (page == 27 && cxN == 2)) {
    int k = touchePave(x, y); if (k < 0) return;
    int maxL = (page == 26) ? 5 : cxLen;
    if (k <= 9) { if ((int)cxSaisie.length() < maxL) cxSaisie += char('0' + k); bip(900, 20); return; }
    if (k == 10) { if (cxSaisie.length()) cxSaisie.remove(cxSaisie.length() - 1); bip(600, 20); return; }
    if (!cxSaisie.length()) return;
    if (page == 26) {
      bool bon = cxSaisie.toInt() == cxRep; flashEx(bon); if (!bon) cxErr++;
      if (++cxN >= 10) { int s = (m - cxT0) / 1000; finExercice(100 - max(0, s - 15) * 2 - cxErr * 8, String(s) + " s, " + String(cxErr) + (cxErr > 1 ? " erreurs" : " erreur")); }
      else nouveauCalcul();
    } else {
      bool bon = (cxSaisie == String(cxSeq)); flashEx(bon);
      if (bon) { cxMax = cxLen; cxEssai = 0; if (cxLen >= 12) { finExercice(100, "Record : 12 chiffres"); return; } cxLen++; nouvelleSuite(); }
      else if (++cxEssai >= 2) finExercice((cxMax - 2) * 12, cxMax ? "Record : " + String(cxMax) + " chiffres" : String("Retente, ça vient !"));
      else nouvelleSuite();
    }
  } else if (page == 28) {
    if (y >= 140 && y < 196 && x >= 60 && x < 260 && !cxRepondu && m >= cxT1) { cxRepondu = true; flashEx(cxPos >= 2 && cxSeq[cxPos] == cxSeq[cxPos - 2]); }
  } else if (page == 29) {
    if (y < 150 || y >= 204) return;
    int i = constrain((x - 4) / 79, 0, 3); bool bon = (i == cxEncre); flashEx(bon); if (!bon) cxErr++;
    cxSomme += m - cxT1;
    if (++cxN >= 20) { int moy = cxSomme / 20; finExercice(100 - max(0, moy - 700) / 15 - cxErr * 5, String(moy) + " ms en moyenne, " + String(cxErr) + (cxErr > 1 ? " erreurs" : " erreur")); }
    else nouveauStroop();
  } else if (page == 30) {
    if (y >= 204) return;
    if (cxPos == 0) { cxPos = 2; cxT1 = m + 900; bip(220, 80); }
    else if (cxPos == 1) {
      cxTemps[cxEssai++] = m - cxT0; bip(1200, 40);
      if (cxEssai >= 5) { uint32_t s[5]; memcpy(s, cxTemps, sizeof(s)); for (int a = 0; a < 5; a++) for (int b = a + 1; b < 5; b++) if (s[b] < s[a]) { uint32_t q = s[a]; s[a] = s[b]; s[b] = q; } finExercice(100 - max(0, (int)s[2] - 200) / 4, "Ton temps : " + String(s[2]) + " ms"); }
      else { cxPos = 0; cxT1 = m + random(1500, 4000); }
    }
  }
}
void gererTactilePersoCerveau(const m5::touch_detail_t& t) {
  if (!t.wasPressed()) return;
  int x = t.x, y = t.y;
  if (page == 24) {
    if (y >= 206 && x < 104) { page = 0; bip(700, 40); return; }
    if (x >= 200 || y < 26 || y >= 202) return;
    int l = (y - 26) / 22; bip(700, 40);
    if (l == 0) { R.perso = (R.perso + 1) % NB_PERSOS; sauverReglages(); return; }
    static const int DEST[8] = {0, 15, 17, 16, 18, 19, 20, 25};
    pageRetourPerso = 24; retourSous = 24; page = DEST[l]; return;
  }
  if (page == 25) {
    if (y >= 206 && x < 104) { page = 24; bip(700, 40); return; }
    if (x >= 200 || y < 26 || y >= 136) return;
    page = 26 + (y - 26) / 22; cxEtat = 0; bip(700, 40); return;
  }
  toucheExercice(x, y);
}

void dessinerPage() {
  switch (page) {
    case 0: dessinerAccueil(); break;
    case 1: dessinerReglages(); break;
    case 5: dessinerMateriel(); break;
    case 6: dessinerMenu(); break;
    case 7: dessinerRadarWifi(); break;
    case 8: dessinerAnalyseur(); break;
    case 9: dessinerDetection(); break;
    case 10: dessinerMeteoPage(); break;
    case 11: dessinerHorloge(); break;
    case 12: dessinerNiveau(); break;
    case 13: dessinerSonometre(); break;
    case 14: dessinerLampe(); break;
    case 15: dessinerApparence(); break;
    case 16: dessinerReactions(); break;
    case 17: dessinerCaracteres(); break;
    case 18: dessinerCouleursHumeur(); break;
    case 19: dessinerCorps(); break;
    case 20: dessinerMenuJeux(); break;
    case 21: dessinerJeuAttraper(); break;
    case 22: dessinerJeuMemoire(); break;
    case 23: dessinerJeuPFC(); break;
    case 24: dessinerMenuPerso(); break;
    case 25: dessinerMenuCerveau(); break;
    case 26: case 27: case 28: case 29: case 30: dessinerExercice(); break;
    default: dessinerWifi(); break;
  }
}
void gererTactilePages(const m5::touch_detail_t& t) {
  bool clic = t.wasPressed();
  int x = t.x, y = t.y;
  if (page >= 15 && page <= 19) { gererTactileApparence(t); return; }
  if (page >= 20 && page <= 23) { gererTactileJeux(t); return; }
  if (page >= 24 && page <= 30) { gererTactilePersoCerveau(t); return; }
  if (page == 6) {
    if (!clic) return;
    for (int i = 0; i < NB_TUILES; i++) { int c = i % 3, r = i / 3; int tx = 6 + c * 104, ty = 28 + r * 40; if (x >= tx && x < tx + 100 && y >= ty && y < ty + 36) { page = TUILES[i].page; pageRetourPerso = 6; retourSous = 15; if (page == 2) lancerScan(); bip(700, 40); return; } }
    return;
  }
  if (page == 14) { if (clic && y >= 180 && y < 206 && x < 100) { page = 6; bip(700, 40); } else if (t.isPressed()) lampeNiveau = constrain((int)(255.0f * x / 320.0f), 10, 255); return; }
  if (!clic) return;
  bool bas = (y >= 206);
  if (page == 7) { if (bas && x < 104) page = 6; else if (bas) lancerScan(); }
  else if (page == 8) { if (bas && x < 104) page = 6; else if (bas) { if (sniffActif) arreterSniff(); else demarrerSniff(); } }
  else if (page == 9) { if (bas && x < 104) page = 6; else if (bas) { if (detection) arreterDetection(); else demarrerDetection(); } }
  else if (page == 10) { if (bas && x < 104) page = 6; else if (bas) { W.derniereMeteo = 0; W.meteoValide = false; } }
  else if (page == 11) { if (bas && x < 104) page = 6; else if (bas && x < 208) { if (chronoMarche) { chronoFige = millis() - chronoDebut; chronoMarche = false; } else { chronoDebut = millis() - chronoFige; chronoMarche = true; } } else if (bas) { chronoFige = 0; chronoDebut = millis(); chronoMarche = false; } }
  else if (page == 12) { if (bas && x < 104) page = 6; }
  else if (page == 13) { if (bas && x < 104) page = 6; }
  if (page == 6) bip(700, 40);
}

// ---------- Veille de l'écran ----------
// Économie de pile : sur pile, le Wi-Fi dort entre deux échanges ; écran éteint, le processeur ralentit aussi.
void appliquerEconomie() { WiFi.setSleep(eco || veille || !brancheUsb); setCpuFrequencyMhz(veille ? 80 : 240); }
void dessinerGriffes(int px, int py) { for (int i = 0; i < 3; i++) { int x = px - 36 + i * 24; cv.drawLine(x, py - 45, x + 14, py + 12, C(255, 60, 60)); cv.drawLine(x + 1, py - 45, x + 15, py + 12, C(255, 60, 60)); } }
void dessinerPerso1() { int px, py; float e; positionEcran(px, py, e); dessinerPersonnage(px, py, e); if (millis() < finGriffe) dessinerGriffes(px, py); }
// Bulle fait un petit salut de la main avant de s'endormir.
void saluerAuRevoir() {
  if (FA.forme == 1) return;      // le salut de la main est le geste de Bulle
  int px, py; float e;
  for (int f = 0; f < 16; f++) {
    uint32_t t0 = millis();
    dessinerBoite(); positionEcran(px, py, e); dessinerPerso1();
    int ex = px + (int)(26 * e), ey = py - (int)(6 * e);
    float a = -1.15f + sinf(f * 0.85f) * 0.55f;
    int hx = ex + (int)(cosf(a) * 30 * e), hy = ey + (int)(sinf(a) * 30 * e);
    cv.drawLine(px + (int)(14 * e), py, ex, ey, C(0, 230, 255));
    cv.drawLine(ex, ey, hx, hy, C(0, 230, 255));
    cv.fillCircle(hx, hy, max(3, (int)(6 * e)), C(255, 210, 90));
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255));
    cv.drawString("A bientot !", 160, 28); cv.setTextDatum(top_left);
    cv.pushSprite(0, 0); esp_task_wdt_reset();
    while (millis() - t0 < 55) delay(2);
  }
  bip(700, 80); bip(500, 130);
}
void endormirEcran() {
  if (!veille) saluerAuRevoir();
  veille = true; M5.Display.setBrightness(0); M5.Display.sleep(); appliquerEconomie();
}
void reveillerEcran() {
  veille = false; appliquerEconomie(); M5.Display.wakeup(); M5.Display.setBrightness(R.luminosite); activite();
  for (int f = 0; f < 8; f++) {
    dessinerBoite(); dessinerPerso1();
    int px, py; float e; positionEcran(px, py, e);
    int r = 64 - f * 8; if (r > 0) { cv.drawCircle(px, py, r, C(0, 230, 255)); cv.drawCircle(px, py, r + 3, C(255, 40, 170)); }
    cv.pushSprite(0, 0); esp_task_wdt_reset(); delay(32);
  }
  jouer(S_REVEIL);
}

// ---------- Actions de la barre du bas (écran et boutons ronds sous l'écran) ----------
void nourrir() { P.faim = max(0.0f, P.faim - 35); P.joie = min(100.0f, P.joie + 6); attitude(CONTENT, 3000); finRepas = millis() + 4500; jouer(S_MANGER); }
void caresser() {
  static uint32_t derniere = 0; if (millis() - derniere < 500) return; derniere = millis();
  float jc = constrain(2.0f + (FA.carac[1] - 50) / 50.0f, 1.0f, 3.0f);
  P.joie = min(100.0f, P.joie + jc); attitude(CONTENT, 2500); finCaresse = millis() + 1500; jouer(S_CARESSE);
  if (millis() > finParole && !(W.message.length() && (W.finMessage == 0 || millis() < W.finMessage))) dire(phrase(1), 2500);
  if (FA.carac[4] >= 60 && phase() != OEUF) { geste = G_CACHE; debutGeste = millis(); finGeste = millis() + 2200; }
  reagir(EV_CARESSE);
}
void actionBarre(int b) {
  if (b == 0) { if (phase() != OEUF) nourrir(); else bip(300, 60); }
  else if (b == 1) { W.gravite = !W.gravite; prefs.putBool("grav", W.gravite); jouer(S_GRAVITE); }
  else { page = 6; bip(700, 40); }
}
void gererBoutons() {
  int b = M5.BtnA.wasPressed() ? 0 : (M5.BtnB.wasPressed() ? 1 : (M5.BtnC.wasPressed() ? 2 : -1));
  if (b < 0) return;
  activite();
  if (veille) { reveillerEcran(); return; }
  if (page == 0) { actionBarre(b); return; }
  bip(700, 40);        // sur les autres pages : gauche = retour, milieu = accueil, droite = réglages
  if (b == 0) page = (page >= 7) ? 6 : ((page == 3) ? 2 : ((page == 1 || page == 5 || page == 6) ? 0 : 1));
  else if (b == 1) page = 0;
  else page = 1;
}

// ---------- Tactile ----------
bool touchePerso = false; int xPrec = 0, yPrec = 0; uint32_t debutToucher = 0;
void gererTactile() {
  auto t = M5.Touch.getDetail();
  if (t.wasPressed() || t.isPressed()) { activite(); nbTouch++; }
  if (t.y >= 240) { if (t.wasReleased()) { P.porte = false; touchePerso = false; } return; }   // zone des boutons ronds : gérée par gererBoutons()
  if (page == 5) {
    if (t.wasPressed() && t.y >= 204) {
      if (t.x < 96) page = 1;
      else if (t.x < 196) { jouer(S_MANGER); vibrer(200); }
      else if (versionDispo.length()) demandeMaj = true;
      else { infoMaj = "Recherche d'une mise à jour…"; prochaineVerif = 0; }
      bip(700, 40);
    }
    return;
  }
  if (page >= 6) { gererTactilePages(t); return; }
  if (page >= 2) { gererTactileWifi(t.wasPressed(), t.x, t.y); return; }
  if (page == 1) {
    if (t.wasPressed()) {
      int x = t.x, y = t.y;
      for (int i = 0; i < M_NB; i++) { int bx = 6 + (i % 2) * 158, by = 24 + (i / 2) * 30; if (x >= bx && x < bx + 152 && y >= by && y < by + 26) { R.actif[i] = !R.actif[i]; sauverReglages(); bip(R.actif[i] ? 880 : 440, 60); } }
      if (x >= 164 && y >= 114 && y < 140) { page = 5; bip(700, 60); }
      else if (x >= 236 && y >= 146 && y < 172) { page = 2; lancerScan(); bip(700, 60); }
      else if (x >= 236 && y >= 174 && y < 202) { eco = !eco; prefs.putBool("eco", eco); changerDirect = true; bip(700, 60); }
      else if (y >= 204 && x < 108) { page = 0; }
      else if (y >= 204 && x < 212) { direct = !direct; prefs.putBool("direct", direct); changerDirect = true; bip(700, 60); }
      else if (y >= 204) { sauverPerso(); esp_restart(); }
    }
    return;
  }
  int px, py; float e; positionEcran(px, py, e);
  uint32_t m = millis();
  int x = t.x, y = t.y;
  if (rotEcran) { float th = rotEcran * 0.0174533f, dx = x - 160, dy = y - 120; x = 160 + (int)((dx * cosf(th) + dy * sinf(th)) / 0.75f); y = 120 + (int)((-dx * sinf(th) + dy * cosf(th)) / 0.75f); }
  bool dehors = (x < 0 || x >= 320 || y < 0 || y >= 240);
  bool sol = (FA.forme == 1 && phase() != OEUF);
  if (t.wasPressed() && !dehors) {
    if (y >= 204) { int b = constrain((x - 4) / 106, 0, 2); if (b == 1 && x >= 161) { pageRetourPerso = 24; page = 24; bip(700, 40); } else actionBarre(b); return; }
    if (hypot(x - px, y - py) < 64 * e) { touchePerso = true; debutToucher = m; xPrec = x; yPrec = y; }   // zone de toucher large : on l'attrape facilement
    else if (phase() != OEUF) { miroirFin = m + 6000; miroirX = x; miroirY = y; }                          // un doigt sur la vitre : il vient voir
  }
  if (t.isPressed() && !dehors) {
    if (touchePerso) {
      // Doigt posé plus d'une demi-seconde : on le soulève et il suit le doigt. Sinon, frotter = caresse.
      if (!P.porte && m - debutToucher > 500) { P.porte = true; attitude(SURPRIS, 600); }
      if (P.porte) {
        if (sol && vue == 1 && rotEcran == 0) { P.x = constrain((x - 160) / 130.0f, -0.78f, 0.78f); P.y = constrain((y - 100) / 80.0f, -0.78f, 0.78f); }
        else if (sol) { float fy = y + 0.9f * 34 * 0.8f * tailleAge() + 18; float d = constrain((197 - fy) / 33.0f, 0.0f, 1.0f); P.y = d * 1.56f - 0.78f; P.x = constrain((x - 160) / ((160 - 52 * d) * 0.82f) * 0.78f, -0.78f, 0.78f); }
        else { P.x = constrain((x - 160) / (110.0f * (1.0f - 0.45f * P.z)), -0.78f, 0.78f); P.y = constrain((y - 100) / (70.0f * (1.0f - 0.45f * P.z)), -0.78f, 0.78f); }
        P.vx = (x - xPrec) / 10.0f; P.vy = (y - yPrec) / 10.0f; if (sol) { P.vx *= 0.1f; P.vy *= 0.1f; }
      } else if (abs(x - xPrec) + abs(y - yPrec) > 3) caresser();
      xPrec = x; yPrec = y;
    } else if (m < miroirFin) { miroirX = x; miroirY = y; miroirFin = max(miroirFin, m + 3000); }
  }
  if (t.wasReleased()) {
    if (touchePerso && !P.porte) caresser();       // un simple toucher = une caresse (il ne bouge plus)
    if (touchePerso && P.porte && sol) choc(1.6f, true);   // on le repose : boum
    P.porte = false; touchePerso = false;
  }
}

// Pile vide : on prévient, on enregistre, et on éteint proprement.
void arretPileVide() {
  sauverPerso(); prefs.putInt("pile", 0);
  M5.Display.wakeup(); M5.Display.setBrightness(120); M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setFont(&fonts::lgfxJapanGothic_16); M5.Display.setTextColor(TFT_WHITE); M5.Display.setTextDatum(middle_center);
  M5.Display.drawString("Pile vide.", 160, 100); M5.Display.drawString("Branche-moi pour me recharger.", 160, 130);
  delay(4000); M5.Power.powerOff();
}

// Mise à jour par Internet : le Core2 va chercher lui-même la nouvelle version (Wi-Fi de la maison ou partage du téléphone).
void majDepuisInternet() {
  demandeMaj = false;
  if (!urlBinDispo.length() || WiFi.status() != WL_CONNECTED) { infoMaj = "Mise à jour impossible : pas d'Internet"; return; }
  if (veille) reveillerEcran();
  sauverPerso();
  M5.Display.fillScreen(TFT_BLACK); M5.Display.setFont(&fonts::lgfxJapanGothic_16); M5.Display.setTextColor(TFT_WHITE); M5.Display.setTextDatum(middle_center);
  M5.Display.drawString("Mise à jour " + versionDispo + " en cours…", 160, 90); M5.Display.drawRect(18, 128, 284, 24, TFT_WHITE);
  M5.Display.setTextDatum(top_left);
  esp_task_wdt_delete(NULL);
  WiFiClientSecure client; client.setInsecure();
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.onProgress([](int fait, int total) { static int dernier = -1; int p = total > 0 ? (int)(100LL * fait / total) : 0; if (p != dernier) { dernier = p; M5.Display.fillRect(20, 130, 280 * p / 100, 20, TFT_GREEN); } });
  t_httpUpdate_return r = httpUpdate.update(client, urlBinDispo);
  esp_task_wdt_add(NULL);
  if (r == HTTP_UPDATE_OK) {
    prefs.putInt("essais", 0);
    M5.Display.fillScreen(TFT_BLACK); M5.Display.setTextDatum(middle_center); M5.Display.drawString("Mise à jour installée. Redémarrage…", 160, 110);
    delay(1200); esp_restart();
  }
  infoMaj = "Échec de la mise à jour : " + httpUpdate.getLastErrorString();
  page = 5;
}

// ---------- Serveur web (réglages + mise à jour sans câble) ----------
String jsonEtat() {
  JsonDocument d;
  ajouterFiche(d);
  d["nom"] = R.nom; d["version"] = VERSION_PROGRAMME; d["phase"] = NOMS_PHASES[phase()]; d["ageMin"] = P.ageMin;
  d["faim"] = (int)P.faim; d["joie"] = (int)P.joie; d["energie"] = (int)P.energie;
  d["batterie"] = W.batterie; d["charge"] = W.charge; d["wifi"] = W.wifiOk; d["rssi"] = W.rssi;
  d["temperature"] = W.temperature; d["meteo"] = W.codeMeteo; d["gravite"] = W.gravite;
  d["message"] = W.message; d["lat"] = R.lat; d["lon"] = R.lon; d["luminosite"] = R.luminosite; d["volume"] = R.volume;
  d["ssid"] = R.ssid; d["ssid2"] = R.ssid2; d["reseauActif"] = W.wifiOk ? WiFi.SSID() : String(""); d["direct"] = (direct || modeAcces); d["eco"] = eco;
  d["veille"] = veille; d["branche"] = brancheUsb; d["majDispo"] = versionDispo;
  JsonObject m = d["modules"].to<JsonObject>();
  for (int i = 0; i < M_NB; i++) m[CLES_MODULES[i]] = R.actif[i];
  String s; serializeJson(d, s); return s;
}

void ajouterFiche(JsonDocument& d) {
  d["perso"] = R.perso; d["objets"] = R.objets; d["reveil"] = R.reveil; d["points"] = points;
  JsonArray np = d["persos"].to<JsonArray>(); for (int i = 0; i < NB_PERSOS; i++) np.add(String(FI[i].nom));
  Fiche& f = FA; JsonObject fo = d["fiche"].to<JsonObject>(); char hx[8];
  snprintf(hx, sizeof(hx), "%02x%02x%02x", f.r, f.g, f.b); fo["couleur"] = String(hx); fo["auto"] = f.couleurAuto; fo["humeurcol"] = f.humeurCouleur; fo["nom"] = String(f.nom);
  JsonArray c = fo["carac"].to<JsonArray>(); for (int i = 0; i < 5; i++) c.add(f.carac[i]);
  JsonArray a = fo["aime"].to<JsonArray>(); for (int i = 0; i < EV_NB; i++) a.add(f.aime[i]);
  JsonArray h = fo["hum"].to<JsonArray>();
  for (int i = 0; i < 13; i++) { if (!f.cHum[i][0] && !f.cHum[i][1] && !f.cHum[i][2]) h.add("none"); else { snprintf(hx, sizeof(hx), "%02x%02x%02x", f.cHum[i][0], f.cHum[i][1], f.cHum[i][2]); h.add(String(hx)); } }
  fo["poids"] = f.poids; fo["milieu"] = f.milieu; fo["gravite"] = f.gravite; fo["saut"] = f.saut; fo["rebond"] = f.rebond; fo["mode0g"] = f.mode0g; fo["chapeau"] = f.chapeau;
}
void autoriserCors() { serveur.sendHeader("Access-Control-Allow-Origin", "*"); }

void appliquerReglagesWeb() {
  for (int i = 0; i < M_NB; i++) if (serveur.hasArg(CLES_MODULES[i])) R.actif[i] = serveur.arg(CLES_MODULES[i]) == "1";
  if (serveur.hasArg("nom")) { String n = serveur.arg("nom"); n.trim(); if (n.length() && n.length() < 16) R.nom = n; }
  if (serveur.hasArg("ssid") && serveur.arg("ssid").length()) R.ssid = serveur.arg("ssid");
  if (serveur.hasArg("mdp") && serveur.arg("mdp").length()) R.motDePasse = serveur.arg("mdp");
  if (serveur.hasArg("ssid2") && serveur.arg("ssid2").length()) R.ssid2 = serveur.arg("ssid2");
  if (serveur.hasArg("mdp2") && serveur.arg("mdp2").length()) R.mdp2 = serveur.arg("mdp2");
  if (serveur.hasArg("oublier2")) { oublierConnu(R.ssid2); R.ssid2 = ""; R.mdp2 = ""; }
  if (serveur.hasArg("reveil")) { if (veille) reveillerEcran(); activite(); }
  if (serveur.hasArg("lat")) R.lat = serveur.arg("lat").toFloat();
  if (serveur.hasArg("lon")) R.lon = serveur.arg("lon").toFloat();
  if (serveur.hasArg("lum")) { R.luminosite = constrain(serveur.arg("lum").toInt(), 10, 255); if (!veille) M5.Display.setBrightness(R.luminosite); }
  if (serveur.hasArg("vol")) R.volume = constrain(serveur.arg("vol").toInt(), 0, 255);
  if (serveur.hasArg("gravite")) { W.gravite = serveur.arg("gravite") == "1"; prefs.putBool("grav", W.gravite); }
  if (serveur.hasArg("message")) { W.message = serveur.arg("message"); W.finMessage = serveur.hasArg("duree") ? millis() + serveur.arg("duree").toInt() * 1000UL : 0; }
  if (serveur.hasArg("nouveaucode") && serveur.arg("nouveaucode").length() >= 3) prefs.putString("code", serveur.arg("nouveaucode"));
  if (serveur.hasArg("direct")) { direct = serveur.arg("direct") == "1"; prefs.putBool("direct", direct); changerDirect = true; }
  if (serveur.hasArg("eco")) { eco = serveur.arg("eco") == "1"; prefs.putBool("eco", eco); changerDirect = true; }
  if (serveur.hasArg("perso")) R.perso = constrain(serveur.arg("perso").toInt(), 0, NB_PERSOS - 1);
  { Fiche& f = FA; bool mf = false;
    if (serveur.hasArg("couleur")) { String c = serveur.arg("couleur"); c.replace("#", ""); if (c == "auto") f.couleurAuto = true; else if (c.length() == 6) { long v = strtol(c.c_str(), nullptr, 16); f.r = (v >> 16) & 255; f.g = (v >> 8) & 255; f.b = v & 255; f.couleurAuto = false; } mf = true; }
    if (serveur.hasArg("humeurcol")) { f.humeurCouleur = serveur.arg("humeurcol") == "1"; mf = true; }
    for (int i = 0; i < 5; i++) { String k = "carac" + String(i); if (serveur.hasArg(k)) { f.carac[i] = constrain(serveur.arg(k).toInt(), 0, 100); mf = true; } }
    for (int i = 0; i < EV_NB; i++) { String k = "aime" + String(i); if (serveur.hasArg(k)) { f.aime[i] = constrain(serveur.arg(k).toInt(), -1, 1); mf = true; } }
    for (int i = 0; i < 13; i++) { String k = "hum" + String(i); if (serveur.hasArg(k)) { String c = serveur.arg(k); c.replace("#", ""); if (c.length() != 6) { f.cHum[i][0] = f.cHum[i][1] = f.cHum[i][2] = 0; } else { long v = strtol(c.c_str(), nullptr, 16); f.cHum[i][0] = (v >> 16) & 255; f.cHum[i][1] = (v >> 8) & 255; f.cHum[i][2] = v & 255; } mf = true; } }
    auto num = [&](const char* k, uint8_t& d, int lo, int hi) { if (serveur.hasArg(k)) { d = constrain(serveur.arg(k).toInt(), lo, hi); mf = true; } };
    num("poids", f.poids, 0, 100); num("milieu", f.milieu, 0, 100); num("gravite", f.gravite, 0, 200); num("saut", f.saut, 0, 200); num("rebond", f.rebond, 0, 100); num("mode0g", f.mode0g, 0, 2);
    if (serveur.hasArg("chapeau")) { int h = constrain(serveur.arg("chapeau").toInt(), 0, 3); if (points >= SEUIL_CHAPEAU[h]) { f.chapeau = h; mf = true; } }
    if (serveur.hasArg("pnom")) { String n = serveur.arg("pnom"); n.trim(); if (n.length() && n.length() < 15) { strncpy(f.nom, n.c_str(), 15); f.nom[15] = 0; mf = true; } }
    if (mf) sauverFiche(R.perso);
  }
  if (serveur.hasArg("reveilheure")) { String r = serveur.arg("reveilheure"); R.reveil = (r.length() == 5) ? r : String(""); }
  if (serveur.hasArg("objets")) R.objets = serveur.arg("objets") == "1";
  if (serveur.hasArg("nourrir")) nourrir();
  if (serveur.hasArg("caresser")) caresser();
  sauverReglages();
}

static bool majOk = true, codeOk = false;
void demarrerServeur() {
  serveur.on("/", HTTP_GET, []() { serveur.send_P(200, "text/html; charset=utf-8", PAGE_WEB); });
  serveur.on("/api/etat", HTTP_GET, []() { autoriserCors(); serveur.send(200, "application/json; charset=utf-8", jsonEtat()); });
  serveur.on("/api/reglages", HTTP_POST, []() {
    autoriserCors(); appliquerReglagesWeb();
    bool reseau = (serveur.hasArg("ssid") && serveur.arg("ssid").length()) || (serveur.hasArg("ssid2") && serveur.arg("ssid2").length());
    serveur.send(200, "application/json; charset=utf-8", jsonEtat());
    if (reseau) { delay(300); sauverPerso(); esp_restart(); }
  });
  serveur.on("/api/reglages", HTTP_OPTIONS, []() { autoriserCors(); serveur.sendHeader("Access-Control-Allow-Headers", "*"); serveur.send(204); });
  serveur.on("/api/maj", HTTP_POST, []() {
    autoriserCors();
    if (!codeOk) { serveur.send(403, "text/plain; charset=utf-8", "Code de mise à jour incorrect : rien n'a changé."); return; }
    serveur.send(majOk ? 200 : 500, "text/plain; charset=utf-8", majOk ? "Mise à jour reçue. Le Core2 redémarre. S'il ne démarre pas bien, il reprend tout seul l'ancienne version." : "La mise à jour a échoué : rien n'a changé.");
    if (majOk) { delay(400); sauverPerso(); esp_restart(); }
  }, []() {
    HTTPUpload& u = serveur.upload();
    if (u.status == UPLOAD_FILE_START) {
      codeOk = serveur.arg("code") == prefs.getString("code", "666666");
      majOk = codeOk; if (!codeOk) return;
      M5.Display.fillScreen(TFT_BLACK); M5.Display.setFont(&fonts::lgfxJapanGothic_16); M5.Display.setTextColor(TFT_WHITE); M5.Display.drawString("Mise à jour en cours…", 20, 100);
      esp_task_wdt_reset();
      WiFi.setAutoReconnect(false); if (WiFi.status() != WL_CONNECTED) WiFi.disconnect(false, false); WiFi.setSleep(false); Update.abort();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) majOk = false;
    }
    else if (u.status == UPLOAD_FILE_WRITE) { esp_task_wdt_reset(); if (majOk && Update.write(u.buf, u.currentSize) != u.currentSize) majOk = false; }
    else if (u.status == UPLOAD_FILE_END) { if (!(majOk && Update.end(true))) majOk = false; else prefs.putInt("essais", 0); }
    else if (u.status == UPLOAD_FILE_ABORTED) { Update.abort(); majOk = false; WiFi.setAutoReconnect(true); }
  });
  serveur.on("/api/retour", HTTP_POST, []() {      // retour arrière demandé à la main
    autoriserCors();
    if (serveur.arg("code") != prefs.getString("code", "666666")) { serveur.send(403, "text/plain; charset=utf-8", "Code incorrect."); return; }
    const esp_partition_t* autre = esp_ota_get_next_update_partition(esp_ota_get_running_partition());
    uint8_t mg = 0;
    if (autre && esp_partition_read(autre, 0, &mg, 1) == ESP_OK && mg == 0xE9) { serveur.send(200, "text/plain; charset=utf-8", "Retour à l'ancienne version…"); delay(300); esp_ota_set_boot_partition(autre); esp_restart(); }
    else serveur.send(409, "text/plain; charset=utf-8", "Il n'y a pas d'ancienne version.");
  });
  serveur.on("/api/majinternet", HTTP_POST, []() {      // le Core2 va chercher lui-même la nouvelle version sur Internet
    autoriserCors();
    if (serveur.arg("code") != prefs.getString("code", "666666")) { serveur.send(403, "text/plain; charset=utf-8", "Code incorrect."); return; }
    if (versionDispo.length()) { demandeMaj = true; serveur.send(200, "text/plain; charset=utf-8", "Mise à jour " + versionDispo + " lancée : regarde l'écran du Core2."); }
    else { prochaineVerif = 0; serveur.send(200, "text/plain; charset=utf-8", "Pas de version plus récente connue. Je revérifie sur Internet : réessaie dans 20 secondes."); }
  });
  serveur.on("/api/capteurs", HTTP_GET, []() {          // pour se servir du Core2 comme capteur ou manette depuis le téléphone
    autoriserCors(); JsonDocument d; float gx = 0, gy = 0, gz = 0; if (M5.Imu.isEnabled()) M5.Imu.getGyro(&gx, &gy, &gz);
    auto t = M5.Touch.getDetail();
    d["ax"] = ax; d["ay"] = ay; d["az"] = az; d["gx"] = gx; d["gy"] = gy; d["gz"] = gz; d["son"] = (int)W.niveauSon;
    d["boutonGauche"] = M5.BtnA.isPressed(); d["boutonMilieu"] = M5.BtnB.isPressed(); d["boutonDroit"] = M5.BtnC.isPressed();
    d["appui"] = t.isPressed(); d["x"] = t.x; d["y"] = t.y; d["pile"] = W.batterie; d["branche"] = brancheUsb;
    String r; serializeJson(d, r); serveur.send(200, "application/json; charset=utf-8", r);
  });
  serveur.on("/api/ondes", HTTP_GET, []() {          // mesures des ondes Wi-Fi, pour une autre application
    autoriserCors(); JsonDocument d;
    d["detection"] = detection; d["mouvement"] = mouvement; d["agitation"] = mvtNiveau; d["repos"] = mvtBase; d["canal"] = canalDet; d["rssiMaison"] = W.rssi;
    JsonArray h = d["historique"].to<JsonArray>(); for (int i = 0; i < (int)sizeof(histMvt); i++) h.add(histMvt[(histMvtPos + i) % (int)sizeof(histMvt)]);
    JsonArray ca = d["canaux"].to<JsonArray>(); for (int c = 1; c <= 13; c++) ca.add(sniffCanal[c]);
    String r; serializeJson(d, r); serveur.send(200, "application/json; charset=utf-8", r);
  });
  serveur.on("/api/reseaux", HTTP_GET, []() {
    autoriserCors(); arreterReconnexion(); int n = WiFi.scanNetworks(false, true); if (n <= 0) { delay(300); n = WiFi.scanNetworks(false, true); } WiFi.setAutoReconnect(true); JsonDocument d; JsonArray a = d.to<JsonArray>();
    for (int i = 0; i < n && i < 30; i++) {
      String nom = WiFi.SSID(i); if (!nom.length()) continue;
      bool deja = false; for (JsonObject o : a) if (nom == o["nom"].as<String>()) deja = true;
      if (deja) continue;
      JsonObject o = a.add<JsonObject>(); o["nom"] = nom; o["dbm"] = WiFi.RSSI(i); o["ferme"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN); o["canal"] = WiFi.channel(i);
    }
    WiFi.scanDelete(); String r; serializeJson(d, r); serveur.send(200, "application/json; charset=utf-8", r);
  });
  serveur.on("/api/test", HTTP_GET, []() {
    autoriserCors(); JsonDocument d;
    d["version"] = VERSION_PROGRAMME; d["wifiMaison"] = (WiFi.status() == WL_CONNECTED); d["reseau"] = WiFi.SSID(); d["reseau1"] = R.ssid; d["reseau2"] = R.ssid2;
    d["pourcent"] = W.batterie; d["parTension"] = pourcentTension(pileLissee); d["veille"] = veille; d["branche"] = brancheUsb; d["majDispo"] = versionDispo; d["infoMaj"] = infoMaj; d["ip"] = WiFi.localIP().toString();
    d["signal"] = WiFi.RSSI(); d["telephonesSurWifiDirect"] = WiFi.softAPgetStationNum(); d["memoireLibre"] = ESP.getFreeHeap();
    d["secondesDepuisDemarrage"] = millis() / 1000;
    d["uptimeMs"] = millis(); d["derniereAct"] = derniereActivite; d["dbgM"] = dbgM; d["dbgAct"] = dbgAct; d["touchers"] = nbTouch; d["clicsPwr"] = nbClicPwr; d["sommeilClic"] = nbSommeilClic; d["sommeilTimeout"] = nbSommeilTimeout; d["veille"] = veille; d["page"] = page;
    d["pileMv"] = M5.Power.getBatteryVoltage(); d["usbMv"] = M5.Power.getVBUSVoltage(); d["courantMa"] = M5.Power.getBatteryCurrent(); d["niveau"] = M5.Power.getBatteryLevel(); d["etatCharge"] = (int)M5.Power.isCharging(); d["puce"] = (int)M5.Power.getType();
    { JsonObject a = d["axp"].to<JsonObject>(); static const uint8_t regs[] = {0x00, 0x01, 0x12, 0x18, 0x22, 0x27, 0x30, 0x61, 0x62, 0x63, 0x64, 0x69};
      for (uint8_t r : regs) { char k[8]; snprintf(k, sizeof(k), "r%02X", r); a[k] = (int)M5.In_I2C.readRegister8(0x34, r, 400000); } }
    const esp_partition_t* autre = esp_ota_get_next_update_partition(esp_ota_get_running_partition()); uint8_t mg = 0;
    d["ancienneVersionDisponible"] = (autre && esp_partition_read(autre, 0, &mg, 1) == ESP_OK && mg == 0xE9);
    String r; serializeJson(d, r); serveur.send(200, "application/json; charset=utf-8", r);
  });
  serveur.begin();
  MDNS.addService("http", "tcp", 80);
}

void demarrerReseau() {
  direct = prefs.getBool("direct", false); eco = prefs.getBool("eco", false);
  WiFi.setHostname("core2-francais");
  WiFi.setAutoReconnect(true); WiFi.persistent(false);
  if (!nbConnus()) modeAcces = true;
  appliquerDirect();
  configTzTime("CET-1CEST,M3.5.0,M3.10.0/3", "pool.ntp.org", "time.google.com");
}

// ---------- Programme principal ----------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  // courant de charge : la puce arrivait avec 0 mA, la pile ne se chargeait donc pas
  M5.In_I2C.writeRegister8(0x34, 0x61, 4, 400000);      // pré-charge : 4 x 25 = 100 mA
  M5.In_I2C.writeRegister8(0x34, 0x62, 8, 400000);      // charge normale : 200 mA
  // (le bouton marche/arrêt garde le comportement standard de la bibliothèque : ne pas toucher à ses registres)
  debutDemarrage = millis();
  brancheUsb = M5.Power.getVBUSVoltage() > 4000;
  M5.Display.setBrightness(160);
  chargerReglages(); chargerFiches(); chargerPerso(); chargerCerveau();
  W.gravite = prefs.getBool("grav", true);
  M5.Display.setBrightness(R.luminosite);
  gererDemarrageSecurise();
  { esp_task_wdt_config_t cw = {}; cw.timeout_ms = 15000; cw.idle_core_mask = 0; cw.trigger_panic = true; esp_task_wdt_reconfigure(&cw); esp_task_wdt_add(NULL); }   // chien de garde : si la boucle reste bloquée 15 s, redémarrage
  cv.setColorDepth(16); cv.setPsram(true);
  if (!cv.createSprite(320, 240)) { cv.setColorDepth(8); cv.createSprite(320, 240); }
  M5.Mic.begin();
  demarrerReseau(); demarrerServeur();
  xTaskCreatePinnedToCore(tacheMeteo, "meteo", 8192, NULL, 1, NULL, 0);
}

void loop() {
  M5.update();
  esp_task_wdt_reset();
  static uint32_t tLent = 0, tSon = 0, tDessin = 0, tPhysique = millis();
  uint32_t m = millis();
  if (finVibre && m > finVibre) { finVibre = 0; M5.Power.setVibration(0); }
  if (m - tLent > 2000) {
    tLent = m;
    bool avantUsb = brancheUsb; brancheUsb = M5.Power.getVBUSVoltage() > 4000;
    Serial.printf("[energie] pourcent=%d pile=%dmV usb=%dmV courant=%dmA charge=%d veille=%d\n", W.batterie, (int)M5.Power.getBatteryVoltage(), (int)M5.Power.getVBUSVoltage(), (int)M5.Power.getBatteryCurrent(), (int)W.charge, (int)veille);
    moduleBatterie();
    if (M5.In_I2C.readRegister8(0x34, 0x62, 400000) != 8) { M5.In_I2C.writeRegister8(0x34, 0x61, 4, 400000); M5.In_I2C.writeRegister8(0x34, 0x62, 8, 400000); }
    if (brancheUsb != avantUsb) appliquerEconomie();
    if (brancheUsb && !avantUsb) { if (veille) reveillerEcran(); activite(); if (FA.aime[EV_COURANT] < 0) reagir(EV_COURANT); else dire(phrase(6), 3500); }       // on le branche : l'écran se rallume
    if (!veille && brancheUsb && M5.Display.getBrightness() < R.luminosite) M5.Display.setBrightness(R.luminosite);
    { static bool alerte = false;
      if (!brancheUsb && W.batterie < 15) { if (!alerte) { alerte = true; if (!veille) jouer(S_PILE); } } else if (brancheUsb || W.batterie > 20) alerte = false; }
    { static int vide = 0;        // pile vide depuis 10 secondes : arrêt propre
      if (!brancheUsb && pileLissee > 2500 && pileLissee < 3250) vide++; else vide = 0;
      if (vide >= 5) arretPileVide(); }
    moduleWifi();
    if (R.actif[M_HEURE]) moduleHeure();
  }
  bool clicMarche = M5.BtnPWR.wasClicked();
  if (clicMarche) { nbClicPwr++; activite(); }
  gererBoutons();
  if (veille) {
    // écran éteint : deux appuis rapprochés sur l'écran, ou un appui court sur le bouton marche/arrêt, le rallument
    auto t = M5.Touch.getDetail();
    if (t.wasPressed() && t.y < 240) reveillerEcran();   // un simple toucher reveille l ecran
    if (clicMarche && veille) reveillerEcran();
    tPhysique = m;
  } else {
    if (clicMarche && (int32_t)(millis() - derniereActivite) > 1500) { nbSommeilClic++; endormirEcran(); }   // le toucher n'eteint jamais l'ecran : seul le bouton power, et seulement sans toucher en cours
    if (R.actif[M_SON] && m - tSon > 250) { tSon = m; moduleSon(); }
    debutModule(M_GRAVITE);
    float dt = (m - tPhysique) / 1000.0f; tPhysique = m; if (dt > 0.1f) dt = 0.1f;
    physique(dt); objetsPhysique(dt);
    finModule();
    gererTactile();
    if (page != pageAvant) { if (pageAvant == 8 && sniffActif) arreterSniff(); if (pageAvant == 9 && detection) arreterDetection(); pageAvant = page; }
    if (sniffActif) { if (m - dernierHop > 300) { dernierHop = m; canalSniff = canalSniff % 13 + 1; esp_wifi_set_channel(canalSniff, WIFI_SECOND_CHAN_NONE); } if (m - sniffDerniereSec > 1000) { sniffDerniereSec = m; sniffParSeconde = sniffTotal - sniffDernierTotal; sniffDernierTotal = sniffTotal; } }
    if (detection && m - detDerniere > 500) {
      detDerniere = m;
      long sm = sRssiSum, sq = sRssiSq; int n = sRssiN; sRssiSum = 0; sRssiSq = 0; sRssiN = 0;
      float ec = 0;
      if (n > 4) { double moy = (double)sm / n; double var = (double)sq / n - moy * moy; ec = var > 0 ? (float)sqrt(var) : 0; }
      mvtNiveau = mvtNiveau * 0.5f + ec * 0.5f;
      if (mvtNiveau < mvtBase) mvtBase = mvtBase * 0.95f + mvtNiveau * 0.05f;
      else mvtBase = mvtBase * 0.997f + mvtNiveau * 0.003f;
      float dev = mvtNiveau - mvtBase;
      mouvement = (dev > 2.5f && n > 8);
      int h = (int)constrain(dev * 14.0f, 0.0f, 86.0f);
      histMvt[histMvtPos] = (uint8_t)h; histMvtPos = (histMvtPos + 1) % (int)sizeof(histMvt);
    }
    if (!veille && m - tDessin > (brancheUsb ? 33u : 50u)) { tDessin = m; dessinerPage(); }
    if (sniffActif || detection || page == 11 || page == 13 || (page >= 21 && page <= 30)) activite();
    if (!veille && !brancheUsb && (int32_t)(millis() - derniereActivite) > 300000) { nbSommeilTimeout++; endormirEcran(); }   // 5 min sans aucun toucher : veille (calcul signe pour ne jamais confondre un toucher avec un depassement)
  }
  vieDuPerso();
  {   // reconnexion automatique : toutes les 12 s sans Wi-Fi, on cherche les réseaux et on prend le réseau connu qui capte le mieux
    static uint32_t tWifi = 0; static bool rechFond = false; static uint32_t debFond = 0;
    bool libre = !(page >= 2 && page <= 4) && page != 7 && page != 8 && page != 9 && !sniffActif && !detection && !scanEnCours && !demandeMaj && nbConnus() > 0;
    if (W.wifiOk || WiFi.status() == WL_CONNECTED) { tWifi = m; if (rechFond) { WiFi.scanDelete(); rechFond = false; } }
    else if (libre && !rechFond && m - tWifi > 12000) {
      tWifi = m; WiFi.setAutoReconnect(false); WiFi.disconnect(false, false); delay(50);
      if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) { WiFi.scanDelete(); essaiReseau++; int k = essaiReseau % max(1, nbConnus()); WiFi.setAutoReconnect(true); WiFi.begin(connuN[k].c_str(), connuM[k].c_str()); }
      else { rechFond = true; debFond = m; }
    }
    if (rechFond) {
      int n = WiFi.scanComplete();
      if (n >= 0 || m - debFond > 12000) {
        int best = -1, bestDbm = -200;
        for (int i = 0; i < n; i++) { int j = indexConnu(WiFi.SSID(i)); if (j >= 0 && WiFi.RSSI(i) > bestDbm) { best = j; bestDbm = WiFi.RSSI(i); } }
        WiFi.scanDelete(); rechFond = false; tWifi = m; WiFi.setAutoReconnect(true);
        if (best >= 0) WiFi.begin(connuN[best].c_str(), connuM[best].c_str());
      }
    }
  }
  { static bool mdns = false; if (!mdns && WiFi.status() == WL_CONNECTED) { mdns = MDNS.begin("core2-francais"); if (mdns) MDNS.addService("http", "tcp", 80); } }
  if (changerDirect) { changerDirect = false; appliquerDirect(); }
  if (demandeMaj) majDepuisInternet();
  { static uint32_t tAutoMaj = 0; static uint8_t essaisAutoMaj = 0;   // mise a jour automatique : des qu'il a Internet et qu'une version plus recente existe
    if (!demandeMaj && versionDispo.length() && W.wifiOk && essaisAutoMaj < 2 && m > 60000 && m - tAutoMaj > 90000 && (brancheUsb || W.batterie >= 40)) { tAutoMaj = m; essaisAutoMaj++; infoMaj = "Mise a jour automatique..."; demandeMaj = true; } }
  serveur.handleClient();
  validerSiStable();
  delay(veille ? 20 : (brancheUsb ? 1 : 4));
}
