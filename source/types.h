#pragma once
// Types du personnage (dans un fichier à part : le préprocesseur d'Arduino place les prototypes avant les types du programme principal).
enum Phase { OEUF = 0, BEBE, ENFANT, ADO, ADULTE, ANCIEN };
enum Attitude { NORMALE, CONTENT, TRISTE, PEUR, SOMMEIL, SURPRIS, FROID, CHAUD, AFFAME, ECOUTE, ETOURDI, PLUIE, ENERVE };
#include <stdint.h>
enum Evenement { EV_COURANT = 0, EV_CARESSE, EV_SECOUSSE, EV_BRUIT, EV_PLUIE, EV_FROID, EV_CHAUD, EV_FAIM, EV_NUIT, EV_WIFI, EV_RENVERSE, EV_NB };
struct Fiche {
  uint8_t version, forme; char nom[16];
  uint8_t r, g, b; bool couleurAuto, humeurCouleur;
  uint8_t carac[5];                 // curieux, câlin, farceur, bougon, timide : 0 à 100
  int8_t aime[EV_NB];               // -1 déteste, 0 indifférent, 1 aime
  uint8_t poids, milieu, gravite, saut, rebond, mode0g, chapeau;
  uint8_t cHum[13][3];              // couleur selon l'humeur (0,0,0 = ne change pas)
};
