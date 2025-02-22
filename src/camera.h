#ifndef CAMERA_H
#define CAMERA_H

#include "window.h"

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

// Convert world coordinates to screen coordinates
vec2f_t world2screen(float x, float y, camera_t camera);

// Convert screen coordinates to world coordinates
vec2f_t screen2world(float x, float y, camera_t camera);

// Zoom the camera by the given value
void cam_zoom(camera_t* camPtr, float value);

#endif // CAMERA_H