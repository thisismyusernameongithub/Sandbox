#ifndef PIPEM
#define PIPEM

#include <stdio.h>
 #include <vector>
 #include <cmath>

//H = saves water flow in two directions and water depth and terrain height
//index 0 is water depth, 1 terrain height, 2 flow in positive x dir, 3 flow in positive y dir
//HB is a buffer for H
//the simulation tends to get unstable, if it does try lowering dTime by some factor  

namespace PIPE{

	void setup();
	void flow_update();
	void depth_update();
	
	const int width = 256;
	const int height = 256;
	float H[4*width*height]; //water height and flow direction index 
	float HB[4*width*height]; // buffer
	float A = 1; // cross sectional area of pipes
	float g = 9.81; // gravitational acceleration		
	float l = 1; //lenght of virtual pipe				
	float dTime = 0.05; //delta time
	float friction = 0.99; 
	float totalVolume = 0;
	
	void setup(){
		
	}				
	 
	 
	void flow_update(){
		
		for(int y=1;y<height-1;y++){
			for(int x=1;x<width-1;x++){
				//calculate outflow Q
				float outflow = 0;
				// Q += Area of pipe * gravity / pipe length * height difference * delta time
	if(H[0+x*4+y*width*4]>0||H[0+(x+1)*4+y*width*4]>0||H[0+(x-1)*4+y*width*4]>0||H[0+x*4+(y+1)*width*4]>0||H[0+x*4+(y-1)*width*4]>0){
				//Right
				if(x+1 <= width){
					float waterDiff = H[0+x*4+y*width*4]-H[0+(x+1)*4+y*width*4];
					float terrainDiff = H[1+x*4+y*width*4]-H[1+(x+1)*4+y*width*4];
					HB[2+x*4+y*width*4] = H[2+x*4+y*width*4]*friction + A*g*(waterDiff+terrainDiff)/l*dTime;
					if(H[0+x*4+y*width*4] <= 0 && HB[2+x*4+y*width*4] > 0 || H[0+(x+1)*4+y*width*4] <= 0 && HB[2+x*4+y*width*4] < 0) HB[2+x*4+y*width*4] = 0;
		//			if(std::abs(HB[2+x*4+y*width*4]) < 0.0000 && H[0+x*4+y*width*4] <= 0.001){ HB[2+x*4+y*width*4] = 0; H[0+x*4+y*width*4] = 0;}
				}
				//Down
				if(y+1 <= height){
					float waterDiff = H[0+x*4+y*width*4]-H[0+(x)*4+(y+1)*width*4];
					float terrainDiff = H[1+x*4+y*width*4]-H[1+(x)*4+(y+1)*width*4];
					HB[3+x*4+y*width*4] = H[3+x*4+y*width*4]*friction + A*g*(waterDiff+terrainDiff)/l*dTime;
					if(H[0+x*4+y*width*4] <= 0 && HB[3+x*4+y*width*4] > 0 || H[0+x*4+(y+1)*width*4] <= 0 && HB[3+x*4+y*width*4] < 0) HB[3+x*4+y*width*4] = 0;
			//		if(std::abs(HB[3+x*4+y*width*4]) < 0.0000 && H[0+x*4+y*width*4] <= 0.001){ HB[3+x*4+y*width*4] = 0;H[0+x*4+y*width*4] = 0;}
				}
				//border conditions
				if(x == width-2){
					HB[2+x*4+y*width*4] = 0;
				}
				if(y == height-2){
					HB[3+x*4+y*width*4] = 0;
				}
				if(x == 2){
					HB[2+(x-1)*4+y*width*4] = 0;
				}
				if(y == 2){
					HB[3+(x)*4+(y-1)*width*4] = 0;
				}

				}
			}
		}
		//make sure the same value is flowing in the same pipe
		for(int y=1;y<height-1;y++){
			for(int x=1;x<width-1;x++){
				//scale flux
				if(HB[2+x*4+y*width*4]+HB[3+x*4+y*width*4]-HB[2+(x-1)*4+y*width*4]-HB[3+x*4+(y-1)*width*4] > H[0+x*4+(y)*width*4]){
			//	std::cout << "outflow " << H[2+x*4+y*width*4]+H[3+x*4+y*width*4]-H[2+(x-1)*4+y*width*4]-H[3+x*4+(y-1)*width*4] << std::endl;
				float K = std::min(1.f, l*l*H[0+x*4+y*width*4]/(HB[2+x*4+y*width*4]+HB[3+x*4+y*width*4]-HB[2+(x-1)*4+y*width*4]-HB[3+x*4+(y-1)*width*4]));
				H[2+x*4+y*width*4] = K*HB[2+x*4+y*width*4];
				H[3+x*4+y*width*4] = K*HB[3+x*4+y*width*4];
				H[2+(x-1)*4+y*width*4] = K*HB[2+(x-1)*4+y*width*4];
				H[3+x*4+(y-1)*width*4] = K*HB[3+x*4+(y-1)*width*4];
			//	std::cout << "adjusted outflow " << H[2+x*4+y*width*4]+H[3+x*4+y*width*4]-H[2+(x-1)*4+y*width*4]-H[3+x*4+(y-1)*width*4] << std::endl;
			//	std::cout << "water left " << H[0+x*4+y*width*4] -(H[2+x*4+y*width*4]+H[3+x*4+y*width*4]-H[2+(x-1)*4+y*width*4]-H[3+x*4+(y-1)*width*4]) << std::endl;
				
				}else{
				H[2+x*4+y*width*4] = HB[2+x*4+y*width*4];
				H[3+x*4+y*width*4] = HB[3+x*4+y*width*4];
				H[2+(x-1)*4+y*width*4] = HB[2+(x-1)*4+y*width*4];
				H[3+x*4+(y-1)*width*4] = HB[3+x*4+(y-1)*width*4];	
				}
//				if(std::abs(H[1+x*4+y*width*4]) > std::abs(H[3+(x+1)*4+y*width*4])) H[1+x*4+y*width*4] = -H[3+(x+1)*4+y*width*4]; 
//				if(std::abs(H[2+x*4+y*width*4]) > std::abs(H[4+(x)*4+(y-1)*width*4])) H[2+x*4+y*width*4] = -H[4+(x)*4+(y-1)*width*4];   
//				if(std::abs(H[3+x*4+y*width*4]) > std::abs(H[1+(x-1)*4+y*width*4])) H[3+x*4+y*width*4] = -H[1+(x-1)*4+y*width*4];   
//				if(std::abs(H[4+x*4+y*width*4]) > std::abs(H[2+(x)*4+(y+1)*width*4])) H[4+x*4+y*width*4] = -H[2+(x)*4+(y+1)*width*4];   
			}
		}
		
	}
	
	void depth_update(){
		//update depth
		// depth = sum of flows / area of square * delta time
		for(int y=1;y<height-1;y++){
			for(int x=1;x<width-1;x++){
				//H[0+x*4+y*width*4] += (-H[2+x*4+y*width*4]-H[3+x*4+y*width*4]+H[2+(x-1)*4+y*width*4]+H[3+x*4+(y-1)*width*4])*dTime;
										H[0+x*4+y*width*4] += -H[2+x*4+y*width*4];
										H[0+x*4+y*width*4] += -H[3+x*4+y*width*4];
				if(x-1 > 0) H[0+x*4+y*width*4] += H[2+(x-1)*4+y*width*4];
				if(y-1 > 0) H[0+x*4+y*width*4] += H[3+x*4+(y-1)*width*4];
				if(H[0+x*4+y*width*4] < 0.00000) H[0+x*4+y*width*4] = 0;
				
				

			}
		}
	}
	void calculate_volume(){
		totalVolume = 0;
		for(int y=0;y<height;y++){
			for(int x=0;x<width;x++){	
				totalVolume += H[0+x*4+y*width*4];
			}
		}
	}
	
	void calculate_curl(){
		for(int y=0;y<height;y++){
			for(int x=0;x<width;x++){	
				float velX = H[2+(x)*4+y*width*4] - H[2+(x-1)*4+y*width*4];
				float velY = H[3+(x)*4+y*width*4] - H[3+x*4+(y-1)*width*4];
				//curl[x+y*Lx] = velX + velY;
			}
		}
	}
	
}
#endif


