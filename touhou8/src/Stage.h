#pragma once

#include "Objects.h"

#include "xorshf96.h"

#include <vector>

#define PLAY_AREA_W 384
#define PLAY_AREA_H 448

#define MAX_PLAYERS 4

#define MAX_POWER 128

namespace th {

	typedef uint32_t InputState;

	enum {
		INPUT_RIGHT = 1,
		INPUT_UP    = 1 << 1,
		INPUT_LEFT  = 1 << 2,
		INPUT_DOWN  = 1 << 3,
		INPUT_FIRE  = 1 << 4,
		INPUT_BOMB  = 1 << 5,
		INPUT_FOCUS = 1 << 6,

		INPUT_COUNT = 7
	};

	class Stage {
	public:
		Stage() { _instance = this; }

		static Stage& GetInstance() { return *_instance; }

		void Init();
		void Quit();

		void Update(float delta);
		void Draw(float delta);

		Player& ResetPlayer(size_t player_index, bool from_death);
		Boss& CreateBoss(int boss_index);
		Enemy& CreateEnemy();
		Bullet& CreateBullet();
		PlayerBullet& CreatePlayerBullet();
		Pickup& CreatePickup();

		void FreeBoss(Boss& boss);
		void FreeEnemy(Enemy& enemy);
		void FreeBullet(Bullet& bullet);

		Object* FindObject(full_instance_id full_id);

		void StartBossPhase(Boss& boss);
		bool EndBossPhase(Boss& boss);

		float time = 0.0f;
		xorshf96 random;
		lua_State* L = nullptr;
		int coroutine = LUA_REFNIL;

		InputState player_input[MAX_PLAYERS]{};
		Player players[MAX_PLAYERS]{};
		std::vector<Boss> bosses;
		std::vector<Enemy> enemies;
		std::vector<Bullet> bullets;
		std::vector<PlayerBullet> player_bullets;
		std::vector<Pickup> pickups;

	private:
		static Stage* _instance;

		void UpdatePlayer(size_t player_index, float delta);
		bool UpdateBoss(Boss& boss, float delta);

		void PhysicsUpdate(float delta);

		void InitLua();
		void CallCoroutines();

		full_instance_id GenFullInstanceID(object_type type) {
			instance_id_id id = next_instance_id++;
			return MAKE_INSTANCE_ID(id, type);
		}

		instance_id_id next_instance_id = 0;

		float coro_update_timer = 0.0f;
		float spellcard_bg_alpha = 0.0f;

		friend class Game; // to show next_instance_id
	};

}
