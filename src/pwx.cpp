/* The wrong reported division by zero happens in "c_line" function */
#define YD 1.5
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "pv.h"
#include "pwx.h"

unsigned char now_color=14;
double _st1,_st2,_st3,e1,e2,e3,e4,e5,p,q;

int move_status=0;/* UP:0 DOWN:1 LEFT:2 RIGHT:3 */
int equ2_status=0;

void _arc(int *xy);

int sgh(long a) {
	if (a>0) return 1;
	if (a==0) return 0;
	if (a<0) return -1;
}

double equ3(double a1,double a2,double a3)
{double p=a2-a1*a1/3,q=2*a1*a1*a1/27-a1*a2/3+a3;
 double pq=sqrt(q*q/4+p*p*p/27);
 return pow(-q/2+pq,1/3)+pow(-q/2-pq,1/3)-a1/3;
}

double equ2_z()
{return (-p+sqrt(p*p-4*q))/2;
}

double equ2_f()
{return (-p-sqrt(p*p-4*q))/2;
}

double f(double x)
{float y;
 y=x*(x*(x+_st1)+_st2)+_st3;
 return y;
}
double xpoint(double x1,double x2)
{double y;
 y=(x1*f(x2)-x2*f(x1))/(f(x2)-f(x1));
 return y;
}
double root(float x1,float x2)
{double x,y,y1;
 y1=f(x1);
 do
 {x=xpoint(x1,x2);
  y=f(x);
  if (y*y1>0){y1=y;x1=x;}
    else x2=x;
 }while(fabs(y)>=0.00000001);
 return x;
}

double compute_a(int sin_sign,int cos_sign,int x,int y,double c)
{double s=std::sqrt(1-c*c);
 return (sin_sign*s*x+c*cos_sign*y)/pow((cos_sign*c*x-s*sin_sign*y),2);
}

int look_like(double a1,double a2)
{if (std::fabs(a1-a2)>0.000001)return 0;
 return 1;
}

int all_look_like(double *a,int n)
{int n1;
 for(n1=0;n1<n;n1+=2)if (!look_like(fabs(a[n1]),fabs(a[n1+1])))return 0;
 return 1;
}

int c_line(int x0,int y0,int x1,int y1,int x2,int y2,int x3,int y3)
    /*tc=0   yc=1*/
{double k=1.0*(y0-y1)/(x0-x1);
 double b=y0-x0*k;
 double a1=k*x2+b-y2,a2=k*x3+b-y3;
 if (look_like(a1,0)) return 1;
 if (sgh(a1)==sgh(a2)) return 0;
 return 1;
}

static void pset(int x, int y, int c) {}

void arc(const std::vector<double>& xy, double* AA, double* theta) {
	double x1=xy[4]-xy[0],y1=xy[5]-xy[1],x2=xy[2]-xy[0],y2=xy[3]-xy[1];
	double d=x1*x2*(y2-y1),e=x2*x2*y1-x1*x1*y2,f=y1*y2*(y2-y1);
	double k=x1*x2*(x2-x1),m=x1*y2*y2-x2*y1*y1,n=y1*y2*(x2-x1);
	double j1=2*d+e-f,j2=k-m-2*n,j3=2*d-f;
	double st__=j1*j1+j2*j2;
	double st1=((2*m-j2)*j2-2*j1*j3)/st__;
	double st2=(j3*j3+m*((-2)*j2+m))/st__;
	double st3=-(m*m)/st__;
	double t;

	double aa[4],aa_[4],*_aa=aa,*__x=&x1,*__y=&y1;
	int r1,r2;

// double a1,a1_,a2,a2_,a3,a3_,a4,a4_;
 double a[4];
 int sin_sign[4],cos_sign[4],n1=0;
 _st1=st1;_st2=st2;_st3=st3;
 double A,c,s;
 int SIN_SIGN,COS_SIGN;
 c=sqrt(t=root(0,1));
/* a1=a4*x1+c*y1/pow(c*x1-a4*y1,2);
 a1_=a4*x2+c*y2/pow(c*x2-a4*y2,2);
 a2=a4*x1+c*y1/pow(c*x1-a4*y1,2);
 a2_=a4*x2+c*y2/pow(c*x2-a4*y2,2);
 a3=-a4*x1+c*y1/pow(c*x1+a4*y1,2);
 a3_=-a4*x2+c*y2/pow(c*x2+a4*y2,2);
 a4_=-a4*x2+c*y2/pow(c*x2+a4*y2,2);
 a4=-a4*x1+c*y1/pow(c*x1+a4*y1,2);
*/
/*
 a1=compute_a(+1,+1,x1,y1,c);
 a2=compute_a(+1,+1,x2,y2,c);
 a3=compute_a(+1,-1,x1,y1,c);
 a4=compute_a(+1,-1,x2,y2,c);
 a1_=compute_a(-1,+1,x1,y1,c);
 a2_=compute_a(-1,+1,x2,y2,c);
 a3_=compute_a(-1,-1,x1,y1,c);
 a4_=compute_a(-1,-1,x2,y2,c);
*/
 for(r1=0;r1<2;r1++)
 {if (r1) {_aa=aa_;
	   __x=&x2;
	   __y=&y2;
	  }
  for(r2=0;r2<4;r2++,_aa++)
  *_aa=compute_a(r2/2?-1:+1,r2%2?-1:+1,*__x,*__y,c);
 }

 for(r1=0;r1<4;r1++)
 if (look_like(aa[r1],aa_[r1])){a[n1]=(aa[r1]+aa_[r1])/2;
				sin_sign[n1]=r1/2?-1:+1;
				cos_sign[n1]=r1%2?-1:+1;
				n1++;
			       }
/*
 if (look_like(a1,a2)) {a[n1]=c;
			sin_sign[n1]=+1;
			cos_sign[n1]=+1;
			n1++;
		       }
 if (look_like(a3,a4)) {a[n1]=c;
			sin_sign[n1]=+1;
			cos_sign[n1]=-1;
			n1++;
		       }
 if (look_like(a1_,a2_)) {a[n1]=c;
			  sin_sign[n1]=-1;
			  cos_sign[n1]=+1;
			  n1++;
			 }
 if (look_like(a3_,a4_)) {a[n1]=c;
			  sin_sign[n1]=-1;
			  cos_sign[n1]=-1;
			  n1++;
			 }
*/

	if (n1==1) {
		A=a[0];
		SIN_SIGN=sin_sign[0];
		COS_SIGN=cos_sign[0];
		printf("\a\a");
	}
	if (n1==0) {
		printf("\a");
		//close_screen_320_200_256();
		for(r1=0;r1<4;r1++)
			printf("%g ",aa[r1]);
		printf("\n");
		for(r1=0;r1<4;r1++)
			printf("%g ",_aa[r1]);
		printf("\n");
		exit(0);
	}
	if (all_look_like(a,n1)) {
		A=a[0];
		SIN_SIGN=sin_sign[0];
		COS_SIGN=cos_sign[0];
	}
	s=sqrt(1-c*c);
	s=s*SIN_SIGN;c=c*COS_SIGN;
	e1=A*c*c;e2=-2*A*c*s;e3=A*s*s;e4=-s;e5=-c;
	// A*(x*cos-y*sin)^2 = x*sin+y*cos
	// (X, Y) = [cos, -sin] * [x]
	//          [sin,  cos]   [y]
	// A*x^2 = Y
	if (e1==0)
		{equ2_status=1;e1=e3;e2=e4;}
	if (e3==0)
		{equ2_status=2;e2=e5;}
	printf("%lf*x^2 + %lf*xy + %lf*y^2 + %lf*x + %lf*y = 0\n", e1, e2, e3, e4, e5);
	
	*AA = A;
	*theta = std::atan2(c, s);
	//_arc(xy);
}


void compute_pq(double x3,double y3)
{switch(move_status)
 {case 0:p=(e2*y3-e2+e4)/e1;q=(e3*(y3-1)*(y3-1)+e5*y3-e5)/e1;break;
  case 1:p=(e2*y3+e2+e4)/e1;q=(e3*(y3+1)*(y3+1)+e5*y3+e5)/e1;break;
  case 2:p=(e2*x3-e2+e5)/e3;q=(e1*(x3-1)*(x3-1)+e4*x3-e4)/e3;break;
  case 3:p=(e2*x3+e2+e5)/e3;q=(e1*(x3+1)*(x3+1)+e4*x3+e4)/e3;break;
 }
}

double compute_dt()
{return p*p-4*q;
}

void root_1(double *x,double *y)
{switch(move_status)
 {case 0:(*y)--;
  case 1:(*y)++;
  case 2:(*x)--;
  case 3:(*x)++;
 }
 (move_status/2)?(*y=p/2):(*x=p/2);
}

void root_2(double *x1,double *y1,double *x2,double *y2)
{switch(move_status)
 {case 0:(*y1)--;(*y2)--;*x1=equ2_z();*x2=equ2_f();break;
  case 1:(*y1)++;(*y2)++;*x1=equ2_z();*x2=equ2_f();break;
  case 2:(*x1)--;(*x2)--;*y1=equ2_z();*y2=equ2_f();break;
  case 3:(*x1)++;(*x2)++;*y1=equ2_z();*y2=equ2_f();break;

 }
}

/* 1:have roots 0:haven't */
int equ2_un(double *x1, double *y1, double *x2, double *y2) {
	switch(move_status) {
		case 0: {
			if (equ2_status==1) {
				if ((y1-1)<0) {
					return 0;
				} else {
					*x1=sqrt((--(*y1))*e2/e1);
					*x2=-sqrt(*y1*e2/e1);
					return 1;
					//*y2--;
				}
			} else {
				*x1=*x2=(--(*y1))*(*y1)*e1/e2;
				*y2--;
				return 1;
			}
		}
		case 1: {
			if (equ2_status==1) {
				if ((y1+1)<0) {
					return 0;
				} else {
					*x1=sqrt((++(*y1))*e2/e1);
					*x2=-sqrt(*y1*e2/e1);
					return 1;
					//*y2++;
				}
			} else {
				*x1=*x2=(++(*y1))*(*y1)*e1/e2;
				*y2++;
				return 1;
		 	}
		}
		case 2: {
			if (equ2_status==1) {
				if ((x1-1)<0) {
					return 0;
				} else {
					*y1=sqrt((--(*x1))*e2/e1);
					*y2=-sqrt(*x1*e2/e1);
					return 1;
					//*x2--;
				}
	    } else {
				*y1=*y2=(--(*x1))*(*x1)*e1/e2;
				return 1;
				//*x2--;
		 	}
		}
		case 3: {
			if (equ2_status==1) {
				if ((x1+1)<0) {
					return 0;
				} else {
					*y1=sqrt((++(*x1))*e2/e1);
					*y2=-sqrt(*x1*e2/e1);
					return 1;
					*x2++;
				}
			} else {
				*y1=*y2=(++(*x1))*(*x1)*e1/e2;
				*x2++;
				return 1;
			}
		}
 	}
 return 0 ;
}

void _arc(int *xy) {
	int status=1;
	 double x1=xy[2]-xy[0],y1=xy[3]-xy[1],x2=xy[4]-xy[0],y2=xy[5]-xy[1];
	 double x3,y3,x4,y4,dt,*_x=&x1,*_y=&y1,*__x=&x2,*__y=&y2;
	 double o_x3,o_x4,o_y3,o_y4;
	 double tmp1,tmp2;
	 x3=x4=o_x3=o_x4=x1;y3=y4=o_y3=o_y4=y1;
	while (status != 0) {
		o_x3=x3;o_y3=y3;o_x4=x3;o_y4=y3;
		if (equ2_status)
		  if (equ2_un(&x3,&y3,&x4,&y4))
		     if (look_like(x3,x4)&&look_like(y3,y4)) goto q3;
		  else goto q4;
		     else goto q1;
		compute_pq(x3,y3);
		dt=compute_dt();
		if (dt<0) {
			q1:
			switch(move_status) {
				case 0:move_status=1; continue;
				case 1:move_status=0; continue;
				case 2:move_status=3; continue;
				case 3:move_status=2; continue;
			}
		}
		if (look_like(dt,0)) {
			root_1(&x3,&y3);
			q3:
			if (o_x3==0) goto next;
			if (c_line(0,0,o_x3,o_y3,x3,y3,*__x,*__y)) {
				q2:
				if (fabs(o_x3-x3)<YD&&fabs(o_y3-y3)<YD) {
					pset(x3+xy[0],y3+xy[1],now_color);
					continue;
				} else {
					switch(move_status) {
						case 0:
						case 1:move_status=2;x3=o_x3;y3=o_y3; continue;
						case 2:
						case 3:move_status=0;x3=o_x3;y3=o_y3; continue;
					}
				}
			}
			x3=o_x3;y3=o_y3;goto q1;
		}
		if (dt>0) {
			root_2(&x3,&y3,&x4,&y4);
			q4:
			if ((int)o_x3==0) goto next;
			tmp1=c_line(0,0,o_x3,o_y3,x3,y3,*__x,*__y);
			if ((int)o_x4==0) goto next;
			tmp2=c_line(0,0,o_x4,o_y4,x4,y4,*__x,*__y);
			if (!tmp1 && tmp2) {
				o_x3=o_x4;o_y3=o_y4;x3=x4;y3=y4;goto q2;
			}
			if (tmp1 && !tmp2) goto q2;
			if (!tmp1 && !tmp2) {x3=o_x3;y3=o_y3;goto q1;}
			tmp1=move_status/2?fabs(o_y3-y3):fabs(o_x3-x3);
			tmp2=move_status/2?fabs(o_y4-y4):fabs(o_x4-x4);
			if (tmp1<YD && tmp2>=YD) {
				pset(x3+xy[0],y3+xy[1],now_color);
				continue;
			}
			if (tmp1>=YD && tmp2<YD) {
				pset(x4+xy[0],y4+xy[1],now_color);
			 x3=x4;y3=y4;
			 continue;
			}
			if (tmp1<YD && tmp2<YD) {
				if (distance(x3,y3,o_x3,o_y3)>distance(x4,y4,o_x4,o_y4)) {
					pset(x3+xy[0],y3+xy[1],now_color);
					continue;
				} else {
					pset(x4+xy[0],y4+xy[1],now_color);
		 			x3=x4;y3=y4;
		 			continue;
				}
			}
		}
		next:
		if (status)
			return;
		status = 1;
		_x=&x2;_y=&y2;__x=&x1;__y=&y1;
		x3=x4=o_x3=o_x4=x2;y3=y4=o_y3=o_y4=y2;
	}
}

// void pwx_test() {
// 	//int xy[6]={100,50,140,50,100,90};
// 	int xy[6]={100,50,70,100,200,100};
// 	arc(xy);
// 	// for(int a=0;a<6;a+=2)pset(xy[a],xy[a+1],now_color-2);
// 	// line(xy[0],xy[1],xy[2],xy[3],now_color-2);
// 	// line(xy[4],xy[5],xy[0],xy[1],now_color-2);
// 	// line(xy[4],xy[5],xy[2],xy[3],now_color-2);
// 	//printf("%f,%d,%d,%f,%f\n",A,SIN_SIGN,COS_SIGN,s,c);
// 	//printf("%gx^2+%gxy+%gy^2+%gx+%gy=0\n",A*c*c,-2*A*s*c*SIN_SIGN*COS_SIGN,A*s*s,-s*SIN_SIGN,-c*COS_SIGN);
// 	//printf("%f",A*c*c*x*x-2*A*s*c*SIN_SIGN*COS_SIGN*x*y+A*s*s*y*y-s*SIN_SIGN*x-c*COS_SIGN*y);
// }
