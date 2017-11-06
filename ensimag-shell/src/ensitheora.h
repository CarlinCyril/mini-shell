#ifndef THEORA_H
#define THEORA_H

#include <SDL2/SDL.h>
#include "ensivideo.h"

#define NBTEX 10

struct TextureDate {
	SDL_Texture *texture;
	double timems; // expiration date of the frame, in ms
};


extern struct streamstate *theorastrstate;

void *draw2SDL(void *arg);
void theora2SDL(struct streamstate *s);

extern int windowsx;
extern int windowsy;

#endif
