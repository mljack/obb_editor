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

// 求解三次方程 x^3 + k1*x^2 + k2*x + k3 = 0 的实根
void solve3_a(std::vector<double>* rr, double k1, double k2, double k3) {
  using namespace std;
  typedef complex<double> cplx;

  vector<double> roots;

  // 降阶：x = y - k1/3
  double a_over_3 = k1 / 3.0;
  double p = k2 - k1 * a_over_3;
  double q = 2.0 * a_over_3 * a_over_3 * a_over_3 - a_over_3 * k2 + k3;

  // 判别式
  cplx discriminant = cplx(q*q / 4.0 + p * p*p / 27.0, 0.0);

  // 统一使用复数计算
  cplx sqrt_disc = sqrt(discriminant);

  // 三次方程立方根的三条主支公式
  const cplx omega1(-0.5, sqrt(3) / 2.0);  // ω = e^(2πi/3)
  const cplx omega2(-0.5, -sqrt(3) / 2.0);

  cplx u = pow(cplx(-q / 2.0, 0.0) + sqrt_disc, 1.0 / 3.0);
  cplx v = pow(cplx(-q / 2.0, 0.0) - sqrt_disc, 1.0 / 3.0);

  // 三个根
  cplx y1 = u + v;
  cplx y2 = u * omega1 + v * omega2;
  cplx y3 = u * omega2 + v * omega1;

  // 转回原方程 x = y - a/3
  cplx yy[3] = { y1 - a_over_3, y2 - a_over_3, y3 - a_over_3 };

  printf("[%lf + %lf*i]\n", yy[0].real(), yy[0].imag());
  printf("[%lf + %lf*i]\n", yy[1].real(), yy[1].imag());
  printf("[%lf + %lf*i]\n", yy[2].real(), yy[2].imag());

  // 忽略虚部很小的
  for (int i = 0; i < 3; i++) {
    if (abs(yy[i].imag()) < 1e-6)
      rr->push_back(yy[i].real());
  }

}


// 求解一元三次方程 x³ + k1x² + k2x + k3 = 0 的所有根
// 返回值：包含三个复数根的vector
double solve3_b(double k1, double k2, double k3) {
  using namespace std::complex_literals; // 启用复数字面量（如1i）
  std::vector<std::complex<double>> roots(3);

  // 步骤1：变量代换消去二次项，化为 y³ + p*y + q = 0
  double p = k2 - (k1 * k1) / 3.0;
  double q = (2.0 * k1 * k1 * k1) / 27.0 - (k1 * k2) / 3.0 + k3;

  // 步骤2：计算判别式相关参数
  std::complex<double> delta = (q / 2.0) * (q / 2.0) + (p / 3.0) * (p / 3.0) * (p / 3.0);
  std::complex<double> sqrt_delta = std::sqrt(delta);
  std::complex<double> C1 = -q / 2.0 + sqrt_delta;
  std::complex<double> C2 = -q / 2.0 - sqrt_delta;

  // 步骤3：计算三次方根（使用复平面三次方根，确保覆盖所有分支）
  std::complex<double> u = std::pow(C1, 1.0 / 3.0);
  std::complex<double> v = std::pow(C2, 1.0 / 3.0);

  // 三次单位根（除1外的两个）
  const std::complex<double> omega = (-1.0 + std::sqrt(3.0) * 1i) / 2.0;
  const std::complex<double> omega2 = (-1.0 - std::sqrt(3.0) * 1i) / 2.0;

  // 步骤4：计算y的三个根
  std::complex<double> y1 = u + v;
  std::complex<double> y2 = u * omega + v * omega2;
  std::complex<double> y3 = u * omega2 + v * omega;

  // 步骤5：转换回x的根（x = y - k1/3）
  std::complex<double> shift = -k1 / 3.0;
  roots[0] = y1 + shift;
  roots[1] = y2 + shift;
  roots[2] = y3 + shift;

  printf("[%lf + %lf*i]\n", roots[0].real(), roots[0].imag());
  printf("[%lf + %lf*i]\n", roots[1].real(), roots[1].imag());
  printf("[%lf + %lf*i]\n", roots[2].real(), roots[2].imag());

  return roots[0].real();
}

// 安全的复数三次方根（处理实部为正/负的情况）
std::complex<double> cube_root(std::complex<double> z) {
  if (std::abs(z.imag()) < 1e-15) { // 实复数特殊处理
    double re = z.real();
    if (re >= 0) return std::complex<double>(std::pow(re, 1.0 / 3.0), 0.0);
    else return std::complex<double>(-std::pow(-re, 1.0 / 3.0), 0.0);
  }
  return std::pow(z, 1.0 / 3.0);
}

// 求解一元三次方程 x³ + k1x² + k2x + k3 = 0 的所有根
void solve3_c(std::vector<double>* rr, double k1, double k2, double k3) {

  using namespace std::complex_literals;
  std::vector<std::complex<double>> roots(3);

  // 消去二次项：x = y - k1/3，化为 y³ + p*y + q = 0
  double p = k2 - (k1 * k1) / 3.0;
  double q = (2.0 * k1 * k1 * k1) / 27.0 - (k1 * k2) / 3.0 + k3;

  // 判别式：Δ = (q/2)² + (p/3)³
  double delta = (q * q) / 4.0 + (p * p * p) / 27.0;
  std::complex<double> shift = -k1 / 3.0; // 转换回x的偏移量

  if (delta >= 0) {
    // 情况1：Δ ≥ 0（1个实根，2个共轭复根），用卡尔达诺公式
    std::complex<double> sqrt_delta = std::sqrt(std::complex<double>(delta, 0.0));
    std::complex<double> C1 = -q / 2.0 + sqrt_delta;
    std::complex<double> C2 = -q / 2.0 - sqrt_delta;

    std::complex<double> u = cube_root(C1);
    std::complex<double> v = cube_root(C2);

    // 强制满足 u*v = -p/3（修正分支错误）
    if (std::abs(u * v + p / 3.0) > 1e-10) {
      v = -p / (3.0 * u);
    }

    const std::complex<double> omega = (-1.0 + std::sqrt(3.0) * 1i) / 2.0;
    const std::complex<double> omega2 = (-1.0 - std::sqrt(3.0) * 1i) / 2.0;

    roots[0] = u + v + shift;
    roots[1] = u * omega + v * omega2 + shift;
    roots[2] = u * omega2 + v * omega + shift;
  }
  else {
    // 情况2：Δ < 0（3个实根），用三角函数解法（避免复数开方误差）
    double r = std::sqrt(std::pow(-p / 3.0, 3.0)); // 半径
    double phi = std::acos(-q / (2.0 * r));       // 角度
    double sqrt_p_over_3 = std::sqrt(-p / 3.0);     // 辅助变量

    // 三个实根（用三角函数公式）
    roots[0] = std::complex<double>(2 * sqrt_p_over_3 * std::cos(phi / 3.0), 0.0) + shift;
    roots[1] = std::complex<double>(2 * sqrt_p_over_3 * std::cos((phi + 2 * M_PI) / 3.0), 0.0) + shift;
    roots[2] = std::complex<double>(2 * sqrt_p_over_3 * std::cos((phi - 2 * M_PI) / 3.0), 0.0) + shift;
  }

  // 清理微小虚部（数值误差）
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

bool solve3(std::vector<double>* rr, double k1, double k2, double k3) {
  bool ret = false;
  using cplx = std::complex<double>;
  const double PI = std::acos(-1.0);
  const double EPS_IMAG = 1e-12;
  rr->clear();

  // 降阶: x = y - k1/3
  double a_over_3 = k1 / 3.0;
  double p = k2 - k1 * a_over_3;
  double q = 2.0 * a_over_3 * a_over_3 * a_over_3 - a_over_3 * k2 + k3;

  // Δ（可为正、零或负）
  double delta_real = (q*q) / 4.0 + (p*p*p) / 27.0;
  cplx Delta = cplx(delta_real, 0.0);

  // 复数 sqrt(Delta)
  cplx sqrtD = std::sqrt(Delta);
  //if (delta_real < 0.0)
  //  Delta = -Delta;

  // A 和 B
  cplx A = cplx(-q / 2.0, 0.0) + sqrtD;
  //cplx B = cplx(-q/2.0, 0.0) - sqrtD; // 不需要单独挙取立方根

  // 取 A 的一个立方根 u0（使用主值）
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

  // 强制匹配 v0 使 u0 * v0 = -p/3
  cplx v0 = cplx(-p / 3.0, 0.0) / u0;

  // 三次单位根
  cplx omega = std::polar(1.0, 2.0 * PI / 3.0);

  static double last_roots[6];
  static bool init = false;
  // 收集实根（去重）
  //std::set<long long> seen_hash; // 用哈希避免重复（按一定精度）
  //const double HASH_SCALE = 1e12; // 用于哈希/去重（可调整）
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
      //long long h = (long long)std::llround(xr * HASH_SCALE);
      //if (seen_hash.find(h) == seen_hash.end()) {
        //seen_hash.insert(h);
        rr->push_back(xr);
      //}
    }
  }
  init = true;

  // 排序输出（从小到大）
  //std::sort(rr->begin(), rr->end());
  return ret;
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

double F(double x1, double y1, double x2, double y2, double theta) {
	double a = compute_trial_a(x1, y1, theta);
	return f2(x2, y2, a, theta);
}

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
	
	double t, a;
	// printf("v1: [%lf, %lf]\n", x1, y1);
	// printf("v2: [%lf, %lf]\n", x2, y2);
	// printf("t range[%lf, %lf]\n", glm::degrees(t_low), glm::degrees(t_high));

	t = solve_with_bisect(x1, y1, x2, y2, t_low, t_high);
	t = solve_with_secant(x1, y1, x2, y2, t_low, t_high);
	t = solve_with_newton_method(x1, y1, x2, y2, t_low, t_high);
	//t = solve_with_fixed_point_theorem(x1, y1, x2, y2, t_low, t_high);

	double theta1 = t;
	double a1 = compute_trial_a(x1, y1, t);
	double c = std::cos(theta1);
	// double s = std::sin(theta1);
	double residual = f2(x2, y2, a1, theta1);
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
#if 0
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
	compute_other_roots_for_equ3(&roots, k1, k2, k3, root1);
	//std::sort(roots.begin(), roots.end());
#endif

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
