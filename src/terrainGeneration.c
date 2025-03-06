/**
 * @file terrainGeneration.c
 * @brief Terrain generation implementation using Perlin noise
 * 
 * This file contains the implementation of terrain generation algorithms
 * using Perlin noise from the stb library to create realistic heightmaps
 * with features like stone terrain and sand dunes.
 */

#define STB_PERLIN_IMPLEMENTATION
#include <stdlib.h> //rand()
#include "stb_perlin.h"
#include "window.h"

#include "css_profile.h"

/**
 * @brief Generates terrain heightmap with stone and sand layers
 * 
 * @param maxHeight Maximum height of the terrain in meters
 * @param detail Level of detail/complexity for the terrain (affects number of octaves)
 * @param sand Sand level height as a percentage (0.0 - 1.0) of maxHeight
 * @param xOffset X-axis offset for the Perlin noise
 * @param yOffset Y-axis offset for the Perlin noise
 * @param mapW Width of the terrain map in pixels
 * @param mapH Height of the terrain map in pixels
 * @param stoneMap Output array for the stone heightmap values
 * @param sandMap Output array for the sand layer heightmap values
 * 
 * @details
 * The function generates two heightmaps:
 * - A stone base terrain using layered Perlin noise
 * - A sand layer that forms dunes using ridge noise
 * 
 * The detail parameter controls the number of octaves used in the Perlin noise,
 * with more octaves creating more complex terrain features.
 */
void generateTerrain(float maxHeight, float detail, float sand, float xOffset, float yOffset, int mapW, int mapH, float* stoneMap, float* sandMap)
{
	
	int w = mapW;
	int h = mapH;
	

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
				stoneMap[x + y * w] = 10.f;
				sandMap[x + y * w] = 0.f;

				min = 0.f;
				max = 100.f;
			}
		}
	}else{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				stoneMap[x + y * w] = 0.f;
				sandMap[x + y * w] = 0.f;
				float oPow2 = 1.f;
				for(int o = 1; o <= noOctaves; o++, oPow2 *= 2.f){ 
					
					stoneMap[x + y * w] += -fabsf(stb_perlin_noise3(oPow2*((float)x + xOffset)/(float)w, oPow2*((float)y + yOffset)/(float)h, 0, 0, 0, 0) / (oPow2));
					
				}
				

				min = minf(stoneMap[x + y * w], min);
				max = maxf(stoneMap[x + y * w], max);
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
			stoneMap[x + y * w] = (-min + (stoneMap[x + y * w])) / hDiff * maxHeight;

			//Flatten the stone to a height of 10m below sand height, then add 10m of sand on top along with dunes.
			if(stoneMap[x + y * w] < sandHeight){
				if(stoneMap[x + y * w] < sandHeight - 10.f){
					stoneMap[x + y * w] = lerp(stoneMap[x + y * w], (sandHeight - 10.f), (1.f - stoneMap[x + y * w]/maxHeight));
				}
				
				// sandMap[x + y * w] = lerp(stoneMap[x + y * w], sandHeight, (1.f - stoneMap[x + y * w]/maxHeight)) - stoneMap[x + y * w];
			}


			min2 = minf(stoneMap[x + y * w], min2);
			max2 = maxf(stoneMap[x + y * w], max2);
		}
	}

	for (int y = 2; y < h-2; y++)
	{
		for (int x = 2; x < w-2; x++)
		{
			if(stoneMap[x + y * w] < sandHeight){
				sandMap[x + y * w] = (-min + stb_perlin_ridge_noise3(((float)x + xOffset)/(float)w, ((float)y + yOffset)/(float)h, 0.f, 2.5f, 2.8f, 1.f, 3, 0, 0, 0))  * 10.f * (1.f - stoneMap[x + y * w] / sandHeight);
			}
		}

	}

	// for (int y = 0; y < h; y++)
	// {
	// 	for (int x = 0; x < w; x++)
	// 	{
	// 		vec2f_t prim = {
	// 			.x = stoneMap[(x+1)+(y)*w] - stoneMap[(x-1)+(y)*w],
	// 			.y = stoneMap[(x)+(y+1)*w] - stoneMap[(x)+(y-1)*w]
	// 			};
	// 		vec2f_t bis = {
	// 			.x = (stoneMap[(x+1)+(y)*w] - stoneMap[(x)+(y)*w]) - (stoneMap[(x)+(y)*w] - stoneMap[(x-1)+(y)*w]),
	// 			.y = (stoneMap[(x)+(y+1)*w] - stoneMap[(x)+(y)*w]) - (stoneMap[(x)+(y)*w] - stoneMap[(x)+(y-1)*w])
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



void erodeOld(int w, int h, float* stone, float* sand)
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
	int numberOfDroplets = 1000;

	for (int iter = 0; iter < numberOfDroplets; iter++)
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
		x = rand() % w;
		y = rand() % h;



		for (int step = 0; step < Pmaxsteps; step++)
		{

			int oldPosID = (int)(x) + (int)(y) * w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// get slope gradient
			float slopeX = stone[(oldX + 1) + (oldY) * w] + sand[(oldX + 1) + (oldY) * w] - stone[(oldX - 1) + (oldY) * w] - sand[(oldX - 1) + (oldY) * w]; // height difference
			float slopeY = stone[(oldX) + (oldY + 1) * w] + sand[(oldX) + (oldY + 1) * w] - stone[(oldX) + (oldY - 1) * w] - sand[(oldX) + (oldY - 1) * w]; // height difference
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
			int newPosID = (int)(x) + (int)(y) * w;
			// check if droplet is still within the map
			if(!((int)x > 1 && (int)y > 1 && (int)x < w - 1 && (int)y < h - 1))
			{
				sand[oldPosID] += sediment;
				break;
			}
			// get height difference
			hdiff = stone[newPosID] + sand[newPosID] - stone[oldPosID] - sand[oldPosID];
			// calculate new carrying capacity
			float capacity = minf(maxf(-hdiff, Pminslope),Pmaxslope) * speed * water * Pcapacity;
			// printf("%d: %f * %f * %f * %f = %f\n", step, hdiff, speed, water, Pcapacity, capacity);
			// if (stone[newPosID] + sand[newPosID] < -1){
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
					sand[oldPosID] += amount;

				}
				
				{
					// drop (sediment - capacity)*Pdeposition at old pos
					float amount = (sediment - capacity) * Pdeposition;
					sediment -= amount;
					sand[oldPosID] += amount;

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
						if (oldX + k > 1 && oldY + j > 1 && oldX + k < w - 1 && oldY + j < h - 1)
						{
							float r = sqrtf(k * k + j * j);
							float temp_amount = amount * expf(-(r * r) / (s)) / (M_PI * s);
							stone[oldPosID + k + j * w] += minf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
							sand[oldPosID + k + j * w] = maxf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
							sediment += temp_amount;

							// float temp_amount = amount * (exp(-(k * k + j * j) / (2 * 8)) / (2 * 3.14159265359 * 8)) / 1.14022; // amount to pick up
							// if (sand[oldPosID] > 0)
							// { // if there is sediment
							// 	float diff = sand[oldPosID + k + j * w] - temp_amount;
							// 	if (diff > 0)
							// 	{ // there is more sediment then is picked up
							// 		sand[oldPosID + k + j * w] -= temp_amount;

							// 		temp_amount = 0;
							// 	}
							// 	else
							// 	{ // there is less sediment then is picked up
							// 		temp_amount -= sand[oldPosID + k + j * w];

							// 		sand[oldPosID + k + j * w] = 0;
							// 	}
							// }

							// stone[oldPosID + k + j * w] -= temp_amount; //*exp(-(k*k+j*j)/(2))/(2*3.14159265359)/1.32429; //remove rest of sediment from rock
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


	// css_profile.print();
}

#define RADIUS 4
const int radius = RADIUS;
const int diameter = 2 * (int)radius + 1;
const float sigma = radius / 4.f; // Width of distribution
const float s = 2.f * sigma * sigma; //Standard deviation I think
float gaussianWeights[(RADIUS * 2 + 1) * (RADIUS * 2 + 1)] = {0.f};


void erode(int w, int h, float* stone, float* sand)
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
	int numberOfDroplets = 1000;


	if(gaussianWeights[0] == 0.f){
		// Fill the Gaussian weight table
		for (int j = -radius; j <= radius; j++) {
			for (int k = -radius; k <= radius; k++) {
				int index = (j + (int)radius) * diameter + (k + (int)radius);
				gaussianWeights[index] = expf(-(k * k + j * j) / s) / (M_PI * s);
			}
		}
	}

	for (int iter = 0; iter < numberOfDroplets; iter++)
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
		x = rand() % w;
		y = rand() % h;



		for (int step = 0; step < Pmaxsteps; step++)
		{

			int oldPosID = (int)(x) + (int)(y) * w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// get slope gradient
			float slopeX = stone[(oldX + 1) + (oldY) * w] + sand[(oldX + 1) + (oldY) * w] - stone[(oldX - 1) + (oldY) * w] - sand[(oldX - 1) + (oldY) * w]; // height difference
			float slopeY = stone[(oldX) + (oldY + 1) * w] + sand[(oldX) + (oldY + 1) * w] - stone[(oldX) + (oldY - 1) * w] - sand[(oldX) + (oldY - 1) * w]; // height difference
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
			int newPosID = (int)(x) + (int)(y) * w;
			// check if droplet is still within the map
			if(!((int)x > 1 && (int)y > 1 && (int)x < w - 1 && (int)y < h - 1))
			{
				sand[oldPosID] += sediment;
				break;
			}
			// get height difference
			hdiff = stone[newPosID] + sand[newPosID] - stone[oldPosID] - sand[oldPosID];
			// calculate new carrying capacity
			float capacity = minf(maxf(-hdiff, Pminslope),Pmaxslope) * speed * water * Pcapacity;
			// printf("%d: %f * %f * %f * %f = %f\n", step, hdiff, speed, water, Pcapacity, capacity);
			// if (stone[newPosID] + sand[newPosID] < -1){
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
					sand[oldPosID] += amount;

				}
				
				{
					// drop (sediment - capacity)*Pdeposition at old pos
					float amount = (sediment - capacity) * Pdeposition;
					sediment -= amount;
					sand[oldPosID] += amount;

				}
			}
			else
			{ // if drop carries less sediment then capacity, pick up sediment

				// get std::min((c-sediment)*Perosion,-hdiff) sediment from old pos
				float amount = minf((capacity - sediment) * Perosion, -hdiff);
				// sediment += amount;


				// pick up sediment from an area around the old pos using normal distribution
				for (int j = -radius; j <= radius; j++)
				{
					for (int k = -radius; k <= radius; k++)
					{
						if (oldX + k > 1 && oldY + j > 1 && oldX + k < w - 1 && oldY + j < h - 1)
						{
							int index = (j + (int)radius) * diameter + (k + (int)radius);
							float temp_amount = amount * gaussianWeights[index];
							stone[oldPosID + k + j * w] += minf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
							sand[oldPosID + k + j * w] = maxf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
							sediment += temp_amount;

							
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


	// css_profile.print();
}
