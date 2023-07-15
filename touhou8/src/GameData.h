#pragma once

#include "Sprite.h"

namespace th {

	enum character_index {
		CHARACTER_REIMU,
		//CHARACTER_MARISA,

		CHARACTER_COUNT
	};

	enum phase_type {
		PHASE_NONSPELL,
		PHASE_SPELLCARD
	};

	enum boss_type {
		BOSS_MIDBOSS,
		BOSS_BOSS
	};

	struct CharacterData {
		char name[32];
		float move_spd;
		float focus_spd;
		float radius;
		float graze_radius;
		float deathbomb_time;
		int starting_bombs;
		void (*shot_type)(size_t player_index, float delta);
		void (*bomb)(size_t player_index);
		Sprite* spr_idle;
		Sprite* spr_move_right;
		Sprite* spr_move_left;
	};

	struct PhaseData {
		float hp;
		float time;
		phase_type type;
		char name[32];
	};

	struct BossData {
		char name[32];
		int phase_count;
		Sprite* spr_idle;
		Sprite* spr_move_right;
		Sprite* spr_move_left;
		boss_type type;
		PhaseData phase_data[25];
	};

	void FillDataTables();

	CharacterData* GetCharacterData(character_index index);
	PhaseData* GetPhaseData(BossData* boss, int phase_index);
	BossData* GetBossData(int boss_index);

}
