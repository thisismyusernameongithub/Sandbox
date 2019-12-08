#ifndef LABSWE1
#define LABSWE1

#include <stdio.h>
 #include <vector>
 #include <cmath>
//a,x,y - Loop integers
//ex,ey - x and y components of particles' velocities
//f - Distribution function
//feq - local equilibrium distribution function
//force_x - x-direction component of force term
//force_y - y-direction component of force term
//ftemp - Temple distribution function
//gacl - Gravitational acceleration
//h - water depth
//Lx,Ly - Total lattice numbers in x and y directions
//nu - Molecular viscosity
//tau - Relaxation time
//u,v - x and y components of flow velocity

namespace LABSWE{
	
	void setup();
	void compute_feq();
	void collide_stream();
	void calculate_curl();
	
		const int Lx = 256;
		const int Ly = 128;

		float tau,nu,gacl = 9.81;
		float ex[9],ey[9];
		float u[Lx*Ly],v[Lx*Ly],h[Lx*Ly],force_x[Lx*Ly],force_y[Lx*Ly], mask[Lx*Ly],  curl[Lx*Ly];
		float f[9*Lx*Ly],feq[9*Lx*Ly],ftemp[9*Lx*Ly];		
		float dTime;

	
	void setup(){
		float quarter_pi = std::atan(1);
		//calculate molecular viscosity
		nu = (2*tau-1)/6.f;
		dTime = 1;
		//compute the particle velocities
		ex[1] =              std::cos(quarter_pi*0);
		ey[1] = 						 std::sin(quarter_pi*0);
		ex[2] = std::sqrt(2)*std::cos(quarter_pi*1);
		ey[2] = std::sqrt(2)*std::sin(quarter_pi*1);
		ex[3] =              std::cos(quarter_pi*2);
		ey[3] = 						 std::sin(quarter_pi*2);
		ex[4] = std::sqrt(2)*std::cos(quarter_pi*3);
		ey[4] = std::sqrt(2)*std::sin(quarter_pi*3);
		ex[5] =              std::cos(quarter_pi*4);
		ey[5] = 						 std::sin(quarter_pi*4);
		ex[6] = std::sqrt(2)*std::cos(quarter_pi*5);
		ey[6] = std::sqrt(2)*std::sin(quarter_pi*5);
		ex[7] =              std::cos(quarter_pi*6);
		ey[7] = 						 std::sin(quarter_pi*6);
		ex[8] = std::sqrt(2)*std::cos(quarter_pi*7);
		ey[8] = std::sqrt(2)*std::sin(quarter_pi*7);
		ex[0] = 0;
		ey[0] = 0;
		//compute the equilibrium distribution function feq
		compute_feq();
		//set the initial distribution function to feq
		for(int i=0;i<9*Lx*Ly;i++){
			f[i] = feq[i];
		}
	}
	
	void collide_stream(){
		//calculate distribution function with LABSWE
		//local integers
		int xf,yf,xb,yb;
		
		for(int y=0;y<Ly;y++){
			yf = y+1;
			yb = y-1;
			for(int x=0;x<Lx;x++){
				if(mask[x+y*Lx]){
				xf = x+1;
				xb = x-1;
				//following 4 lines implement periodic BCs
				if(xf>Lx) xf = xf - Lx;
				if(xb< 0) xb = Lx + xb;
				if(yf>Ly) yf = yf - Ly;
				if(yb< 0) yb = Ly + yb;
				//start streaming and collision
				if(!(xf>Lx||xb<0||yf>Ly||yb<0)){
				int ixf = xf*9;
				int ix  = x*9;
				int ixb = xb*9;
				int iyf = yf*Lx*9;
				int iy  = y*Lx*9;
				int iyb = yb*Lx*9;
				
				if(f[1+ixf+iy] != 0)  ftemp[1+ixf+iy]  = std::max(f[1+ix+iy] - (f[1+ix+iy]-feq[1+ix+iy])/(1*tau) + 1/6.f * (ex[1]*force_x[x+y*Lx]+ey[1]*force_y[x+y*Lx]),0.f);	
				if(f[2+ixf+iyb] != 0) ftemp[2+ixf+iyb] = std::max(f[2+ix+iy] - (f[2+ix+iy]-feq[2+ix+iy])/(1*tau) + 1/6.f * (ex[2]*force_x[x+y*Lx]+ey[2]*force_y[x+y*Lx]),0.f);
				if(f[3+ix+iyb] != 0)  ftemp[3+ix+iyb]  = std::max(f[3+ix+iy] - (f[3+ix+iy]-feq[3+ix+iy])/(1*tau) + 1/6.f * (ex[3]*force_x[x+y*Lx]+ey[3]*force_y[x+y*Lx]),0.f);				
				if(f[4+ixb+iyb] != 0) ftemp[4+ixb+iyb] = std::max(f[4+ix+iy] - (f[4+ix+iy]-feq[4+ix+iy])/(1*tau) + 1/6.f * (ex[4]*force_x[x+y*Lx]+ey[4]*force_y[x+y*Lx]),0.f);
				if(f[5+ixb+iy] != 0)  ftemp[5+ixb+iy]  = std::max(f[5+ix+iy] - (f[5+ix+iy]-feq[5+ix+iy])/(1*tau) + 1/6.f * (ex[5]*force_x[x+y*Lx]+ey[5]*force_y[x+y*Lx]),0.f);													
				if(f[6+ixb+iyf] != 0) ftemp[6+ixb+iyf] = std::max(f[6+ix+iy] - (f[6+ix+iy]-feq[6+ix+iy])/(1*tau) + 1/6.f * (ex[6]*force_x[x+y*Lx]+ey[6]*force_y[x+y*Lx]),0.f);	
				if(f[7+ix+iyf] != 0)  ftemp[7+ix+iyf]  = std::max(f[7+ix+iy] - (f[7+ix+iy]-feq[7+ix+iy])/(1*tau) + 1/6.f * (ex[7]*force_x[x+y*Lx]+ey[7]*force_y[x+y*Lx]),0.f);	
				if(f[8+ixf+iyf] != 0) ftemp[8+ixf+iyf] = std::max(f[8+ix+iy] - (f[8+ix+iy]-feq[8+ix+iy])/(1*tau) + 1/6.f * (ex[8]*force_x[x+y*Lx]+ey[8]*force_y[x+y*Lx]),0.f);	
				if(f[0+ix+iy] != 0)   ftemp[0+ix+iy]   = std::max(f[0+ix+iy] - (f[0+ix+iy]-feq[0+ix+iy])/(1*tau),0.f);
				}
				if(y==0){
					ftemp[6+x*9+y*Lx*9] = f[6+x*9+y*Lx*9]; 
					ftemp[7+x*9+y*Lx*9] = f[7+x*9+y*Lx*9]; 
					ftemp[8+x*9+y*Lx*9] = f[8+x*9+y*Lx*9]; 
				}
				if(y==Ly-1){
					ftemp[2+x*9+y*Lx*9] = f[2+x*9+y*Lx*9]; 
					ftemp[3+x*9+y*Lx*9] = f[3+x*9+y*Lx*9]; 
					ftemp[4+x*9+y*Lx*9] = f[4+x*9+y*Lx*9]; 
				}
				if(x==0){
					ftemp[2+x*9+y*Lx*9] = f[2+x*9+y*Lx*9]; 
					ftemp[1+x*9+y*Lx*9] = f[1+x*9+y*Lx*9]; 
					ftemp[8+x*9+y*Lx*9] = f[8+x*9+y*Lx*9]; 
				}
				if(x==Lx-1){
					ftemp[4+x*9+y*Lx*9] = f[4+x*9+y*Lx*9]; 
					ftemp[5+x*9+y*Lx*9] = f[5+x*9+y*Lx*9]; 
					ftemp[6+x*9+y*Lx*9] = f[6+x*9+y*Lx*9]; 
				}
			}
			}
		}
	}
	
	void solution(){
		//compute the physical variables h,u and v
		//set the distribution function f
		for(int i=0;i<9*Lx*Ly;i++){
			f[i] = ftemp[i];
		}
		//compute the  velocity and depth
		for(int i=0;i<Lx*Ly;i++){
			h[i] = 0;
			u[i] = 0;
			v[i] = 0;
		}
		
		for(int a=0;a<9;a++){
			for(int i=0;i<Lx*Ly;i++){
				h[i] = h[i] + f[a+i*9];
				u[i] = u[i] + ex[a]*f[a+i*9];
				v[i] = v[i] + ey[a]*f[a+i*9];
			}			
		}
		for(int i=0;i<Lx*Ly;i++){
		if(h[i] != 0) u[i] = u[i] / std::abs(h[i]);
		else u[i] = 0;
		if(h[i] != 0) v[i] = v[i] / std::abs(h[i]);
		else v[i] = 0;
		}
	}
	
	void compute_feq(){
		//compute local equilibrium distribution function
			for(int i=0;i<Lx*Ly;i++){
				feq[0+i*9] = h[i] - 5*gacl*h[i]*h[i]/6 - 2*h[i]/3.f * (u[i]*u[i] + v[i]*v[i]);
				feq[1+i*9] = gacl * (h[i]*h[i])/6.f + h[i]/3.f * (ex[1] * u[i] + ey[1] * v[i]) + h[i]/2.f *
					(ex[1]*u[i]*ex[1]*u[i] + 2.f * ex[1]*u[i]*ey[1]*v[i] + ey[1] * v[i] * ey[1] * v[i]) - h[i]/6.f * (u[i]*u[i] + v[i]*v[i]);
				feq[2+i*9] = gacl * (h[i]*h[i])/24.f + h[i]/12.f * (ex[2] * u[i] + ey[2] * v[i]) + h[i]/8.f *
					(ex[2]*u[i]*ex[2]*u[i] + 2.f * ex[2]*u[i]*ey[2]*v[i] + ey[2] * v[i] * ey[2] * v[i]) - h[i]/24.f * (u[i]*u[i] + v[i]*v[i]);
				feq[3+i*9] = gacl * (h[i]*h[i])/6.f + h[i]/3.f * (ex[3] * u[i] + ey[3] * v[i]) + h[i]/2.f *
					(ex[3]*u[i]*ex[3]*u[i] + 2.f * ex[3]*u[i]*ey[3]*v[i] + ey[3] * v[i] * ey[3] * v[i]) - h[i]/6.f * (u[i]*u[i] + v[i]*v[i]);
				feq[4+i*9] = gacl * (h[i]*h[i])/24.f + h[i]/12.f * (ex[4] * u[i] + ey[4] * v[i]) + h[i]/8.f *
					(ex[4]*u[i]*ex[4]*u[i] + 2.f * ex[4]*u[i]*ey[4]*v[i] + ey[4] * v[i] * ey[4] * v[i]) - h[i]/24.f * (u[i]*u[i] + v[i]*v[i]);
				feq[5+i*9] = gacl * (h[i]*h[i])/6.f + h[i]/3.f * (ex[5] * u[i] + ey[5] * v[i]) + h[i]/2.f *
					(ex[5]*u[i]*ex[5]*u[i] + 2.f * ex[5]*u[i]*ey[5]*v[i] + ey[5] * v[i] * ey[5] * v[i]) - h[i]/6.f * (u[i]*u[i] + v[i]*v[i]);
				feq[6+i*9] = gacl * (h[i]*h[i])/24.f + h[i]/12.f * (ex[6] * u[i] + ey[6] * v[i]) + h[i]/8.f *
					(ex[6]*u[i]*ex[6]*u[i] + 2.f * ex[6]*u[i]*ey[6]*v[i] + ey[6] * v[i] * ey[6] * v[i]) - h[i]/24.f * (u[i]*u[i] + v[i]*v[i]);
				feq[7+i*9] = gacl * (h[i]*h[i])/6.f + h[i]/3.f * (ex[7] * u[i] + ey[7] * v[i]) + h[i]/2.f *
					(ex[7]*u[i]*ex[7]*u[i] + 2.f * ex[7]*u[i]*ey[7]*v[i] + ey[7] * v[i] * ey[7] * v[i]) - h[i]/6.f * (u[i]*u[i] + v[i]*v[i]);
				feq[8+i*9] = gacl * (h[i]*h[i])/24.f + h[i]/12.f * (ex[8] * u[i] + ey[8] * v[i]) + h[i]/8.f *
					(ex[8]*u[i]*ex[8]*u[i] + 2.f * ex[8]*u[i]*ey[8]*v[i] + ey[8] * v[i] * ey[8] * v[i]) - h[i]/24.f * (u[i]*u[i] + v[i]*v[i]);
				
			}
	}

	void Noslip_BC(){
		//noslip boundary with bounce back scheme
		//for top boundary
		for(int a=2;a<=4;a++){
			for(int x=0;x<Lx;x++){
				ftemp[a+4+x*9+0*Lx*9] = ftemp[a+x*9+0*Lx*9];
				//ftemp[a+x*9+0*Lx*9] = 0;;
			}
		}
		//low
		for(int a=6;a<=8;a++){
			for(int x=0;x<Lx;x++){
				ftemp[a-4+x*9+(Ly-1)*Lx*9] = ftemp[a+x*9+(Ly-1)*Lx*9];
				//ftemp[a+x*9+(Ly-1)*Lx*9] = 0;
			}
		}
		//left
		
			for(int y=0;y<Ly;y++){
				ftemp[8+0*9+y*Lx*9] = ftemp[4+0*9+y*Lx*9];
				ftemp[1+0*9+y*Lx*9] = ftemp[5+0*9+y*Lx*9];
				ftemp[2+0*9+y*Lx*9] = ftemp[6+0*9+y*Lx*9];
			}
		//right
		for(int y=0;y<Ly;y++){
				ftemp[4+(Lx-1)*9+y*Lx*9] = ftemp[8+(Lx-1)*9+y*Lx*9];
				ftemp[5+(Lx-1)*9+y*Lx*9] = ftemp[1+(Lx-1)*9+y*Lx*9];
				ftemp[6+(Lx-1)*9+y*Lx*9] = ftemp[4+(Lx-1)*9+y*Lx*9];
			}
		//corners
		//top left
//		ftemp[1+0*9+0*Lx*9] = ftemp[5+0*9+0*Lx*9]; 
//		ftemp[8+0*9+0*Lx*9] = ftemp[4+0*9+0*Lx*9]; 
//		ftemp[7+0*9+0*Lx*9] = ftemp[3+0*9+0*Lx*9];
//				//top right
//		ftemp[5+(Lx-1)*9+0*Lx*9] = ftemp[1+(Lx-1)*9+0*Lx*9]; 
//		ftemp[6+(Lx-1)*9+0*Lx*9] = ftemp[2+(Lx-1)*9+0*Lx*9]; 
//		ftemp[7+(Lx-1)*9+0*Lx*9] = ftemp[3+(Lx-1)*9+0*Lx*9];
//				//down right
//		ftemp[3+(Lx-1)*9+(Ly-1)*Lx*9] = ftemp[7+(Lx-1)*9+(Ly-1)*Lx*9]; 
//		ftemp[4+(Lx-1)*9+(Ly-1)*Lx*9] = ftemp[8+(Lx-1)*9+(Ly-1)*Lx*9]; 
//		ftemp[5+(Lx-1)*9+(Ly-1)*Lx*9] = ftemp[1+(Lx-1)*9+(Ly-1)*Lx*9];
//		//down left
//		ftemp[1+0*9+(Ly-1)*Lx*9] = ftemp[5+0*9+(Ly-1)*Lx*9]; 
//		ftemp[2+0*9+(Ly-1)*Lx*9] = ftemp[6+0*9+(Ly-1)*Lx*9]; 
//		ftemp[3+0*9+(Ly-1)*Lx*9] = ftemp[7+0*9+(Ly-1)*Lx*9]; 

	}

	void calculate_curl(){
		for(int y=0;y<Ly;y++){
			for(int x=0;x<Lx;x++){	
				curl[x+y*Lx] = u[x+1+y*Lx] - u[x-1+y*Lx] - v[x+(y+1)*Lx] + v[x+(y-1)*Lx];
			}
		}
	}
 
}
#endif


