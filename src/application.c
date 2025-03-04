#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>	//sin/cos /M_PI
#include <string.h> //memcpy

#include "stb_perlin.h"

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
#include "camera.h"



#ifndef APP_NAME //These should be defined in the makefile
	#define APP_NAME "0"
	#define APP_VER_MAJOR 0
	#define APP_VER_MINOR 0
	#define APP_VER_BUILD 0
#endif


// Link -lgdi32 -lSDL2_ttf -lSDL2 -lm -lSDL2_image

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*(M_PI/180.f))
#define RAD2DEG(x) ((x)*(180.f/M_PI))

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
    argb_t waterDark;
    argb_t water;
    argb_t waterLight;
    argb_t foam;
    argb_t mist;
    argb_t sand;
	argb_t lava;
	argb_t lavaBright;
}pallete = {
	.white        = rgb(255, 255, 255),
    .black        = rgb(0, 0, 0),
    .gray         = rgb(48, 48, 48),
	.red          = rgb(255, 0, 0),
	.orange       = rgb(255, 165, 0),
	.green        = rgb(0, 255, 0),
	.blue         = rgb(0, 0, 255),
	.yellow       = rgb(255, 255, 0),
    .stone        = rgb(61, 53, 51),
    .waterDark    = rgb(64, 96, 99),
    .water        = rgb(76, 138, 133),
    .waterLight   = rgb(147, 189, 168),
    .foam         = rgb(210, 210, 210),
    .mist         = rgb(235, 233, 236),
    .sand         = rgb(186, 165, 136),
    .lava         = rgb(254, 62, 10),
	.lavaBright   = rgb(254, 162, 3)
};
// https://coolors.co/406063-4c8a85-93bda8-baa588-ebe9ec-d2d2d2-3d3533-fe3e0a-fea203

#define MAPW 256
#define MAPH 256

#define windowSizeX 512	
#define windowSizeY 512
#define rendererSizeX 512
#define rendererSizeY 512





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


camera_t g_cam = {
    .limits.zoomMax = 0.5f,
    .limits.zoomMin = 0.03f
};


typedef struct{
	int w;
	int h;
	float tileWidth; // width of one tile, used for adjusting fluid simulation
	argb_t argbSed[MAPW * MAPH];
	argb_t argbStone[MAPW * MAPH];
	argb_t argb[MAPW * MAPH];
	argb_t argbBlured[MAPW * MAPH];
	argb_t argbBuffer[MAPW * MAPH];
	float shadowSoft[MAPW * MAPH];
	float shadow[MAPW * MAPH];
	float height[MAPW * MAPH];
	float stone[MAPW * MAPH];
	float sand[MAPW * MAPH];
	// fluidSWE_t waterSWE;
	new_fluid_t water;
	// fluid_t water[MAPW * MAPH];
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


Map map;

struct{
	float height[MAPW * MAPH];
	float mistDepth[MAPW * MAPH];
	argb_t argb[MAPW * MAPH];
	argb_t argbBlured[MAPW * MAPH];
}renderMapBuffer; //Stores stuff from map that will be used for rendering, frees up the map structure to be updated for next frame while current frame is rendering

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
	.detail = 0.f,
	.sand = 0.0f,
	.x = 0.f,
	.y = 0.f
};


#define FOAMSPAWNER_MAX (1000)
int foamSpawnerHead = 0;
struct{
	vec2f_t pos;
	float amount;
}foamSpawner[FOAMSPAWNER_MAX] = {
	{},{.pos.x = 100, .pos.y = 100, .amount = 1000.f}
};

void generateTerrain(float maxHeight, float detail, float sand, float xOffset, float yOffset, int mapW, int mapH, float* stoneMap, float* sandMap);


static argb_t getTileColorWater(int x, int y, int ys, vec2f_t upVec, float shade);

argb_t frameBuffer[rendererSizeX * rendererSizeY];
argb_t background[rendererSizeX * rendererSizeY]; // Stores background image that get copied to framebuffer at start of render

float totalFoamLevel;
float totalSandLevel;
float totalSusSedLevel;
float totalStoneLevel;
float totalWaterLevel;
float totalMistLevel;
float totalLavaLevel;
Layer botLayer;
Layer topLayer;




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
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);
	}
}
EMSCRIPTEN_KEEPALIVE
void changeSandHeight(float sand)
{
	terrainGen.sand = sand;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);
	}
}
EMSCRIPTEN_KEEPALIVE
void generateMap()
{
	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);
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
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);
	}
}
EMSCRIPTEN_KEEPALIVE
void setmapGenY(int y)
{
	terrainGen.y = y;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);
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
						map.water.depth[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.water.depth[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
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
						if(map.present[(cursor.worldX + k) + (cursor.worldY + j) * map.w].lava){
							map.lavaFoamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.lavaFoamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
						}else{
							map.foamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] = maxf(map.foamLevel[(cursor.worldX + k) + (cursor.worldY + j) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
						}
					}
					break;
                case TOOL_WIND:
					if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
						// map.water.up[(cursor.worldX + k) + (cursor.worldY + j) * map.w] += 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						// map.water.down[(cursor.worldX + k) + (cursor.worldY + j) * map.w] -= 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
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
		printf("camera: x:%f y:%f rot:%f zoom:%f\n", g_cam.x, g_cam.y, g_cam.rot, g_cam.zoom);
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
		erode();
	}
	if (key.L == eKEY_PRESSED){
		NEWFEATURE = (NEWFEATURE) ? 0 : 1;
		printf("NEWFATURE %s\n", NEWFEATURE ? "ON" : "OFF");
	}


	//TODO: Move into some update camera function that only runs when the camera rotation changes
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
				
				argb = lerpargb(pallete.lava, pallete.lavaBright, minf((map.lava[x+ mapPitch].depth) / 10.f, 1.f));
				// argb = lerpargb(argb, pallete.white, minf((map.lava[x+ mapPitch].depth) / 50.f, 1.f));
				
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
				// if(glare != glare) printf("heh\n");
				glare = minf(glare*0.05f, 1.f);

				argb = lerpargb(argb, pallete.white, glare);


				//Add foam
				if (map.lavaFoamLevel[x + mapPitch] > 0)
				{
					argb = lerpargb(argb, pallete.stone, minf(map.lavaFoamLevel[x + mapPitch] / 10.f, 1.f));
				}


				argb = lerpargb(argb, map.argbStone[x + mapPitch], minf(1.f/map.lava[x+ mapPitch].depth, 1.f));

			}

			//Add water if present
			if(map.present[x + mapPitch].water){
				float slopX = (map.height[(x + 1) + (y) * map.w] - map.height[(x - 1) + (y) * map.w]); 
				float slopY = (map.height[(x) + (y + 1) * map.w] - map.height[(x) + (y - 1) * map.w]);

				argb = lerpargb(argb, pallete.water, minf(map.water.depth[x + mapPitch]*0.1f, 1.f));
				argb = lerpargb(argb, pallete.waterDark, minf(map.water.depth[x + mapPitch]*0.05f, 1.f));


				if (map.susSed[x + y * map.w] > 0)
				{
					argb.r = lerp(argb.r, pallete.sand.r, minf(map.susSed[x + mapPitch], 0.75f));
					argb.g = lerp(argb.g, pallete.sand.g, minf(map.susSed[x + mapPitch], 0.75f));
					argb.b = lerp(argb.b, pallete.sand.b, minf(map.susSed[x + mapPitch], 0.75f));
				}
				
				// Highlight according to slope
				vec2f_t slopeVec = {.x = slopX + 0.000000001f, .y = slopY + 0.000000001f}; //The small addition is to prevent normalizing a zero length vector which we don't handle
				vec2f_t slopeVecNorm = normalizeVec2f(slopeVec);
				
				float glare = ((slopeVecNorm.x - upVec.x)*(slopeVecNorm.x - upVec.x)+(slopeVecNorm.y - upVec.y)*(slopeVecNorm.y - upVec.y)) * ((x-3) && (y-3) && (x-map.w+3) && (y-map.h+3)); //The thing at the end with makes it so if the x or y position is on the border then slopX and Y gets multiplied by 0 otherwise by 1
				// if(glare != glare) printf("heh\n");
				glare = minf(glare*0.05f, 1.f);

				argb = lerpargb(argb, pallete.white, glare);

				// I want to give a look where the water normal vector is facing the camera is lighter, unfortunately this shallow water on slopes to be light as well so I removed it until I have a better solution.
				// argb = lerpargb(argb, pallete.waterLight, clampf( ((slopeVec.x)*(slopeVec.x)+(slopeVec.y)*(slopeVec.y))  * (0.05f-glare) , 0.f, 1.f));
				
				
				//Foam at high velocity
				// float vel = (map.waterVel[x + mapPitch].x*map.waterVel[x + mapPitch].x) + (map.waterVel[x + mapPitch].y*map.waterVel[x + mapPitch].y);
				// argb = lerpargb(argb, pallete.foam, minf(vel * 0.001f, 1.f));
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


	//Draw cursor
	float radius = cursor.radius;
	for (int j = -radius; j <= radius; j++){
		for (int k = -radius; k <= radius; k++){
			float r = sqrtf(k * k + j * j);
				if (r > radius / 4){ continue; } //Skip if outside of cursor cirlce radius

				int x = cursor.worldX + k;
				int y = cursor.worldY + j;
				
				if (cursor.worldX + k > 2 && cursor.worldY + j > 2 && cursor.worldX + k < map.w - 2 && cursor.worldY + j < map.h - 2){
					switch (cursor.tool)
					{
					case TOOL_WATER:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.blue, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_SAND:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.yellow, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_STONE:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.white, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_FOAM:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.green, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_MIST:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.mist, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_LAVA:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.orange, 0.1f + 0.02f * cursor.amount);
						break;
					case TOOL_WIND:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.red, 0.1f + 0.02f * cursor.amount);
						break;
					default:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.black, 0.1f + 0.02f * cursor.amount);
						break;
					}
				}
		}
	}




	//CPU based blur
	// if(window.time.tick.ms100)
	// {
	// 	memcpy(map.argbBuffer, map.argb, sizeof(map.argb));
	// 	gaussBlurargb(map.argbBuffer, map.argbBlured, map.w*map.h, map.w, map.h, 10);
	// }
	
	//GPU blur
	if(window.time.tick.ms100)
	{
		static Shader blurShader = {
			.state = eSHADERSTATE_UNINITIALIZED,
			.type = eSHADERTYPE_ARGB,
			.vertexShaderSource = "src/shaders/shader.vert",
			.fragmentShaderSource = "src/shaders/blurShader.frag",
			.width = MAPW,
			.height = MAPH,
			.input.no = 1,
			.input.data[0].name = "inputDataTexture1",
			.input.data[0].ptr_argb = map.argb,
			.uniform.no = 3,
			.uniform.data[0].name = "resolution",
			.uniform.data[0].type = eUNIFROMTYPE_FLOAT_VEC2,
			.uniform.data[0].val_vec2f = {MAPW , MAPH},
			.uniform.data[1].name = "blurDirection",
			.uniform.data[1].type = eUNIFROMTYPE_INT,
			.uniform.data[1].val_i = 0,
			.uniform.data[2].name = "radius",
			.uniform.data[2].type = eUNIFROMTYPE_INT,
			.uniform.data[2].val_i = 20,
			.output.no = 1,
			.output.data[0].ptr_argb = map.argbBlured
		};

		//Run blur shader for two passes, use the output as the first as the input for the second, then reset the pointer for next loop
		blurShader.uniform.data[1].val_i = 0; // Horizontal pass
		runShader(&blurShader);
		void* tempPtr = blurShader.input.data[0].ptr_argb;
		blurShader.input.data[0].ptr_argb = blurShader.output.data[0].ptr_argb;
		blurShader.uniform.data[1].val_i = 1; //Vertical pass
		runShader(&blurShader);
		blurShader.input.data[0].ptr_argb = tempPtr;

	}
	
	
	
}


static void simulation(float dTime)
{
    int w = map.w;
    int h = map.h;


	for(int i = 0; i < FOAMSPAWNER_MAX; i++){
		if(foamSpawner[i].amount > 0.f){
			float x = foamSpawner[i].pos.x;
			float y = foamSpawner[i].pos.y;

			foamSpawner[i].pos.x += map.waterVel[(int)x + (int)y * map.w].x * dTime;
			foamSpawner[i].pos.y += map.waterVel[(int)x + (int)y * map.w].y * dTime;

			float spawnAmount = minf(foamSpawner[i].amount, 100.f * dTime);
			foamSpawner[i].amount -= spawnAmount;
			map.foamLevel[(int)x + (int)y * map.w] += spawnAmount;

		}
	}

    //foam
    //spawn foam where water is turbulent
    for(int y=1;y<h-1;y++){
        for(int x=1;x<w-1;x++){

//			float velX = map.waterVel[x+y*map.w].x;
//			float velY = map.waterVel[x+y*map.w].y;
            //curl is something, velDiff is speed difference with nearby tiles
			float curl = map.waterVel[(x+1)+y*map.w].y - map.waterVel[(x-1)+y*map.w].y - map.waterVel[x+(y+1)*map.w].x + map.waterVel[x+(y-1)*map.w].y;
            float velDiff = map.waterVel[x+y*w].x*2 - map.waterVel[(x-1)+(y)*w].x - map.waterVel[(x+1)+(y)*w].x + map.waterVel[x+y*w].y*2 - map.waterVel[x+(y-1)*w].y - map.waterVel[x+(y+1)*w].y;
            float deltaV = (map.water.flow[(x-1)+(y)*MAPW].right + map.water.flow[(x)+(y+1)*MAPW].down + map.water.flow[(x+1)+(y)*MAPW].left + map.water.flow[(x)+(y-1)*MAPW].up - (map.water.flow[(x)+(y)*MAPW].right + map.water.flow[(x)+(y)*MAPW].down + map.water.flow[(x)+(y)*MAPW].left + map.water.flow[(x)+(y)*MAPW].up));

            if(deltaV > 10.f){
                map.foamLevel[x+y*h] = map.foamLevel[x+y*h] + velDiff * 0.001f * dTime;
				// foamSpawnerHead++;
				// if(foamSpawnerHead > FOAMSPAWNER_MAX) foamSpawnerHead = 0;
				// foamSpawner[foamSpawnerHead].pos.x = x;
				// foamSpawner[foamSpawnerHead].pos.y = y;
				// foamSpawner[foamSpawnerHead].amount = velDiff;
            }

			// map.foamLevel[x+y*map.w] += 0.1f*(map.waterVel[x+y*map.w].x*map.waterVel[x+y*map.w].x+map.waterVel[x+y*map.w].y*map.waterVel[x+y*map.w].y);

            map.foamLevel[x+y*w]  = minf(map.foamLevel[x+y*w], 1000.f);
        	map.foamLevel[x+y*w] -= minf(map.foamLevel[x+y*w], 1.f * dTime);

//- map.lava[x+y*w].depth/10.f
			if(map.lava[x+y*w].depth > 0.f){
				float lavaConverted = minf(map.lava[x+y*w].depth, clampf(1.f / map.lava[x+y*w].depth, 0.25f, 0.5f)  * dTime);
				map.lava[x+y*w].depth -= lavaConverted; 
				map.stone[x+y*w] += lavaConverted * 0.5f;
				map.stone[x+y*w] += map.sand[x+y*w] * 0.75f;
				map.sand[x+y*w] = 0.f;

				// float r = sinf(window.time.ms10 / 100.f);
				// map.argbStone[x + y * map.w].r = pallete.stone.r + r*5.f;
				// map.argbStone[x + y * map.w].g = pallete.stone.g + r*5.f;
				// map.argbStone[x + y * map.w].b = pallete.stone.b + r*5.f;
				
				// if((map.lavaVel[x+y*w].x*map.lavaVel[x+y*w].x)+(map.lavaVel[x+y*w].y*map.lavaVel[x+y*w].y) < 1.f){
					if(rand() % 10000 < 1){
						map.lavaFoamLevel[x+y*w] +=  500.f * dTime;
					}

				// }
				map.lavaFoamLevel[x+y*w]  = minf(map.lavaFoamLevel[x+y*w], 1000.f);
				map.lavaFoamLevel[x+y*w] -= minf(map.lavaFoamLevel[x+y*w], 0.01f * dTime);

				if(map.water.depth[x+y*w] > 0.f){
					lavaConverted = minf(minf(map.lava[x+y*w].depth, map.water.depth[x+y*w]), 2.f * dTime);
					map.lava[x+y*w].depth  -= lavaConverted; 
					map.stone[x+y*w]       += lavaConverted;
					map.water.depth[x+y*w] -= lavaConverted * 1.f; 
					map.water.flow[x+y*w].down  -= lavaConverted * 1.f; 
					map.water.flow[x+y*w].up    -= lavaConverted * 1.f; 
					map.water.flow[x+y*w].left  -= lavaConverted * 1.f; 
					map.water.flow[x+y*w].right -= lavaConverted * 1.f; 
					map.mist[x+y*w].depth  += lavaConverted * 4.f; 
					// map.mist[x+y*w].down   += lavaConverted * 5.f; 
					// map.mist[x+y*w].up     += lavaConverted * 5.f; 
					// map.mist[x+y*w].left   += lavaConverted * 5.f; 
					// map.mist[x+y*w].right  += lavaConverted * 5.f; 

				}
			}else{
				map.lavaFoamLevel[x+y*w] = 0;
			}

			map.mist[x+y*w].depth -= minf(map.mist[x+y*w].depth, 0.25f * dTime);

        }
    }


    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] = map.stone[x + y * w] + map.sand[x + y * w];
        }
    }


    PROFILE(simFluid(map.lava, map.height, 9.81f, 20.f, map.tileWidth, w, h, 1.f, minf(dTime*program.simSpeed, 0.13f));)


	//Handle border conditions of fluids
	for (int y = 0; y < h; y++)
	{
		map.lava[2 + y * w].depth += map.lava[1 + y * w].depth;
		map.lava[1 + y * w].depth = 0.f;
		map.lava[1 + y * w].right += map.lava[2 + y * w].left;
		map.lava[2 + y * w].left = 0.f;
		map.lava[(w - 2) + y * w].depth += map.lava[(w - 1) + y * w].depth;
		map.lava[(w - 1) + y * w].depth = 0.f;
		map.lava[(w - 1) + y * w].left += map.lava[(w - 2) + y * w].right;
		map.lava[(w - 2) + y * w].right = 0.f;
		
	}
	for (int x = 0; x < w; x++)
	{
		map.lava[x + 2 * w].depth += map.lava[x + 1 * w].depth;
		map.lava[x + 1 * w].depth = 0.f;
		map.lava[x + 1 * w].up += map.lava[x + 2 * w].down;
		map.lava[x + 2 * w].down = 0;
		map.lava[x + (h - 2) * w].depth += map.lava[x + (h - 1) * w].depth;
		map.lava[x + (h - 1) * w].depth = 0.f;
		map.lava[x + (h - 1) * w].down += map.lava[x + (h - 2) * w].up;
		map.lava[x + (h - 2) * w].up = 0.f;
	}


	//Add lava to height
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] += map.lava[x + y * w].depth;
        }
    }


	static uint32_t delay = 100;
	delay += mouse.dWheel;
	if(key.V){
		static float var = 1;
		for(uint32_t i=0;i<delay;i++){
			var = powf(var, var);
			// printf("%d %f\n",delay,var);
		}
	}

	for(int i=0;i<1;i++){
		// simFluidSWE(&(map.waterSWE), map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.90f, 0.01f /*minf(dTime*program.simSpeed, 0.13f)*/);

	}
	if(key.Y){
		simFluidGPU(&(map.water), map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.98f, 0.02f /*minf(dTime*program.simSpeed, 0.13f)*/);
	}else{
    	simFluidBackup(&(map.water), map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.98f, 0.02f /*minf(dTime*program.simSpeed, 0.13f)*/);
	}

	// memcpy(map.water.depth, map.waterSWE.depth, sizeof(float)*map.w*map.h);

	//Handle border conditions of fluids
	for (int y = 0; y < h; y++)
	{
		map.water.depth[3 + y * w] += map.water.depth[2 + y * w];
		map.water.depth[2 + y * w] = 0.f;
		map.water.flow[2 + y * w].right += map.water.flow[3 + y * w].left;
		map.water.flow[3 + y * w].left = 0.f;
		map.water.depth[(w - 3) + y * w] += map.water.depth[(w - 2) + y * w];
		map.water.depth[(w - 2) + y * w] = 0.f;
		map.water.flow[(w - 2) + y * w].left += map.water.flow[(w - 3) + y * w].right;
		map.water.flow[(w - 3) + y * w].right = 0.f;
		
	}
	for (int x = 0; x < w; x++)
	{
		map.water.depth[x + 3 * w] += map.water.depth[x + 2 * w];
		map.water.depth[x + 2 * w] = 0.f;
		map.water.flow[x + 2 * w].up += map.water.flow[x + 3 * w].down;
		map.water.flow[x + 3 * w].down = 0;
		map.water.depth[x + (h - 3) * w] += map.water.depth[x + (h - 2) * w];
		map.water.depth[x + (h - 2) * w] = 0.f;
		map.water.flow[x + (h - 2) * w].down += map.water.flow[x + (h - 3) * w].up;
		map.water.flow[x + (h - 3) * w].up = 0.f;
	}


    //Add water to height
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            map.height[x + y * w] += map.water.depth[x + y * w];
        }
    }


    PROFILE(simFluid(map.mist, map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.90f, minf(dTime*program.simSpeed, 0.13f));)


	//Handle border conditions of fluids
	for (int y = 0; y < h; y++)
	{
		map.mist[4 + y * w].depth += map.mist[3 + y * w].depth;
		map.mist[3 + y * w].depth = 0.f;
		map.mist[3 + y * w].right += map.mist[4 + y * w].left;
		map.mist[4 + y * w].left = 0.f;
		map.mist[(w - 4) + y * w].depth += map.mist[(w - 3) + y * w].depth;
		map.mist[(w - 3) + y * w].depth = 0.f;
		map.mist[(w - 3) + y * w].left += map.mist[(w - 4) + y * w].right;
		map.mist[(w - 4) + y * w].right = 0.f;
		
	}
	for (int x = 0; x < w; x++)
	{
		map.mist[x + 4 * w].depth += map.mist[x + 3 * w].depth;
		map.mist[x + 3 * w].depth = 0.f;
		map.mist[x + 3 * w].up += map.mist[x + 4 * w].down;
		map.mist[x + 4 * w].down = 0;
		map.mist[x + (h - 4) * w].depth += map.mist[x + (h - 3) * w].depth;
		map.mist[x + (h - 3) * w].depth = 0.f;
		map.mist[x + (h - 3) * w].down += map.mist[x + (h - 4) * w].up;
		map.mist[x + (h - 4) * w].up = 0.f;
	}

    for(int y=4;y<h-5;y++){
        for(int x=4;x<w-5;x++){
            // calculate velocity
            // map.waterVel[x + y * w].x = (map.water[(x - 1) + (y)*w].right - map.water[(x) + (y)*w].left + map.water[(x) + (y)*w].right - map.water[(x + 1) + (y)*w].left); // X
            // map.waterVel[x + y * w].y = (map.water[(x) + (y - 1)*w].down - map.water[(x) + (y)*w].up + map.water[(x) + (y)*w].down - map.water[(x) + (y + 1)*w].up);       // Y

			map.lavaVel[x + y * w].x = (map.lava[(x - 1) + (y)*w].right - map.lava[(x) + (y)*w].left + map.lava[(x) + (y)*w].right - map.lava[(x + 1) + (y)*w].left) / (32.f); // X
            map.lavaVel[x + y * w].y = (map.lava[(x) + (y - 1)*w].down - map.lava[(x) + (y)*w].up + map.lava[(x) + (y)*w].down - map.lava[(x) + (y + 1)*w].up) / (32.f);       // Y

            map.present[x + y * w].water = (map.water.depth[x + y * w] > 0.01f) ? 1 : 0;
            map.present[x + y * w].mist  = (map.mist[x + y * w].depth > 0.01f) ? 1 : 0;
            map.present[x + y * w].lava  = (map.lava[x + y * w].depth > 0.01f) ? 1 : 0;
        }
    }


    
    // PROFILE(erodeAndDeposit(map.sand, map.susSed, map.stone, map.water, map.waterVel, w, h);)
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
        totalWaterLevel += map.water.depth[i];
    }
#endif /*DEBUG*/

}

argb_t getTileColorMist(int x, int y, int ys, vec2f_t upVec){

//Ide, använd screen2world för att få rutan som motsvarar botten istället.
    argb_t argb = pallete.red;

    float mistHeight =  renderMapBuffer.height[x+y*map.w] + renderMapBuffer.mistDepth[x+y*map.w];

    float d;
    for(d = 0.f; d < 300.f; d += 1.f){
		int X = x+upVec.x*d;
		int Y = y+upVec.y*d;
		if(X >= 0 && X < map.w && Y >= 0 && Y < map.h){
			if(renderMapBuffer.height[(int)(x+upVec.x*d)+(int)(y+upVec.y*d)*map.w] > mistHeight - (d * 0.79f)){ //0.7071f = 1/sqrt(2)

				argb = renderMapBuffer.argb[X+Y*map.w];

				argb = lerpargb(argb, renderMapBuffer.argbBlured[X+Y*map.w], minf(d/10.f, 1.f));

				break;
			}
		}else{
			
			//If the mist is up against the wall, sample the background picture to make it appear transparent
			//I don't know why the coordinates are like this, I just tried stuff until it worked....
			argb.r = background[rendererSizeY - ys].r; //(102+(int)ys)>>2;//67
			argb.g = background[rendererSizeY - ys].g; //(192+(int)ys)>>2;//157
			argb.b = background[rendererSizeY - ys].b; //(229+(int)ys)>>2;//197

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
	
	float camZoom = camera.zoom;
	float camZoomDiv = 1.f / camera.zoom;
	float camZoomDivBySqrt2 = (camZoomDiv) /  sqrtf(2.f);

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



		ys = y - (renderMapBuffer.height[xwti + ywti * map.w] + renderMapBuffer.mistDepth[xwti + ywti * map.w]) * camZoomDivBySqrt2;  // offset y by terrain height (sqr(2) is to adjust for isometric projection)

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
				argb = renderMapBuffer.argb[xwti + ywti * map.w];
			}
			

			// make borders of tiles darker, make it so they become darker the more zoomed in you are
			if (camZoom < 0.3f && !border)
			{
				argb = lerpargb(argb, pallete.black, camZoomDiv/255.f);
			}

			// only draw visible pixels
			for (register int Y = ybuffer - 1; Y >= ys; Y--)
			{
				frameBuffer[window.drawSize.h - 1 - Y + (x)*window.drawSize.h] = argb; // draw pixels
			}
			

			ybuffer = ys; // save current highest point in pixel column
		}

		// this piece of code calculates how many y pixels (dPixels) there is to the next tile
		// and the border thing makes it so it only jumps one pixel when there is a
		// new tile and the next drawing part is darker, thus making the edge of the tile darker
		if (!border)
		{
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

	clearLayer(botLayer);
	clearLayer(topLayer);

	// drawText(topLayer, 10, 10, printfLocal("fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime));

	


#ifdef DEBUG
    int cwx = clampf(cursor.worldX, 0, map.w);
    int cwy = clampf(cursor.worldY, 0, map.h);
	// drawText(topLayer, 10, 30,  printfLocal("Mouse: %d, %d | %d, %d | zoom: %f", mouse.pos.x, mouse.pos.y, cwx, cwy, g_cam.zoom) );
	// drawText(topLayer, 10, 50,  printfLocal("Shadow: %f", map.shadow[cwx+cwy*map.w]));
	// drawText(topLayer, 10, 70,  printfLocal("foam: %f total: %f", map.foamLevel[cwx+cwy*map.w], totalFoamLevel));
	// drawText(topLayer, 10, 90,  printfLocal("Stone: %f total: %f", map.stone[cwx+cwy*map.w], totalStoneLevel));
	// drawText(topLayer, 10, 110,  printfLocal("Sand: %f total: %f", map.sand[cwx+cwy*map.w], totalSandLevel));
    // drawText(topLayer, 10, 130, printfLocal("Water: %f total: %f", map.water[cwx+cwy*map.w].depth, totalWaterLevel));
    // drawText(topLayer, 10, 150, printfLocal("Mist: %f total: %f", map.mist[cwx+cwy*map.w].depth, totalMistLevel));
	// drawText(topLayer, 10, 170,  printfLocal("waterVel: %f, %f", map.waterVel[cwx+cwy*map.w].x, map.waterVel[cwx+cwy*map.w].y));

    //Draw water is present data
	// for (int y = 0; y < map.h; y++)
	// {
	// 	for (int x = 0; x < map.w; x++)
	// 	{
    //         if(map.present[x+y*map.w].water){
    //             vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
    //             sPos.y = sPos.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)   / (sqrtf(2.f) * g_cam.zoom);
    //             drawPoint(topLayer, sPos.x, sPos.y, pallete.red);
    //         }
    //     }
    // }

	//Draw water velocity markers
	// for (int y = 0; y < map.h; y++)
	// {
	// 	if((y % 10)) continue;
	// 	for (int x = 0; x < map.w; x++)
	// 	{
	// 		if((x % 10)) continue;
	// 		float x2 = x + map.waterVel[x + y * map.w].x;
	// 		float y2 = y + map.waterVel[x + y * map.w].y;
	// 		vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
	// 		vec2f_t sPos2 = world2screen((float)x2+0.5f,(float)y2+0.5f,g_cam);
	// 		sPos.y = sPos.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)   / (sqrtf(2.f) * g_cam.zoom);
	// 		sPos2.y = sPos2.y - (map.height[x+y*map.w] + map.water[x+y*map.w].depth)   / (sqrtf(2.f) * g_cam.zoom);
	// 		float vel = sqrtf((map.waterVel[x + y * map.w].x*map.waterVel[x + y * map.w].x)+(map.waterVel[x + y * map.w].y*map.waterVel[x + y * map.w].y));
	// 		drawLine(topLayer, sPos.x,sPos.y, sPos2.x, sPos2.y, pallete.white);
	// 		drawPoint(topLayer, sPos.x, sPos.y, pallete.red);
	// 	}
	// }

	//Draw mouse position
	// for (int y = -1; y < 1; y++)
	// {
	// 	for (int x = -1; x < 1; x++)
	// 	{
	// 		if (mouse.pos.x > 1 && mouse.pos.x < window.drawSize.w - 1)
	// 		{
	// 			if (mouse.pos.y > 1 && mouse.pos.x < window.drawSize.h - 1)
	// 			{
	// 				topLayer.frameBuffer[(mouse.pos.x + x) + (mouse.pos.y + y) * topLayer.w].argb = 0xFFFFFFFF;
	// 			}
	// 		}
	// 	}
	// }

	//Draw foamspawner
	for(int i = 0; i < FOAMSPAWNER_MAX; i++){
		if(foamSpawner[i].amount > 0.f){
			int x = foamSpawner[i].pos.x;
			int y = foamSpawner[i].pos.y;
			vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
			sPos.y = sPos.y - (map.height[x+y*map.w] + map.water.depth[x+y*map.w])  / (sqrtf(2.f) * g_cam.zoom);
			drawSquare(topLayer, sPos.x - 1, sPos.y - 1, 2, 2, pallete.red);
		}
	}
	

#endif


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

		renderColumn(x, botMostYCoord, topMostYCoord, upVec, worldCoord.x, worldCoord.y, dDxw, dDyw, cam);
	}

	// for memory access pattern reasons the terrain gets drawn sideways. That's why we below transpose the framebuffer while copying it to the framebuffer
	for (int y = 0; y < window.drawSize.h; y++)
	{
		int pixelPitch = y * window.drawSize.w;
		int bufferPitch = window.drawSize.h - 1 - y;
		for (int x = 0; x < window.drawSize.w; x++)
		{
			botLayer.frameBuffer[x + pixelPitch] = frameBuffer[bufferPitch + (x)*window.drawSize.w];
		}
	}

	// Draw mouse position
	for (int y = -1; y < 1; y++)
	{
		for (int x = -1; x < 1; x++)
		{
			if (mouse.pos.x > 1 && mouse.pos.x < window.drawSize.w - 1)
			{
				if (mouse.pos.y > 1 && mouse.pos.x < window.drawSize.h - 1)
				{
					topLayer.frameBuffer[(mouse.pos.x + x) + (mouse.pos.y + y) * topLayer.w] = rgb(255,0,0);
				}
			}
		}
	}

    drawText(topLayer, 100, 130, printfLocal("Total amount of water: %f", totalWaterLevel));
	char* simStr = ((key.Y) ? "GPU" : "Processor");
    drawText(topLayer, 100, 160, printfLocal("Sim running on: %s", simStr));

}



static void init()
{

	map.w = MAPW;
	map.h = MAPH;
	map.tileWidth = 1.f;

	// map.waterSWE.depth = calloc(map.w * map.h, sizeof(float));
	// map.waterSWE.flow = calloc(map.w * map.h, sizeof(vec2f_t));

	map.water.depth = calloc(map.w * map.h, sizeof(float));
	map.water.flow = calloc(map.w * map.h, sizeof(Fluid_flow));
	map.water.vel = calloc(map.w * map.h, sizeof(vec2f_t));

	// init camera position, the following will init camera to center overview a 256x256 map
	// camera: x:336.321991 y:-93.287224 rot:1.570000 zoom:0.609125camera: x:327.101379 y:-84.052345 rot:1.570000 zoom:0.609125
	
	//The equation for starting camera position was found by sampling three correct starting positions at three different resolutions
	// res	 x	        y	        z
	// 256	 517,614   -273,724	    1,4669
	// 512	 302,6	    -54,5	    0,726
	// 1024	-129,934	395,956	    0,369
	// 2048	-977,719	1287,959	0,184

	// g_cam.x = 302.6;
	g_cam.x = rendererSizeX * -0.8343 + 729.11;
	// g_cam.y = -54.5;
	g_cam.y = rendererSizeY * 0.8724 - 498.59;
	g_cam.rot = 3.14f / 2;
	// g_cam.zoom = 0.726;
	g_cam.zoom = 366.03 * pow(rendererSizeX, -0.996);

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


	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, map.stone, map.sand);

	map.flags.updateShadowMap = 1; // make sure shadows are updated after map load
	map.flags.updateColorMap = 1;  // make sure shadows are updated after map load

	// Create background for render
	for (int x = 0; x < window.drawSize.w; x++)
	{
		// argb_t argb = (argb_t){.b = ((219 + x) >> 2), .g = (((182 + x) >> 2)), .r = (((92 + x) >> 2))};
		argb_t argb = lerpargb(ARGB(255,25,47,56),ARGB(255,222,245,254),((float)x)/((float)window.drawSize.w)); //Gradient over rendersize
		for (int y = 0; y < window.drawSize.h; y++)
		{

			background[window.drawSize.w - 1 - x + y * window.drawSize.w] = argb;
			// if((x & 0x80) ^ (y & 0x80)){
			// 	background[window.drawSize.w - 1 - x + y * window.drawSize.w] = pallete.water;
			// }else{
			// 	background[window.drawSize.w - 1 - x + y * window.drawSize.w] = pallete.sand;
			// }
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

	for(int y=0;y<map.h;y++){
        for(int x=0;x<map.w;x++){
            // map.water[x + y * map.w].depth = 10.f;
        }
    }


}



void process(){
	


	PROFILE(simulation(window.time.dTime);)

	if (window.time.tick.ms10 || map.flags.updateShadowMap == true)
	{
		PROFILE(generateShadowMap();)
	}

	if (window.time.tick.ms10 || map.flags.updateColorMap == true)
	{
		PROFILE(generateColorMap();)
	}


}


static int mainLoop()
{


	char titleString[100];
	sprintf(titleString, "fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime);
	if(window.time.tick.ms100) window_setTitle(titleString);

	

	updateInput();



	process();

	//Update buffers that render will use
	for(int i = 0; i < MAPW * MAPH; i++)
	{
		renderMapBuffer.height[i] = map.height[i];
		renderMapBuffer.mistDepth[i] = map.mist[i].depth;
		renderMapBuffer.argb[i] = map.argb[i];
		renderMapBuffer.argbBlured[i] = map.argbBlured[i];
	}

	PROFILE(render();)


	return window_run();
}



int main()
{
	// Disable console buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	system("CHCP 65001"); //Enable unicode characters in the terminal


	printf("%s %d.%d.%d - %s %s\n", APP_NAME, APP_VER_MAJOR, APP_VER_MINOR, APP_VER_BUILD, __DATE__, __TIME__);

	window.drawSize.w = rendererSizeX;
	window.drawSize.h = rendererSizeY;
	window.size.w = windowSizeX;
	window.size.h = windowSizeY;

	window.settings.vSync = false;
	


	window_init();

	// init bottom layer
	botLayer = window_createLayer();
	// init top layer
	// topLayer = window_createLayer();
	topLayer = botLayer;

	PROFILE(init();)


	//Create two seperate running threads, one will update the simulation and one will render the map
	// pthread_t render_pthread;
	// pthread_t process_pthread;
	
	// if(pthread_create(&render_pthread, NULL, renderThread, NULL)) printf("Error creating thread\n");
	// if(pthread_create(&process_pthread, NULL, processThread, NULL)) printf("Error creating thread\n");

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
		for(int i=0;i<=css_profile.numberOfProfiles;i++){
			printf("┃ %s", css_profile.profiles[i].id); for(unsigned int j=0;j<nameWidth - strlen(css_profile.profiles[i].id);j++){ printf(" ");}
			printf("│ %7d ", css_profile.profiles[i].numberOfCalls);
			printf("│ %01.5f ", css_profile.profiles[i].lowTime);
			printf("│ %01.5f ", css_profile.profiles[i].avgTime);
			printf("│ %01.5f ", css_profile.profiles[i].highTime);
			printf("│ %07.3f ", css_profile.profiles[i].totalTime);
			double percentage = css_profile.profiles[i].totalTime / ((double)window.time.ms1 / 100000.0);
			printf("(%06.2f%%) ", percentage);
			printf("┃ \n");
			if(i == css_profile.numberOfProfiles){ printf("┗━"); for(int i=0;i<nameWidth;i++){ printf("━");} printf("┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━┛\n"); }
			else{ printf("┠─"); for(int i=0;i<nameWidth;i++){ printf("─");}                                      printf("┼─────────┼─────────┼─────────┼─────────┼───────────────────┨\n"); }
			
		}
	#endif
#endif /*DEBUG*/



	return 0;
}



