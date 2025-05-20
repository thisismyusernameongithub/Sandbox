#ifndef CAMERA_H
#define CAMERA_H

#include "window.h"

typedef struct{
    float x, y;
	float rot; // 0-2*PI
	float zoom;
    struct{
        float zoomMax;
        float zoomMin;
		float xMax;
		float xMin;
		float yMax;
		float yMin;
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

// Convert 3D world coordinates to screen coordinates
vec2f_t world2screen3D(float x, float y, float z, camera_t camera);

// Convert screen coordinates to world coordinates
vec2f_t screen2world(float x, float y, camera_t camera);

// Zoom the camera by the given value
void cam_zoom(camera_t* camPtr, float value);

// Pan the camera by the given x and y values
void cam_pan(camera_t* camPtr, float x, float y);

// Rotate the camera by the given angle
void cam_rot(camera_t* camPtr, float angle);

#endif // CAMERA_H