/**
 * @file pwx.cpp
 * @brief Mathematical utilities for solving rotated parabola problems and cubic equations
 * 
 * This file contains implementations of various numerical methods for:
 * 1. Solving cubic equations (x³ + k1x² + k2x + k3 = 0) using different approaches
 * 2. Finding roots of equations using numerical methods (bisection, secant, Newton's method)
 * 3. Solving rotated parabola problems (Y = A*X²) with coordinate transformations
 * 
 * The main functionality is used to find parameters (amplitude A and rotation angle θ)
 * of a rotated parabola that passes through specific points in a 2D space.
 * 
 * Solution of the rotated parabola problem:
 * Given points in 2D space, find a rotated parabola (Y = A*X²) that passes through them.
 * This involves finding the rotation angle θ and amplitude A of the parabola.
 * 
 * Mathematical formulation:
 * Y = A*X^2
 * c = cos(theta)
 * s = sin(theta)
 * p0 = (0, 0)     // parabola vertex
 * p1 = (x1, y1)   // on the parabola
 * p2 = (x2, y2)   // on the parabola as p1, but with different sign of X
 *
 * x,y: rotated coodinates
 * X,Y: original coodinates
 *
 * A*X^2 = Y
 *
 * A*(X*cos-Y*sin)^2 = X*sin+Y*cos
 * [x] = [cos, -sin] * [X]           90 [0, -1]*[1]=[0]
 * [y]   [sin,  cos]   [Y]              [1,  0] [0] [1]
 *
 * A*(x*cos+y*sin)^2 = -x*sin+y*cos
 * [X] = [cos,  sin] * [x]          -90 [ 0, 1]*[1]=[0]
 * [Y]   [-sin, cos]   [y]              [-1, 0] [0] [1]
 *
 * A*(x*c+y*s)^2 = (-x*s+y*c)
 *
 *
 * A*(x1*c+y1*s)^2 = (-x1*s+y1*c)
 * A*(x2*c+y2*s)^2 = (-x2*s+y2*c)
 *
 * (-x1*s+y1*c)/(x1*c+y1*s)^2 = (-x2*s+y2*c)/(x2*c+y2*s)^2
 * 0 = (-x1*s+y1*c)/(x1*c+y1*s)^2(x2*c+y2*s)^2+x2*s-y2*c
 *
 * 0 = (-x1*s+y1*c)*(x2*c+y2*s)^2-(-x2*s+y2*c)*(x1*c+y1*s)^2
 * 0 = (-x1*s+y1*c)*(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-(-x2*s+y2*c)*(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)
 * 0 = y1*c(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-x1*s(x2^2*c^2+2*x2*y2*c*s+y2^2*s^2)-y2*c(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)+x2*s(x1^2*c^2+2*x1*y1*c*s+y1^2*s^2)
 * 0 = (x2^2*y1*c^3+2*x2*y2*y1*c^2*s+y2^2*y1*s^2*c)+(-x2^2*x1*c^2*s-2*x2*y2*x1*c*s^2-y2^2*x1*s^3)+(-x1^2*y2*c^3-2*x1*y1*y2*c^2*s-y1^2*y2*c*s^2)+(x1^2*x2*c^2*s+2*x1*y1*x2*c*s^2+y1^2*x2*s^3)
 * 0 = (x2^2*y1-x1^2*y2)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c*s^2+(y1^2*x2-y2^2*x1)*s^3
 * 0 = (x2^2*y1-x1^2*y2)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c*(1-c^2)+(y1^2*x2-y2^2*x1)*(1-c^2)*s
 * 0 = (x2^2*y1-x1^2*y2-2*x1*y1*x2+2*x2*y2*x1+y1^2*y2-y2^2*y1)*c^3+(2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1-y1^2*x2+y2^2*x1)*c^2*s+(2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1)*c+(y1^2*x2-y2^2*x1)*s
 *
 * h1 = x2^2*y1-x1^2*y2-2*x1*y1*x2+2*x2*y2*x1+y1^2*y2-y2^2*y1
 * h2 = 2*x2*y2*y1-2*x1*y1*y2+x1^2*x2-x2^2*x1-y1^2*x2+y2^2*x1
 * h3 = 2*x1*y1*x2-2*x2*y2*x1-y1^2*y2+y2^2*y1
 * h4 = y1^2*x2-y2^2*x1
 *
 * 0 = h1*c^3+h2*c^2*s+h3*c+h4*s
 * 0 = (h1*c^3+h3*c)^2-(h2*c^2+h4)^2(1-c^2)
 * 0 = (h1^2*c^6+2*h1*h3*c^4+h3^2*c^2)-(h2^2*c^4+2*h2*h4c^2+h4^2)*(1-c^2)
 * 0 = (h1^2*c^6+2*h1*h3*c^4+h3^2*c^2)-(h2^2*c^4+2*h2*h4c^2+h4^2)+(h2^2*c^6+2*h2*h4c^4+h4^2*c^2)
 * 0 = ((h1^2+h2^2)*c^6+(2*h1*h3-h2^2+2*h2*h4)*c^4+(h3^2-2*h2*h4+h4^2)*c^2)-h4^2
 *
 * k0 = h1^2+h2^2
 * k1 = {2*h1*h3-h2^2+2*h2*h4}/{k0}
 * k2 = {h3^2-2*h2*h4+h4^2}/{k0}
 * k3 = {-h4^2}/{k0}
 *
 * 0 = c^6+k1*c^4+k2*c^2+k3
 */

#include <cstdio>
#include <complex>
#include <cmath>
#include <algorithm>
#include <set>
#include <glm/vec2.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "pwx.h"

#define M_PI 3.141592653589793

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

/**
 * @brief Finds a root of a function using a numerical method
 * 
 * This function implements a modified secant method to find where f(x) = 0.
 * It uses the xpoint function to calculate the x-intercept of the line
 * connecting two points on the function, then iteratively refines the solution.
 * 
 * Algorithm details:
 * 1. Evaluate function at both endpoints of the bracket [x1, x2]
 * 2. Calculate a new approximation using linear interpolation (via xpoint)
 * 3. Replace one of the bracket endpoints with the new approximation
 * 4. Repeat until convergence (when |f(x)| is small enough)
 * 
 * The method has safeguards to ensure convergence:
 * - Limits the number of iterations to prevent infinite loops
 * - Uses a tolerance check to determine when sufficient accuracy is reached
 * 
 * @param x1 First bound of the interval
 * @param x2 Second bound of the interval
 * @return Approximation of the root
 */
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

/**
 * @brief Solves a cubic equation x^3 + k1*x^2 + k2*x + k3 = 0 using complex number approach
 * 
 * This implementation uses Cardano's formula with complex numbers to find all roots
 * of the cubic equation. It transforms the equation to the form y^3 + py + q = 0
 * by substituting x = y - k1/3, then applies the cubic formula.
 * 
 * Mathematical steps:
 * 1. Substitution: x = y - k1/3 to eliminate the quadratic term
 * 2. This transforms the equation to: y^3 + py + q = 0
 *    where p = k2 - k1^2/3 and q = k3 - k1*k2/3 + 2k1^3/27
 * 3. Using Cardano's formula:
 *    - Calculate discriminant: Δ = (q/2)^2 + (p/3)^3
 *    - Find u = cbrt(-q/2 + sqrt(Δ)) and v = cbrt(-q/2 - sqrt(Δ))
 *    - The three roots are: y1 = u + v, y2 = u*ω + v*ω^2, y3 = u*ω^2 + v*ω
 *      where ω is the complex cube root of unity (e^(2πi/3))
 * 4. Transform back to x = y - k1/3 to get the original roots
 * 5. Filter out roots with significant imaginary parts
 * 
 * @param rr Output vector to store the real roots found
 * @param k1 Coefficient of x^2
 * @param k2 Coefficient of x
 * @param k3 Constant term
 */
void solve3_a(std::vector<double>* rr, double k1, double k2, double k3) {
  using namespace std;
  typedef complex<double> cplx;

  vector<double> roots;

  // Step 1: Substitution x = y - k1/3 to eliminate quadratic term
  double a_over_3 = k1 / 3.0;
  // Calculate transformed coefficients p and q
  double p = k2 - k1 * a_over_3;  // p = k2 - k1²/3
  double q = 2.0 * a_over_3 * a_over_3 * a_over_3 - a_over_3 * k2 + k3;  // q = k3 - k1*k2/3 + 2k1³/27

  // Step 2: Calculate discriminant Δ = (q/2)² + (p/3)³
  cplx discriminant = cplx(q*q / 4.0 + p * p*p / 27.0, 0.0);

  // Step 3: Calculate square root of discriminant for Cardano's formula
  cplx sqrt_disc = sqrt(discriminant);

  // Define complex cube roots of unity
  // ω = -1/2 + i√3/2 (e^(2πi/3))
  const cplx omega1(-0.5, sqrt(3) / 2.0);  // ω = e^(2πi/3)
  const cplx omega2(-0.5, -sqrt(3) / 2.0); // ω² = e^(4πi/3)

  // Calculate u and v from Cardano's formula
  cplx u = pow(cplx(-q / 2.0, 0.0) + sqrt_disc, 1.0 / 3.0); // u = ∛(-q/2 + √Δ)
  cplx v = pow(cplx(-q / 2.0, 0.0) - sqrt_disc, 1.0 / 3.0); // v = ∛(-q/2 - √Δ)

  // Calculate the three roots in the transformed equation
  cplx y1 = u + v;                    // First root: y₁ = u + v
  cplx y2 = u * omega1 + v * omega2;  // Second root: y₂ = ωu + ω²v
  cplx y3 = u * omega2 + v * omega1;  // Third root: y₃ = ω²u + ωv

  // Transform back to the original equation: x = y - k1/3
  cplx yy[3] = { y1 - a_over_3, y2 - a_over_3, y3 - a_over_3 };

  printf("[%lf + %lf*i]\n", yy[0].real(), yy[0].imag());
  printf("[%lf + %lf*i]\n", yy[1].real(), yy[1].imag());
  printf("[%lf + %lf*i]\n", yy[2].real(), yy[2].imag());

  // Filter out roots with significant imaginary parts
  for (int i = 0; i < 3; i++) {
    if (abs(yy[i].imag()) < 1e-6)
      rr->push_back(yy[i].real());
  }

}


/**
 * @brief Solves a cubic equation x³ + k1x² + k2x + k3 = 0 using an alternative approach
 * 
 * @param k1 Coefficient of x²
 * @param k2 Coefficient of x
 * @param k3 Constant term
 * @return The real part of the first root found
 */
double solve3_b(double k1, double k2, double k3) {
  using namespace std::complex_literals; // Enable complex literals (e.g., 1i)
  std::vector<std::complex<double>> roots(3);

  // Step 1: Variable substitution to eliminate the quadratic term, converting to y³ + p*y + q = 0
  double p = k2 - (k1 * k1) / 3.0;
  double q = (2.0 * k1 * k1 * k1) / 27.0 - (k1 * k2) / 3.0 + k3;

  // Step 2: Calculate discriminant-related parameters
  std::complex<double> delta = (q / 2.0) * (q / 2.0) + (p / 3.0) * (p / 3.0) * (p / 3.0);
  std::complex<double> sqrt_delta = std::sqrt(delta);
  std::complex<double> C1 = -q / 2.0 + sqrt_delta;
  std::complex<double> C2 = -q / 2.0 - sqrt_delta;

  // Step 3: Calculate cube roots (using complex plane cube roots to cover all branches)
  std::complex<double> u = std::pow(C1, 1.0 / 3.0);
  std::complex<double> v = std::pow(C2, 1.0 / 3.0);

  // The two complex cube roots of unity (other than 1)
  const std::complex<double> omega = (-1.0 + std::sqrt(3.0) * 1i) / 2.0;
  const std::complex<double> omega2 = (-1.0 - std::sqrt(3.0) * 1i) / 2.0;

  // Step 4: Calculate the three roots of y
  std::complex<double> y1 = u + v;
  std::complex<double> y2 = u * omega + v * omega2;
  std::complex<double> y3 = u * omega2 + v * omega;

  // Step 5: Convert back to roots of x (x = y - k1/3)
  std::complex<double> shift = -k1 / 3.0;
  roots[0] = y1 + shift;
  roots[1] = y2 + shift;
  roots[2] = y3 + shift;

  printf("[%lf + %lf*i]\n", roots[0].real(), roots[0].imag());
  printf("[%lf + %lf*i]\n", roots[1].real(), roots[1].imag());
  printf("[%lf + %lf*i]\n", roots[2].real(), roots[2].imag());

  return roots[0].real();
}

/**
 * @brief Safely computes the cube root of a complex number
 * @param z Complex number input
 * @return Cube root of z, handling special cases for real numbers
 */
std::complex<double> cube_root(std::complex<double> z) {
  if (std::abs(z.imag()) < 1e-15) { // Special handling for real complex numbers
    double re = z.real();
    if (re >= 0) return std::complex<double>(std::pow(re, 1.0 / 3.0), 0.0);
    else return std::complex<double>(-std::pow(-re, 1.0 / 3.0), 0.0);
  }
  return std::pow(z, 1.0 / 3.0);
}

/**
 * @brief Solves a cubic equation x³ + k1x² + k2x + k3 = 0 with improved numerical stability
 * 
 * This implementation handles different cases based on the discriminant:
 * - When delta >= 0: Uses Cardano's formula (1 real root, 2 complex conjugate roots)
 * - When delta < 0: Uses trigonometric solution (3 real roots)
 * 
 * Key improvements in this implementation:
 * 1. Special handling for different discriminant cases to improve numerical stability
 * 2. Uses trigonometric method for the three real roots case to avoid complex arithmetic errors
 * 3. Handles potential division by zero and branch selection issues
 * 4. Cleans up numerical noise in the results (removing tiny imaginary components)
 * 
 * The trigonometric solution (when delta < 0) uses the identity:
 *   x_k = 2*sqrt(-p/3)*cos((phi + 2πk)/3) - a/3  for k=0,1,2
 *   where phi = acos(-q/(2*sqrt((-p/3)^3)))
 * 
 * @param rr Output vector to store the real roots found
 * @param k1 Coefficient of x²
 * @param k2 Coefficient of x
 * @param k3 Constant term
 */
void solve3_c(std::vector<double>* rr, double k1, double k2, double k3) {

  using namespace std::complex_literals;
  std::vector<std::complex<double>> roots(3);

  // Step 1: Transform to the form y³ + py + q = 0 by substituting x = y - k1/3
  // This eliminates the quadratic term
  double p = k2 - (k1 * k1) / 3.0;  // p = k2 - k1²/3
  double q = (2.0 * k1 * k1 * k1) / 27.0 - (k1 * k2) / 3.0 + k3;  // q = k3 - k1*k2/3 + 2k1³/27

  // Step 2: Calculate the discriminant Δ = (q/2)² + (p/3)³
  // This determines whether we have 1 real + 2 complex roots (Δ ≥ 0) or 3 real roots (Δ < 0)
  double delta = (q * q) / 4.0 + (p * p * p) / 27.0;
  std::complex<double> shift = -k1 / 3.0;  // Shift value to transform back to original variable x

  if (delta >= 0) {
    // Case 1: Δ ≥ 0 (1 real root, 2 complex conjugate roots)
    // Using Cardano's formula with complex numbers
    
    // Calculate square root of discriminant
    std::complex<double> sqrt_delta = std::sqrt(std::complex<double>(delta, 0.0));
    
    // Calculate C1 = -q/2 + √Δ and C2 = -q/2 - √Δ
    std::complex<double> C1 = -q / 2.0 + sqrt_delta;
    std::complex<double> C2 = -q / 2.0 - sqrt_delta;

    // Calculate u = ∛C1 and v = ∛C2 using the custom cube_root function
    // which handles special cases for real numbers
    std::complex<double> u = cube_root(C1);
    std::complex<double> v = cube_root(C2);

    // Ensure the constraint u*v = -p/3 is satisfied
    // This corrects potential branch selection errors in the cube root calculation
    if (std::abs(u * v + p / 3.0) > 1e-10) {
      v = -p / (3.0 * u);
    }

    // Define complex cube roots of unity
    // ω = e^(2πi/3) = -1/2 + i√3/2
    const std::complex<double> omega = (-1.0 + std::sqrt(3.0) * 1i) / 2.0;
    // ω² = e^(4πi/3) = -1/2 - i√3/2
    const std::complex<double> omega2 = (-1.0 - std::sqrt(3.0) * 1i) / 2.0;

    roots[0] = u + v + shift;
    roots[1] = u * omega + v * omega2 + shift;
    roots[2] = u * omega2 + v * omega + shift;
  }
  else {
    // Case 2: Δ < 0 (three distinct real roots)
    // Using trigonometric solution for better numerical stability
    
    // Calculate r = √((-p/3)³) for the trigonometric formula
    double r = std::sqrt(std::pow(-p / 3.0, 3.0));
    
    // Calculate φ = arccos(-q/(2r)) for the angle in the trigonometric formula
    double phi = std::acos(-q / (2.0 * r));
    
    // Calculate √(-p/3) which appears in all three root formulas
    double sqrt_p_over_3 = std::sqrt(-p / 3.0);

    // Three real roots (using trigonometric formulas)
    roots[0] = std::complex<double>(2 * sqrt_p_over_3 * std::cos(phi / 3.0), 0.0) + shift;
    roots[1] = std::complex<double>(2 * sqrt_p_over_3 * std::cos((phi + 2 * M_PI) / 3.0), 0.0) + shift;
    roots[2] = std::complex<double>(2 * sqrt_p_over_3 * std::cos((phi - 2 * M_PI) / 3.0), 0.0) + shift;
  }

  // Clean up tiny imaginary parts (numerical errors)
  auto clean = [](std::complex<double> c) {
    if (std::abs(c.imag()) < 1e-10) {
      return std::complex<double>(c.real(), 0.0);
    }
    return c;
  };
  for (auto& root : roots) root = clean(root);

  printf("[%lf + %lf*i]\n", roots[0].real(), roots[0].imag());
  printf("[%lf + %lf*i]\n", roots[1].real(), roots[1].imag());
  printf("[%lf + %lf*i]\n", roots[2].real(), roots[2].imag());

  for (int i = 0; i < 3; i++) {
    if (abs(roots[i].imag()) < 1e-6)
      rr->push_back(roots[i].real());
  }
}

double sign(double x) {
  if (x < 0) return -1.0;
  return 1.0;
}

/**
 * @brief Primary implementation for solving cubic equation x³ + k1x² + k2x + k3 = 0
 * 
 * This function uses a numerically stable approach to find all real roots of the cubic equation.
 * It handles edge cases and returns whether new roots were found.
 * 
 * @param rr Output vector to store the real roots found
 * @param k1 Coefficient of x²
 * @param k2 Coefficient of x
 * @param k3 Constant term
 * @return true if new roots were found, false otherwise
 */
bool solve3(std::vector<double>* rr, double k1, double k2, double k3) {
  bool ret = false;
  using cplx = std::complex<double>;
  const double PI = std::acos(-1.0);
  const double EPS_IMAG = 1e-12;
  rr->clear();

  // Reduction: x = y - k1/3
  double a_over_3 = k1 / 3.0;
  double p = k2 - k1 * a_over_3;
  double q = 2.0 * a_over_3 * a_over_3 * a_over_3 - a_over_3 * k2 + k3;

  // Δ (can be positive, zero, or negative)
  double delta_real = (q*q) / 4.0 + (p*p*p) / 27.0;
  cplx Delta = cplx(delta_real, 0.0);

  // Complex sqrt(Delta)
  cplx sqrtD = std::sqrt(Delta);
  //if (delta_real < 0.0)
  //  Delta = -Delta;

  // A and B
  cplx A = cplx(-q / 2.0, 0.0) + sqrtD;
  //cplx B = cplx(-q/2.0, 0.0) - sqrtD; // No need to take the cube root separately

  // Take a cube root u0 of A (using principal value)
  cplx u0 = std::pow(A, 1.0 / 3.0);
  if (delta_real >= 0) {
    u0 = cplx(sign(A.real()) * std::pow(std::abs(A.real()), 1.0 / 3.0), 0.0);
  }

  //// 如果 u0 太接近 0（数值不稳定），可改为从 B 取根再反算 u0
  //if (std::abs(u0) < 1e-16) {
  //  cplx B = cplx(-q / 2.0, 0.0) - sqrtD;
  //  cplx v0temp = std::pow(B, 1.0 / 3.0);
  //  if (std::abs(v0temp) < 1e-16) {
  //    // 极端退化情况，退回用简单方法（y=0的近似）
  //    u0 = cplx(0.0, 0.0);
  //  }
  //  else {
  //    u0 = cplx(-p / 3.0, 0.0) / v0temp;
  //  }
  //}

  cplx v0 = cplx(-p / 3.0, 0.0) / u0;
  cplx omega = std::polar(1.0, 2.0 * PI / 3.0);

  static double last_roots[6];
  static bool init = false;
  for (int k = 0; k < 3; ++k) {
    cplx uk = u0 * std::pow(omega, k);
    cplx vk = v0 * std::pow(omega, -k);
    cplx yk = uk + vk;
    cplx xk = yk - a_over_3;

    if (k == 0 && (!init || abs(last_roots[0] - xk.real()) > 1e-6 || abs(last_roots[1] - xk.imag()) > 1e-6)) {
      last_roots[0] = xk.real();
      last_roots[1] = xk.imag();
      ret = true;
    }
    if (ret)
      printf("### %lf + %lf*i\n", xk.real(), xk.imag());

    if (std::abs(xk.imag()) < EPS_IMAG) {
      double xr = xk.real();
      rr->push_back(xr);
    }
  }
  init = true;

  return ret;
}

/**
 * @brief Computes the amplitude parameter A for a rotated parabola
 * 
 * This function calculates the amplitude parameter A for a rotated parabola Y = A*X²
 * given a point (x,y) on the parabola and the rotation angle theta.
 * 
 * The calculation transforms the point from the original coordinate system to the
 * rotated coordinate system where the parabola has the standard form Y = A*X²:
 * - xx = x*cos(theta) + y*sin(theta)     (X-coordinate in rotated system)
 * - yy = -x*sin(theta) + y*cos(theta)    (Y-coordinate in rotated system)
 * 
 * Then it solves for A using the equation: yy = A*(xx²)
 * 
 * @param x x-coordinate of a point on the parabola in original system
 * @param y y-coordinate of a point on the parabola in original system
 * @param theta rotation angle in radians
 * @return The amplitude parameter A of the rotated parabola
 */
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

/**
 * @brief Calculates the residual for a point on a rotated parabola
 * 
 * @param x X-coordinate of the point
 * @param y Y-coordinate of the point
 * @param A Amplitude of the parabola
 * @param theta Rotation angle of the parabola
 * @return The residual value (how far the point is from the parabola)
 */
double f2(double x, double y, double A, double theta) {
	double xx = x*std::cos(theta)+y*std::sin(theta);
	double yy = -x*std::sin(theta)+y*std::cos(theta);
	return A*xx*xx - yy;
}

/**
 * @brief Calculates the residual function for two points on a rotated parabola
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param theta Rotation angle of the parabola
 * @return The residual value for the given rotation angle
 */
double F(double x1, double y1, double x2, double y2, double theta) {
	double a = compute_trial_a(x1, y1, theta);
	return f2(x2, y2, a, theta);
}

/**
 * @brief Calculates the derivative of the residual function with respect to theta
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param theta Rotation angle of the parabola
 * @return The derivative of the residual function
 */
double dF(double x1, double y1, double x2, double y2, double theta) {
	double d = 0.0;
	double c = std::cos(theta);
	double s = std::sin(theta);
	double t1 = -x1 * s + y1 * c;
	double t2 = x2 * c + y2 * s;
	double t3 = x1 * c + y1 * s;
	double t4 = -x2 * s + y2 * c;

	d += t2;
	d += t1 * t2 * t2 * (-2) / t3 / t3 / t3 * t1;
	d += (-t3 * t2 *t2 + t1 * 2 * t2 * t4) / t3 / t3;
	return d;
}

double sin_t(double x1, double y1, double x2, double y2, double c, double s) {
	double t1 = -x1 * s + y1 * c;
	double t2 = x2 * c + y2 * s;
	double t3 = x1 * c + y1 * s;
	double t4 = -x2 * s + y2 * c;
	return (t1 * t2 * t2 / t3 / t3 + y2 * c) / x2;
}

double cos_t(double x1, double y1, double x2, double y2, double c, double s) {
	double t1 = -x1 * s + y1 * c;
	double t2 = x2 * c + y2 * s;
	double t3 = x1 * c + y1 * s;
	double t4 = -x2 * s + y2 * c;
	return -(t1 * t2 * t2 / t3 / t3 + x2 * s) / y2;
}

double f3(double x1, double y1, double x2, double y2, double c, double s) {
	double t1 = -x1 * s + y1 * c;
	double t2 = x2 * c + y2 * s;
	double t3 = x1 * c + y1 * s;
	double t4 = -x2 * s + y2 * c;
	return t1 * t2 * t2 - t4 * t3 * t3;
}

double f4(double x1, double y1, double x2, double y2, double t) {
	double c = std::cos(t);
	double s = std::sin(t);
	double t1 = -x1 * s + y1 * c;
	double t2 = x2 * c + y2 * s;
	double t3 = x1 * c + y1 * s;
	double t4 = -x2 * s + y2 * c;
	return t1 * t2 * t2 - t4 * t3 * t3;
}

/**
 * @brief Checks if a given rotation angle is a valid solution for the rotated parabola
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param theta Rotation angle to check
 * @param A Optional pointer to store the calculated amplitude
 * @param residual Optional pointer to store the calculated residual
 * @return True if the angle is a valid solution, false otherwise
 */
bool is_a_solution(double x1, double y1, double x2, double y2, double theta, double* A = nullptr, double* residual = nullptr) {
	double a = compute_trial_a(x1, y1, theta);
	double r = f2(x2, y2, a, theta);
	if (residual)
		*residual = r;
	if (A)
		*A = a;
	return (std::abs(r) < 1e-1);
}

/**
 * @brief Computes the remaining roots of a cubic equation given one root
 * 
 * This function uses polynomial division to reduce the cubic equation to a quadratic
 * equation after one root is known, then solves the quadratic to find the other two roots.
 * 
 * @param roots Vector to store the computed roots
 * @param k1 Coefficient of x²
 * @param k2 Coefficient of x
 * @param k3 Constant term
 * @param root1 One known root of the cubic equation
 */
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

/**
 * @brief Finds a root using the bisection method for the rotated parabola problem
 * 
 * This function implements the bisection method to find a value of theta where
 * the residual function f4 equals zero. The bisection method repeatedly divides
 * an interval in half and selects the subinterval where the function changes sign.
 * 
 * Algorithm steps:
 * 1. Evaluate residuals at both endpoints of the interval [t_low, t_high]
 * 2. Check if the function changes sign across the interval
 * 3. Repeatedly divide the interval in half, selecting the subinterval where
 *    the function changes sign
 * 4. Continue until the residual is sufficiently small or max iterations reached
 * 
 * @param x1 x-coordinate of first point on the parabola
 * @param y1 y-coordinate of first point on the parabola
 * @param x2 x-coordinate of second point on the parabola
 * @param y2 y-coordinate of second point on the parabola
 * @param t_low Lower bound of the initial interval for theta
 * @param t_high Upper bound of the initial interval for theta
 * @return The approximated value of theta where f4 equals zero
 */
double solve_with_bisect(double x1, double y1, double x2, double y2, double t_low, double t_high) {
	double residual = 1.0, a;

	a = compute_trial_a(x1, y1, t_low);
	double low_residual = f2(x2, y2, a, t_low);
	a = compute_trial_a(x1, y1, t_high);
	double high_residual = f2(x2, y2, a, t_high);
	// printf("residual: %lf, t: %lf\n", high_residual, glm::degrees(t_low));
	// printf("residual: %lf, t: %lf\n", low_residual, glm::degrees(t_high));

	if (low_residual * high_residual > 0.0) {
		printf("No root found...\n");
		return -9999.0;
	}

	double t, r;
	int count = 0;
	do {
		t = (t_low + t_high) / 2;
		a = compute_trial_a(x1, y1, t);
		residual = f2(x2, y2, a, t);
		//printf("residual: %lf, t: %lf, range[%lf, %lf]\n", residual, glm::degrees(t), glm::degrees(t_low), glm::degrees(t_high));
		if (residual * high_residual <= 0.0) {
			t_low = t;
			low_residual = residual;
		} 
		else {
			t_high = t;
			high_residual = residual;
		}
		r = f4(x1, y1, x2, y2, t);
		++count;
	} while (std::abs(r) >= 1e-6 && count < 100000);
	printf("num of bisect(): %d, residual: %e, %f\n", count, r, t);
	return t;
}

/**
 * @brief Finds a root using the secant method
 * 
 * Implements the secant method to find the rotation angle that makes the residual zero
 * for a rotated parabola passing through the given points.
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param t_low Lower bound for the rotation angle
 * @param t_high Upper bound for the rotation angle
 * @return The rotation angle that minimizes the residual
 */
double solve_with_secant(double x1, double y1, double x2, double y2, double t_low, double t_high) {
	double residual = 1.0, a;

	double low_residual = f4(x1, y1, x2, y2, t_low);
	double high_residual = f4(x1, y1, x2, y2, t_high);
	// printf("residual: %lf, t: %lf\n", high_residual, glm::degrees(t_low));
	// printf("residual: %lf, t: %lf\n", low_residual, glm::degrees(t_high));

	if (low_residual * high_residual > 0.0) {
		printf("No root found...\n");
		return -9999.0;
	}

	double r;
	double alpha = std::abs(low_residual) / (std::abs(low_residual) + std::abs(high_residual));
	double t = alpha * t_high + (1 - alpha) * t_low;
	int count = 0;
	do {
		residual = f4(x1, y1, x2, y2, t);
		//printf("residual: %.11lf, t: %lf, range[%lf, %lf]\n", residual, glm::degrees(t), glm::degrees(t_low), glm::degrees(t_high));
		if (residual * high_residual <= 0.0) {
			t_low = t;
			low_residual = residual;
		}
		else {
			t_high = t;
			high_residual = residual;
		}
		alpha = std::abs(low_residual) / (std::abs(low_residual) + std::abs(high_residual));
		t = alpha * t_high + (1 - alpha) * t_low;
		r = f4(x1, y1, x2, y2, t);
		count++;
	} while (std::abs(r) >= 1e-6 && count <= 100000);

	printf("num of secant(): %d, residual: %e, %f\n", count, r, t);
	return t;
}

/**
 * @brief Finds a root using Newton's method
 * 
 * Implements Newton's method to find the rotation angle that makes the residual zero
 * for a rotated parabola passing through the given points.
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param t_low Lower bound for the rotation angle
 * @param t_high Upper bound for the rotation angle
 * @return The rotation angle that minimizes the residual
 */
double solve_with_newton_method(double x1, double y1, double x2, double y2, double t_low, double t_high) {
	double t = (t_low + t_high) / 2;
	double f = F(x1, y1, x2, y2, t);
	int count = 0;
	double r;
	do {
		//printf("num of newton(): %d, t: %.10f, residual: %e\n", count, t, f);
		double df = dF(x1, y1, x2, y2, t);
		//double f2 = F(x1, y1, x2, y2, t+0.0001);
		//double df2 = (f2 - f) / 0.0001;
		t -= f / df;
		//t -= f / df2;
		//printf("dF:  %.10f\n", df);
		//printf("dF2: %.10f\n\n", df2);
		f = F(x1, y1, x2, y2, t);
		r = f4(x1, y1, x2, y2, t);
		count++;
	} while (std::abs(r) >= 1e-6 && count <= 100000);
	printf("num of newton(): %d, residual: %e, %f\n", count, r, t);
	return t;
}

/**
 * @brief Finds a root using fixed-point iteration
 * 
 * Implements fixed-point iteration to find the rotation angle for a rotated parabola.
 * 
 * @param x1 X-coordinate of the first point
 * @param y1 Y-coordinate of the first point
 * @param x2 X-coordinate of the second point
 * @param y2 Y-coordinate of the second point
 * @param t_low Lower bound for the rotation angle
 * @param t_high Upper bound for the rotation angle
 * @return The rotation angle that satisfies the fixed-point equation
 */
double solve_with_fixed_point_theorem(double x1, double y1, double x2, double y2, double t_low, double t_high) {
	double t = (t_low + t_high) / 2;
	double tt = t;
	double c = std::cos(t);
	double s = std::sin(t);
	double r = f3(x1, y1, x2, y2, c, s);
  double rr;
	//printf("num of fixed_point(): %d, residual: %e\n", -1, r);
	int count = 0;
	do {
		t = tt;
		double ff = f3(x1, y1, x2, y2, c, s);
		s = s - 0.0000001 * ff;
		double r1 = f3(x1, y1, x2, y2, sqrt(1 - s * s), s);
		double r2 = f3(x1, y1, x2, y2, -sqrt(1 - s * s), s);
		if (std::abs(r1) < std::abs(r2)) {
			c = sqrt(1 - s * s);
			r = r1;
		}
		else {
			c = -sqrt(1 - s * s);
			r = r2;
		}

		count++;
		tt = std::atan2(s, c);
		rr = f4(x1, y1, x2, y2, tt);
		//printf("num of fixed_point(): %d, c: %f, t: %f, residual: %e\n", count, c, tt, rr);
	} while (std::abs(rr) >= 1e-6 && count <= 100000);
	printf("num of fixed_point(): %d, residual: %e, %f\n", count, r, tt);
	return tt;
}

/**
 * @brief Finds parameters of a rotated parabola passing through given points
 * 
 * This function solves the rotated parabola problem to find amplitude A and rotation angle θ
 * for a parabola Y = A*X² in a rotated coordinate system that passes through specified points.
 * It uses multiple numerical methods to find the solution and handles various edge cases.
 * 
 * The algorithm follows these steps:
 * 1. Translate points relative to the parabola vertex (x0,y0)
 * 2. Determine appropriate search ranges for the rotation angle
 * 3. Apply numerical methods (bisection, secant, Newton's) to find an initial solution
 * 4. Calculate the cubic equation in cos²(θ) derived from the mathematical constraints
 * 5. Find all roots of this equation to identify all possible solutions
 * 6. Verify each potential solution and store valid ones
 * 
 * @param xy Vector containing coordinates of points [x1, y1, x2, y2, x0, y0] where:
 *           (x1,y1) and (x2,y2) are points on the parabola
 *           (x0,y0) is the vertex of the parabola
 * @param A_array Output vector to store the amplitude values of valid solutions
 * @param theta_array Output vector to store the rotation angles of valid solutions
 */
void arc(const std::vector<double>& xy, std::vector<double>* A_array, std::vector<double>* theta_array) {
	// Translate points relative to the vertex (x0,y0)
	double x1 = xy[0]-xy[4];
	double y1 = xy[1]-xy[5];
	double x2 = xy[2]-xy[4];
	double y2 = xy[3]-xy[5];

	// Calculate potential angle ranges based on point positions
	// Subtract π/2 to account for the rotation between parabola axis and coordinate system
	double t_low1 = std::atan2(y1, x1) - pi0_5;
	double t_high1 = std::atan2(y2, x2) - pi0_5;
	// Consider additional ranges by adding 2π
	double t_low2 = t_low1 + pi2;
	double t_high2 = t_high1 + pi2;
	
	// Find the smallest angle range to search in by comparing all combinations
	double delta11 = std::fabs(t_high1 - t_low1);
	double delta12 = std::fabs(t_high1 - t_low2);
	double delta21 = std::fabs(t_high2 - t_low1);
	double delta22 = std::fabs(t_high2 - t_low2);
	double t_low, t_high;
	
	// Select the range with smallest angular distance
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

	// Ensure t_low < t_high for the search algorithms
	if (t_high < t_low)
		std::swap(t_low, t_high);
	
	double t, a;
	// printf("v1: [%lf, %lf]\n", x1, y1);
	// printf("v2: [%lf, %lf]\n", x2, y2);
	// printf("t range[%lf, %lf]\n", glm::degrees(t_low), glm::degrees(t_high));

	// Try different numerical methods to find the rotation angle
	// Each method is an alternative approach, not a refinement
	// Using bisection method (most robust but slowest)
	double t_bisect = solve_with_bisect(x1, y1, x2, y2, t_low, t_high);
	// Using secant method (faster but less robust)
	double t_secant = solve_with_secant(x1, y1, x2, y2, t_low, t_high);
	// Using Newton's method (fastest but requires good initial guess)
	double t_newton = solve_with_newton_method(x1, y1, x2, y2, t_low, t_high);
	
	// Choose the result from Newton's method as it's typically most accurate
	// when all methods converge
	t = t_newton;
	//t = solve_with_fixed_point_theorem(x1, y1, x2, y2, t_low, t_high);

	// Store the first solution found
	double theta1 = t;
	double a1 = compute_trial_a(x1, y1, t);
	double c = std::cos(theta1);
	// double s = std::sin(theta1);
	double residual = f2(x2, y2, a1, theta1);
	double root1 = c*c;  // cos²(θ) is a root of our cubic equation
	A_array->emplace_back(a1);
	theta_array->emplace_back(theta1);

	// Calculate coefficients for the cubic equation in cos²(θ)
	// These are derived from the mathematical constraints explained in the comments above
	double h1 = x2*x2*y1-x1*x1*y2-2*x1*y1*x2+2*x2*y2*x1+y1*y1*y2-y2*y2*y1;
	double h2 = 2*x2*y2*y1-2*x1*y1*y2+x1*x1*x2-x2*x2*x1-y1*y1*x2+y2*y2*x1;
	double h3 = 2*x1*y1*x2-2*x2*y2*x1-y1*y1*y2+y2*y2*y1;
	double h4 = y1*y1*x2-y2*y2*x1;

	// Normalize the coefficients for the standard form of the cubic equation
	double k0 = h1*h1+h2*h2;
	double k1 = (2*h1*h3-h2*h2+2*h2*h4)/k0;
	double k2 = (h3*h3-2*h2*h4+h4*h4)/k0;
	double k3 = -(h4*h4)/k0;

	// Print the resulting cubic equation in c² (where c = cos(θ))
	printf("equation: c2^3 + %lf*c2^2 + %lf*c2 + %lf = 0\n", k1, k2, k3);
	printf("equation: y=x^3 + (%lf)*x^2 + (%lf)*x + (%lf)\n", k1, k2, k3);

	// Find all roots of the cubic equation
	std::vector<double> roots;
#if 0
	// Alternative method using solve3 (disabled by default)
	printf("x1 = %lf\n", x1);
	printf("y1 = %lf\n", y1);
	printf("x2 = %lf\n", x2);
	printf("y2 = %lf\n", y2);
	static double last_root;
	bool ret = solve3(&roots, k1, k2, k3);
	if (!roots.empty()) {
		printf("root by solve3: %lf\n", roots[0]);
		if (ret)
			for (int i = 0; i < 3; ++i) {
				if (abs(root1 - roots[i]) < 1e-4) {
					printf("### %lf + 0.0*i\n", roots[i]);
					last_root = roots[i];
					break;
				}
			}
	}
	A_array->clear();
	theta_array->clear();
#else
	// Find other roots of the cubic equation given that root1 is already known
	compute_other_roots_for_equ3(&roots, k1, k2, k3, root1);
	//std::sort(roots.begin(), roots.end());
#endif

	// Print information about the first solution
	{
		double c = std::cos(theta1);
		double s = std::sin(theta1);
		printf("root1: %.10lf\n", root1);
		printf("%lf*((%lf)*x+(%lf)*y)^2 = (%lf)*x+(%lf)*y\n", a1, c, s, -s, c);
		printf("   [%ld]: theta=%14.10f, A=%14.10f, residual=%13.10f\n", 9L, std::fmod(glm::degrees(theta1)+3600.0, 360.0), a1, residual);
	}

	// Process each root to find all possible solutions
	for(auto& r : roots) {
		printf("root: %.10lf\n", r);
		// Convert cos²(θ) to θ
		double theta = std::acos(std::min(1.0, std::sqrt(r)));
		
		// Check if this angle gives a valid solution
		if (is_a_solution(x1, y1, x2, y2, theta, &a, &residual)) {
			// Handle sign of amplitude A (positive A is preferred)
			if (a > 0) {
				A_array->emplace_back(a);
				theta_array->emplace_back(theta);
			} else {
				// If A is negative, use equivalent form with positive A
				A_array->emplace_back(-a);
				theta_array->emplace_back(theta + pi);
			}
			// Print the solution in parametric form
			double c = std::cos(theta_array->back());
			double s = std::sin(theta_array->back());
			printf("%lf*((%lf)*x+(%lf)*y)^2 = (%lf)*x+(%lf)*y\n", A_array->back(), c, s, -s, c);
			printf("   [%ld]: theta=%14.10f, A=%14.10f, residual=%13.10f\n", &r-roots.data(), std::fmod(glm::degrees(theta_array->back())+3600.0, 360.0), A_array->back(), residual);
		} else {
			printf("                                                residual=%13.10f\n", residual);
		}
		
		// Also check the negative angle (-θ) which gives the same cos²(θ)
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
