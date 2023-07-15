#include "Game.h"

#include "utils.h"
#include "external/stb_sprintf.h"

namespace th {

	Game* Game::_instance = nullptr;

	static void LogOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message) {
		auto& game = Game::GetInstance();

		const char* prefix[] = {
			nullptr,
			"ERROR: ",
			"ASSERT: ",
			"SYSTEM: ",
			"Audio: ",
			"Video: ",
			"Render: ",
			"Input: ",
			"TEST: "
		};

		if (0 <= category && category < ArrayLength(prefix)) {
			if (prefix[category]) {
				game.console_text += prefix[category];
				printf("%s", prefix[category]);
			}
		}

		game.console_text += message;
		printf("%s", message);

		if (category != SDL_LOG_CATEGORY_CUSTOM) {
			game.console_text += '\n';
			printf("\n");
		}

		size_t size = game.console_text.size();
		if (size > 4096) {
			game.console_text.erase(0, size - 4096);
		}
	}

	void Game::Init() {
		SDL_Init(SDL_INIT_AUDIO
				 | SDL_INIT_VIDEO
				 | SDL_INIT_GAMECONTROLLER);

		SDL_LogSetOutputFunction(LogOutputFunction, nullptr);
		SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

		LOG("Starting game...");

		SDL_SetHint(SDL_HINT_RENDER_DRIVER,                "opengl");
		SDL_SetHint(SDL_HINT_RENDER_BATCHING,              "1");
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1");

		window = SDL_CreateWindow("touhou8",
								  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
								  GAME_W, GAME_H,
								  SDL_WINDOW_RESIZABLE);

		renderer = SDL_CreateRenderer(window, -1,
									  SDL_RENDERER_ACCELERATED
									  | SDL_RENDERER_TARGETTEXTURE);

		SDL_RendererInfo info;
		SDL_GetRendererInfo(renderer, &info);
		if (SDL_strcmp(info.name, "opengl") == 0) {
			int major;
			int minor;
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
			LOG("OpenGL version: %d.%d", major, minor);
		}

		Mix_Init(MIX_INIT_MP3);
		Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048);
		//Mix_MasterVolume

		LOG("");
		double t = GetTime();
		assets.LoadAssets();
		LOG("Loading took %fms", (GetTime() - t) * 1000.0);
		LOG("");

		FillDataTables();

		LOG("type help to get a list of commands");
		LOG("");

		//SDL_DisplayMode mode;
		//int display_index = SDL_GetWindowDisplayIndex(window);
		//SDL_GetDesktopDisplayMode(display_index, &mode);
		//SDL_SetWindowDisplayMode(window, &mode);
		//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	}

	void Game::Quit() {
		switch (scene.index()) {
			case GAME_SCENE: {
				std::get<GAME_SCENE>(scene).Quit();
				break;
			}
			case TITLE_SCENE: {
				std::get<TITLE_SCENE>(scene).Quit();
				break;
			}
		}

		assets.UnloadAssets();

		Mix_Quit();

		SDL_DestroyRenderer(renderer);
		renderer = nullptr;

		SDL_DestroyWindow(window);
		window = nullptr;

		SDL_Quit();

		LOG("Game finished.");
	}

	void Game::Run() {
		double prev_time = GetTime();

		bool quit = false;
		while (!quit) {
			double frame_end_time = GetTime() + (1.0 / 60.0);

			skip_frame = frame_advance;
			memset(&key_pressed, 0, sizeof(key_pressed));

			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
				switch (ev.type) {
					case SDL_QUIT: {
						quit = true;
						break;
					}

					case SDL_KEYDOWN: {
						SDL_KeyboardEvent* key = &ev.key;
						int scancode = key->keysym.scancode;

						if (!console_on_screen) {
							if (0 <= scancode && scancode < sizeof(key_pressed)) {
								key_pressed[scancode] = true;
							}
						}

						switch (scancode) {
							case SDL_SCANCODE_GRAVE: {
								console_on_screen ^= true;
								break;
							}

							case SDL_SCANCODE_BACKSPACE: {
								if (console_on_screen) {
									if (console_caret != 0) {
										console_command.erase(console_caret - 1, 1);
										console_caret--;
									}
								}
								break;
							}

							case SDL_SCANCODE_LEFT: {
								if (console_on_screen) {
									if (console_caret != 0) console_caret--;
								}
								break;
							}

							case SDL_SCANCODE_RIGHT: {
								if (console_on_screen) {
									console_caret++;
									if (console_caret > console_command.size()) console_caret = console_command.size();
								}
								break;
							}

							case SDL_SCANCODE_UP: {
								if (console_on_screen) {
									console_command = console_prev_command;
									console_caret = console_command.size();
								}
								break;
							}

							case SDL_SCANCODE_DOWN: {
								if (console_on_screen) {
									console_command.clear();
									console_caret = 0;
								}
								break;
							}

							case SDL_SCANCODE_RETURN: {
								if (console_on_screen) {
									if (console_is_lua) {
										LOG("~%s", console_command.c_str());
									} else {
										LOG(">%s", console_command.c_str());
									}

									if (console_command == "lua") {
										console_is_lua = true;
									} else if (console_command == "exit") {
										console_is_lua = false;
									} else if (console_command == "clear") {
										console_text.clear();
									} else if (console_command == "help") {
										LOG("lua: enter lua console");
										LOG("exit: exit lua console");
										LOG("clear: clear the console");
										LOG("help: show this message");
										LOG("");
									} else {
										if (console_is_lua) {
											HandleLuaCommand();
										} else {
											HandleCommand();
										}
									}

									if (!console_command.empty()) console_prev_command = console_command;
									console_command.clear();
									console_caret = 0;
								}
								break;
							}

							case SDL_SCANCODE_F5: {
								frame_advance = true;
								skip_frame = false;
								break;
							}

							case SDL_SCANCODE_F6: {
								frame_advance = false;
								break;
							}
						}
						break;
					}

					case SDL_TEXTINPUT: {
						SDL_TextInputEvent* text = &ev.text;
						if (console_on_screen) {
							if (text->text[0] == '`') break;

							for (size_t i = 0; i < SDL_TEXTINPUTEVENT_TEXT_SIZE; i++) {
								char ch = text->text[i];

								if (ch == '\0') break;
								if (ch < 32 || ch > 126) break;

								console_command.insert(console_caret, 1, ch);
								console_caret++;
							}
						}
						break;
					}

					case SDL_MOUSEWHEEL: {
						SDL_MouseWheelEvent* wheel = &ev.wheel;
						if (console_on_screen) {
							console_scroll += wheel->y * 40;
						}
						break;
					}
				}
			}

			float delta = 1.0f;

			Update(delta);

			Draw(delta);

			double current_time = GetTime();
			frame_took = (current_time - prev_time) * 1000.0;
			fps = 1.0 / (current_time - prev_time);
			prev_time = current_time;

			SDL_RendererInfo info;
			SDL_GetRendererInfo(renderer, &info);
			if (!(info.flags & SDL_RENDERER_PRESENTVSYNC)) {
				double time_left = frame_end_time - current_time;

				if (time_left > 0.0) {
					double time_to_sleep = time_left * 0.95;
					SDL_Delay((unsigned int)(time_to_sleep * 1000.0));

					while (GetTime() < frame_end_time) {}
				}
			}
		}
	}

	void Game::Update(float delta) {
		double update_start_t = GetTime();
		everything_start_t = GetTime();

		if (next_scene != 0) {
			switch (scene.index()) {
				case GAME_SCENE: {
					std::get<GAME_SCENE>(scene).Quit();
					break;
				}
				case TITLE_SCENE: {
					std::get<TITLE_SCENE>(scene).Quit();
					break;
				}
			}

			SceneIndex scene_to_create = next_scene;
			next_scene = (SceneIndex) 0;

			random.seed();

			switch (scene_to_create) {
				case GAME_SCENE: {
					scene.emplace<GAME_SCENE>().Init();
					break;
				}
				case TITLE_SCENE: {
					scene.emplace<TITLE_SCENE>().Init();
					break;
				}
			}
		}

		if (!skip_frame || scene.index() == GAME_SCENE) {
			switch (scene.index()) {
				case GAME_SCENE: {
					std::get<GAME_SCENE>(scene).Update(delta);
					break;
				}
				case TITLE_SCENE: {
					std::get<TITLE_SCENE>(scene).Update(delta);
					break;
				}
			}
		}

		double update_end_t = GetTime();
		update_took = (update_end_t - update_start_t) * 1000.0;
	}

	void Game::Draw(float delta) {
		double draw_start_t = GetTime();

		SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		switch (scene.index()) {
			case GAME_SCENE: {
				std::get<GAME_SCENE>(scene).Draw(delta);
				break;
			}
			case TITLE_SCENE: {
				std::get<TITLE_SCENE>(scene).Draw(delta);
				break;
			}
		}

		{
			char buf[11];
			stb_snprintf(buf, sizeof(buf), "%7.2ffps", fps);
			Font* font = assets.FindFont("Mincho");
			DrawText(font, buf, 30 * 16, 29 * 16);
		}

		SDL_RenderSetLogicalSize(renderer, 0, 0);

		{
			Font* font = assets.FindFont("Mincho");
			char buf[100];
			stb_snprintf(buf, sizeof(buf),
						 "update: %fms\n"
						 "draw: %fms\n"
						 "all: %fms\n"
						 "frame: %fms\n"
						 "SDL allocs: %d\n\n",
						 update_took,
						 draw_took,
						 everything_took,
						 frame_took,
						 SDL_GetNumAllocations());
			int x = 0;
			int y = 0;
			SDL_Point pos = DrawText(font, buf, x, y);
			switch (scene.index()) {
				case GAME_SCENE: {
					auto& stage = Stage::GetInstance();
					char buf[200];
					stb_snprintf(buf, sizeof(buf),
								 "next id: %u\n"
								 "players: %zu\n"
								 "bosses: %zu\n"
								 "enemies: %zu\n"
								 "bullets: %zu\n"
								 "player bullets: %zu\n"
								 "pickups: %zu\n"
								 "lua top: %d\n"
								 "lua mem: %fKb\n",
								 stage.next_instance_id,
								 player_count,
								 stage.bosses.size(),
								 stage.enemies.size(),
								 stage.bullets.size(),
								 stage.player_bullets.size(),
								 stage.pickups.size(),
								 lua_gettop(stage.L),
								 (double)lua_gc(stage.L, LUA_GCCOUNT) + ((double)lua_gc(stage.L, LUA_GCCOUNTB) / 1024.0));
					DrawText(font, buf, pos.x, pos.y);
					break;
				}
			}
		}

		if (console_on_screen) {
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_RenderFillRect(renderer, nullptr);

			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

			Font* font = assets.FindFont("Mincho");
			const char* text = console_text.c_str();

			int window_w;
			int window_h;
			SDL_GetWindowSize(window, &window_w, &window_h);
			SDL_Point size = MeasureText(font, text);
			int x = 0;
			int y = window_h - size.y - font->lineskip + console_scroll;
			SDL_Point user_pos = DrawText(font, text, x, y);

			char user[64];
			char user_caret[64];
			if (console_is_lua) {
				stb_snprintf(user, sizeof(user), "~%s", console_command.c_str());
				stb_snprintf(user_caret, sizeof(user_caret), "~%.*s", console_caret, console_command.c_str());
			} else {
				stb_snprintf(user, sizeof(user), ">%s", console_command.c_str());
				stb_snprintf(user_caret, sizeof(user_caret), ">%.*s", console_caret, console_command.c_str());
			}
			DrawText(font, user, user_pos.x, user_pos.y);
			SDL_Point caret_pos;
			caret_pos.x = user_pos.x + MeasureText(font, user_caret).x;
			caret_pos.y = user_pos.y;

			SDL_Rect caret_rect{caret_pos.x, caret_pos.y, 2, font->height};
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(renderer, &caret_rect);
		}

		double draw_end_t = GetTime();
		draw_took = (draw_end_t - draw_start_t) * 1000.0;
		double everything_end_t = GetTime();
		everything_took = (everything_end_t - everything_start_t) * 1000.0;

		SDL_RenderPresent(renderer);
	}

	void Game::HandleCommand() {
		if (console_command.empty()) return;

		size_t cursor = 0;
		std::string_view command = ReadWord(console_command, &cursor);
	}

	void Game::HandleLuaCommand() {
		if (console_command.empty()) return;
		if (scene.index() != GAME_SCENE) return;

		Stage& stage = Stage::GetInstance();
		lua_State* L = stage.L;
		const char* str = console_command.c_str();

		if (luaL_loadstring(L, str) != LUA_OK) {
			const char* err = lua_tostring(L, -1);
			LOG("%s", err);
			lua_settop(L, 0);
			return;
		}

		if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
			const char* err = lua_tostring(L, -1);
			LOG("%s", err);
			lua_settop(L, 0);
			return;
		}

		int ret = lua_gettop(L);
		for (int i = 1; i <= ret; i++) {
			const char* s = luaL_tolstring(L, i, nullptr);
			lua_pop(L, 1);
			LOG("%s", s);
		}
		lua_settop(L, 0);
	}

}
