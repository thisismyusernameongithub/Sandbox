#include "camera.h"
#include "window.h"



vec2f_t world2screen(float x, float y, camera_t camera)
{
	float sinAlpha = sinf(camera.rot);
	float cosAlpha = cosf(camera.rot);

	// scale for zoom level
	float xs = x / camera.zoom;
	float ys = y / camera.zoom;
	// project

	// rotate
	float xw = cosAlpha * (xs + (camera.x - 614.911f)) + sinAlpha * (ys + (camera.y - 119.936f)) - (camera.x - 614.911f);
	float yw = -sinAlpha * (xs + (camera.x - 614.911f)) + cosAlpha * (ys + (camera.y - 119.936f)) - (camera.y - 119.936f);
	// offset for camPtrera position

	xs = xw + camera.x;
	ys = yw + camera.y;

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
	// transform screen coordinates to world coordinates
	float xw = sq2d2 * (float)x + sq6d2 * (float)y;
	float yw = sq6d2 * (float)y - sq2d2 * (float)x;
	xw = xw - camera.x;
	yw = yw - camera.y;

	// rotate view
	float xwr = cosAlpha * (xw + (camera.x - 614.911f)) - sinAlpha * (yw + (camera.y - 119.936f)) - (camera.x - 614.911f);
	float ywr = sinAlpha * (xw + (camera.x - 614.911f)) + cosAlpha * (yw + (camera.y - 119.936f)) - (camera.y - 119.936f);
	// 616 121
	// apply zoom
	xw = xwr * camera.zoom;
	yw = ywr * camera.zoom;

	vec2f_t retVal = {xw, yw};
	// double xw = (cosAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*cam.zoom;
	// double yw = (sinAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*cam.zoom;
	return retVal;
}

void cam_zoom(camera_t* camPtr, float value){
    		// if (camPtr->zoom > camPtr->limits.zoomMin && camPtr->zoom < camPtr->limits.zoomMax){
			float rotation = camPtr->rot; // save camera rotation
			camPtr->rot = 0.f;				 // set camera rotation to 0

			vec2f_t pos1 = screen2world(window.drawSize.w / 2.f, window.drawSize.h / 2.f, *camPtr);
			camPtr->zoom += value * camPtr->zoom * window.time.dTime;
            camPtr->zoom = clampf(camPtr->zoom, camPtr->limits.zoomMin, camPtr->limits.zoomMax);
			vec2f_t pos2 = world2screen(pos1.x, pos1.y, *camPtr);
			vec2f_t deltapos = {window.drawSize.w / 2.f - pos2.x, window.drawSize.h / 2.f - pos2.y};
			// transform coordinate offset to isometric
			float sq2d2 = sqrtf(2.f) / 2.f;
			float sq6d2 = sqrtf(6.f) / 2.f;
			float xw = sq2d2 * deltapos.x + sq6d2 * deltapos.y;
			float yw = sq6d2 * deltapos.y - sq2d2 * deltapos.x;
			camPtr->y += yw;
			camPtr->x += xw;
			camPtr->rot = rotation; // restore camera rotation
		// }
}