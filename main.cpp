#include <SDL2/SDL.h>
#include <iostream>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h> //rand
#include <string>
#include <sstream>
#include <math.h>
#include <chrono>
#include <ctime>
#include <vector>
#include "LABSWE.h"
#include "pipe-method.h"
#include "nmmintrin.h" // for SSE4.2



int windowSizeX = 1000;
int windowSizeY = 600;
int rendererSizeX = 700;
int rendererSizeY = 600;

#define MAP_WIDTH 256
#define MAP_HEIGHT 256

SDL_Window * window;
SDL_Renderer * renderer;

Uint32 getpixel(SDL_Surface *surface, int x, int y);//gets pixel from surface, used of loading maps from png
double dTime = 1;
double Time = 1;




class rgb{
public:
	Uint8 r;
	Uint8 g;
	Uint8 b;
};

class vec2{
public:
	double x;
	double y;
};

class vec2Int{
public:
	int x;
	int y;
};

class vec3{
public:
	double x, y, z;
};

class tri{
public:
	vec2 a;
	vec2 b;
	vec2 c;
};

class mouse{
	public:
	vec2Int posScreen;
	vec2Int posWorld;
	float radius = 2;
	float amount = 2;
};

//https://flatuicolors.com/palette/defo
rgb colorScheme[13] =  {{(Uint8)131,(Uint8)178,(Uint8)208}, //Blue
												{(Uint8)149,(Uint8)218,(Uint8)182}, //Green
												{(Uint8)242,(Uint8)230,(Uint8)117}, //Yellow
												{(Uint8)220,(Uint8)133,(Uint8)128}, //Red
												{ (Uint8)44, (Uint8)62, (Uint8)80}, //Dark blue   ---Background
												{ (Uint8)52, (Uint8)73, (Uint8)94}, //Dark blue light
												{ (Uint8)39,(Uint8)174, (Uint8)96}, //Green
												{ (Uint8)46,(Uint8)204,(Uint8)113}, //Green light
												{ (Uint8)41,(Uint8)128,(Uint8)185}, //Blue
												{ (Uint8)52,(Uint8)152,(Uint8)219}, //Blue light
												{(Uint8)192, (Uint8)57, (Uint8)43}, //Red
												{(Uint8)231, (Uint8)76, (Uint8)60},//Red light
												{(Uint8)189,(Uint8)195,(Uint8)199}}; //light gray ---Highlights


class texture{
public:
	SDL_Texture * Texture;
	void * mPixels;
	Uint32 * pixels;
	int w; 
	int h;
	int pitch;
	
	texture(int width, int height)
	{
		//setup texture and array of pixels
		w = width;
		h = height;
		pitch = w * sizeof(Uint32);
		pixels = new Uint32[w * h];
		mPixels = NULL;
	}
	
	void set(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a){
		pixels[x + y * w] = (a << 24) | (r << 16) | (g << 8) | (b);
	}
	
	void init(bool transparent){
		Texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
		if(transparent){
			SDL_SetTextureBlendMode(Texture,SDL_BLENDMODE_BLEND);
		}
	}
	
	void lock(void){
		mPixels = NULL;
		SDL_LockTexture(Texture,NULL,&mPixels, &pitch);
		pixels = (Uint32*)mPixels;
	}
	
	void unlock(void){
		SDL_UnlockTexture(Texture);
	}
	
};

texture scrTexture(windowSizeX,windowSizeY);
texture rendTexture(rendererSizeX,rendererSizeY);
texture hudTexture(windowSizeX,windowSizeY);


class Font{
//0 börjar på 136, 8x8, 95 tecken
public:
	int w, h, n;
	Uint8 * fontMap;
	SDL_Surface* gPNGSurface = NULL; //will contain loaded pngs
	Font(int width, int height, int lenght){
		w = width;
		h = height;
		n = lenght;
		fontMap = new Uint8[h*n];
	}
	//load font
	void load(char *adress){
		std::cout << "loading font: ";
		std::cout << adress << std::endl;
		gPNGSurface = IMG_Load(adress);
		SDL_LockSurface(gPNGSurface);
		for(int N = 0; N < n; N++){
			for(int y = 0; y < h; y++){
				for(int x = 0; x < w; x++){
					Uint8 r,g,b;
					SDL_GetRGB(getpixel(gPNGSurface,1+x+N*(w+1),y+1),gPNGSurface->format,&r,&g,&b);
					if(r == 252){ //might need adjustment depending on the font
						fontMap[y+N*h] |= (0b00000001 << x); //save each 8px row as one byte
					}
				}
			}
		}
		SDL_UnlockSurface(gPNGSurface);
	}
	//print std::string at coordinates
	void print(texture * tex, std::string str,int x, int y){
		for(int n=0;n<str.length();n++){ //iterate over each char in the string and print from fontMap
			for(int i=0;i<8;i++){
				for(int j=0;j<8;j++){
					if(fontMap[i+(w*((int)str[n]-32))] & (1 << j)){ //fontMap starts on ! so offset by 33
						tex->set(x+j+n*(w+1),y+i,255,255,255,255);
					}
				}
			}
		}
	}
	
};
	Font font(8,8,95);

class Button{
public:
	int w,h,x,y;
	int pressed = 0;
	int hover = 0;
	std::string	text = "";
	int color = 6; //deafult green

//	if(Mouse.posWorld.x+Mouse.posWorld.y*Map.w<Map.w*Map.h)posString.append(std::to_string(PIPE::totalVolume));
//	posString.append(")");
//	font.print(&hudTexture,posString , 5,hudTexture.h-60);
	
	Button(int posX, int posY, int width, int height, char * Color, char * Text){
		w = width;
		h = height;
		x = posX;
		y = posY;
		text = Text;
		if(Color == "DARKBLUE") color = 4;
		if(Color == "GREEN") color = 6;
		if(Color == "BLUE") color = 8;
		if(Color == "RED") color = 10;
	}
	

	
	void draw(texture * tex){
		//draw button
		for(int Y=0;Y<h;Y++){
			for(int X=0;X<w;X++){
				tex->pixels[(x+X)+(y+Y)*tex->w] = (255<<24) | ((colorScheme[color+pressed].r+hover*30)<<16) | ((colorScheme[color+pressed].g+hover*30)<<8) | (colorScheme[color+pressed].b+hover*30);
			}
		}
		//draw text centered
		font.print(tex,text,x+w/2-text.length()*(font.w+1)/2,y+h/2-font.h/2);

	}
};
//ui buttons
Button BUTTON_terrain(700+10,10,86,60,"GREEN","Terrain");
Button BUTTON_water(700+106,10,86,60,"BLUE","Water");
Button BUTTON_reset(700+202,10,86,60,"RED","Reset");

class Slider{
public:
	int x,y,w,h;
	std::string text;
	float * pvar;
	float min,max;
	
	Slider(int posX, int posY, int width, int height, float minValue, float maxValue, float * variablePointer, char * Text){
		x = posX;
		y = posY;
		w = width;
		h = height;
		min = minValue;
		max = maxValue;
		text = Text;
		pvar = variablePointer;
	}
	

	void draw(texture * tex){
		//draw slider
		for(int Y=0;Y<h;Y++){
			for(int X=0;X<w;X++){
				//draw whole slider background
				if(X < (*pvar)/(max-min)*w){
					tex->pixels[(x+X)+(y+Y)*tex->w] = (255<<24) | ((colorScheme[8].r)<<16) | ((colorScheme[8].g)<<8) | (colorScheme[8].b);
				}else{
					tex->pixels[(x+X)+(y+Y)*tex->w] = (255<<24) | ((colorScheme[12].r)<<16) | ((colorScheme[12].g)<<8) | (colorScheme[12].b);
				}
			}
		}
		//draw text centered
		std::string output = text;
		output.append(": ");
		output.append(std::to_string(*pvar));
		font.print(tex,output,x+w/2-output.length()*(font.w+1)/2,y+h/2-font.h/2);
		//draw variable value
		
	}
	
	void test(){
		*pvar = 10.f;
	}
};
	mouse Mouse;

Slider SLIDER_radius(700+10,80,280,10,0,256,&Mouse.radius,"radius");
Slider SLIDER_amount(700+10,100,280,10,0,10,&Mouse.amount,"amount");

bool InitEverything();
void Render();
void RunGame();
void reset();



class camera{
public:
	double x;
	double y;
	double z;
	vec2 oldpos;
	double rot;
	double pitch;
	int horizon;
	double zoom = 2;
	int scale_height;
	int draw_distance;
	bool isoview = true;
	vec3 cursor;
	tri cone; //cone of vision
	vec2 * coneBuffer;
	camera(int pos_x, int pos_y, double rotation, int pos_z, int horizon_height, int scale_heightwise, int drawDistance, float Zoom){
		x = pos_x;
		y = pos_y;
		z = pos_z;
		oldpos.x = x;
		oldpos.y = y;
		zoom = Zoom;
		rot = rotation;
		horizon = horizon_height;
		scale_height = scale_heightwise;
		draw_distance = drawDistance;
		coneBuffer = new vec2[drawDistance*drawDistance*sizeof(int)];
	}
};

class droplet{
public:
	vec2 pos, vel;
};

class data{
public:
	Uint8 a,r,g,b;
	int waterLevel;
	float foamLevel;
	float height; //rock height map
	int shadow; //if position is in shadow or not
	vec2 slope; //slope
	float sediment; //sediment height map
	float groundHeight; //total groundheight in world coords
	float waterHeight; //waterheight in world coords
};

class map{
public:
	int w, h;
	data* pos;
	SDL_Surface* gPNGSurface = NULL; //will contain loaded pngs
	std::vector<droplet> droplets;
	std::vector<droplet> foam;
	void add_droplet(vec2 position, vec2 velocity){
		droplet t = {position,velocity};
		this->droplets.push_back(t);
	}
	void add_foam(vec2 position, vec2 velocity){
		droplet t = {position,velocity};
		this->foam.push_back(t);
	}

	map(int width, int height){
		w = width;
		h = height;
		pos = new data[w*h];
	}
	void load_colormap(char *adress){
		std::cout << "loading colormap: " << adress << std::endl;
		gPNGSurface = IMG_Load(adress);
		SDL_LockSurface(gPNGSurface);
		Uint8 r,g,b;
		for(int y=0;y<h;y++){
			for(int x=0;x<w;x++){
				SDL_GetRGB(getpixel(gPNGSurface,x,y),gPNGSurface->format,&r,&g,&b);
				pos[x+y*w].a = 255;
				pos[x+y*w].r = r;
				pos[x+y*w].g = g;
				pos[x+y*w].b = b;
			}
		}
		SDL_UnlockSurface(gPNGSurface);
	}
	void load_heightmap(char *adress){
		std::cout << "loading heightmap: " << adress << std::endl;
		gPNGSurface = IMG_Load(adress);
		SDL_LockSurface(gPNGSurface);
		Uint8 r,g,b;
		for(int y=0;y<h;y++){
			for(int x=0;x<w;x++){
				SDL_GetRGB(getpixel(gPNGSurface,x,y),gPNGSurface->format,&r,&g,&b);
				pos[x+y*w].height = r/255.f;
				pos[x+y*w].height = g/255.f;
				pos[x+y*w].height = b/255.f;
			}
		}
		SDL_UnlockSurface(gPNGSurface);
	}
	void generate_map(){ //generate map from parameters
		std::cout << "generating map: [-------------------------------]";
		for(int i=0; i<32;i++){std::cout << '\b';} //winds back the cursos to the beginning of the proggres bar
		int l = h/30; //how much an o on the progressbar is worth
		float max_val = 0; //saves max height to get medelvärde
		float min_val = 0; //saves min height to get medelvärde
		for(int y=0;y<h;y++){
			if(y % l == 1) std::cout << 'o' << std::flush; //progress the progressbar
			for(int x=0;x<w;x++){
				float val;

					 val = cos(y/30.f)+20*sin(x/40.f)+((x-w/2)*(x-w/2) + (y-h/2)*(y-h/2)) / 1000.f;
				pos[x+y*w].height = val;
				if(val > max_val) max_val = val; // save new highest height
				if(val < min_val) min_val = val; // save new lowest height
			}
		}
		for(int y=0;y<h;y++){
			for(int x=0;x<w;x++){
				//normalize height between 0-255 and use as color value
				pos[x+y*w].height = (pos[x+y*w].height - min_val) / (max_val - min_val);
				float val = 255 * (pos[x+y*w].height - min_val) / (max_val - min_val);
				pos[x+y*w].a = val;
				pos[x+y*w].r = val;
				pos[x+y*w].g = val;
				pos[x+y*w].b = val;
			}
		}
				std::cout << std::endl;
	}
	
	void generate_colormap(){
		/////////////////////////////////////
		
		
		///////////////////////////////////
		std::cout << "generating colormap: [-------------------------------]";
		for(int i=0; i<32;i++){std::cout << '\b';} //winds back the cursor to the beginning of the proggres bar
		int l = h/30; //how much an o on the progressbar is worth
		for(int y=0;y<h;y++){
			if(y % l == 1) std::cout << 'o' << std::flush; //progress the progressbar
			for(int x=0;x<w;x++){
				Uint8 r,g,b;
				double slopex = pos[x+y*w].slope.x;
				double slopey = pos[x+y*w].slope.y;
				double slope = sqrt(slopex*slopex+slopey*slopey);
				int height = pos[x+y*w].height;
				pos[x+y*w].a = 255;
				if(slope > 15){
					pos[x+y*w].r = 0xDC;
					pos[x+y*w].g = 0xE1;
					pos[x+y*w].b = 0xDE;
				}if(slope <= 15){
					pos[x+y*w].r = 0x49+((255-0x49)*pow(height/255,2));
					pos[x+y*w].g = 0xA0+((255-0xA0)*pow(height/255,2));
					pos[x+y*w].b = 0x78+((255-0x78)*pow(height/255,2));										
				}
				if(slope <= 20 && height > 50 && height < 100){
					pos[x+y*w].r = 0xBD;
					pos[x+y*w].g = 0xD8;
					pos[x+y*w].b = 0xB3;
				}
			}
		}
		std::cout << std::endl;
	}
		//calculate shadowmap
 void generate_shadowmap(){		
		vec3 sun = {w,h,1.5};
		vec3 lightdir;
		vec3 lightpos;
				int i=0;
		std::cout << "generating shadowmap: [-------------------------------]";
		for(int i=0; i<32;i++){std::cout << '\b';} //winds back the cursor to the beginning of the proggres bar
		int l = h/30; //how much an o on the progressbar is worth
		for(int y=0;y<h;y++){
			if(y % l == 1) std::cout << 'o' << std::flush; //progress the progressbar
			for(int x=0;x<w;x++){
				pos[x+y*w].shadow = 0;
				double z = pos[x+y*w].height + pos[x+y*w].sediment;
				lightdir.x = x - sun.x;
				lightdir.y = y - sun.y;
				lightdir.z = z - sun.z;
				lightpos.x = sun.x;
				lightpos.y = sun.y;
				lightpos.z = sun.z;
				//normalize
				double lightdir_length = sqrt(lightdir.x*lightdir.x+lightdir.y*lightdir.y+lightdir.z*lightdir.z);
				lightdir.x /=lightdir_length;
				lightdir.y /=lightdir_length;
				lightdir.z /=lightdir_length;
				//walk the sun
				while(lightpos.x >= 0 && lightpos.x <= w && 
							lightpos.y >= 0 && lightpos.y <= h && 
							lightpos.z >= z){
							
								//increase position
								lightpos.x += lightdir.x;
								lightpos.y += lightdir.y;
								lightpos.z += lightdir.z;
								//check if hit ground
								if(lightpos.z+0.01 <= pos[(int)(lightpos.x)+(int)(lightpos.y)*w].height + pos[(int)(lightpos.x)+(int)(lightpos.y)*w].sediment){
									pos[x+y*w].shadow = 40;
									i++;
									break;
								}
					}
				}
			}
		
				std::cout << std::endl;
				
		}
	
	
	
			int totalDrops = 0;
	void erode(){
			float Pminslope = 0.01;
			float Pmaxslope = 10;
			float Pmaxdepth = -100; //max depth terrain can erode to (prevents super deep canyons)
			float Pcapacity = 4;
			float Pdeposition = 0.3; //how much of surplus sediment is deposited
			float Perosion = 0.3; //how much a droplet can erode a tile
			float Pgravity = 4;
			int Pmaxsteps = 100; //max number of steps for a droplet
			float Pevaporation = 0.01;
			float initialSpeed = 1;
			float initialWaterVolume = 1;
			float inertia = 0.1;
		
		for(int iter=0; iter < 1000; iter++){
			totalDrops++;
			float dirX_new = 0;
			float dirY_new = 0;
			float dirX_old = 0;
			float dirY_old = 0;
			float x = 0;
			float y = 0;
			float hdiff = 0;
			float water = initialWaterVolume;
			float speed = initialSpeed;
			float sediment = 0;
			//get random starting coordinates
			x = rand() % w;
			y = rand() % h;
			

			
				float sedup = 0;
				float sedown = 0;
			for(int step=0;step<Pmaxsteps;step++){
				int oldPosID = int(x)+int(y)*w;
				int oldX = int(x);
				int oldY = int(y);
				
				//get slope gradient
				float slopeX = 0, slopeY = 0;
				pos[oldPosID].slope.x = 0;
				pos[oldPosID].slope.y = 0;
				for(int i=-1;i<2;i++){ //y
					for(int j=-1;j<2;j++){ //x
						int m = y+i;
						int n = x+j;
						if(m > 0 && m < h && n > 0 && n < w){
							double dh = pos[oldPosID].height+pos[oldPosID].sediment - pos[n+m*w].height-pos[oldPosID].sediment; //height difference
							slopeX -= dh*j;
							slopeY -= dh*i;
							pos[oldPosID].slope.x += dh*j;
							pos[oldPosID].slope.y += dh*i;
						}
					}
				}
				//get new direction
				dirX_new = dirX_new * inertia - slopeX * (1 - inertia);
				dirY_new = dirY_new * inertia - slopeY * (1 - inertia);

				//normalize direction 
				float dirLenght = sqrt(dirX_new*dirX_new+dirY_new*dirY_new);
				if(dirLenght != 0){
					dirX_new = dirX_new / dirLenght;
					dirY_new = dirY_new / dirLenght;				
				}
				//get new pos by adding direction to old pos
				x += dirX_new;
				y += dirY_new;
				int newPosID = int(x)+int(y)*w;
				//check if droplet is still within the map
				if((dirX_new == 0 && dirY_new == 0) || x < 0 || x > w-1 || y < 0 || y > h-1 ){
					std::cout << "out of bounds" << std::endl;
					break;
				}
				//get height difference
				hdiff = pos[newPosID].height + pos[newPosID].sediment - pos[oldPosID].height - pos[oldPosID].sediment;
					//calculate new carrying capacity
					float capacity = std::min(std::max(-hdiff,Pminslope)*speed*water*Pcapacity,Pmaxslope);
					if(pos[newPosID].height + pos[newPosID].sediment < -1) capacity = Pminslope*speed*water*Pcapacity;
					//if drop carries more sediment then capacity
					if(sediment > capacity || hdiff > 0){
						if(hdiff > 0){ //uphill
							//deposit at old pos
							//fill the hole but not with more than hdiff
							float amount = std::min(hdiff,sediment);
							sediment -= amount;
							pos[oldPosID].sediment += amount;
							sedown += amount;
						}else{
							//drop (sediment - capacity)*Pdeposition at old pos
							float amount = (sediment - capacity)*Pdeposition;
							sediment -= amount;
							pos[oldPosID].sediment += amount;
							sedown += amount;
						}
					}else{ //if drop carries less sediment then capacity, pick up sediment 
						//get std::min((c-sediment)*Perosion,-hdiff) sediment from old pos
						float amount = std::min((capacity-sediment)*Perosion,-hdiff);
						sediment += amount;
						//pick up sediment from an area around the old pos using normal distribution
						for(int j=-4;j<=4;j++){
							for(int k=-4;k<=4;k++){
								if(oldX >= 0 && oldX <= w && oldY >= 0 && oldY <= h){
									float temp_amount = amount*(exp(-(k*k+j*j)/(2*8))/(2*3.14159265359*8))/1.14022; //amount to pick up
									if(pos[oldPosID].sediment > 0){//if there is sediment 
										float diff = pos[oldPosID+k+j*w].sediment - temp_amount;
										if(diff > 0){ //there is more sediment then is picked up
											pos[oldPosID+k+j*w].sediment -= temp_amount;
											sedup += temp_amount;
											temp_amount = 0;
										}else{//there is less sediment then is picked up
											temp_amount -= pos[oldPosID+k+j*w].sediment;
											sedup += temp_amount;
											pos[oldPosID+k+j*w].sediment = 0;
										}
									}
									sedup += temp_amount;
									pos[oldPosID+k+j*w].height -= temp_amount;//*exp(-(k*k+j*j)/(2))/(2*3.14159265359)/1.32429; //remove rest of sediment from rock
								}
							}
						}
						
					}
				//get new speed
				speed = sqrt(speed*speed+abs(hdiff)*Pgravity);
				//evaporate some water
				water = water*(1-Pevaporation);
		//		std::cout << "pos:" << x << "," << y << " dirX:" << dirX_new << " dirY:" << dirY_new << " speed:" << speed << std::endl;
		//		std::cout << " hdiff:" << hdiff  << " capacity:" << capacity << " sediment:"<< sediment << " water:" << water << std::endl;
			}
				//std::cout << "up:" <<  sedup << " down:" << sedown << std::endl;
		}
		std::cout << "total drops: " << totalDrops << std::endl;
		
		float rocksum=0;
		float sedsum =0;
		for(int y=0;y<w;y++){
		
			for(int x=0;x<w;x++){
				rocksum += pos[x+y*w].height;
				sedsum += pos[x+y*w].sediment;
				
			}
		}
		std::cout << "r:" << rocksum << "s:" << sedsum << "t:" << rocksum+sedsum << std::endl;
		float sum = 0;
		for(int j=-4;j<=4;j++){
			for(int k=-4;k<=4;k++){
				sum += exp(-(k*k+j*j)/(2*8))/(2*3.14159265359*8)/1.14022; //amount to pick up
			}
		}
		std::cout << sum << std::endl;
	}
	
	void generate_flowfield(){
		std::cout << "generating flowfield: [-------------------------------]";
		for(int i=0; i<32;i++){std::cout << '\b';}
		int l = h/30;
		for(int y=0;y<h;y++){
			if(y % l == 1) std::cout << 'o' << std::flush;
			for(int x=0;x<w;x++){
				pos[x+y*w].slope.x = 0;
				pos[x+y*w].slope.y = 0;
				for(int i=-1;i<2;i++){ //y
					for(int j=-1;j<2;j++){ //x
						int m = y+i;
						int n = x+j;
						if(m > 0 && m < h && n > 0 && n < w){
							double dh = pos[x+y*w].height - pos[n+m*w].height + pos[x+y*w].sediment - pos[n+m*w].sediment; //height difference
							pos[x+y*w].slope.x += dh*j;
							pos[x+y*w].slope.y += dh*i;
						}
					}
				}
			}
		}
		std::cout << std::endl;
	}
};



class fpsCounter{
public:
	rgb color[4] = {{(Uint8)131,(Uint8)178,(Uint8)208},
									{(Uint8)149,(Uint8)218,(Uint8)182},
									{(Uint8)242,(Uint8)230,(Uint8)117},
									{(Uint8)220,(Uint8)133,(Uint8)128}};
 int x,y,w,h; //coordinates for fps
 int tx, ty, tw, th; //coordinates for timers
 texture * pTex; //texture pointer
 int * history; //contains past fpses
 int * thistory; //contains past timers
 int max = 0; //saves highest fps 
 int frameCount = 0;
 int fps = 0;
 int nTimers;
 Uint32 time = 0;
 
 int * timers;
 int * tempTimers;
 std::string * timerNames;

 Font * pFont;
 
 void init(Font * font, texture * tex, int NumberOfTimers, int posX, int posY, int width, int height){
	 pTex = tex;
	 x = posX;
	 y = posY;
	 w = width;
	 h = height;
	 tx = x + w + 5;
	 ty = y;
	 tw = rendererSizeX - tx - 5;
	 th = h;
	 pFont = font;
	 nTimers = NumberOfTimers;
	 timers = new int[nTimers];
	 tempTimers = new int[nTimers];
	 timerNames = new std::string[nTimers];
	 history = new int[w];
	 for(int i=0;i<w;i++){
		 history[i] = 0;
	 }
	 thistory = new int[th * nTimers];
		for(int i=0;i<th*nTimers;i++){
		 thistory[i] = 0;
	 }
 }
 
	void timeStart(int id,char * name){
		std::stringstream ss;
		ss.str(name);
		timerNames[id] = ss.str();
	  auto now = std::chrono::high_resolution_clock::now();
		auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
		tempTimers[id] = nanos;
	}
 
 void timeEnd(int id){
	  auto now = std::chrono::high_resolution_clock::now();
		auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
		tempTimers[id] = nanos - tempTimers[id];
		timers[id] += tempTimers[id];	
 }
 

	
 void draw(){
	//draw box timers
	for(int i=-1;i<th+1;i++){
		for(int j=-1;j<tw+1;j++){
			pTex->set(tx+j,ty+i,0,0,0,100);
		} 
	} 
	//draw history timers
	
	for(int i=nTimers*th;i>0;i--){
		thistory[i] = thistory[i-nTimers];
	}
	for(int i=0;i<nTimers;i++){
		thistory[i] = timers[i];
	}
	
	for(int k=0;k<th;k++){
		Uint32 tot = 0;
		for(int i=0;i<nTimers;i++){
			tot += thistory[i+k*nTimers];
		}
		int nX = 0; //starting position for next timer
		for(int i=0;i<nTimers;i++){
			int width = tw * (double)thistory[i+k*nTimers]/(double)tot;
			//std::cout << tw << "*" << timers[i] << "/" << tot << "=" << width << std::endl;
				for(int j=0;j<width;j++){
					pTex->set(tx+nX+j,ty+th-k,colorScheme[i].r,colorScheme[i].g,colorScheme[i].b,200);
				}
			nX += width;
			timers[i] = 0; //reset timer
		}		
	}
	//draw legend
	for(int k=0;k<nTimers;k++){
		//draw colorbox
		for(int i=0;i<10;i++){
			for(int j=0;j<10;j++){
				pTex->set(tx+j,ty+i+10*k,colorScheme[k].r,colorScheme[k].g,colorScheme[k].b,255); 
			}
		}
		std::string timeString = std::to_string((float)thistory[k]/1000000.f);
		if((float)thistory[k]/1000000.f < 10){
			timeString.insert(0," ");
		}
		timeString.erase(4,10);
		std::string tempString = " ms";
		timeString.append(tempString);
		pFont->print(pTex,timerNames[k],tx+10,ty+1+10*k); //print nametag
		pFont->print(pTex,timeString,tx+10+180,ty+1+10*k); //print time
	}

	//draw box fps
	for(int i=-1;i<h+1;i++){
		for(int j=-1;j<w+1;j++){
			pTex->set(x+j,y+i,0,0,0,100);
		} 
	}
	//draw history fps
	for(int i=0;i<w;i++){
		for(int k=0;k<history[i];k++){
			pTex->set(x+w-i,y+h-k*h/max,255,255,255,200);
		}
	}
	//draw fps
	pFont->print(pTex,std::to_string(fps),x,y);
 }
 
 void update(){
	if(SDL_GetTicks()-time > 1000){ //if one second has pased
		//std::cout << SDL_GetTicks() << "-" << fps.time << std::endl;
		fps = frameCount; //save number of frames since last second
		//push back history array and adds current fps in front
		for(int i=w;i>0;i--){
			history[i] = history[i-1];
		}
		history[0] = fps; //add current fps to history
		if(max < fps) max = fps; //adjust highest number of fps if needed
		time = SDL_GetTicks(); //save current time
		//std::cout << fps.frameCount << std::endl; //print fps to console
		frameCount = 0; //reset frame counter
	}
 }
};

	map Map(MAP_WIDTH,MAP_HEIGHT);
	camera Cam(0,0,0,200,120,120,2000,1);
	fpsCounter fps;


float LOD = 1;

vec2 world2screen(double x, double y){
	double sinAlpha = sin(Cam.rot);
	double cosAlpha = cos(Cam.rot);

	//scale for zoom level
	double xs = x/Cam.zoom;
	double ys = y/Cam.zoom;
	//project

	//rotate 
	double xw = cosAlpha*(xs+(Cam.x-614.911)) + sinAlpha*(ys+(Cam.y-119.936)) - (Cam.x-614.911);
	double yw = -sinAlpha*(xs+(Cam.x-614.911)) + cosAlpha*(ys+(Cam.y-119.936)) - (Cam.y-119.936);
	//offset for camera position

	 xs = xw + Cam.x;
	ys = yw + Cam.y;
	
	xw = ((xs-ys)/sqrt(2));
	yw = ((xs+ys)/sqrt(6));
	return {xw,yw};
}

vec2 screen2world(double x,double y){
	double sinAlpha = sin(Cam.rot);
	double cosAlpha = cos(Cam.rot);
	double sq2d2 = sqrt(2)/2;
	double sq6d2 = sqrt(6)/2;
	//transform screen coordinates to world coordinates 
	double xw = sq2d2*x+sq6d2*y;
	double yw = sq6d2*y-sq2d2*x;
	xw = xw - Cam.x;
	yw = yw - Cam.y;


	//rotate view
	double xwr = cosAlpha*(xw+(Cam.x-614.911)) - sinAlpha*(yw+(Cam.y-119.936)) - (Cam.x-614.911);
	double ywr = sinAlpha*(xw+(Cam.x-614.911)) + cosAlpha*(yw+(Cam.y-119.936)) - (Cam.y-119.936);
//616 121
	//apply zoom
	xw = xwr*Cam.zoom;
	yw = ywr*Cam.zoom;
	
	
	//double xw = (cosAlpha*(Cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(Cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*Cam.zoom;
	//double yw = (sinAlpha*(Cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(Cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*Cam.zoom;
	return {xw,yw};
}

void renderCamera(){
	
	//clear screen texture with an gradient
	for(int y=0;y<rendererSizeY;y++){
		Uint8 r = (102+y)>>2;//67
		Uint8 g = (192+y)>>2;//157
		Uint8 b = (229+y)>>2;//197
		//fill in gradient row by row
		std::fill(rendTexture.pixels+y*rendTexture.w, rendTexture.pixels + (y+1)*rendTexture.w, ((r)<<16) | ((g)<<8) | (b));
	}
	
	//draw 3d
	if(!Cam.isoview){
		//precalculate viewing angle parameters
		double sinRot = sin(Cam.rot);
		double cosRot = cos(Cam.rot);
		
		//init visibility array
		int ybuffer[rendererSizeX];
		for(int i=0; i<rendererSizeX;i++){
			ybuffer[i] = rendererSizeY;
		}
		
		int n = 0;
			//draw from front to back where z is depth from camera in camera direction
			double dz = 1;
			double z = 1;
			
			while(z < Cam.draw_distance){

				//find start and end points of line on map fov=90
				double pleftX  = (-(cosRot*z) - sinRot*z) + Cam.x;
				double pleftY  = (  sinRot*z  - cosRot*z) + Cam.y;
				double prightX = (  cosRot*z  - sinRot*z) + Cam.x;
				double prightY = (-(sinRot*z) - cosRot*z) + Cam.y;
				

				//save cone of vision
				if(z - Cam.draw_distance < 1){
					Cam.cone.a.x = Cam.x;
					Cam.cone.a.y = Cam.y;
					Cam.cone.b.x = pleftX;
					Cam.cone.b.y = pleftY;
					Cam.cone.c.x = prightX;
					Cam.cone.c.y = prightY;
				}
				
				//segment line
				double dx = (prightX - pleftX) / (double)rendererSizeX;
				double dy = (prightY - pleftY) / (double)rendererSizeX;
				//raster line and draw a vertical line for each segment

				for(int i=0; i<rendererSizeX; i += LOD){
					//if(pleftX < 0) pleftX = 1024 - pleftX;//wrap coordinates
					//if(pleftY < 0) pleftY = 1024 - pleftY;
					//infinite//int mapSpaceCoord = (((int)pleftX % 1024) + ((int)pleftY % 1024)*Map.w);
					int mapSpaceCoord = (int)pleftX + (int)pleftY * Map.w;
					if(pleftX >= 0 && pleftX <= Map.w && pleftY >= 0 && pleftY <= Map.h){
						double height_on_screen = ((Cam.z - Map.pos[mapSpaceCoord].height) / (double)z * (double)Cam.scale_height) + Cam.horizon;
				//97
						if(height_on_screen < ybuffer[i] && height_on_screen > 0){
								Uint32 rgb = (Map.pos[mapSpaceCoord].r << 16) | (Map.pos[mapSpaceCoord].g << 8) | (Map.pos[mapSpaceCoord].b);
								for(int y=height_on_screen;y<ybuffer[i];y++){
									int screenSpaceY = y * rendTexture.w;
								for(int k=0;k<LOD;k++){
									//rendTexture.set(i+k,y,r,g,b,255); //set pixel
									rendTexture.pixels[i+k + screenSpaceY] = rgb; //faster way
								}
								
								//save coordinates for whatever cursor is pointing at
								if(i == rendererSizeX/2 && y == rendererSizeY/2){ 
									Cam.cursor.x = (int)pleftX;
									Cam.cursor.y = (int)pleftY;
									Cam.cursor.z = Map.pos[mapSpaceCoord].height;
									//std::cout << Cam.cursor.x << ", " << Cam.cursor.y << ", " << Cam.cursor.z << std::endl;
								}
							}
							

						
							ybuffer[i] = height_on_screen;
						}
						

						
					}
					pleftX += dx*LOD;
					pleftY += dy*LOD;
				}

				double gg = ((Cam.z) / (double)z * (double)Cam.scale_height) + Cam.horizon;
				z += 1000.f/gg;//dz;
				dz += gg/1000000.f;//0.001*(LOD);
			}
	}
	else { //isometric rendering
		
		
		
		double xw,yw,zw;
		double xs,ys;

		//furstum
		xs = 0;
		ys = 0;
		vec2 ftl = screen2world(xs,ys);
		xs = rendTexture.w;
		ys = rendTexture.h;
		vec2 fbr = screen2world(xs,ys);
		xs = 0;
		ys = rendTexture.h;
		vec2 fbl = screen2world(xs,ys);
		xs = 0;
		ys = rendTexture.h+100/Cam.zoom;
		vec2 fblb = screen2world(xs,ys);
		
		double dxw = (fbr.x - fbl.x)/rendTexture.w; //delta x worldspace
		double dyw = (fbr.y - fbl.y)/rendTexture.w; //delta y worldspace
		double dDxw = (ftl.x - fbl.x)/rendTexture.h; //delta x worldspace depth
		double dDyw = (ftl.y - fbl.y)/rendTexture.h; //delta y worldspace depth
		

		//init visibility array and fill with screen height
		int ybuffer[rendTexture.w];
		std::fill(ybuffer, ybuffer + rendTexture.w, rendTexture.h);
		
	
		
		float camZoom = Cam.zoom;
		float camZoomDiv = 1/camZoom;
		double dropletlevel;
		float sed,hei,sloX,sloY;
		int water;

		Uint32 rgb;
		
		xw = fblb.x; //set world coords to coresponding coords for bottom left of screen
		yw = fblb.y; //then iterate left to right, bottom to top of screen
		for(int y=rendTexture.h+camZoomDiv*100;y>0;y--){ //the 100 offset is so terrain starts drawing a little bit below the screen
			double xwt = xw; //make a copy of world coordinate for leftmost position at current depth
			double ywt = yw;
			for(int x=0;x<rendTexture.w;x++){
				xs = x;
				ys = y;
				int ywti = (int)ywt;
				int xwti = (int)xwt;
				if(ywti > 0 && ywti < Map.h && xwti > 0 && xwti < Map.w){
					int posID = xwti + ywti*Map.w;
					sed = Map.pos[posID].sediment;
					hei = Map.pos[posID].height;
					Map.pos[posID].groundHeight = (hei + sed)*50.f;//get height

					Map.pos[posID].waterHeight = PIPE::H[xwti*4 + ywti*PIPE::width*4] * 50.f;//(LABSWE::h[(int)xwt + (int)ywt*LABSWE::Lx]*10000.f );

					float groundHeight = Map.pos[posID].groundHeight;
					float waterHeight = Map.pos[posID].waterHeight*500.f;

					dropletlevel = Map.pos[posID].waterLevel/10.f;
					
					ys = ys-groundHeight*camZoomDiv-dropletlevel; //offset y by terrain height
					water = 0;
					if(waterHeight > 0){
						water = 1;
						ys -= (waterHeight)*camZoomDiv;
					}
					if(ys < ybuffer[(int)xs]){ //check if position is obscured by terrain infront of it and draw if not
					if(xs > 0 && xs < rendTexture.w){ //check if position is within texture space
							//get color at worldspace and draw at screenspace 
							sloX = Map.pos[posID].slope.x;
							sloY = Map.pos[posID].slope.y;
							int r,g,b;
							
							//rgb = (Map.pos[posID].r << 16) | (Map.pos[posID].g << 8) | (Map.pos[posID].b);
								r = (std::max(std::min(100 + (int)(hei*1) + (int)(sloX*250) + (int)(sloY*250),200),55));
								g	=	(std::max(std::min(100 + (int)(hei*1) + (int)(sloX*250) + (int)(sloY*250),200),55)); 
								b =	(std::max(std::min(100 + (int)(hei*1) + (int)(sloX*250) + (int)(sloY*250),200),55));
								if(sed > 0){
									r -= std::min((int)(sed*255*4),63/8);
									g -= std::min(2*(int)(sed*255*4),127/8);
									b -= std::min(4*(int)(sed*255*4),255/8);
								}
							
							if(dropletlevel > 0){
								r = 0;
								g = 155 - (int)std::min(10*dropletlevel,100.d);
								b = 255 - (int)std::min(10*dropletlevel,255.d);
							}
							if(water){
								if(1){
								float slopX = -PIPE::H[(xwti+1)*4 + (ywti)*PIPE::width*4] + PIPE::H[(xwti-1)*4 + (ywti)*PIPE::width*4];
								float slopY = PIPE::H[(xwti)*4 + (ywti-1)*PIPE::width*4] - PIPE::H[(xwti)*4 + (ywti+1)*PIPE::width*4];
								
								r += (slopX+slopY)*500;
								g += (int)(2+waterHeight*camZoom*2+((slopX+slopY)*500));
								b += (int)(10+waterHeight*camZoom*10+((slopX+slopY)*500));
								if(slopX + slopY > 0.00000001 && slopX + slopY < 0.001){
									r += 10;
									g += 10;
									b += 10;
								}
								if(Map.pos[xwti+1 + ywti*PIPE::width].foamLevel > 0){
									r = 250;
									g = 250;
									b = 250; 
								}
									 
								}
	
								
							}
							//cursor
							if(round(sqrt((xwti-Mouse.posWorld.x)*(xwti-Mouse.posWorld.x) + (ywti-Mouse.posWorld.y)*(ywti-Mouse.posWorld.y))) == round(Mouse.radius)){
								r += 0;
								g += 50;
								b += 0;
							}
							//make borders of tiles darker, make it so they become darker the more zoomed in you are
							if(camZoom < 0.3 &&(xwt - floor(xwt) < 1.6*camZoom || ywt - floor(ywt) < 1.6*camZoom || ceil(xwt) - xwt < 1.6*camZoom || ceil(ywt) - ywt < 1.6*camZoom)){
								r -= (int)(1*camZoomDiv);  
								g -= (int)(1*camZoomDiv);
								b -= (int)(1*camZoomDiv);
							}
							//apply shadowmap
						//	rgb -= (Map.pos[(int)xwt + (int)ywt * Map.w].shadow << 16) | (Map.pos[(int)xwt + (int)ywt * Map.w].shadow << 8) | (Map.pos[(int)xwt + (int)ywt * Map.w].shadow);
							//clamp color values
							rgb = (std::min(std::max(r,0),255) << 16) | (std::min(std::max(g,0),255) << 8) | (std::min(std::max(b,0),255));

							for(int Y=ys;Y<ybuffer[(int)xs];Y++){
								
								//ide för optimisering, Omorganisera så pixlar sparas y + x*h ist så kan man använda std::fill sen för 
								//vertikala pixlar ligger brevid varandra i minnet då
								if(Y > 0 && Y < rendTexture.h ){
									rendTexture.pixels[(int)xs + Y*rendTexture.w] = rgb;	//draw pixels
								}
							}
						}
						ybuffer[(int)xs] = ys; //save current highest point in pixel column
					}

				}
				xwt += dxw; //update world coords corresponding to one pixel right
				ywt += dyw;
			}
			xw += dDxw; //update world coords corresponding to one pixel up
			yw += dDyw;

		}

		
	}
	


	//render droplets as pixels
	//buggad
	if(0){
		for(int i=0;i<Map.droplets.size();i++){
			double x = Map.droplets[i].pos.x;
			double y = Map.droplets[i].pos.y;
			vec2 pos = world2screen(x,y);
				double z = Map.pos[(int)x+(int)y*Map.w].height/Cam.zoom;
				int vel = 100+(abs(Map.droplets[i].vel.x) + abs(Map.droplets[i].vel.y))*2;
				if(vel>255) vel = 255;
				Uint32 rgb = ((vel)<<16) | ((vel)<<8) | (vel); //water is blue dabidee dabiduu
				rendTexture.pixels[(int)pos.x + (int)(pos.y-z)*rendTexture.w] = rgb;				
	//			
		}
	}

//draw water bodies
	if(0){
		for(int y=0;y<LABSWE::Ly;y += 1){
			for(int x=0;x<LABSWE::Lx;x += 1){
				//Uint32 rgb = ((int)((rho[x+y*MAP_WIDTH])*255*0.7)<<16) | ((int)((rho[x+y*MAP_WIDTH])*255*0.7)<<8) | (int)((rho[x+y*MAP_WIDTH])*255*0.7);
				int h = PIPE::H[(x)*4 + (y)*PIPE::width*4] * 2000;//LABSWE::h[x+y*LABSWE::Lx] * 40000 + LABSWE::mask[x+y*LABSWE::Lx]*10;
				int u = PIPE::H[2+(x)*4 + (y)*PIPE::width*4] - PIPE::H[2+(x-1)*4 + (y)*PIPE::width*4] * 2000;//LABSWE::u[x+y*LABSWE::Lx] * 40000 + LABSWE::mask[x+y*LABSWE::Lx]*10;
				int v = PIPE::H[3+(x)*4 + (y)*PIPE::width*4] - PIPE::H[3+(x)*4 + (y-1)*PIPE::width*4] * 2000;//LABSWE::v[x+y*LABSWE::Lx] * 40000 + LABSWE::mask[x+y*LABSWE::Lx]*10;
				Uint32 rgb = ((255<<24) | (std::min(abs(u),255)<<16) | (std::min(abs(v),255)<<8) | h);
				rendTexture.pixels[x/1 + (y/1+50)*rendTexture.w] = rgb;				
			}
		}
		
	}

//draw minimap
	if(0){
		for(int y=0;y<Map.h;y+=4){
				for(int x=0;x<Map.w;x+=4){
				Uint32 rgb = (Map.pos[x + y*Map.w].r << 16) | (Map.pos[x + y*Map.w].g << 8) | (Map.pos[x + y*Map.w].b);
				//rendTexture.pixels[x/4 + y/4*rendTexture.w] = rgb;	
			}
		}
	}
	
}
		


void renderHud(){

	
	//clear hud texture
	std::fill(hudTexture.pixels, hudTexture.pixels + hudTexture.w*hudTexture.h, 0);
	
	//fill menu
	for(int y=0;y<hudTexture.h;y++){
		std::fill(hudTexture.pixels+700+y*hudTexture.w, hudTexture.pixels+700+y*hudTexture.w + 300, ((255<<24)|(colorScheme[4].r<<16)|(colorScheme[4].g<<8)|(colorScheme[4].b)));
	}
	
	//draw minimap
	int mapWidth = 280;
	int mapHeight = 280;
	int mapX = 710;
	int mapY = hudTexture.h-400;
	float sX = Map.w / (float)mapWidth;
	float sY = Map.h / (float)mapHeight;
	for(int y=0;y<Map.h;y+=ceil(sY)){
		for(int x=0;x<Map.w;x+=ceil(sX)){
			int rgb = (255<<24) | ((5*(int)Map.pos[x+y*Map.w].groundHeight+50)<<16) | ((5*(int)Map.pos[x+y*Map.w].groundHeight+50)<<8) | ((int)Map.pos[x+y*Map.w].groundHeight*5+50);
			hudTexture.pixels[mapX+(int)((float)x/sX) + ((int)((float)y/sY)+mapY)*hudTexture.w] = rgb;	
		}
	}
	//draw rendered area on minimap
	double xs,ys;
		xs = 0;
		ys = 0;
		vec2 ftl = screen2world(xs,ys);
		xs = rendTexture.w;
		ys = 0;
		vec2 ftr = screen2world(xs,ys);
		xs = rendTexture.w;
		ys = rendTexture.h;
		vec2 fbr = screen2world(xs,ys);
		xs = 0;
		ys = rendTexture.h;
		vec2 fbl = screen2world(xs,ys);
		int rgb = (255<<24) | ((255)<<16) | ((255)<<8) | (255);
	for(int i=0;i<100;i++){
		float x = (ftl.x+(fbl.x-ftl.x)/100*i)/sX;
		float y = (ftl.y+(fbl.y-ftl.y)/100*i)/sY;
		if(x>0&&x<mapWidth&&y>0&&y<mapHeight)hudTexture.pixels[700+10+(int)(x) + ((int)(y)+hudTexture.h-400)*hudTexture.w] = rgb;		
		x = (fbl.x+(fbr.x-fbl.x)/100*i)/sX;
		y = (fbl.y+(fbr.y-fbl.y)/100*i)/sY;
		if(x>0&&x<mapWidth&&y>0&&y<mapHeight)hudTexture.pixels[700+10+(int)(x) + ((int)(y)+hudTexture.h-400)*hudTexture.w] = rgb;		
		x = (fbr.x+(ftr.x-fbr.x)/100*i)/sX;
		y = (fbr.y+(ftr.y-fbr.y)/100*i)/sY;
		if(x>0&&x<mapWidth&&y>0&&y<mapHeight)hudTexture.pixels[700+10+(int)(x) + ((int)(y)+hudTexture.h-400)*hudTexture.w] = rgb;
		x = (ftr.x+(ftl.x-ftr.x)/100*i)/sX;
		y = (ftr.y+(ftl.y-ftr.y)/100*i)/sY;
		if(x>0&&x<mapWidth&&y>0&&y<mapHeight)hudTexture.pixels[700+10+(int)(x) + ((int)(y)+hudTexture.h-400)*hudTexture.w] = rgb;
	}
		vec2 cen = screen2world(rendTexture.w/2,rendTexture.h/2);
		if(cen.x>0&&cen.x<mapWidth&&cen.y>0&&cen.y<mapHeight)hudTexture.pixels[700+10+(int)(cen.x) + ((int)(cen.y)+hudTexture.h-400)*hudTexture.w] = rgb;
	
	
	//draw buttons
	BUTTON_terrain.draw(&hudTexture);
	BUTTON_water.draw(&hudTexture);
	BUTTON_reset.draw(&hudTexture);
	//draw sliders
	SLIDER_radius.draw(&hudTexture);
	SLIDER_amount.draw(&hudTexture);
	
	//draw fps counter
	fps.draw();
	//draw mouse position
	std::string posString = "screenPos: (" ;
	posString.append(std::to_string(Mouse.posScreen.x));
	posString.append(",");
	posString.append(std::to_string(Mouse.posScreen.y));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-10);
	posString = "worldPos: (" ;
	posString.append(std::to_string(Mouse.posWorld.x));
	posString.append(",");
	posString.append(std::to_string(Mouse.posWorld.y));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-20);
	posString = "groundHeight: (" ;
	if(Mouse.posWorld.x+Mouse.posWorld.y*Map.w<Map.w*Map.h)posString.append(std::to_string(Map.pos[Mouse.posWorld.x+Mouse.posWorld.y*Map.w].groundHeight));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-30);
	posString = "waterHeight: (" ;
	if(Mouse.posWorld.x+Mouse.posWorld.y*Map.w<Map.w*Map.h)posString.append(std::to_string(Map.pos[Mouse.posWorld.x+Mouse.posWorld.y*Map.w].waterHeight));//Map.pos[Mouse.posWorld.x+Mouse.posWorld.y*Map.w].waterHeight));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-40);
	posString = "totalHeight: (" ;
	if(Mouse.posWorld.x+Mouse.posWorld.y*Map.w<Map.w*Map.h)posString.append(std::to_string(Map.pos[Mouse.posWorld.x+Mouse.posWorld.y*Map.w].waterHeight + Map.pos[Mouse.posWorld.x+Mouse.posWorld.y*Map.w].groundHeight));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-50);
	PIPE::calculate_volume();
	posString = "total water volume: (" ;
	if(Mouse.posWorld.x+Mouse.posWorld.y*Map.w<Map.w*Map.h)posString.append(std::to_string(PIPE::totalVolume));
	posString.append(")");
	font.print(&hudTexture,posString , 700+5,hudTexture.h-60);
	posString = "Cam.pos: " ;
	posString.append(std::to_string(Cam.x));
	posString.append(",");
	posString.append(std::to_string(Cam.y));
	font.print(&hudTexture,posString , 700+5,hudTexture.h-100);	
	
	posString = "Cam.zoom: " ;
	posString.append(std::to_string(Cam.zoom));
	font.print(&hudTexture,posString , 700+5,hudTexture.h-90);
	posString = "Cam.rot: " ;
	posString.append(std::to_string(Cam.rot));
	font.print(&hudTexture,posString , 700+5,hudTexture.h-80);
}

void Render()
{
	Uint32 tempTime = SDL_GetTicks(); //measyre render tajm
	
	// Clear the window and make it all black
	SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
	SDL_RenderClear( renderer );
	


	fps.timeStart(0,"renderCamera()");
	rendTexture.lock();
	renderCamera();
	rendTexture.unlock();
	fps.timeEnd(0);
	
	fps.timeStart(1,"renderHud()");
	hudTexture.lock();
	renderHud();
	hudTexture.unlock();
	fps.timeEnd(1);



	
	SDL_Rect test;
	test.x = 0;
	test.y = 0;
	test.w = 700;
	test.h = 600;

		
	SDL_RenderCopy(renderer,rendTexture.Texture,NULL,&test); //copy screen texture to renderer
	SDL_RenderCopy(renderer,hudTexture.Texture,NULL,NULL); //copy hud texture to renderer

	// Render the changes above
	SDL_RenderPresent( renderer);
	fps.frameCount++;
	fps.update();
}

void process(){
	
//	//foam
	//spawn foam where curl is high
	for(int y=0;y<PIPE::height;y++){
		for(int x=0;x<PIPE::width;x++){
				float velX = PIPE::H[2+(x)*4+y*PIPE::width*4] - PIPE::H[2+(x-1)*4+y*PIPE::width*4];
				float velY = PIPE::H[3+(x)*4+y*PIPE::width*4] - PIPE::H[3+x*4+(y-1)*PIPE::width*4];
				if(velX+velY > 0.01){
					//Map.add_foam({x,y},{0,0});
				}
		}
	}
	//update foam posistion
	for(int i=0;i<Map.foam.size();){
		int x = Map.foam[i].pos.x;
		int y = Map.foam[i].pos.y;
		if(x <= 0 || x >= Map.w || y <= 0 || y >= Map.h){
			Map.foam.erase(Map.foam.begin()+i);
		}else{
			i++; //only increase i if no foam is deleted
		}
	}
	for(int i=0;i<Map.foam.size();i++){
		int posX = Map.foam[i].pos.x;
		int posY = Map.foam[i].pos.y;
		float velX = PIPE::H[2+(posX)*4+posY*PIPE::width*4] - PIPE::H[2+(posX-1)*4+posY*PIPE::width*4];
		float velY = PIPE::H[3+(posX)*4+posY*PIPE::width*4] - PIPE::H[3+posX*4+(posY-1)*PIPE::width*4];
		if(Map.pos[posX + posY*Map.w].foamLevel > 0) Map.pos[posX + posY*Map.w].foamLevel -= 1; 
		//apply acceleration from water speed
		Map.foam[i].pos.x += velX*1000;
		Map.foam[i].pos.y -= velY*1000;
		posX = Map.foam[i].pos.x;
		posY = Map.foam[i].pos.y;
		Map.pos[posX + posY*Map.w].foamLevel += 1; 
	}
	
	//droplets

	//remove droplets out of bounds
	for(int i=0;i<Map.droplets.size();){
		int x = Map.droplets[i].pos.x;
		int y = Map.droplets[i].pos.y;
		if(x <= 0 || x >= Map.w || y <= 0 || y >= Map.h){
			Map.droplets.erase(Map.droplets.begin()+i);
		}else{
			i++; //only increase i if no droplet is deleted
		}
	}

	for(int i=0;i<Map.droplets.size();i++){
		int posX = Map.droplets[i].pos.x;
		int posY = Map.droplets[i].pos.y;
		//apply acceleration from slope
		Map.droplets[i].vel.x += Map.pos[posX+posY*Map.w].slope.x*dTime*1000;
		Map.droplets[i].vel.y += Map.pos[posX+posY*Map.w].slope.y*dTime*1000;
		double vel = sqrt(Map.droplets[i].vel.x*Map.droplets[i].vel.x+Map.droplets[i].vel.y*Map.droplets[i].vel.y);
		//apply friction
		int waterlevel = Map.pos[posX+posY*Map.w].waterLevel;
		if(Map.droplets[i].vel.x > 20) Map.droplets[i].vel.x -= 1*dTime*vel;
		if(Map.droplets[i].vel.y > 20) Map.droplets[i].vel.y -= 1*dTime*vel;
		if(Map.droplets[i].vel.x < -20) Map.droplets[i].vel.x += 1*dTime*vel;
		if(Map.droplets[i].vel.y < -20) Map.droplets[i].vel.y += 1*dTime*vel;
		//in water bodies

	}

	//aplly velocity
	for(int i=0;i<Map.droplets.size();i++){
		double x = Map.droplets[i].pos.x;
		double y = Map.droplets[i].pos.y;
		if(Map.pos[(int)x + (int)y*Map.w].waterLevel > 0) Map.pos[(int)x + (int)y*Map.w].waterLevel -= 1; 
		Map.droplets[i].pos.x += Map.droplets[i].vel.x*dTime;
		Map.droplets[i].pos.y += Map.droplets[i].vel.y*dTime;
		
		x = Map.droplets[i].pos.x;
		y = Map.droplets[i].pos.y;
		Map.pos[(int)x + (int)y*Map.w].waterLevel += 1; 
	}
	
	

}
bool moved = 0;
int mod = 1;
void RunGame()
{
	const Uint8 *state = SDL_GetKeyboardState(NULL); // get pointer to key states 
	vec2 pos = screen2world(rendTexture.w/2,rendTexture.h/2);
	Cam.cursor.x = pos.x;
	Cam.cursor.y = pos.y;
	
	
	
	bool loop = true;

	while ( loop )
	{
		//get mouse posistion in screen coordinates
		Uint32 mouseState = SDL_GetMouseState(&Mouse.posScreen.x,&Mouse.posScreen.y); // get pointer to key states 
		//get mouse position in world coordinates
		vec2 pos = screen2world(Mouse.posScreen.x,Mouse.posScreen.y);
		Mouse.posWorld.x = pos.x;
		Mouse.posWorld.y = pos.y;

		
		
		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			if ( event.type == SDL_QUIT )
				loop = false;
		}
		if(state[SDL_SCANCODE_ESCAPE]){
			loop = false;
		}
		
		
		
		//if mouse is on render area
		if(Mouse.posScreen.x < rendTexture.w ){
			
					//height aware cursor algorithm
		if(0){
			//get position of bottom of screen at mouse x coord
			vec2 posbot = screen2world(Mouse.posScreen.x,rendTexture.h);
			//get delta x and y 
			float dX = pos.x - posbot.x;
			float dY = pos.y - posbot.y;
			//normalize 
			float dis = sqrt(dX*dX+dY*dY);
			dX /= dis;
			dY /= dis;
			//walk a line from bottom to mouse
			float x = posbot.x;
			float y = posbot.y;
			int thisisaunnecesaryboolbutiamabadprogrammer = 1;
			while(thisisaunnecesaryboolbutiamabadprogrammer){
				x += dX;
				y += dY;
				float height = 0;
				if(x<=0&&x<Map.w&&y>=0&&y<Map.h) height = Map.pos[(int)x+(int)y*Map.w].height + Map.pos[(int)x+(int)y*Map.w].sediment;
				vec2 mousePos = world2screen(x,y);

			//	Map.pos[(int)x+(int)y*Map.w].sediment = 0.1;
				if(mousePos.y - height*50.f/Cam.zoom < Mouse.posScreen.y){
					thisisaunnecesaryboolbutiamabadprogrammer = 0;
					Mouse.posWorld.x = x;
					Mouse.posWorld.y = y;
				}
			}
			
		}
		
			if(mouseState == SDL_BUTTON_LEFT){
				
				
				if(BUTTON_terrain.pressed){
					float radius = Mouse.radius;
					for(int j=-radius*4;j<=radius*4;j++){
						for(int k=-radius*4;k<=radius*4;k++){
							if(Mouse.posWorld.x+k >= 0 && Mouse.posWorld.y+j >= 0 && Mouse.posWorld.x+k < Map.w && Mouse.posWorld.y+j < Map.h) Map.pos[Mouse.posWorld.x+k+(Mouse.posWorld.y+j)*Map.w].height += Mouse.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*dTime; //amount to pick up
						}
					}
					
					Map.generate_flowfield();

					for(int y=0;y<PIPE::height;y++){
						for(int x=0;x<PIPE::width;x++){
							PIPE::H[1+x*4+y*PIPE::width*4] = (Map.pos[x+y*PIPE::width].height + Map.pos[x+y*PIPE::width].sediment)/500.f;
						}
					}
				
				}
				if(BUTTON_water.pressed){
					float radius = Mouse.radius;
					for(int j=-radius;j<=radius;j++){
						for(int k=-radius;k<=radius;k++){
							//Map.pos[Mouse.posWorld.x+k+(Mouse.posWorld.y+j)*Map.w].height += 25*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime; //amount to pick up
							//LABSWE::f[9*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*9] +=  2*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime;
							//LABSWE::ftemp[9*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*9] +=  2*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime;
							PIPE::H[4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] +=  0.01*Mouse.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*dTime;
						}
					}
				}
			}
			if(mouseState == 4){				
						for(int i=0;i<1000;i++){
							int x = rand() % Map.w;
							int y = rand() % Map.h;

						//	Map.add_droplet({x,y},{0,0});
						}
				if(BUTTON_terrain.pressed){
					float radius = Mouse.radius;
					for(int j=-radius*4;j<=radius*4;j++){
						for(int k=-radius*4;k<=radius*4;k++){
							if(Mouse.posWorld.x+k >= 0 && Mouse.posWorld.y+j >= 0 && Mouse.posWorld.x+k < Map.w && Mouse.posWorld.y+j < Map.h) Map.pos[Mouse.posWorld.x+k+(Mouse.posWorld.y+j)*Map.w].height -= Mouse.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*dTime; //amount to pick up
						}
					}
					
					Map.generate_flowfield();

					for(int y=0;y<PIPE::height;y++){
						for(int x=0;x<PIPE::width;x++){
							PIPE::H[1+x*4+y*PIPE::width*4] = (Map.pos[x+y*PIPE::width].height + Map.pos[x+y*PIPE::width].sediment)/500.f;
						}
					}
				
				}
				if(BUTTON_water.pressed){
					float radius = Mouse.radius;
					for(int j=-radius;j<=radius;j++){
						for(int k=-radius;k<=radius;k++){
							//Map.pos[Mouse.posWorld.x+k+(Mouse.posWorld.y+j)*Map.w].height += 25*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime; //amount to pick up
							//LABSWE::f[9*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*9] +=  2*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime;
							//LABSWE::ftemp[9*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*9] +=  2*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime;
							PIPE::H[4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] -=  0.01*Mouse.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*dTime;
							if(PIPE::H[4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] <= 0){
								PIPE::H[0+4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] = 0;
								PIPE::H[2+4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] = 0;
								PIPE::H[3+4*(Mouse.posWorld.x+k)+(Mouse.posWorld.y+j)*PIPE::width*4] = 0;
							}
						}
					}
				}

			}
			if(state[SDL_SCANCODE_A]){
				if(!Cam.isoview){
					Cam.x -= cos(Cam.rot)*10;
					Cam.y += sin(Cam.rot)*10;
				}else{
					Cam.x += cos(Cam.rot-3.1415/4)*300 *dTime;
					Cam.y += sin(Cam.rot-3.1415/4)*300 *dTime;
				}
				moved = 1;
			}
			if(state[SDL_SCANCODE_D]){
				if(!Cam.isoview){
					Cam.x += cos(Cam.rot)*10;
					Cam.y -= sin(Cam.rot)*10;
				}else{
					Cam.x -= cos(Cam.rot-3.1415/4)*300 *dTime;
					Cam.y -= sin(Cam.rot-3.1415/4)*300 *dTime;
				}
				moved = 1;
			}
			if(state[SDL_SCANCODE_W]){
				if(!Cam.isoview){
					Cam.y -= cos(Cam.rot)*10;
					Cam.x -= sin(Cam.rot)*10;				
				}else{
					Cam.x -= sin(Cam.rot-3.1415/4)*450 *dTime;
					Cam.y += cos(Cam.rot-3.1415/4)*450 *dTime;
				}
				moved = 1;
			}
			if(state[SDL_SCANCODE_S]){
				if(!Cam.isoview){
					Cam.y += cos(Cam.rot)*10;
					Cam.x += sin(Cam.rot)*10;
				}else{
					Cam.x += sin(Cam.rot-3.1415/4)*450 *dTime;
					Cam.y -= cos(Cam.rot-3.1415/4)*450 *dTime;
				}
				moved = 1;
			}
			if(state[SDL_SCANCODE_R]){
				if(!Cam.isoview){
					Cam.z += 10;
					Cam.draw_distance += 10;
				}else{
					
					float rotation = Cam.rot; //save camera rotation 
					Cam.rot = 0; //set camera rotation to 0

					vec2 pos1 = screen2world(rendTexture.w/2,rendTexture.h/2);
					Cam.zoom += 1 *Cam.zoom *dTime;
					vec2 pos2 = world2screen(pos1.x,pos1.y);
					vec2 deltapos = {rendTexture.w/2-pos2.x,rendTexture.h/2 -pos2.y};
					//transform coordinate offset to isometric
					double sq2d2 = sqrt(2)/2;
					double sq6d2 = sqrt(6)/2;
					double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
					double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
					Cam.y += yw;
					Cam.x += xw;			
					Cam.rot = rotation; //restore camera rotation
				}
			}
			if(state[SDL_SCANCODE_F]){
				if(!Cam.isoview){
					Cam.z -= 10;
					Cam.draw_distance -= 10;
				}else{
					if(Cam.zoom > 0.03){
						float rotation = Cam.rot; //save camera rotation 
						Cam.rot = 0; //set camera rotation to 0

						vec2 pos1 = screen2world(rendTexture.w/2,rendTexture.h/2);
						Cam.zoom -= 1 *Cam.zoom *dTime;
						vec2 pos2 = world2screen(pos1.x,pos1.y);
						vec2 deltapos = {rendTexture.w/2-pos2.x,rendTexture.h/2 -pos2.y};
						//transform coordinate offset to isometric
						double sq2d2 = sqrt(2)/2;
						double sq6d2 = sqrt(6)/2;
						double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
						double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
						Cam.y += yw;
						Cam.x += xw;			
						Cam.rot = rotation; //restore camera rotation
						
					}
				}
			}
			if(state[SDL_SCANCODE_Q]){
				float angle = 32*3.14/180.f;
				Cam.rot -= angle *2 *dTime;
				if(!Cam.isoview){
					Cam.x -= cos(Cam.rot)*(Cam.z*Cam.horizon)/(5100*(Cam.cursor.z/mod+1));
					Cam.y += sin(Cam.rot)*(Cam.z*Cam.horizon)/(5100*(Cam.cursor.z/mod+1));				
					
				}
				moved = 1;
			}
			if(state[SDL_SCANCODE_E]){
				float angle = 32*3.14/180.f;
				Cam.rot += angle *2 *dTime;
				if(!Cam.isoview){
//					Cam.x += cos(Cam.rot)*(Cam.z/100);
//					Cam.y -= sin(Cam.rot)*(Cam.z/100);				
				}		
				moved = 1;
			}
			if(state[SDL_SCANCODE_P]){
				Map.erode();
				//LOD += 1;
			}
			if(state[SDL_SCANCODE_G]){
				Cam.horizon -= 10;
			}
			if(state[SDL_SCANCODE_T]){
				Cam.horizon += 10;
			}
			if(state[SDL_SCANCODE_M]){
				//Map.add_droplet({512,512},{0,0});
				//std::cout << int(Map.droplets.size()) << std::endl;
				float radius = 128;
				for(int j=-radius;j<=radius;j++){
					for(int k=-radius;k<=radius;k++){
						Map.pos[Mouse.posWorld.x+k+(Mouse.posWorld.y+j)*Map.w].height -= 255*exp(-(k*k+j*j)/(2.f*radius))/(2*3.14159265359*radius)*dTime; //amount to pick up
					}
				}
				Map.generate_flowfield();
				for(int y=0;y<PIPE::height;y++){
					for(int x=0;x<PIPE::width;x++){
						PIPE::H[1+x*4+y*PIPE::width*4] = (Map.pos[x+y*PIPE::width].height + Map.pos[x+y*PIPE::width].sediment)/10.f;
					//	LABSWE::force_x[x+y*PIPE::width] = Map.pos[x+y*PIPE::width].slope.x/3000.f;
					//	LABSWE::force_y[x+y*PIPE::width] = -Map.pos[x+y*PIPE::width].slope.y/3000.f;
					}
				}
			}
			if(state[SDL_SCANCODE_N]){
				//modfluidsquare(2,MAP_WIDTH/2,MAP_HEIGHT/2); //lägg till vätska
					for(int i=0;i<50;i++){
							int x = rand() % 4;
							int y = rand() % 4;

							Map.add_foam({Mouse.posWorld.x+2-x,Mouse.posWorld.y+2-y},{0,0});
					}
			}
			//generate shadowmap
			if(state[SDL_SCANCODE_B]){
			//Map.generate_shadowmap();
					std::cout << "H: " << PIPE::H[0+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] << std::endl;
					std::cout << "T: " << PIPE::H[1+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] << std::endl;
					std::cout << "outflow: " << PIPE::H[2+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] + PIPE::H[3+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] + PIPE::H[2+(Mouse.posWorld.x-1)*4+Mouse.posWorld.y*PIPE::width*4] +PIPE::H[3+Mouse.posWorld.x*4+(Mouse.posWorld.y-1)*PIPE::width*4]  << std::endl;
					std::cout << PIPE::H[2+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] << " " << PIPE::H[3+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4] << " " << PIPE::H[2+(Mouse.posWorld.x-1)*4+Mouse.posWorld.y*PIPE::width*4] << " " << PIPE::H[3+Mouse.posWorld.x*4+(Mouse.posWorld.y-1)*PIPE::width*4] << std::endl;
									float waterDiff = PIPE::H[0+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4]-PIPE::H[0+(Mouse.posWorld.x+1)*4+Mouse.posWorld.y*PIPE::width*4];
					std::cout << "n " << PIPE::H[2+Mouse.posWorld.x*4+Mouse.posWorld.y*PIPE::width*4]*PIPE::friction + PIPE::A*PIPE::g*(waterDiff)/PIPE::l*PIPE::dTime << std::endl;


		}
		}else{ //if mouse is over hud area
			//terrain
			if(Mouse.posScreen.x > BUTTON_terrain.x && Mouse.posScreen.x < BUTTON_terrain.x+BUTTON_terrain.w && Mouse.posScreen.y > BUTTON_terrain.y && Mouse.posScreen.y < BUTTON_terrain.y+BUTTON_terrain.h){
				BUTTON_terrain.hover = 1;
				if(mouseState == SDL_BUTTON_LEFT){
					BUTTON_terrain.pressed = 1;
					BUTTON_water.pressed = 0;
				}
			}else{
				BUTTON_terrain.hover = 0;
			}
			//water
			if(Mouse.posScreen.x > BUTTON_water.x && Mouse.posScreen.x < BUTTON_water.x+BUTTON_water.w && Mouse.posScreen.y > BUTTON_water.y && Mouse.posScreen.y < BUTTON_water.y+BUTTON_water.h){
				BUTTON_water.hover = 1;
				if(mouseState == SDL_BUTTON_LEFT){
					BUTTON_water.pressed = 1;
					BUTTON_terrain.pressed = 0;
				}
			}else{
				BUTTON_water.hover = 0;
			}
			//reset
			if(Mouse.posScreen.x > BUTTON_reset.x && Mouse.posScreen.x < BUTTON_reset.x+BUTTON_reset.w && Mouse.posScreen.y > BUTTON_reset.y && Mouse.posScreen.y < BUTTON_reset.y+BUTTON_reset.h){
				BUTTON_reset.hover = 1;
				if(mouseState == SDL_BUTTON_LEFT){
					reset();
				}
			}else{
				BUTTON_reset.hover = 0;
			}
			//slider radius
			if(Mouse.posScreen.x > SLIDER_radius.x && Mouse.posScreen.x < SLIDER_radius.x+SLIDER_radius.w && Mouse.posScreen.y > SLIDER_radius.y && Mouse.posScreen.y < SLIDER_radius.y+SLIDER_radius.h){
				if(mouseState == SDL_BUTTON_LEFT){
					*SLIDER_radius.pvar = (Mouse.posScreen.x-SLIDER_radius.x)/(float)(SLIDER_radius.w)*SLIDER_radius.max;
				}
			}
			//slider amount
			if(Mouse.posScreen.x > SLIDER_amount.x && Mouse.posScreen.x < SLIDER_amount.x+SLIDER_amount.w && Mouse.posScreen.y > SLIDER_amount.y && Mouse.posScreen.y < SLIDER_amount.y+SLIDER_amount.h){
				if(mouseState == SDL_BUTTON_LEFT){
					*SLIDER_amount.pvar = (Mouse.posScreen.x-SLIDER_amount.x)/(float)(SLIDER_amount.w)*SLIDER_amount.max;
				}
			}
				
		}

		fps.timeStart(2,"process()");
	//	process();
		fps.timeEnd(2);
		
		fps.timeStart(3,"simulate()");
		


		PIPE::flow_update();
		PIPE::depth_update();
		
		///
		fps.timeEnd(3);
		
		Render();
		

		dTime = (SDL_GetTicks() - Time)/1000;
		Time = SDL_GetTicks();
		LABSWE::dTime = dTime;
		PIPE::dTime = dTime;
	}

}
void reset(){
	//Map.load_colormap("/home/david/Documents/cpp/3dSDL/1k.png");
	
	//Map.load_heightmap("/home/david/Documents/cpp/Sandbox/heightmap1.png");
	//Map.generate_map();
	for(int y=0;y<Map.h;y++){
		for(int x=0;x<Map.w;x++){
			Map.pos[x+y*Map.w].height = 0.f;
			Map.pos[x+y*Map.w].sediment = 0.f;
			//Map.pos[x+y*Map.w].height = x/400.f;
			//if(x > 100 && x < 110 || x > 170 && x < 180 || y > 30 && y < 40 || y > 80 && y < 90 || x < 10 || x > 250 || y < 10 || y > 120) Map.pos[x+y*Map.w].height -= 0.35;
		}	
	}
	Map.generate_flowfield();
	Map.generate_colormap();
	
	
	
	for(int y=0;y<PIPE::height;y++){
		for(int x=0;x<PIPE::width;x++){
			PIPE::H[0+x*4+y*PIPE::width*4] = 0.0;
			PIPE::H[1+x*4+y*PIPE::width*4] = (Map.pos[x+y*Map.w].height+Map.pos[x+y*Map.w].sediment)/500.f;
			PIPE::H[2+x*4+y*PIPE::width*4] = 0.0;
			PIPE::H[3+x*4+y*PIPE::width*4] = 0.0;
			PIPE::HB[0+x*4+y*PIPE::width*4] = 0.0;
			PIPE::HB[1+x*4+y*PIPE::width*4] = (Map.pos[x+y*Map.w].height+Map.pos[x+y*Map.w].sediment)/500.f;
			PIPE::HB[2+x*4+y*PIPE::width*4] = 0.0;
			PIPE::HB[3+x*4+y*PIPE::width*4] = 0.0;
		}
	}
}

int main( int argc, char* args[] )
{
	
	reset();
	///
	
	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )	{std::cout << " Failed to initialize SDL : " << SDL_GetError() << std::endl;}
	

	window = SDL_CreateWindow( "Server", 10, 10, windowSizeX, windowSizeY, 0 );
	if ( window == nullptr )	{std::cout << "Failed to create window : " << SDL_GetError();}

	renderer = SDL_CreateRenderer( window, -1, 0 );
	if ( renderer == nullptr )	{std::cout << "Failed to create renderer : " << SDL_GetError();	}
	

	// Set size of renderer 
	SDL_RenderSetLogicalSize( renderer, windowSizeX, windowSizeY );
	//Set blend mode of renderer
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// Set color of renderer to black
	SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
	
	rendTexture.init(false); //initiate screen texture
	hudTexture.init(true); //initiate HUD texture
	font.load("/home/david/Documents/cpp/3dSDL/VictoriaBold.png"); //load font lol
	fps.init(&font,&hudTexture,4,5,5,70,40);
	
	


	RunGame();
	
	//SDL_DestroyTexture(windowTexture);
	//delete[] scrPixels;
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	
	return 0;
}

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

////////////////////////


//Om jag vill ha en moses delar vattnet effekt kan jag skicka höjdkartan till pipe method 
//som högre än den är.