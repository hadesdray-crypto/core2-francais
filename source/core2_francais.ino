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

#define VERSION_PROGRAMME "1.9.6"
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
} R;
Preferences prefs;

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
}
void sauverReglages() {
  for (int i = 0; i < M_NB; i++) prefs.putBool(CLES_MODULES[i], R.actif[i]);
  prefs.putString("ssid", R.ssid); prefs.putString("mdp", R.motDePasse); prefs.putString("nom", R.nom);
  prefs.putString("ssid2", R.ssid2); prefs.putString("mdp2", R.mdp2);
  prefs.putFloat("lat", R.lat); prefs.putFloat("lon", R.lon);
  prefs.putInt("lum", R.luminosite); prefs.putInt("vol", R.volume);
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
String versionDispo, urlBinDispo, infoMaj; bool demandeMaj = false; volatile uint32_t prochaineVerif = 30000;
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
  P.ageMin = prefs.getUInt("age", 0); P.faim = prefs.getFloat("faim", 20); P.joie = prefs.getFloat("joie", 70); P.energie = prefs.getFloat("ener", 80);
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
  WiFi.begin(R.ssid.length() ? R.ssid.c_str() : R.ssid2.c_str(), R.ssid.length() ? R.motDePasse.c_str() : R.mdp2.c_str());
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
  WiFi.begin(R.ssid.length() ? R.ssid.c_str() : R.ssid2.c_str(), R.ssid.length() ? R.motDePasse.c_str() : R.mdp2.c_str());
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
void moduleBatterie() {
  debutModule(M_BATTERIE);
  static uint32_t tPrec = 0; static int dernierSauve = -100;
  uint32_t m = millis();
  float mv = M5.Power.getBatteryVoltage(), ma = M5.Power.getBatteryCurrent();
  bool avant = W.charge;
  W.charge = (M5.Power.isCharging() == M5.Power.is_charging);
  bool finie = brancheUsb && ((M5.In_I2C.readRegister8(0x34, 0x01, 400000) & 0x07) == 4);
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
    if (niveau > 6000 && millis() - W.picSon > 2000) { W.picSon = millis(); attitude(PEUR, 2500); P.joie = max(0.0f, P.joie - 3); }
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
    P.faim = min(100.0f, P.faim + 0.6f);
    if (P.faim > 80) P.joie = max(0.0f, P.joie - 1);
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
  cible = 0;
  if (phase() != OEUF && W.charge) cible = 3;
  else if (phase() != OEUF) { if (P.attitude == SOMMEIL) cible = 1; else if (millis() < finRepas || P.attitude == AFFAME) cible = 2; }
}

// ---------- Mouvement dans la boîte ----------
float ax = 0, ay = 0, az = 1, axPrec = 0, ayPrec = 0, azPrec = 1;
uint32_t dernierSecouement = 0;

// Un choc contre une paroi : il s'écrase un instant, ça fait « boum » et le boîtier vibre.
void choc(float v) {
  if (v < 0.9f) return;
  finEcrase = millis() + 180;
  if (!veille) { jouer(S_CHOC); vibrer(60); }
}
void physique(float dt) {
  uint32_t m = millis();
  bool pesanteur = R.actif[M_GRAVITE] && W.gravite;
  if (M5.Imu.isEnabled()) {
    axPrec = ax; ayPrec = ay; azPrec = az;
    M5.Imu.getAccel(&ax, &ay, &az);
    float sec = fabs(ax - axPrec) + fabs(ay - ayPrec) + fabs(az - azPrec);
    if (sec > 2.2f && m - dernierSecouement > 1200) {
      dernierSecouement = m; activite();
      attitude(ETOURDI, 3500); P.joie = max(0.0f, P.joie - 4);
      float force = pesanteur ? 25.0f : 60.0f;     // avec la gravité il est projeté bien plus fort
      P.vx += (random(-100, 100) / force); P.vy += (random(-100, 100) / force);
      finVol = 0; jouer(S_SECOUE);
    }
  }
  if (P.porte) { P.vx = P.vy = 0; return; }
  if (cible && !(pesanteur && (fabs(ax) > 0.45f || fabs(ay) > 0.45f))) {   // il va au lit ou à la gamelle
    P.z = 0.5f;
    P.vx += (cibleX[cible] - P.x) * 3.0f * dt; P.vy += (cibleY[cible] - P.y) * 3.0f * dt;
    P.vx *= 0.9f; P.vy *= 0.9f; P.x += P.vx * dt * 10.0f; P.y += P.vy * dt * 10.0f;
    return;
  }
  if (pesanteur) {
    // Gravité réelle : il tombe vers le bas du boîtier, quelle que soit la façon dont on le tient.
    float gx = -ax * 7.0f, gy = ay * 7.0f;
    bool pente = (fabs(ax) + fabs(ay)) > 0.25f;
    // C'est un être qui vole : de temps en temps il décolle, plane un peu, puis retombe.
    if (pente && phase() != OEUF && m > prochainVol && (P.attitude == NORMALE || P.attitude == CONTENT) && P.energie > 15) {
      finVol = m + 1300 + random(1100); prochainVol = finVol + 9000 + random(14000);
      if (!veille) jouer(S_VOL);
    }
    if (m < finVol) { P.vx += -gx * 1.5f * dt + sinf(m / 300.0f) * 0.8f * dt; P.vy += -gy * 1.5f * dt; P.vx *= 0.96f; P.vy *= 0.96f; }
    else { P.vx += gx * dt; P.vy += gy * dt; }
    P.vx *= 0.995f; P.vy *= 0.995f;
  } else {
    // Sans gravité : il est en lévitation, il flotte doucement et se balade.
    float tx = 0.45f * sinf(m / 5300.0f), ty = 0.22f * sinf(m / 3700.0f) + 0.06f * sinf(m / 700.0f);
    P.vx += (tx - P.x) * 1.4f * dt; P.vy += (ty - P.y) * 1.4f * dt;
    P.vx *= 0.97f; P.vy *= 0.97f; finVol = 0;
  }
  P.x += P.vx * dt; P.y += P.vy * dt;
  const float lim = 0.78f;
  bool contact = false;
  if (P.x > lim)  { P.x = lim;  choc(fabs(P.vx)); P.vx = -P.vx * 0.35f; if (fabs(P.vx) < 0.15f) P.vx = 0; contact = true; P.vy *= 0.9f; }
  if (P.x < -lim) { P.x = -lim; choc(fabs(P.vx)); P.vx = -P.vx * 0.35f; if (fabs(P.vx) < 0.15f) P.vx = 0; contact = true; P.vy *= 0.9f; }
  if (P.y > lim)  { P.y = lim;  choc(fabs(P.vy)); P.vy = -P.vy * 0.35f; if (fabs(P.vy) < 0.15f) P.vy = 0; contact = true; P.vx *= 0.9f; }
  if (P.y < -lim) { P.y = -lim; choc(fabs(P.vy)); P.vy = -P.vy * 0.35f; if (fabs(P.vy) < 0.15f) P.vy = 0; contact = true; P.vx *= 0.9f; }
  (void)contact;
  P.z = pesanteur ? 0.35f : 0.35f + 0.25f * sinf(m / 4000.0f);
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
  cv.fillRect(ix, iy, L - 2 * ix, H - 2 * iy, C(10, 8, 28));
  static const uint8_t haut[9] = {50, 78, 40, 92, 62, 84, 46, 70, 56};
  for (int i = 0; i < 9; i++) {
    int bx = ix + 4 + i * 23, bh = haut[i], by = H - iy - bh;
    cv.fillRect(bx, by, 20, bh, C(16, 14, 40)); cv.drawRect(bx, by, 20, bh, C(44, 32, 96));
    for (int f = 0; f < 6; f++) {
      int wx = bx + 3 + (f % 3) * 6, wy = by + 5 + (f / 3) * 10 + ((i * 7 + f * 3) % 4) * 6;
      if (wy < H - iy - 6 && ((i * 5 + f * 3 + (int)(millis() / 3000)) % 3) != 0) cv.fillRect(wx, wy, 3, 4, ((i + f) % 2) ? C(255, 220, 90) : cyan);
    }
  }
  cv.drawRect(ix + 150, iy + 8, 40, 12, rose); cv.drawFastHLine(ix + 154, iy + 14, 32, rose);     // enseigne néon
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

void positionEcran(int& px, int& py, float& echelle) {
  const int L = 320, H = 200;
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
    float ph = fmodf(t / 900.0f + i / 10.0f, 1.0f), a = i * 0.63f + t / 1500.0f, d = (1.0f - ph) * 78.0f;
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

void dessinerBulle(int px, int py, float e) {
  Phase ph = phase();
  float taille = (ph == BEBE) ? 0.6f : (ph == ENFANT) ? 0.8f : (ph == ADO) ? 0.95f : (ph == ADULTE) ? 1.1f : 1.0f;
  int r = (int)(34 * e * taille);
  uint16_t neon = (ph == BEBE) ? C(255, 80, 200) : (ph == ENFANT) ? C(255, 170, 40) : (ph == ADO) ? C(0, 220, 255) : (ph == ADULTE) ? C(80, 255, 140) : C(190, 120, 255);
  if (P.attitude == FROID) neon = C(150, 200, 255);
  if (P.attitude == CHAUD) neon = C(255, 90, 60);
  if (P.attitude == TRISTE) neon = C(110, 120, 170);
  uint32_t t = millis();
  float respire = 1.0f + 0.04f * sinf(t / 600.0f);
  float ecrase = (P.attitude == ETOURDI) ? 0.85f + 0.15f * sinf(t / 90.0f) : 1.0f;
  if (t < finEcrase) ecrase *= 0.7f;      // il s'écrase un instant au moment du choc
  int rx = (int)(r * respire), ry = (int)(r * 0.92f * ecrase);
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
  int yeuY = py - ry / 6, ecart = rx / 2;
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
    case ETOURDI:
      cv.drawLine(px - ecart - 5, yeuY - 5, px - ecart + 5, yeuY + 5, neon); cv.drawLine(px - ecart - 5, yeuY + 5, px - ecart + 5, yeuY - 5, neon);
      cv.drawLine(px + ecart - 5, yeuY - 5, px + ecart + 5, yeuY + 5, neon); cv.drawLine(px + ecart - 5, yeuY + 5, px + ecart + 5, yeuY - 5, neon);
      break;
    default:
      if (W.charge && P.attitude != SURPRIS) {   // yeux heureux en ^ ^
        cv.drawLine(px - ecart - ouvert, yeuY + 3, px - ecart, yeuY - 3, neon); cv.drawLine(px - ecart, yeuY - 3, px - ecart + ouvert, yeuY + 3, neon);
        cv.drawLine(px + ecart - ouvert, yeuY + 3, px + ecart, yeuY - 3, neon); cv.drawLine(px + ecart, yeuY - 3, px + ecart + ouvert, yeuY + 3, neon);
      } else if (clignote) { cv.drawFastHLine(px - ecart - ouvert, yeuY, ouvert * 2, neon); cv.drawFastHLine(px + ecart - ouvert, yeuY, ouvert * 2, neon); }
      else {
        int big = (P.attitude == ECOUTE) ? ouvert + 2 : ouvert;
        cv.fillRoundRect(px - ecart - big, yeuY - big, big * 2, big * 2 + 3, 2, neon); cv.fillRoundRect(px + ecart - big, yeuY - big, big * 2, big * 2 + 3, 2, neon);
        cv.fillRect(px - ecart - 1, yeuY - 2, 3, 3, C(255, 255, 255)); cv.fillRect(px + ecart - 1, yeuY - 2, 3, 3, C(255, 255, 255));
      }
  }
  int boucheY = py + ry / 3;
  if (millis() < finRepas) {   // il mange
    cv.fillEllipse(px, boucheY + 2, 7, 3 + ((t / 150) % 2) * 4, neon);
  } else switch (P.attitude) {
    case CONTENT: cv.fillArc(px, boucheY - 4, 6, 12, 20, 160, neon); break;
    case TRISTE: cv.fillArc(px, boucheY + 8, 6, 11, 200, 340, neon); break;
    case AFFAME: cv.fillEllipse(px, boucheY + 2, 7, 6, neon); cv.fillEllipse(px, boucheY + 4, 3, 2, C(255, 40, 170)); break;
    case SURPRIS: case PEUR: cv.fillEllipse(px, boucheY + 2, 5, 7, neon); break;
    case SOMMEIL: cv.drawFastHLine(px - 5, boucheY, 10, pale); break;
    default: if (W.charge) cv.fillArc(px, boucheY - 4, 6, 12, 20, 160, neon); else cv.fillArc(px, boucheY - 2, 5, 9, 40, 140, neon);
  }
  if (P.attitude == FROID) { cv.fillRect(px - rx, py + ry / 2, rx * 2, 7, C(230, 70, 70)); cv.fillRect(px + rx / 2, py + ry / 2, 7, 18, C(230, 70, 70)); }
  if (P.attitude == CHAUD) { cv.fillRect(px - ecart - ouvert - 3, yeuY - 4, ecart * 2 + ouvert * 2 + 6, 9, C(255, 40, 170)); cv.drawFastHLine(px - ecart - ouvert - 3, yeuY - 3, ecart * 2 + ouvert * 2 + 6, C(255, 255, 255)); }
  if (P.attitude == PLUIE) { cv.fillArc(px, py - ry - 18, 0, 36, 180, 360, C(230, 190, 60)); cv.drawLine(px, py - ry - 18, px, py - ry + 4, C(120, 120, 120));
    for (int i = 0; i < 8; i++) cv.drawLine(20 + (i * 41 + millis() / 8) % 280, 40 + (i * 29 + millis() / 5) % 120, 18 + (i * 41 + millis() / 8) % 280, 48 + (i * 29 + millis() / 5) % 120, C(130, 170, 255));
    if (W.codeMeteo >= 95 && (millis() % 5000) < 160) { cv.drawLine(250, 20, 238, 60, C(255, 255, 200)); cv.drawLine(238, 60, 252, 62, C(255, 255, 200)); cv.drawLine(252, 62, 240, 104, C(255, 255, 200)); } }   // orage : un éclair de temps en temps
  if (P.attitude == FROID && W.codeMeteo >= 71 && W.codeMeteo <= 86) { for (int i = 0; i < 10; i++) cv.fillCircle(20 + (i * 37 + (int)(8 * sinf(millis() / 700.0f + i))) % 280, 30 + (i * 23 + millis() / 30) % 140, 2, C(240, 245, 255)); }   // neige
  if (P.attitude == ETOURDI) { for (int i = 0; i < 3; i++) { float a = millis() / 150.0f + i * 2.1f; cv.fillCircle(px + (int)(cosf(a) * 22), py - ry - 10 + (int)(sinf(a) * 5), 3, C(255, 230, 90)); } }
}

void dessinerBarre() {
  // boutons du bas : Nourrir | Gravité | Réglages
  cv.fillRect(0, 200, 320, 40, C(8, 8, 20)); cv.drawFastHLine(0, 200, 320, C(255, 40, 170));
  cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center);
  const char* lib[3] = {"Nourrir", "Gravité", "Menu"};
  for (int i = 0; i < 3; i++) {
    int bx = 4 + i * 106;
    cv.fillRoundRect(bx, 204, 102, 32, 8, C(18, 20, 44)); cv.drawRoundRect(bx, 204, 102, 32, 8, C(0, 230, 255));
    cv.setTextColor(C(240, 240, 255)); cv.drawString(lib[i], bx + (i == 1 ? 44 : 51), 220);
    if (i == 1) cv.fillCircle(bx + 88, 220, 5, W.gravite ? C(80, 255, 140) : C(255, 70, 90));
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
  if (phase() != OEUF) { jauge("Faim", P.faim, C(240, 140, 80)); jauge("Joie", P.joie, C(250, 220, 90)); }
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
  }
  cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(top_center);
  if (W.batterie < 15 && !brancheUsb) { cv.setTextColor(C(255, 110, 110)); cv.drawString("Pile faible : branche-moi", 160, 3); }
  else if (versionDispo.length()) { cv.setTextColor(C(120, 255, 150)); cv.drawString("Mise à jour " + versionDispo + " : Réglages > Matériel", 160, 3); }
  cv.setTextDatum(top_left);
  if (infoDemarrage.length() && millis() - debutDemarrage < 12000) {
    cv.fillRoundRect(10, 92, 300, 38, 8, C(120, 40, 40)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextColor(C(255, 255, 255)); cv.setTextDatum(middle_center);
    cv.drawString(infoDemarrage.substring(0, 48), 160, 111); cv.setTextDatum(top_left);
  }
}

void dessinerAccueil() {
  dessinerBoite();
  int px, py; float e; positionEcran(px, py, e);
  if (W.charge) { py -= (int)(3 * fabsf(sinf(millis() / 250.0f))); effetChargeAvant(px, py, e); }
  bool pesanteur = R.actif[M_GRAVITE] && W.gravite;
  if (!pesanteur && !W.charge) { int rr = (int)(30 * e) + (int)(3 * sinf(millis() / 400.0f)); cv.drawEllipse(px, py + (int)(46 * e), rr, 5, C(0, 230, 255)); cv.drawEllipse(px, py + (int)(46 * e), rr - 8, 3, C(255, 40, 170)); }   // anneau de lévitation
  if (millis() < finVol) { int fy = py + (int)(38 * e), l = 8 + (int)((millis() / 40) % 8); cv.fillTriangle(px - 14, fy, px - 6, fy, px - 10, fy + l, C(255, 200, 60)); cv.fillTriangle(px + 6, fy, px + 14, fy, px + 10, fy + l, C(255, 200, 60)); }   // petits réacteurs quand il vole
  if (phase() == OEUF) dessinerOeuf(px, py, e); else dessinerBulle(px, py, e);
  if (millis() < finCaresse) { for (int i = 0; i < 3; i++) { int hx = px - 22 + i * 22, hy = py - (int)(44 * e) - (int)((1500 - (finCaresse - millis())) / 40) - (i % 2) * 6; cv.fillCircle(hx - 3, hy, 3, C(255, 60, 150)); cv.fillCircle(hx + 3, hy, 3, C(255, 60, 150)); cv.fillTriangle(hx - 6, hy + 1, hx + 6, hy + 1, hx, hy + 8, C(255, 60, 150)); } }   // petits cœurs quand on le caresse
  if (W.charge) effetChargeApres(px, py, e);
  if (millis() < finRepas) for (int i = 0; i < 3; i++) cv.fillCircle(GAM_X + GAM_W / 2 + (i - 1) * 8, 146 - (int)((millis() / 90 + i * 5) % 14), 2, C(80, 255, 140));
  dessinerInfos(); dessinerBarre();
  cv.pushSprite(0, 0);
}

// ---------- Page de réglages à l'écran ----------
static bool direct = false, changerDirect = false, eco = false;
void appliquerDirect() {
  if (direct || modeAcces) { WiFi.mode(WIFI_AP_STA); WiFi.softAP("Core2-francais", "core2fran"); }
  else { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); }
  if (R.ssid.length()) WiFi.begin(R.ssid.c_str(), R.motDePasse.c_str());
  else if (R.ssid2.length()) WiFi.begin(R.ssid2.c_str(), R.mdp2.c_str());
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
void lancerScan() { WiFi.scanDelete(); WiFi.scanNetworks(true); scanEnCours = true; debutScan = millis(); nbR = 0; decalR = 0; }
void lireScan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) { if (millis() - debutScan > 15000) scanEnCours = false; return; }
  scanEnCours = false; if (n < 0) return;
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
  sauverReglages();
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
    cv.drawString(emplacement == 2 ? "Pour : le téléphone (touche ici pour changer)" : "Pour : la maison (touche ici pour changer)", 10, 4);
    if (scanEnCours) cv.drawString("Recherche des réseaux… (2,4 GHz seulement)", 8, 60);
    else if (!nbR) cv.drawString("Aucun réseau trouvé : touche « Chercher »", 8, 60);
    for (int i = 0; i < 5; i++) {
      int k = decalR + i; if (k >= nbR) break; int y = 22 + i * 34;
      cv.fillRoundRect(4, y, 312, 30, 8, C(40, 46, 86));
      String n = listeR[k].nom; if (n.length() > 17) n = n.substring(0, 16) + "…";
      cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_left); cv.setTextColor(C(255, 255, 255)); cv.drawString(n, 10, y + 15);
      int pc = constrain(2 * (listeR[k].dbm + 100), 0, 100);
      cv.setFont(&fonts::lgfxJapanGothic_12); cv.setTextDatum(middle_right);
      cv.setTextColor(pc > 60 ? C(120, 255, 150) : (pc > 35 ? C(255, 220, 90) : C(255, 130, 110)));
      cv.drawString(String(pc) + " %  canal " + String(listeR[k].canal) + (listeR[k].ferme ? "" : "  ouvert"), 310, y + 15);
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
    else { cv.setTextColor(C(255, 130, 110)); cv.drawString("Échec de la connexion", 10, 60); cv.setTextColor(C(255, 255, 255)); cv.setFont(&fonts::lgfxJapanGothic_12); cv.drawString("Code " + String((int)WiFi.status()) + " : vérifie le mot de passe, puis réessaie.", 10, 90); }
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
      if (x < 96) page = 1;
      else if (x < 162) { if (decalR > 0) decalR--; }
      else if (x < 228) { if (decalR + 5 < nbR) decalR++; }
      else { lancerScan(); }
      bip(700, 40); return;
    }
    if (y < 21) { emplacement = (emplacement == 2) ? 1 : 2; bip(700, 40); return; }
    if (y >= 22 && y < 192) {
      int k = decalR + (y - 22) / 34;
      if (k < nbR) { nomChoisi = listeR[k].nom; mdpSaisi = ""; majKb = false; symbKb = false; bip(800, 50); if (listeR[k].ferme) page = 3; else validerWifi(); }
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
    if (y >= 204) { if (x < 160) page = (resultatConn == 2) ? 3 : 1; else page = 0; bip(700, 40); }
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
  {"Réglages", 1, 0x4A4F}, {"Matériel", 5, 0x3269}, {"Accueil", 0, 0x2C6B},
};
static const int NB_TUILES = sizeof(TUILES) / sizeof(TUILES[0]);
void dessinerMenu() {
  cadrePage("Menu — tout ce que je sais faire");
  for (int i = 0; i < NB_TUILES; i++) {
    int c = i % 3, r = i / 3; int x = 6 + c * 104, y = 30 + r * 44;
    cv.fillRoundRect(x, y, 100, 40, 8, TUILES[i].coul); cv.drawRoundRect(x, y, 100, 40, 8, C(120, 160, 220));
    cv.setFont(&fonts::lgfxJapanGothic_16); cv.setTextDatum(middle_center); cv.setTextColor(C(255, 255, 255)); cv.drawString(TUILES[i].nom, x + 50, y + 20);
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
    default: dessinerWifi(); break;
  }
}
void gererTactilePages(const m5::touch_detail_t& t) {
  bool clic = t.wasPressed();
  int x = t.x, y = t.y;
  if (page == 6) {
    if (!clic) return;
    for (int i = 0; i < NB_TUILES; i++) { int c = i % 3, r = i / 3; int tx = 6 + c * 104, ty = 30 + r * 44; if (x >= tx && x < tx + 100 && y >= ty && y < ty + 40) { page = TUILES[i].page; bip(700, 40); return; } }
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
void dessinerPerso1() { int px, py; float e; positionEcran(px, py, e); if (phase() == OEUF) dessinerOeuf(px, py, e); else dessinerBulle(px, py, e); }
// Bulle fait un petit salut de la main avant de s'endormir.
void saluerAuRevoir() {
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
  P.joie = min(100.0f, P.joie + 2); attitude(CONTENT, 2500); finCaresse = millis() + 1500; jouer(S_CARESSE);
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
  if (t.wasPressed()) {
    int x = t.x, y = t.y;
    if (y >= 204) { actionBarre(constrain((x - 4) / 106, 0, 2)); return; }
    if (hypot(x - px, y - py) < 64 * e) { /* zone de toucher large : on l'attrape facilement */ touchePerso = true; debutToucher = m; xPrec = x; yPrec = y; }
  }
  if (touchePerso && t.isPressed()) {
    int x = t.x, y = t.y;
    // Doigt posé plus d'une demi-seconde : on le soulève et il suit le doigt. Sinon, frotter = caresse.
    if (!P.porte && m - debutToucher > 500) { P.porte = true; attitude(SURPRIS, 600); }
    if (P.porte) {
      P.x = constrain((x - 160) / (110.0f * (1.0f - 0.45f * P.z)), -0.78f, 0.78f);
      P.y = constrain((y - 100) / (70.0f * (1.0f - 0.45f * P.z)), -0.78f, 0.78f);
      P.vx = (x - xPrec) / 10.0f; P.vy = (y - yPrec) / 10.0f;
    } else if (abs(x - xPrec) + abs(y - yPrec) > 3) caresser();
    xPrec = x; yPrec = y;
  }
  if (t.wasReleased()) {
    if (touchePerso && !P.porte) caresser();       // un simple toucher = une caresse (il ne bouge plus)
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

void autoriserCors() { serveur.sendHeader("Access-Control-Allow-Origin", "*"); }

void appliquerReglagesWeb() {
  for (int i = 0; i < M_NB; i++) if (serveur.hasArg(CLES_MODULES[i])) R.actif[i] = serveur.arg(CLES_MODULES[i]) == "1";
  if (serveur.hasArg("nom")) { String n = serveur.arg("nom"); n.trim(); if (n.length() && n.length() < 16) R.nom = n; }
  if (serveur.hasArg("ssid") && serveur.arg("ssid").length()) R.ssid = serveur.arg("ssid");
  if (serveur.hasArg("mdp") && serveur.arg("mdp").length()) R.motDePasse = serveur.arg("mdp");
  if (serveur.hasArg("ssid2") && serveur.arg("ssid2").length()) R.ssid2 = serveur.arg("ssid2");
  if (serveur.hasArg("mdp2") && serveur.arg("mdp2").length()) R.mdp2 = serveur.arg("mdp2");
  if (serveur.hasArg("oublier2")) { R.ssid2 = ""; R.mdp2 = ""; }
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
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) majOk = false;
    }
    else if (u.status == UPLOAD_FILE_WRITE) { esp_task_wdt_reset(); if (majOk && Update.write(u.buf, u.currentSize) != u.currentSize) majOk = false; }
    else if (u.status == UPLOAD_FILE_END) { if (!(majOk && Update.end(true))) majOk = false; else prefs.putInt("essais", 0); }
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
    autoriserCors(); int n = WiFi.scanNetworks(); JsonDocument d; JsonArray a = d.to<JsonArray>();
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
  if (!R.ssid.length() && !R.ssid2.length()) modeAcces = true;
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
  chargerReglages(); chargerPerso();
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
    if (brancheUsb && !avantUsb) { if (veille) reveillerEcran(); activite(); }       // on le branche : l'écran se rallume
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
    physique(dt);
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
    if (sniffActif || detection || page == 11 || page == 13) activite();
    if (!veille && !brancheUsb && (int32_t)(millis() - derniereActivite) > 300000) { nbSommeilTimeout++; endormirEcran(); }   // 5 min sans aucun toucher : veille (calcul signe pour ne jamais confondre un toucher avec un depassement)
  }
  vieDuPerso();
  {   // reconnexion automatique toutes les 20 s : on essaie le Wi-Fi de la maison, puis celui du téléphone, à tour de rôle
    static uint32_t tWifi = 0;
    if ((page < 2 || page == 5) && (R.ssid.length() || R.ssid2.length()) && m - tWifi > 20000) {
      tWifi = m;
      if (WiFi.status() != WL_CONNECTED) {
        essaiReseau++;
        bool deux = R.ssid2.length() && (!R.ssid.length() || (essaiReseau % 2) == 0);
        WiFi.disconnect();
        if (deux) WiFi.begin(R.ssid2.c_str(), R.mdp2.c_str()); else WiFi.begin(R.ssid.c_str(), R.motDePasse.c_str());
      }
    }
  }
  { static bool mdns = false; if (!mdns && WiFi.status() == WL_CONNECTED) { mdns = MDNS.begin("core2-francais"); if (mdns) MDNS.addService("http", "tcp", 80); } }
  if (changerDirect) { changerDirect = false; appliquerDirect(); }
  if (demandeMaj) majDepuisInternet();
  serveur.handleClient();
  validerSiStable();
  delay(veille ? 20 : (brancheUsb ? 1 : 4));
}
