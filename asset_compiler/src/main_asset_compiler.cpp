



#define COMPILE_FONTS 0
#define COMPILE_SPRITES 1

#define LZ4_HC 1


//#include "Game.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <lz4.h>
#include <lz4hc.h>

#include <filesystem>
#include <sstream>
#include <fstream>

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

int IMG_SaveLZ4_RW (SDL_Surface* surface, SDL_RWops* dst, int freedst, int hc)
{
	Uint16 width = (Uint16)(surface->w);
	Uint16 height = (Uint16)(surface->h);
	Uint32 surface_format = surface->format->format;

	SDL_RWwrite (dst, &width, sizeof(width), 1);
	SDL_RWwrite (dst, &height, sizeof(height), 1);
	SDL_RWwrite (dst, &surface_format, sizeof(surface_format), 1);
	Uint8 bpp = surface->format->BytesPerPixel;
	Uint32 uncompressed_size = width * height * (Uint32)bpp;

	const char* uncompressed_buffer = (const char*)(surface->pixels);
	int max_lz4_size = LZ4_compressBound (uncompressed_size);
	char* compressed_buffer = (char*) malloc (max_lz4_size);
	int true_size = -1;

	if (hc)
		true_size = LZ4_compress_HC(uncompressed_buffer, compressed_buffer,
									uncompressed_size, max_lz4_size,
									LZ4HC_CLEVEL_MAX);
	else
		true_size = LZ4_compress_default (uncompressed_buffer, compressed_buffer,
										  uncompressed_size, max_lz4_size);

	SDL_RWwrite (dst, &true_size, sizeof(int), 1);
	SDL_RWwrite (dst, compressed_buffer, 1, true_size);

	free (compressed_buffer);

	if (freedst)
		SDL_RWclose (dst);

	return 0;
}

int IMG_SaveLZ4 (SDL_Surface* surface, const char* file, int hc)
{
	SDL_RWops* dst = SDL_RWFromFile (file, "wb");
	return (dst ? IMG_SaveLZ4_RW (surface, dst, 1, hc) : -1);
}






std::string assetsFolder = "Assets/";


template <typename F>
static bool ReadTextFile(const std::string& assetsFolder, const std::string& fname, const F& f) {
	std::ifstream file(assetsFolder + fname);

	if (!file) {
		//LOG("Couldn't open %s", fname.c_str());
		return false;
	}

	for (std::string line; std::getline(file, line);) {
		if (line.empty()) continue;

		if (line[0] == ';') continue;

		f(line);
	}

	return true;
}





int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	IMG_Init(IMG_INIT_PNG);

	TTF_Init();

	double t = GetTime();

	{
#if COMPILE_SPRITES
		for (const auto& dir_entry : std::filesystem::directory_iterator("../workspace")) {
			const auto& path = dir_entry.path();
			const auto& ext = path.extension();

			if (ext == ".png") {
				const auto& string = path.string();
				SDL_Surface* surface = IMG_Load(string.c_str());

				std::filesystem::path p = "Assets" / path.filename();
				p.replace_extension("lz4.hc");
				IMG_SaveLZ4(surface, p.string().c_str(), LZ4_HC);
			} else if (ext == ".ttf") {
				
			}
#endif
		}



		//ReadTextFile(assetsFolder, "All.textures", [](const std::string& line) {
		//	std::string png_path = "../workspace/" + line + ".png";
		//	SDL_Surface* surface = IMG_Load(png_path.c_str());
		//
		//	std::string lz4_path = assetsFolder + line + ".lz4.hc";
		//	IMG_SaveLZ4(surface, lz4_path.c_str(), LZ4_HC);
		//});



		//ReadTextFile(assetsFolder, "All.sprites", [](const std::string& line) {
		//	std::istringstream stream(line);
		//});


#if COMPILE_FONTS
		ReadTextFile(assetsFolder, "All.fonts", [](const std::string& line) {


			std::stringstream stream(line);

			std::string filename;
			int ptsize = 12;
			stream >> filename >> ptsize;


			const auto& ttf_path = "../workspace/" + filename + ".ttf";

			int atlas_width = 256;
			int atlas_height = 256;

			TTF_Font* font = TTF_OpenFont(ttf_path.c_str(), ptsize);
			SDL_Surface* atlas_surf = SDL_CreateRGBSurfaceWithFormat(0, atlas_width, atlas_height, 32, SDL_PIXELFORMAT_ARGB8888);

			int x = 0;
			int y = 0;

			std::string glyphs_file_path = "Assets/" + filename + ".glyphs";
			SDL_RWops* glyphs_file = SDL_RWFromFile(glyphs_file_path.c_str(), "wb");

			int font_height = TTF_FontHeight(font);
			int font_ascent = TTF_FontAscent(font);
			int font_descent = TTF_FontDescent(font);
			int font_lineskip = TTF_FontLineSkip(font);
			const char* name = TTF_FontFaceFamilyName(font);
			const char* style_name = TTF_FontFaceStyleName(font);

			{
				char _name[32]{};
				char _style_name[32]{};

				SDL_strlcpy(_name, name, sizeof _name);
				SDL_strlcpy(_style_name, style_name, sizeof _style_name);

				SDL_RWwrite(glyphs_file, &ptsize, sizeof(ptsize), 1);
				SDL_RWwrite(glyphs_file, &font_height, sizeof(font_height), 1);
				SDL_RWwrite(glyphs_file, &font_ascent, sizeof(font_ascent), 1);
				SDL_RWwrite(glyphs_file, &font_descent, sizeof(font_descent), 1);
				SDL_RWwrite(glyphs_file, &font_lineskip, sizeof(font_lineskip), 1);
				SDL_RWwrite(glyphs_file, _name, 1, sizeof _name);
				SDL_RWwrite(glyphs_file, _style_name, 1, sizeof _style_name);
			}

			{
				int minx;
				int maxx;
				int miny;
				int maxy;
				int advance;
				TTF_GlyphMetrics(font, ' ', &minx, &maxx, &miny, &maxy, &advance);

				int zero = 0;
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &zero, sizeof(zero), 1);
				SDL_RWwrite(glyphs_file, &advance, sizeof(advance), 1);
			}

			for (unsigned char ch = 33; ch <= 126; ch++) {
				SDL_Surface* glyph_surf = TTF_RenderGlyph_Blended(font, ch, {255, 255, 255, 255});

				//SDL_GetPixelFormatName(glyph_surf->format->format);

				int minx;
				int maxx;
				int miny;
				int maxy;
				int advance;
				TTF_GlyphMetrics(font, ch, &minx, &maxx, &miny, &maxy, &advance);

				if (x + maxx - minx > atlas_surf->w) {
					y += font_lineskip;
					x = 0;
				}

				SDL_Rect src{minx, font_ascent - maxy, maxx - minx, maxy - miny};
				SDL_Rect dest{x, y, maxx - minx, maxy - miny};
				SDL_BlitSurface(glyph_surf, &src, atlas_surf, &dest);

				int w = maxx - minx;
				int h = maxy - miny;
				int xoffset = minx;
				int yoffset = font_ascent - maxy;

				SDL_RWwrite(glyphs_file, &x, sizeof(x), 1);
				SDL_RWwrite(glyphs_file, &y, sizeof(y), 1);
				SDL_RWwrite(glyphs_file, &w, sizeof(w), 1);
				SDL_RWwrite(glyphs_file, &h, sizeof(h), 1);
				SDL_RWwrite(glyphs_file, &xoffset, sizeof(xoffset), 1);
				SDL_RWwrite(glyphs_file, &yoffset, sizeof(yoffset), 1);
				SDL_RWwrite(glyphs_file, &advance, sizeof(advance), 1);

				x += maxx - minx;

				SDL_FreeSurface(glyph_surf);
			}

			SDL_RWclose(glyphs_file);

			std::string atlas_path = "Assets/" + filename + ".lz4.hc";
			IMG_SaveLZ4(atlas_surf, atlas_path.c_str(), LZ4_HC);

			{
				std::string atlas_path = "../workspace/atlases/" + filename + "-atlas.png";
				IMG_SavePNG(atlas_surf, atlas_path.c_str());
			}



		});
#endif
	}

	printf("Took %fsec", (GetTime() - t));

	return 0;
}
