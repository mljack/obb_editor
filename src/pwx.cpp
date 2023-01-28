#include <cstdio>
#include <cmath>
#include <algorithm>
#include <glm/vec2.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "pwx.h"

namespace {
	double pi = glm::pi<double>();
	double pi2 = glm::two_pi<double>();
	double pi0_5 = glm::half_pi<double>();
	double _st1,_st2,_st3;
}

double f(double x) {
	return x*(x*(x+_st1)+_st2)+_st3;
}

double xpoint(double x1, double x2) {
	return (x1*f(x2)-x2*f(x1))/(f(x2)-f(x1));
}

double root(double x1, double x2) {
	int max_iterations = 10000;
	double x, y;
	double y1 = f(x1);
	int i = 0;
	for(; i < max_iterations; ++i) {
		x = xpoint(x1, x2);
		y = f(x);
		if (y*y1>0){
			y1 = y;
			x1 = x;
		} else {
			x2 = x;
		}
		if (fabs(y) < 1e-8)
			break;
	}
	if (i == max_iterations)
		printf("root returns with fabs(y) == %.10lf\n", fabs(y));
	return x;
}


// ================================================================
// Solution of the rotated parabola problem
// Y = A*X^2
// c = cos(theta)
// s = sin(theta)
// p0 = (0, 0)     // parabola vertex
// p1 = (x1, y1)   // on the parabola
// p2 = (x2, y2)   // on the parabola as p1, but with different sign of X
//
// x,y: rotated coodinates
// X,Y: original coodinates
//
// A*X^2 = Y
//
// A*(X*cos-Y*sin)^2 = X*sin+Y*cos
// [x] = [cos, -sin] * [X]           90 [0, -1]*[1]=[0]
// [y]   [sin,  cos]   [Y]              [1,  0] [0] [1]
//
// A*(x*cos+y*sin)^2 = -x*sin+y*cos
// [X] = [cos,  sin] * [x]          -90 [ 0, 1]*[1]=[0]
// [Y]   [-sin, cos]   [y]              [-1, 0] [0] [1]
//
// A*(x*c+y*s)^2 = -x*s+y*c
//
//
// A*(x1*c+y1*s)^2 = (-x1*s+y1*c)
// A*(x2*c+y2*s)^2 = (-x2*s+y2*c)
//
// (-x1*s+y1*c)/(x1*c+y1*s)^2 = (-x2*s+y2*c)/(x2*c+y2*s)^2
// 0 = (-x1*s+y1*c)/(x1*c+y1*s)^2(x2*c+y2*s)^2+x2*s-y2*c
//
// 0 = (-x1*s+y1*c)*(x2*c+y2*s)^2-(-x2*s+y2*c)*(x1*c+y1*s)^2
// 0 = (-x1*s+y1*c)*(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-(-x2*s+y2*c)*(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)
// 0 = y1*c(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-x1*s(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-y2*c(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)+x2*s(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)
// 0 = (x2^2*y1*c^3+2*x2*y2*y1*c^2*s+y2^2*y1*s^2*c)+(-x2^2*x1*c^2*s-2*x2*y2*x1*c*s^2-y2^2*x1*s^3)+(-x1^2*y2*c^3-2*x1*y1*y2*c^2*s-y1^2*y2*c*s^2)+(x1^2*x2*c^2*s+2*x1*y1*x2*c*s^2+y1^2*x2*s^3)
// 0 = (x2^2*y1-x1^2*y2)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c*s^2+(y1^2*x2-y2^2*x1)*s^3
// 0 = (x2^2*y1-x1^2*y2)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c*(1-c^2)+(y1^2*x2-y2^2*x1)*(1-c^2)*s
// 0 = (x2^2*y1-x1^2*y2-2*x1*y1*x2+2*x2*y2*x1+y1^2*y2-y2^2*y1)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1-y1^2*x2+y2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c+(y1^2*x2-y2^2*x1)*s
//
// h1 = x2^2*y1-x1^2*y2-2*x1*y1*x2+2*x2*y2*x1+y1^2*y2-y2^2*y1
// h2 = 2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1-y1^2*x2+y2^2*x1
// h3 = 2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1
// h4 = y1^2*x2-y2^2*x1
//
// 0 = h1*c^3+h2*c^2*s+h3*c+h4s
// 0 = (h1*c^3+h3*c)^2-(h2*c^2+h4)^2(1-c^2)
// 0 = (h1^2*c^6+2*h1*h3*c^4+h3^2*c^2)-(h2^2*c^4+2*h2*h4c^2+h4^2)*(1-c^2)
// 0 = (h1^2*c^6+2*h1*h3*c^4+h3^2*c^2)-(h2^2*c^4+2*h2*h4c^2+h4^2)+(h2^2*c^6+2*h2*h4c^4+h4^2*c^2)
// 0 = ((h1^2+h2^2)*c^6+(2*h1*h3-h2^2+2*h2*h4)*c^4+(h3^2-2*h2*h4+h4^2)*c^2)-h4^2
//
// k0 = h1^2+h2^2
// k1 = {2*h1*h3-h2^2+2*h2*h4}/{k0}
// k2 = {h3^2-2*h2*h4+h4^2}/{k0}
// k3 = {-h4^2}/{k0}
//
// 0 = c^6+k1*c^4+k2*c^2+k3
// ================================================================

double compute_trial_a(double x, double y, double theta) {
	double xx = x*std::cos(theta)+y*std::sin(theta);
	double yy = -x*std::sin(theta)+y*std::cos(theta); 
	return yy/(xx*xx);

	// glm::vec2 x_dir(std::cos(theta), std::sin(theta));
	// glm::vec2 y_dir(-std::sin(theta), std::cos(theta));
	// glm::vec2 p(x, y);

	// double xx = glm::dot(x_dir, p); // x*c + y*s
	// double yy = glm::dot(y_dir, p); // -x*s + y*c
	// if (std::abs(xx) < 1e-6)
	// 	return 0.0;
	// return yy/(xx*xx);
}

double f2(double x, double y, double A, double theta) {
	double xx = x*std::cos(theta)+y*std::sin(theta);
	double yy = -x*std::sin(theta)+y*std::cos(theta);
	return A*xx*xx - yy;
}

bool is_a_solution(double x1, double y1, double x2, double y2, double theta, double* A = nullptr, double* residual = nullptr) {
	double a = compute_trial_a(x1, y1, theta);
	double r = f2(x2, y2, a, theta);
	if (residual)
		*residual = r;
	if (A)
		*A = a;
	return (std::abs(r) < 1e-1);
}

void compute_other_roots_for_equ3(std::vector<double>* roots, double k1, double k2, double k3, double root1) {
	// assert(roots->size() == 1)
	// equation: y=x^3 + k1*x^2 + k2*x + k3\n", k1, k2, k3);
	// k1 = -(x1+x2+x3)
	// k3 = -x1*x2*x3
	// a = 1
	// b = -(x2+x3) = k1+x1
	// c = x2*x3 = -k3/x1
	double x1 = root1;
	double a = 1.0;
	double b = k1+x1;
	double c = -k3/x1;
	double d = b*b-4*a*c;
	if (d > 1e-6) {
		roots->emplace_back((-b+sqrt(d))/(2*a));
		roots->emplace_back((-b-sqrt(d))/(2*a));
	} else if (d > -1e-6) {
		roots->emplace_back(-b/(2*a));
	} 
}

void arc(const std::vector<double>& xy, std::vector<double>* A_array, std::vector<double>* theta_array) {
	double x1 = xy[0]-xy[4];
	double y1 = xy[1]-xy[5];
	double x2 = xy[2]-xy[4];
	double y2 = xy[3]-xy[5];

	double t_low1 = std::atan2(y1, x1) - pi0_5;
	double t_high1 = std::atan2(y2, x2) - pi0_5;
	double t_low2 = t_low1 + pi2;
	double t_high2 = t_high1 + pi2;
	double delta11 = std::fabs(t_high1 - t_low1);
	double delta12 = std::fabs(t_high1 - t_low2);
	double delta21 = std::fabs(t_high2 - t_low1);
	double delta22 = std::fabs(t_high2 - t_low2);
	double t_low, t_high;
	if (delta11 <= delta12 && delta11 <= delta21 && delta11 <= delta22) {
		t_low = t_low1;
		t_high = t_high1;
	} else if (delta12 <= delta11 && delta12 <= delta21 && delta12 <= delta22) {
		t_low = t_low2;
		t_high = t_high1;
	} else if (delta21 <= delta11 && delta21 <= delta12 && delta21 <= delta22) {
		t_low = t_low1;
		t_high = t_high2;
	} else {
		t_low = t_low2;
		t_high = t_high2;
	}

	if (t_high < t_low)
		std::swap(t_low, t_high);
	
	double t = (t_low + t_high) / 2;

	// printf("v1: [%lf, %lf]\n", x1, y1);
	// printf("v2: [%lf, %lf]\n", x2, y2);
	// printf("t range[%lf, %lf]\n", glm::degrees(t_low), glm::degrees(t_high));

	double residual = 1.0, a;

	a = compute_trial_a(x1, y1, t_low);
	double high_residual = f2(x2, y2, a, t_low);
	a = compute_trial_a(x1, y1, t_high);
	double low_residual = f2(x2, y2, a, t_high);
	// printf("residual: %lf, t: %lf\n", high_residual, glm::degrees(t_low));
	// printf("residual: %lf, t: %lf\n", low_residual, glm::degrees(t_high));

	if (low_residual * high_residual > 0.0) {
		printf("No root found...\n");
		return;
	}

	for (int count = 0; std::abs(residual) >= 1e-10; ++count) {
		t = (t_low + t_high) / 2;
		a = compute_trial_a(x1, y1, t);
		residual = f2(x2, y2, a, t);
		if (count > 100000) {
			printf("max iteration reached.\n");
			break;
		}
		// printf("residual: %lf, t: %lf, range[%lf, %lf]\n", residual, glm::degrees(t), glm::degrees(t_low), glm::degrees(t_high));
		if (residual * high_residual <= 0.0)
			t_high = t;
		else
			t_low = t;
	} 

	double theta1 = t;
	double a1 = a;
	double c = std::cos(theta1);
	// double s = std::sin(theta1);
	double root1 = c*c;
	A_array->emplace_back(a1);
	theta_array->emplace_back(theta1);

	double h1 = x2*x2*y1-x1*x1*y2-2*x1*y1*x2+2*x2*y2*x1+y1*y1*y2-y2*y2*y1;
	double h2 = 2*x2*y2*y1-2*x1*y1*y2+x1*x1*x2-x2*x2*x1-y1*y1*x2+y2*y2*x1;
	double h3 = 2*x1*y1*x2-2*x2*y2*x1-y1*y1*y2+y2*y2*y1;
	double h4 = y1*y1*x2-y2*y2*x1;

	double k0 = h1*h1+h2*h2;
	double k1 = (2*h1*h3-h2*h2+2*h2*h4)/k0;
	double k2 = (h3*h3-2*h2*h4+h4*h4)/k0;
	double k3 = -(h4*h4)/k0;

	// double rr1 = h1*c*c*c + h2*c*c*s + h3*c + h4*s;
	// printf("residual of the equation of cos(theta): %.10lf\n", rr1);
	// c = -c;
	// double rr2 = h1*c*c*c + h2*c*c*s + h3*c + h4*s;
	// printf("residual of the equation of cos(theta): %.10lf\n", rr2);
	// s = -s;
	// double rr3 = h1*c*c*c + h2*c*c*s + h3*c + h4*s;
	// printf("residual of the equation of cos(theta): %.10lf\n", rr3);
	// c = -c;
	// double rr4 = h1*c*c*c + h2*c*c*s + h3*c + h4*s;
	// printf("residual of the equation of cos(theta): %.10lf\n", rr4);
	// double c2 = c*c;
	// double rrr1 = c2*c2*c2 + k1*c2*c2 + k2*c2 + k3;
	// printf("residual of the equation of cos(theta)^2: %13.10lf, c2=%13.10lf bisect, c=%13.10lf, A=%.10f, theta=%.10f\n", rrr1, c2, c, a1, std::fmod(glm::degrees(theta1)+3600.0, 360.0));
	// return;

	printf("equation: c2^3 + %lf*c2^2 + %lf*c2 + %lf = 0\n", k1, k2, k3);
	printf("equation: y=x^3 + (%lf)*x^2 + (%lf)*x + (%lf)\n", k1, k2, k3);

	std::vector<double> roots;
	compute_other_roots_for_equ3(&roots, k1, k2, k3, root1);
	//std::sort(roots.begin(), roots.end());

	{
		double c = std::cos(theta1);
		double s = std::sin(theta1);
		printf("root1: %.10lf\n", root1);
		printf("%lf*((%lf)*x+(%lf)*y)^2 = (%lf)*x+(%lf)*y\n", a1, c, s, -s, c);
		printf("   [%ld]: theta=%14.10f, A=%14.10f, residual=%13.10f\n", 9L, std::fmod(glm::degrees(theta1)+3600.0, 360.0), a1, residual);
	}

	for(auto& r : roots) {
		printf("root: %.10lf\n", r);
		double theta = std::acos(std::min(1.0, std::sqrt(r)));
		if (is_a_solution(x1, y1, x2, y2, theta, &a, &residual)) {
			if (a > 0) {
				A_array->emplace_back(a);
				theta_array->emplace_back(theta);
			} else {
				A_array->emplace_back(-a);
				theta_array->emplace_back(theta + pi);
			}
			double c = std::cos(theta_array->back());
			double s = std::sin(theta_array->back());
			printf("%lf*((%lf)*x+(%lf)*y)^2 = (%lf)*x+(%lf)*y\n", A_array->back(), c, s, -s, c);
			printf("   [%ld]: theta=%14.10f, A=%14.10f, residual=%13.10f\n", &r-roots.data(), std::fmod(glm::degrees(theta_array->back())+3600.0, 360.0), A_array->back(), residual);
		} else {
			printf("                                                residual=%13.10f\n", residual);
		}
		if (is_a_solution(x1, y1, x2, y2, -theta, &a, &residual)) {
			if (a > 0) {
				A_array->emplace_back(a);
				theta_array->emplace_back(-theta);
			} else {
				A_array->emplace_back(-a);
				theta_array->emplace_back(-theta + pi);
			}
			double c = std::cos(theta_array->back());
			double s = std::sin(theta_array->back());
			printf("%lf*((%lf)*x+(%lf)*y)^2 = (%lf)*x+(%lf)*y\n", A_array->back(), c, s, -s, c);
			printf("   [%ld]: theta=%14.10f, A=%14.10f, residual=%13.10f\n", &r-roots.data(), std::fmod(glm::degrees(theta_array->back())+3600.0, 360.0), A_array->back(), residual);
		} else {
			printf("                                                residual=%13.10f\n", residual);
		}
	}
	printf("================================================================\n");

	// // another method to find roots.
	// _st1=st1;_st2=st2;_st3=st3;
	// c2 = root(0, 1);
	// *theta = std::acos(std::min(1.0, std::sqrt(c2)));
	// *A = compute_trial_a(x1, y1, *theta);
}
