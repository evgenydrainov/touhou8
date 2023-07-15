#include "GameScene.h"

#include "Game.h"

#include "external/stb_sprintf.h"

#define PLAY_AREA_X 32
#define PLAY_AREA_Y 16

namespace th {

	GameScene* GameScene::_instance = nullptr;

	void GameScene::Init() {
		auto& game = Game::GetInstance();

		for (size_t player_index = 0; player_index < game.player_count; player_index++) {
			ResetStats(player_index);
		}

		stage.emplace();
		stage->Init();
	}

	void GameScene::Quit() {
		stage->Quit();
	}

	void GameScene::Update(float delta) {
		auto& game = Game::GetInstance();

		if (game.key_pressed[SDL_SCANCODE_ESCAPE]) {
			paused ^= true;
		}

		if (!game.skip_frame) {
			if (!paused) {
				stage->Update(delta);
			}
		}
	}

	void GameScene::Draw(float delta) {
		auto& game = Game::GetInstance();
		auto& assets = Assets::GetInstance();
		SDL_Renderer* renderer = game.renderer;

		{
			SDL_Texture* texture = assets.FindTexture("bg");
			SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		}

		{
			SDL_Rect old_view;
			SDL_RenderGetViewport(renderer, &old_view);

			SDL_Rect view{old_view.x + PLAY_AREA_X, old_view.y + PLAY_AREA_Y, PLAY_AREA_W, PLAY_AREA_H};
			SDL_RenderSetViewport(renderer, &view);

			stage->Draw(delta);

			if (paused) {
				SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
				SDL_RenderFillRect(renderer, nullptr);

				SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

				Font* font = assets.FindFont("Mincho");
				const char* text = "PAUSED";
				int x = (PLAY_AREA_W - MeasureText(font, text).x) / 2;
				int y = (PLAY_AREA_H - font->height) / 2;
				DrawText(font, text, x, y);
			}

			SDL_RenderSetViewport(renderer, &old_view);

			// stats
			{
				int x = PLAY_AREA_X + PLAY_AREA_W + 1 * 16;
				int y = PLAY_AREA_Y + 2 * 16;
				Font* font = assets.FindFont("Mincho");
				Stats& s = stats[0];

				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "HiScore %09d", 0);
					DrawText(font, buf, x, y);
					y += 16;
				}
				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Score   %09d", s.score);
					DrawText(font, buf, x, y);
					y += 2 * 16;
				}

				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Player %d", s.lives);
					DrawText(font, buf, x, y);
					y += 16;
				}
				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Bomb %d", s.bombs);
					DrawText(font, buf, x, y);
					y += 2 * 16;
				}

				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Power %d", s.power);
					DrawText(font, buf, x, y);
					y += 16;
				}
				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Graze %d", s.graze);
					DrawText(font, buf, x, y);
					y += 16;
				}
				{
					char buf[20];
					stb_snprintf(buf, sizeof(buf), "Point %d", s.points);
					DrawText(font, buf, x, y);
					y += 16;
				}
			}

			// bottom enemy label
			for (Boss& boss : stage->bosses) {
				Sprite* sprite = assets.FindSprite("enemy_label");
				float x = (float)PLAY_AREA_X + std::clamp(boss.x, (float)sprite->width / 2.0f, (float)PLAY_AREA_W - (float)sprite->width / 2.0f);
				float y = (float)PLAY_AREA_Y + (float)PLAY_AREA_H;
				DrawSprite(sprite, 0, x, y);
			}
		}
	}

	void GameScene::GetScore(size_t player_index, int score) {
		Stats& s = stats[player_index];
		s.score += score;
	}

	void GameScene::GetLives(size_t player_index, int lives) {
		Stats& s = stats[player_index];
		while (lives--) {
			if (s.lives < 8) {
				s.lives++;
				//PlaySound("se_extend.wav");
			} else {
				GetBombs(player_index, 1);
			}
		}
	}

	void GameScene::GetBombs(size_t player_index, int bombs) {
		Stats& s = stats[player_index];
		while (bombs--) {
			if (s.bombs < 8) {
				s.bombs++;
			}
		}
	}

	void GameScene::GetPower(size_t player_index, int power) {
		Stats& s = stats[player_index];
		while (power--) {
			if (s.power < MAX_POWER) {
				s.power++;
				switch (s.power) {
					case 8:
					case 16:
					case 32:
					case 48:
					case 64:
					case 80:
					case 96:
					case 128: {
						//PlaySound("se_powerup.wav");
						break;
					}
				}
			}
		}
	}

	void GameScene::GetGraze(size_t player_index, int graze) {
		Stats& s = stats[player_index];
		s.graze += graze;
	}

	void GameScene::GetPoints(size_t player_index, int points) {
		Stats& s = stats[player_index];
		while (points--) {
			s.points++;
			if (s.points >= 800) {
				if (s.points % 200 == 0) {
					GetLives(player_index, 1);
				}
			} else {
				switch (s.points) {
					case 50:
					case 125:
					case 200:
					case 300:
					case 450: {
						GetLives(player_index, 1);
						break;
					}
				}
			}
		}
	}

	void GameScene::ResetStats(size_t player_index) {
		auto& game = Game::GetInstance();
		Stats& s = stats[player_index];
		CharacterData* char_data = GetCharacterData(game.player_character[player_index]);

		s = {};
		s.lives = game.options.starting_lives;
		s.bombs = char_data->starting_bombs;
	}

}
