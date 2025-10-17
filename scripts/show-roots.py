import matplotlib.pyplot as plt
import numpy as np
import re
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation, FFMpegWriter

# ========== 文件路径 ==========
filename = "roots.txt"

# ========== 读取数据 ==========
pattern = re.compile(r"([+-]?\d*\.?\d+)\s*\+\s*([+-]?\d*\.?\d+)\*i")
colors = ['r', 'y', 'b', 'g']

roots_time = []
roots_real = [[] for _ in range(4)]
roots_imag = [[] for _ in range(4)]

with open(filename, 'r', encoding='utf-8') as f:
    lines = [line.strip() for line in f if line.strip()]
    num_groups = len(lines) // 4
    for g in range(num_groups):
        roots_time.append(g)
        for i in range(4):
            line = lines[g * 4 + i]
            match = pattern.search(line)
            if match:
                re_part = float(match.group(1))
                im_part = float(match.group(2))
                if i == 3:
                    re_part += 0.05  # 第4根实部偏移 +0.05
                roots_real[i].append(re_part)
                roots_imag[i].append(im_part)

# ========== 创建3D绘图 ==========
fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection='3d')

ax.set_xlabel("Equation index (Time)", fontsize=12)
ax.set_ylabel("Real part", fontsize=12)
ax.set_zlabel("Imaginary part", fontsize=12)
ax.set_title("Animated 3D Scatter of Cubic Roots", fontsize=14)

# 设置坐标范围
ax.set_xlim(0, len(roots_time) - 1)
all_real = np.concatenate(roots_real)
all_imag = np.concatenate(roots_imag)
ax.set_ylim(all_real.min() - 0.1, all_real.max() + 0.1)
ax.set_zlim(all_imag.min() - 0.1, all_imag.max() + 0.1)

# 初始化散点对象
points = []
for i in range(4):
    if i == 3:
        p = ax.scatter([], [], [], c=colors[i], s=10, alpha=0.8, label=f"Solution")
    else:
        p = ax.scatter([], [], [], c=colors[i], s=10, alpha=0.8, label=f"Root {i+1}")
    points.append(p)
ax.legend()

# ========== 动画更新函数 ==========
def update(frame):
    # elev 动态变化：73 -> 50 -> 73 来回
    elev = 83 - 43 * np.abs(np.sin(frame / len(roots_time) * np.pi * 8))
    ax.view_init(elev=elev, azim=23, roll=114)

    for i in range(4):
        x = roots_time[:frame + 1]
        y = roots_real[i][:frame + 1]
        z = roots_imag[i][:frame + 1]
        points[i]._offsets3d = (x, y, z)
    return points

ani = FuncAnimation(fig, update, frames=len(roots_time), interval=100, blit=False, repeat=True)

if 0:
    plt.tight_layout()
    plt.show()
else:
    writer = FFMpegWriter(fps=15, metadata=dict(artist='ChatGPT'), bitrate=1800)
    ani.save("cubic_roots_animation.mp4", writer=writer)
    plt.close(fig)
