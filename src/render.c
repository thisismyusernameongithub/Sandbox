#include <string.h> //memcpy
#include <stdlib.h> //malloc
#include <stdbool.h> //bool

#include "window.h"
#include "render.h"
#include "camera.h"
#include "globals.h"


argb_t* frameBuffer;
argb_t* background; // Stores background image that get copied to framebuffer at start of render





static void initRenderStuff();
static argb_t getTileColorMist(RenderMapBuffer* renderMapBuffer, int x, int y, int ys, vec2f_t upVec);
static void renderColumn(RenderMapBuffer* renderMapBuffer, int x, int yBot, int yTop, vec2f_t upVec, float xwt, float ywt, float dDxw, float dDyw, camera_t camera);
void drawUI(Layer layer);

argb_t getTileColorMist(RenderMapBuffer* renderMapBuffer, int x, int y, int ys, vec2f_t upVec)
{
    argb_t argb = rgb(255,0,0);
    int mapW = renderMapBuffer->mapW;
    int mapH = renderMapBuffer->mapH;

    float mistHeight =  renderMapBuffer->height[x+y*mapW] + renderMapBuffer->mistDepth[x+y*mapW];

    float d;
    for(d = 0.f; d < 300.f; d += 1.f){
        int X = x + upVec.x * d;
        int Y = y + upVec.y * d;
        if(X >= 0 && X < mapW && Y >= 0 && Y < mapH){
            if(renderMapBuffer->height[(int)(x+upVec.x*d)+(int)(y+upVec.y*d)*mapW] > mistHeight - (d * 0.79f)){ //0.7071f = 1/sqrt(2)

                argb = renderMapBuffer->argb[X+Y*mapW];

                argb = lerpargb(argb, renderMapBuffer->argbBlured[X+Y*mapW], minf(d/10.f, 1.f));

                break;
            }
        }else{
            
            //If the mist is up against the wall, sample the background picture to make it appear transparent
            //I don't know why the coordinates are like this, I just tried stuff until it worked....
            argb.r = background[window.drawSize.h - ys].r; //(102+(int)ys)>>2;//67
            argb.g = background[window.drawSize.h - ys].g; //(192+(int)ys)>>2;//157
            argb.b = background[window.drawSize.h - ys].b; //(229+(int)ys)>>2;//197

            break;

        }
    }

    
    // vec2f_t a = world2screen(upVec.x, upVec.y, g_cam);
    // vec2f_t b = world2screen(upVec.x + 1.f, upVec.y + 1.f, g_cam);
    // printf("%f %f %f %f %f\n",a.x, a.y, b.x, b.y, b.y-a.y);


    argb = lerpargb(argb, pallete.mist, minf(d/50.f, 1.f));



    // argb.r = lerp(argb.r, pallete.white.r, mistDensity);
    // argb.g = lerp(argb.g, pallete.white.g, mistDensity);
    // argb.b = lerp(argb.b, pallete.white.b, mistDensity);

    return argb;
    
}



// renders one pixel column
static void renderColumn(RenderMapBuffer* renderMapBuffer, int x, int yBot, int yTop, vec2f_t upVec, float xwt, float ywt, float dDxw, float dDyw, camera_t camera)
{
    int mapW = renderMapBuffer->mapW;
    int mapH = renderMapBuffer->mapH;

    
    float camZoom = camera.zoom;
    float camZoomDiv = 1.f / camera.zoom;
    float camZoomDivBySqrt2 = (camZoomDiv) /  sqrtf(2.f);

    // init some variables

    int border = false; // used when skipping pixels to decide when at a edge of a tile that should be a darker shade
    int dPixels;		// how many pixels are skipped to get to the next tile

    // init ybuffer with lowest mappoint at current x screen position
    // ybuffer stores the last drawn lowest position so there is no overdraw

    int ybuffer = minf(yBot, window.drawSize.h); // Limit lowest drawing point (ybuffer start value) to edge of screen

    // start drawing column
    for (int y = yBot; y > yTop; y -= dPixels)
    {
        int ys;

        int ywti = (int)ywt;
        int xwti = (int)xwt;

        ys = y - (renderMapBuffer->height[xwti + ywti * mapW] + renderMapBuffer->mistDepth[xwti + ywti * mapW]) * camZoomDivBySqrt2;  // offset y by terrain height (sqr(2) is to adjust for isometric projection)

        ys = ys * !(ys & 0x80000000);			// Non branching version of : ys = maxf(ys, 0);

        if (ys < ybuffer)
        {
            // get color at worldspace and draw at screenspace
            register argb_t argb; // store color of Pixel that will be drawn

            // draw mist if present
            if (renderMapBuffer->mistDepth[xwti + ywti * mapW] > 0.f)    
            {
                argb = getTileColorMist(renderMapBuffer, xwti, ywti, ys, upVec);
            } 
            else 
            {
                argb = renderMapBuffer->argb[xwti + ywti * mapW];
            }
            

            // make borders of tiles darker, make it so they become darker the more zoomed in you are
            if (camZoom < 0.3f && !border)
            {
                argb = lerpargb(argb, pallete.black, camZoomDiv / 255.f);
            }

            // only draw visible pixels
            for (register int Y = ybuffer - 1; Y >= ys; Y--)
            {
                frameBuffer[window.drawSize.h - 1 - Y + (x)*window.drawSize.h] = argb; // draw pixels
            }
            

            ybuffer = ys; // save current highest point in pixel column
        }

        // this piece of code calculates how many y pixels (dPixels) there is to the next tile
        // and the border thing makes it so it only jumps one pixel when there is a
        // new tile and the next drawing part is darker, thus making the edge of the tile darker
        if (!border)
        {
            border = 1;
            float testX, testY;
            switch (camera.direction)
            {
            case NW:
                testX = ((xwt - (int)xwt)) / dDxw;
                testY = ((ywt - (int)ywt)) / dDyw;
                break;
            case NE:
                testX = ((1 - (xwt - (int)xwt)) / dDxw);
                testY = (((ywt - (int)ywt)) / dDyw);
                break;
            case SE:
                testX = (1 - (xwt - (int)xwt)) / dDxw;
                testY = (1 - (ywt - (int)ywt)) / dDyw;
                break;
            case SW:
                testX = (((xwt - (int)xwt)) / dDxw);
                testY = ((1 - (ywt - (int)ywt)) / dDyw);
                break;
            default:
                fprintf(stderr, "Unitialized value used at %s %d\n", __FILE__, __LINE__);
            }
            dPixels = minf(fabs(testX), fabs(testY));
            if (dPixels < 1)
                dPixels = 1;
            xwt += dDxw * dPixels;
            ywt += dDyw * dPixels;
        }
        else
        {

            border = 0;
            xwt += dDxw;
            ywt += dDyw;
            dPixels = 1;
        }
    }
}

void drawUI(Layer layer)
{
    clearLayer(layer);

    // drawText(layer, 10, 10, printfLocal("fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime));

    
#ifdef DEBUG
int cwx = clampf(cursor.worldX, 0, mapW);
int cwy = clampf(cursor.worldY, 0, mapH);
// drawText(layer, 10, 30,  printfLocal("Mouse: %d, %d | %d, %d | zoom: %f", mouse.pos.x, mouse.pos.y, cwx, cwy, g_cam.zoom) );
// drawText(layer, 10, 50,  printfLocal("Shadow: %f", map.shadow[cwx+cwy*mapW]));
// drawText(layer, 10, 70,  printfLocal("foam: %f total: %f", map.foamLevel[cwx+cwy*mapW], totalFoamLevel));
// drawText(layer, 10, 90,  printfLocal("Stone: %f total: %f", map.stone[cwx+cwy*mapW], totalStoneLevel));
// drawText(layer, 10, 110,  printfLocal("Sand: %f total: %f", map.sand[cwx+cwy*mapW], totalSandLevel));
// drawText(layer, 10, 130, printfLocal("Water: %f total: %f", mapWater[cwx+cwy*mapW].depth, totalWaterLevel));
// drawText(layer, 10, 150, printfLocal("Mist: %f total: %f", map.mist[cwx+cwy*mapW].depth, totalMistLevel));
// drawText(layer, 10, 170,  printfLocal("waterVel: %f, %f", mapWaterVel[cwx+cwy*mapW].x, mapWaterVel[cwx+cwy*mapW].y));

//Draw water is present data
// for (int y = 0; y < mapH; y++)
// {
// 	for (int x = 0; x < mapW; x++)
// 	{
//         if(map.present[x+y*mapW].water){
//             vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
//             sPos.y = sPos.y - (mapHeight[x+y*mapW] + mapWater[x+y*mapW].depth)   / (sqrtf(2.f) * g_cam.zoom);
//             drawPoint(layer, sPos.x, sPos.y, pallete.red);
//         }
//     }
// }

//Draw water velocity markers
// for (int y = 0; y < mapH; y++)
// {
// 	if((y % 10)) continue;
// 	for (int x = 0; x < mapW; x++)
// 	{
// 		if((x % 10)) continue;
// 		float x2 = x + mapWaterVel[x + y * mapW].x;
// 		float y2 = y + mapWaterVel[x + y * mapW].y;
// 		vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
// 		vec2f_t sPos2 = world2screen((float)x2+0.5f,(float)y2+0.5f,g_cam);
// 		sPos.y = sPos.y - (mapHeight[x+y*mapW] + mapWater[x+y*mapW].depth)   / (sqrtf(2.f) * g_cam.zoom);
// 		sPos2.y = sPos2.y - (mapHeight[x+y*mapW] + mapWater[x+y*mapW].depth)   / (sqrtf(2.f) * g_cam.zoom);
// 		float vel = sqrtf((mapWaterVel[x + y * mapW].x*mapWaterVel[x + y * mapW].x)+(mapWaterVel[x + y * mapW].y*mapWaterVel[x + y * mapW].y));
// 		drawLine(layer, sPos.x,sPos.y, sPos2.x, sPos2.y, pallete.white);
// 		drawPoint(layer, sPos.x, sPos.y, pallete.red);
// 	}
// }

//Draw mouse position
// for (int y = -1; y < 1; y++)
// {
// 	for (int x = -1; x < 1; x++)
// 	{
// 		if (mouse.pos.x > 1 && mouse.pos.x < window.drawSize.w - 1)
// 		{
// 			if (mouse.pos.y > 1 && mouse.pos.x < window.drawSize.h - 1)
// 			{
// 				layer.frameBuffer[(mouse.pos.x + x) + (mouse.pos.y + y) * layer.w].argb = 0xFFFFFFFF;
// 			}
// 		}
// 	}
// }

//Draw foamspawner
for(int i = 0; i < FOAMSPAWNER_MAX; i++){
    if(foamSpawner[i].amount > 0.f){
        int x = foamSpawner[i].pos.x;
        int y = foamSpawner[i].pos.y;
        vec2f_t sPos = world2screen((float)x+0.5f,(float)y+0.5f,g_cam);
        sPos.y = sPos.y - (mapHeight[x+y*mapW] + mapWater.depth[x+y*mapW])  / (sqrtf(2.f) * g_cam.zoom);
        drawSquare(layer, sPos.x - 1, sPos.y - 1, 2, 2, pallete.red);
    }
}


#endif

    // Draw mouse position
    for (int y = -1; y < 1; y++)
    {
        for (int x = -1; x < 1; x++)
        {
            if (mouse.pos.x > 1 && mouse.pos.x < window.drawSize.w - 1)
            {
                if (mouse.pos.y > 1 && mouse.pos.x < window.drawSize.h - 1)
                {
                    layer.frameBuffer[(mouse.pos.x + x) + (mouse.pos.y + y) * layer.w] = rgb(255,0,0);
                }
            }
        }
    }

    extern float totalWaterLevel;
    drawText(layer, 100, 130, printfLocal("Total amount of water: %f", totalWaterLevel));
    char* simStr = ((key.Y) ? "GPU" : "Processor");
    drawText(layer, 100, 160, printfLocal("Sim running on: %s", simStr));

}

void render(Layer layer, RenderMapBuffer* renderMapBuffer)
{

    //First time this function is called, create background framebuffer
    static int firstTime = 1;
    if (firstTime) {
        initRenderStuff();
        firstTime = 0;
    }

    clearLayer(layer);
    

    int mapW = renderMapBuffer->mapW;
    int mapH = renderMapBuffer->mapH;



    extern camera_t g_cam;
    // Since this runs on a separate thread from input update I need to back up camera variables so they don't change during rendering
    camera_t cam = g_cam;

    // Copy background to framebuffer
    memcpy(frameBuffer, background, window.drawSize.w * window.drawSize.h * sizeof(argb_t));


    float xw, yw;
    float xs, ys;

    // frustum
    xs = 0;
    ys = 0;
    vec2f_t ftl = screen2world(xs, ys, cam);
    xs = window.drawSize.w;
    ys = window.drawSize.h;
    vec2f_t fbr = screen2world(xs, ys, cam);
    xs = 0;
    ys = window.drawSize.h;
    vec2f_t fbl = screen2world(xs, ys, cam);
    xs = 0;
    ys = window.drawSize.h + 100.f / cam.zoom;

    float dDxw = (ftl.x - fbl.x) / (float)window.drawSize.h; // delta x worldspace depth
    float dDyw = (ftl.y - fbl.y) / (float)window.drawSize.h; // delta y worldspace depth

    // create normalized vector that point up on the screen but in world coorinates, for use with raytracing water refraction
    vec2f_t upVec = {ftl.x - fbl.x, ftl.y - fbl.y};
    float tempDistLongName = sqrtf(upVec.x * upVec.x + upVec.y * upVec.y);
    upVec.x /= tempDistLongName;
    upVec.y /= tempDistLongName;

    ///////// merge these calculations later
    // calculate screen coordinates of world corners
    vec2f_t tlw = world2screen(1, 1, cam);
    vec2f_t trw = world2screen(mapW, 1, cam);
    vec2f_t blw = world2screen(1, mapH, cam);
    vec2f_t brw = world2screen(mapW, mapH, cam);
    // check what relative postion map corners have
    vec2f_t mapCornerTop, mapCornerLeft, mapCornerBot, mapCornerRight;
    switch (cam.direction)
    {
    case NW:
        mapCornerTop = (vec2f_t){tlw.x, tlw.y};
        mapCornerLeft = (vec2f_t){blw.x, blw.y};
        mapCornerBot = (vec2f_t){brw.x, brw.y};
        mapCornerRight = (vec2f_t){trw.x, trw.y};
        break;
    case NE:
        mapCornerRight = (vec2f_t){brw.x, brw.y};
        mapCornerTop = (vec2f_t){trw.x, trw.y};
        mapCornerLeft = (vec2f_t){tlw.x, tlw.y};
        mapCornerBot = (vec2f_t){blw.x, blw.y};
        break;
    case SE:
        mapCornerBot = (vec2f_t){tlw.x, tlw.y};
        mapCornerRight = (vec2f_t){blw.x, blw.y};
        mapCornerTop = (vec2f_t){brw.x, brw.y};
        mapCornerLeft = (vec2f_t){trw.x, trw.y};
        break;
    case SW:
        mapCornerLeft = (vec2f_t){brw.x, brw.y};
        mapCornerBot = (vec2f_t){trw.x, trw.y};
        mapCornerRight = (vec2f_t){tlw.x, tlw.y};
        mapCornerTop = (vec2f_t){blw.x, blw.y};
        break;
    default:
        fprintf(stderr, "Unitialized value used at %s %d\n", __FILE__, __LINE__);
        break;
    }

    // calculate slope of tile edges on screen
    float tileEdgeSlopeRight = (float)(mapCornerRight.y - mapCornerBot.y) / (float)(mapCornerRight.x - mapCornerBot.x);
    float tileEdgeSlopeLeft = (float)(mapCornerBot.y - mapCornerLeft.y) / (float)(mapCornerBot.x - mapCornerLeft.x);
    /////////

    // these coordinates will be the bounds at which renderColumn() will render any terrain
    int leftMostXCoord = maxf((int)mapCornerLeft.x, 0);
    int rightMostXCoord = minf((int)mapCornerRight.x, window.drawSize.w);

    for (int x = leftMostXCoord; x < rightMostXCoord; x++)
    {
        int botMostYCoord;
        int topMostYCoord;

        if (x > mapCornerBot.x)
        {
            botMostYCoord = mapCornerBot.y + tileEdgeSlopeRight * (x - mapCornerBot.x);
        }
        else
        {
            botMostYCoord = mapCornerBot.y + tileEdgeSlopeLeft * (x - mapCornerBot.x);
        }
        if (x > mapCornerTop.x)
        {
            topMostYCoord = mapCornerTop.y + tileEdgeSlopeLeft * (x - mapCornerTop.x);
        }
        else
        {
            topMostYCoord = mapCornerTop.y + tileEdgeSlopeRight * (x - mapCornerTop.x);
        }

        topMostYCoord = topMostYCoord * !(topMostYCoord & 0x80000000); // Branchless topMostYCoord = maxf(topMostYCoord, 0);

        vec2f_t worldCoord = screen2world(x, botMostYCoord, cam);

        renderColumn(renderMapBuffer, x, botMostYCoord, topMostYCoord, upVec, worldCoord.x, worldCoord.y, dDxw, dDyw, cam);
    }

    // for memory access pattern reasons the terrain gets drawn sideways. That's why we below transpose the framebuffer while copying it to the framebuffer
    for (int y = 0; y < window.drawSize.h; y++)
    {
        int pixelPitch = y * window.drawSize.w;
        int bufferPitch = window.drawSize.h - 1 - y;
        for (int x = 0; x < window.drawSize.w; x++)
        {
            layer.frameBuffer[x + pixelPitch] = frameBuffer[bufferPitch + (x)*window.drawSize.w];
        }
    }

    //Draw a dot in the middle of the screen
    layer.frameBuffer[window.drawSize.w / 2 + (window.drawSize.h / 2) * window.drawSize.w] = rgb(255, 0, 0);

}




static void initRenderStuff()
{

    // Allocate memory needed
    frameBuffer = malloc(window.drawSize.w * window.drawSize.h * sizeof(argb_t));
    background = malloc(window.drawSize.w * window.drawSize.h * sizeof(argb_t));

    // Fill out background with colors
    for (int x = 0; x < window.drawSize.w; x++)
    {
        // argb_t argb = (argb_t){.b = ((219 + x) >> 2), .g = (((182 + x) >> 2)), .r = (((92 + x) >> 2))};
        argb_t argb = lerpargb(ARGB(255,25,47,56),ARGB(255,222,245,254),((float)x)/((float)window.drawSize.w)); //Gradient over rendersize
        for (int y = 0; y < window.drawSize.h; y++)
        {
            background[window.drawSize.w - 1 - x + y * window.drawSize.w] = argb;
        }
    }

}
