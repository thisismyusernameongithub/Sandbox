#ifndef GLOBALS_H
#define GLOBALS_H

#include "window.h"

typedef struct{
	argb_t white;
	argb_t black;
	argb_t gray;
	argb_t red;
	argb_t orange;
	argb_t green;
	argb_t blue;
	argb_t yellow;
    argb_t stone;
    argb_t waterDark;
    argb_t water;
    argb_t waterLight;
    argb_t foam;
    argb_t mist;
    argb_t sand;
	argb_t lava;
	argb_t lavaBright;
}Pallete;

extern Pallete pallete;

#endif // GLOBALS_H