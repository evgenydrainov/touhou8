#pragma once

#include "Sprite.h"
#include "Font.h"

#include <SDL_mixer.h>

#include <unordered_map>
#include <string>
#include <string_view>

namespace th {

	struct Script {
		char* data;
		size_t size;
	};

	class Assets {
	public:
		Assets() { _instance = this; }

		static Assets& GetInstance() { return *_instance; }

		bool LoadAssets();
		void UnloadAssets();

		SDL_Texture* FindTexture(const std::string& name);
		Sprite* FindSprite(const std::string& name);
		Font* FindFont(const std::string& name);

		const auto& GetScripts() const { return scripts; }

		SDL_Texture* LoadTexture(std::string_view name);
		Font* LoadFont(std::string_view name);

	private:
		static Assets* _instance;

		std::unordered_map<std::string, SDL_Texture*> textures;
		std::unordered_map<std::string, Sprite*> sprites;
		std::unordered_map<std::string, Font*> fonts;
		std::unordered_map<std::string, Script> scripts;

		SDL_Texture* stub_tex_black = nullptr;
		SDL_Texture* stub_tex_white = nullptr;
		Sprite* stub_sprite = nullptr;
		Font* stub_font = nullptr;
	};

}
