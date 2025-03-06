#include "camera.h"
#include "window.h"



vec2f_t world2screen(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);

	// scale for zoom level
	float xs = x / camera.zoom;
	float ys = y / camera.zoom;
	
	// rotate
	float xw =  cosAlpha * (xs) + sinAlpha * (ys);
	float yw = -sinAlpha * (xs) + cosAlpha * (ys);
	
	// Translate camera position
	xs = xw + camera.x;
	ys = yw + camera.y;
	
	// Project to isometric
	xw = ((xs - ys) / sqrtf(2.f));
	yw = ((xs + ys) / sqrtf(6.f));

	vec2f_t retVal = {xw, yw};

	return retVal;
}

vec2f_t screen2world(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);
	float sq2d2 = sqrtf(2.f) / 2.f;
	float sq6d2 = sqrtf(6.f) / 2.f;
	
	// Convert from isometric to world
	float xw = sq2d2 * x + sq6d2 * y;
	float yw = sq6d2 * y - sq2d2 * x;

	// Translate camera position
	xw = xw - camera.x;
	yw = yw - camera.y;

	// Rotate view
	float xwr = cosAlpha * xw - sinAlpha * yw;
	float ywr = sinAlpha * xw + cosAlpha * yw;
	
	// Apply zoom
	xw = xwr * camera.zoom;
	yw = ywr * camera.zoom;

	vec2f_t retVal = {xw, yw};

	return retVal;
}

void cam_rot(camera_t* camPtr, float angle)
{
    // Calculate the center of the screen in world coordinates
    vec2f_t pos1 = screen2world(window.drawSize.w / 2, window.drawSize.h / 2, *camPtr);

    // Translate the camera to the origin
    // camPtr->x -= pos1.x;
    // camPtr->y -= pos1.y;
	// cam_pan(camPtr, -pos1.x, -pos1.y);

    // Apply the rotation
    camPtr->rot = fmod(camPtr->rot + angle, 2 * M_PI);
    if (camPtr->rot < 0.f)
        camPtr->rot += 2 * M_PI;

    // Translate the camera back to its original position
    // camPtr->x += pos1.x;
    // camPtr->y += pos1.y;
	// cam_pan(camPtr, pos1.x, pos1.y);

	//Return camera to original position
	vec2f_t pos2 = world2screen(pos1.x, pos1.y, *camPtr);

	vec2f_t deltapos = {(window.drawSize.w / 2.f) - pos2.x, (window.drawSize.h / 2.f) - pos2.y};

	cam_pan(camPtr, -deltapos.x, -deltapos.y);

	// Determine the direction the camera is facing
	if (fabsf(camPtr->rot) < 45.f * M_PI / 180.f || fabsf(camPtr->rot) >= 315.f * M_PI / 180.f)
	{
		camPtr->direction = NW;
	}
	else if (fabsf(camPtr->rot) < 135.f * M_PI / 180.f)
	{
		camPtr->direction = NE;
	}
	else if (fabsf(camPtr->rot) < 225.f * M_PI / 180.f)
	{
		camPtr->direction = SE;
	}
	else if (fabsf(camPtr->rot) < 315.f * M_PI / 180.f)
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

	camPtr->x += deltaX;
	camPtr->y += deltaY;

	// Apply limits
	// camPtr->x = clampf(camPtr->x, limMinX, limMaxX);
	// camPtr->y = clampf(camPtr->y, limMinY, limMaxY);
}

void cam_zoom(camera_t* camPtr, float value){
	// float rotation = camPtr->rot; // save camera rotation
	// camPtr->rot = 0.f;				 // set camera rotation to 0
	
	//Get current world position of center of screen
	vec2f_t pos1 = screen2world(window.drawSize.w / 2.f, window.drawSize.h / 2.f, *camPtr);

	//Apply zoom, this will shift the camera position
	camPtr->zoom += value * camPtr->zoom;
	camPtr->zoom = clampf(camPtr->zoom, camPtr->limits.zoomMin, camPtr->limits.zoomMax);

	//Return camera to original position
	vec2f_t pos2 = world2screen(pos1.x, pos1.y, *camPtr);

	vec2f_t deltapos = {(window.drawSize.w / 2.f) - pos2.x, (window.drawSize.h / 2.f) - pos2.y};

	cam_pan(camPtr, -deltapos.x, -deltapos.y);

	// transform coordinate offset to isometric
	// float sq2d2 = sqrtf(2.f) / 2.f;
	// float sq6d2 = sqrtf(6.f) / 2.f;
	// float xw = sq2d2 * deltapos.x + sq6d2 * deltapos.y;
	// float yw = sq6d2 * deltapos.y - sq2d2 * deltapos.x;
	// camPtr->y += yw;
	// camPtr->x += xw;
	// camPtr->rot = rotation; // restore camera rotation
}


// vec2f_t world2screen(float x, float y, camera_t camera)
// {
// 	float sinAlpha = sinf(camera.rot);
// 	float cosAlpha = cosf(camera.rot);

// 	// scale for zoom level
// 	float xs = x / camera.zoom;
// 	float ys = y / camera.zoom;
	
// 	// rotate
// 	float xw = cosAlpha * (xs + (camera.x - 614.911f)) + sinAlpha * (ys + (camera.y - 119.936f)) - (camera.x - 614.911f);
// 	float yw = -sinAlpha * (xs + (camera.x - 614.911f)) + cosAlpha * (ys + (camera.y - 119.936f)) - (camera.y - 119.936f);
	
// 	// Translate camera position
// 	xs = xw + camera.x;
// 	ys = yw + camera.y;
	
// 	// Project to isometric
// 	xw = ((xs - ys) / sqrtf(2.f));
// 	yw = ((xs + ys) / sqrtf(6.f));

// 	vec2f_t retVal = {xw, yw};

// 	return retVal;
// }

// vec2f_t screen2world(float x, float y, camera_t camera)
// {
// 	float sinAlpha = sinf(camera.rot);
// 	float cosAlpha = cosf(camera.rot);
// 	float sq2d2 = sqrtf(2.f) / 2.f;
// 	float sq6d2 = sqrtf(6.f) / 2.f;
	
// 	// Transform screen coordinates to world coordinates
// 	float xw = sq2d2 * (float)x + sq6d2 * (float)y;
// 	float yw = sq6d2 * (float)y - sq2d2 * (float)x;
// 	xw = xw - camera.x;
// 	yw = yw - camera.y;

// 	// Rotate view
// 	float xwr = cosAlpha * (xw + (camera.x - 614.911f)) - sinAlpha * (yw + (camera.y - 119.936f)) - (camera.x - 614.911f);
// 	float ywr = sinAlpha * (xw + (camera.x - 614.911f)) + cosAlpha * (yw + (camera.y - 119.936f)) - (camera.y - 119.936f);
// 	// 616 121
	
// 	// Apply zoom
// 	xw = xwr * camera.zoom;
// 	yw = ywr * camera.zoom;

// 	vec2f_t retVal = {xw, yw};
// 	// double xw = (cosAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*cam.zoom;
// 	// double yw = (sinAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*cam.zoom;
// 	return retVal;
// }
