#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "window.h"

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