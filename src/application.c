#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>	//sin/cos /M_PI
#include <string.h> //memcpy
#include <pthread.h>
#include <stdatomic.h>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#ifndef __EMSCRIPTEN__
	#define EMSCRIPTEN_KEEPALIVE
	#define EM_ASM_(...)
	#define EM_ASM(...)
	#define emscripten_run_script
#endif

#include "window.h"
#include "simulation.h"
#include "css_profile.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"



// Link -lgdi32 -lSDL2_ttf -lSDL2 -lm -lSDL2_image

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*(M_PI/180.f))
#define RAD2DEG(x) ((x)*(180.f/M_PI))

#define errLog(message) \
	fprintf(stderr, "\nFile: %s, Function: %s, Line: %d, Note: %s\n", __FILE__, __FUNCTION__, __LINE__, message);

#define DEBUG

#ifdef DEBUG
	#ifndef CSS_PROFILE_H_
		#define PROFILE(x) x
	#endif
#else /*DEBUG*/
	#ifdef PROFILE
		#undef PROFILE
	#endif
		#define PROFILE(x) x
#endif /*DEBUG*/

int NEWFEATURE = 1;
float newFloat = 0.79f;


struct{
	argb_t white;
	argb_t black;
	argb_t gray;
	argb_t red;
	argb_t orange;
	argb_t green;
	argb_t blue;
	argb_t yellow;
    argb_t stone;
    argb_t water;
    argb_t foam;
    argb_t mist;
    argb_t sand;
	argb_t lava;
}pallete = {
	.white.argb   = 0xFFFFFFFF,
    .black.argb   = 0xFF000000,
    .gray.argb    = 0xFF303030,
	.red.argb     = 0xFFFF0000,
	.orange.argb  = 0xFFFFA500,
	.green.argb   = 0xFF00FF00,
	.blue.argb    = 0xFF0000FF,
	.yellow.argb  = 0xFFFFFF00,
    .stone.argb   = 0xFF3D3533,
    .water.argb   = 0xFF4C8A85,
    .foam.argb    = 0xFFD2D2D2,
    .mist.argb    = 0xFFEBE9EC,
    .sand.argb    = 0xFFBAA588,
    .lava.argb    = 0xFFF69E4C
};


#define MAPW 256
#define MAPH 256

#define windowSizeX 800
#define windowSizeY 800
#define rendererSizeX 600
#define rendererSizeY 600

static inline int maxi(const int a, const int b){
    return (a > b) ? a : b;
}

static inline int mini(const int a, const int b)
{
    return (a < b) ? a : b;
}

static inline float maxf(const float a, const float b)
{
    return (a > b) ? a : b;
}

static inline float minf(const float a, const float b)
{
    return (a < b) ? a : b;
}

static float clampf(const float value, const float min, const float max) 
{
    const float t = value < min ? min : value;
    return t > max ? max : t;
}

static float cosLerp(const float y1, const float y2, const float mu)
{
	const double mu2 = (1-cos(mu*M_PI))/2;
	return(y1*(1-mu2)+y2*mu2);
}

static inline float lerp(const float s, const float e, const float t)
{
	return s + (e - s) * t;
}

static inline argb_t lerpargb(const argb_t s, const argb_t e, const float t)
{
	argb_t result;
	result.r = lerp(s.r, e.r, t);
	result.g = lerp(s.g, e.g, t);
	result.b = lerp(s.b, e.b, t);
	return result;
}

static inline float blerp(const float c00, const float c10, const float c01, const float c11, const float tx, const float ty)
{
	//    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
	const float s = c00 + (c10 - c00) * tx;
	const float e = c01 + (c11 - c01) * tx;
	return (s + (e - s) * ty);
}






struct{
	float simSpeed;
} program = {
	.simSpeed = 10.f
    };

struct{
	int screenX;
	int screenY;
	int worldX;
	int worldY;
	float worldVelX;
	float worldVelY;
	enum
	{
		TOOL_WATER = 1,
		TOOL_SAND = 2,
		TOOL_STONE = 3,
		TOOL_LAVA = 4,
		TOOL_MIST = 5,
		TOOL_FOAM = 6,
		TOOL_WIND = 7,
		TOOL_ERODE = 8
	} tool;
	float radius;
	float amount;
    struct{
        struct{
            float min;
            float max;
        }radius;
    }limits;
} cursor = {
    .tool = TOOL_WATER,
    .limits.radius.max = 100.f,
    .limits.radius.min = 5.f
    };

typedef struct{
	float x, y;
	float rot;
	float zoom;
    struct{
        float zoomMax;
        float zoomMin;
    }limits;
	enum
	{
		NE,
		SE,
		SW,
		NW
	} direction;
} camera_t;

camera_t g_cam = {
    .limits.zoomMax = 10.f,
    .limits.zoomMin = 0.03f
};

#define CHUNKW 64
#define CHUNKH 64
#define NOCHUNKSW 2
#define NOCHUNKSH 2

struct{
	int w;
	int h;
	float tileWidth; // width of one tile, used for adjusting fluid simulation
	argb_t argbSed[MAPW * MAPH];
	argb_t argbStone[MAPW * MAPH];
	struct{
		argb_t argb[CHUNKW * CHUNKH];
	}test[NOCHUNKSW * NOCHUNKSH];
	argb_t argb[MAPW * MAPH];
	argb_t argbBlured[MAPW * MAPH];
	argb_t argbBuffer[MAPW * MAPH];
	float shadowSoft[MAPW * MAPH];
	float shadow[MAPW * MAPH];
	float height[MAPW * MAPH];
	float stone[MAPW * MAPH];
	float sand[MAPW * MAPH];
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
} map;


typedef struct{
	float x;
	float y;
	float sand;
	float detail;
	float maxHeight;
	struct{
		int autoGenerate : 1;
	};
}TerrainGen;

TerrainGen terrainGen = {
	.autoGenerate = 0,
	.maxHeight = 100.f,
	.detail = 1.f,
	.sand = 0.5f,
	.x = 0.f,
	.y = 0.f
};


static argb_t getTileColorWater(int x, int y, int ys, vec2f_t upVec, float shade);
static void generateTerrain(float maxHeight, float detail, float sand, float xOffset, float yOffset);

argb_t frameBuffer[rendererSizeX * rendererSizeY];
argb_t background[rendererSizeX * rendererSizeY]; // Stores background image that get copied to framebuffer at start of render

float totalFoamLevel;
float totalSandLevel;
float totalStoneLevel;
float totalWaterLevel;
float totalMistLevel;
float totalLavaLevel;
Layer botLayer;
Layer topLayer;



static vec2f_t world2screen(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);

	// scale for zoom level
	float xs = x / camera.zoom;
	float ys = y / camera.zoom;
	// project

	// rotate
	float xw = cosAlpha * (xs + (camera.x - 614.911f)) + sinAlpha * (ys + (camera.y - 119.936f)) - (camera.x - 614.911f);
	float yw = -sinAlpha * (xs + (camera.x - 614.911f)) + cosAlpha * (ys + (camera.y - 119.936f)) - (camera.y - 119.936f);
	// offset for camPtrera position

	xs = xw + camera.x;
	ys = yw + camera.y;

	xw = ((xs - ys) / sqrtf(2.f));
	yw = ((xs + ys) / sqrtf(6.f));

	vec2f_t retVal = {xw, yw};

	return retVal;
}

static vec2f_t screen2world(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);
	float sq2d2 = sqrtf(2.f) / 2.f;
	float sq6d2 = sqrtf(6.f) / 2.f;
	// transform screen coordinates to world coordinates
	float xw = sq2d2 * (float)x + sq6d2 * (float)y;
	float yw = sq6d2 * (float)y - sq2d2 * (float)x;
	xw = xw - camera.x;
	yw = yw - camera.y;

	// rotate view
	float xwr = cosAlpha * (xw + (camera.x - 614.911f)) - sinAlpha * (yw + (camera.y - 119.936f)) - (camera.x - 614.911f);
	float ywr = sinAlpha * (xw + (camera.x - 614.911f)) + cosAlpha * (yw + (camera.y - 119.936f)) - (camera.y - 119.936f);
	// 616 121
	// apply zoom
	xw = xwr * camera.zoom;
	yw = ywr * camera.zoom;

	vec2f_t retVal = {xw, yw};
	// double xw = (cosAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*cam.zoom;
	// double yw = (sinAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*cam.zoom;
	return retVal;
}

static void cam_zoom(camera_t* camPtr, float value){
    		// if (camPtr->zoom > camPtr->limits.zoomMin && camPtr->zoom < camPtr->limits.zoomMax){
			float rotation = camPtr->rot; // save camera rotation
			camPtr->rot = 0.f;				 // set camera rotation to 0

			vec2f_t pos1 = screen2world(window.drawSize.w / 2.f, window.drawSize.h / 2.f, g_cam);
			camPtr->zoom += value * camPtr->zoom * window.time.dTime;
            camPtr->zoom = clampf(camPtr->zoom, camPtr->limits.zoomMin, camPtr->limits.zoomMax);
			vec2f_t pos2 = world2screen(pos1.x, pos1.y, g_cam);
			vec2f_t deltapos = {window.drawSize.w / 2.f - pos2.x, window.drawSize.h / 2.f - pos2.y};
			// transform coordinate offset to isometric
			float sq2d2 = sqrtf(2.f) / 2.f;
			float sq6d2 = sqrtf(6.f) / 2.f;
			float xw = sq2d2 * deltapos.x + sq6d2 * deltapos.y;
			float yw = sq6d2 * deltapos.y - sq2d2 * deltapos.x;
			camPtr->y += yw;
			camPtr->x += xw;
			camPtr->rot = rotation; // restore camera rotation
		// }
}

void erodeOld()
{
	
	float Pminslope = 0.01;
	float Pmaxslope = 10;
	float Pmaxdepth = -100; // max depth terrain can erode to (prevents super deep canyons)
	float Pcapacity = 4;
	float Pdeposition = 0.3; // how much of surplus sediment is deposited
	float Perosion = 0.3;	 // how much a droplet can erode a tile
	float Pgravity = 4;
	int Pmaxsteps = 100; // max number of steps for a droplet
	float Pevaporation = 0.01;
	float initialSpeed = 1;
	float initialWaterVolume = 1;
	float inertia = 0.1;

	for (int iter = 0; iter < 1000; iter++)
	{
		float dirX_new = 0;
		float dirY_new = 0;
		float dirX_old = 0;
		float dirY_old = 0;
		float x = 0;
		float y = 0;
		float hdiff = 0;
		float water = initialWaterVolume;
		float speed = initialSpeed;
		float sediment = 0;
		// get random starting coordinates
		x = rand() % map.w;
		y = rand() % map.h;

		float sedup = 0;
		float sedown = 0;
		for (int step = 0; step < Pmaxsteps; step++)
		{
			int oldPosID = (int)(x) + (int)(y) * map.w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// get slope gradient
			float slopeX = 0, slopeY = 0;
			float oldSlopeX = 0;
			float oldSlopeY = 0;
			for (int i = -1; i < 2; i++)
			{ // y
				for (int j = -1; j < 2; j++)
				{ // x
					int m = y + i;
					int n = x + j;
					if (m > 0 && m < map.h && n > 0 && n < map.w)
					{
						double dh = map.stone[oldPosID] + map.sand[oldPosID] - map.stone[n + m * map.w] - map.sand[n + m * map.w]; // height difference
						slopeX -= dh * j;
						slopeY -= dh * i;
						oldSlopeX += dh * j;
						oldSlopeY += dh * i;
					}
				}
			}
			// get new direction
			dirX_new = dirX_new * inertia - slopeX * (1 - inertia);
			dirY_new = dirY_new * inertia - slopeY * (1 - inertia);

			// normalize direction
			float dirLenght = sqrt(dirX_new * dirX_new + dirY_new * dirY_new);
			if (dirLenght != 0)
			{
				dirX_new = dirX_new / dirLenght;
				dirY_new = dirY_new / dirLenght;
			}
			// get new pos by adding direction to old pos
			x += dirX_new;
			y += dirY_new;
			int newPosID = (int)(x) + (int)(y) * map.w;
			// check if droplet is still within the map
			if ((dirX_new == 0 && dirY_new == 0) || x < 0 || x > map.w - 1 || y < 0 || y > map.h - 1)
			{
				// printf("Out of bounds\n");
				break;
			}
			// get height difference
			hdiff = map.stone[newPosID] + map.sand[newPosID] - map.stone[oldPosID] - map.sand[oldPosID];
			// calculate new carrying capacity
			float capacity = minf(maxf(-hdiff, Pminslope) * speed * water * Pcapacity, Pmaxslope);
			if (map.stone[newPosID] + map.sand[newPosID] < -1)
				capacity = Pminslope * speed * water * Pcapacity;
			// if drop carries more sediment then capacity
			if (sediment > capacity || hdiff > 0)
			{
				if (hdiff > 0)
				{ // uphill
					// deposit at old pos
					// fill the hole but not with more than hdiff
					float amount = minf(hdiff, sediment);
					sediment -= amount;
					map.sand[oldPosID] += amount;
					sedown += amount;
				}
				else
				{
					// drop (sediment - capacity)*Pdeposition at old pos
					float amount = (sediment - capacity) * Pdeposition;
					sediment -= amount;
					map.sand[oldPosID] += amount;
					sedown += amount;
				}
			}
			else
			{ // if drop carries less sediment then capacity, pick up sediment
				// get std::min((c-sediment)*Perosion,-hdiff) sediment from old pos
				float amount = minf((capacity - sediment) * Perosion, -hdiff);
				sediment += amount;
				// pick up sediment from an area around the old pos using normal distribution
				for (int j = -4; j <= 4; j++)
				{
					for (int k = -4; k <= 4; k++)
					{
						if (oldX >= 0 && oldX <= map.w && oldY >= 0 && oldY <= map.h)
						{
							float temp_amount = amount * (exp(-(k * k + j * j) / (2 * 8)) / (2 * 3.14159265359 * 8)) / 1.14022; // amount to pick up
							if (map.sand[oldPosID] > 0)
							{ // if there is sediment
								float diff = map.sand[oldPosID + k + j * map.w] - temp_amount;
								if (diff > 0)
								{ // there is more sediment then is picked up
									map.sand[oldPosID + k + j * map.w] -= temp_amount;
									sedup += temp_amount;
									temp_amount = 0;
								}
								else
								{ // there is less sediment then is picked up
									temp_amount -= map.sand[oldPosID + k + j * map.w];
									sedup += temp_amount;
									map.sand[oldPosID + k + j * map.w] = 0;
								}
							}
							sedup += temp_amount;
							map.stone[oldPosID + k + j * map.w] -= temp_amount; //*exp(-(k*k+j*j)/(2))/(2*3.14159265359)/1.32429; //remove rest of sediment from rock
						}
					}
				}
			}
			// get new speed
			speed = sqrt(speed * speed + fabsf(hdiff) * Pgravity);
			// evaporate some water
			water = water * (1 - Pevaporation);
			//		std::cout << "pos:" << x << "," << y << " dirX:" << dirX_new << " dirY:" << dirY_new << " speed:" << speed << std::endl;
			//		std::cout << " hdiff:" << hdiff  << " capacity:" << capacity << " sediment:"<< sediment << " water:" << water << std::endl;
		}
		// std::cout << "up:" <<  sedup << " down:" << sedown << std::endl;
	}
	

	float rocksum = 0;
	float sedsum = 0;
	for (int y = 0; y < map.w; y++)
	{

		for (int x = 0; x < map.w; x++)
		{
			rocksum += map.stone[x + y * map.w];
			sedsum += map.sand[x + y * map.w];
		}
	}
	
	float sum = 0;
	for (int j = -4; j <= 4; j++)
	{
		for (int k = -4; k <= 4; k++)
		{
			sum += exp(-(k * k + j * j) / (2 * 8)) / (2 * 3.14159265359 * 8) / 1.14022; // amount to pick up
		}
	}
	
}

float mapBuffer[MAPW*MAPH];
float mapBuffer2[MAPW*MAPH];
void erode()
{
	
	float Pminslope = 0.f;
	float Pmaxslope = 1.f;
	float Pcapacity = 2.f;
	float Pdeposition = 0.1f; // how much of surplus sediment is deposited
	float Perosion = 0.01f;	 // how much a droplet can erode a tile
	float Pgravity = 9.81f;
	int Pmaxsteps = 100; // max number of steps for a droplet
	float Pevaporation = 0.01f;
	float initialSpeed = 1.f;
	float initialWaterVolume = 10.f;
	float inertia = 0.5f;

	for (int iter = 0; iter < 1000; iter++)
	{
		float dirX_new = 0;
		float dirY_new = 0;
		float dirX_old = 0;
		float dirY_old = 0;
		float x = 0;
		float y = 0;
		float hdiff = 0;
		float water = initialWaterVolume;
		float speed = initialSpeed;
		float sediment = 0;
		// get random starting coordinates
		x = rand() % map.w;
		y = rand() % map.h;



		for (int step = 0; step < Pmaxsteps; step++)
		{

			int oldPosID = (int)(x) + (int)(y) * map.w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// get slope gradient
			float slopeX = map.stone[(oldX + 1) + (oldY) * map.w] + map.sand[(oldX + 1) + (oldY) * map.w] - map.stone[(oldX - 1) + (oldY) * map.w] - map.sand[(oldX - 1) + (oldY) * map.w]; // height difference
			float slopeY = map.stone[(oldX) + (oldY + 1) * map.w] + map.sand[(oldX) + (oldY + 1) * map.w] - map.stone[(oldX) + (oldY - 1) * map.w] - map.sand[(oldX) + (oldY - 1) * map.w]; // height difference
			// get new direction
			dirX_new = dirX_new * inertia - slopeX * (1 - inertia);
			dirY_new = dirY_new * inertia - slopeY * (1 - inertia);

			// normalize direction
			float dirLenght = sqrt(dirX_new * dirX_new + dirY_new * dirY_new);
			if (dirLenght != 0)
			{
				dirX_new = dirX_new / dirLenght;
				dirY_new = dirY_new / dirLenght;
			}
			// get new pos by adding direction to old pos
			x += dirX_new;
			y += dirY_new;
			int newPosID = (int)(x) + (int)(y) * map.w;
			// check if droplet is still within the map
			if(!((int)x > 1 && (int)y > 1 && (int)x < map.w - 1 && (int)y < map.h - 1))
			{
				map.sand[oldPosID] += sediment;
				break;
			}
			// get height difference
			hdiff = map.stone[newPosID] + map.sand[newPosID] - map.stone[oldPosID] - map.sand[oldPosID];
			// calculate new carrying capacity
			float capacity = minf(maxf(-hdiff, Pminslope),Pmaxslope) * speed * water * Pcapacity;
			// printf("%d: %f * %f * %f * %f = %f\n", step, hdiff, speed, water, Pcapacity, capacity);
			// if (map.stone[newPosID] + map.sand[newPosID] < -1){
			// 	capacity = Pminslope * speed * water * Pcapacity;
			// }
			// printf("step: %d capacity: %f\n", step, capacity);


			// if drop carries more sediment then capacity
			if (sediment > capacity || hdiff > 0)
			{
				if (hdiff > 0)
				{ // uphill
					// deposit at old pos
					// fill the hole but not with more than hdiff
					float amount = minf(hdiff, sediment);
					sediment -= amount;
					map.sand[oldPosID] += amount;

				}
				
				{
					// drop (sediment - capacity)*Pdeposition at old pos
					float amount = (sediment - capacity) * Pdeposition;
					sediment -= amount;
					map.sand[oldPosID] += amount;

				}
			}
			else
			{ // if drop carries less sediment then capacity, pick up sediment
				// get std::min((c-sediment)*Perosion,-hdiff) sediment from old pos
				float amount = minf((capacity - sediment) * Perosion, -hdiff);
				// sediment += amount;

				float radius = 4.f;
				float sigma = radius / 4.f; // Width of distribution
				float s = 2.f * sigma * sigma; //Standard deviation I think
				// pick up sediment from an area around the old pos using normal distribution
				for (int j = -radius; j <= radius; j++)
				{
					for (int k = -radius; k <= radius; k++)
					{
						if (oldX + k > 1 && oldY + j > 1 && oldX + k < map.w - 1 && oldY + j < map.h - 1)
						{
							float r = sqrtf(k * k + j * j);
							float temp_amount = amount * expf(-(r * r) / (s)) / (M_PI * s);
							map.stone[oldPosID + k + j * map.w] += minf(map.sand[oldPosID + k + j * map.w] - temp_amount, 0.f);
							map.sand[oldPosID + k + j * map.w] = maxf(map.sand[oldPosID + k + j * map.w] - temp_amount, 0.f);
							sediment += temp_amount;
							// float temp_amount = amount * (exp(-(k * k + j * j) / (2 * 8)) / (2 * 3.14159265359 * 8)) / 1.14022; // amount to pick up
							// if (map.sand[oldPosID] > 0)
							// { // if there is sediment
							// 	float diff = map.sand[oldPosID + k + j * map.w] - temp_amount;
							// 	if (diff > 0)
							// 	{ // there is more sediment then is picked up
							// 		map.sand[oldPosID + k + j * map.w] -= temp_amount;

							// 		temp_amount = 0;
							// 	}
							// 	else
							// 	{ // there is less sediment then is picked up
							// 		temp_amount -= map.sand[oldPosID + k + j * map.w];

							// 		map.sand[oldPosID + k + j * map.w] = 0;
							// 	}
							// }

							// map.stone[oldPosID + k + j * map.w] -= temp_amount; //*exp(-(k*k+j*j)/(2))/(2*3.14159265359)/1.32429; //remove rest of sediment from rock
						}
					
					}
				}
				// get new speed
				speed = sqrt(speed * speed + fabsf(hdiff) * Pgravity);
				// evaporate some water
				water = water * (1 - Pevaporation);
				
			}

		}
	}
	


}


EMSCRIPTEN_KEEPALIVE
void selectToolStone()
{
	cursor.tool = TOOL_STONE;
	printf("Stone\n");
}
EMSCRIPTEN_KEEPALIVE
void selectToolSand()
{
	cursor.tool = TOOL_SAND;
	printf("Sand\n");
}
EMSCRIPTEN_KEEPALIVE
void selectToolWater()
{
	cursor.tool = TOOL_WATER;
	printf("water\n");
}
EMSCRIPTEN_KEEPALIVE
void selectToolLava()
{
	cursor.tool = TOOL_LAVA;
	printf("Lava\n");
}
EMSCRIPTEN_KEEPALIVE
void selectToolMist()
{
	cursor.tool = TOOL_MIST;
	printf("Mist\n");
}
EMSCRIPTEN_KEEPALIVE
void selectToolFoam()
{
	cursor.tool = TOOL_FOAM;
	printf("Foam\n");
}
EMSCRIPTEN_KEEPALIVE
void changeToolRadius(float radius)
{
	cursor.radius = radius;
	printf("radius %f\n", cursor.radius);
}
EMSCRIPTEN_KEEPALIVE
void changeToolAmount(float amount)
{
	cursor.amount = amount;
	printf("amount %f\n", cursor.amount);
}
EMSCRIPTEN_KEEPALIVE
void changeMapGenDetail(float detail)
{
	terrainGen.detail = detail;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y);
	}
}
EMSCRIPTEN_KEEPALIVE
void changeSandHeight(float sand)
{
	terrainGen.sand = sand;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y);
	}
}
EMSCRIPTEN_KEEPALIVE
void generateMap()
{
	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y);
}
EMSCRIPTEN_KEEPALIVE
void setAutoGenerate(int yesPleaseDoThat)
{
	if(yesPleaseDoThat) printf("Auto generate on\n");
	else printf("Auto generate off\n");
	terrainGen.autoGenerate = yesPleaseDoThat;
}
EMSCRIPTEN_KEEPALIVE
void setmapGenX(int x)
{
	terrainGen.x = x;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y);
	}
}
EMSCRIPTEN_KEEPALIVE
void setmapGenY(int y)
{
	terrainGen.y = y;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y);
	}
}

static void updateInput(){
	cursor.screenX = mouse.pos.x;
	cursor.screenY = mouse.pos.y;
	vec2f_t pos = screen2world(cursor.screenX, cursor.screenY, g_cam);
	cursor.worldVelX = (pos.x - (float)cursor.worldX) * window.time.dTime;
	cursor.worldVelY = (pos.y - (float)cursor.worldY) * window.time.dTime;
	cursor.worldX = pos.x;
	cursor.worldY = pos.y;

	if (mouse.left == eKEY_HELD || mouse.right == eKEY_HELD){
        float add = ((mouse.left) ? 1 : 0) + ((mouse.right) ? -1 : 0); //If left is pressed we add by 1, if right is pressed we subtract by 1, if both then they cancel out to 0

		float sigma = cursor.radius / 4.f; // Width of distribution
		float s = 2.f * sigma * sigma; //Standard deviation I think

		float radius = cursor.radius;
		for (int j = -radius; j <= radius; j++){
			for (int k = -radius; k <= radius; k++){
				float r = sqrtf(k * k + j * j);
				if (r > radius){ continue; } //Skip if outside of cursor cirlce radius

				switch (cursor.tool){
				case TOOL_WATER:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						map.water[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth = maxf(map.water[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[cursor.worldX+cursor.worldY*map.w].water = 1;
					}
					break;
				case TOOL_SAND:
					if (cursor.worldX + k > 1 && cursor.worldY + j > 1 && cursor.worldX + k < map.w - 1 && cursor.worldY + j < map.h - 1){
						map.sand[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.sand[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
					}
					break;
				case TOOL_STONE:
					if (cursor.worldX + k > 0 && cursor.worldY + j > 0 && cursor.worldX + k < map.w - 0 && cursor.worldY + j < map.h - 0){
						map.stone[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.stone[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
					}
					break;
				case TOOL_FOAM:
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						map.foamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.foamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
					}
					break;
                case TOOL_WIND:
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						map.water[(cursor.worldX + k) + (cursor.worldY + j) * map.w].up += 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						map.water[(cursor.worldX + k) + (cursor.worldY + j) * map.w].down -= 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						map.mist[(cursor.worldX + k) + (cursor.worldY + j) * map.w].up += 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						map.mist[(cursor.worldX + k) + (cursor.worldY + j) * map.w].down -= 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
					}
					break;
                case TOOL_MIST:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						map.mist[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth = maxf(map.mist[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[cursor.worldX+cursor.worldY*map.w].mist = 1;
					}
					break;
				case TOOL_LAVA:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						map.lava[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth = maxf(map.lava[(cursor.worldX + k) + (cursor.worldY + j) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[cursor.worldX+cursor.worldY*map.w].lava = 1;
					}
					break;
				default:

					break;
				}
			}
		}
	}


	if (key.A == eKEY_HELD){
		g_cam.x += cosf(g_cam.rot - (M_PI / 4.f)) * 300.f * window.time.dTime;
		g_cam.y += sinf(g_cam.rot - (M_PI / 4.f)) * 300.f * window.time.dTime;
	}
	if (key.D == eKEY_HELD){
		g_cam.x -= cosf(g_cam.rot - (M_PI / 4.f)) * 300.f * window.time.dTime;
		g_cam.y -= sinf(g_cam.rot - (M_PI / 4.f)) * 300.f * window.time.dTime;
	}
	if (key.W == eKEY_HELD){
		g_cam.x -= sinf(g_cam.rot - (M_PI / 4.f)) * 450.f * window.time.dTime;
		g_cam.y += cosf(g_cam.rot - (M_PI / 4.f)) * 450.f * window.time.dTime;

		// Print camera pos
		// printf("camera: x:%f y:%f rot:%f zoom:%f", g_cam.x, g_cam.y, g_cam.rot, g_cam.zoom);
	}
	if (key.S == eKEY_HELD){
		g_cam.x += sinf(g_cam.rot - (M_PI / 4.f)) * 450.f * window.time.dTime;
		g_cam.y -= cosf(g_cam.rot - (M_PI / 4.f)) * 450.f * window.time.dTime;
	}
	if (key.R == eKEY_HELD){
		cam_zoom(&g_cam, 1.f);
	}
	if (key.F == eKEY_HELD){
        cam_zoom(&g_cam, -1.f);
	}
	if (key.Q == eKEY_HELD){
		float angle = 32.f * M_PI / 180.f;
		g_cam.rot = fmod((g_cam.rot - angle * 2.f * window.time.dTime), 6.283185307f);
		if (g_cam.rot < 0.f)
			g_cam.rot = 6.283185307f;
	}
	if (key.E == eKEY_HELD){
		float angle = 32.f * M_PI / 180.f;
		g_cam.rot = fmod((g_cam.rot + angle * 2.f * window.time.dTime), 6.283185307f);
	}
	if (key.ESC == eKEY_HELD){
		window.closeWindow = true;
	}

	if (key.num1 == eKEY_PRESSED){
		cursor.tool = TOOL_STONE;
		EM_ASM(Stone.checked = true;);
	}

	if (key.num2 == eKEY_PRESSED){
		cursor.tool = TOOL_SAND;
		EM_ASM(Sand.checked = true;);
	}

	if (key.num3 == eKEY_PRESSED){
		cursor.tool = TOOL_WATER;
		EM_ASM(Water.checked = true;);
	}

    if (key.num4 == eKEY_PRESSED){
		cursor.tool = TOOL_LAVA;
		EM_ASM(Lava.checked = true;);
	}    

    if (key.num5 == eKEY_PRESSED){
		cursor.tool = TOOL_MIST;
		EM_ASM(Mist.checked = true;);
	}

	if (key.num6 == eKEY_PRESSED){
		cursor.tool = TOOL_FOAM;
		EM_ASM(Foam.checked = true;);
	}    

	if (key.num7 == eKEY_PRESSED){
		cursor.tool = TOOL_WIND;
	}

	
	if (key.num9 == eKEY_PRESSED){
		cursor.tool = TOOL_ERODE;
	}



	if(mouse.dWheel){
		if(key.shiftLeft == eKEY_HELD){
			newFloat += 0.01f * mouse.dWheel;
			printf("%f\n",newFloat);
            // cam_zoom(&g_cam, mouse.dWheel);

		}else if(key.ctrlLeft == eKEY_HELD){
			cursor.amount += mouse.dWheel;
			cursor.amount = clampf(cursor.amount, 1.f, 10.f);
				EM_ASM_({
				toolAmountAmount.value = $0;
				toolAmountSlider.value = $0;
			}, cursor.amount);
		}else{
			cursor.radius += mouse.dWheel;
			cursor.radius = clampf(cursor.radius, cursor.limits.radius.min, cursor.limits.radius.max);
			EM_ASM_({
				toolRadiusAmount.value = $0;
				toolRadiusSlider.value = $0;
			},cursor.radius);
		}
	}



	if (key.K == eKEY_PRESSED){
		PROFILE(erode();)
	}
	if (key.L == eKEY_PRESSED){
		NEWFEATURE = (NEWFEATURE) ? 0 : 1;
	}
}


float shadow[MAPW * MAPH];
static void generateShadowMap()
{
    int R = 20;
    int Rmin = 1;
    int Rmax = 40;
    float A = 1.f;
    float Amin = -2.f;
    float Amax = 2.f;

	map.flags.updateShadowMap = 0;


	float scale = 2;

	// Copy heap stored variables to stack before calculation
	int mapW = map.w;
	int mapH = map.h;
	for (int i = 0; i < mapW * mapH; i++)
		shadow[i] = 0.90f;

	//		Calculate shadows by iterating over map diagonally like example below.
	//		------- Save the highest tileheight in diagonal and decrease by 1 each step.
	//		|6|3|1| If current tile is higher, save that one as new highest point.
	//		|8|5|2| If not then that tile is in shadow.
	//		|9|7|4|
	//		------- ONLY WORKS ON SQUARE MAPS!!!
	//
	int diagonalLines = (mapW + mapH) - 1;	// number of diagonal lines in map
	int midPoint = (diagonalLines / 2) + 1; // number of the diagonal that crosses midpoint of map
	int itemsInDiagonal = 0;				// stores number of tiles in a diagonal

	for (int diagonal = 1; diagonal <= diagonalLines; diagonal++)
	{
		float terrainPeakHeight = 1;
		int x, y;
		if (diagonal <= midPoint)
		{
			itemsInDiagonal++;
			for (int item = 0; item < itemsInDiagonal; item++)
			{
				y = (diagonal - item) - 1;
				x = mapW - item - 1;
				terrainPeakHeight -= 0.5f * A;
				if (terrainPeakHeight > map.height[x + y * mapW])
				{
					if ((terrainPeakHeight - map.height[x + y * mapW]) > 2.f)
					{
						shadow[x + y * mapW] -= 0.10f;
					}
					else
					{
						//At the edge of the shadow, interpolate between shaded value and no shade
						shadow[x + y * mapW] -= 0.05f * (terrainPeakHeight - map.height[x + y * mapW]);
					}
				}
				else
				{
					terrainPeakHeight = map.height[x + y * mapW];
				}
				//Add shading based on angle
				//TODO: Will sample outside shadow map 
				shadow[x+y*map.w] += clampf(((map.height[(x-1)+(y-1)*map.w] - map.height[(x+1)+(y+1)*map.w]) / 20.f), -0.05f, 0.05f);
			}
		}
		else
		{
			itemsInDiagonal--;
			for (int item = 0; item < itemsInDiagonal; item++)
			{
				y = (mapH - 1) - item;
				x = diagonalLines - diagonal - item;
				terrainPeakHeight -= 0.5f * A;
				if (terrainPeakHeight > map.height[x + y * mapW])
				{
					if ((terrainPeakHeight - map.height[x + y * mapW]) > 2.f)
					{
						shadow[x + y * mapW] -= 0.1f;
					}
					else
					{
						//At the edge of the shadow, interpolate between shaded value and no shade
						shadow[x + y * mapW] -= 0.05f * (terrainPeakHeight - map.height[x + y * mapW]);
					}
				}
				else
				{
					terrainPeakHeight = map.height[x + y * mapW];
				}
				//Add shading based on angle
				//TODO: Will sample outside shadow map 
				shadow[x+y*map.w] += clampf(((map.height[(x-1)+(y-1)*map.w] - map.height[(x+1)+(y+1)*map.w]) / 20.f), -0.05f, 0.05f);
			}
		}
	}

	// ambient occlusion
	// Calculate ambient occlusion using a box filter, optimized with technique found here: http://blog.ivank.net/fastest-gaussian-blur.html#results

	//	int R = 20;
	// float diameterDiv = 1.f / (R + 1 + R);
	// for (int i = 0; i < mapH; i++)
	// {
	// 	int ti = i * mapW;
	// 	int li = ti;
	// 	int ri = ti + R;
	// 	float fv = map.height[ti];
	// 	float lv = map.height[ti + mapW - 1];
	// 	float val = (float)(R + 1) * fv;
	// 	for (int j = 0; j < R; j++)
	// 	{
	// 		val += map.height[ti + j];
	// 	}
	// 	for (int j = 0; j <= R; j++)
	// 	{
	// 		val += map.height[ri++] - fv;
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		ti++;
	// 	}
	// 	for (int j = R + 1; j < mapW - R; j++)
	// 	{
	// 		val += map.height[ri++] - map.height[li++];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		ti++;
	// 	}
	// 	for (int j = mapW - R; j < mapW; j++)
	// 	{
	// 		val += lv - map.height[li++];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		ti++;
	// 	}
	// }
	// for (int i = 0; i < mapW; i++)
	// {
	// 	int ti = i;
	// 	int li = ti;
	// 	int ri = ti + R * mapW;
	// 	float fv = map.height[ti];
	// 	float lv = map.height[ti + mapW * (mapH - 1)];
	// 	float val = (float)(R + 1) * fv;
	// 	for (int j = 0; j < R; j++)
	// 	{
	// 		val += map.height[ti + j * mapW];
	// 	}
	// 	for (int j = 0; j <= R; j++)
	// 	{
	// 		val += map.height[ri] - fv;
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		ri += mapW;
	// 		ti += mapW;
	// 	}
	// 	for (int j = R + 1; j < mapH - R; j++)
	// 	{
	// 		val += map.height[ri] - map.height[li];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		li += mapW;
	// 		ri += mapW;
	// 		ti += mapW;
	// 	}
	// 	for (int j = mapH - R; j < mapH; j++)
	// 	{
	// 		val += lv - map.height[li];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv));
	// 		li += mapW;
	// 		ti += mapW;
	// 	}
	// }

	// smooth shadows
	//	boxBlur_4(shadow,map.shadow,mapW*mapH,mapW,mapH,1);
	//	boxBlur_4(map.shadow,shadow,mapW*mapH,mapW,mapH,1);
	//	boxBlur_4(shadow,map.shadow,mapW*mapH,mapW,mapH,10);
	// smoother shadows
	//	boxBlur_4(shadow,map.shadowSoft,mapW*mapH,mapW,mapH,4);

	for(int y=0;y<map.h/2;y++){
		for(int x=0;x<map.w;x++){
			// shadow[x+y*map.w] += (map.height[(x)+(y)*map.w] - map.height[(x+1)+(y+1)*map.w] + map.height[(x-1)+(y-1)*map.w] - map.height[(x)+(y)*map.w]) / 2.f;
		}
	}

	for(int y=map.h/2;y<map.h;y++){
		for(int x=0/2;x<map.w;x++){
			// shadow[x+y*map.w] += sqrtf((map.height[(x)+(y)*map.w] - map.height[(x+1)+(y+1)*map.w])*(map.height[(x)+(y)*map.w] - map.height[(x+1)+(y+1)*map.w]) + (map.height[(x-1)+(y-1)*map.w] - map.height[(x)+(y)*map.w])*(map.height[(x-1)+(y-1)*map.w] - map.height[(x)+(y)*map.w]));
		}
	}

	for (int i = 0; i < mapW * mapH; i++){
		// shadow[i] = 0.5f;
	}

	//Clamp shadow value
	for (int i = 0; i < mapW * mapH; i++){
		// shadow[i] = minf(maxf(shadow[i], 0.40f), 0.60f);
	}




	memcpy(map.shadow, shadow, sizeof(shadow));
}


static void generateColorMap()
{
	map.flags.updateColorMap = false;


	// create normalized vector that point up on the screen but in world coorinates
	vec2f_t ftl = screen2world(0, 0, g_cam);
	vec2f_t fbl = screen2world(0, window.drawSize.h, g_cam);
	vec2f_t upVec = {ftl.x - fbl.x, ftl.y - fbl.y};
	upVec = normalizeVec2f(upVec);

	for (int y = 0; y < map.h; y++)
	{
		int mapPitch = y * map.w;
		for (int x = 0; x < map.w; x++)
		{
			argb_t argb;

			//Blend between sand and stone
			argb = lerpargb(map.argbStone[x + mapPitch], map.argbSed[x + mapPitch], minf(map.sand[x + mapPitch]/2.f, 1.0f));
			
			

			if(map.present[x + mapPitch].lava){
				float slopX = (map.height[(x + 1) + (y) * map.w] - map.height[(x - 1) + (y) * map.w]); //The thing at the end with makes it so if the x or y position is on the border then slopX and Y gets multiplied by 0 otherwise by 1
				float slopY = (map.height[(x) + (y + 1) * map.w] - map.height[(x) + (y - 1) * map.w]);

				float lavaHeight = map.lava[x + mapPitch].depth;
				float slopeX = map.lava[(x+1) + (y)*map.w].depth - map.lava[(x-1) + (y)*map.w].depth;
				float slopeY = map.lava[(x) + (y+1)*map.w].depth - map.lava[(x) + (y-1)*map.w].depth;
				argb.r = mini(pallete.stone.r + lavaHeight*50, pallete.lava.r) ;
				argb.g = mini(pallete.stone.g + lavaHeight*15, pallete.lava.g) ;
				argb.b = mini(pallete.stone.b + lavaHeight*5 , pallete.lava.b) ;
				
				argb = lerpargb(argb, pallete.stone, minf(map.lavaFoamLevel[x+ mapPitch], 1.f));
				// if(slopeX + slopeY > 0.1 && slopeX + slopeY < 1){
				// 	argb.r += 20;
				// 	argb.g += 20;
				// 	argb.b += 20;
				// }
				// argb = pallete.red;
				// argb = lerpargb(argb, pallete.lava, minf(lavaHeight / 10.f, 1.f));

				// // highligt according to slope
				vec2f_t slopeVec = {.x = slopX + 0.000000001f, .y = slopY + 0.000000001f}; //The small addition is to prevent normalizing a zero length vector which we don't handle
				slopeVec = normalizeVec2f(slopeVec);
				
				float glare = ((slopeVec.x - upVec.x)*(slopeVec.x - upVec.x)+(slopeVec.y - upVec.y)*(slopeVec.y - upVec.y)) * ((x-3) && (y-3) && (x-map.w+3) && (y-map.h+3));
				if(glare != glare) printf("heh\n");
				glare = minf(glare*0.05f, 1.f);

				argb = lerpargb(argb, pallete.white, glare);

			}

			//Add water if present
			if(map.present[x + mapPitch].water){
				float slopX = (map.height[(x + 1) + (y) * map.w] - map.height[(x - 1) + (y) * map.w]); //The thing at the end with makes it so if the x or y position is on the border then slopX and Y gets multiplied by 0 otherwise by 1
				float slopY = (map.height[(x) + (y + 1) * map.w] - map.height[(x) + (y - 1) * map.w]);

				argb = lerpargb(argb, pallete.water, minf(map.water[x + mapPitch].depth*0.05f, 1.f));


				if (map.susSed[x + y * map.w] > 0)
				{
					argb.r = lerp(argb.r, pallete.sand.r, minf(map.susSed[x + mapPitch], 0.75f));
					argb.g = lerp(argb.g, pallete.sand.g, minf(map.susSed[x + mapPitch], 0.75f));
					argb.b = lerp(argb.b, pallete.sand.b, minf(map.susSed[x + mapPitch], 0.75f));
				}
				
				// // highligt according to slope
				vec2f_t slopeVec = {.x = slopX + 0.000000001f, .y = slopY + 0.000000001f}; //The small addition is to prevent normalizing a zero length vector which we don't handle
				slopeVec = normalizeVec2f(slopeVec);
				
				float glare = ((slopeVec.x - upVec.x)*(slopeVec.x - upVec.x)+(slopeVec.y - upVec.y)*(slopeVec.y - upVec.y)) * ((x-3) && (y-3) && (x-map.w+3) && (y-map.h+3));
				if(glare != glare) printf("heh\n");
				glare = minf(glare*0.1f, 1.f);

				argb = lerpargb(argb, pallete.white, glare);
			}

			//Add foam
			if (map.foamLevel[x + mapPitch] > 0)
			{
				argb = lerpargb(argb, pallete.foam, minf(map.foamLevel[x + mapPitch] / 10.f, 1.f));
			}

			//Add shadowmap if no lava because lava is emmisive
			if(!map.present[x + mapPitch].lava){
				argb.r *= map.shadow[x + mapPitch];
				argb.g *= map.shadow[x + mapPitch];
				argb.b *= map.shadow[x + mapPitch];
			}

			map.argb[x + mapPitch] = argb;
		}
	}




	memcpy(map.argbBuffer, map.argb, sizeof(map.argb));
	// memcpy(map.argbBlured, map.argbBuffer, sizeof(map.argb));
	gaussBlurargb(map.argbBuffer, map.argbBlured, map.w*map.h, map.w, map.h, 10);
	
}


static void process(float dTime)
{
    int w = map.w;
    int h = map.h;

    //foam
    //spawn foam where water is turbulent
    for(int y=1;y<h-1;y++){
        for(int x=1;x<w-1;x++){

//			float velX = map.waterVel[x+y*map.w].x;
//			float velY = map.waterVel[x+y*map.w].y;
            //curl is something, velDiff is speed difference with nearby tiles
//			float curl = map.waterVel[(x+1)+y*map.w].y - map.waterVel[(x-1)+y*map.w].y - map.waterVel[x+(y+1)*map.w].x + map.waterVel[x+(y-1)*map.w].y;
            float velDiff = map.waterVel[x+y*w].x*2 - map.waterVel[(x-1)+(y)*w].x - map.waterVel[(x+1)+(y)*w].x + map.waterVel[x+y*w].y*2 - map.waterVel[x+(y-1)*w].y - map.waterVel[x+(y+1)*w].y;
            // float deltaV = (map.water[(x-1)+(y)*MAPW].right+map.water[(x)+(y+1)*MAPW].down+map.water[(x+1)+(y)*MAPW].left+map.water[(x)+(y-1)*MAPW].up - (map.water[(x)+(y)*MAPW].right+map.water[(x)+(y)*MAPW].down+map.water[(x)+(y)*MAPW].left+map.water[(x)+(y)*MAPW].up));

            if(velDiff > 1.f){
                // map.foamLevel[x+y*h] = map.foamLevel[x+y*h] + 1.25f;
            }

			// map.foamLevel[x+y*map.w] += 0.1f*(map.waterVel[x+y*map.w].x*map.waterVel[x+y*map.w].x+map.waterVel[x+y*map.w].y*map.waterVel[x+y*map.w].y);

            map.foamLevel[x+y*w]  = minf(map.foamLevel[x+y*w], 1000.f);
        	map.foamLevel[x+y*w] -= minf(map.foamLevel[x+y*w], 1.f * dTime);

//- map.lava[x+y*w].depth/10.f
			if(map.lava[x+y*w].depth > 0.f){
				float lavaConverted = minf(map.lava[x+y*w].depth, 1.f * dTime);
				map.lava[x+y*w].depth -= lavaConverted; 
				map.stone[x+y*w] += lavaConverted;
				map.stone[x+y*w] += map.sand[x+y*w];
				map.sand[x+y*w] = 0.f;

				// float r = sinf(window.time.ms10 / 100.f);
				// map.argbStone[x + y * map.w].r = pallete.stone.r + r*5.f;
				// map.argbStone[x + y * map.w].g = pallete.stone.g + r*5.f;
				// map.argbStone[x + y * map.w].b = pallete.stone.b + r*5.f;

				if(map.water[x+y*w].depth > 0.f){
					lavaConverted = minf(minf(map.lava[x+y*w].depth, map.water[x+y*w].depth), 2.f * dTime);
					map.lava[x+y*w].depth  -= lavaConverted; 
					map.stone[x+y*w]       += lavaConverted;
					map.water[x+y*w].depth -= lavaConverted * 1.f; 
					map.water[x+y*w].down  -= lavaConverted * 1.f; 
					map.water[x+y*w].up    -= lavaConverted * 1.f; 
					map.water[x+y*w].left  -= lavaConverted * 1.f; 
					map.water[x+y*w].right -= lavaConverted * 1.f; 
					map.mist[x+y*w].depth  += lavaConverted * 5.f; 
					// map.mist[x+y*w].down   += lavaConverted * 5.f; 
					// map.mist[x+y*w].up     += lavaConverted * 5.f; 
					// map.mist[x+y*w].left   += lavaConverted * 5.f; 
					// map.mist[x+y*w].right  += lavaConverted * 5.f; 

				}
			}

        }
    }


    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] = map.stone[x + y * w] + map.sand[x + y * w];
        }
    }

    PROFILE(simFluid(map.water, map.height, 9.81f, 0.f, 1.f, w, h, 0.97f, minf(dTime*program.simSpeed, 0.13f));)



	//Handle border conditions of fluids
	for (int y = 0; y < h; y++)
	{
		map.water[3 + y * w].depth += map.water[2 + y * w].depth;
		map.water[2 + y * w].depth = 0.f;
		map.water[2 + y * w].right += map.water[3 + y * w].left;
		map.water[3 + y * w].left = 0.f;
		map.water[(w - 3) + y * w].depth += map.water[(w - 2) + y * w].depth;
		map.water[(w - 2) + y * w].depth = 0.f;
		map.water[(w - 2) + y * w].left += map.water[(w - 3) + y * w].right;
		map.water[(w - 3) + y * w].right = 0.f;
		
	}
	for (int x = 0; x < w; x++)
	{
		map.water[x + 3 * w].depth += map.water[x + 2 * w].depth;
		map.water[x + 2 * w].depth = 0.f;
		map.water[x + 2 * w].up += map.water[x + 3 * w].down;
		map.water[x + 3 * w].down = 0;
		map.water[x + (h - 3) * w].depth += map.water[x + (h - 2) * w].depth;
		map.water[x + (h - 2) * w].depth = 0.f;
		map.water[x + (h - 2) * w].down += map.water[x + (h - 3) * w].up;
		map.water[x + (h - 3) * w].up = 0.f;
	}


    //Add water to height
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] += map.water[x + y * w].depth;
        }
    }

    PROFILE(simFluid(map.lava, map.height, 9.81f, 20.f, 1.f, w, h, 1.f, minf(dTime*program.simSpeed, 0.13f));)

	//Add lava to height
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] += map.lava[x + y * w].depth;
        }
    }


    PROFILE(simFluid(map.mist, map.height, 9.81f, 0.f, 1.f, w, h, 0.90f, minf(dTime*program.simSpeed, 0.13f));)



    for(int y=0;y<h-0;y++){
        for(int x=0;x<w-0;x++){
            // calculate velocity
            map.waterVel[x + y * w].x = (map.water[(x - 1) + (y)*w].right - map.water[(x) + (y)*w].left + map.water[(x) + (y)*w].right - map.water[(x + 1) + (y)*w].left) / (2.f); // X
            map.waterVel[x + y * w].y = (map.water[(x) + (y - 1)*w].down - map.water[(x) + (y)*w].up + map.water[(x) + (y)*w].down - map.water[(x) + (y + 1)*w].up) / (2.f);       // Y

			map.lavaVel[x + y * w].x = (map.lava[(x - 1) + (y)*w].right - map.lava[(x) + (y)*w].left + map.lava[(x) + (y)*w].right - map.lava[(x + 1) + (y)*w].left) / (2.f); // X
            map.lavaVel[x + y * w].y = (map.lava[(x) + (y - 1)*w].down - map.lava[(x) + (y)*w].up + map.lava[(x) + (y)*w].down - map.lava[(x) + (y + 1)*w].up) / (2.f);       // Y

            map.present[x + y * w].water = (map.water[x + y * w].depth > 0.01f) ? 1 : 0;
            map.present[x + y * w].mist  = (map.mist[x + y * w].depth > 0.01f) ? 1 : 0;
            map.present[x + y * w].lava  = (map.lava[x + y * w].depth > 0.01f) ? 1 : 0;
        }
    }


    
    PROFILE(erodeAndDeposit(map.sand, map.susSed, map.stone, map.water, map.waterVel, w, h);)
    PROFILE(relax(map.sand, map.stone, 40.f, 9.81f, w, h, minf(dTime*program.simSpeed, 0.13f)); )
    PROFILE(advect(map.susSed, map.susSed2, map.waterVel, w, h, minf(dTime*program.simSpeed, 0.13f));)

    //Advect the foam
    PROFILE(advect(map.foamLevel, map.foamLevelBuffer, map.waterVel, w, h, minf(dTime*program.simSpeed, 0.13f));)
    PROFILE(advect(map.lavaFoamLevel, map.lavaFoamLevelBuffer, map.lavaVel, w, h, minf(dTime*program.simSpeed, 0.13f));)


#ifdef DEBUG
    //Count the total amount of foam on map
    totalFoamLevel = 0.f;
    for(int i = 0; i < w*h; i++){
        totalFoamLevel += map.foamLevel[i]; //Count total foamLevel for last loop (We don't count this loop because then we have to wait for the write to finish to do the read)
    }

    //Count total amount of sand
    totalSandLevel = 0.f;
    for(int i = 0; i < w*h; i++){
        totalSandLevel += map.sand[i];
        totalSandLevel += map.susSed[i];
    }
	//Count total amount of stone
    totalStoneLevel = 0.f;
    for(int i = 0; i < w*h; i++){
        totalStoneLevel += map.stone[i];
    }
	//Count total amount of water
	totalWaterLevel = 0.f;
    for(int i = 0; i < w*h; i++){
        totalWaterLevel += map.water[i].depth;
    }
#endif /*DEBUG*/

}

argb_t getTileColorMist(int x, int y, int ys, vec2f_t upVec){

//Ide, använd screen2world för att få rutan som motsvarar botten istället.
    argb_t argb = pallete.red;

    float mistHeight =  map.height[x+y*map.w] + map.mist[x+y*map.w].depth;

    float d;
    for(d = 0.f; d < 300.f; d += 1.f){
		int X = x+upVec.x*d;
		int Y = y+upVec.y*d;
		if(X >= 0 && X < map.w && Y >= 0 && Y < map.h){
			if(map.height[(int)(x+upVec.x*d)+(int)(y+upVec.y*d)*map.w] > mistHeight - (d * 0.79f)){ //0.7071f = 1/sqrt(2)

				argb = map.argb[X+Y*map.w];

				argb = lerpargb(argb, map.argbBlured[X+Y*map.w], minf(d/10.f, 1.f));

				break;
			}
		}else{
			argb.r = (102+(int)ys)>>2;//67
			argb.g = (192+(int)ys)>>2;//157
			argb.b = (229+(int)ys)>>2;//197

            break;

		}
    }

	
	// vec2f_t a = world2screen(upVec.x, upVec.y, g_cam);
	// vec2f_t b = world2screen(upVec.x + 1.f, upVec.y + 1.f, g_cam);
	// printf("%f %f %f %f %f\n",a.x, a.y, b.x, b.y, b.y-a.y);


	argb = lerpargb(argb, pallete.mist, minf(d/50.f, 1.f));



    // argb.r = lerp(argb.r, pallete.white.r, mistDensity);
    // argb.g = lerp(argb.g, pallete.white.g, mistDensity);
    // argb.b = lerp(argb.b, pallete.white.b, mistDensity);

	return argb;
	
}



// renders one pixel column
static void renderColumn(int x, int yBot, int yTop, vec2f_t upVec, float xwt, float ywt, float dDxw, float dDyw, camera_t camera)
{
	// save some variables as local

	// init some variables

	int border = false; // used when skiping pixels to decide when at a edge of a tile that should be a darker shade
	int dPixels;		// how many pixels are skipped to get to the next tile

	// init ybuffer with lowest mappoint at current x screen position
	// ybuffer stores the last drawn lowest position so there is no overdraw

	int ybuffer = minf(yBot, window.drawSize.h); // Limit lowest drawing point (ybuffer start value) to edge of screen

	// start drawing column
	for (int y = yBot; y > yTop; y -= dPixels)
	{
		int ys;

		int ywti = (int)ywt;
		int xwti = (int)xwt;
		float camZoom = camera.zoom;
		float camZoomDiv = 1.f / camera.zoom;

		//        float gndHeight = map.packed[posID].height;//map.stone[posID] + map.sand[posID]; //not using map.height ensures instant terrain update, map.height is okay for blurry/unclear renderings as underwater
		//        float wtrHeight = map.packed[posID].water; //map.water[posID].depth;
		float mistHeight = 0; // Map.mistHeight[posID];
		float lavaHeight = 0; // Map.lavaHeight[posID];



		ys = y - (map.height[xwti + ywti * map.w] + map.mist[xwti + ywti * map.w].depth) * (camZoomDiv) /  sqrtf(2.f);  // offset y by terrain height (sqr(2) is to adjust for isometric projection)
		// ys = y - (mapPack.height) * camZoomDiv; // offset y by terrain height
		ys = ys * !(ys & 0x80000000);			// Non branching version of : ys = maxf(ys, 0);

		if (ys < ybuffer)
		{
			// get color at worldspace and draw at screenspace
			register argb_t argb; // store color of Pixel that will be drawn

			// draw mist if present
			if (map.present[xwti + ywti*map.w].mist)
			{
                argb = getTileColorMist(xwti, ywti, ys, upVec);
			}else{
				argb = map.argb[xwti + ywti * map.w];
			}
			// else if (map.present[xwti + ywti*map.w].water)
			// { // draw water if present
			// 	argb = getTileColorWater(xwti, ywti, ys, upVec, 1.f);

            //     argb.r *= map.shadow[xwti + ywti * map.w];
            //     argb.g *= map.shadow[xwti + ywti * map.w];
            //     argb.b *= map.shadow[xwti + ywti * map.w];

			// }
			// else if (lavaHeight > 0.f)
			// { // draw lava if present
			// 	//	rgb lavaRGB = getTileColorLava(xwti, ywti, 0.f);
			// 	//	r = lavaRGB.r;
			// 	//	g = lavaRGB.g;
			// 	//	b = lavaRGB.b;
            //         argb.r = 0;
			// 		argb.g = 0;
			// 		argb.b = 0;
			// }
			// else
			// { // only ground
			// 	//				xwt + ywt*map.w;  	202, 188, 145
            //     argb = getTileColorGround(xwti, ywti);

            //     if (map.foamLevel[xwti + ywti * map.w] > 0)
            //     {
            //         argb.r = lerp(argb.r, pallete.foam.r, minf(map.foamLevel[xwti + ywti * map.w] / 20.f, 1.f));
            //         argb.g = lerp(argb.g, pallete.foam.g, minf(map.foamLevel[xwti + ywti * map.w] / 20.f, 1.f));
            //         argb.b = lerp(argb.b, pallete.foam.b, minf(map.foamLevel[xwti + ywti * map.w] / 20.f, 1.f));
            //     }

            //     argb.r *= map.shadow[xwti + ywti * map.w];
            //     argb.g *= map.shadow[xwti + ywti * map.w];
            //     argb.b *= map.shadow[xwti + ywti * map.w];

			// }

			



			// calculate and draw cursor branchlessly
			// argb.g += 30 * (((uint32_t)(((xwti - cursor.worldX) * (xwti - cursor.worldX) +
			// 							 (ywti - cursor.worldY) * (ywti - cursor.worldY)) -
			// 							((int)(cursor.radius) << 4)) &
			// 				 0x80000000) >>
			// 				31);

			if((xwti - cursor.worldX) * (xwti - cursor.worldX) + (ywti - cursor.worldY) * (ywti - cursor.worldY) < cursor.radius*4){ //Why *4?
				switch (cursor.tool)
				{
				case TOOL_WATER:
					argb = lerpargb(argb, pallete.blue, 0.1f + 0.02f * cursor.amount);
					break;
				case TOOL_SAND:
					argb = lerpargb(argb, pallete.yellow, 0.1f + 0.02f * cursor.amount);
					break;
				case TOOL_STONE:
					argb = lerpargb(argb, pallete.white, 0.1f + 0.02f * cursor.amount);
					break;
				case TOOL_FOAM:
					argb = lerpargb(argb, pallete.green, 0.1f + 0.02f * cursor.amount);
					break;
                case TOOL_MIST:
					argb = lerpargb(argb, pallete.mist, 0.1f + 0.02f * cursor.amount);
					break;
				case TOOL_LAVA:
					argb = lerpargb(argb, pallete.orange, 0.1f + 0.02f * cursor.amount);
					break;
                case TOOL_WIND:
					argb = lerpargb(argb, pallete.red, 0.1f + 0.02f * cursor.amount);
                    break;
				default:
					argb = lerpargb(argb, pallete.black, 0.1f + 0.02f * cursor.amount);
					break;
				}
				
			}




			// make borders of tiles darker, make it so they become darker the more zoomed in you are
			if (camZoom < 0.3f && !border)
			{
				argb = lerpargb(argb, pallete.black, camZoomDiv/255.f);
			}

			// clamp color values
			//			argb = (minf(maxf(r,0),255) << 16) | (minf(maxf(g,0),255) << 8) | (minf(maxf(b,0),255)); //85-87

			//                argb = (argb_t) {
			//                        .r = CLAMP(r, 0, 255),
			//                        .g = CLAMP(g, 0, 255),
			//                        .b = CLAMP(b, 0, 255)
			//                };
			//                    ((CLAMP(r,0,255)) << 16) | ((CLAMP(g,0,255)) << 8) | (CLAMP(b,0,255)); //86-90

			// only draw visible pixels
			for (register int Y = ybuffer - 1; Y >= ys; Y--)
			{
				//            for (register int Y = ys ; Y < ybuffer ; Y++) {
				//                int ix = rendTexture.h - (ybuffer-ys) + Y + (x) * rendTexture.h;

				//                frameBuffer[ix] = argb;    //draw pixels
				frameBuffer[window.drawSize.h - 1 - Y + (x)*window.drawSize.h] = argb; // draw pixels
			}
			

			ybuffer = ys; // save current highest point in pixel column
		}

		// this piece of code calculates how many y pixels (dPixels) there is to the next tile
		// and the border thing makes it so it only jumps one pixel when there is a
		// new tile and the next drawing part is darker, thus making the edge of the tile darker
		if (!border)
		{ //! border
			border = 1;
			float testX, testY;
			switch (camera.direction)
			{
			case NW:
				testX = ((xwt - (int)xwt)) / dDxw;
				testY = ((ywt - (int)ywt)) / dDyw;
				break;
			case NE:
				testX = ((1 - (xwt - (int)xwt)) / dDxw);
				testY = (((ywt - (int)ywt)) / dDyw);
				break;
			case SE:
				testX = (1 - (xwt - (int)xwt)) / dDxw;
				testY = (1 - (ywt - (int)ywt)) / dDyw;
				break;
			case SW:
				testX = (((xwt - (int)xwt)) / dDxw);
				testY = ((1 - (ywt - (int)ywt)) / dDyw);
				break;
			default:
				fprintf(stderr, "Unitialized value used at %s %d\n", __FILE__, __LINE__);
				exit(0);
			}
			dPixels = minf(fabs(testX), fabs(testY));
			if (dPixels < 1)
				dPixels = 1;
			xwt += dDxw * dPixels;
			ywt += dDyw * dPixels;
		}
		else
		{

			border = 0;
			xwt += dDxw;
			ywt += dDyw;
			dPixels = 1;
		}
	}
}

static void render()
{
	// Since this runs on a separate thread from input update I need to back up camera variables so they don't change during rendering
	camera_t cam;
	cam.rot = g_cam.rot;
	cam.zoom = g_cam.zoom;
	cam.x = g_cam.x;
	cam.y = g_cam.y;
	cam.direction = g_cam.direction;

	// Copy background to framebuffer
	memcpy(frameBuffer, background, sizeof(frameBuffer));



	float xw, yw;
	float xs, ys;

	// furstum
	xs = 0;
	ys = 0;
	vec2f_t ftl = screen2world(xs, ys, cam);
	xs = window.drawSize.w;
	ys = window.drawSize.h;
	vec2f_t fbr = screen2world(xs, ys, cam);
	xs = 0;
	ys = window.drawSize.h;
	vec2f_t fbl = screen2world(xs, ys, cam);
	xs = 0;
	ys = window.drawSize.h + 100.f / cam.zoom;

	float dDxw = (ftl.x - fbl.x) / (float)window.drawSize.h; // delta x worldspace depth
	float dDyw = (ftl.y - fbl.y) / (float)window.drawSize.h; // delta y worldspace depth

	// create normalized vector that point up on the screen but in world coorinates, for use with raytracing water refraction
	vec2f_t upVec = {ftl.x - fbl.x, ftl.y - fbl.y};
	float tempDistLongName = sqrtf(upVec.x * upVec.x + upVec.y * upVec.y);
	upVec.x /= tempDistLongName;
	upVec.y /= tempDistLongName;

	///////// merge these calculations later
	// calculate screen coordinates of world corners
	vec2f_t tlw = world2screen(1, 1, cam);
	vec2f_t trw = world2screen(map.w, 1, cam);
	vec2f_t blw = world2screen(1, map.h, cam);
	vec2f_t brw = world2screen(map.w, map.h, cam);
	// check what relative postion map corners have
	vec2f_t mapCornerTop, mapCornerLeft, mapCornerBot, mapCornerRight;
	switch (cam.direction)
	{
	case NW:
		mapCornerTop = (vec2f_t){tlw.x, tlw.y};
		mapCornerLeft = (vec2f_t){blw.x, blw.y};
		mapCornerBot = (vec2f_t){brw.x, brw.y};
		mapCornerRight = (vec2f_t){trw.x, trw.y};
		break;
	case NE:
		mapCornerRight = (vec2f_t){brw.x, brw.y};
		mapCornerTop = (vec2f_t){trw.x, trw.y};
		mapCornerLeft = (vec2f_t){tlw.x, tlw.y};
		mapCornerBot = (vec2f_t){blw.x, blw.y};
		break;
	case SE:
		mapCornerBot = (vec2f_t){tlw.x, tlw.y};
		mapCornerRight = (vec2f_t){blw.x, blw.y};
		mapCornerTop = (vec2f_t){brw.x, brw.y};
		mapCornerLeft = (vec2f_t){trw.x, trw.y};
		break;
	case SW:
		mapCornerLeft = (vec2f_t){brw.x, brw.y};
		mapCornerBot = (vec2f_t){trw.x, trw.y};
		mapCornerRight = (vec2f_t){tlw.x, tlw.y};
		mapCornerTop = (vec2f_t){blw.x, blw.y};
		break;
	default:
		fprintf(stderr, "Unitialized value used at %s %d\n", __FILE__, __LINE__);
		exit(0);
		break;
	}

	// calculate slope of tile edges on screen
	float tileEdgeSlopeRight = (float)(mapCornerRight.y - mapCornerBot.y) / (float)(mapCornerRight.x - mapCornerBot.x);
	float tileEdgeSlopeLeft = (float)(mapCornerBot.y - mapCornerLeft.y) / (float)(mapCornerBot.x - mapCornerLeft.x);
	/////////

	// these coordinates will be the bounds at which renderColumn() will render any terrain
	int leftMostXCoord = maxf((int)mapCornerLeft.x, 0);
	int rightMostXCoord = minf((int)mapCornerRight.x, window.drawSize.w);

	for (int x = leftMostXCoord; x < rightMostXCoord; x++)
	{
		int botMostYCoord;
		int topMostYCoord;

		if (x > mapCornerBot.x)
		{
			botMostYCoord = mapCornerBot.y + tileEdgeSlopeRight * (x - mapCornerBot.x);
		}
		else
		{
			botMostYCoord = mapCornerBot.y + tileEdgeSlopeLeft * (x - mapCornerBot.x);
		}
		if (x > mapCornerTop.x)
		{
			topMostYCoord = mapCornerTop.y + tileEdgeSlopeLeft * (x - mapCornerTop.x);
		}
		else
		{
			topMostYCoord = mapCornerTop.y + tileEdgeSlopeRight * (x - mapCornerTop.x);
		}

		topMostYCoord = topMostYCoord * !(topMostYCoord & 0x80000000); // Branchless topMostYCoord = maxf(topMostYCoord, 0);

		vec2f_t worldCoord = screen2world(x, botMostYCoord, cam);

		PROFILE(renderColumn(x, botMostYCoord, topMostYCoord, upVec, worldCoord.x, worldCoord.y, dDxw, dDyw, cam);)
	}

	// for memory access pattern reasons the terrain gets drawn sideways. That's why we below transpose the framebuffer while copying it to rendTexture
	for (int y = 0; y < window.drawSize.h; y++)
	{
		int pixelPitch = y * window.drawSize.w;
		int bufferPitch = window.drawSize.h - 1 - y;
		for (int x = 0; x < window.drawSize.w; x++)
		{
			botLayer.frameBuffer[x + pixelPitch] = frameBuffer[bufferPitch + (x)*window.drawSize.w];
		}
	}
}



static void generateTerrain(float maxHeight, float detail, float sand, float xOffset, float yOffset)
{
	
	int w = MAPW;
	int h = MAPH;
	

	float min = 0.f;
	float max = 0.f;
	float min2 = 0.f;
	float max2 = 0.f;
	int noOctaves = detail * 9.f;
	if(noOctaves == 0){
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				map.stone[x + y * w] = 10.f;
				map.sand[x + y * w] = 0.f;

				min = 0.f;
				max = 100.f;
			}
		}
	}else{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				map.stone[x + y * w] = 0.f;
				map.sand[x + y * w] = 0.f;
				float oPow2 = 1.f;
				for(int o = 1; o <= noOctaves; o++, oPow2 *= 2.f){ 
					
					map.stone[x + y * w] += -fabsf(stb_perlin_noise3(oPow2*((float)x + xOffset)/(float)map.w, oPow2*((float)y + yOffset)/(float)map.h, 0, 0, 0, 0) / (oPow2));
					
				}
					


				min = minf(map.stone[x + y * w], min);
				max = maxf(map.stone[x + y * w], max);
			}
			// printf("\rGenerating terrain %f", (float)y / (float)h);
			// fflush(stdout);
		}
	}

	float sandHeight = clampf(sand, 0.f, 1.f) * maxHeight;

	float hDiff = max - min;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			//Normalize terrain height and multiply by max Height
			map.stone[x + y * w] = (-min + (map.stone[x + y * w])) / hDiff * maxHeight;

			//Flatten the stone to a height of 10m below sand height, then add 10m of sand on top along with dunes.
			if(map.stone[x + y * w] < sandHeight){
				if(map.stone[x + y * w] < sandHeight - 10.f){
					map.stone[x + y * w] = lerp(map.stone[x + y * w], (sandHeight - 10.f), (1.f - map.stone[x + y * w]/maxHeight));
				}
				
				// map.sand[x + y * w] = lerp(map.stone[x + y * w], sandHeight, (1.f - map.stone[x + y * w]/maxHeight)) - map.stone[x + y * w];
			}


			min2 = minf(map.stone[x + y * w], min2);
			max2 = maxf(map.stone[x + y * w], max2);
		}
	}

	for (int y = 2; y < h-2; y++)
	{
		for (int x = 2; x < w-2; x++)
		{
			if(map.stone[x + y * w] < sandHeight){
				map.sand[x + y * w] = (-min + stb_perlin_ridge_noise3(((float)x + xOffset)/(float)map.w, ((float)y + yOffset)/(float)map.h, 0.f, 2.5f, 2.8f, 1.f, 3, 0, 0, 0))  * 10.f * (1.f - map.stone[x + y * w] / sandHeight);
			}
		}

	}

	// for (int y = 0; y < h; y++)
	// {
	// 	for (int x = 0; x < w; x++)
	// 	{
	// 		vec2f_t prim = {
	// 			.x = map.stone[(x+1)+(y)*w] - map.stone[(x-1)+(y)*w],
	// 			.y = map.stone[(x)+(y+1)*w] - map.stone[(x)+(y-1)*w]
	// 			};
	// 		vec2f_t bis = {
	// 			.x = (map.stone[(x+1)+(y)*w] - map.stone[(x)+(y)*w]) - (map.stone[(x)+(y)*w] - map.stone[(x-1)+(y)*w]),
	// 			.y = (map.stone[(x)+(y+1)*w] - map.stone[(x)+(y)*w]) - (map.stone[(x)+(y)*w] - map.stone[(x)+(y-1)*w])
	// 		};


			
	// 		// map.argbStone[(x)+(y)*w].r += bis.x*10.f;
	// 		// map.argbStone[(x)+(y)*w].g += bis.y*10.f;
	// 		// map.argbStone[(x)+(y)*w].b += bis.y*10.f;
	// 		// map.argbStone[(x)+(y)*w].g = 0.f;
	// 		// map.argbStone[(x)+(y)*w].b = 0.f;
	// 	}
	// }




	printf("\nMapelevation extremes: %f -> %f\n" ,min2, max2);

}

static void init()
{

	map.w = MAPW;
	map.h = MAPH;
	map.tileWidth = 1.f;

	// init camera position, the following will init camera to center overview a 256x256 map
	// camera: x:336.321991 y:-93.287224 rot:1.570000 zoom:0.609125camera: x:327.101379 y:-84.052345 rot:1.570000 zoom:0.609125
	g_cam.x = 221.321991;
	g_cam.y = 21.287224;
	g_cam.rot = 3.14f / 2;
	g_cam.zoom = 0.609125;

	//Init tool values
	cursor.amount = 10;
	cursor.radius = 15;
	cursor.tool = TOOL_WATER;



	//Call javascript that synchronise initialized settings with html gui
	switch (cursor.tool)
	{
	case TOOL_WATER:
		EM_ASM(Water.checked = true;);
		break;
	case TOOL_SAND:
		EM_ASM(Sand.checked = true;);
		break;	
	case TOOL_STONE:
		EM_ASM(Stone.checked = true;);
		break;
	default:
		break;
	}
	//set html sliders
	EM_ASM_({
		toolAmountAmount.value = $0;
		toolAmountSlider.value = $0;
		toolRadiusAmount.value = $1;
		toolRadiusSlider.value = $1;
    }, cursor.amount, cursor.radius);


	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand,terrainGen.x, terrainGen.y);

	map.flags.updateShadowMap = 1; // make sure shadows are updated after map load
	map.flags.updateColorMap = 1;  // make sure shadows are updated after map load

	// Create background for render
	for (int x = 0; x < window.drawSize.w; x++)
	{
		argb_t argb = (argb_t){.b = ((219 + x) >> 2), .g = (((182 + x) >> 2)), .r = (((92 + x) >> 2))};
		for (int y = 0; y < window.drawSize.h; y++)
		{

			background[window.drawSize.w - 1 - x + y * window.drawSize.w] = argb;
		}
	}

	// Generate initial stone rgb map
	
	for (int y = 0; y < map.h; y++)
	{
		for (int x = 0; x < map.w; x++)
		{//     octaves    =   6     -- number of "octaves" of noise3() to sum
//     lacunarity = ~ 2.0   -- spacing between successive octaves (use exactly 2.0 for wrapping output)
//     gain       =   0.5   -- relative weighting applied to each successive octave
//     offset     =   1.0?  -- used to invert the ridges, may need to be larger, not sure
//
			map.argbStone[x + y * map.w] = pallete.stone;
			float r = stb_perlin_turbulence_noise3((float)x / (float)map.w, (float)y / (float)map.h,0.f, 20.0f, 0.5f, 4, 0, 0, 0);
			map.argbStone[x + y * map.w].r += (0.5f + r/2.f)*50.f - 50;
			map.argbStone[x + y * map.w].g += (0.5f + r/2.f)*50.f - 50;
			map.argbStone[x + y * map.w].b += (0.5f + r/2.f)*50.f - 50;
			

		}
	}


	// generate initial sand rgb map
	for (int y = 0; y < map.h; y++)
	{
		for (int x = 0; x < map.w; x++)
		{
			map.argbSed[x + y * map.w].r = pallete.sand.r + (-4 + rand() % 8);
			map.argbSed[x + y * map.w].g = pallete.sand.g;
			map.argbSed[x + y * map.w].b = pallete.sand.b + (-4 + rand() % 8);
			// if(x > 100 && x < 150 && y > 100 && y < 150){
			// 	map.argbSed[x + y * map.w].r = 50;
			// }
		}
	}

	for(int y=0;y<map.h;y++){
        for(int x=0;x<map.w;x++){
            map.height[x + y * map.w] = map.stone[x + y * map.w] + map.sand[x + y * map.w];
        }
    }


}

static int mainLoop()
{

	clearLayer(botLayer);
	clearLayer(topLayer);

	char titleString[100];
	sprintf(titleString, "fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime);
	if(window.time.tick.ms100) window_setTitle(titleString);

	updateInput();

	if (fabsf(g_cam.rot) < 45.f * M_PI / 180.f || fabsf(g_cam.rot) >= 315.f * M_PI / 180.f)
	{
		g_cam.direction = NW;
	}
	else if (fabsf(g_cam.rot) < 135.f * M_PI / 180.f)
	{
		g_cam.direction = NE;
	}
	else if (fabsf(g_cam.rot) < 225.f * M_PI / 180.f)
	{
		g_cam.direction = SE;
	}
	else if (fabsf(g_cam.rot) < 315.f * M_PI / 180.f)
	{
		g_cam.direction = SW;
	}
	else
	{
		fprintf(stderr, "Unitialized value used at %s %d\n", __FILE__, __LINE__);
		exit(0);
	}


	PROFILE(process(window.time.dTime););



	if (window.time.tick.ms10 || map.flags.updateShadowMap == true)
	{
		PROFILE(generateShadowMap(););
	}

	if (window.time.tick.ms10 || map.flags.updateColorMap == true)
	{
		PROFILE(generateColorMap(););
	}

	PROFILE(render();)

	drawText(topLayer, 10, 10, printfLocal("fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime));

	


#ifdef DEBUG
    int cwx = clampf(cursor.worldX, 0, map.w);
    int cwy = clampf(cursor.worldY, 0, map.h);
	drawText(topLayer, 10, 30,  printfLocal("Mouse: %d, %d | %d, %d | zoom: %f", mouse.pos.x, mouse.pos.y, cwx, cwy, g_cam.zoom) );
	drawText(topLayer, 10, 50,  printfLocal("Shadow: %f", map.shadow[cwx+cwy*map.w]));
	drawText(topLayer, 10, 70,  printfLocal("foam: %f total: %f", map.foamLevel[cwx+cwy*map.w], totalFoamLevel));
	drawText(topLayer, 10, 90,  printfLocal("Stone: %f total: %f", map.stone[cwx+cwy*map.w], totalStoneLevel));
	drawText(topLayer, 10, 110,  printfLocal("Sand: %f total: %f", map.sand[cwx+cwy*map.w], totalSandLevel));
    drawText(topLayer, 10, 130, printfLocal("Water: %f total: %f", map.water[cwx+cwy*map.w].depth, totalWaterLevel));
    drawText(topLayer, 10, 150, printfLocal("Mist: %f total: %f", map.mist[cwx+cwy*map.w].depth, totalMistLevel));

    //Draw water is present data
	for (int y = 0; y < map.h; y++)
	{
		for (int x = 0; x < map.w; x++)
		{
            if(map.present[x+y*map.w].water){
                vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
                sPos.y = sPos.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)  / g_cam.zoom;
                // drawPoint(topLayer, sPos.x, sPos.y, pallete.red);
            }
        }
    }

	//Draw water velocity markers
	for (int y = 0; y < map.h; y++)
	{
		if((y % 10)) continue;
		for (int x = 0; x < map.w; x++)
		{
			if((x % 10)) continue;
			float x2 = x + map.waterVel[x + y * map.w].x;
			float y2 = y + map.waterVel[x + y * map.w].y;
			vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
			vec2f_t sPos2 = world2screen((float)x2+0.5f,(float)y2+0.5f,g_cam);
			// sPos.y = sPos.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)  / g_cam.zoom;
			// sPos2.y = sPos2.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)  / g_cam.zoom;
			// float vel = sqrtf((map.waterVel[x + y * map.w].x*map.waterVel[x + y * map.w].x)+(map.waterVel[x + y * map.w].y*map.waterVel[x + y * map.w].y));
			// drawLine(topLayer, sPos.x,sPos.y, sPos2.x, sPos2.y, pallete.white);
			// drawPoint(topLayer, sPos.x, sPos.y, pallete.red);
		}
	}

	//Draw mouse position
	for (int y = -1; y < 1; y++)
	{
		for (int x = -1; x < 1; x++)
		{
			if (mouse.pos.x > 1 && mouse.pos.x < window.drawSize.w - 1)
			{
				if (mouse.pos.y > 1 && mouse.pos.x < window.drawSize.h - 1)
				{
					topLayer.frameBuffer[(mouse.pos.x + x) + (mouse.pos.y + y) * topLayer.w].argb = 0xFFFFFFFF;
				}
			}
		}
	}

	

	drawSquare(topLayer, 200, 100, 100, 100, pallete.sand); //middle



	argb_t result1 = argbAdd(pallete.sand, ARGB(255,24,53,53));

	drawSquare(topLayer, 100, 100, 100, 100, pallete.green);
	drawSquare(topLayer, 300, 100, 100, 100, pallete.blue);
#endif

    

	return window_run();
}

// void* testThread(){
// 	while(1){
// 		render();

// 	}
// 	return NULL;
// }

// void* testThread2(){
// 	while(1){
// 		printf("kuk\n");

// 	}
// 	return NULL;
// }

atomic_int threadDone = 0;

int main()
{
	// Disable console buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	system("CHCP 65001"); //Enable unicode characters in the terminal


	window.drawSize.w = rendererSizeX;
	window.drawSize.h = rendererSizeY;
	window.size.w = windowSizeX;
	window.size.h = windowSizeY;


	window_init();

	// init bottom layer
	botLayer = window_createLayer();
	// init top layer
	topLayer = window_createLayer();

	init();
	// pthread_t render_pthread;
	// pthread_t render_pthread2;
	// /* create a second thread which executes inc_x(&x) */
	// if(pthread_create(&render_pthread, NULL, testThread, NULL)) printf("Error creating thread\n");
	// if(pthread_create(&render_pthread2, NULL, testThread2, NULL)) printf("Error creating thread\n");

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop((void (*)(void))mainLoop, 0, 1);
#else
	while (mainLoop())
	{
	}
#endif

#ifdef DEBUG
	#ifdef CSS_PROFILE_H_
		//Print profile result
		int nameWidth = 20; //Minimum 20
		for(int i=0;i<css_profile.numberOfProfiles;i++){
			int len = strlen(css_profile.profiles[i].id) + 1;
			nameWidth = MAX(nameWidth, len);
		}
		

		printf("\n Total runtime: %f seconds \n", (double)window.time.ms1 / 1000.0);

		printf("\n");
		printf("┏━┫Profiling results┣━"); for(int i=0;i<nameWidth - 20;i++){ printf("━");}                       printf("┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━┓\n");
		printf("┃ Name "); for(int i=0;i<nameWidth - 5;i++){ printf(" ");}                                       printf("┃ noCalls ┃   low   ┃   avg   ┃   high  ┃       total       ┃\n");
		printf("┣━"); for(int i=0;i<nameWidth;i++){ printf("━");}                                                printf("╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━┫\n");
		for(int i=0;i<css_profile.numberOfProfiles;i++){
			printf("┃ %s", css_profile.profiles[i].id); for(unsigned int j=0;j<nameWidth - strlen(css_profile.profiles[i].id);j++){ printf(" ");}
			printf("│ %7d ", css_profile.profiles[i].numberOfCalls);
			printf("│ %01.5f ", css_profile.profiles[i].lowTime);
			printf("│ %01.5f ", css_profile.profiles[i].avgTime);
			printf("│ %01.5f ", css_profile.profiles[i].highTime);
			printf("│ %07.3f ", css_profile.profiles[i].totalTime);
			double percentage = css_profile.profiles[i].totalTime / ((double)window.time.ms1 / 100000.0);
			printf("(%06.2f%%) ", percentage);
			printf("┃ \n");
			if(i == css_profile.numberOfProfiles - 1){ printf("┗━"); for(int i=0;i<nameWidth;i++){ printf("━");} printf("┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━┛\n"); }
			else{ printf("┠─"); for(int i=0;i<nameWidth;i++){ printf("─");}                                      printf("┼─────────┼─────────┼─────────┼─────────┼───────────────────┨\n"); }
			
		}
	#endif
#endif /*DEBUG*/

	return 0;
}



