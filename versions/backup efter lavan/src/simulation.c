#include "simulation.h"

#include <math.h> //sqrtf()
#include <string.h> //memcpy()

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*(M_PI/180.f))
#define RAD2DEG(x) ((x)*(180.f/M_PI))

#define errLog(message) \
	fprintf(stderr, "\nFile: %s, Function: %s, Line: %d, Note: %s\n", __FILE__, __FUNCTION__, __LINE__, message);


static inline float maxf(float a, float b){
    return (a > b) ? a : b;
}

static inline float minf(float a, float b){
    return (a < b) ? a : b;
}

static inline float lerp(float s, float e, float t)
{
	return s + (e - s) * t;
}

static inline float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
	//    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
	const float s = c00 + (c10 - c00) * tx;
	const float e = c01 + (c11 - c01) * tx;
	return (s + (e - s) * ty);
}

static float sqrtFast(float number)
{
	int i;
	float x = number * 0.5f;
	float y = number;
	i = *(int*)&y;
	i = 0x5F3759DF - (i >> 1);
	y = *(float*)&i;
	y = y * (1.5f - (x * y * y));
	y = y * (1.5f - (x * y * y));
	return (number * y);
}




//Advects a density matrix along velocity vectors of some fluid 
void advect(float* restrict densityMatrix, float* restrict bufferMatrix, vec2f_t* restrict velocityVector, const int w, const int h, const float dTime){

    //We are using a semi-lagrangian advection scheme where we take a timestep backwards along the velocity vector and move that density to our position.
	for(int y = 2; y < h - 2; y++){
		for(int x = 2; x < w - 2; x++){

            //Get velocity vector
            const float dX = (velocityVector[x+y*w].x)*dTime;
            const float dY = (velocityVector[x+y*w].y)*dTime;

            const int xdx = (int)(x-dX);
            const int ydy = (int)(y-dY);

			//If reverse timestep coordinates is out of bounds: skip advection for this cell
            if( xdx < 3 || xdx > w - 3  || ydy < 3 || ydy > h - 3){
				bufferMatrix[(x)+(y)*w] = densityMatrix[(x)+(y)*w];
				continue;
			} 

            //Sample the four points around the target cell and interpolate between them
            float f1 = densityMatrix[(xdx)  +(ydy)*w];
            float f2 = densityMatrix[(xdx+1)+(ydy)*w];
            float f3 = densityMatrix[(xdx)  +(ydy+1)*w];
            float f4 = densityMatrix[(xdx+1)+(ydy+1)*w];

            float tX = (x-dX) - xdx;
            float tY = (y-dY) - ydy;

            float amountTransported = blerp(f1,f2,f3,f4,tX,tY);

            //Get factor of each sampled point
            // float r1 = (1-tX)*(1-tY);
            // float r2 = (tX)  *(1-tY);
            // float r3 = (1-tX)*(tY);
            // float r4 = (tX)  *(tY);

            // printf("%f %f = %f %f %f %f = %f\n",tX,tY,r1,r2,r3,r4,r1+r2+r3+r4); //Validate that the sum of r is 1
            // printf("\n");
            // printf("(%f)\t(%f)\n(%f)\t(%f) = (%f)\n",
            // foamLevelBuffer[((int)(x-dX))  +((int)(y-dY))*map.w],
            // foamLevelBuffer[((int)(x-dX)+1)+((int)(y-dY))*map.w],
            // foamLevelBuffer[((int)(x-dX))  +((int)(y-dY)+1)*map.w],
            // foamLevelBuffer[((int)(x-dX)+1)+((int)(y-dY)+1)*map.w],
            // foamLevelBuffer[(x)+(y)*map.w]);

            // printf("%f %f %f %f = %f = %f\n", amountTransported * r1, amountTransported * r2, amountTransported * r3, amountTransported * r4, amountTransported * r1+ amountTransported * r2+ amountTransported * r3 + amountTransported * r4,amountTransported);

            // foamLevelBuffer[((int)(x-dX))  +((int)(y-dY))*map.w] -= amountTransported   * r1;
            // foamLevelBuffer[((int)(x-dX)+1)+((int)(y-dY))*map.w] -= amountTransported   * r2;
            // foamLevelBuffer[((int)(x-dX))  +((int)(y-dY)+1)*map.w] -= amountTransported * r3;
            // foamLevelBuffer[((int)(x-dX)+1)+((int)(y-dY)+1)*map.w] -= amountTransported * r4;

            bufferMatrix[(x)+(y)*w] = amountTransported;
		}
	}
	memcpy(densityMatrix,bufferMatrix,sizeof(float)*w*h);
}

//Relax some sort of matter (Probably sand) over a terrain. 
void relax(float* restrict subject, float* restrict terrain, float talusAngle, const float g, const int w, const int h, const float dTime){

    float talusAngleRad = DEG2RAD(talusAngle);
    float maxSlope = tanf(talusAngleRad);

	int jMax, jMin, iMax, iMin;
	for (int y = 2; y < h - 1; y++)
	{
		iMin = (y == 2) ? 0 : -1;
		iMax = (y == h - 2) ? 0 : 1;
		for (int x = 2; x < w - 1; x++)
		{
			//If no subject is present to relax: then skip this cell
			if(subject[x + y * w] <= 0.f)
			{
				continue;
			}

			jMin = (x == 2) ? 0 : -1;
			jMax = (x == w - 2) ? 0 : 1;
			for (int i = iMin; i <= iMax; i++)
			{
				for (int j = jMin; j <= jMax; j++)
				{
					float slope = terrain[x + y * w] + subject[x + y * w] - terrain[(x + j) + (y + i) * w] - subject[(x + j) + (y + i) * w];
					if (slope > maxSlope)
					{
						float heightDiff = minf(slope, subject[x + y * w]) * g * dTime;

						subject[(x + j) + (y + i) * w] += heightDiff * 0.2f;
						subject[x + y * w] -= heightDiff * 0.2f;

					}
				}
			}
		}
	}

}

void erodeAndDeposit(float* restrict subject, float* restrict suspendedSubject, float* restrict terrain, fluid_t* restrict medium, vec2f_t* restrict mediumVel, const int w, const int h){
    // erosion/deposition of sediment
	//  https://ranmantaru.com/blog/2011/10/08/water-erosion-on-heightmap-terrain/
	//  https://old.cescg.org/CESCG-2011/papers/TUBudapest-Jako-Balazs.pdf
	//  https://hal.inria.fr/file/index/docid/402079/filename/FastErosion_PG07.pdf chapter 3.3
	// memcpy(map.sandTemp, map.sand, sizeof(map.sand));
	for (int y = 2; y < h - 3; y++)
	{
		for (int x = 2; x < w - 3; x++)
		{
			if (medium[x + y * w].depth < 0.001f)
			{
				float dropped = (suspendedSubject[x + y * w]);
				subject[x + y * w] += dropped;
				suspendedSubject[x + y * w] = 0;
				// medium[x + y * w].depth = 0;
			}else{
				// calculate tilt https://math.stackexchange.com/questions/1044044/local-tilt-angle-based-on-height-field
				float tiltX = (terrain[(x + 1) + y * w] + subject[(x + 1) + y * w] - terrain[(x - 1 + y * w)] - subject[(x - 1 + y * w)]) / 2.f;
				float tiltY = (terrain[(x) + (y + 1) * w] + subject[(x) + (y + 1) * w] - terrain[(x + (y - 1) * w)] - subject[(x + (y - 1) * w)]) / 2.f;
				float sinTheta = (sqrtFast(tiltX * tiltX + tiltY * tiltY)) / (sqrtFast(1 + tiltX * tiltX + tiltY * tiltY));
				float vel = sqrtFast(mediumVel[x + y * w].x * mediumVel[x + y * w].x + mediumVel[x + y * w].y * mediumVel[x + y * w].y);
				float Kc = 0.08f;  // sediment capacity constant
				float Ks = 0.09f;  // sediment dissolving constant
				float Kd = 0.03f; // sediment deposition constant

				// sediment transport capacity
				float C = Kc * sinTheta * vel;

				if (C <= suspendedSubject[x + y * w]) //If suspended sediment is larger than what the medium can handle
				{ // Drop sand
					// Make sure dropped sand does not create a peak by not allowing to drop on the local highest point
					float localMax = terrain[x + y * w] + subject[x + y * w];
					for (int i = -1; i < 2; i++)
					{
						for (int j = -1; j < 2; j++)
						{
							localMax = maxf(localMax, terrain[(x + j) + (y + i) * w] + subject[(x + j) + (y + i) * w]);
						}
					}
					if (localMax - terrain[x + y * w] + subject[x + y * w] < 0.001f){
						continue;
					}
					float dropped = Kd * (suspendedSubject[x + y * w] - C);
					subject[x + y * w] += dropped;
					suspendedSubject[x + y * w] -= dropped;
					// medium[x + y * w].depth -= dropped;
				}
				else if (C > suspendedSubject[x + y * w]) //If sediment transport capacity is larger than what is currently transported
				{ // Pick up sand
					// Make sure the sand picked up is not more than any nearby tile, creating a pit
					float localMin = terrain[x + y * w] + subject[x + y * w];
					for (int i = -1; i < 2; i++)
					{
						for (int j = -1; j < 2; j++)
						{
							localMin = minf(localMin, terrain[(x + j) + (y + i) * w] + subject[(x + j) + (y + i) * w]);
						}
					}
					float pickedUp = minf(Ks * (C - suspendedSubject[x + y * w]), subject[x + y * w]);
					subject[x + y * w] -= pickedUp;
					suspendedSubject[x + y * w] += pickedUp;
					// medium[x + y * w].depth += pickedUp;
				}

			}

			

		}
	}
}


void simFluid(fluid_t* restrict fluid, float* restrict terrain, const float g, float visc, const float l, const int w, const int h, const float friction, const float dTime)
{

	const float A = l*l; //Cross sectional area of pipe
	fluid_t* restrict f = fluid; // shorter name
	float* restrict t = terrain; // shorter name
	const float v = visc;

	if(visc == 0.f){
		for (int y = 2; y < h - 2; y++)
		{
			const int yw = y * w;
			for (int x = 2; x < w - 2; x++)
			{
				if((f[(x) + (y)*w].depth) > 0.01f){
					f[x + yw].right = maxf(f[x + yw].right * friction + (f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ) * dTime * A * g / l, 0.f);					   
					f[x + yw].down  = maxf(f[x + yw].down  * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ) * dTime * A * g / l, 0.f); 
					f[x + yw].left  = maxf(f[x + yw].left  * friction + (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ) * dTime * A * g / l, 0.f);						   
					f[x + yw].up    = maxf(f[x + yw].up    * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ) * dTime * A * g / l, 0.f);	  

					// float d = f[x + yw].depth;
					// float V = (d*d) / ((d*d) + 3.f * v * dTime);
					// f[x + yw].right *= V;
					// f[x + yw].down  *= V;
					// f[x + yw].left  *= V;
					// f[x + yw].up    *= V;

				}else{
					f[x + y * w].right = 0;
					f[x + y * w].down = 0;
					f[x + y * w].left = 0;
					f[x + y * w].up = 0;
				}
			}
		}
	}else{
		for (int y = 2; y < h - 2; y++)
		{
			const int yw = y * w;
			for (int x = 2; x < w - 2; x++)
			{
				if((f[(x) + (y)*w].depth) > 0.01f){
					f[x + yw].right = maxf(f[x + yw].right * friction + (f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ) * dTime * A * g / l, 0.f);					   
					f[x + yw].down  = maxf(f[x + yw].down  * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ) * dTime * A * g / l, 0.f); 
					f[x + yw].left  = maxf(f[x + yw].left  * friction + (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ) * dTime * A * g / l, 0.f);						   
					f[x + yw].up    = maxf(f[x + yw].up    * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ) * dTime * A * g / l, 0.f);	  

					float d = f[x + yw].depth;
					float V = (d*d) / ((d*d) + 3.f * v * dTime);
					f[x + yw].right *= V;
					f[x + yw].down  *= V;
					f[x + yw].left  *= V;
					f[x + yw].up    *= V;

				}else{
					f[x + y * w].right = 0;
					f[x + y * w].down = 0;
					f[x + y * w].left = 0;
					f[x + y * w].up = 0;
				}
			}
		}
	}


	// border conditions
	for (int y = 0; y < h; y++)
	{
		f[(w - 3) + y * w].right = 0;
		f[3 + y * w].left = 0;
	}
	for (int x = 0; x < w; x++)
	{
		f[x + 3 * w].up = 0;
		f[x + (h - 3) * w].down = 0;
	}

	for (int i = 0; i < w*h; i++){
		// make sure flow out of cell isn't greater than inflow + existing fluid
		if (f[i].depth - (f[i].right + f[i].down + f[i].left + f[i].up) < 0)
		{
			float K = minf(f[i].depth * l * l / ((f[i].right + f[i].down + f[i].left + f[i].up) * dTime), 1.0f);
			f[i].right *= K;
			f[i].down  *= K;
			f[i].left  *= K;
			f[i].up    *= K;
		}
	}

	// update depth
	for (int y = 2; y < h - 2; y++)
	{
		for (int x = 2; x < w - 2; x++)
		{
			float deltaV = (f[(x - 1) + (y)*w].right + f[(x) + (y + 1) * w].up + f[(x + 1) + (y)*w].left + f[(x) + (y - 1) * w].down - (f[(x) + (y)*w].right + f[(x) + (y)*w].down + f[(x) + (y)*w].left + f[(x) + (y)*w].up)) * dTime;

			f[(x) + (y)*w].depth = maxf(f[(x) + (y)*w].depth + deltaV / (l * l), 0.f);


		}
	}



}