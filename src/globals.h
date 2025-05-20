#ifndef GLOBALS_H
#define GLOBALS_H

#include "window.h"
#include "simulation.h"

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


#define MAPW 256
#define MAPH 256

typedef struct{
	int w;
	int h;
	float tileWidth; // width of one tile, used for adjusting fluid simulation
	argb_t argbSed[MAPW * MAPH];
	argb_t argbStone[MAPW * MAPH];
	argb_t argb[MAPW * MAPH];
	argb_t argbBlured[MAPW * MAPH];
	argb_t argbBuffer[MAPW * MAPH];
	float sunAngle; // 0-2PI, should represent the angle of the sun with PI / 2 being zenith
	float shadow[MAPW * MAPH];
	float height[MAPW * MAPH];
	float stone[MAPW * MAPH];
	float sand[MAPW * MAPH];
	uint8_t wetMap[MAPW * MAPH];
	// fluidSWE_t waterSWE;
	// new_fluid_t water;
	fluid_t water[MAPW * MAPH];
	vec2f_t waterVel[MAPW * MAPH]; // X/Y
	fluid_t mist[MAPW * MAPH];
	fluid_t lava[MAPW * MAPH];
	vec2f_t lavaVel[MAPW * MAPH]; // X/Y
    struct{
        uint8_t water : 1;
        uint8_t stone : 1;
        uint8_t sand  : 1;
        uint8_t mist  : 1;
        uint8_t lava  : 1;
    }present[MAPW * MAPH];
	float susSed[MAPW * MAPH];
	float susSed2[MAPW * MAPH];
	float lavaFoamLevel[MAPW * MAPH];
	float lavaFoamLevelBuffer[MAPW * MAPH];
	float foamLevel[MAPW * MAPH];
	float foamLevelBuffer[MAPW * MAPH];
	struct
	{
		unsigned int updateShadowMap : 1;
		unsigned int updateColorMap : 1;
	} flags;
} Map;


extern Map map;


#define DROPLET_MAX 8192*2*2*2*2*2*2*2
typedef struct {
	vec3f_t pos;
	vec3f_t vel;
} Droplet;

typedef struct 
{
	int head;
	int max;
	uint8_t active[DROPLET_MAX];
	Droplet array[DROPLET_MAX];
} Droplets;

extern Droplets droplets;



#endif // GLOBALS_H