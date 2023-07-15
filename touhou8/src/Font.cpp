#include "Font.h"

#include "Game.h"

namespace th {

	SDL_Point DrawText(Font* font, const char* text,
					   int x, int y,
					   SDL_Color color) {
		//if (!font->texture) return {};

		int text_x = x;
		int text_y = y;

		SDL_SetTextureColorMod(font->texture, color.r, color.g, color.b);

		for (const char* it = text; *it; it++) {
			char ch = *it;
			if (ch == '\n') {
				text_x = x;
				text_y += font->lineskip;
				continue;
			}

			if (ch < 32 || ch > 126) {
				ch = '?';
			}

			GlyphData* glyph = &font->glyphs[ch - 32];

			if (ch != ' ') {
				SDL_Rect src = glyph->src;

				SDL_Rect dest;
				dest.x = text_x + glyph->xoffset;
				dest.y = text_y + glyph->yoffset;
				dest.w = src.w;
				dest.h = src.h;

				auto& game = Game::GetInstance();
				SDL_RenderCopy(game.renderer, font->texture, &src, &dest);
			}
			
			text_x += glyph->advance;
		}

		return {text_x, text_y};
	}

	SDL_Point MeasureText(Font* font, const char* text) {
		if (!font->texture) return {};

		int text_x = 0;
		int text_y = 0;

		int text_w = 0;
		int text_h = font->height;

		for (const char* it = text; *it; it++) {
			char ch = *it;
			if (ch == '\n') {
				text_x = 0;
				text_y += font->lineskip;
				text_h = std::max(text_h, text_y + font->height);
				continue;
			}

			if (ch < 32 || ch > 126) {
				ch = '?';
			}

			GlyphData* glyph = &font->glyphs[ch - 32];

			if (ch == ' ') {
				text_w = std::max(text_w, text_x + glyph->advance);
				text_h = std::max(text_h, text_y + font->height);
			} else {
				SDL_Rect src = glyph->src;

				SDL_Rect dest;
				dest.x = text_x + glyph->xoffset;
				dest.y = text_y + glyph->yoffset;
				dest.w = src.w;
				dest.h = src.h;

				text_w = std::max(text_w, dest.x + dest.w);
				text_h = std::max(text_h, text_y + font->height);
			}

			text_x += glyph->advance;
		}

		return {text_w, text_h};
	}

}
