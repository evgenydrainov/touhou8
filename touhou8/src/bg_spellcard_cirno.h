#pragma once

namespace th {

	static void cirno_draw_spellcard_background(float delta, float spellcard_bg_alpha) {
		auto& assets = Assets::GetInstance();
		auto& stage = Stage::GetInstance();
		auto& game = Game::GetInstance();

		SDL_Renderer* renderer = game.renderer;
		float time = stage.time;

		SDL_Texture* texture = assets.FindTexture("CirnoSpellcardBG");
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
		SDL_SetTextureAlphaMod(texture, (uint8_t) (255.0f * spellcard_bg_alpha));

		{
			SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W - PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
			SDL_RenderCopy(renderer, texture, nullptr, &dest);
		}

		{
			SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
			SDL_RenderCopy(renderer, texture, nullptr, &dest);
		}

		{
			SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W + PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
			SDL_RenderCopy(renderer, texture, nullptr, &dest);
		}
	}

}
