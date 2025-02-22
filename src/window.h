#ifndef WINDOW_H_
#define WINDOW_H_

#include <stdint.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x)*(M_PI/180.f))
#define RAD2DEG(x) ((x)*(180.f/M_PI))

#define errLog(...) \
	fprintf(stderr, "\nFile: %s, Function: %s, Line: %d, Note: %s\n", __FILE__, __FUNCTION__, __LINE__, printfLocal(__VA_ARGS__));

typedef struct{
	float x;
	float y;
	float z;
}vec3f_t;

typedef struct{
	int x;
	int y;
	int z;
}vec3i_t;


typedef struct{
	float x;
	float y;
}vec2f_t;

typedef struct{
	int x;
	int y;
}vec2i_t;

typedef struct{
	int x;
	int y;
	int w;
	int h;
}rect_t;

typedef struct{
	struct{
		int x;
		int y;
	}pos; //Screen coordinates of topleft corner of window
	struct{
		int w;
		int h;
	}size;
	struct{
		int w;
		int h;
	}drawSize;
	char title[100];
	struct{
		unsigned int fullScreen        : 1;
		unsigned int fullScreenDesktop : 1;
		unsigned int maximized         : 1;
		unsigned int minimized         : 1;
		unsigned int borderLess        : 1;
		unsigned int roundedCorners    : 1;
		unsigned int resizable         : 1;
		unsigned int alwaysOnTop       : 1;
		unsigned int vSync             : 1;
	}settings;
	struct{
		uint32_t ms1;
		uint32_t ms10;
		uint32_t ms100;
		uint32_t s1;
		struct{
			unsigned int ms1   : 1;
			unsigned int ms10  : 1;
			unsigned int ms100 : 1;
			unsigned int s1    : 1;
		}tick;
		double dTime;
		double fps;
	}time;
	int closeWindow;
}Window;

enum KeyState{
	eKEY_IDLE = 0,
	eKEY_PRESSED = 1,
	eKEY_HELD = 2,
	eKEY_RELEASED = 3
};


typedef struct{
	enum KeyState left;
	enum KeyState right;
	vec2i_t screenPos; //Mouse position in screen coordinates
	vec2i_t pos;
	vec2i_t dPos;
	int dWheel;
}Mouse;

typedef struct{
	enum KeyState LEFT;
	enum KeyState UP;
	enum KeyState DOWN;
	enum KeyState RIGHT;
	enum KeyState A;
	enum KeyState B;
	enum KeyState C;
	enum KeyState D;
	enum KeyState E;
	enum KeyState F;
	enum KeyState G;
	enum KeyState H;
	enum KeyState I;
	enum KeyState J;
	enum KeyState K;
	enum KeyState L;
	enum KeyState M;
	enum KeyState N;
	enum KeyState O;
	enum KeyState P;
	enum KeyState Q;
	enum KeyState R;
	enum KeyState S;
	enum KeyState T;
	enum KeyState U;
	enum KeyState V;
	enum KeyState W;
	enum KeyState X;
	enum KeyState Y;
	enum KeyState Z;
	enum KeyState num0;
	enum KeyState num1;
	enum KeyState num2;
	enum KeyState num3;
	enum KeyState num4;
	enum KeyState num5;
	enum KeyState num6;
	enum KeyState num7;
	enum KeyState num8;
	enum KeyState num9;
	enum KeyState ctrlLeft;
	enum KeyState shiftLeft;
	enum KeyState ESC;
}Key;

typedef struct{
	union{
		struct{
			uint8_t b;
			uint8_t g;
			uint8_t r;
			uint8_t a;
		};
		struct{
			uint32_t rgb : 24;
		};
		uint32_t argb;
	};
}argb_t;

#define ARGB(A,R,G,B) (argb_t){.a=(A),.r=(R),.g=(G),.b=(B)}
#define rgb(R,G,B) (argb_t){.a=(255),.r=(R),.g=(G),.b=(B)} //Not fond of this macro but if you're using VS Code and setting "Editor: Default Color Decorators = True", Then the color picker is enabled in editor when using this macro


typedef struct{
	argb_t * frameBuffer;
	int w;
	int h;
	int pitch;
	int depth;
}Layer;


extern Mouse mouse;
extern Key key;
extern Window window;

Window* window_init();
Layer window_createLayer(void);
void window_setTitle(char* title);
int window_run(void);
void drawText(Layer layer, int xPos, int yPos, char* string);
char* printfLocal(const char *format, ...);
void clearLayer(Layer layer);

//Returns time since application start
double getWindowTime(void);

void gaussBlurf(float *source, float *target, int source_lenght, int w, int h, int r);
void gaussBlurargb(argb_t *source, argb_t *target, int source_lenght, int w, int h, int r);
void gaussBlurargb2(argb_t *source, argb_t *target, int source_lenght, int w, int h, int r);
void gaussBlurargb3(argb_t *source, argb_t *target, int source_lenght, int w, int h, int r);

vec2f_t normalizeVec2f(vec2f_t vector);
vec3f_t normalizeVec3f(vec3f_t vector);
float dotProduct(vec3f_t v0, vec3f_t v1);
vec3f_t crossProduct(vec3f_t v0, vec3f_t v1);

void drawPoint(Layer layer, int x, int y, argb_t color);
void drawSquare(Layer layer, int x, int y, int w, int h, argb_t color);
void drawLine(Layer layer, int xStart, int yStart, int xEnd, int yEnd, argb_t color);


void testFunc(void);


static inline int maxi(const int a, const int b){
    return (a > b) ? a : b;
}

static inline int mini(const int a, const int b)
{
    return (a < b) ? a : b;
}

static inline float maxf(const float a, const float b)
{
    return (a > b) ? a : b;
}

static inline float minf(const float a, const float b)
{
    return (a < b) ? a : b;
}

static inline float clampf(const float value, const float min, const float max) 
{
    const float t = (value < min) ? min : value;
    return (t > max) ? max : t;
}

static inline int clampi(int value, int min, int max) {
    const int t = (value < min) ? min : value;
    return (t > max) ? max : t;
}

static inline float cosLerp(const float y1, const float y2, const float mu)
{
	const double mu2 = (1-cos(mu*M_PI))/2;
	return(y1*(1-mu2)+y2*mu2);
}

static inline float lerp(const float s, const float e, const float t)
{
	return s + (e - s) * t;
}

static inline float blerp(const float c00, const float c10, const float c01, const float c11, const float tx, const float ty)
{
	//    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
	const float s = c00 + (c10 - c00) * tx;
	const float e = c01 + (c11 - c01) * tx;
	return (s + (e - s) * ty);
}


static inline argb_t lerpargb(const argb_t s, const argb_t e, const float t)
{
	argb_t result;
	result.a = 255;
	result.r = lerp(s.r, e.r, t);
	result.g = lerp(s.g, e.g, t);
	result.b = lerp(s.b, e.b, t);
	return result;
}




typedef enum{
	eUNIFROMTYPE_FLOAT = 0x01,
	eUNIFROMTYPE_FLOAT_VEC2 = 0x02,
	eUNIFROMTYPE_FLOAT_VEC3 = 0x03,
	eUNIFROMTYPE_INT = 0x11,
	eUNIFROMTYPE_INT_VEC2 = 0x12,
	eUNIFROMTYPE_INT_VEC3 = 0x13,
}UniformType;

typedef struct{
	enum{
		eSHADERSTATE_UNINITIALIZED,
		eSHADERSTATE_INITIALIZED,
		eSHADERSTATE_FAILED
	}state;

	enum{
		eSHADERTYPE_FLOAT,
		eSHADERTYPE_ARGB
	}type;

	char* vertexShaderSource; 
	char* fragmentShaderSource; 

	int width;
	int height;

	struct{
		int no;
		struct{
			char* name;
			union{
				float*  ptr_f;
				argb_t* ptr_argb;
			};
		}data[16];
	}input;
	
	struct{
		int no;
		struct{
			char* name;
			UniformType type;
			union{
				float val_f;
				vec2f_t val_vec2f;
				vec3f_t val_vec3f;
				int val_i;
				vec2i_t val_vec2i;
				vec3i_t val_vec3i;
			};
		}data[16];
	}uniform;

	struct{
		int no;
		union{
			float* ptr_f;
			argb_t* ptr_argb;
		}data[16];
	}output;


	//Private variables (Should not be modified by the application, hide these in the future)
	uint32_t program;
	//element buffer object
	uint32_t EBO;
	//vertex buffer object
	uint32_t VBO;
	//Vertex array object
	uint32_t VAO;
	//framebuffer object
	uint32_t fbo;
	//Texture ID storage
	uint32_t textureIn[16];
	uint32_t textureOut[16];

}Shader;

uint32_t compileShaderProgram(char* vertexShaderPath, char* fragmentShaderPath);
void runShader(Shader* shader);

#endif /* WINDOW_H_ */

