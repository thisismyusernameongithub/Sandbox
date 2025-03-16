#include <string.h> //memcpy
#include <stdlib.h> //malloc
#include <stdbool.h> //bool

#include "window.h"
#include "render.h"
#include "camera.h"
#include "globals.h"


argb_t* frameBuffer;
argb_t* background; // Stores background image that get copied to framebuffer at start of render


extern camera_t g_cam;


static void initRenderStuff();
static argb_t getTileColorMist(RenderMapBuffer* renderMapBuffer, int x, int y, int ys, vec2f_t upVec);
static void renderColumn(RenderMapBuffer* renderMapBuffer, int x, int yBot, int yTop, vec2f_t upVec, float xwt, float ywt, float dDxw, float dDyw, camera_t camera);
void drawUI(Layer layer);

#define RENDERER_CPU

#ifdef RENDERER_CPU

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

        ys = y - (renderMapBuffer->height[xwti + ywti * mapW]) * camZoomDivBySqrt2;  // offset y by terrain height (sqr(2) is to adjust for isometric projection)

        ys = ys * !(ys & 0x80000000);			// Non branching version of : ys = maxf(ys, 0);

        if (ys < ybuffer)
        {
            // get color at worldspace and draw at screenspace
            register argb_t argb; // store color of Pixel that will be drawn

            // draw mist if present
            // if (renderMapBuffer->mistDepth[xwti + ywti * mapW] > 0.f)    
            // {
            //     argb = getTileColorMist(renderMapBuffer, xwti, ywti, ys, upVec);
            // } 
            // else 
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

    // check what relative position map corners have
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

#else

#include "../dependencies/include/glad/glad.h"


void render(Layer layer, RenderMapBuffer* renderMapBuffer)
{
    static Shader shader = {
        .state = eSHADERSTATE_UNINITIALIZED,
        .vertexShaderSource = "src/shaders/isometric.vert",
        .fragmentShaderSource = "src/shaders/isometric.frag"
    };
    static uint32_t heightTexture;
    static uint32_t colorTexture;
    static uint32_t outputTexture;
    static float* heightData = NULL;
    static argb_t* colorData = NULL;

    extern camera_t g_cam;
    
    int mapW = renderMapBuffer->mapW;
    int mapH = renderMapBuffer->mapH;

    // First time this function is called, create resources
    static bool firstTime = true;
    if (firstTime) {
        initRenderStuff();
        firstTime = false;
    }

    switch(shader.state) {
        case eSHADERSTATE_UNINITIALIZED:
            // Allocate memory for textures
            heightData = malloc(mapW * mapH * sizeof(float));
            colorData = malloc(mapW * mapH * sizeof(argb_t));
            
            // Copy height and color data from renderMapBuffer
            for (int i = 0; i < mapW * mapH; i++) {
                heightData[i] = renderMapBuffer->height[i];
                colorData[i] = renderMapBuffer->argb[i];
            }
            
            shader.width = window.drawSize.w;
            shader.height = window.drawSize.h;
            
            // Compile shader program
            shader.program = compileShaderProgram(shader.vertexShaderSource, shader.fragmentShaderSource);
            if (shader.program == 0) {
                shader.state = eSHADERSTATE_FAILED;
                return;
            }
            
            // Create height texture
            glGenTextures(1, &heightTexture);
            glBindTexture(GL_TEXTURE_2D, heightTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mapW, mapH, 0, GL_RED, GL_FLOAT, heightData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // Create color texture
            glGenTextures(1, &colorTexture);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mapW, mapH, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // Create output texture
            glGenTextures(1, &outputTexture);
            glBindTexture(GL_TEXTURE_2D, outputTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, shader.width, shader.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            
            // Create framebuffer
            glGenFramebuffers(1, &(shader.fbo));
            glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
            
            // Create render buffer for depth and stencil
            glGenRenderbuffers(1, &(shader.rbo));
            glBindRenderbuffer(GL_RENDERBUFFER, shader.rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, shader.width, shader.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, shader.rbo);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                printf("Framebuffer not complete!\n");
                shader.state = eSHADERSTATE_FAILED;
                return;
            }
            
            // Define vertices and indices for a fullscreen quad
            float vertices[] = {
                // positions          // texture coords
                 1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
                 1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
                -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
                -1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
            };
            
            unsigned int indices[] = {  
                0, 1, 3, // first triangle
                1, 2, 3  // second triangle
            };  
            
            glGenVertexArrays(1, &(shader.VAO));
            glGenBuffers(1, &(shader.VBO));
            glGenBuffers(1, &(shader.EBO));
            
            glBindVertexArray(shader.VAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, shader.VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shader.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
            
            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            // texture coord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            
            shader.state = eSHADERSTATE_INITIALIZED;
            break;
            
        case eSHADERSTATE_INITIALIZED:
            // Update the texture data
            for (int i = 0; i < mapW * mapH; i++) {
                heightData[i] = renderMapBuffer->height[i];
                colorData[i] = renderMapBuffer->argb[i];
            }
            
            glBindTexture(GL_TEXTURE_2D, heightTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapW, mapH, GL_RED, GL_FLOAT, heightData);
            
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapW, mapH, GL_RGBA, GL_UNSIGNED_BYTE, colorData);
            
            // Render to framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, shader.fbo);
            glViewport(0, 0, shader.width, shader.height);
            
            // Clear the buffer
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // Use shader program
            glUseProgram(shader.program);
            
            // Set uniforms
            glUniform1i(glGetUniformLocation(shader.program, "mapWidth"), mapW);
            glUniform1i(glGetUniformLocation(shader.program, "mapHeight"), mapH);
            glUniform1f(glGetUniformLocation(shader.program, "cameraZoom"), g_cam.zoom);
            glUniform2f(glGetUniformLocation(shader.program, "cameraPos"), g_cam.x, g_cam.y);
            glUniform1f(glGetUniformLocation(shader.program, "cameraRot"), g_cam.rot);
            glUniform2f(glGetUniformLocation(shader.program, "screenSize"), (float)shader.width, (float)shader.height);
            
            // Add height scale uniform - adjust the value to control displacement intensity
            float heightScaleValue = 0.2f; // Adjust this value to control the effect
            glUniform1f(glGetUniformLocation(shader.program, "heightScale"), heightScaleValue);
            
            // Bind textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, heightTexture);
            glUniform1i(glGetUniformLocation(shader.program, "heightMap"), 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, colorTexture);
            glUniform1i(glGetUniformLocation(shader.program, "colorMap"), 1);
            
            // Draw quad
            glBindVertexArray(shader.VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            // Read the rendered image back to CPU
            argb_t* outputBuffer = malloc(shader.width * shader.height * sizeof(argb_t));
            glReadPixels(0, 0, shader.width, shader.height, GL_RGBA, GL_UNSIGNED_BYTE, outputBuffer);
            
            // Copy to the layer's framebuffer with Y-flip (OpenGL has bottom-left origin)
            for (int y = 0; y < shader.height; y++) {
                for (int x = 0; x < shader.width; x++) {
                    int srcIdx = x + (shader.height - 1 - y) * shader.width;
                    int destIdx = x + y * layer.w;
                    layer.frameBuffer[destIdx] = outputBuffer[srcIdx];
                }
            }
            
            free(outputBuffer);
            
            // Return to default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            break;
            
        case eSHADERSTATE_FAILED:
            printf("Shader failed to initialize! Falling back to simple rendering.\n");
            
            // Simple fallback rendering - just clear to blue
            for (int i = 0; i < layer.w * layer.h; i++) {
                layer.frameBuffer[i] = rgb(0, 0, 255);
            }
            
            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                printf("OpenGL error: %X\n", error);
            }
            break;
    }
    
    // Draw a dot in the middle of the screen
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

#endif