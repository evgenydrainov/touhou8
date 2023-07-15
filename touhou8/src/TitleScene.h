#pragma once

#include <SDL.h>

namespace th {

	class TitleScene {
	public:
		void Init();
		void Quit();

		void Update(float delta);
		void Draw(float delta);

	private:
	};

}
