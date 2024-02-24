#ifndef WINDOW_H_
#define WINDOW_H_

#include <stdint.h>

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

#define ARGB(A,R,G,B) (argb_t){.a=A,.r=R,.g=G,.b=B}

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

vec2f_t normalizeVec2f(vec2f_t vector);

void drawPoint(Layer layer, int x, int y, argb_t color);

void drawLine(Layer layer, int xStart, int yStart, int xEnd, int yEnd, argb_t color);

#endif /* WINDOW_H_ */

