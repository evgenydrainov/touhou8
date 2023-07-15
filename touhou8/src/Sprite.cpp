#include "Sprite.h"

#include "Game.h"

#include "cpml.h"

namespace th {

	static double AngleToSDL(float angle) {
		return -(double)angle;
	}

	void DrawSprite(Sprite* sprite, int frame_index,
					float x, float y,
					float angle, float xscale, float yscale,
					SDL_Color color) {
		auto& game = Game::GetInstance();

		//if (!sprite->texture) return;

		frame_index = std::clamp(frame_index, 0, sprite->frame_count - 1);

		int cell_x = frame_index % sprite->frames_in_row;
		int cell_y = frame_index / sprite->frames_in_row;

		SDL_Rect src;
		src.x = sprite->u + cell_x * sprite->width;
		src.y = sprite->v + cell_y * sprite->height;
		src.w = sprite->width;
		src.h = sprite->height;

		SDL_Rect dest;
		dest.x = (int) (x - (float)sprite->xorigin * fabsf(xscale));
		dest.y = (int) (y - (float)sprite->yorigin * fabsf(yscale));
		dest.w = (int) ((float)sprite->width  * fabsf(xscale));
		dest.h = (int) ((float)sprite->height * fabsf(yscale));

		int flip = SDL_FLIP_NONE;
		if (xscale < 0.0f) flip |= SDL_FLIP_HORIZONTAL;
		if (yscale < 0.0f) flip |= SDL_FLIP_VERTICAL;

		SDL_Point center;
		center.x = (int) ((float)sprite->xorigin * fabsf(xscale));
		center.y = (int) ((float)sprite->yorigin * fabsf(yscale));

		SDL_SetTextureColorMod(sprite->texture, color.r, color.g, color.b);
		SDL_SetTextureAlphaMod(sprite->texture, color.a);
		SDL_RenderCopyEx(game.renderer, sprite->texture, &src, &dest, AngleToSDL(angle), &center, (SDL_RendererFlip) flip);
	}

}
