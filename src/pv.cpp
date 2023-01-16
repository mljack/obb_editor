#include "pv.h"

#include"math.h"
#include"stdlib.h"
#include"stdio.h"
#include <glm/vec2.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

typedef glm::dvec2 vector_xy;

double z1;
double t, dt_t, dt_tn, tp;
vector_xy v0, s, now, last;

struct vector_re {
	vector_re(double r, double angle) {
		r_ = r;
		a_ = angle;
	}
	vector_xy to_xy() {
		return vector_xy(r_ * std::cos(a_), r_ * std::sin(a_));
	}
	double r_;
	double a_;
};

// pi => glm::pi()
// angle() => glm::orientedAngle(), glm::angle()
// distance_v() => glm::distance()
// dot()
// cross()
// glm::length()

double distance(int x, int y, int x1, int y1) {
	int dx = (x1-x);
	int dy = (y1-y);
	return sqrt(dx * dx + dy * dy);
}

int pset(int x,int y,int c) {
	return 0;
}

int line(int x,int y,int x1,int y1,int color) {
	if (x<0||x>639||y<0||y>479)
		return -1;
	int x2,y2,x3,y3;
	long x_step,y_step,x_step1,y_step1;
	if (color!=0&&color!=1)
		return -3;
	x3=glm::sign(x1-x); y3=glm::sign(y1-y);
	x2=std::abs(x1-x)+1; y2=std::abs(y1-y)+1;
	if (x2>y2){x_step=x3;x_step1=1;
		y_step=y2*y3;y_step1=x2;}
	else {y_step=y3;y_step1=1;
		x_step=x2*x3;x_step1=y2;}
	for(int a=0;a<=639;a++) {
		x2=x+a*x_step/x_step1+0.5;
		y2=y+a*y_step/y_step1+0.5;
		pset(x2,y2,color);
		if (x2==x1&&y2==y1){return 0;}
	}
	return 0;
}

void circle(int x,int y,int r,int c) {
	int x1,y1,x2,y2,y3,x2_,y2_,y3_;
	for(x1=-r;x1<=r;x1++) {
		y1=sqrt(r*r-x1*x1); x2=x+x1;
		y2=y-y1; y3=y+y1;
		if (x1==-r)
			{x2_=x2; y2_=y2; y3_=y3;}
		line(x2,y2,x2_,y2_,c);
		line(x2,y3,x2_,y3_,c);
		x2_=x2; y2_=y2; y3_=y3;
	}
}

void full_circle(int x,int y,int r,int color) {
	int a1,a2;
	for(a1=0;a1<=2*r;a1++)
		for(a2=0;a2<=2*r;a2++)
			if (distance(r,r,a1,a2)<r)
				pset(a1+x-r,a2+y-r,color);
}

/* Physic Vector */
vector_xy accel();
void pv(int c) {
	double n,j;
	t=0;
	pset(now.x,now.y,c);
	while (true) {
		n = dt_t/dt_tn;
		last = now;
		for(j=0; j<n; j++) {
			s = v0 * dt_tn + accel() * (dt_tn * dt_tn/2);
			now = last + s;
			//line(now.x,now.y,last.x,last.y,c);
			v0 += accel() * dt_tn;
			last = now;
			t += dt_tn;
		}
		pset(now.x,now.y,c);
		// tmp4=distance_v(now,vector_xy(200,200));
		// pset(t/1.2+.5,tmp4+.5,1);
		//pset(t,-300+(distance_v(vector_xy(0,0),v0)*distance_v(vector_xy(0,0),v0)/2.0+distance_v(now,vector_xy(200,200))*z-13)*2000,1);
		// printf("%Lf\n",distance_v(vector_xy(0,0),v0)*distance_v(vector_xy(0,0),v0)/2.0+distance_v(now,vector_xy(200,200))*z);
		if (t >= tp)
			break;
	}
}

vector_xy accel() {
	return vector_xy(0, 9.8) + v0 * z1;
}

void test_pv(){
	line(0,200,639,200,1);
	dt_t=.1,dt_tn=.0005,tp=10.5;
	z1=0;
	now = vector_xy(0,200);
	v0 = vector_xy(40,-50);
	pv(1);
	dt_t=.1,dt_tn=.0005,tp=9.0;
	z1=-0.2;
	now = vector_xy(0,200);
	v0 = vector_xy(40,-50);
	pv(1);
}
/* dt_tn=.00001  mv^2/2+mgh=mv'^2/2+mgh' */