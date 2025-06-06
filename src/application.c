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
#include "render.h"
#include "globals.h"



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



#define windowSizeX 800	
#define windowSizeY 800
#define rendererSizeX 800
#define rendererSizeY 800


void drawUI(Layer layer);
static void init();


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
		TOOL_ERODE = 8,
		TOOL_WETMAP = 9
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
    .limits.zoomMin = 0.03f,
    .limits.yMax = 1000.f,
    .limits.yMin = -1000.f,
	.limits.xMax = 1000.f,	
	.limits.xMin = -1000.f
};



Map map;


RenderMapBuffer renderMapBuffer;


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


#define FOAMSPAWNER_MAX (1000)
int foamSpawnerHead = 0;
struct{
	vec2f_t pos;
	float amount;
}foamSpawner[FOAMSPAWNER_MAX] = {
	// {},{.pos.x = 100, .pos.y = 100, .amount = 1000.f}
};

//Function prototypes from terrainGeneration.c
void generateTerrain(float maxHeight, float detail, float sand, float xOffset, float yOffset, int mapW, int mapH, Map* map);
void erode(int w, int h, float* stone, float* sand);
void oldErode(int w, int h, float* stone, float* sand);




float totalFoamLevel;
float totalSandLevel;
float totalSusSedLevel;
float totalStoneLevel;
float totalWaterLevel;
float totalMistLevel;
float totalLavaLevel;
Layer backgroundLayer;
Layer botLayer;
Layer midLayer;
Layer uiLayer;


extern argb_t* background; //Declared in render.c, needed here for transparent mist agains edge of map


//Functions

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
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);
	}
}
EMSCRIPTEN_KEEPALIVE
void changeSandHeight(float sand)
{
	terrainGen.sand = sand;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);
	}
}
EMSCRIPTEN_KEEPALIVE
void generateMap()
{
	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);
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
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);
	}
}
EMSCRIPTEN_KEEPALIVE
void setmapGenY(int y)
{
	terrainGen.y = y;
	if(terrainGen.autoGenerate)
	{
		generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);
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
		for (int y = cursor.worldY - radius; y <= cursor.worldY + radius; y++){
			for (int x = cursor.worldX - radius; x <= cursor.worldX + radius; x++){
				float r = sqrtf((x - cursor.worldX) * (x - cursor.worldX) + (y - cursor.worldY) * (y - cursor.worldY));
				if (r > radius){ continue; } //Skip if outside of cursor cirlce radius



				switch (cursor.tool){
				case TOOL_WATER:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2)
					{
						map.water[(x) + (y) * map.w].depth = maxf(map.water[(x) + (y) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[(x) + (y) * map.w].water = 1;
					}
					break;
				case TOOL_SAND:
					if (x > 1 && y > 1 && x < map.w - 1 && y < map.h - 1){
						map.sand[(x) + (y) * map.w] = maxf(map.sand[(x) + (y) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
					}
					break;
				case TOOL_STONE:
					if (x > 0 && y > 0 && x < map.w - 0 && y < map.h - 0){
						map.stone[(x) + (y) * map.w] = maxf(map.stone[(x) + (y) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
					}
					break;
				case TOOL_FOAM:
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2){
						if(map.present[(x) + (y) * map.w].lava){
							map.lavaFoamLevel[(x) + (y) * map.w] = maxf(map.lavaFoamLevel[(x) + (y) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
						}else{
							map.foamLevel[(x) + (y) * map.w] = maxf(map.foamLevel[(x) + (y) * map.w] + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
						}
					}
					break;
                case TOOL_WIND:
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2){
						// map.water.up[(x) + (y) * map.w] += 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						// map.water.down[(x) + (y) * map.w] -= 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						map.mist[(x) + (y) * map.w].up += 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
						map.mist[(x) + (y) * map.w].down -= 10.f * add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime;
					}
					break;
                case TOOL_MIST:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2){
						map.mist[(x) + (y) * map.w].depth = maxf(map.mist[(x) + (y) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[(x) + (y) * map.w].mist = 1;
					}
					break;
				case TOOL_LAVA:
					if (r > radius / 4){ continue; } //Fluids need smaller circle or we get a huge area of this fluid
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2){
						map.lava[(x) + (y) * map.w].depth = maxf(map.lava[(x) + (y) * map.w].depth + add * cursor.radius * cursor.radius * cursor.amount * expf(-(r * r) / (s)) / (M_PI * s) * window.time.dTime, 0.f);
                        map.present[(x) + (y) * map.w].lava = 1;
					}
					break;
				case TOOL_WETMAP:
					if (r > radius / 4){ continue; } 
					if (x > 2 && y > 2 && x < map.w - 2 && y < map.h - 2)
					{
						map.wetMap[(x) + (y) * map.w] += add;;
					}
					break;
				default:

					break;
				}
			}
		}
	}


	if (key.A == eKEY_HELD){
		cam_pan(&g_cam, -300.f * window.time.dTime, 0.f);
	}
	if (key.D == eKEY_HELD){
		cam_pan(&g_cam, 300.f * window.time.dTime, 0.f);
	}
	if (key.W == eKEY_HELD){
		cam_pan(&g_cam, 0.f, -450.f * window.time.dTime);
	}
	if (key.S == eKEY_HELD){
		cam_pan(&g_cam, 0.f, 450.f * window.time.dTime);
	}
	if (key.R == eKEY_HELD){
		cam_zoom(&g_cam, 1.f  * window.time.dTime);
	}
	if (key.F == eKEY_HELD){
        cam_zoom(&g_cam, -1.f  * window.time.dTime);
	}
	if (key.Q == eKEY_HELD){
		float angle = 64.f * M_PI / 180.f;
		cam_rot(&g_cam, -angle * window.time.dTime);
	}
	if (key.E == eKEY_HELD){
		float angle = 64.f * M_PI / 180.f;
		cam_rot(&g_cam, angle * window.time.dTime);
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
		cursor.tool = TOOL_WETMAP;
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

		

		// printf("sunAngleTest: %f\n", sunAngleTest);
	}

	if (key.M == eKEY_HELD){
		map.sunAngle += 0.45f * window.time.dTime;
		map.sunAngle = fmodf(map.sunAngle, 2 * M_PI);
		if (map.sunAngle < 0.f)
		{
			map.sunAngle += 2 * M_PI;
		}
	}

	if (key.N == eKEY_HELD){
		map.sunAngle -= 0.45f * window.time.dTime;
		map.sunAngle = fmodf(map.sunAngle, 2 * M_PI);
		if (map.sunAngle < 0.f)
		{
			map.sunAngle += 2 * M_PI;
		}
	}


	if (key.B == eKEY_PRESSED)
	{
		// Temporary map reset, will allocate more memory so it's not a good solution
		init();
	}

	if (key.I == eKEY_HELD){
		for(int i=0;i<1;i++){
			erode(map.w, map.h, map.stone, map.sand);
		}
	}

	if (key.L == eKEY_PRESSED){
		NEWFEATURE = (NEWFEATURE) ? 0 : 1;
		printf("NEWFATURE %s\n", NEWFEATURE ? "ON" : "OFF");
	}

	//Spawn a volcano
	for(int y = map.h/2-1; y < map.h/2+1; y++)
	{
		for(int x = map.w/2-10; x < map.w/2+10; x++)
		{
			// map.lava[(x) + (y) * map.w].depth += 1.0f;
		}
	}

	//Spawn a fountain
	for(int y = map.h - 25; y < map.h - 20; y++)
	{
		for(int x = 100; x < 105; x++)
		{
			map.water[(x) + (y) * map.w].depth += 0.5f;
		}
	}

#ifdef DEBUG
	if (key.ESC == eKEY_HELD){
		window.closeWindow = true;
	}
#endif

}


float shadow[MAPW * MAPH];
/**
 * @brief 
 */
static void generateShadowMap()
{


	map.flags.updateShadowMap = 0;



	// Copy heap stored variables to stack before calculation
	int mapW = map.w;
	int mapH = map.h;
	for (int i = 0; i < mapW * mapH; i++)
	{
		shadow[i] = 1.0f;
	}


	// Calculate the angle of sunlight
	float sunAngleX = cosf(map.sunAngle);
	float sunAngleY;
	if(map.sunAngle < M_PI / 2.f)
	{
		sunAngleY = tan(map.sunAngle);
	}
	else if(map.sunAngle < M_PI + M_PI / 2.f)
	{
		sunAngleY = 1.f / tan(map.sunAngle - M_PI / 2.f);
	}
	else
	{
		sunAngleY = tan(map.sunAngle);
	}

	// ambient occlusion
	// Calculate ambient occlusion using a box filter, optimized with technique found here: http://blog.ivank.net/fastest-gaussian-blur.html#results

	// float intensity = 0.05f;
	// int R = 10;
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
	// 		shadow[ti] = ((map.height[ti]) - (val * diameterDiv)) * intensity;
	// 		ti++;
	// 	}
	// 	for (int j = R + 1; j < mapW - R; j++)
	// 	{
	// 		val += map.height[ri++] - map.height[li++];
	// 		shadow[ti] = ((map.height[ti]) - (val * diameterDiv)) * intensity;
	// 		ti++;
	// 	}
	// 	for (int j = mapW - R; j < mapW; j++)
	// 	{
	// 		val += lv - map.height[li++];
	// 		shadow[ti] = ((map.height[ti]) - (val * diameterDiv)) * intensity;
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
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv)) * intensity;
	// 		ri += mapW;
	// 		ti += mapW;
	// 	}
	// 	for (int j = R + 1; j < mapH - R; j++)
	// 	{
	// 		val += map.height[ri] - map.height[li];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv)) * intensity;
	// 		li += mapW;
	// 		ri += mapW;
	// 		ti += mapW;
	// 	}
	// 	for (int j = mapH - R; j < mapH; j++)
	// 	{
	// 		val += lv - map.height[li];
	// 		shadow[ti] += ((map.height[ti]) - (val * diameterDiv)) * intensity;
	// 		li += mapW;
	// 		ti += mapW;
	// 	}
	// }

	//Clamp shadow value
	float sunFactor = 0.8f + clampf(sunAngleY / (2.f * M_PI), -0.4f, 0.2f);
	for (int i = 0; i < mapW * mapH; i++){
		shadow[i] = 0.8f * sunFactor + clampf(shadow[i], -0.5f * sunFactor, 0.2f * sunFactor);
	}
	// printf("sunAngleX: %f sunAngleY: %f, sunFactor: %f\n", sunAngleX, sunAngleY, sunFactor);
	// Normalize shadow values to 0.0f - 1.0f
	float shadowMax = 0.f;
	float shadowMin = 1.f;
	for (int i = 0; i < mapW * mapH; i++)
	{
		if(shadow[i] > shadowMax)
			shadowMax = shadow[i];
		if(shadow[i] < shadowMin)
			shadowMin = shadow[i];
	}
	// printf("shadowMax: %f shadowMin: %f\n", shadowMax, shadowMin);
	for (int i = 0; i < mapW * mapH; i++){
		// shadow[i] = (shadow[i] - shadowMin) / (shadowMax - shadowMin);
	}


#define SHADOWS
#ifdef SHADOWS


	//		Calculate shadows by iterating over map diagonally like example below.
	//		------- Save the highest tileheight in diagonal and decrease by 1 each step.
	//		|1|2|4|7| If current tile is higher, save that one as new highest point.
	//		|3|5|8|B| If not then that tile is in shadow.
	//		|6|9|C|E|
	//      |A|D|F|G|
	//		------- ONLY WORKS ON SQUARE MAPS!!!
	//
	int diagonalLines = (mapW + mapH) - 1;	// number of diagonal lines in map
	int midPoint = (diagonalLines / 2) + 1; // number of the diagonal that crosses midpoint of map
	int itemsInDiagonal = 0;				// stores number of tiles in a diagonal

	for (int diagonal = 1; diagonal <= diagonalLines; diagonal++)
	{
		itemsInDiagonal = -abs(diagonal - mapW) + mapW; // How many items are in the current diagonal
		float terrainPeakHeight = 1.f;

		// check the angle of the sun, if it has crossed zenith we need to run the diagonal in reverse
		int startItem = (sunAngleX >= 0) ? 0 : itemsInDiagonal - 1;
		int endItem = (sunAngleX >= 0) ? itemsInDiagonal - 1 : 0;
		int stepItem = (sunAngleX >= 0) ? 1 : -1;
		// printf("diagonal: %d startItem: %d endItem: %d stepItem: %d\n", diagonal, startItem, endItem, stepItem);
		for(int item = startItem; item != endItem; item += stepItem)
		{
			// X and Y start values needs to follow the upper edge and then go down the right edge of the map
			int y = item + (diagonal > mapW ? diagonal - mapW : 0);
			int x = ((diagonal > mapW) ? mapW - 1 : diagonal - 1) - item;

			// TODO: Find a way to calulate start and end values so we don't need to check if x and y are in bounds
			if(x == 0 || y == 0 || x == mapW - 1 || y == mapH - 1)
			{
				continue; // Skip the edges of the map
			}

			terrainPeakHeight -= sunAngleY;
			if (terrainPeakHeight > map.height[x + y * mapW])
			{
				if ((terrainPeakHeight - map.height[x + y * mapW]) > 2.f)
				{
					shadow[x + y * mapW] -= 0.10f * sunFactor;
				}
				else
				{
					//At the edge of the shadow, interpolate between shaded value and no shade
					shadow[x + y * mapW] -= 0.05f * (terrainPeakHeight - map.height[x + y * mapW]) * sunFactor;
				}
			}
			else
			{
				terrainPeakHeight = map.height[x + y * mapW];
			}
			//Add shading based on angle
			//TODO: Will sample outside shadow map 
			shadow[x+y*map.w] +=  clampf( (map.height[(x-1)+(y+1)*map.w] - map.height[(x+1)+(y-1)*map.w]) * (sunAngleX / (2 * M_PI)) , -0.3f * sunFactor, 0.0f);
		}
	}


#endif

	// smooth shadows
	//	boxBlur_4(shadow,map.shadow,mapW*mapH,mapW,mapH,1);
	//	boxBlur_4(map.shadow,shadow,mapW*mapH,mapW,mapH,1);
	//	boxBlur_4(shadow,map.shadow,mapW*mapH,mapW,mapH,10);
	// smoother shadows
	//	boxBlur_4(shadow,map.shadowSoft,mapW*mapH,mapW,mapH,4);



	for(int y=0;y<map.h;y++){
		for(int x=0;x<map.w;x++){
			// shadow[x+y*map.w] += (map.height[(x-1)+(y-1)*map.w] - map.height[(x+1)+(y+1)*map.w]) / 20.f;
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




	for(int y = 100; y < 120; y++){
		for(int x = 100; x < 120; x++){
			// shadow[x+y*map.w] = 1.0f;
		}
	}


	memcpy(map.shadow, shadow, sizeof(shadow));
}


argb_t getTileColorMist(Map* mapPtr, int x, int y, int ys, vec2f_t upVec)
{
    argb_t argb = rgb(255,0,0);
    int mapW = mapPtr->w;
    int mapH = mapPtr->h;

    float mistHeight =  mapPtr->stone[x+y*mapW] + mapPtr->sand[x+y*mapW] + mapPtr->water[x+y*mapW].depth + mapPtr->lava[x+y*mapW].depth + mapPtr->mist[x+y*mapW].depth;

	// Shoot a ray from the camera perspective down from the mist tile until it hit the ground, there sample the ground color
    float d; // How far the ray had to travel to reach the ground
	float mistDepth = 0.f; // How far the ray had to travel through mist
	float stepSize = 0.99f; // How far the ray travels each step
    for(d = 0.f; d < 300.f; d += stepSize)
	{
        int X = x + upVec.x * d; // Current x position of the ray
        int Y = y + upVec.y * d; // Current y position of the ray
		float rayHeight = mistHeight - (d * 0.79f); // Current height of the ray  //0.7071f = 1/sqrt(2)
        if(X >= 0 && X < mapW && Y >= 0 && Y < mapH)
		{

			// The ray can enter and leave masses of mist while traveling to the ground, we add mistDepth whenever the ray travel through mist
			if( mapPtr->stone[x+y*mapW] + mapPtr->sand[x+y*mapW] + mapPtr->water[x+y*mapW].depth + mapPtr->lava[x+y*mapW].depth + mapPtr->mist[X+Y*mapW].depth > rayHeight )
			{
				mistDepth += stepSize;
			}

			// Detect ray collision with ground
            if( mapPtr->stone[x+y*mapW] + mapPtr->sand[x+y*mapW] + mapPtr->water[x+y*mapW].depth + mapPtr->lava[x+y*mapW].depth > rayHeight )
			{

                argb = lerpargb(mapPtr->argb[X+Y*mapW], mapPtr->argbBlured[X+Y*mapW], clampf(mistDepth/20.f, 0.1f, 1.f));

                break;
            }
        }
		else
		{

			int yScreen = world2screen(X, Y, g_cam).y; // Get the screen y position of the ray
            
            //If the mist is up against the wall, sample the background picture to make it appear transparent
            //I don't know why the coordinates are like this, I just tried stuff until it worked....
            // argb.r = background[window.drawSize.h - ys].r; //(102+(int)ys)>>2;//67
            // argb.g = background[window.drawSize.h - ys].g; //(192+(int)ys)>>2;//157
            // argb.b = background[window.drawSize.h - ys].b; //(229+(int)ys)>>2;//197
			argb = backgroundLayer.frameBuffer[window.drawSize.h - yScreen];

            break;

        }

    }

	// Apply mist color based on depth
    argb = lerpargb(argb, pallete.mist, clampf(mistDepth/50.f, 0.1f, 1.f));

    return argb;
    
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
			
			

			if(map.present[x + mapPitch].lava)
			{
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
			else if(map.present[x + mapPitch].water) // Add water if present
			{
				float slopX = (map.height[(x + 1) + (y) * map.w] - map.height[(x - 1) + (y) * map.w]); 
				float slopY = (map.height[(x) + (y + 1) * map.w] - map.height[(x) + (y - 1) * map.w]);

				argb = lerpargb(argb, pallete.water, minf(map.water[x + mapPitch].depth*0.1f, 1.f));
				argb = lerpargb(argb, pallete.waterDark, minf(map.water[x + mapPitch].depth*0.05f, 1.f));


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
			else if(map.wetMap[x + mapPitch] > 0)
			{
				float slopX = (map.height[(x + 1) + (y) * map.w] - map.height[(x - 1) + (y) * map.w]); 
				float slopY = (map.height[(x) + (y + 1) * map.w] - map.height[(x) + (y - 1) * map.w]);

				// Highlight according to slope
				vec2f_t slopeVec = {.x = slopX + 0.000000001f, .y = slopY + 0.000000001f}; //The small addition is to prevent normalizing a zero length vector which we don't handle
				vec2f_t slopeVecNorm = normalizeVec2f(slopeVec);
				
				float glare = ((slopeVecNorm.x - upVec.x)*(slopeVecNorm.x - upVec.x)+(slopeVecNorm.y - upVec.y)*(slopeVecNorm.y - upVec.y)) * ((x-3) && (y-3) && (x-map.w+3) && (y-map.h+3)); //The thing at the end with makes it so if the x or y position is on the border then slopX and Y gets multiplied by 0 otherwise by 1
				// if(glare != glare) printf("heh\n");
				glare = minf(glare*0.05f, 1.f);

				argb = lerpargb(argb, pallete.white, glare);

				// Darken a bit
				argb = lerpargb(argb, pallete.black, 0.2f);
			}

			//Add foam
			if (map.foamLevel[x + mapPitch] > 0)
			{
				argb = lerpargb(argb, pallete.foam, minf(map.foamLevel[x + mapPitch] / 10.f, 1.f));
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
					case TOOL_WETMAP:
						map.argb[x + y * map.w] = lerpargb(map.argb[x + y * map.w], pallete.black, 0.1f + 0.02f * cursor.amount);
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
	
	// Uncomment to check result of blur shader
	// memcpy(map.argb, map.argbBlured, sizeof(map.argbBlured) / 2);
	
	for (int y = 0; y < map.h; y++)
	{
		int mapPitch = y * map.w;
		for (int x = 0; x < map.w; x++)
		{
			//Add mist if present
			if (map.mist[x + mapPitch].depth > 0)
			{
				map.argbBuffer[x + mapPitch] = getTileColorMist(&map, x, y, y, upVec);
			}
			else
			{
				map.argbBuffer[x + mapPitch] = map.argb[x + mapPitch];
			}

			//Add shadowmap if no lava because lava is emmisive
			if(!map.present[x + mapPitch].lava){
				map.argbBuffer[x + mapPitch].r *= map.shadow[x + mapPitch];
				map.argbBuffer[x + mapPitch].g *= map.shadow[x + mapPitch];
				map.argbBuffer[x + mapPitch].b *= map.shadow[x + mapPitch];
			}
		}
	}

	//TODO: Could be optimized by pointer swap?
	for(int i=0;i<map.w*map.h;i++){
		map.argb[i] = map.argbBuffer[i];
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
            float deltaV = (map.water[(x-1)+(y)*MAPW].right + map.water[(x)+(y+1)*MAPW].down + map.water[(x+1)+(y)*MAPW].left + map.water[(x)+(y-1)*MAPW].up - (map.water[(x)+(y)*MAPW].right + map.water[(x)+(y)*MAPW].down + map.water[(x)+(y)*MAPW].left + map.water[(x)+(y)*MAPW].up));

            if(deltaV > 10.f){
                map.foamLevel[x+y*h] = map.foamLevel[x+y*h] + velDiff * 0.1f * dTime;
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

				if(map.water[x+y*w].depth > 0.f){
					lavaConverted = minf(minf(map.lava[x+y*w].depth, map.water[x+y*w].depth), 2.f * dTime);
					map.lava[x+y*w].depth  -= lavaConverted; 
					map.stone[x+y*w]       += lavaConverted;
					map.water[x+y*w].depth -= lavaConverted * 1.f; 
					map.water[x+y*w].down  -= lavaConverted * 1.f; 
					map.water[x+y*w].up    -= lavaConverted * 1.f; 
					map.water[x+y*w].left  -= lavaConverted * 1.f; 
					map.water[x+y*w].right -= lavaConverted * 1.f; 
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


    simFluid(map.lava, map.height, 9.81f, 20.f, map.tileWidth, w, h, 1.f, minf(dTime*program.simSpeed, 0.13f));


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


	for(int i=0;i<1;i++){
		// simFluidSWE(&(map.waterSWE), map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.90f, 0.01f /*minf(dTime*program.simSpeed, 0.13f)*/);

	}

	simFluid(map.water, map.height, 9.81f, 0.f, map.tileWidth, w, h, 0.98f, minf(dTime*program.simSpeed, 0.13f));


	// memcpy(map.water.depth, map.waterSWE.depth, sizeof(float)*map.w*map.h);

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


    simFluid(map.mist, map.height, 9.81f, 1.f, map.tileWidth, w, h, 0.20f, minf(dTime*program.simSpeed, 0.13f));

	//Add mist to height
	for(int y=0;y<h;y++){
		for(int x=0;x<w;x++){
			map.height[x + y * w] += map.mist[x + y * w].depth;
		}
	}


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

    for(int y=1;y<h-1;y++){
        for(int x=1;x<w-1;x++){
            // calculate velocity
            map.waterVel[x + y * w].x = (map.water[(x - 1) + (y)*w].right - map.water[(x) + (y)*w].left + map.water[(x) + (y)*w].right - map.water[(x + 1) + (y)*w].left); // X
            map.waterVel[x + y * w].y = (map.water[(x) + (y - 1)*w].down - map.water[(x) + (y)*w].up + map.water[(x) + (y)*w].down - map.water[(x) + (y + 1)*w].up);       // Y

			map.lavaVel[x + y * w].x = (map.lava[(x - 1) + (y)*w].right - map.lava[(x) + (y)*w].left + map.lava[(x) + (y)*w].right - map.lava[(x + 1) + (y)*w].left) / (32.f); // X
            map.lavaVel[x + y * w].y = (map.lava[(x) + (y - 1)*w].down - map.lava[(x) + (y)*w].up + map.lava[(x) + (y)*w].down - map.lava[(x) + (y + 1)*w].up) / (32.f);       // Y

            map.present[x + y * w].water = (map.water[x + y * w].depth > 0.01f) ? 1 : 0;
            map.present[x + y * w].mist  = (map.mist[x + y * w].depth > 0.01f) ? 1 : 0;
            map.present[x + y * w].lava  = (map.lava[x + y * w].depth > 0.01f) ? 1 : 0;
        }
    }


    
    // PROFILE(erodeAndDeposit(map.sand, map.susSed, map.stone, map.water, map.waterVel, w, h);)
    relax(map.sand, map.stone, 45.f, 9.81f, w, h, minf(dTime*program.simSpeed, 0.13f)); 
    advect(map.susSed, map.susSed2, map.waterVel, w, h, minf(dTime*program.simSpeed, 0.13f));

    //Advect the foam
    advect(map.foamLevel, map.foamLevelBuffer, map.waterVel, w, h, minf(dTime*program.simSpeed, 0.13f));
    advect(map.lavaFoamLevel, map.lavaFoamLevelBuffer, map.lavaVel, w, h, minf(dTime*program.simSpeed, 0.13f));



}


static void init()
{

	map.w = MAPW;
	map.h = MAPH;
	map.tileWidth = 1.f;

	// map.waterSWE.depth = calloc(map.w * map.h, sizeof(float));
	// map.waterSWE.flow = calloc(map.w * map.h, sizeof(vec2f_t));

	renderMapBuffer.height = calloc(map.w * map.h, sizeof(float));
	renderMapBuffer.mistDepth = calloc(map.w * map.h, sizeof(float));
	renderMapBuffer.argb = calloc(map.w * map.h, sizeof(argb_t));
	renderMapBuffer.argbBlured = calloc(map.w * map.h, sizeof(argb_t));

	map.sunAngle = M_PI / 6.f;

	g_cam.zoom = minf((float)map.w / (float)rendererSizeX, (float)map.h / (float)rendererSizeY);
	
	//Center camera to map
	vec2f_t screenCenter = {rendererSizeX / 2, rendererSizeY / 2 + 50};
	for(int i=0;i<10;i++){
		vec2f_t worldCenter = world2screen(map.w / 2, map.h / 2, g_cam);
		vec2f_t centerDiff = {worldCenter.x - screenCenter.x, worldCenter.y - screenCenter.y};
		cam_pan(&g_cam, centerDiff.x, centerDiff.y);
	}
	//Rotate camera so shadows start from right to left
	cam_rot(&g_cam, 0.f);

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


	generateTerrain(terrainGen.maxHeight, terrainGen.detail, terrainGen.sand, terrainGen.x, terrainGen.y, map.w, map.h, &map);


	map.flags.updateShadowMap = 1; // make sure shadows are updated after map load
	map.flags.updateColorMap = 1;  // make sure shadows are updated after map load



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


			#ifdef GENERATE_CHECKERBOARD
			int tileSize = 32; // adjust tileSize for larger/smaller checkers
			int check = ((x / tileSize) + (y / tileSize)) % 2;
			if (check == 0) {
				map.argbStone[x + y * map.w].r = 200;
				map.argbStone[x + y * map.w].g = 50;
				map.argbStone[x + y * map.w].b = 50;
			} else {
				map.argbStone[x + y * map.w].r = 50;
				map.argbStone[x + y * map.w].g = 200;
				map.argbStone[x + y * map.w].b = 200;
			}
			#endif

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

	// for(int y=0;y<map.h;y++){
    //     for(int x=0;x<map.w;x++){
    //         map.sand[x + y * map.w] = 0.f;
	// 		map.stone[x + y * map.w] = 0.f;
	// 		map.water[x + y * map.w].depth = 0.f;
    //     }
    // }

	// Generate a stone tower in the middle
	for(int y = map.h/2 - 10; y < map.h/2 + 10; y++){
		for(int x = map.w/2 - 10; x < map.w/2 + 10; x++){
			// map.stone[x + y * map.w] = 100.f;
		}
	}

	drawBackground(backgroundLayer);


	// Init droplets to random positions within a 3d box
	for(int i = 0; i < droplets.max; i++){
		droplets.active[i] = 1;
		droplets.array[i].pos.x = rand() % map.w;
		droplets.array[i].pos.y = rand() % map.h;
		droplets.array[i].pos.z = rand() % 100;
		droplets.array[i].vel.x = 0.f;//rand() % 10 - 10.f;
		droplets.array[i].vel.y = 0.f;//rand() % 10 - 10.f;
		droplets.array[i].vel.z = 0.f;//rand() % 10 - 10.f;
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


void renderDroplets(Layer layer, Droplets* droplets, camera_t cam)
{
	clearLayer(layer);

	for(int i = 0; i < droplets->max; i++){
		if(droplets->active[i]){
			Droplet droplet = droplets->array[i];
			vec2f_t sPos = world2screen3D(droplet.pos.x,droplet.pos.y, droplet.pos.z, cam);
			if(sPos.x >= layer.w || sPos.x < 0 || sPos.y >= layer.h || sPos.y < 0){
				continue;
			}
			argb_t color = (argb_t){255, 255, 255, 50};
			// drawPoint(layer, sPos.x, sPos.y, color);
			layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].r = color.r ;//clampi(layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].r + color.r, 0, 255);
			layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].g = color.g ;//clampi(layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].g + color.g, 0, 255);
			layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].b = color.b ;//clampi(layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].b + color.b, 0, 255);
			layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].a = clampi(layer.frameBuffer[(int)sPos.x + (int)sPos.y * layer.w].a + color.a, 0, 255);

		}
	}
}

void updateDroplets(Droplets* droplets, float dTime)
{
	for(int i = 0; i < droplets->max; i++){
		if(droplets->active[i]){
			Droplet* droplet = &droplets->array[i];
			
			// Update droplet velocities
			droplet->vel.z -= 9.81f * dTime;

			// Update droplet positions
			droplet->pos.x += droplet->vel.x * dTime;
			droplet->pos.y += droplet->vel.y * dTime;
			droplet->pos.z += droplet->vel.z * dTime;
			
			// Handle collosion with the ground and edges
			if(droplet->pos.z < map.height[(int)droplet->pos.x + (int)droplet->pos.y * map.w]){
				// Reset droplet position
				droplet->pos.z = map.height[(int)droplet->pos.x + (int)droplet->pos.y * map.w];
				droplet->vel.z = -droplet->vel.z * 0.5f;

				// Apply gradient to droplet velocity
				vec2f_t gradient;
				gradient.x = map.height[(int)droplet->pos.x - 1 + (int)droplet->pos.y * map.w] - map.height[(int)droplet->pos.x + 1 + (int)droplet->pos.y * map.w];
				gradient.y = map.height[(int)droplet->pos.x + ((int)droplet->pos.y - 1) * map.w] - map.height[(int)droplet->pos.x + ((int)droplet->pos.y + 1) * map.w];
				droplet->vel.x += gradient.x * 0.9f;
				droplet->vel.y += gradient.y * 0.9f;
			}

			droplet->vel.x *= 0.999f;
			droplet->vel.y *= 0.999f;
			droplet->vel.z *= 0.999f;

			if(droplet->pos.x < 0)
			{
				droplet->pos.x = 0;
				droplet->vel.x = -droplet->vel.x;
			}
			if(droplet->pos.x > map.w)
			{
				droplet->pos.x = map.w;
				droplet->vel.x = -droplet->vel.x;
			}
			if(droplet->pos.y < 0)
			{
				droplet->pos.y = 0;
				droplet->vel.y = -droplet->vel.y;
			}
			if(droplet->pos.y > map.h)
			{
				droplet->pos.y = map.h;
				droplet->vel.y = -droplet->vel.y;
			}
		}
	}
}


static int mainLoop()
{


#ifdef DEBUG
	char titleString[200];
	sprintf(titleString, "fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime);
	if(window.time.tick.ms100) window_setTitle(titleString);
#endif /*DEBUG*/

	

	PROFILE(updateInput();)



	PROFILE(process();)

	//Update buffers that render will use
	renderMapBuffer.mapW = map.w;
	renderMapBuffer.mapH = map.h;
	for(int i = 0; i < MAPW * MAPH; i++)
	{
		renderMapBuffer.height[i] = map.height[i];
		renderMapBuffer.mistDepth[i] = map.mist[i].depth;
		renderMapBuffer.argb[i] = map.argb[i];
		renderMapBuffer.argbBlured[i] = map.argbBlured[i];
	}

	PROFILE(render(botLayer, &renderMapBuffer);)

	
	// Render middle layer
	// Render droplets
	updateDroplets(&droplets, minf(window.time.dTime, 0.1f));
	renderDroplets(midLayer, &droplets, g_cam);

	drawUI(uiLayer);

	drawProfile(uiLayer,10,10);

	return window_run();
}



int main()
{
	// Disable console buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	#ifdef _WIN32
		// Windows-specific code to enable UTF-8
		system("CHCP 65001"); // Enable unicode characters in the terminal
	#endif

	//Print title and set window title
	sprintf(window.title, "%s %d.%d.%d - %s %s\n", APP_NAME, APP_VER_MAJOR, APP_VER_MINOR, APP_VER_BUILD, __DATE__, __TIME__);
	printf("%s", window.title);


	window.drawSize.w = rendererSizeX;
	window.drawSize.h = rendererSizeY;
	window.size.w = windowSizeX;
	window.size.h = windowSizeY;

	window.settings.vSync = false;
	


	window_init();

	backgroundLayer = window_createLayer();
	// init bottom layer
	botLayer = window_createLayer();
	// init middle layer
	midLayer = window_createLayer();
	// init top layer
	uiLayer = window_createLayer();
	

	
	

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
	css_profile.print();
#endif /*DEBUG*/



	return 0;
}



