#ifndef SYNCHRO_H
#define SYNCHRO_H

#include <stdbool.h>
#include "ensitheora.h"

extern bool fini;

/* Les extern des variables pour la synchro ici */

/* Synchronisation pour la lecture des flux */
extern pthread_cond_t cond_streamstate;
extern pthread_mutex_t mutex_streamstate;

/* Synchronisation pour la création de la fenêtre */
extern bool window_ready, window_size_ready;
extern pthread_cond_t cond_window;
extern pthread_mutex_t mutex_window;

/* Synchronisation pour la production/consommation des textures */
extern int textures_ready;
extern pthread_cond_t cond_textures;
extern pthread_mutex_t mutex_textures;

/* Fonctions de synchro à implanter */

void envoiTailleFenetre(th_ycbcr_buffer buffer);
void attendreTailleFenetre();

void attendreFenetreTexture();
void signalerFenetreEtTexturePrete();

void debutConsommerTexture();
void finConsommerTexture();

void debutDeposerTexture();
void finDeposerTexture();

#endif
