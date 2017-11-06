#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <SDL2/SDL.h>

#include "stream_common.h"
#include "oggstream.h"

#include <pthread.h>
#include "synchro.h"

int main(int argc, char *argv[]) {
	int res;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s FILE", argv[0]);
		exit(EXIT_FAILURE);
	}
	assert(argc == 2);


	// Initialisation de la SDL
	res = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS);
	atexit(SDL_Quit);
	assert(res == 0);

	/* Initialisation de toutes les variables utils à la synchronisation */
	pthread_cond_init(&cond_streamstate, NULL);
	pthread_mutex_init(&mutex_streamstate, NULL);

	window_ready = false;
	window_size_ready = false;
	pthread_cond_init(&cond_window, NULL);
	pthread_mutex_init(&mutex_window, NULL);

	textures_ready = 0;
	pthread_cond_init(&cond_textures, NULL);
	pthread_mutex_init(&mutex_textures, NULL);
	// start the two stream readers

	pthread_t theora_decoder_thread;
	pthread_t vorbis_decoder_thread;
	if (pthread_create(&theora_decoder_thread, NULL, theoraStreamReader, argv[1]) != 0) {
		fprintf(stderr, "Could not create Theora decoder thread\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Theora decoder thread started.\n");
	}
	if (pthread_create(&vorbis_decoder_thread, NULL, vorbisStreamReader, argv[1]) != 0) {
		fprintf(stderr, "Could not create Vorbis decoder thread\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Vorbis decoder thread started.\n");
	}

	// wait audio thread
	int return_value;
	pthread_join(vorbis_decoder_thread, (void*)(&return_value));
	printf("Vorbis decoder thread terminated with code %d\n", return_value);

	// 1 seconde de garde pour le son,
	sleep(1);

	// tuer les deux threads videos si ils sont bloqués
	pthread_cancel(theora_decoder_thread);
	pthread_cancel(theora2sdlthread);

	// attendre les 2 threads videos
	pthread_join(theora_decoder_thread, (void*)(&return_value));
	printf("Theora decoder thread terminated with code %d\n", return_value);
	pthread_join(theora2sdlthread, (void*)(&return_value));
	printf("Theora2SDL thread terminated with code %d\n", return_value);

	exit(EXIT_SUCCESS);    
}
