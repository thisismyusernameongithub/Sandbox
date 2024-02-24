#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "window.h"


typedef struct
{
	float right;
	float down;
	float left;
	float up;
	float depth;
} fluid_t;


void advect(float* densityMatrix, float* bufferMatrix, vec2f_t* velocityVector, int w, int h, float dTime);
void erodeAndDeposit(float* subject, float* suspendedSubject, float* terrain, fluid_t* medium, vec2f_t* mediumVel, int w, int h);
void relax(float* subject, float* terrain, float talusAngle, float g, int w, int h, float dTime);
void simFluid(fluid_t* restrict fluid, float* restrict terrain, const float g, float visc,  const float l, const int w, const int h, const float friction, const float dTime);

#endif /* SIMULATION_H_ */ 