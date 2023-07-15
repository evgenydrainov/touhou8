#pragma once

#include <SDL.h>

namespace th {

	struct GlyphData {
		SDL_Rect src;
		int xoffset;
		int yoffset;
		int advance;
	};

	struct Font {
		SDL_Texture* texture;
		int ptsize;
		int height;
		int ascent;
		int descent;
		int lineskip;
		char name[32];
		char style_name[32];
		GlyphData glyphs[95]; // 32..126
	};

	SDL_Point DrawText(Font* font, const char* text,
					   int x, int y,
					   SDL_Color color = {255, 255, 255, 255});

	SDL_Point MeasureText(Font* font, const char* text);

}
