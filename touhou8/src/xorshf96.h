#pragma once

namespace th {

	class xorshf96 {
	public:
		xorshf96(unsigned long x=123456789, unsigned long y=362436069, unsigned long z=521288629)
			: x(x), y(y), z(z) {}

		void seed(unsigned long x=123456789, unsigned long y=362436069, unsigned long z=521288629) {
			this->x = x;
			this->y = y;
			this->z = z;
		}

		unsigned long operator()() {
			unsigned long t;

			x ^= x << 16;
			x ^= x >> 5;
			x ^= x << 1;

			t = x;
			x = y;
			y = z;

			z = t ^ x ^ y;
			return z;
		}

		float range(float a, float b) {
			float r = (float)(*this)() / (float)ULONG_MAX;
			float range = b - a;
			r = a + fmodf(range * r, range);
			return r;
		}

		int rangei(int a, int b) {
			int r = (*this)();
			int range = b - a;
			r = a + (((r % range) + range) % range);
			return r;
		}

	private:
		unsigned long x, y, z;
	};

}
