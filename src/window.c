#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h> //clampf
#include <immintrin.h> //SIMD stuff

#include "../dependencies/include/glad/glad.h"

#define SDL_MAIN_HANDLED
#include "../dependencies/include/SDL2/SDL.h"
#include "../dependencies/include/SDL2/SDL_image.h"
#include "../dependencies/include/SDL2/SDL_ttf.h"

#include "window.h"

//Build window.c with -mavx 
//Link with -lgdi32 -lSDL2_ttf -lSDL2 -lm -lSDL2_image

// Static variable
const Uint8 *keyboardState; // pointer to key states

// Static functions
static char *getMouseStateString(void);

#define WINDOW_DEFAULT_W 800
#define WINDOW_DEFAULT_H 600
#define WINDOW_DEFAULT_X SDL_WINDOWPOS_CENTERED
#define WINDOW_DEFAULT_Y SDL_WINDOWPOS_CENTERED
#define WINDOW_DEFAULT_TITLE "Default title"

// window default values
Window window = {
	.pos.x = WINDOW_DEFAULT_X,
	.pos.y = WINDOW_DEFAULT_Y,
	.size.w = WINDOW_DEFAULT_W,
	.size.h = WINDOW_DEFAULT_H,
	.drawSize.w = WINDOW_DEFAULT_W,
	.drawSize.h = WINDOW_DEFAULT_H,
	.title = WINDOW_DEFAULT_TITLE
	};

Window oldWindow; // Copy of window, used to compare against to detect changes in settings



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
SDL_GLContext GLcontext;

GLuint windowShaderProgram; //Used to render framebuffer
GLuint windowShaderTexture;
GLuint VAO;
GLint windowShaderUniformloc;

#define MAX_TEXTURES (3) // Max number of allowed textures
static struct
{
	int noTextures;
	sdlTexture textures[MAX_TEXTURES];
} textureStorage;


static void windowResized(void);
static void updateTime(void);
static sdlTexture createSDLTexture(int width, int height);
static void updateInput(void);
void drawText(Layer layer, int xPos, int yPos, char *string);

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
	if (!freq){
		freq = SDL_GetPerformanceFrequency();
	}
	static uint64_t oldTime = 0;
	uint64_t newTime = SDL_GetPerformanceCounter();
	window.time.dTime = (double)(newTime - oldTime) / (double)freq;
	oldTime = newTime;

	window.time.fps = 1.0 / window.time.dTime;
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
	// texture.texture = SDL_CreateTexture(SDLrenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	// if (texture.texture == NULL)
	// {
	// 	errLog(SDL_GetError());
	// }
	// if (textureStorage.noTextures > 0)
	// {
	// 	if (SDL_SetTextureBlendMode(texture.texture, SDL_BLENDMODE_BLEND) < 0)
	// 	{ // Only bottom layer texture is opaque
	// 		errLog(SDL_GetError());
	// 	}
	// }
	texture.w = width;
	texture.h = height;
	texture.pitch = width * sizeof(uint32_t);
	texture.pixels = NULL;
	texture.frameBuffer = malloc(width * height * sizeof(uint32_t));
	texture.layer = textureStorage.noTextures;

	textureStorage.textures[textureStorage.noTextures++] = texture;

	return texture;
}



Window* window_init()
{

	// Copy window so we can detect changes after this
	memcpy(&oldWindow, &window, sizeof(Window));

	if (SDL_Init( SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR ) < 0)
	{
		errLog(SDL_GetError());
		return NULL;
	}

	// CONFIGURE OPENGL ATTRIBUTES USING SDL:
	// #ifdef __EMSCRIPTEN__
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	// #else
    	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// #endif
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	uint32_t flags = SDL_WINDOW_OPENGL;
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

    //Create GL context
    GLcontext = SDL_GL_CreateContext(SDLwindow);
    if(GLcontext == NULL) {
		errLog(SDL_GetError());
		return NULL;
    }

    //Make GL context current
	SDL_GL_MakeCurrent(SDLwindow, GLcontext);

	// Disable vsync
	SDL_GL_SetSwapInterval(window.settings.vSync ? 1 : 0);

    // INITIALIZE GLAD FOR GLES:
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("%s\n","Failed to initialize GLAD");
    }

   // LOG OPENGL VERSION, VENDOR (IMPLEMENTATION), RENDERER, GLSL, ETC.:
    printf("OpenGL Version: %d . %d \n",  GLVersion.major, GLVersion.minor);
    printf("OpenGL Shading Language Version: %s \n",  (char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("OpenGL Vendor: %s \n",  (char *)glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s \n",  (char *)glGetString(GL_RENDERER));


	uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
	if (window.settings.vSync)
		rendererFlags |= SDL_RENDERER_PRESENTVSYNC;


	// SDLrenderer = SDL_CreateRenderer(SDLwindow, -1, rendererFlags); 
	// if (SDLrenderer == NULL)
	// {
	// 	errLog(SDL_GetError());
	// 	return NULL;
	// }

	// // Set size of SDLrenderer
	// SDL_RenderSetLogicalSize(SDLrenderer, window.size.w, window.size.h);
	// // Set blend mode of SDLrenderer
	// SDL_SetRenderDrawBlendMode(SDLrenderer, SDL_BLENDMODE_BLEND);

	// // Set color of SDLrenderer to black
	// SDL_SetRenderDrawColor(SDLrenderer, 100, 0, 0, 255);
	// SDL_RenderClear(SDLrenderer);



	const char* vertexShaderSource =
		"#version 300 es\n"
		"precision mediump float;\n"
		"\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec2 aTexCoord;\n"
		"\n"
		"out vec2 texCoord;\n"
		"\n"
		"void main() {\n"
		"    gl_Position = vec4(aPos, 1.0);\n"
		"    texCoord = aTexCoord;\n"
		"}\n";

	
    //Load and compile vertex shader
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint vertexSuccess;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexSuccess);
    if (!vertexSuccess) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("Shader compilation failed\n%s\n", infoLog);
		return 0;
    }

	const char* fragmentShaderSource =
		"#version 300 es\n"
		"precision mediump float;\n"
		"\n"
		"in vec2 texCoord;\n"
		"uniform sampler2D inputDataTexture1;\n"
		"\n"
		"out vec4 FragColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec2 invertedTexCoords = vec2(texCoord.x, 1.0 - texCoord.y);  // Invert Y coordinate\n"
		"    FragColor = texture(inputDataTexture1, invertedTexCoords).bgra;  // Fetch the color using inverted coordinates\n"
		"}\n";

    //Load and compile fragment shader
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLint fragmentSuccess;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentSuccess);
    if (!fragmentSuccess) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        errLog("Shader compilation failed\n%s\n", infoLog);
		return 0;
    }


    //Compile program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint programSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programSuccess);
    if(!programSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        errLog("Shader program linking failed\n%s\n", infoLog);
		return 0;
    }

    //Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);  

	windowShaderProgram = shaderProgram;
	if(windowShaderProgram == 0){
		errLog("Failed to compile shader");
	}
	glGenTextures(1, &windowShaderTexture);
	glBindTexture(GL_TEXTURE_2D, windowShaderTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.drawSize.w, window.drawSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);  // No initial data
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
	windowShaderUniformloc = glGetUniformLocation(windowShaderProgram, "inputDataTexture1");
	if (windowShaderUniformloc == -1) {
		errLog("Uniform '%s' not found in shader.", "inputDataTexture1");
		return 0;
	}

	//Define vertices and indices for a fullscreen quad
	float vertices[] = {
		1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
		1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
	};

	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};  

	GLuint EBO;
	GLuint VBO;
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);  
	
	//Bind vertex array
	glBindVertexArray(VAO);
	
	//Bind vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//Copy verticies into VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//Copy indices into EBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	//Specify the input in the vertex shader
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0 * sizeof(float)));


	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindVertexArray(0); 


	keyboardState = SDL_GetKeyboardState(NULL); // get pointer to key states

	return &window;
}


int window_run(void)
{
	// SDL_RenderClear(SDLrenderer);
	// // Go through Layers in order they were created and draw on top of each other
	// for (int i = 0; i < textureStorage.noTextures; i++)
	// {
	// 	if (textureStorage.textures[i].pixels != NULL)
	// 	{ // Make sure we don't draw anything before lockTexture is run the first time
	// 		// Copy framebuffer to screen texture
	// 		memcpy(textureStorage.textures[i].pixels, textureStorage.textures[i].frameBuffer, textureStorage.textures[i].pitch * textureStorage.textures[i].h);
	// 		// Send texture to GPU
	// 		SDL_UnlockTexture(textureStorage.textures[i].texture);
	// 		// Adjust texture to target rectangle (Scale and translate pixels)
	// 		if (SDL_RenderCopy(SDLrenderer, textureStorage.textures[i].texture, NULL, NULL) < 0)
	// 		{
	// 			errLog(SDL_GetError());
	// 		}
	// 	}
	// }
	// // Present result on screen
	// SDL_RenderPresent(SDLrenderer);
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
	// for (int i = 0; i < textureStorage.noTextures; i++)
	// {
	// 	if (SDL_LockTexture(textureStorage.textures[i].texture, NULL, &textureStorage.textures[i].pixels, &textureStorage.textures[i].pitch) < 0)
	// 	{
	// 		errLog(SDL_GetError());
	// 	}
	// }


	//Select shader
	glUseProgram(windowShaderProgram);

	//Select framebuffer to render to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Set size of rendering target
	glViewport(0, 0, window.size.w, window.size.h); 

	//Select the vertex array containing the fullscreen quad
	glBindVertexArray(VAO); 
	
	//Bind input textures to shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, windowShaderTexture);

	//Update texture data
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureStorage.textures[0].w, textureStorage.textures[0].h, GL_RGBA, GL_UNSIGNED_BYTE, textureStorage.textures[0].frameBuffer);

	

	glUniform1i(windowShaderUniformloc, 0);
	

	//Render to the fullsreen quad
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SDL_GL_SwapWindow(SDLwindow);

	return true;
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
				// SDL_Log("Window %d shown", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_HIDDEN:
				// SDL_Log("Window %d hidden", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				// SDL_Log("Window %d exposed", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_MOVED:
				// SDL_Log("Window %d moved to %d,%d",
				// 		event.window.windowID, event.window.data1,
				// 		event.window.data2);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				// SDL_Log("Window %d resized to %dx%d",
				// 		event.window.windowID, event.window.data1,
				// 		event.window.data2);
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				// SDL_Log("Window %d size changed to %dx%d",
				// 		event.window.windowID, event.window.data1,
				// 		event.window.data2);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				// SDL_Log("Window %d minimized", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				// SDL_Log("Window %d maximized", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_RESTORED:
				// SDL_Log("Window %d restored", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_ENTER:
				// SDL_Log("Mouse entered window %d",
				// 		event.window.windowID);
				break;
			case SDL_WINDOWEVENT_LEAVE:
				// SDL_Log("Mouse left window %d", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				// SDL_Log("Window %d gained keyboard focus",
				// 		event.window.windowID);

				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				// SDL_Log("Window %d lost keyboard focus",
				// 		event.window.windowID);
				break;
			case SDL_WINDOWEVENT_CLOSE:
				// SDL_Log("Window %d closed", event.window.windowID);
				break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
			case SDL_WINDOWEVENT_TAKE_FOCUS:
				// SDL_Log("Window %d is offered a focus", event.window.windowID);
				break;
			case SDL_WINDOWEVENT_HIT_TEST:
				// SDL_Log("Window %d has a special hit test", event.window.windowID);
				break;
#endif
			default:
				// SDL_Log("Window %d got unknown event %d",
				// 		event.window.windowID, event.window.event);
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




void drawPoint(Layer layer, int x, int y, argb_t color){
	if(x >= layer.w || x < 0 || y >= layer.h || y < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", x, y));
		return;
	}

	layer.frameBuffer[x+y*layer.w] = color;
}

void drawSquare(Layer layer, int x, int y, int w, int h, argb_t color){
	
	int xStart = clampi(x, 0, layer.w);
	int yStart = clampi(y, 0, layer.h);
	int xEnd = clampi(x + w, 0, layer.w);
	int yEnd = clampi(y + h, 0, layer.h);

	for(int Y = yStart; Y < yEnd; Y++){
		for(int X = xStart; X < xEnd; X++){
			layer.frameBuffer[X+Y*layer.w] = color;
		}
	}

}

void drawLine(Layer layer, int xStart, int yStart, int xEnd, int yEnd, argb_t color){
	int outOfBounds = 0; //If both point are outside bounds we return from this funciton before drawing anything
	if(xStart >= layer.w || xStart < 0 || yStart >= layer.h || yStart < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", xStart, yStart));
		xStart = clampi(xStart, 0, layer.w-1);
		yStart = clampi(yStart, 0, layer.h-1);
		outOfBounds++;
	}
	if(xEnd >= layer.w || xEnd < 0 || yEnd >= layer.h || yEnd < 0){
		// errLog(printfLocal("(%d,%d) Out of bounds", xStart, yStart));
		xEnd = clampi(xEnd, 0, layer.w-1);
		yEnd = clampi(yEnd, 0, layer.h-1);
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


argb_t argbAdd1(argb_t color1, argb_t color2){
	argb_t result_color;

	result_color = ARGB(color1.a+color2.a, color1.r+color2.r, color1.g+color2.g, color1.b+color2.b);

	return result_color;
}

argb_t argbAdd2(argb_t color1, argb_t color2){
    argb_t result_color;

	// Load colors into 128-bit vectors
    __m128i color1_vec = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, color1.a, color1.r, color1.g, color1.b);
    __m128i color2_vec = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, color2.a, color2.r, color2.g, color2.b);

    // Add the vectors element-wise
    __m128i result_vec = _mm_add_epi8(color1_vec, color2_vec);

    // Store the result back into a color struct
    result_color.b = _mm_extract_epi8(result_vec, 0);
    result_color.g = _mm_extract_epi8(result_vec, 1);
    result_color.r = _mm_extract_epi8(result_vec, 2);
    result_color.a = _mm_extract_epi8(result_vec, 3);
	
    return result_color;
}

void testFunc(void){

	__m128 val = _mm_set_ps(1.f, 2.f, 3.f, 4.f);
	float val3;
	float val2;
	float val1;
	float val0;
	_MM_EXTRACT_FLOAT(val3, val, 3);
	_MM_EXTRACT_FLOAT(val2, val, 2);
	_MM_EXTRACT_FLOAT(val1, val, 1);
	_MM_EXTRACT_FLOAT(val0, val, 0);

	printf("val = %f %f %f %f \n", val3, val2, val1 ,val0);
	__m128 addend = _mm_set_ps(10.f, 20.f, 30.f, 40.f);

	_MM_EXTRACT_FLOAT(val3, addend, 3);
	_MM_EXTRACT_FLOAT(val2, addend, 2);
	_MM_EXTRACT_FLOAT(val1, addend, 1);
	_MM_EXTRACT_FLOAT(val0, addend, 0);

	printf("addend = %f %f %f %f \n", val3, val2, val1 ,val0);

	__m128 res = _mm_add_ps(val, addend);


	_MM_EXTRACT_FLOAT(val3, res, 3);
	_MM_EXTRACT_FLOAT(val2, res, 2);
	_MM_EXTRACT_FLOAT(val1, res, 1);
	_MM_EXTRACT_FLOAT(val0, res, 0);

	printf("result = %f %f %f %f \n", val3, val2, val1 ,val0);



}


static void boxBlurTargb(const argb_t *source, argb_t *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
	__m128 iarr_vec  = _mm_set_ps(iarr, iarr, iarr, iarr); //new
    for(int i=0; i<w; i++){
        int ti = i;
		int li = ti;
		int ri = ti+r*w;
		
		__m128 fv  = _mm_set_ps(source[ti].a, source[ti].r, source[ti].g, source[ti].b);
		__m128 lv  = _mm_set_ps(source[ti+w*(h-1)].a, source[ti+w*(h-1)].r, source[ti+w*(h-1)].g, source[ti+w*(h-1)].b);
		__m128 val = _mm_set_ps((float)(r+1)*source[ti].a, (float)(r+1)*source[ti].r, (float)(r+1)*source[ti].g, (float)(r+1)*source[ti].b);


        for(int j=0; j<r; j++){
			__m128 addend = _mm_set_ps(source[ti+j*w].a, source[ti+j*w].r, source[ti+j*w].g, source[ti+j*w].b);
			val = _mm_add_ps(val, addend);
		} 
		
		
        for(int j=0  ; j<=r ; j++){
			__m128 addend = _mm_set_ps(source[ri].a, source[ri].r, source[ri].g, source[ri].b);
			addend = _mm_sub_ps(addend, fv);
			val = _mm_add_ps(val, addend);
			
			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;

			ri += w; 
			ti += w; 
		}


		
        for(int j=r+1; j<h-r; j++){
			__m128 riVec = _mm_set_ps(source[ri].a, source[ri].r, source[ri].g, source[ri].b);
			__m128 liVec = _mm_set_ps(source[ri].a, source[li].r, source[li].g, source[li].b);
			__m128 diffVec = _mm_sub_ps(riVec, liVec);
			val = _mm_add_ps(val, diffVec);

			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;

			li += w; 
			ri += w; 
			ti += w; 
		}

        for(int j=h-r; j<h  ; j++){ 
			__m128 addend = _mm_set_ps(source[li].a, source[li].r, source[li].g, source[li].b);
			addend = _mm_sub_ps(lv, addend);
			val = _mm_add_ps(val, addend);
			
			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;

			li += w; 
			ti += w; 
		}
    }
}


static void boxBlurHargb(const argb_t *source, argb_t *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
	__m128 iarr_vec  = _mm_set_ps(iarr, iarr, iarr, iarr); //new
    for(int i=0; i<h; i++) {
        int ti = i*w;
		int li = ti;
		int ri = ti+r;

		__m128 fv  = _mm_set_ps(source[ti].a, source[ti].r, source[ti].g, source[ti].b);
		__m128 lv  = _mm_set_ps(source[ti+w-1].a, source[ti+w-1].r, source[ti+w-1].g, source[ti+w-1].b);
		__m128 val = _mm_set_ps((float)(r+1)*source[ti].a, (float)(r+1)*source[ti].r, (float)(r+1)*source[ti].g, (float)(r+1)*source[ti].b);

		
        for(int j=0; j<r; j++){
			__m128 addend = _mm_set_ps(source[ti+j].a, source[ti+j].r, source[ti+j].g, source[ti+j].b);
			val = _mm_add_ps(val, addend);
		}

        for(int j=0  ; j<=r ; j++){
			__m128 addend = _mm_set_ps(source[ri].a, source[ri].r, source[ri].g, source[ri].b);
			addend = _mm_sub_ps(addend, fv);
			val = _mm_add_ps(val, addend);
			
			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;

			ri++;
			ti++;
		}
        for(int j=r+1; j<w-r; j++){
			__m128 riVec = _mm_set_ps(source[ri].a, source[ri].r, source[ri].g, source[ri].b);
			__m128 liVec = _mm_set_ps(source[ri].a, source[li].r, source[li].g, source[li].b);
			__m128 diffVec = _mm_sub_ps(riVec, liVec);
			val = _mm_add_ps(val, diffVec);

			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;

			ri++;
			li++;
			ti++;
		}

        for(int j=w-r; j<w  ; j++){
			__m128 addend = _mm_set_ps(source[li].a, source[li].r, source[li].g, source[li].b);
			addend = _mm_sub_ps(lv, addend);
			val = _mm_add_ps(val, addend);
			
			__m128 product =_mm_mul_ps(val, iarr_vec);
			product = _mm_round_ps(product, (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));

			// Convert the floats to 32-bit packed integers with rounding
			__m128i intVec = _mm_cvtps_epi32(product);

			// Extract each 32-bit integer, mask to get the lower 8 bits, and store in the structure
			target[ti].a = (_mm_extract_epi32(intVec, 3)) & 0xFF;
			target[ti].r = (_mm_extract_epi32(intVec, 2)) & 0xFF;
			target[ti].g = (_mm_extract_epi32(intVec, 1)) & 0xFF;
			target[ti].b = (_mm_extract_epi32(intVec, 0)) & 0xFF;
 
			li++;
			ti++;
		}
    }
}

static void boxBlurargb(argb_t *source, argb_t *target,int source_lenght, int w, int h, int r) {
    for(int i=0; i<source_lenght; i++){
		target[i] = source[i];
	} 
    boxBlurTargb(target, source, w, h, r);
    boxBlurHargb(source, target, w, h, r);
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


//Vector stuff

//Normalize given vector
vec2f_t normalizeVec2f(vec2f_t vector){
	float length = sqrtf(vector.x * vector.x + vector.y * vector.y);
	vector.x /= length;
	vector.y /= length;
	return vector;
}

//Normalize given vector
vec3f_t normalizeVec3f(vec3f_t vector){
	float length = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
	vector.x /= length;
	vector.y /= length;
	vector.z /= length;
	return vector;
}


float dotProduct(vec3f_t v0, vec3f_t v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

vec3f_t crossProduct(vec3f_t v0, vec3f_t v1){
	vec3f_t resultVec;
	resultVec.x = v0.y * v1.z - v0.z * v1.y;
    resultVec.y = v0.z * v1.x - v0.x * v1.z;
    resultVec.z = v0.x * v1.y - v0.y * v1.x;
	return resultVec;
}




// Function to read the shader source code from a file
char* readShaderSource(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        errLog("Failed to open shader file: %s\n", filePath);
        return NULL;
    }

 	//Get length of file
    fseek(file, 0, SEEK_END);
    int fileLength = ftell(file);
    fseek(file, 0, SEEK_SET);

	// Allocate memory for the shader source code
    char* buffer = (char*)calloc(fileLength, sizeof(char));
    if (!buffer) {
        errLog("Failed to allocate memory for shader source\n");
		return NULL;
    }

    //Read file to buffer
    char chr;
    int len = 0;
    do{
        chr = fgetc(file);
        buffer[len++] = chr;
    }while(chr != EOF);

    //Add null termination to buffer for use as c string
    buffer[len-1] = '\0';

    fclose(file);
    return buffer;
}

uint32_t compileShaderProgram(char* vertexShaderPath, char* fragmentShaderPath){

    //Load shader source from path
    const char* vertexShaderSource = readShaderSource(vertexShaderPath);
	if(vertexShaderSource == NULL){
		return 0;
	}
	
    //Load and compile vertex shader
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint vertexSuccess;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexSuccess);
    if (!vertexSuccess) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		errLog("Vertex shader compilation failed for %s\n%s\n", vertexShaderPath, infoLog);
		return 0;
    }

	//Load shader source from path
    const char* fragmentShaderSource = readShaderSource(fragmentShaderPath);

    //Load and compile fragment shader
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLint fragmentSuccess;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentSuccess);
    if (!fragmentSuccess) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        errLog("Fragment shader compilation failed for %s\n%s\n", fragmentShaderPath, infoLog);
		return 0;
    }


    //Compile program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint programSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programSuccess);
    if(!programSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        errLog("Shader program linking failed\n%s\n", infoLog);
		return 0;
    }

    //Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);  
    free((char*)vertexShaderSource);
    free((char*)fragmentShaderSource);


    return  shaderProgram;
}


int setUniform(GLuint program, UniformType type, char* name, void* dataPtr){
	
	static struct{
		int noUniforms;
		struct{
			uint64_t hash;
			GLint location;
		}uniform[32];
	}cache;

	//Look in cache for uniform location
	GLint location = -1;
	uint64_t hash = (uint64_t)program ^ (uint64_t)type ^ (uint64_t)name; 
	for(int i = 0; i < cache.noUniforms; i++){
		if(hash == cache.uniform[i].hash){
			location = cache.uniform[i].location;
			break;
		}
	}

	//If no cached location is found, get one and cache it
	if(location == -1){
		//Get uniform location
		GLint loc = glGetUniformLocation(program, name);
		if (loc == -1) {
			errLog("Uniform '%s' not found in shader.", name);
			return 0;
		}else{
			cache.uniform[cache.noUniforms].location = loc;
			cache.uniform[cache.noUniforms].hash = (uint64_t)program ^ (uint64_t)type ^ (uint64_t)name; 
			cache.noUniforms++;
		}
	}



	
	switch (type) {
		case eUNIFROMTYPE_FLOAT:
		{
			float v1 = *(float*)(dataPtr);
			glUniform1f(location, v1);
		}	break;
		case eUNIFROMTYPE_FLOAT_VEC2:
		{
			vec2f_t v1 = *(vec2f_t*)(dataPtr);
			glUniform2f(location, v1.x, v1.y);
		}	break;
		case eUNIFROMTYPE_FLOAT_VEC3:
		{
			vec3f_t v1 = *(vec3f_t*)(dataPtr);
			glUniform3f(location, v1.x, v1.y, v1.z);
		}	break;
		case eUNIFROMTYPE_INT:
		{
			int v1 = *(int*)(dataPtr);
			glUniform1i(location, v1);
		}	break;
		case eUNIFROMTYPE_INT_VEC2:
		{
			vec2i_t v1 = *(vec2i_t*)(dataPtr);
			glUniform2i(location, v1.x, v1.y);
		}	break;
		case eUNIFROMTYPE_INT_VEC3:
		{
			vec3i_t v1 = *(vec3i_t*)(dataPtr);
			glUniform3i(location, v1.x, v1.y, v1.z);
		}	break;
		default:
			errLog("Unsupported uniform type for uniform '%s'.", name);
			return 0;
	}


	return 1;
}


void runShader(Shader* shader){

	static GLenum drawBuffers[16] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11, GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};
	



	switch(shader->state){
		case eSHADERSTATE_UNINITIALIZED:


			shader->program =  compileShaderProgram(shader->vertexShaderSource, shader->fragmentShaderSource);
			if(shader->program == 0){
				shader->state = eSHADERSTATE_FAILED;
				return;
			}


			// //Create texture
			// glGenTextures(1, &texture1);
			// glBindTexture(GL_TEXTURE_2D, texture1);

			// //Set texture edge options
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			// //Set texture scaling options
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// stbi_set_flip_vertically_on_load(1);  

			// //Load image file
			// int imgW;
			// int imgH;
			// int noChannels;
			// unsigned char* data;
			// data = stbi_load("Resources/container.jpg", &imgW, &imgH, &noChannels, 0);
			// if(data == NULL){
			// 	errLog("Failed to load texture");
			// 	shader->state = eSHADERSTATE_FAILED;
			// }
			// //Load image to texture
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgW, imgH, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

			// //Generate mipmaps
			// glGenerateMipmap(GL_TEXTURE_2D);

			// //Clean up image
			// stbi_image_free(data);

			// //Create texture
			// glGenTextures(1, &texture2);
			// glBindTexture(GL_TEXTURE_2D, texture2);

			// //Set texture edge options
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			// //Set texture scaling options
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


			// data = stbi_load("Resources/awesomeface.png", &imgW, &imgH, &noChannels, 0);
			// if(data == NULL){
			// 	errLog("Failed to load texture");
			// 	shader->state = eSHADERSTATE_FAILED;
			// }
			// //Load image to texture
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgW, imgH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

			// //Generate mipmaps
			// glGenerateMipmap(GL_TEXTURE_2D);

			// //Clean up image
			// stbi_image_free(data);

			//Initalize and load input textures
			for(int i = 0; i < shader->input.no; i++){
				glGenTextures(1, &(shader->textureIn[i]));
				glBindTexture(GL_TEXTURE_2D, shader->textureIn[i]);
				if(shader->type == eSHADERTYPE_FLOAT){
					glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shader->width, shader->height, 0, GL_RED, GL_FLOAT, shader->input.data[i].ptr_f);
				}else{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shader->width, shader->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, shader->input.data[i].ptr_argb);
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				
			}

			glGenFramebuffers(1, &(shader->fbo));
			glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

			for(int i = 0; i < shader->output.no; i++){
				//Initialize output texture where result will be stored.
				glGenTextures(1, &(shader->textureOut[i]));
				glBindTexture(GL_TEXTURE_2D, shader->textureOut[i]);
				if(shader->type == eSHADERTYPE_FLOAT){
					glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shader->width, shader->height, 0, GL_RED, GL_FLOAT, NULL);  // No initial data
				}else{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shader->width, shader->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);  // No initial data
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				//Create a framebuffer that the shader will render to, we will read the result from this framebuffer.
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, shader->textureOut[i], 0);
				if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
					errLog("framebuffer error: %X \n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
					shader->state = eSHADERSTATE_FAILED;
					return;
				}
			}

			// Set the list of draw buffers
			glDrawBuffers(shader->output.no, drawBuffers);




			//Define vertices and indices for a fullscreen quad
			float vertices[] = {
				1.0f,  1.0f, 0.0f,   1.0f, 1.0f, // top right
				1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // bottom right
				-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // bottom left
				-1.0f,  1.0f, 0.0f,   0.0f, 1.0f  // top left 
			};

			unsigned int indices[] = {  // note that we start from 0!
				0, 1, 3,   // first triangle
				1, 2, 3    // second triangle
			};  

			glGenBuffers(1, &(shader->EBO));
			glGenVertexArrays(1, &(shader->VAO));
			glGenBuffers(1, &(shader->VBO));  
			
			//Bind vertex array
			glBindVertexArray(shader->VAO);
			
			//Bind vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, shader->VBO);
			//Copy verticies into VBO
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			//Bind element buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shader->EBO);
			//Copy indices into EBO
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


			//Specify the input in the vertex shader
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0 * sizeof(float)));


			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0); 
			glBindVertexArray(0); 


			shader->state = eSHADERSTATE_INITIALIZED;
			__attribute__((fallthrough));
		case eSHADERSTATE_INITIALIZED:
		{

			//Select shader
			glUseProgram(shader->program);

			//Select framebuffer to render to
			glBindFramebuffer(GL_FRAMEBUFFER, shader->fbo);

			// Set the list of draw buffers
			glDrawBuffers(shader->output.no, drawBuffers);

		    glViewport(0, 0, shader->width, shader->height); //Set width and height of render target (Should be equal to outputData dimensions)

			//Not sure if clearing is needed
			// glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			// glClear(GL_COLOR_BUFFER_BIT);

			//Select the vertex array containing the fullscreen quad
			glBindVertexArray(shader->VAO); 
			
			//Bind input textures to shader
			for(int i = 0; i < shader->input.no; i++){
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, shader->textureIn[i]);

				//Update texture data
				if(shader->type == eSHADERTYPE_FLOAT){
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader->width, shader->height, GL_RED, GL_FLOAT, shader->input.data[i].ptr_f);
				}else{
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shader->width, shader->height, GL_RGBA, GL_UNSIGNED_BYTE, shader->input.data[i].ptr_argb);
				}
				
				setUniform(shader->program, eUNIFROMTYPE_INT, shader->input.data[i].name, &i);
			}

			//Set uniforms
			for(int i = 0; i < shader->uniform.no; i++){
				//The last parameter below just need the pointer, we can use whatever type since it's a union
				setUniform(shader->program, shader->uniform.data[i].type, shader->uniform.data[i].name, &(shader->uniform.data[i].val_i));
			}


			//Render to the fullsreen quad
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


			for(int i = 0; i < shader->output.no; i++){
				glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
				if(shader->type == eSHADERTYPE_FLOAT){
					glReadPixels(0, 0, shader->width, shader->height, GL_RED, GL_FLOAT, shader->output.data[i].ptr_f); 
				}else{
					glReadPixels(0, 0, shader->width, shader->height, GL_RGBA, GL_UNSIGNED_BYTE, shader->output.data[i].ptr_argb); 
				}
			}



		}	break;
		case eSHADERSTATE_FAILED:
		{

			//Check if any errors has occured
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				errLog("OpenGL error: %X", error);
				shader->state = eSHADERSTATE_FAILED;
			}

			printf("Shader failed\n");
			exit(0);
		}	break;
	}
}