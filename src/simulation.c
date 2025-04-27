#include "simulation.h"

#include <math.h> //sqrtf()
#include <string.h> //memcpy()
#include <stdlib.h>

#include <immintrin.h> //SIMD stuff
// #include <avxintrin.h>

#include "window.h"



#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*(M_PI/180.f))
#define RAD2DEG(x) ((x)*(180.f/M_PI))

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

			//If there is no velocity then there is no denisty moving into this cell, skip this cell
			if(dX == 0.f && dY == 0.f){
				bufferMatrix[(x)+(y)*w] = densityMatrix[(x)+(y)*w];
				continue;
			}

            const int xdx = (int)((float)x-dX);
            const int ydy = (int)((float)y-dY);

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

			//If no density is availibe to advect, skip this cell.
			if(f1 + f2 + f3 + f4 == 0.f){
				bufferMatrix[(x)+(y)*w] = densityMatrix[(x)+(y)*w];
				continue;
			}

            float tX = ((float)x-dX) - (float)xdx;
            float tY = ((float)y-dY) - (float)ydy;

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
void relax(float* restrict subject, float* restrict terrain, float talusAngle, const float g, const int w, const int h, const float dTime)
{

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
	const float dTimeAgbyl = dTime * A * g / l;

	__m128 frictionVec  = _mm_set1_ps(friction);
	__m128 dTimeAgbylVec  = _mm_set1_ps(dTimeAgbyl);
	__m128 zeroVec  = _mm_set1_ps(0.f);

	if(visc == 0.f)
	{
		for (int y = 1; y < h - 1; y++)
		{
			const int yw = y * w;
			for (int x = 1; x < w - 1; x++)
			{
				if((f[(x) + yw].depth) > 0.01f)
				{
					__m128 dirVec  = _mm_set_ps(f[x + yw].right, f[x + yw].down, f[x + yw].left, f[x + yw].up);
					__m128 depth0Vec  = _mm_set1_ps(f[x + yw].depth);
					__m128 t0Vec  = _mm_set1_ps(t[x + yw]);
					__m128 depth1Vec  = _mm_set_ps(f[(x + 1) + (yw)].depth, f[(x) + (y + 1) * w].depth, f[(x - 1) + (yw)].depth, f[(x) + (y - 1) * w].depth);
					__m128 t1Vec  = _mm_set_ps(t[(x + 1) + yw], t[(x) + (y + 1) * w], t[(x - 1) + yw], t[(x) + (y - 1) * w]);
					__m128 thingVec = _mm_add_ps(depth0Vec, t0Vec);
					thingVec = _mm_sub_ps(thingVec, depth1Vec);
					thingVec = _mm_sub_ps(thingVec, t1Vec);
					// __m128 thingVec  = _mm_set_ps((f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ),  (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ), (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ), (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ));
					
					thingVec = _mm_mul_ps(thingVec, dTimeAgbylVec);
					dirVec = _mm_mul_ps(dirVec, frictionVec);
					dirVec = _mm_add_ps(dirVec, thingVec);
					dirVec = _mm_max_ps(dirVec, zeroVec);

					_MM_EXTRACT_FLOAT(f[x + yw].right, dirVec, 3);
					_MM_EXTRACT_FLOAT(f[x + yw].down, dirVec, 2);
					_MM_EXTRACT_FLOAT(f[x + yw].left, dirVec, 1);
					_MM_EXTRACT_FLOAT(f[x + yw].up, dirVec, 0);


					// f[x + yw].right = maxf(f[x + yw].right * friction + (f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ) * dTimeAgbyl, 0.f);					   
					// f[x + yw].down  = maxf(f[x + yw].down  * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ) * dTimeAgbyl, 0.f); 
					// f[x + yw].left  = maxf(f[x + yw].left  * friction + (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ) * dTimeAgbyl, 0.f);						   
					// f[x + yw].up    = maxf(f[x + yw].up    * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ) * dTimeAgbyl, 0.f);	  

					// float d = f[x + yw].depth;
					// float V = (d*d) / ((d*d) + 3.f * v * dTime);
					// f[x + yw].right *= V;
					// f[x + yw].down  *= V;
					// f[x + yw].left  *= V;
					// f[x + yw].up    *= V;

				}
				else
				{
					f[x + yw].right = 0;
					f[x + yw].down = 0;
					f[x + yw].left = 0;
					f[x + yw].up = 0;
				}
			}
		}
	}
	else
	{
		for (int y = 1; y < h - 1; y++)
		{
			const int yw = y * w;
			for (int x = 1; x < w - 1; x++)
			{
				if((f[(x) + (y)*w].depth) > 0.01f)
				{
					__m128 dirVec  = _mm_set_ps(f[x + yw].right, f[x + yw].down, f[x + yw].left, f[x + yw].up);
					__m128 depth0Vec  = _mm_set1_ps(f[x + yw].depth);
					__m128 t0Vec  = _mm_set1_ps(t[x + yw]);
					__m128 depth1Vec  = _mm_set_ps(f[(x + 1) + (yw)].depth, f[(x) + (y + 1) * w].depth, f[(x - 1) + (yw)].depth, f[(x) + (y - 1) * w].depth);
					__m128 t1Vec  = _mm_set_ps(t[(x + 1) + yw], t[(x) + (y + 1) * w], t[(x - 1) + yw], t[(x) + (y - 1) * w]);
					__m128 thingVec = _mm_add_ps(depth0Vec, t0Vec);
					thingVec = _mm_sub_ps(thingVec, depth1Vec);
					thingVec = _mm_sub_ps(thingVec, t1Vec);
					// __m128 thingVec  = _mm_set_ps((f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ),  (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ), (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ), (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ));
					
					thingVec = _mm_mul_ps(thingVec, dTimeAgbylVec);
					dirVec = _mm_mul_ps(dirVec, frictionVec);
					dirVec = _mm_add_ps(dirVec, thingVec);
					dirVec = _mm_max_ps(dirVec, zeroVec);

					_MM_EXTRACT_FLOAT(f[x + yw].right, dirVec, 3);
					_MM_EXTRACT_FLOAT(f[x + yw].down, dirVec, 2);
					_MM_EXTRACT_FLOAT(f[x + yw].left, dirVec, 1);
					_MM_EXTRACT_FLOAT(f[x + yw].up, dirVec, 0);

					// f[x + yw].right = maxf(f[x + yw].right * friction + (f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ) * dTime * A * g / l, 0.f);					   
					// f[x + yw].down  = maxf(f[x + yw].down  * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ) * dTime * A * g / l, 0.f); 
					// f[x + yw].left  = maxf(f[x + yw].left  * friction + (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ) * dTime * A * g / l, 0.f);						   
					// f[x + yw].up    = maxf(f[x + yw].up    * friction + (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ) * dTime * A * g / l, 0.f);	  

					float d = f[x + yw].depth;
					float V = (d*d) / ((d*d) + 3.f * v * dTime);
					f[x + yw].right *= V;
					f[x + yw].down  *= V;
					f[x + yw].left  *= V;
					f[x + yw].up    *= V;

				}
				else
				{
					f[x + y * w].right = 0;
					f[x + y * w].down = 0;
					f[x + y * w].left = 0;
					f[x + y * w].up = 0;
				}
			}
		}
	}


	// // border conditions
	// for (int y = 0; y < h; y++)
	// {
	// 	f[(w - 3) + y * w].right = 0;
	// 	f[3 + y * w].left = 0;
	// }
	// for (int x = 0; x < w; x++)
	// {
	// 	f[x + 3 * w].up = 0;
	// 	f[x + (h - 3) * w].down = 0;
	// }

	for (int i = 0; i < w*h ; i++){
		// make sure flow out of cell isn't greater than inflow + existing fluid
		if (f[i].depth - (f[i].right + f[i].down + f[i].left + f[i].up) < 0)
		{
			float K = minf(f[i].depth * l * l / ((f[i].right + f[i].down + f[i].left + f[i].up) * dTime), 1.0f);
			f[i].right = f[i].right * K;
			f[i].down  = f[i].down  * K;
			f[i].left  = f[i].left  * K;
			f[i].up    = f[i].up    * K;
		}
	}

	// update depth
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{
			float deltaV = (f[(x - 1) + (y)*w].right + f[(x) + (y + 1) * w].up + f[(x + 1) + (y)*w].left + f[(x) + (y - 1) * w].down - (f[(x) + (y)*w].right + f[(x) + (y)*w].down + f[(x) + (y)*w].left + f[(x) + (y)*w].up)) * dTime;

			f[(x) + (y)*w].depth = maxf(f[(x) + (y)*w].depth + deltaV / (l * l), 0.f);


		}
	}




}


void simFluidSWE(fluidSWE_t* fluid, float* terrain, float g, float visc, float l, int w, int h, float friction, float dTime)
{

	static float depthBuffer[256*256];
	static vec2f_t flowBuffer[256*256];
	static vec2f_t newFlowVel[256*256];

	float* depth = fluid->depth;
	vec2f_t* flow = fluid->flow;

	//Flow advection
	//Predictor step
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{
			flowBuffer[(x)+(y)*w].x = flow[(x)+(y)*w].x - (flow[(x+1)+(y)*w].x - flow[(x)+(y)*w].x) * dTime;  
			flowBuffer[(x)+(y)*w].y = flow[(x)+(y)*w].y - (flow[(x)+(y+1)*w].y - flow[(x)+(y)*w].y) * dTime;  
		}
	}

	//Corrector step
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{
			newFlowVel[(x)+(y)*w].x = 0.5f * (flow[(x)+(y)*w].x + flowBuffer[(x)+(y)*w].x - dTime * (flowBuffer[(x)+(y)*w].x - flowBuffer[(x-1)+(y)*w].x));
			newFlowVel[(x)+(y)*w].y = 0.5f * (flow[(x)+(y)*w].y + flowBuffer[(x)+(y)*w].y - dTime * (flowBuffer[(x)+(y)*w].y - flowBuffer[(x)+(y-1)*w].y));

			//Fallback to semi-lagrangian if value is out of bounds.
			if(newFlowVel[x+y*w].x > 1.f || newFlowVel[x+y*w].x < -1.f || newFlowVel[x+y*w].y > 1.f || newFlowVel[x+y*w].x < -1.f){
					
				//Get velocity vector
				const float dX = (flow[x+y*w].x)*dTime;
				const float dY = (flow[x+y*w].y)*dTime;

				const int xdx = (int)((float)x-dX);
				const int ydy = (int)((float)y-dY);

				//Sample the four points around the target cell and interpolate between them
				vec2f_t f1 = flow[(xdx)  +(ydy)*w];
				vec2f_t f2 = flow[(xdx+1)+(ydy)*w];
				vec2f_t f3 = flow[(xdx)  +(ydy+1)*w];
				vec2f_t f4 = flow[(xdx+1)+(ydy+1)*w];


				float tX = ((float)x-dX) - (float)xdx;
				float tY = ((float)y-dY) - (float)ydy;

				newFlowVel[(x)+(y)*w].x = blerp(f1.x,f2.x,f3.x,f4.x,tX,tY);
				newFlowVel[(x)+(y)*w].y = blerp(f1.y,f2.y,f3.y,f4.y,tX,tY);
			}
		}
	}



	//Update height
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{

			float depthLeft  = (flow[(x-1)+(y)*w].x >= 0) ? (depth[(x-1)+(  y)*w]) : (depth[(x)+(y)*w]);
			float depthRight = (flow[(x)+(y)*w].x   <= 0) ? (depth[(x+1)+(  y)*w]) : (depth[(x)+(y)*w]);
			float depthTop   = (flow[(x)+(y-1)*w].y >= 0) ? (depth[(  x)+(y-1)*w]) : (depth[(x)+(y)*w]);
			float depthDown  = (flow[(x)+(y)*w].y   <= 0) ? (depth[(  x)+(y+1)*w]) : (depth[(x)+(y)*w]);


			//Overshooting reduction
			float beta = 2.f;
			float heightAvgMax = beta / (g * dTime);
			float heightAdjustment = maxf(0.f, ((depth[(x-1)+(  y)*w] + depth[(x+1)+(  y)*w] + depth[(  x)+(y-1)*w] + depth[(  x)+(y+1)*w]) / 4.f) - heightAvgMax);
			depthLeft  -= heightAdjustment;
			depthRight -= heightAdjustment;
			depthTop   -= heightAdjustment;
			depthDown  -= heightAdjustment;

			float dH = -( (depthRight*flow[(x)+(y)*w].x - depthLeft*flow[(x-1)+(y)*w].x) + (depthDown*flow[(x)+(y)*w].y - depthTop*flow[(x)+(y-1)*w].y) );

			depthBuffer[(x)+(y)*w] = depth[(x)+(y)*w] + (dH) * dTime;

			if(depthBuffer[(x)+(y)*w] < 0.f){
				depthBuffer[(x)+(y)*w] = 0.f;
			}

			
		}

	}

	for (int y = 0; y < h - 0; y++)
	{
		for (int x = 0; x < w - 0; x++)
		{
			depth[(x)+(y)*w] = depthBuffer[(x)+(y)*w];
		}
	}

	//Update flow
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{
			flow[(x)+(y)*w].x += (-g) * dTime * (depth[(x+1)+(y)*w] + terrain[(x+1)+(y)*w] - depth[(x)+(y)*w] - terrain[(x)+(y)*w]); 
			flow[(x)+(y)*w].y += (-g) * dTime * (depth[(x)+(y+1)*w] + terrain[(x)+(y+1)*w] - depth[(x)+(y)*w] - terrain[(x)+(y)*w]); 

			// 2.1.4 Boundary conditions
			if( ( (depth[(  x)+(y)*w] <= 0.0001f) && ( terrain[(  x)+(y)*w] > (terrain[(x+1)+(y)*w]+depth[(x+1)+(y)*w]) ) ) ||
				( (depth[(x+1)+(y)*w] <= 0.0001f) && ( terrain[(x+1)+(y)*w] > (terrain[(  x)+(y)*w]+depth[(  x)+(y)*w]) ) ) )
			{
				flow[x+y*w].x = 0.f;
			}

			if( ( (depth[(x)+(  y)*w] <= 0.0001f) && ( terrain[(x)+(  y)*w] > (terrain[(x)+(y+1)*w]+depth[(x)+(y+1)*w]) ) ) ||
				( (depth[(x)+(y+1)*w] <= 0.0001f) && ( terrain[(x)+(y+1)*w] > (terrain[(x)+(  y)*w]+depth[(x)+(  y)*w]) ) ) )
			{
				flow[x+y*w].y = 0.f;
			}

			//Normalize and scale down velocity vector
			//Normalize given vector
			float length = sqrtf(flow[(x)+(y)*w].x * flow[(x)+(y)*w].x + flow[(x)+(y)*w].y * flow[(x)+(y)*w].y);
			if(length > 0.f){
				float alpha = 0.5f;
				flow[(x)+(y)*w].x /= length;
				flow[(x)+(y)*w].y /= length;
				length = minf(length, alpha / dTime);
				flow[(x)+(y)*w].x *= length;
				flow[(x)+(y)*w].y *= length;
			}
		}
	}


	


}

void simFluidBackup(fluid_t* restrict fluid, float* restrict terrain, const float g, float visc, const float l, const int w, const int h, const float friction, const float dTime)
{

	const float A = l*l; //Cross sectional area of pipe
	fluid_t* restrict f = fluid; // shorter name
	float* restrict t = terrain; // shorter name
	const float v = visc;
	float friction_dTime = 1.f - dTime * (1.f - friction);

	// if(visc == 0.f){
		for (int y = 1; y < h - 1; y++)
		{
			const int yw = y * w;
			for (int x = 1; x < w - 1; x++)
			{
				// if((f->depth[(x) + (y)*w]) > 0.01f){
					f[x + yw].right = maxf(f[x + yw].right * friction_dTime + (f[x + yw].depth + t[x + yw] - f[(x + 1) + (yw)].depth    - t[(x + 1) + yw]      ) * dTime * A * g / l, 0.f);					   
					f[x + yw].down  = maxf(f[x + yw].down  * friction_dTime + (f[x + yw].depth + t[x + yw] - f[(x) + (y + 1) * w].depth - t[(x) + (y + 1) * w] ) * dTime * A * g / l, 0.f); 
					f[x + yw].left  = maxf(f[x + yw].left  * friction_dTime + (f[x + yw].depth + t[x + yw] - f[(x - 1) + (yw)].depth    - t[(x - 1) + yw]      ) * dTime * A * g / l, 0.f);						   
					f[x + yw].up    = maxf(f[x + yw].up    * friction_dTime + (f[x + yw].depth + t[x + yw] - f[(x) + (y - 1) * w].depth - t[(x) + (y - 1) * w] ) * dTime * A * g / l, 0.f);	  

				// }else{
				// 	f[x + yw].right = 0;
				// 	f[x + yw].down = 0;
				// 	f[x + yw].left = 0;
				// 	f[x + yw].up = 0;
				// }
			}
		}
	// }else{
	// 	for (int y = 0; y < h - 0; y++)
	// 	{
	// 		const int yw = y * w;
	// 		for (int x = 0; x < w - 0; x++)
	// 		{
	// 			if((f->depth[(x) + (y)*w]) > 0.01f){
	// 				f[x + yw].right = maxf(f[x + yw].right  + (f->depth[x + yw] + t[x + yw] - f->depth[(x + 1) + (yw)]    - t[(x + 1) + yw]      ) * dTime * A * g / l, 0.f);					   
	// 				f[x + yw].down  = maxf(f[x + yw].down   + (f->depth[x + yw] + t[x + yw] - f->depth[(x) + (y + 1) * w] - t[(x) + (y + 1) * w] ) * dTime * A * g / l, 0.f); 
	// 				f[x + yw].left  = maxf(f[x + yw].left   + (f->depth[x + yw] + t[x + yw] - f->depth[(x - 1) + (yw)]    - t[(x - 1) + yw]      ) * dTime * A * g / l, 0.f);						   
	// 				f[x + yw].up    = maxf(f[x + yw].up     + (f->depth[x + yw] + t[x + yw] - f->depth[(x) + (y - 1) * w] - t[(x) + (y - 1) * w] ) * dTime * A * g / l, 0.f);	  

	// 				float d = f->depth[x + yw];
	// 				float V = (d*d) / ((d*d) + 3.f * v * dTime);
	// 				f[x + yw].right *= V;
	// 				f[x + yw].down  *= V;
	// 				f[x + yw].left  *= V;
	// 				f[x + yw].up    *= V;

	// 			}else{
	// 				f[x + y * w].right = 0;
	// 				f[x + y * w].down = 0;
	// 				f[x + y * w].left = 0;
	// 				f[x + y * w].up = 0;
	// 			}
	// 		}
	// 	}
	// }


	for (int i = 0; i < w*h; i++){
		// make sure flow out of cell isn't greater than inflow + existing fluid
		if (f[i].depth - (f[i].right + f[i].down + f[i].left + f[i].up) < 0)
		{
			float K = minf((f[i].depth * l * l) / ((f[i].right + f[i].down + f[i].left + f[i].up) * dTime), 1.0f);
			f[i].right *= K;
			f[i].down  *= K;
			f[i].left  *= K;
			f[i].up    *= K;
		}
	}

	// update depth
	for (int y = 1; y < h - 1; y++)
	{
		for (int x = 1; x < w - 1; x++)
		{
			float deltaV = (f[(x - 1) + (y)*w].right + f[(x) + (y + 1) * w].up + f[(x + 1) + (y)*w].left + f[(x) + (y - 1) * w].down - (f[(x) + (y)*w].right + f[(x) + (y)*w].down + f[(x) + (y)*w].left + f[(x) + (y)*w].up)) * dTime;

			f[(x) + (y)*w].depth = maxf(f[(x) + (y)*w].depth + deltaV / (l * l), 0.f);


		}
	}



}

#include "../dependencies/include/glad/glad.h"

void simFluidGPU(new_fluid_t* fluid, float* terrain, const float g, float visc,  const float l, const int w, const int h, const float friction, const float dTime){
	
	static Shader shader = {
		.state = eSHADERSTATE_UNINITIALIZED,
		.vertexShaderSource = "src/shaders/shader.vert",
		.fragmentShaderSource = "src/shaders/fluidShader.frag"
	};
	static uint32_t texture_depth;
	static uint32_t texture_flow;
	static uint32_t texture_vel;
	static uint32_t texture_terrain;
	static uint32_t texture_depth_out;
	static uint32_t texture_flow_out;
	static uint32_t texture_vel_out;

	static new_fluid_t fluidBuffer;

	static GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

	switch(shader.state){
		case eSHADERSTATE_UNINITIALIZED:

			fluidBuffer.depth = malloc(w * h * sizeof(float));
			fluidBuffer.flow = malloc(w * h * sizeof(Fluid_flow));
			fluidBuffer.vel = malloc(w * h * sizeof(vec2f_t));

			shader.width = w;
			shader.height = h;


			shader.program =  compileShaderProgram(shader.vertexShaderSource, shader.fragmentShaderSource);
			if(shader.program == 0){
				shader.state = eSHADERSTATE_FAILED;
				return;
			}


			//Initalize and load textures
			glGenTextures(1, &(texture_depth)); //float
			glBindTexture(GL_TEXTURE_2D, texture_depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, fluid->depth);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			
			glGenTextures(1, &(texture_flow)); //4x float
			glBindTexture(GL_TEXTURE_2D, texture_flow);	
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, fluid->flow);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			
			glGenTextures(1, &(texture_terrain)); //float
			glBindTexture(GL_TEXTURE_2D, texture_terrain);	
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, terrain);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			





			glGenFramebuffers(1, &(shader.fbo));
			glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);

			//Output textures
			glGenTextures(1, &(texture_depth_out)); //float
			glBindTexture(GL_TEXTURE_2D, texture_depth_out);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, fluidBuffer.depth);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			
			glGenTextures(1, &(texture_flow_out)); //4x float
			glBindTexture(GL_TEXTURE_2D, texture_flow_out);	
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, fluidBuffer.flow);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			
			glGenTextures(1, &(texture_vel_out)); //2x float out
			glBindTexture(GL_TEXTURE_2D, texture_vel_out);	
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, w, h, 0, GL_RG, GL_FLOAT, fluidBuffer.vel); //NO data since output
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			

			// Attach textures to framebuffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_depth_out, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture_flow_out, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texture_vel_out, 0);

			// Set the list of draw buffers
			glDrawBuffers(3, drawBuffers);

			// Check framebuffer status
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				printf("framebuffer not complete");
			}




			//Define vertices and indices for a fullscreen quad
			float vertices[] = {
				1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
				1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
				-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
				-1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
			};

			unsigned int indices[] = {  // note that we start from 0!
				0, 1, 3,   // first triangle
				1, 2, 3    // second triangle
			};  

			glGenBuffers(1, &(shader.EBO));
			glGenVertexArrays(1, &(shader.VAO));
			glGenBuffers(1, &(shader.VBO));  
			
			//Bind vertex array
			glBindVertexArray(shader.VAO);
			
			//Bind vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, shader.VBO);
			//Copy verticies into VBO
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			//Bind element buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shader.EBO);
			//Copy indices into EBO
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


			//Specify the input in the vertex shader
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0 * sizeof(float)));


			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0); 
			glBindVertexArray(0); 


			shader.state = eSHADERSTATE_INITIALIZED;
			break;
			__attribute__((fallthrough));
		case eSHADERSTATE_INITIALIZED:
		{
	
			//Select shader
			glUseProgram(shader.program);

			//Select framebuffer to render to
			glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);

			// Set the list of draw buffers
			glDrawBuffers(3, drawBuffers);

		    glViewport(0, 0, shader.width, shader.height); //Set width and height of render target (Should be equal to outputData dimensions)

			//Not sure if clearing is needed
			// glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			// glClear(GL_COLOR_BUFFER_BIT);

			//Select the vertex array containing the fullscreen quad
			glBindVertexArray(shader.VAO); 

			glUniform1f(glGetUniformLocation(shader.program, "g"), g);
			glUniform1f(glGetUniformLocation(shader.program, "visc"), visc);
			glUniform1f(glGetUniformLocation(shader.program, "l"), l);
			glUniform1i(glGetUniformLocation(shader.program, "w"), w);
			glUniform1i(glGetUniformLocation(shader.program, "h"), h);
			glUniform1f(glGetUniformLocation(shader.program, "friction"), friction);
			glUniform1f(glGetUniformLocation(shader.program, "dTime"), dTime);

for(int i=0;i<1;i++){


			
			// Bind depth texture to texture unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_depth);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader.width, shader.height, GL_RED, GL_FLOAT, fluid->depth);
			glUniform1i(glGetUniformLocation(shader.program, "depthTexture"), 0);

			// Bind flow texture to texture unit 1
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, texture_flow);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader.width, shader.height, GL_RGBA, GL_FLOAT, fluid->flow);
			glUniform1i(glGetUniformLocation(shader.program, "flowTexture"), 1);

			// Bind velocity texture to texture unit 2
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, texture_vel);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader.width, shader.height, GL_RG, GL_FLOAT, fluid->vel);
			glUniform1i(glGetUniformLocation(shader.program, "velocityTexture"), 2);

			// Bind terrain texture to texture unit 3
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, texture_terrain);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader.width, shader.height, GL_RED, GL_FLOAT, terrain);
			glUniform1i(glGetUniformLocation(shader.program, "terrainTexture"), 3);



			//Render to the fullsreen quad
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



			// Read pixels from the depth texture (COLOR_ATTACHMENT0)
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(0, 0, w, h, GL_RED, GL_FLOAT, fluidBuffer.depth);

			// Read pixels from the flow texture (COLOR_ATTACHMENT1)
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, fluidBuffer.flow);

			// Read pixels from the velocity texture (COLOR_ATTACHMENT2)
			glReadBuffer(GL_COLOR_ATTACHMENT2);
			glReadPixels(0, 0, w, h, GL_RG, GL_FLOAT, fluidBuffer.vel);


			//Switch pointers with buffer and fluid
			new_fluid_t tempFluid = *fluid;
			*fluid = fluidBuffer;
			fluidBuffer = tempFluid;
}

		// memcpy(fluid->depth, fluidBuffer.depth, sizeof(float) * w * h);
		// memcpy(fluid->flow, fluidBuffer.flow, sizeof(Fluid_flow) * w * h);
		// memcpy(fluid->vel, fluidBuffer.vel, sizeof(vec2f_t) * w * h);

		}	break;
		case eSHADERSTATE_FAILED:
		{

			//Check if any errors has occured
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				printf("OpenGL error: %X", error);
				shader.state = eSHADERSTATE_FAILED;
			}

			printf("Shader failed\n");
			exit(0);
		}	break;
	}

}