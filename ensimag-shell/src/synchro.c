#include "synchro.h"
#include "ensitheora.h"

#include <pthread.h>

bool fini;

/* les variables pour la synchro, ici */
pthread_t theora2sdlthread;
pthread_cond_t cond_streamstate;
pthread_mutex_t mutex_streamstate;
bool window_ready, window_size_ready;
pthread_cond_t cond_window;
pthread_mutex_t mutex_window;
int textures_ready;
pthread_cond_t cond_textures;
pthread_mutex_t mutex_textures;

/* l'implantation des fonctions de synchro ici */
void envoiTailleFenetre(th_ycbcr_buffer buffer) {
	pthread_mutex_lock(&mutex_window);
	printf("sending window size ...\n");
	windowsx = buffer[0].width;
	windowsy = buffer[0].height;
	window_size_ready = true;
	printf("signaling that window size is ready.\n");
	pthread_cond_signal(&cond_window);
	printf("window size sent\n");
	pthread_mutex_unlock(&mutex_window);
}

void attendreTailleFenetre() {
	pthread_mutex_lock(&mutex_window);
	printf("waiting to receive window size ...\n");
	while (!window_size_ready) {
		printf("window size isn't ready, sleep !\n");
		pthread_cond_wait(&cond_window, &mutex_window);
	}
	printf("window size received\n");
	pthread_mutex_unlock(&mutex_window);
}

void signalerFenetreEtTexturePrete() {
	pthread_mutex_lock(&mutex_window);
	printf("notifying that window and texture are ready ...\n");
	window_ready = true;
	printf("signaling that window and texture are ready.\n");
	pthread_cond_signal(&cond_window);
	printf("window and texture signaled as ready\n");
	pthread_mutex_unlock(&mutex_window);
}

void attendreFenetreTexture() {
	pthread_mutex_lock(&mutex_window);
	printf("waiting for window and texture to be ready ...\n");
	while (!window_ready) {
		printf("window and texture are not ready yet, sleep !\n");
		pthread_cond_wait(&cond_window, &mutex_window);
	}
	printf("window and texture are ready\n");
	pthread_mutex_unlock(&mutex_window);
}

void debutConsommerTexture() {
	pthread_mutex_lock(&mutex_textures);
	printf("getting a texture ...\n");
	while (textures_ready == 0) {
		printf("no texture ready yet, sleep !\n");
		pthread_cond_wait(&cond_textures, &mutex_textures);
	}
	printf("texture got\n");
	pthread_mutex_unlock(&mutex_textures);
}

void finConsommerTexture() {
	pthread_mutex_lock(&mutex_textures);
	printf("destroying a texture ...\n");
	textures_ready--;
	printf("signaling that room will be made.\n");
	pthread_cond_signal(&cond_textures);
	printf("texture destroyed (%d remaining)\n", textures_ready);
	pthread_mutex_unlock(&mutex_textures);
}


void debutDeposerTexture() {
	pthread_mutex_lock(&mutex_textures);
	printf("creating a texture ...\n");
	while (textures_ready == NBTEX) {
		printf("no room for my texture yet, sleep !\n");
		pthread_cond_wait(&cond_textures, &mutex_textures);
	}
	printf("texture created\n");
	pthread_mutex_unlock(&mutex_textures);
}

void finDeposerTexture() {
	pthread_mutex_lock(&mutex_textures);
	printf("adding a texture ...\n");
	textures_ready++;
	printf("signaling that a texture will be added.\n");
	pthread_cond_signal(&cond_textures);
	printf("texture added (%d in stock)\n", textures_ready);
	pthread_mutex_unlock(&mutex_textures);
}
