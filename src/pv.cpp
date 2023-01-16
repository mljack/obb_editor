long double z,R,H,z1;
#include"dos.h"
#include"math.h"
#include"stdlib.h"
#include"conio.h"
#include"stdio.h"
 unsigned char far *screen=(unsigned char far*)0xa0000000l;
struct vector_xy
{long double x;
 long double y;
};
long double pi=3.141592654;
long double t,dt_t,dt_tn,tp;
struct vector_xy v0,s,now,last;
struct vector_xy add(),sub(),num_times();

struct vector_re
{long double r;
 long double a;
};
void re2xy(struct vector_xy *a,struct vector_re b)
{a->x=b.r*cos(b.a);
 a->y=b.r*sin(b.a);
}
long double angle(struct vector_xy a1,struct vector_xy a2)
{long double r,t1,t2,s,c;
 t1=a1.x-a2.x;
 t2=a1.y-a2.y;
 r=sqrt(t1*t1+t2*t2);
 s=t2/r; c=t1/r;
 if (s>=0&&c>=0) return asin(s);
 if (s>=0&&c<=0) return pi-asin(s);
 if (s<=0&&c<=0) return pi+asin(-s);
 if (s<=0&&c>=0) return 2*pi-asin(-s);
}
long double distance_v(struct vector_xy a1,struct vector_xy a2)
{long double t1,t2;
 t1=a1.x-a2.x;
 t2=a1.y-a2.y;
 return sqrt(t1*t1+t2*t2);
}

int line_abs(int a)
{if (a<0) return -a; else return a;
}

long double distance(int x,int y,int x1,int y1)
{int a,a1;
 a=(x1-x); a1=(y1-y);
 return sqrt(a*a+a1*a1);
}

int sgh(long a)
{if (a>0) return 1;
 if (a==0) return 0;
 if (a<0) return -1;
 }
void init_screen(void)
{union REGS r;
 r.x.ax=0x0011; int86(0x10,&r,&r);
}
void close_screen(void)
{union REGS r;
 r.x.ax=0x0003; int86(0x10,&r,&r);
}
void pset_(long n,int c)
{unsigned char a=screen[n/8];
 unsigned b; b=1<<(7-n%8);
 if (!c){asm not b; a&=b; }    else  a|=b;
 screen[n/8]=a;
}

int pset(int x,int y,int c)
{//x-=200;y-=200;x*=5;y*=5;
// x+=.5;y+=.5;
 if (x<0.0||x>639.0||y<0.0||y>479.0)return -2;
 if (c!=0&&c!=1)return -3;
 pset_(x+y*640.0,c); return 0;
}

int line(int x,int y,int x1,int y1,int color)
{register int a;
 if (x<0||x>639||y<0||y>479) return -1;
 int x2,y2,x3,y3;
 long x_step,y_step,x_step1,y_step1;
 if (color!=0&&color!=1)return -3;
 x3=sgh(x1-x); y3=sgh(y1-y);
 x2=line_abs(x1-x)+1; y2=line_abs(y1-y)+1;
 if (x2>y2){x_step=x3;x_step1=1;
     y_step=y2*y3;y_step1=x2;}
  else {y_step=y3;y_step1=1;
     x_step=x2*x3;x_step1=y2;}
 for(a=0;a<=639;a++)
 {x2=x+a*x_step/x_step1+0.5;
  y2=y+a*y_step/y_step1+0.5;
  pset(x2,y2,color);
  if (x2==x1&&y2==y1){return 0;}
 }}
void circle(int x,int y,int r,int c)
{int x1,y1,x2,y2,y3,x2_,y2_,y3_;
 for(x1=-r;x1<=r;x1++)
 {y1=sqrt(r*r-x1*x1); x2=x+x1;
  y2=y-y1; y3=y+y1;
  if (x1==-r)
     {x2_=x2; y2_=y2; y3_=y3;}
  line(x2,y2,x2_,y2_,c);
  line(x2,y3,x2_,y3_,c);
  x2_=x2; y2_=y2; y3_=y3;
 }}

void full_circle(int x,int y,int r,int color)
{int a1,a2;
 for(a1=0;a1<=2*r;a1++)
 for(a2=0;a2<=2*r;a2++)
 if (distance(r,r,a1,a2)<r)
  pset(a1+x-r,a2+y-r,color);
}

/* Physic Vector */
struct vector_xy add(struct vector_xy v1,struct vector_xy v2)
{struct vector_xy tmp1;
 tmp1.x=v1.x+v2.x;
 tmp1.y=v1.y+v2.y;
 return tmp1;
}
struct vector_xy sub(struct vector_xy v1,struct vector_xy v2)
{struct vector_xy tmp1;
 tmp1.x=v1.x-v2.x;
 tmp1.y=v1.y-v2.y;
 return tmp1;
}
struct vector_xy num_times(struct vector_xy v1,long double a1)
{struct vector_xy tmp1;
 tmp1.x=a1*v1.x;
 tmp1.y=a1*v1.y;
 return tmp1;
}

struct vector_xy ff(long double x,long double y)
{struct vector_xy tmp1;
 tmp1.x=x;
 tmp1.y=y;
 return tmp1;
}

void dy(struct vector_xy *a,struct vector_xy b)
{a->x=b.x;
 a->y=b.y;
}

void pv(int c)
{struct vector_xy aa();
 long double tmp4,max=0;
 long double n,j;
 t=0;
 pset(now.x,now.y,c);
 l1:
 n=dt_t/dt_tn;
 dy(&last,now);
 for(j=0;j<n;j++)
 {dy(&s,add(num_times(aa(),dt_tn*dt_tn/2),num_times(v0,dt_tn)));
  dy(&now,add(last,s));
//  line(now.x,now.y,last.x,last.y,c);
  dy(&v0,add(num_times(aa(),dt_tn),v0));
  dy(&last,now);
  t+=dt_tn;
 }
  pset(now.x,now.y,c);
// tmp4=distance_v(now,ff(200,200));
// pset(t/1.2+.5,tmp4+.5,1);
//pset(t,-300+(distance_v(ff(0,0),v0)*distance_v(ff(0,0),v0)/2.0+distance_v(now,ff(200,200))*z-13)*2000,1);
// printf("%Lf\n",distance_v(ff(0,0),v0)*distance_v(ff(0,0),v0)/2.0+distance_v(now,ff(200,200))*z);
 if (max<tmp4) max=tmp4;
 if (t>=tp) {dy(&now,ff(0,0));return;}
 goto l1;
}
struct vector_xy aa(void)
{struct vector_xy tmp1;
 tmp1.x=0;
 tmp1.y=9.8;
 dy(&tmp1,add(num_times(v0,z1),tmp1));
 return tmp1;
}
void main(void)
{unsigned int a;
 init_screen();
 line(0,200,639,200,1);
 dt_t=.1,dt_tn=.0005,tp=10.5;
 z1=0;
 dy(&now,ff(0,200));
 dy(&v0,ff(40,-50));
 pv(1);
 dt_t=.1,dt_tn=.0005,tp=9.0;
 z1=-0.2;
 dy(&now,ff(0,200));
 dy(&v0,ff(40,-50));
 pv(1);
 getch();
// close_screen();
}
/* dt_tn=.00001  mv^2/2+mgh=mv'^2/2+mgh' */