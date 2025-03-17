#include "camera.h"
#include "window.h"



vec2f_t world2screen(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);

	// Zoom
	vec2f_t zoomed;
	zoomed.x = x / camera.zoom;
	zoomed.y = y / camera.zoom;

	// Rotate
	vec2f_t rotated;
	rotated.x =  cosAlpha * zoomed.x + sinAlpha * zoomed.y;
	rotated.y = -sinAlpha * zoomed.x + cosAlpha * zoomed.y;

	// Translate
	vec2f_t translated;
	translated.x = rotated.x - camera.x;
	translated.y = rotated.y - camera.y;

	// Isometric projection
	vec2f_t isometric;
	isometric.x = (translated.x - translated.y) / sqrtf(2.f);
	isometric.y = (translated.x + translated.y) / sqrtf(6.f);

	vec2f_t retVal = isometric;

	return retVal;
}

vec2f_t screen2world(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);
	float sq2d2 = sqrtf(2.f) / 2.f;
	float sq6d2 = sqrtf(6.f) / 2.f;
	

	// Project to isometric
	vec2f_t isometric;
	isometric.x = sq2d2 * x + sq6d2 * y;
	isometric.y = sq6d2 * y - sq2d2 * x;
	
	// Translate
	vec2f_t translated;
	translated.x = isometric.x + camera.x;
	translated.y = isometric.y + camera.y;

	// Rotate
	vec2f_t rotated;
	rotated.x = cosAlpha * translated.x - sinAlpha * translated.y;
	rotated.y = sinAlpha * translated.x + cosAlpha * translated.y;
	
	// Scale
	vec2f_t scaled;
	scaled.x = rotated.x * camera.zoom;
	scaled.y = rotated.y * camera.zoom;

	vec2f_t retVal = scaled;

	return retVal;
}

void cam_rot(camera_t* camPtr, float angle)
{
	// Get the screen center point
	vec2f_t screenCenter = {window.drawSize.w / 2.f, window.drawSize.h / 2.f + 50};

    // Calculate the center of the screen in world coordinates
    vec2f_t pos1 = screen2world(screenCenter.x, screenCenter.y, *camPtr);

    // Apply the rotation
    camPtr->rot = fmod(camPtr->rot + angle, 2 * M_PI);
    if (camPtr->rot < 0.f)
	{
        camPtr->rot += 2 * M_PI;
	}

	// Return camera to original position, for some reason this has to be done multiple times for accuracy
	for (int i = 0; i < 10; i++) {

		vec2f_t pos2 = world2screen(pos1.x, pos1.y, *camPtr);
		
		vec2f_t deltapos = {pos2.x - screenCenter.x, pos2.y - screenCenter.y};
		
		cam_pan(camPtr, deltapos.x, deltapos.y);
	}
	

	// Determine the direction the camera is facing
	if (camPtr->rot < 45.f * M_PI / 180.f || camPtr->rot >= 315.f * M_PI / 180.f)
	{
		camPtr->direction = NW;
	}
	else if (camPtr->rot < 135.f * M_PI / 180.f)
	{
		camPtr->direction = NE;
	}
	else if (camPtr->rot < 225.f * M_PI / 180.f)
	{
		camPtr->direction = SE;
	}
	else if (camPtr->rot < 315.f * M_PI / 180.f)
	{
		camPtr->direction = SW;
	}
	
}

void cam_pan(camera_t* camPtr, float x, float y)
{
	float limMinX = camPtr->limits.xMin; 
	float limMaxX = camPtr->limits.xMax;
	float limMinY = camPtr->limits.yMin;
	float limMaxY = camPtr->limits.yMax;

	float angle = (M_PI * 3.f / 4.f); //Some offset to make the camera move in the right direction
	
	float deltaX = cosf(angle) * x - sinf(angle) * y;
	float deltaY = sinf(angle) * x + cosf(angle) * y;

	camPtr->x -= deltaX;
	camPtr->y -= deltaY;


	// Apply limits
	// camPtr->x = clampf(camPtr->x, limMinX, limMaxX);
	// camPtr->y = clampf(camPtr->y, limMinY, limMaxY);
}

void cam_zoom(camera_t* camPtr, float value){
	// Get the screen center point
	vec2f_t screenCenter = {window.drawSize.w / 2.f, window.drawSize.h / 2.f};

	// Get current world position of center of screen
	vec2f_t pos1 = screen2world(screenCenter.x, screenCenter.y, *camPtr);

	//Apply zoom, this will shift the camera position
	camPtr->zoom += value * camPtr->zoom;
	camPtr->zoom = clampf(camPtr->zoom, camPtr->limits.zoomMin, camPtr->limits.zoomMax);

	// Return camera to original position, for some reason this has to be done multiple times for accuracy
	for (int i = 0; i < 10; i++) {

		vec2f_t pos2 = world2screen(pos1.x, pos1.y, *camPtr);
		
		vec2f_t deltapos = {screenCenter.x - pos2.x, screenCenter.y - pos2.y};
		
		cam_pan(camPtr, -deltapos.x, -deltapos.y);
	}
}

