#pragma once

#include <SDL.h>

namespace th {

	struct Sprite {
		SDL_Texture* texture;
		int u;
		int v;
		int width;
		int height;
		int xorigin;
		int yorigin;
		int frame_count;
		int frames_in_row;
		float anim_spd;
		int loop_frame;
		int border;
	};

	void DrawSprite(Sprite* sprite, int frame_index,
					float x, float y,
					float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f,
					SDL_Color color = {255, 255, 255, 255});

}
