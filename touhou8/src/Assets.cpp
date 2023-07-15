#include "Assets.h"

#include "Game.h"

#include "utils.h"
#include "external/stb_sprintf.h"
#include <lz4.h>

#define ASSETS_FOLDER "Assets/"

// https://studios.ptilouk.net/superfluous-returnz/blog/2022-09-28_compression.html

static SDL_Surface* IMG_LoadLZ4_RW (SDL_RWops* src, int freesrc)
{
	Uint16 width;
	Uint16 height;
	Uint32 surface_format;
	int compressed_size;

	SDL_RWread (src, &width, sizeof(width), 1);
	SDL_RWread (src, &height, sizeof(height), 1);
	SDL_RWread (src, &surface_format, sizeof(surface_format), 1);
	SDL_RWread (src, &compressed_size, sizeof(compressed_size), 1);

	SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat (0, width, height, 32, surface_format);
	Uint8 bpp = out->format->BytesPerPixel;
	Uint32 uncompressed_size = width * height * (Uint32)bpp;

	char* compressed_buffer = (char*)malloc (compressed_size);
	SDL_RWread (src, compressed_buffer, 1, compressed_size);
	char* uncompressed_buffer = (char*)(out->pixels);
	LZ4_decompress_safe (compressed_buffer, uncompressed_buffer, compressed_size, uncompressed_size);
	free (compressed_buffer);

	if (freesrc)
		SDL_RWclose (src);

	return out;
}

static SDL_Surface* IMG_LoadLZ4 (const char* file)
{
	SDL_RWops* src = SDL_RWFromFile (file, "rb");
	return (src ? IMG_LoadLZ4_RW (src, 1) : NULL);
}

// for error message
static void* SDL_LoadFile2(const char* fname, size_t* size) {
	SDL_RWops* src = SDL_RWFromFile(fname, "rb");
	if (!src) {
		*size = 0;
		return nullptr;
	}

	void* result = SDL_LoadFile_RW(src, size, 1);

	return result;
}

namespace th {

	Assets* Assets::_instance = nullptr;

	static SDL_Texture* IMG_LoadLZ4Texture(const char* fname) {
		auto& game = Game::GetInstance();
		SDL_Renderer* renderer = game.renderer;

		SDL_Surface* surface = IMG_LoadLZ4(fname);

		if (!surface) {
			return nullptr;
		}

		SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);

		return result;
	}

	template <typename F>
	static bool ReadTextFileByLine(const char* fname, const F& f) {
		size_t filesize;
		char* filedata = (char*) SDL_LoadFile2(fname, &filesize);

		if (!filedata) {
			return false;
		}

		std::string_view text(filedata, filesize);

		size_t off = 0;
		size_t pos = text.find('\n', off);
		while (pos != std::string::npos) {
			std::string_view line(filedata + off, pos - off);

			if (!line.empty()) {
				if (line[0] != ';') {
					f(line);
				}
			}

			off = pos + 1;
			pos = text.find('\n', off);
		}

		std::string_view last_line(filedata + off, filesize - off);
		if (!last_line.empty()) {
			if (last_line[0] != ';') {
				f(last_line);
			}
		}

		SDL_free(filedata);

		return true;
	}

	bool Assets::LoadAssets() {
		bool result = true;

		double t = GetTime();
		ReadTextFileByLine(ASSETS_FOLDER "All.textures", [this, &result](std::string_view line) {
			if (!LoadTexture(line)) {
				result = false;
			}
		});
		LOG("Loading textures took %fms", (GetTime() - t) * 1000.0);

		t = GetTime();
		ReadTextFileByLine(ASSETS_FOLDER "All.sprites", [this, &result](std::string_view line) {
			size_t cursor = 0;
			std::string_view name = ReadWord(line, &cursor);

			std::string name_str(name);
			
			if (sprites.find(name_str) != sprites.end()) {
				LOG("DUPLICATE SPRITE \"%s\"", name_str.c_str());
				return;
			}

			std::string_view texture_name = ReadWord(line, &cursor);

			SDL_Texture* texture;

			if (!(texture = LoadTexture(texture_name))) {
				result = false;
				return;
			}

			Sprite* sprite = new Sprite{};
			sprite->texture = texture;

			sprite->u             = StrToInt(ReadWord(line, &cursor));
			sprite->v             = StrToInt(ReadWord(line, &cursor));
			sprite->width         = StrToInt(ReadWord(line, &cursor));
			sprite->height        = StrToInt(ReadWord(line, &cursor));
			sprite->xorigin       = StrToInt(ReadWord(line, &cursor));
			sprite->yorigin       = StrToInt(ReadWord(line, &cursor));
			sprite->frame_count   = StrToInt(ReadWord(line, &cursor));
			sprite->frames_in_row = StrToInt(ReadWord(line, &cursor));
			sprite->anim_spd      = StrToFloat(ReadWord(line, &cursor));
			sprite->loop_frame    = StrToInt(ReadWord(line, &cursor));
			sprite->border        = StrToInt(ReadWord(line, &cursor));

			sprite->frame_count   = (sprite->frame_count   > 0) ? sprite->frame_count   : 1;
			sprite->frames_in_row = (sprite->frames_in_row > 0) ? sprite->frames_in_row : sprite->frame_count;

			sprites.emplace(name_str, sprite);
		});
		LOG("Loading sprites took %fms", (GetTime() - t) * 1000.0);

		t = GetTime();
		ReadTextFileByLine(ASSETS_FOLDER "All.fonts", [this, &result](std::string_view line) {
			size_t cursor = 0;
			std::string_view name = ReadWord(line, &cursor);

			if (!LoadFont(name)) {
				result = false;
			}
		});
		LOG("Loading fonts took %fms", (GetTime() - t) * 1000.0);

		t = GetTime();
		ReadTextFileByLine(ASSETS_FOLDER "All.scripts", [this, &result](std::string_view line) {
			std::string name(line);

			if (scripts.find(name) != scripts.end()) {
				LOG("DUPLICATE SCRIPT \"%s\"", name.c_str());
				return;
			}

			char path[64];
			stb_snprintf(path, sizeof(path), ASSETS_FOLDER "%s", name.c_str());

			Script script{};
			script.data = (char*) SDL_LoadFile2(path, &script.size);

			if (!script.data) {
				result = false;
				return;
			}

			scripts.emplace(name, script);
		});
		LOG("Loading scripts took %fms", (GetTime() - t) * 1000.0);

		{
			auto& game = Game::GetInstance();
			SDL_Renderer* renderer = game.renderer;

			uint32_t pixels[16 * 16];
			SDL_memset4(pixels, 0, ArrayLength(pixels));
			stub_tex_black = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 16, 16);
			SDL_UpdateTexture(stub_tex_black, nullptr, pixels, 16 * sizeof(*pixels));

			SDL_memset4(pixels, 0xFFFFFFFFu, ArrayLength(pixels));
			stub_tex_white = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 16, 16);
			SDL_UpdateTexture(stub_tex_white, nullptr, pixels, 16 * sizeof(*pixels));

			stub_sprite = new Sprite{
				stub_tex_white,
				0, 0,
				16, 16,
				8, 8,
				1, 1
			};

			stub_font = new Font{
				stub_tex_white,
				8,
				8,
				8,
				0,
				8,
				"unknown",
				"unknown"
			};

			stub_font->glyphs[0].advance = 8;

			for (uint8_t ch = 33; ch <= 126; ch++) {
				stub_font->glyphs[ch - 32].advance = 8;
				stub_font->glyphs[ch - 32].src = {0, 0, 7, 7};
			}
		}

		return result;
	}

	void Assets::UnloadAssets() {
		delete stub_font;
		stub_font = nullptr;

		delete stub_sprite;
		stub_sprite = nullptr;

		SDL_DestroyTexture(stub_tex_white);
		stub_tex_white = nullptr;

		SDL_DestroyTexture(stub_tex_black);
		stub_tex_black = nullptr;

		for (auto it = scripts.begin(); it != scripts.end(); ++it) {
			SDL_free(it->second.data);
			it->second.data = nullptr;
			it->second.size = 0;
		}
		scripts.clear();

		for (auto it = fonts.begin(); it != fonts.end(); ++it) {
			delete it->second;
			it->second = nullptr;
		}
		fonts.clear();

		for (auto it = sprites.begin(); it != sprites.end(); ++it) {
			delete it->second;
			it->second = nullptr;
		}
		sprites.clear();

		for (auto it = textures.begin(); it != textures.end(); ++it) {
			if (it->second) {
				SDL_DestroyTexture(it->second);
				it->second = nullptr;
			}
		}
		textures.clear();
	}

	SDL_Texture* Assets::FindTexture(const std::string& name) {
		auto lookup = textures.find(name);
		if (lookup != textures.end()) {
			if (lookup->second) {
				return lookup->second;
			}
		}
		return stub_tex_black;
		//return nullptr;
	}

	Sprite* Assets::FindSprite(const std::string& name) {
		auto lookup = sprites.find(name);
		if (lookup != sprites.end()) {
			return lookup->second;
		}
		return stub_sprite;
	}

	Font* Assets::FindFont(const std::string& name) {
		auto lookup = fonts.find(name);
		if (lookup != fonts.end()) {
			return lookup->second;
		}
		return stub_font;
	}

	SDL_Texture* Assets::LoadTexture(std::string_view name) {
		std::string name_str(name);

		auto lookup = textures.find(name_str);
		if (lookup != textures.end()) {
			return lookup->second;
		}

		char fullPath[64];
		stb_snprintf(fullPath, sizeof(fullPath), ASSETS_FOLDER "%.*s" ".lz4.hc", (int)name.size(), name.data());

		SDL_Texture* texture;

		if (!(texture = IMG_LoadLZ4Texture(fullPath))) {
			textures.emplace(name_str, nullptr);
			return nullptr;
		}

		textures.emplace(name_str, texture);

		return texture;
	}

	Font* Assets::LoadFont(std::string_view name) {
		std::string name_str(name);

		auto lookup = fonts.find(name_str);
		if (lookup != fonts.end()) {
			LOG("DUPLICATE FONT \"%s\"", name_str.c_str());
			return lookup->second;
		}

		SDL_Texture* texture;

		if (!(texture = LoadTexture(name))) {
			return nullptr;
		}

		Font* font = new Font{};
		font->texture = texture;

		char glyphs_path[64];
		stb_snprintf(glyphs_path, sizeof(glyphs_path), ASSETS_FOLDER "%.*s" ".glyphs", (int)name.size(), name.data());

		SDL_RWops* src = SDL_RWFromFile(glyphs_path, "rb");
		if (!src) {
			return nullptr;
		}

		SDL_RWread(src, &font->ptsize,   sizeof(font->ptsize),   1);
		SDL_RWread(src, &font->height,   sizeof(font->height),   1);
		SDL_RWread(src, &font->ascent,   sizeof(font->ascent),   1);
		SDL_RWread(src, &font->descent,  sizeof(font->descent),  1);
		SDL_RWread(src, &font->lineskip, sizeof(font->lineskip), 1);
		SDL_RWread(src, font->name,       1, sizeof(font->name));
		SDL_RWread(src, font->style_name, 1, sizeof(font->style_name));

		for (uint8_t ch = 32; ch <= 126; ch++) {
			GlyphData* glyph = &font->glyphs[ch - 32];

			SDL_RWread(src, &glyph->src.x,   sizeof(glyph->src.x),   1);
			SDL_RWread(src, &glyph->src.y,   sizeof(glyph->src.y),   1);
			SDL_RWread(src, &glyph->src.w,   sizeof(glyph->src.w),   1);
			SDL_RWread(src, &glyph->src.h,   sizeof(glyph->src.h),   1);
			SDL_RWread(src, &glyph->xoffset, sizeof(glyph->xoffset), 1);
			SDL_RWread(src, &glyph->yoffset, sizeof(glyph->yoffset), 1);
			SDL_RWread(src, &glyph->advance, sizeof(glyph->advance), 1);
		}

		SDL_RWclose(src);

		fonts.emplace(name_str, font);

		return font;
	}

}
