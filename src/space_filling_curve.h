#include <vector>

inline int next_power_of_two(int x, int* bits) {
	int xx = x - 1;
	*bits = 0;
	int v = 1;
	while (xx > 0) {
		(*bits)++;
		v <<= 1;
		xx >>= 1;
	}
	return v;
}

inline void z_order_to_2d(int z, int m_n, int* x, int* y) {
	*x = 0;
	*y = 0;

	for (int k = 0; k < m_n; ++k) {
		*x |= ((z >> (2 * k)) & 1) << k;
		*y |= ((z >> (2 * k + 1)) & 1) << k;
	}
}

inline void hilbert_inverse(int z, int order, int* x, int* y) {

	// Recursive base case: Order 0 curve (1¡Á1 grid)
	if (order == 0) {
		*x = 0;
		*y = 0;
		return;
	}

	// Current sub-region size (2^(order-1))
	int n = 1 << (order - 1); // n = grid_size / 2
	int sub_size = n * n;     // Number of points in each sub-region: 4^(order-1)

	// Calculate current quadrant (0-3) and index within sub-region
	int quadrant = z / sub_size;
	int sub_z = z % sub_size;

	// Recursively compute coordinates within the sub-region (x_sub, y_sub)
	int x_sub, y_sub;
	hilbert_inverse(sub_z, order - 1, &x_sub, &y_sub);

	// Apply coordinate transformation based on quadrant (core logic)
	switch (quadrant) {
	case 0:
		// Quadrant 0: Rotation (x,y) ¡û (y_sub, x_sub)
		*x = y_sub;
		*y = x_sub;
		break;
	case 1:
		// Quadrant 1: Translation along y-axis (x remains, y += n)
		*x = x_sub;
		*y = y_sub + n;
		break;
	case 2:
		// Quadrant 2: Translation along x and y axes (x += n, y += n)
		*x = x_sub + n;
		*y = y_sub + n;
		break;
	case 3:
		// Quadrant 3: Rotation + Translation (x = n-1 - y_sub + n; y = n-1 - x_sub)
		*x = (n - 1 - y_sub) + n;
		*y = n - 1 - x_sub;
		break;
	}
}


inline int sgn(int x) {
	if (x < 0) return -1;
	if (x > 0) return 1;
	return 0;
}

inline void generate2d(int x, int y, int ax, int ay, int bx, int by, std::vector<int>& coords, int width) {
	int w = std::abs(ax + ay);
	int h = std::abs(bx + by);

	int dax = sgn(ax);
	int day = sgn(ay);
	int dbx = sgn(bx);
	int dby = sgn(by);

	if (h == 1) {
		for (int i = 0; i < w; ++i) {
			coords.emplace_back(x + y * width);
			x += dax;
			y += day;
		}
		return;
	}

	if (w == 1) {
		for (int i = 0; i < h; ++i) {
			coords.emplace_back(x + y * width);
			x += dbx;
			y += dby;
		}
		return;
	}

	int ax2 = ax / 2;
	int ay2 = ay / 2;
	int bx2 = bx / 2;
	int by2 = by / 2;

	int w2 = std::abs(ax2 + ay2);
	int h2 = std::abs(bx2 + by2);

	if (2 * w > 3 * h) {
		if ((w2 % 2) && (w > 2)) {
			ax2 += dax;
			ay2 += day;
		}
		generate2d(x, y, ax2, ay2, bx, by, coords, width);
		generate2d(x + ax2, y + ay2, ax - ax2, ay - ay2, bx, by, coords, width);
	}
	else {
		if ((h2 % 2) && (h > 2)) {
			bx2 += dbx;
			by2 += dby;
		}
		generate2d(x, y, bx2, by2, ax2, ay2, coords, width);
		generate2d(x + bx2, y + by2, ax, ay, bx - bx2, by - by2, coords, width);
		generate2d(x + (ax - dax) + (bx2 - dbx), y + (ay - day) + (by2 - dby),
			-bx2, -by2, -(ax - ax2), -(ay - ay2), coords, width);
	}
}

inline void gilbert2d(int width, int height, std::vector<int>* coords) {
	if (width >= height) {
		generate2d(0, 0, width, 0, 0, height, *coords, width);
	}
	else {
		generate2d(0, 0, 0, height, width, 0, *coords, width);
	}
}
