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

typedef struct{
	float right;
	float down;
	float left;
	float up;
}Fluid_flow;

typedef struct{
	float* depth;
	Fluid_flow* flow;
	vec2f_t* vel;
}new_fluid_t;

typedef struct{
	float* depth;
	vec2f_t* flow;
}fluidSWE_t;


void advect(float* densityMatrix, float* bufferMatrix, vec2f_t* velocityVector, int w, int h, float dTime);
void erodeAndDeposit(float* subject, float* suspendedSubject, float* terrain, fluid_t* medium, vec2f_t* mediumVel, int w, int h);
void relax(float* subject, float* terrain, float talusAngle, float g, int w, int h, float dTime);
void simFluid(fluid_t* restrict fluid, float* restrict terrain, const float g, float visc,  const float l, const int w, const int h, const float friction, const float dTime);
void simFluidGPU(new_fluid_t* fluid, float* terrain, const float g, float visc,  const float l, const int w, const int h, const float friction, const float dTime);
void simFluidBackup(fluid_t* restrict fluid, float* restrict terrain, const float g, float visc, const float l, const int w, const int h, const float friction, const float dTime);
void simFluidSWE(fluidSWE_t* fluid, float* terrain, float g, float visc, float l, int w, int h, float friction, float dTime);

#endif /* SIMULATION_H_ */ 