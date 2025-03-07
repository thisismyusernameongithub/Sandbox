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

#define RADIUS 3
const int radius = RADIUS;
const int diameter = 2 * (int)radius + 1;
const float sigma = radius / 4.f; // Width of distribution
const float s = 2.f * sigma * sigma; //Standard deviation I think
float gaussianWeights[(RADIUS * 2 + 1) * (RADIUS * 2 + 1)] = {0.f};


/**
 * @brief Simulates hydraulic erosion on a terrain using a particle-based approach
 * 
 * @param w Width of the terrain map
 * @param h Height of the terrain map
 * @param stone Pointer to the base terrain height map array
 * @param sand Pointer to the sand layer height map array
 * 
 * @details
 * This function simulates water droplets falling on the terrain and causing erosion.
 * Each droplet:
 * 1. Picks up sediment based on its velocity and the terrain slope
 * 2. Moves according to the terrain gradient and its inertia
 * 3. Deposits sediment when slowing down or moving uphill
 * 
 * Key parameters that control the erosion:
 * - Pcapacity: Maximum sediment a droplet can carry
 * - Pdeposition: Rate at which surplus sediment is deposited
 * - Perosion: Rate at which droplets erode the terrain
 * - Pevaporation: Rate at which droplets lose water
 * - initialSpeed: Starting velocity of droplets
 * - initialWaterVolume: Starting water volume of droplets
 * - inertia: How much droplets resist changing direction
 * - numberOfDroplets: Total number of droplets to simulate
 * 
 * The erosion uses a Gaussian kernel to distribute sediment pickup across a small area,
 * creating more natural-looking erosion patterns.
 */
void erode(int w, int h, float* stone, float* sand)
{
	
	float Pminslope = 0.f;
	float Pmaxslope = 1.f;
	float Pcapacity = 2.f;
	float Pdeposition = 0.1f; // how much of surplus sediment is deposited
	float Perosion = 0.01f;	 // how much a droplet can erode a tile
	float Pgravity = 9.81f;
	int Pmaxsteps = 100; // max number of steps for a droplet
	float Pevaporation = 0.03f;
	float initialSpeed = 1.f;
	float initialWaterVolume = 10.f;
	float inertia = 0.5f;
	int numberOfDroplets = 1000;


	if(gaussianWeights[0] == 0.f)
	{
		// Fill the Gaussian weight table
		for (int j = -radius; j <= radius; j++)
		{
			for (int k = -radius; k <= radius; k++) 
			{
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
		float capacity = 0;
		int earlyTerminationCount = 0;
		// get random starting coordinates
		x = rand() % w;
		y = rand() % h;


		int step = 0;
		for (step = 0; step < Pmaxsteps; step++)
		{

			int oldPosID = (int)(x) + (int)(y) * w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// Early termination conditions:
			// 1. If water volume is too small to cause changes
			// 2. If speed is too low for meaningful movement
			// 3. If the droplet is stationary (no direction)
			if (capacity < 0.1f && sediment < 0.1f) 
			{
				earlyTerminationCount++;
				if(earlyTerminationCount > 1)
				{
					// printf("Terminating early at step %d\n", step);
					break;
				}
			}
			else
			{
				earlyTerminationCount = 0;
			}

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
			capacity = minf(maxf(-hdiff, Pminslope),Pmaxslope) * speed * water * Pcapacity;
			
			// if drop carries less sediment then capacity, pick up sediment
			if (!(sediment > capacity || hdiff > 0))
			{
				// Pickup sediment from last position, but not more that the height difference
				float amount = minf((capacity - sediment) * Perosion, -hdiff);

				// pick up sediment from an area around the old pos (area we are leaving) using normal distribution
				// Make sure we aren't sampling outside the map, calculating minJ, maxJ and so on here lets us remove if (oldX + k >= 0 && oldY + j >= 0 && oldX + k < w - 0 && oldY + j < h - 0) from the inner loop
				int minJ = (oldY - radius < 0) * -oldY + !(oldY - radius < 0) * -radius; // Branchless version of: int minJ = (oldY - radius < 0) ? -oldY : -radius;
				int minI = (oldX - radius < 0) * -oldX + !(oldX - radius < 0) * -radius;
				int maxJ = (oldY + radius >= h) * (h - oldY - 1) + !(oldY + radius >= h) * radius;
				int maxI = (oldX + radius >= w) * (w - oldX - 1) + !(oldX + radius >= w) * radius;
				for (int j = minJ; j <= maxJ; j++)
				{
					for (int k = minI; k <= maxI; k++)
					{
						int index = (j + (int)radius) * diameter + (k + (int)radius);
						float temp_amount = amount * gaussianWeights[index];
						sand[oldPosID + k + j * w] = maxf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
						stone[oldPosID + k + j * w] += minf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
						sediment += temp_amount;
					}
				}
			}
			else // if drop carries more sediment then capacity
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

			// get new speed
			speed = sqrt(speed * speed + fabsf(hdiff) * Pgravity);
			// evaporate some water
			water = water * (1 - Pevaporation);
		}
		// printf("Step: %d, speed: %f, water: %f, capacity: %f, sediment: %f\n", step, speed, water, capacity, sediment);
		
		// Drop any remaining sediment
		if(((int)x > 1 && (int)y > 1 && (int)x < w - 1 && (int)y < h - 1))
		{
			sand[(int)x + (int)y * w] += sediment;
		}
	}

}


void oldErode(int w, int h, float* stone, float* sand)
{
	
	float Pminslope = 0.f;
	float Pmaxslope = 1.f;
	float Pcapacity = 2.f;
	float Pdeposition = 0.1f; // how much of surplus sediment is deposited
	float Perosion = 0.01f;	 // how much a droplet can erode a tile
	float Pgravity = 9.81f;
	int Pmaxsteps = 100; // max number of steps for a droplet
	float Pevaporation = 0.03f;
	float initialSpeed = 1.f;
	float initialWaterVolume = 10.f;
	float inertia = 0.5f;
	int numberOfDroplets = 1000;


	if(gaussianWeights[0] == 0.f){
		float sum = 0.f;
		// Fill the Gaussian weight table
		for (int j = -radius; j <= radius; j++) {
			for (int k = -radius; k <= radius; k++) {
				int index = (j + (int)radius) * diameter + (k + (int)radius);
				gaussianWeights[index] = expf(-(k * k + j * j) / s) / (M_PI * s);
				sum += gaussianWeights[index];
			}
		}
		printf("Gaussian weights sum: %f\n", sum);
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
		float capacity = 0;
		int earlyTerminationCount = 0;
		// get random starting coordinates
		x = rand() % w;
		y = rand() % h;


		int step = 0;
		for (step = 0; step < Pmaxsteps; step++)
		{

			int oldPosID = (int)(x) + (int)(y) * w;
			int oldX = (int)(x);
			int oldY = (int)(y);

			// Early termination conditions:
			// 1. If water volume is too small to cause changes
			// 2. If speed is too low for meaningful movement
			// 3. If the droplet is stationary (no direction)
			if (capacity < 0.1f && sediment < 0.1f) {
				earlyTerminationCount++;
				if(earlyTerminationCount > 1){
					// printf("Terminating early at step %d\n", step);
					break;
				}
			}else{
				earlyTerminationCount = 0;
			}

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
			capacity = minf(maxf(-hdiff, Pminslope),Pmaxslope) * speed * water * Pcapacity;
			
			// if drop carries less sediment then capacity, pick up sediment
			if (!(sediment > capacity || hdiff > 0))
			{
				// get std::min((c-sediment)*Perosion,-hdiff) sediment from old pos
				float amount = minf((capacity - sediment) * Perosion, -hdiff);
				// sediment += amount;


				// pick up sediment from an area around the old pos (area we are leaving) using normal distribution
				// Make sure we aren't sampling outside the map, calculating minJ, maxJ and so on here lets us remove if (oldX + k >= 0 && oldY + j >= 0 && oldX + k < w - 0 && oldY + j < h - 0) from the inner loop
				int minJ = (oldY - radius < 0) * -oldY + !(oldY - radius < 0) * -radius; // Branchless version of: int minJ = (oldY - radius < 0) ? -oldY : -radius;
				int minI = (oldX - radius < 0) * -oldX + !(oldX - radius < 0) * -radius;
				int maxJ = (oldY + radius >= h) * (h - oldY - 1) + !(oldY + radius >= h) * radius;
				int maxI = (oldX + radius >= w) * (w - oldX - 1) + !(oldX + radius >= w) * radius;
				for (int j = minJ; j <= maxJ; j++)
				{
					for (int k = minI; k <= maxI; k++)
					{
						int index = (j + (int)radius) * diameter + (k + (int)radius);
						float temp_amount = amount * gaussianWeights[index];
						sand[oldPosID + k + j * w] = maxf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
						stone[oldPosID + k + j * w] += minf(sand[oldPosID + k + j * w] - temp_amount, 0.f);
						sediment += temp_amount;
					}
				}
			}
			else // if drop carries more sediment then capacity
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

			// get new speed
			speed = sqrt(speed * speed + fabsf(hdiff) * Pgravity);
			// evaporate some water
			water = water * (1 - Pevaporation);
		}
		// printf("Step: %d, speed: %f, water: %f, capacity: %f, sediment: %f\n", step, speed, water, capacity, sediment);
		
		// Drop any remaining sediment
		if(((int)x > 1 && (int)y > 1 && (int)x < w - 1 && (int)y < h - 1))
		{
			sand[(int)x + (int)y * w] += sediment;
		}
	}


}
