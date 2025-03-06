#ifndef RENDER_H
#define RENDER_H

typedef struct{
    int mapW;
    int mapH;
    float* height;
	float* mistDepth;
	argb_t* argb;
	argb_t* argbBlured;
}RenderMapBuffer; //Stores stuff from map that will be used for rendering, frees up the map structure to be updated for next frame while current frame is rendering

void render(Layer layer, RenderMapBuffer* renderMapBuffer);
void drawUI(Layer layer);

#endif // RENDER_H