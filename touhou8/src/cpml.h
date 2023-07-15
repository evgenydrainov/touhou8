#pragma once

// Cirno's Perfect Math Library

#include <cmath>
#include <algorithm>

#define CPML_PI    3.141592653589793f
#define CPML_SQRT2 1.4142135623730951f

namespace cpml {

	inline float lerp(float a, float b, float f) {
		return a + (b - a) * f;
	}

	inline float sqr(float x) {
		return x * x;
	}

	inline int wrap(int a, int b) {
		return (a % b + b) % b;
	}

	inline float approach(float start, float end, float shift) {
		return start + std::clamp(end - start, -shift, shift);
	}

	inline float rad(float deg) {
		return deg * CPML_PI / 180.0f;
	}

	inline float deg(float rad) {
		return rad * 180.0f / CPML_PI;
	}

	inline bool circle_vs_circle(float x1, float y1, float r1, float x2, float y2, float r2) {
		return (sqr(x2 - x1) + sqr(y2 - y1)) < sqr(r1 + r2);
	}

	inline float dcos(float deg) {
		return cosf(rad(deg));
	}

	inline float dsin(float deg) {
		return sinf(rad(deg));
	}

	inline float point_direction(float x1, float y1, float x2, float y2) {
		return deg(atan2f(y1 - y2, x2 - x1));
	}

	inline float point_distance(float x1, float y1, float x2, float y2) {
		return sqrtf(sqr(x2 - x1) + sqr(y2 - y1));
	}

	inline float lengthdir_x(float len, float dir) {
		return len * dcos(dir);
	}

	inline float lengthdir_y(float len, float dir) {
		return len * -dsin(dir);
	}

	inline float angle_wrap(float deg) {
		deg = fmodf(deg, 360.0f);
		if (deg < 0.0f) {
			deg += 360.0f;
		}
		return deg;
	}

	inline float angle_difference(float dest, float src) {
		float res = dest - src;
		res = angle_wrap(res + 180.0f) - 180.0f;
		return res;
	}

	inline bool circle_vs_rotated_rect(float circle_x, float circle_y, float circle_radius, float rect_center_x, float rect_center_y, float rect_w, float rect_h, float rect_dir) {
		float dx = circle_x - rect_center_x;
		float dy = circle_y - rect_center_y;

		float x_rotated = rect_center_x - (dx * dsin(rect_dir)) - (dy * dcos(rect_dir));
		float y_rotated = rect_center_y + (dx * dcos(rect_dir)) - (dy * dsin(rect_dir));

		float x_closest = std::clamp(x_rotated, rect_center_x - rect_w / 2.0f, rect_center_x + rect_w / 2.0f);
		float y_closest = std::clamp(y_rotated, rect_center_y - rect_h / 2.0f, rect_center_y + rect_h / 2.0f);

		dx = x_closest - x_rotated;
		dy = y_closest - y_rotated;

		return (sqr(dx) + sqr(dy)) < sqr(circle_radius);
	}

}
