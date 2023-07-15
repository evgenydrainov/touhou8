#pragma once

#include "Stage.h"

#include <optional>

namespace th {

	struct Stats {
		int score;
		int lives;
		int bombs;
		int power;
		int graze;
		int points;
	};

	class GameScene {
	public:
		GameScene() { _instance = this; }

		static GameScene& GetInstance() { return *_instance; }

		void Init();
		void Quit();

		void Update(float delta);
		void Draw(float delta);

		void GetScore(size_t player_index, int score);
		void GetLives(size_t player_index, int lives);
		void GetBombs(size_t player_index, int bombs);
		void GetPower(size_t player_index, int power);
		void GetGraze(size_t player_index, int graze);
		void GetPoints(size_t player_index, int points);

		std::optional<Stage> stage;
		Stats stats[MAX_PLAYERS]{};
		bool paused = false;

	private:
		static GameScene* _instance;

		void ResetStats(size_t player_index);
	};

}
