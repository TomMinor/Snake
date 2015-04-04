#ifndef UTILS_H
#define UTILS_H

#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>


bool detectCollision(const SDL_Rect *_a, const SDL_Rect *_b, int _clipRadius);

int randRange(int _Min, int _Max);

#endif // UTILS_H
