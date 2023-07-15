#include "GameData.h"

#include "Assets.h"

#include "cpml.h"
#include "utils.h"

#include "Game.h"
#include "shottype_reimu.h"
#include "shottype_marisa.h"

namespace th {

	static CharacterData character_data[CHARACTER_COUNT] = {
		{"Reimu Hakurei", 3.75f, 1.6f, 2.0f, 16.0f, 15.0f, 3, reimu_shot_type, reimu_bomb}
	};

	static BossData boss_data[] = {
		{
			"Cirno",
			5,
			nullptr,
			nullptr,
			nullptr,
			BOSS_BOSS,
			{
				{1500.0f, 25.0f * 60.0f, PHASE_NONSPELL},
				{1500.0f, 30.0f * 60.0f, PHASE_SPELLCARD, "Ice Sign \"Icicle Fall\""},
				{1500.0f, 50.0f * 60.0f, PHASE_NONSPELL},
				{1500.0f, 40.0f * 60.0f, PHASE_SPELLCARD, "Freeze Sign \"Perfect Freeze\""},
				{1500.0f, 33.0f * 60.0f, PHASE_SPELLCARD, "Snow Sign \"Diamond Blizzard\""}
			}
		}
	};



	void FillDataTables() {
		auto& assets = Assets::GetInstance();

		character_data[CHARACTER_REIMU].spr_idle = assets.FindSprite("reimu_idle");
		character_data[CHARACTER_REIMU].spr_move_right = assets.FindSprite("reimu_move_right");
		character_data[CHARACTER_REIMU].spr_move_left = assets.FindSprite("reimu_move_left");

		boss_data[0].spr_idle = assets.FindSprite("cirno_idle");
		boss_data[0].spr_move_right = assets.FindSprite("cirno_move_right");
		boss_data[0].spr_move_left = assets.FindSprite("cirno_move_left");
	}



	CharacterData* GetCharacterData(character_index index) {
		if (0 <= index && index < CHARACTER_COUNT) {
			return &character_data[index];
		}
		return &character_data[0];
	}

	PhaseData* GetPhaseData(BossData* boss, int phase_index) {
		if (!boss) {
			boss = &boss_data[0];
		}

		if (0 <= phase_index && phase_index < boss->phase_count) {
			return &boss->phase_data[phase_index];
		}
		return &boss->phase_data[0];
	}

	BossData* GetBossData(int boss_index) {
		if (0 <= boss_index && boss_index < ArrayLength(boss_data)) {
			return &boss_data[boss_index];
		}
		return &boss_data[0];
	}

}
