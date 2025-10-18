import cmath
import math
from typing import List

def solve3(k1: float, k2: float, k3: float) -> bool:
    PI = math.acos(-1.0)
    EPS_IMAG = 1e-5
    
    # 降阶: x = y - k1/3
    a_over_3 = k1 / 3.0
    p = k2 - k1 * a_over_3
    q = 2.0 * (a_over_3 ** 3) - a_over_3 * k2 + k3
    
    # Δ（可为正、零或负）
    Delta = (q**2) / 4.0 + (p**3) / 27.0
    Delta = complex(Delta, 0.0)
    
    # 复数 sqrt(Delta)
    sqrtD = cmath.sqrt(Delta)
    # if abs(Delta.imag) < 0.0:
    #     sqrtD = -sqrtD
    
    # A 和 B（B 未直接使用）
    A = -q / 2.0 + sqrtD
    
    # 取 A 的一个立方根 u0（使用主值）
    if abs(A.imag) < EPS_IMAG:
        A = complex(A.real, 0.0)
        if A.real >= 0.0:
            u0 = complex(A.real ** (1.0 / 3.0), 0.0)
        else:
            u0 = complex(-(-A.real) ** (1.0 / 3.0), 0.0)
    else:
        u0 = A ** (1.0 / 3.0)
    
    # 强制匹配 v0 使 u0 * v0 = -p/3
    v0 = (-p / 3.0) / u0
    
    # 三次单位根
    omega = [1.0, cmath.exp(2.0j * PI / 3.0), cmath.exp(-2.0j * PI / 3.0)]

    print(f"Delta = {Delta.real} + {Delta.imag}*i")
    print(f"sqrtD = {sqrtD.real} + {sqrtD.imag}*i")
    print(f"A = {A.real} + {A.imag}*i")
    print(f"u0 = {u0.real} + {u0.imag}*i")
    print(f"v0 = {v0.real} + {v0.imag}*i")
    #print(f"omega = {omega.real} + {omega.imag}*i")
    
    for k in range(3):
        # 计算 uk 和 vk
        if k == 0:
            uk = u0 * omega[0]
            vk = v0 * omega[0]
        elif k == 1:
            uk = u0 * omega[1]
            vk = v0 * omega[2]
        else:
            uk = u0 * omega[2]
            vk = v0 * omega[1]
        yk = uk + vk
        xk = yk - a_over_3
        print(f"uk = {uk.real} + {uk.imag}*i")
        print(f"vk = {vk.real} + {vk.imag}*i")
        
        print(f"### {xk.real} + {xk.imag}*i")
    print()
        
#solve3(-2.182718, 1.595942, -0.003696)
#solve3(-2.191936, 1.601043, -0.002488)

solve3(-2.182689, 1.559025, -0.365487)
solve3(-2.132321, 1.488034, -0.341252)
solve3(-2.075609, 1.406656, -0.312247)
