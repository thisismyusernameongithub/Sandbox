#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <math.h> //clampf

#include "window.h"


// -lgdi32 -lSDL2_ttf -lSDL2 -lm -lSDL2_image

// Static variable
const Uint8 *keyboardState; // pointer to key states

// Static functions
static char *getMouseStateString(void);

#define WINDOW_DEFAULT_W 800
#define WINDOW_DEFAULT_H 600
#define WINDOW_DEFAULT_X 100
#define WINDOW_DEFAULT_Y 100
#define WINDOW_DEFAULT_TITLE "Default title"

// window default values
Window window = {
	.pos.x = WINDOW_DEFAULT_X,
	.pos.y = WINDOW_DEFAULT_Y,
	.size.w = WINDOW_DEFAULT_W,
	.size.h = WINDOW_DEFAULT_H,
	.drawSize.w = WINDOW_DEFAULT_W,
	.drawSize.h = WINDOW_DEFAULT_H,
	.title = WINDOW_DEFAULT_TITLE};

Window oldWindow; // Copy of window, used to compare against to detect changes in settings

#define IMAGE_MAX_W (4000)
#define IMAGE_MAX_H (4000)

typedef struct
{
	argb_t *color;
	int w;
	int h;
	int pitch;
} Image;

Image screen;
Image screenBlurred;

#define ON_RISING_EDGE(bit)                                            \
	({                                                                 \
		static int oldState = false;                                   \
		(bit != oldState) ? ((oldState = bit) ? true : false) : false; \
	})

char *printfLocal(const char *format, ...)
{
	static char printfLocalBuffer[200];

	va_list arg;

	// store formated input string in printBuffer
	va_start(arg, format);
	vsprintf(printfLocalBuffer, format, arg);
	va_end(arg);

	return (printfLocalBuffer);
}

// External variables
Mouse mouse;
Key key;

__attribute__((unused)) static char *getMouseStateString(void)
{
	static char string[200];
	sprintf(string, "MouseState: left %d, right %d, X %d, Y %d, dX %d, dY %d, dWheel %d", mouse.left, mouse.right, mouse.pos.x, mouse.pos.y, mouse.dPos.x, mouse.dPos.y, mouse.dWheel);
	return string;
}

typedef struct
{
	SDL_Texture *texture;
	void *pixels;
	uint32_t *frameBuffer;
	int layer;
	int w;
	int h;
	int pitch;
} sdlTexture;

SDL_Window *SDLwindow;
SDL_Renderer *SDLrenderer;

#define MAX_TEXTURES (3) // Max number of allowed textures
static struct
{
	int noTextures;
	sdlTexture textures[MAX_TEXTURES];
} textureStorage;

static float clampf(float value, float min, float max);
static void windowResized(void);
static void updateTime(void);
static sdlTexture createSDLTexture(int width, int height);
static void updateInput(void);
void drawText(Layer layer, int xPos, int yPos, char *string);

#define PI 3.14159265358979323846264338327950f
#define DEG2RAD(x) ((x) * (PI / 180.f))
#define RAD2DEG(x) ((x) * (180.f / PI))

#define errLog(message) \
	fprintf(stderr, "File: %s, Function: %s, Line: %d, Note: %s\n", __FILE__, __FUNCTION__, __LINE__, message);

Layer window_createLayer(void)
{
	static int noLayers;
	Layer layer;
	sdlTexture texture = createSDLTexture(window.drawSize.w, window.drawSize.h);
	layer.frameBuffer = (argb_t *)texture.frameBuffer;
	layer.h = texture.h;
	layer.w = texture.w;
	layer.pitch = texture.pitch;
	layer.depth = noLayers++;
	// Clear framebuffer
	for (int y = 0; y < layer.h; y++)
	{
		for (int x = 0; x < layer.w; x++)
		{
			layer.frameBuffer[x + y * layer.w] = (argb_t){.a = 150, .r = 255, .g = 0, .b = 255};
		}
	}

	clearLayer(layer);

	return layer;
}

static void windowResized(void)
{
	// Retrive window scale factor to apply it to mouse positions too:
	int width, height;
	SDL_GetWindowSize(SDLwindow, &width, &height);
	window.size.w = width;
	window.size.h = height;

	return;
}

static void windowMoved()
{
	SDL_GetWindowPosition(SDLwindow, &window.pos.x, &window.pos.y);
	oldWindow.pos.x = window.pos.x;
	oldWindow.pos.y = window.pos.y;
}

void window_setTitle(char *title)
{
	SDL_SetWindowTitle(SDLwindow, title);
	strcpy(window.title, title);
	return;
}

static void updateTime(void)
{
	static uint32_t nowTime = 0;
	static uint32_t oldTime_ms1 = 0;
	static uint32_t oldTime_ms10 = 0;
	static uint32_t oldTime_ms100 = 0;
	static uint32_t oldTime_s1 = 0;

	window.time.tick.ms1 = false;
	window.time.tick.ms10 = false;
	window.time.tick.ms100 = false;
	window.time.tick.s1 = false;

	nowTime = SDL_GetTicks();

	if (nowTime != oldTime_ms1)
	{
		window.time.ms1 += nowTime - oldTime_ms1;
		oldTime_ms1 = nowTime;
		window.time.tick.ms1 = true;
	}
	if (nowTime - oldTime_ms10 >= 10)
	{
		window.time.ms10 += (nowTime - oldTime_ms10) / 10;
		oldTime_ms10 = nowTime - nowTime % 10;
		window.time.tick.ms10 = true;
	}
	if (nowTime - oldTime_ms100 >= 100)
	{
		window.time.ms100 += (nowTime - oldTime_ms100) / 100;
		oldTime_ms100 = nowTime - nowTime % 100;
		window.time.tick.ms100 = true;
	}
	if (nowTime - oldTime_s1 >= 1000)
	{
		window.time.s1 += (nowTime - oldTime_s1) / 1000;
		oldTime_s1 = nowTime - nowTime % 1000;
		window.time.tick.s1 = true;
	}

	// Get dTime and fps with higher precision than 1ms ticks
	static uint64_t freq = 0;
	if (!freq)
		freq = SDL_GetPerformanceFrequency();
	static uint64_t oldTime = 0;
	uint64_t newTime = SDL_GetPerformanceCounter();
	window.time.dTime = (double)(newTime - oldTime) / (double)freq;
	oldTime = newTime;

	// fps
	static uint64_t frameCount = 0;
	static uint64_t frameCountOld = 0;
	frameCount++;
	if (window.time.tick.ms100)
	{
		window.time.fps = 0.1 * (double)(frameCount - frameCountOld) * 10.0 + 0.9 * window.time.fps;
		frameCountOld = frameCount;
	}
}

// Check if window settings has changes and handle whatever that change is
static void updateWindow(void)
{
	if (oldWindow.size.h != window.size.h || oldWindow.size.w != window.size.w)
	{
		oldWindow.size.w = window.size.w;
		oldWindow.size.h = window.size.h;
		SDL_SetWindowSize(SDLwindow, window.size.w, window.size.h);
		windowResized();
	}
	// TODO: implement reallocating buffers and stuff before allowing chane of drawSize
	if (oldWindow.drawSize.w != window.drawSize.w || oldWindow.drawSize.h != window.drawSize.h)
	{
		window.drawSize.w = oldWindow.drawSize.w;
		window.drawSize.h = oldWindow.drawSize.h;
	}
	// Borderless
	if (oldWindow.settings.borderLess != window.settings.borderLess)
	{
		oldWindow.settings.borderLess = window.settings.borderLess;
		SDL_bool bordered = window.settings.borderLess ? SDL_FALSE : SDL_TRUE;
		SDL_SetWindowBordered(SDLwindow, bordered);
	}
	// Resizable
	if (oldWindow.settings.resizable != window.settings.resizable)
	{
		oldWindow.settings.resizable = window.settings.resizable;
		SDL_bool resizable = window.settings.resizable ? SDL_TRUE : SDL_FALSE;
		SDL_SetWindowResizable(SDLwindow, resizable);
	}
	// alwaysOnTop
	if (oldWindow.settings.alwaysOnTop != window.settings.alwaysOnTop)
	{
		oldWindow.settings.alwaysOnTop = window.settings.alwaysOnTop;
		SDL_bool alwaysOnTop = window.settings.alwaysOnTop ? SDL_TRUE : SDL_FALSE;
		SDL_SetWindowAlwaysOnTop(SDLwindow, alwaysOnTop);
	}
	if (oldWindow.pos.x != window.pos.x || oldWindow.pos.y != window.pos.y)
	{
		oldWindow.pos.x = window.pos.x;
		oldWindow.pos.y = window.pos.y;
		SDL_SetWindowPosition(SDLwindow, window.pos.x, window.pos.y);
	}
}

static sdlTexture createSDLTexture(int width, int height)
{

	sdlTexture texture;
	texture.texture = SDL_CreateTexture(SDLrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (texture.texture == NULL)
	{
		errLog(SDL_GetError());
	}
	if (textureStorage.noTextures > 0)
	{
		if (SDL_SetTextureBlendMode(texture.texture, SDL_BLENDMODE_BLEND) < 0)
		{ // Only bottom layer texture is opaque
			errLog(SDL_GetError());
		}
	}
	texture.w = width;
	texture.h = height;
	texture.pitch = width * sizeof(uint32_t);
	texture.pixels = NULL;
	texture.frameBuffer = malloc(width * height * sizeof(uint32_t));
	texture.layer = textureStorage.noTextures;

	textureStorage.textures[textureStorage.noTextures++] = texture;

	return texture;
}


int window_run(void)
{
	SDL_RenderClear(SDLrenderer);
	// Go through Layers in order they were created and draw on top of each other
	for (int i = 0; i < textureStorage.noTextures; i++)
	{
		if (textureStorage.textures[i].pixels != NULL)
		{ // Make sure we don't draw anything before lockTexture is run the first time
			// Copy framebuffer to screen texture
			memcpy(textureStorage.textures[i].pixels, textureStorage.textures[i].frameBuffer, textureStorage.textures[i].pitch * textureStorage.textures[i].h);
			// Send texture to GPU
			SDL_UnlockTexture(textureStorage.textures[i].texture);
			// Adjust texture to target rectangle (Scale and translate pixels)
			if (SDL_RenderCopy(SDLrenderer, textureStorage.textures[i].texture, NULL, NULL) < 0)
			{
				errLog(SDL_GetError());
			}
		}
	}
	// Present result on screen
	SDL_RenderPresent(SDLrenderer);
	////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////
	updateTime();

	updateInput();

	updateWindow();

	windowMoved();

	// If .closeWindow == true then clean up and close window
	if (window.closeWindow == true)
	{
		TTF_Quit();
		SDL_DestroyRenderer(SDLrenderer);
		SDL_DestroyWindow(SDLwindow);
		SDL_Quit();
		return false;
	}
	/////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////
	// Lock texture and get a new pixel pointer
	for (int i = 0; i < textureStorage.noTextures; i++)
	{
		if (SDL_LockTexture(textureStorage.textures[i].texture, NULL, &textureStorage.textures[i].pixels, &textureStorage.textures[i].pitch) < 0)
		{
			errLog(SDL_GetError());
		}
	}
	return true;
}

Window *window_init()
{

	// Copy window so we can detect changes after this
	memcpy(&oldWindow, &window, sizeof(Window));

	if (SDL_Init( SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR ) < 0)
	{
		errLog(SDL_GetError());
		return NULL;
	}
	uint32_t flags = 0;
	if (window.settings.resizable)
		flags |= SDL_WINDOW_RESIZABLE;
	if (window.settings.borderLess)
		flags |= SDL_WINDOW_BORDERLESS;
	if (window.settings.fullScreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	if (window.settings.maximized)
		flags |= SDL_WINDOW_MAXIMIZED;
	if (window.settings.minimized)
		flags |= SDL_WINDOW_MINIMIZED;

	SDLwindow = SDL_CreateWindow(window.title, window.pos.x, window.pos.y, window.size.w, window.size.h, flags);
	if (SDLwindow == NULL)
	{
		errLog(SDL_GetError());
		return NULL;
	}

	SDLrenderer = SDL_CreateRenderer(SDLwindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (SDLrenderer == NULL)
	{
		errLog(SDL_GetError());
		return NULL;
	}

	// Set size of SDLrenderer
	SDL_RenderSetLogicalSize(SDLrenderer, window.size.w, window.size.h);
	// Set blend mode of SDLrenderer
	SDL_SetRenderDrawBlendMode(SDLrenderer, SDL_BLENDMODE_BLEND);

	// Set color of SDLrenderer to black
	SDL_SetRenderDrawColor(SDLrenderer, 100, 0, 0, 255);
	SDL_RenderClear(SDLrenderer);

	keyboardState = SDL_GetKeyboardState(NULL); // get pointer to key states

	return &window;
}

void updateKeyState(enum KeyState *keyState, int scanCode)
{
	if (keyboardState[scanCode])
	{
		if (*keyState == eKEY_IDLE)
		{
			*keyState = eKEY_PRESSED;
		}
		else
		{
			*keyState = eKEY_HELD;
		}
	}
	else
	{
		if (*keyState == eKEY_HELD)
		{
			*keyState = eKEY_RELEASED;
		}
		else
		{
			*keyState = eKEY_IDLE;
		}
	}
	return;
}

static void updateInput(void)
{

	mouse.dWheel = 0; // reset mouseWheel scroll amount from last loop
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			window.closeWindow = true;
			break;
		case SDL_WINDOWEVENT:
		{
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				windowResized();
			}
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_SHOWN:
				SDL_Log("Window %d shown", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_HIDDEN:
				SDL_Log("Window %d hidden", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				SDL_Log("Window %d exposed", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_MOVED:
				SDL_Log("Window %d moved to %d,%d",
						event.window.windowID, event.window.data1,
						event.window.data2);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				SDL_Log("Window %d resized to %dx%d",
						event.window.windowID, event.window.data1,
						event.window.data2);
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				SDL_Log("Window %d size changed to %dx%d",
						event.window.windowID, event.window.data1,
						event.window.data2);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				SDL_Log("Window %d minimized", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				SDL_Log("Window %d maximized", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_RESTORED:
				SDL_Log("Window %d restored", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_ENTER:
				SDL_Log("Mouse entered window %d",
						event.window.windowID);
				break;
			case SDL_WINDOWEVENT_LEAVE:
				SDL_Log("Mouse left window %d", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				SDL_Log("Window %d gained keyboard focus",
						event.window.windowID);

				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				SDL_Log("Window %d lost keyboard focus",
						event.window.windowID);
				break;
			case SDL_WINDOWEVENT_CLOSE:
				SDL_Log("Window %d closed", event.window.windowID);
				break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
			case SDL_WINDOWEVENT_TAKE_FOCUS:
				SDL_Log("Window %d is offered a focus", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_HIT_TEST:
				SDL_Log("Window %d has a special hit test", event.window.windowID);
				break;
#endif
			default:
				SDL_Log("Window %d got unknown event %d",
						event.window.windowID, event.window.event);
				break;
			}
			break;
		}
		case SDL_MOUSEWHEEL:
			mouse.dWheel += event.wheel.y;
			break;
		default:
			break;
		}
	}

	static int oldMouseX;
	static int oldMouseY;

	// update pos and state of mouse
	uint32_t SDL_mouseState = SDL_GetMouseState(&mouse.pos.x, &mouse.pos.y);

	mouse.dPos.x = mouse.pos.x - oldMouseX;
	mouse.dPos.y = mouse.pos.y - oldMouseY;
	oldMouseX = mouse.pos.x;
	oldMouseY = mouse.pos.y;

	SDL_GetGlobalMouseState(&mouse.screenPos.x, &mouse.screenPos.y);

	static int leftTrigger = 0;
	if (SDL_mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		if (leftTrigger == 0)
		{
			mouse.left = eKEY_PRESSED;
			leftTrigger = 1;
		}
		else
		{
			mouse.left = eKEY_HELD;
		}
	}
	else
	{
		if (leftTrigger == 1)
		{
			mouse.left = eKEY_RELEASED;
			leftTrigger = 0;
		}
		else
		{
			mouse.left = eKEY_IDLE;
		}
	}
	static int rightTrigger = 0;
	if (SDL_mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
	{
		if (rightTrigger == 0)
		{
			mouse.right = eKEY_PRESSED;
			rightTrigger = 1;
		}
		else
		{
			mouse.right = eKEY_HELD;
		}
	}
	else
	{
		if (rightTrigger == 1)
		{
			mouse.right = eKEY_RELEASED;
			rightTrigger = 0;
		}
		else
		{
			mouse.right = eKEY_IDLE;
		}
	}

	updateKeyState(&key.LEFT, SDL_SCANCODE_LEFT);
	updateKeyState(&key.UP, SDL_SCANCODE_UP);
	updateKeyState(&key.RIGHT, SDL_SCANCODE_RIGHT);
	updateKeyState(&key.DOWN, SDL_SCANCODE_DOWN);
	updateKeyState(&key.A, SDL_SCANCODE_A);
	updateKeyState(&key.B, SDL_SCANCODE_B);
	updateKeyState(&key.C, SDL_SCANCODE_C);
	updateKeyState(&key.D, SDL_SCANCODE_D);
	updateKeyState(&key.E, SDL_SCANCODE_E);
	updateKeyState(&key.F, SDL_SCANCODE_F);
	updateKeyState(&key.G, SDL_SCANCODE_G);
	updateKeyState(&key.H, SDL_SCANCODE_H);
	updateKeyState(&key.I, SDL_SCANCODE_I);
	updateKeyState(&key.J, SDL_SCANCODE_J);
	updateKeyState(&key.K, SDL_SCANCODE_K);
	updateKeyState(&key.L, SDL_SCANCODE_L);
	updateKeyState(&key.M, SDL_SCANCODE_M);
	updateKeyState(&key.N, SDL_SCANCODE_N);
	updateKeyState(&key.O, SDL_SCANCODE_O);
	updateKeyState(&key.P, SDL_SCANCODE_P);
	updateKeyState(&key.Q, SDL_SCANCODE_Q);
	updateKeyState(&key.R, SDL_SCANCODE_R);
	updateKeyState(&key.S, SDL_SCANCODE_S);
	updateKeyState(&key.T, SDL_SCANCODE_T);
	updateKeyState(&key.U, SDL_SCANCODE_U);
	updateKeyState(&key.V, SDL_SCANCODE_V);
	updateKeyState(&key.W, SDL_SCANCODE_W);
	updateKeyState(&key.X, SDL_SCANCODE_X);
	updateKeyState(&key.Y, SDL_SCANCODE_Y);
	updateKeyState(&key.Z, SDL_SCANCODE_Z);
	updateKeyState(&key.num0, SDL_SCANCODE_0);
	updateKeyState(&key.num1, SDL_SCANCODE_1);
	updateKeyState(&key.num2, SDL_SCANCODE_2);
	updateKeyState(&key.num3, SDL_SCANCODE_3);
	updateKeyState(&key.num4, SDL_SCANCODE_4);
	updateKeyState(&key.num5, SDL_SCANCODE_5);
	updateKeyState(&key.num6, SDL_SCANCODE_6);
	updateKeyState(&key.num7, SDL_SCANCODE_7);
	updateKeyState(&key.num8, SDL_SCANCODE_8);
	updateKeyState(&key.num9, SDL_SCANCODE_9);
	updateKeyState(&key.ctrlLeft, SDL_SCANCODE_LCTRL);
	updateKeyState(&key.shiftLeft, SDL_SCANCODE_LSHIFT);
	updateKeyState(&key.ESC, SDL_SCANCODE_ESCAPE);

	// Scale mouse values against window size
	float scaleX = ((float)window.size.w / window.drawSize.w);
	float scaleY = ((float)window.size.h / window.drawSize.h);
	mouse.pos.x /= scaleX;
	mouse.pos.y /= scaleY;
}

void clearLayer(Layer layer)
{
	memset(layer.frameBuffer, 0, layer.h * layer.pitch);
}

static struct
{
	char fontPath[100];
	TTF_Font *font[32]; // Contains pointers to fonts of different sizes
	int size;
	enum
	{
		eTEXT_SOLID = 0,
		eTEXT_SHADED = 1,
		eTEXT_BLENDED = 2
	} mode;
} textSettings = {
	.fontPath = "Resources/bpdots.squares-bold.ttf",
	.size = 20,
	.mode = eTEXT_SOLID};

void drawText(Layer layer, int xPos, int yPos, char *string)
{
	static int initialized = false; // Set to true after running once

	if (!initialized)
	{
		if (TTF_Init() < 0){
			errLog(SDL_GetError());
		}
		for (int i = 0; i < 32; i++)
		{
			textSettings.font[i] = TTF_OpenFont(textSettings.fontPath, i);
			if (textSettings.font[i] == NULL)
			{
				errLog(SDL_GetError());
			}
		}

		initialized = true;
	}

	SDL_Surface *textSurface = NULL;

	switch (textSettings.mode)
	{
	case eTEXT_SOLID:
		textSurface = TTF_RenderUTF8_Solid(textSettings.font[textSettings.size], string, (SDL_Color){.r = 255, .g = 255, .b = 255, .a = 255});
		if (textSurface == NULL)
			errLog(SDL_GetError());

		break;
	case eTEXT_SHADED:
		textSurface = TTF_RenderUTF8_Shaded(textSettings.font[textSettings.size], string, (SDL_Color){.r = 255, .g = 255, .b = 255, .a = 255}, (SDL_Color){.r = 0, .g = 0, .b = 0, .a = 0});
		if (textSurface == NULL)
			errLog(SDL_GetError());

		break;
	case eTEXT_BLENDED:
		textSurface = TTF_RenderUTF8_Blended(textSettings.font[textSettings.size], string, (SDL_Color){.r = 255, .g = 255, .b = 255, .a = 255});
		if (textSurface == NULL)
			errLog(SDL_GetError());

		break;
	default:
		errLog("Should not happen\n");
		break;
	}

	if (textSurface == NULL)
	{
		errLog("Should not happen\n");
		return;
	}

	for (int i = 0; i < textSurface->h; i++)
	{
		for (int b = 0; b < textSurface->w; b++)
		{
			if (((uint8_t *)textSurface->pixels)[b + i * textSurface->pitch])
			{
				if (textSettings.mode == eTEXT_SOLID)
					layer.frameBuffer[(xPos + b) + (yPos + i) * window.drawSize.w].argb = 0xFFFFFFFF;
				if (textSettings.mode == eTEXT_SHADED)
					layer.frameBuffer[(xPos + b) + (yPos + i) * window.drawSize.w].argb = (0xFF << 24) | (((uint8_t *)textSurface->pixels)[b + i * textSurface->pitch] << 16) | (((uint8_t *)textSurface->pixels)[b + i * textSurface->pitch] << 8) | (((uint8_t *)textSurface->pixels)[b + i * textSurface->pitch]);
			}
			if (textSettings.mode == eTEXT_BLENDED)
				layer.frameBuffer[(xPos + b) + (yPos + i) * window.drawSize.w].argb = ((uint32_t *)(textSurface->pixels))[b + i * textSurface->w];
		}
	}

	//Free textSurface
	SDL_FreeSurface(textSurface);

	//	textureStorage.textures[texture.layer].frameBuffer
}

//Returns time since application start
double getWindowTime(void){
	return  ((double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency());
}




static void boxBlurTf(const float *source, float *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<w; i++) {
        int ti = i, li = ti, ri = ti+r*w;
        float fv = source[ti], lv = source[ti+w*(h-1)], val = (float)(r+1)*fv;
        for(int j=0; j<r; j++) val += source[ti+j*w];
        for(int j=0  ; j<=r ; j++) { val += source[ri] - fv     ;  target[ti] = (val*iarr);  ri+=w; ti+=w; }
        for(int j=r+1; j<h-r; j++) { val += source[ri] - source[li];  target[ti] = (val*iarr);  li+=w; ri+=w; ti+=w; }
        for(int j=h-r; j<h  ; j++) { val += lv      - source[li];  target[ti] = (val*iarr);  li+=w; ti+=w; }
    }
}


static void boxBlurHf(const float *source, float *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<h; i++) {
        int ti = i*w, li = ti, ri = ti+r;
        float fv = source[ti], lv = source[ti+w-1], val = (float)(r+1)*fv;
        for(int j=0; j<r; j++) val += source[ti+j];
        for(int j=0  ; j<=r ; j++) { val += source[ri++] - fv       ;   target[ti++] = (val*iarr); }
        for(int j=r+1; j<w-r; j++) { val += source[ri++] - source[li++];   target[ti++] = (val*iarr); }
        for(int j=w-r; j<w  ; j++) { val += lv        - source[li++];   target[ti++] = (val*iarr); }
    }
}


static void boxBlurf(float *source, float *target,int source_lenght, int w, int h, int r) {
    for(int i=0; i<source_lenght; i++) target[i] = source[i];
    boxBlurHf(target, source, w, h, r);
    boxBlurTf(source, target, w, h, r);
}

void gaussBlurf(float *source, float *target,int source_lenght, int w, int h, int r) {
    int n = 3;
    float bxs[n];
    float wIdeal = sqrtf((12*r*r/n)+1);  // Ideal averaging filter width
    int wl = (int)floorf(wIdeal);  if(wl%2==0) wl--;
    int wu = wl+2;

    float mIdeal = (float)(12*r*r - n*wl*wl - 4*n*wl - 3*n)/(float)(-4*wl - 4);
    int m = roundf(mIdeal);
    // var sigmaActual = std::sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

    for(int i=0; i<n;i++){
        if(i<m){ bxs[i] = (float)wl;}
        else {   bxs[i] = (float)wu;}
    }
    boxBlurf(source, target, source_lenght, w, h, (int)((bxs[0]-1)/2.f));
    boxBlurf(target, source, source_lenght, w, h, (int)((bxs[1]-1)/2.f));
    boxBlurf(source, target, source_lenght, w, h, (int)((bxs[2]-1)/2.f));
}


static void boxBlurTargb(const argb_t *source, argb_t *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<w; i++){
        int ti = i;
		int li = ti;
		int ri = ti+r*w;
        float fv_r = source[ti].r;
        float fv_g = source[ti].g;
        float fv_b = source[ti].b;
		float lv_r = source[ti+w*(h-1)].r;
		float lv_g = source[ti+w*(h-1)].g;
		float lv_b = source[ti+w*(h-1)].b;
		float val_r = (float)(r+1)*fv_r;
		float val_g = (float)(r+1)*fv_g;
		float val_b = (float)(r+1)*fv_b;
        for(int j=0; j<r; j++){
			val_r += source[ti+j*w].r;
			val_g += source[ti+j*w].g;
			val_b += source[ti+j*w].b;
		} 
        for(int j=0  ; j<=r ; j++){
			val_r += source[ri].r - fv_r;
			val_g += source[ri].g - fv_g;
			val_b += source[ri].b - fv_b;
			target[ti].r = roundf(val_r*iarr);  
			target[ti].g = roundf(val_g*iarr);  
			target[ti].b = roundf(val_b*iarr);  
			ri += w; 
			ti += w; 
		}
        for(int j=r+1; j<h-r; j++){
			val_r += source[ri].r - source[li].r;  
			val_g += source[ri].g - source[li].g;  
			val_b += source[ri].b - source[li].b;  
			target[ti].r = roundf(val_r*iarr);  
			target[ti].g = roundf(val_g*iarr);  
			target[ti].b = roundf(val_b*iarr);  
			li += w; 
			ri += w; 
			ti += w; 
		}
        for(int j=h-r; j<h  ; j++){ 
			val_r += lv_r - source[li].r;  
			val_g += lv_g - source[li].g;  
			val_b += lv_b - source[li].b;  
			target[ti].r = roundf(val_r*iarr);  
			target[ti].g = roundf(val_g*iarr);  
			target[ti].b = roundf(val_b*iarr);  
			li += w; 
			ti += w; 
		}
    }
}


static void boxBlurHargb(const argb_t *source, argb_t *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<h; i++) {
        int ti = i*w;
		int li = ti;
		int ri = ti+r;
        float fv_r = source[ti].r;
        float fv_g = source[ti].g;
        float fv_b = source[ti].b;
		float lv_r = source[ti+w-1].r;
		float lv_g = source[ti+w-1].g;
		float lv_b = source[ti+w-1].b;
		float val_r = (float)(r+1)*fv_r;
		float val_g = (float)(r+1)*fv_g;
		float val_b = (float)(r+1)*fv_b;
        for(int j=0; j<r; j++){
			val_r += source[ti+j].r;
			val_g += source[ti+j].g;
			val_b += source[ti+j].b;
		}
        for(int j=0  ; j<=r ; j++){
			val_r += source[ri].r - fv_r;
			val_g += source[ri].g - fv_g;
			val_b += source[ri].b - fv_b;
			target[ti].r = roundf(val_r*iarr); 
			target[ti].g = roundf(val_g*iarr); 
			target[ti].b = roundf(val_b*iarr); 
			ri++;
			ti++;
		}
        for(int j=r+1; j<w-r; j++){
			val_r += source[ri].r - source[li].r;   
			val_g += source[ri].g - source[li].g;   
			val_b += source[ri].b - source[li].b;   
			target[ti].r = roundf(val_r*iarr); 
			target[ti].g = roundf(val_g*iarr); 
			target[ti].b = roundf(val_b*iarr); 
			ri++;
			li++;
			ti++;
		}
        for(int j=w-r; j<w  ; j++){
			val_r += lv_r - source[li].r;
			val_g += lv_g - source[li].g;
			val_b += lv_b - source[li].b;
			target[ti].r = roundf(val_r*iarr); 
			target[ti].g = roundf(val_g*iarr); 
			target[ti].b = roundf(val_b*iarr); 
			li++;
			ti++;
		}
    }
}


static void boxBlurargb(argb_t *source, argb_t *target,int source_lenght, int w, int h, int r) {
    for(int i=0; i<source_lenght; i++){
		target[i] = source[i];
	} 
    boxBlurHargb(target, source, w, h, r);
    boxBlurTargb(source, target, w, h, r);
}

void gaussBlurargb(argb_t *source, argb_t *target,int source_lenght, int w, int h, int r) {
    int n = 3;
    float bxs[n];
    float wIdeal = sqrtf((12*r*r/n)+1);  // Ideal averaging filter width
    int wl = (int)floorf(wIdeal);  
	if(wl%2==0){ 
		wl--;
	}
    int wu = wl+2;

    float mIdeal = (float)(12*r*r - n*wl*wl - 4*n*wl - 3*n)/(float)(-4*wl - 4);
    int m = roundf(mIdeal);
    // var sigmaActual = std::sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

    for(int i=0; i<n;i++){
        if(i<m){
			bxs[i] = (float)wl;
		}else{
			bxs[i] = (float)wu;
		}
    }
    boxBlurargb(source, target, source_lenght, w, h, (int)((bxs[0]-1)/2.f));
    boxBlurargb(target, source, source_lenght, w, h, (int)((bxs[1]-1)/2.f));
    boxBlurargb(source, target, source_lenght, w, h, (int)((bxs[2]-1)/2.f));
}


//Normalize given vector
inline vec2f_t normalizeVec2f(vec2f_t vector){
	float length = sqrtf(vector.x * vector.x + vector.y * vector.y);
	vector.x /= length;
	vector.y /= length;
	return vector;
}


void drawPoint(Layer layer, int x, int y, argb_t color){
	if(x >= layer.w || x < 0 || y >= layer.h || y < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", x, y));
		return;
	}

	layer.frameBuffer[x+y*layer.w] = color;
}

void drawLine(Layer layer, int xStart, int yStart, int xEnd, int yEnd, argb_t color){
	int outOfBounds = 0; //If both point are outside bounds we return from this funciton before drawing anything
	if(xStart >= layer.w || xStart < 0 || yStart >= layer.h || yStart < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", xStart, yStart));
		xStart = clampf(xStart, 0, layer.w-1);
		yStart = clampf(yStart, 0, layer.h-1);
		outOfBounds++;
	}
	if(xEnd >= layer.w || xEnd < 0 || yEnd >= layer.h || yEnd < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", xStart, yStart));
		xEnd = clampf(xEnd, 0, layer.w-1);
		yEnd = clampf(yEnd, 0, layer.h-1);
		outOfBounds++;
	}
	if(outOfBounds >= 2) return;



	int dx = xEnd - xStart;
	int dy = yEnd - yStart;

	 // calculate steps required for generating pixels
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

	// calculate increment in x & y for each steps
	float Xinc = dx / (float) steps;
	float Yinc = dy / (float) steps;

	// Put pixel for each step
	float X = xStart;
	float Y = yStart;
	for (int i = 0; i <= steps; i++)
	{
		int x = round(X);
		int y = round(Y);
		layer.frameBuffer[(x)+(y)*layer.w] = color;
		X += Xinc;           // increment in x at each step
		Y += Yinc;           // increment in y at each step

	}
}


static float clampf(float value, float min, float max) {
    const float t = value < min ? min : value;
    return t > max ? max : t;
}