#pragma once

#include "Assets.h"
#include "GameData.h"

#include "GameScene.h"
#include "TitleScene.h"

#include <variant>

#define GAME_W 640
#define GAME_H 480

namespace th {

	enum SceneIndex : size_t {
		GAME_SCENE = 1,
		TITLE_SCENE,

		LAST_SCENE
	};

	struct Options {
		int starting_lives = 2;
		int master_volume_level = 1;
	};

	class Game {
	public:
		Game() { _instance = this; }

		static Game& GetInstance() { return *_instance; }

		void Init();
		void Quit();
		void Run();

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;

		size_t player_count = 1;
		character_index player_character[MAX_PLAYERS]{};
		xorshf96 random;
		Options options{};

		std::variant<
			std::monostate,
			GameScene,
			TitleScene
		> scene;

		bool skip_frame = false;
		bool frame_advance = false;
		bool key_pressed[SDL_SCANCODE_UP + 1]{};

		bool console_on_screen = false;
		std::string console_text;
		std::string console_command;
		std::string console_prev_command;
		size_t console_caret = 0;
		int console_scroll = 0;
		bool console_is_lua = true;

	private:
		static Game* _instance;

		Assets assets;

		SceneIndex next_scene = GAME_SCENE;

		double fps = 0.0;
		double update_took = 0.0;
		double draw_took = 0.0;
		double everything_took = 0.0;
		double frame_took = 0.0;
		double everything_start_t = 0.0;

		void Update(float delta);
		void Draw(float delta);

		void HandleCommand();
		void HandleLuaCommand();
	};

}
