#include "Stage.h"

#include "Game.h"

#include "cpml.h"
#include "utils.h"
#include "external/stb_sprintf.h"

#define PLAYER_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define PLAYER_STARTING_Y 384.0f

#define PLAYER_DEATH_TIME      15.0f
#define PLAYER_APPEAR_TIME     30.0f
#define PLAYER_RESPAWN_IFRAMES 120.0f
#define PLAYER_BOMB_TIME       (2.5f * 60.0f)

#define BOSS_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define BOSS_STARTING_Y 96.0f

#define BOSS_PHASE_START_TIME 60.0f
#define BOSS_SPELL_END_TIME   60.0f

#define POINT_OF_COLLECTION 96.0f

#include "ScriptGlue.h"

#include "bg_spellcard_cirno.h"

namespace th {

	Stage* Stage::_instance = nullptr;

	void stage0_draw_background(float delta);

	static bool is_in_bounds(float x, float y, float off = 50.0f) {
		return (-off <= x) && (x < (float)PLAY_AREA_W + off)
			&& (-off <= y) && (y < (float)PLAY_AREA_H + off);
	}

	template <typename Object>
	static void MoveObject(Object& object, float delta) {
		object.x += cpml::lengthdir_x(object.spd, object.dir) * delta;
		object.y += cpml::lengthdir_y(object.spd, object.dir) * delta;
		object.spd += object.acc * delta;

		if (object.spd < 0.0f) {
			object.spd = 0.0f;
		}
	}

	template <typename Object>
	static void AnimateObject(Object& object, float delta) {
		int frame_count = object.sprite->frame_count;
		int loop_frame  = object.sprite->loop_frame;
		float anim_spd  = object.sprite->anim_spd;

		object.frame_index += anim_spd * delta;
		if ((int)object.frame_index >= frame_count) {
			object.frame_index = (float)loop_frame + fmodf(object.frame_index - (float)loop_frame, (float)(frame_count - loop_frame));
		}
	}

	template <typename Object>
	static Object* FindClosest(std::vector<Object>& storage, float x, float y) {
		float closest_dist = 1'000'000.0f;
		Object* result = nullptr;
		for (Object& object : storage) {
			float dist = cpml::point_distance(x, y, object.x, object.y);
			if (dist < closest_dist) {
				result = &object;
				closest_dist = dist;
			}
		}
		return result;
	}

	typedef ptrdiff_t ssize;

	template <typename T>
	static T* BinarySearch(std::vector<T>& storage, full_instance_id full_id) {
		ssize left = 0;
		ssize right = (ssize)storage.size() - 1;

		while (left <= right) {
			ssize middle = (left + right) / 2;
			if (storage[middle].full_id < full_id) {
				left = middle + 1;
			} else if (storage[middle].full_id > full_id) {
				right = middle - 1;
			} else {
				return &storage[middle];
			}
		}

		return nullptr;
	}

	static void LaunchTowardsPoint(Object& object, float target_x, float target_y, float acc) {
		acc = fabsf(acc);
		float dist = cpml::point_distance(object.x, object.y, target_x, target_y);
		object.spd = sqrtf(dist * acc * 2.0f);
		object.acc = -acc;
		object.dir = cpml::point_direction(object.x, object.y, target_x, target_y);
	}

	static bool PlayerVsBullet(Player& player, float player_radius, Bullet& bullet) {
		switch (bullet.type) {
			case ProjectileType::Bullet: {
				return cpml::circle_vs_circle(player.x, player.y, player_radius, bullet.x, bullet.y, bullet.radius);
			}
			case ProjectileType::Lazer:
			case ProjectileType::SLazer: {
				float rect_center_x = bullet.x + cpml::lengthdir_x(bullet.lazer_length / 2.0f, bullet.dir);
				float rect_center_y = bullet.y + cpml::lengthdir_y(bullet.lazer_length / 2.0f, bullet.dir);
				return cpml::circle_vs_rotated_rect(player.x, player.y, player_radius, rect_center_x, rect_center_y, bullet.lazer_thickness, bullet.lazer_length, bullet.dir);
			}
		}
		return false;
	}

	static void PlayerGetHit(Player& player) {
		player.state = PlayerState::Dying;
		player.timer = PLAYER_DEATH_TIME;
		//PlaySound("se_pichuun.wav");
	}

	static Pickup& DropPickup(float x, float y, PickupType type) {
		auto& stage = Stage::GetInstance();
		auto& assets = Assets::GetInstance();

		Pickup& result = stage.CreatePickup();
		result.x = x;
		result.y = y;
		result.vsp = -1.5f;
		result.radius = 8.0f;
		result.sprite = assets.FindSprite("pickup");
		result.frame_index = (float) type;
		result.type = type;

		return result;
	}



	void Stage::Init() {
		auto& game = Game::GetInstance();
		auto& assets = Assets::GetInstance();

		for (size_t player_index = 0; player_index < game.player_count; player_index++) {
			ResetPlayer(player_index, false);
		}

		InitLua();
	}

	void Stage::Quit() {
		for (Bullet& bullet : bullets) {
			FreeBullet(bullet);
		}
		bullets.clear();

		for (Enemy& enemy : enemies) {
			FreeEnemy(enemy);
		}
		enemies.clear();

		for (Boss& boss : bosses) {
			FreeBoss(boss);
		}
		bosses.clear();

		lua_close(L);
		L = nullptr;
	}

	void Stage::Update(float delta) {
		auto& game = Game::GetInstance();

		{
			const Uint8* key = SDL_GetKeyboardState(nullptr);

			player_input[0] = 0;

			if (!game.console_on_screen) {
				player_input[0] |= key[SDL_SCANCODE_RIGHT] * INPUT_RIGHT;
				player_input[0] |= key[SDL_SCANCODE_UP]    * INPUT_UP;
				player_input[0] |= key[SDL_SCANCODE_LEFT]  * INPUT_LEFT;
				player_input[0] |= key[SDL_SCANCODE_DOWN]  * INPUT_DOWN;

				player_input[0] |= key[SDL_SCANCODE_Z]      * INPUT_FIRE;
				player_input[0] |= key[SDL_SCANCODE_X]      * INPUT_BOMB;
				player_input[0] |= key[SDL_SCANCODE_LSHIFT] * INPUT_FOCUS;
			}
		}

		// Update
		{
			for (size_t player_index = 0; player_index < game.player_count; player_index++) {
				UpdatePlayer(player_index, delta);
			}

			for (auto boss_it = bosses.begin(); boss_it != bosses.end();) {
				Boss& boss = *boss_it;

				if (!UpdateBoss(boss, delta)) {
					boss_it = bosses.erase(boss_it);
					FreeBoss(boss);
					continue;
				}

				AnimateObject(boss, delta);

				++boss_it;
			}

			for (Bullet& bullet : bullets) {
				switch (bullet.type) {
					case ProjectileType::Lazer:
					case ProjectileType::SLazer: {
						if (bullet.lazer_timer < bullet.lazer_time) {
							bullet.lazer_timer += delta;

							if (bullet.type == ProjectileType::SLazer) {
								bullet.lazer_length = 0.0f;
								if (bullet.lazer_timer >= bullet.lazer_time) {
									//PlaySound("se_lazer.wav");
								}
							} else {
								bullet.lazer_length = cpml::lerp(0.0f, bullet.lazer_target_length, bullet.lazer_timer / bullet.lazer_time);
							}
						} else {
							bullet.lazer_length = bullet.lazer_target_length;
						}
						break;
					}
				}
			}

			for (PlayerBullet& player_bullet : player_bullets) {
				switch (player_bullet.type) {
					case PLAYER_BULLET_REIMU_CARD: {
						player_bullet.angle += 16.0f * delta;
						break;
					}
					case PLAYER_BULLET_REIMU_ORB_SHOT: {
						float target_x = 0.0f;
						float target_y = 0.0f;
						float target_dist = 1'000'000.0f;
						bool home = false;

						Enemy* enemy = FindClosest(enemies, player_bullet.x, player_bullet.y);
						if (enemy) {
							target_x = enemy->x;
							target_y = enemy->y;
							target_dist = cpml::point_distance(player_bullet.x, player_bullet.y, enemy->x, enemy->y);
							home = true;
						}

						Boss* boss = FindClosest(bosses, player_bullet.x, player_bullet.y);
						if (boss) {
							float dist = cpml::point_distance(player_bullet.x, player_bullet.y, boss->x, boss->y);
							if (dist < target_dist) {
								target_x = boss->x;
								target_y = boss->y;
								target_dist = dist;
								home = true;
							}
						}

						// @goofy
						if (home) {
							float hsp = cpml::lengthdir_x(player_bullet.spd, player_bullet.dir);
							float vsp = cpml::lengthdir_y(player_bullet.spd, player_bullet.dir);
							float dx = target_x - player_bullet.x;
							float dy = target_y - player_bullet.y;
							dx = std::clamp(dx, -12.0f, 12.0f);
							dy = std::clamp(dy, -12.0f, 12.0f);
							hsp = cpml::approach(hsp, dx, 1.5f * delta);
							vsp = cpml::approach(vsp, dy, 1.5f * delta);
							player_bullet.spd = cpml::point_distance(0.0f, 0.0f, hsp, vsp);
							player_bullet.dir = cpml::point_direction(0.0f, 0.0f, hsp, vsp);
						} else {
							if (player_bullet.spd < 10.0f) {
								player_bullet.spd += 1.0f * delta;
							}
						}
						break;
					}
				}
			}

			for (Pickup& pickup : pickups) {
				if (pickup.homing_target != NULL_INSTANCE_ID) {
					Object* target = FindObject(pickup.homing_target);
					if (!target) {
						pickup.homing_target = NULL_INSTANCE_ID;
						continue;
					}

					float spd = 8.0f;
					float dir = cpml::point_direction(pickup.x, pickup.y, target->x, target->y);
					pickup.hsp = cpml::lengthdir_x(spd, dir);
					pickup.vsp = cpml::lengthdir_y(spd, dir);

					if (INSTANCE_ID_GET_TYPE(pickup.homing_target) != TYPE_PLAYER) {
						continue;
					}

					Player* player = (Player*) target;
					if (player->state == PlayerState::Dying) {
						pickup.hsp = 0.0f;
						pickup.vsp = -1.5f;
						pickup.homing_target = NULL_INSTANCE_ID;
					}
				} else {
					pickup.hsp = 0.0f;
					pickup.vsp += 0.025f * delta;
					pickup.vsp = std::min(pickup.vsp, 2.0f);
				}
			}
		}

		// Physics
		{
			float physics_update_rate = 1.0f / 300.0f; // 300 fps
			float physics_timer = delta * /*g_stage->gameplay_delta*/1.0f;
			while (physics_timer > 0.0f) {
				float pdelta = std::min(physics_timer, physics_update_rate * 60.0f);
				PhysicsUpdate(pdelta);
				physics_timer -= pdelta;
			}
		}

		// Scripts
		{
			coro_update_timer += delta;
			while (coro_update_timer >= 1.0f) {
				CallCoroutines();
				coro_update_timer -= 1.0f;
			}

			for (size_t enemy_idx = 0; enemy_idx < enemies.size(); enemy_idx++) {
				Enemy& enemy = enemies[enemy_idx];
				CallLuaFunction(L, enemy.update_callback, enemy.full_id);
			}
		}

		// Cleanup
		{
			for (size_t player_index = 0; player_index < game.player_count; player_index++) {
				Player& player = players[player_index];

				if (player.flags & OBJECT_FLAG_DEAD) {
					PlayerGetHit(player);
					player.flags &= ~OBJECT_FLAG_DEAD;
				}
			}

			for (auto boss_it = bosses.begin(); boss_it != bosses.end();) {
				Boss& boss = *boss_it;

				if (boss.flags & OBJECT_FLAG_DEAD) {
					FreeBoss(boss);
					boss_it = bosses.erase(boss_it);
					continue;
				}

				++boss_it;
			}

			for (auto enemy_it = enemies.begin(); enemy_it != enemies.end();) {
				Enemy& enemy = *enemy_it;

				if (enemy.flags & OBJECT_FLAG_DEAD) {
					FreeEnemy(enemy);
					enemy_it = enemies.erase(enemy_it);
					continue;
				}

				++enemy_it;
			}

			for (auto bullet_it = bullets.begin(); bullet_it != bullets.end();) {
				Bullet& bullet = *bullet_it;

				if (bullet.flags & OBJECT_FLAG_DEAD) {
					FreeBullet(bullet);
					bullet_it = bullets.erase(bullet_it);
					continue;
				}

				++bullet_it;
			}
		}

		// Late Update
		{
			for (size_t player_index = 0; player_index < game.player_count; player_index++) {
				Player& player = players[player_index];

				player.x = std::clamp(player.x, 0.0f, (float) (PLAY_AREA_W - 1));
				player.y = std::clamp(player.y, 0.0f, (float) (PLAY_AREA_H - 1));

				switch (game.player_character[player_index]) {
					case CHARACTER_REIMU: {
						player.reimu.orb_x[0] = player.x - 24.0f;
						player.reimu.orb_y[0] = player.y;

						player.reimu.orb_x[1] = player.x + 24.0f;
						player.reimu.orb_y[1] = player.y;
						break;
					}
				}
			}

			for (auto enemy_it = enemies.begin(); enemy_it != enemies.end();) {
				Enemy& enemy = *enemy_it;

				if (!is_in_bounds(enemy.x, enemy.y)) {
					FreeEnemy(enemy);
					enemy_it = enemies.erase(enemy_it);
					continue;
				}

				++enemy_it;
			}

			for (auto bullet_it = bullets.begin(); bullet_it != bullets.end();) {
				Bullet& bullet = *bullet_it;

				if (bullet.lifetime >= bullet.lifespan) {
					FreeBullet(bullet);
					bullet_it = bullets.erase(bullet_it);
					continue;
				}

				if (!is_in_bounds(bullet.x, bullet.y)) {
					FreeBullet(bullet);
					bullet_it = bullets.erase(bullet_it);
					continue;
				}

				bullet.lifetime += delta;

				++bullet_it;
			}

			for (auto player_bullet_it = player_bullets.begin(); player_bullet_it != player_bullets.end();) {
				PlayerBullet& player_bullet = *player_bullet_it;

				if (!is_in_bounds(player_bullet.x, player_bullet.y)) {
					player_bullet_it = player_bullets.erase(player_bullet_it);
					continue;
				}

				++player_bullet_it;
			}

			for (auto pickup_it = pickups.begin(); pickup_it != pickups.end();) {
				Pickup& pickup = *pickup_it;

				if (!is_in_bounds(pickup.x, pickup.y)) {
					pickup_it = pickups.erase(pickup_it);
					continue;
				}

				++pickup_it;
			}
		}

		{
			bool spellcard_bg_on_screen = false;
			if (!bosses.empty()) {
				Boss& boss = bosses.front();

				BossData* boss_data = GetBossData(boss.boss_index);
				PhaseData* phase_data = GetPhaseData(boss_data, boss.phase_index);

				if (phase_data->type == PHASE_SPELLCARD) {
					if (boss.state != BossState::WaitingEnd) {
						spellcard_bg_on_screen = true;
					}
				}
			}

			if (spellcard_bg_on_screen) {
				spellcard_bg_alpha = cpml::approach(spellcard_bg_alpha,
													1.0f,
													1.0f / 30.0f * delta);
			} else {
				spellcard_bg_alpha = 0.0f;
			}
		}

		time += delta;
	}

	void Stage::UpdatePlayer(size_t player_index, float delta) {
		auto& game = Game::GetInstance();
		auto& scene = GameScene::GetInstance();

		Player& player = players[player_index];
		InputState input = player_input[player_index];
		CharacterData* char_data = GetCharacterData(game.player_character[player_index]);
		Stats& stats = scene.stats[player_index];

		player.hsp = 0.0f;
		player.vsp = 0.0f;
		player.is_focused = false;

		switch (player.state) {
			case PlayerState::Normal: {
				player.is_focused = (input & INPUT_FOCUS) > 0;

				float xmove = 0.0f;
				float ymove = 0.0f;

				if (input & INPUT_RIGHT) xmove += 1.0f;
				if (input & INPUT_UP)    ymove -= 1.0f;
				if (input & INPUT_LEFT)  xmove -= 1.0f;
				if (input & INPUT_DOWN)  ymove += 1.0f;

				float len = cpml::point_distance(0.0f, 0.0f, xmove, ymove);
				if (len != 0.0f) {
					xmove /= len;
					ymove /= len;
				}

				float spd = player.is_focused ? char_data->focus_spd : char_data->move_spd;
				player.hsp = xmove * spd;
				player.vsp = ymove * spd;

				if (char_data->shot_type) {
					(*char_data->shot_type)(player_index, delta);
				}

				if (input & INPUT_BOMB) {
					if (player.bomb_timer == 0.0f) {
						if (stats.bombs > 0) {
							if (char_data->bomb) {
								(*char_data->bomb)(player_index);
							}
							stats.bombs--;
							player.bomb_timer = PLAYER_BOMB_TIME;
						}
					}
				}

				player.iframes = std::max(player.iframes - delta, 0.0f);

				if (xmove > 0.0f) {
					player.facing = 1.0f;

					if (player.sprite == char_data->spr_idle) {
						player.sprite = char_data->spr_move_right;
						player.frame_index = 0.0f;
					} else if (player.sprite == char_data->spr_move_left) {
						player.sprite = char_data->spr_move_right;
					}

					AnimateObject(player, delta);
				} else if (xmove < 0.0f) {
					player.facing = -1.0f;

					if (player.sprite == char_data->spr_idle) {
						player.sprite = char_data->spr_move_left;
						player.frame_index = 0.0f;
					} else if (player.sprite == char_data->spr_move_right) {
						player.sprite = char_data->spr_move_left;
					}

					AnimateObject(player, delta);
				} else {
					if (player.sprite == char_data->spr_idle) {
						AnimateObject(player, delta);
					} else {
						if (player.frame_index > (float) (player.sprite->loop_frame - 1)) {
							player.frame_index = (float) (player.sprite->loop_frame - 1);
						}
						player.frame_index -= player.sprite->anim_spd * delta;
						if (player.frame_index < 0.0f) {
							player.sprite = char_data->spr_idle;
							player.frame_index = 0.0f;
						}
					}
				}

				if (player.y < POINT_OF_COLLECTION) {
					for (Pickup& pickup : pickups) {
						pickup.homing_target = player.full_id;
					}
				}
				break;
			}

			case PlayerState::Dying: {
				if (input & INPUT_BOMB) {
					if ((PLAYER_DEATH_TIME - player.timer) < char_data->deathbomb_time) {
						if (player.bomb_timer == 0.0f) {
							if (stats.bombs > 0) {
								if (char_data->bomb) {
									(*char_data->bomb)(player_index);
								}
								stats.bombs--;
								player.bomb_timer = PLAYER_BOMB_TIME;
								player.state = PlayerState::Normal;
								player.iframes = PLAYER_RESPAWN_IFRAMES;
							}
						}
					}
				}

				player.timer = std::max(player.timer - delta, 0.0f);
				if (player.timer == 0.0f) {
					if (stats.lives > 0) {
						stats.lives--;
				
						int drop = std::min(stats.power, 16);
						stats.power -= drop;
						drop = std::min(drop, 12);
						while (drop > 0) {
							PickupType type;
							if (drop >= 8) {
								drop -= 8;
								type = PICKUP_BIGP;
							} else {
								drop--;
								type = PICKUP_POWER;
							}
							float x = player.x + random.range(-50.0f, 50.0f);
							float y = player.y + random.range(-50.0f, 50.0f);
							DropPickup(x, y, type);
						}
					} else {
						//g_screen->GameOver();
						DropPickup(player.x, player.y, PICKUP_FULL_POWER);
					}
					ResetPlayer(player_index, true);
					return;
				}
				break;
			}

			case PlayerState::Appearing: {
				player.timer = std::max(player.timer - delta, 0.0f);
				if (player.timer == 0.0f) {
					player.state = PlayerState::Normal;
				}
				break;
			}
		}

		player.bomb_timer = std::max(player.bomb_timer - delta, 0.0f);

		if (player.is_focused) {
			player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 1.0f, 0.1f * delta);
		} else {
			player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 0.0f, 0.1f * delta);
		}
	}

	bool Stage::UpdateBoss(Boss& boss, float delta) {
		switch (boss.state) {
			case BossState::Normal: {
				boss.timer = std::max(boss.timer - delta, 0.0f);
				if (boss.timer == 0.0f) {
					if (!EndBossPhase(boss)) {
						return false;
					}
				}
				break;
			}
			case BossState::WaitingStart: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					char name[64];
					stb_snprintf(name, sizeof(name), "Boss%d_Phase%d", boss.boss_index, boss.phase_index);
					boss.state = BossState::Normal;
					lua_getglobal(L, name);
					boss.coroutine = CreateCoroutine(L, L, name);
				}
				break;
			}
			case BossState::WaitingEnd: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					boss.phase_index++;
					StartBossPhase(boss);
				}
				break;
			}
		}

		if (boss.spd > 0.01f) {
			if (90.0f <= boss.dir && boss.dir < 270.0f) {
				boss.facing = 1.0f;
			} else {
				boss.facing = -1.0f;
			}
		}

		auto& game = Game::GetInstance();
		if (game.key_pressed[SDL_SCANCODE_B]) {
			if (!EndBossPhase(boss)) return false;
			if (boss.state == BossState::WaitingEnd) {
				boss.wait_timer = 0.0f;
			}
		}

		return true;
	}

	void Stage::StartBossPhase(Boss& boss) {
		BossData* boss_data = GetBossData(boss.boss_index);
		PhaseData* phase_data = GetPhaseData(boss_data, boss.phase_index);

		boss.hp = phase_data->hp;
		boss.timer = phase_data->time;
		boss.wait_timer = BOSS_PHASE_START_TIME;
		boss.state = BossState::WaitingStart;

		if (boss.phase_index > 0) {
			LaunchTowardsPoint(boss, BOSS_STARTING_X, BOSS_STARTING_Y, 0.05f);
		}

		if (phase_data->type == PHASE_SPELLCARD) {
			//PlaySound("se_spellcard.wav");
		}
	}

	bool Stage::EndBossPhase(Boss& boss) {
		for (Bullet& bullet : bullets) {
			DropPickup(bullet.x, bullet.y, PICKUP_SCORE);
			FreeBullet(bullet);
		}
		bullets.clear();

		for (Pickup& pickup : pickups) {
			pickup.homing_target = MAKE_INSTANCE_ID(0, TYPE_PLAYER);
		}

		LuaUnref(&boss.coroutine, L);

		BossData* boss_data = GetBossData(boss.boss_index);
		PhaseData* phase_data = GetPhaseData(boss_data, boss.phase_index);

		if (phase_data->type == PHASE_SPELLCARD) {
			// drop some pickups
			for (int i = 0; i < 5; i++) {
				float x = boss.x + random.range(-50.0f, 50.0f);
				float y = boss.y + random.range(-50.0f, 50.0f);
				PickupType type = (i == 4) ? PICKUP_BIGP : PICKUP_POWER;
				DropPickup(x, y, type);
			}
		}

		if (boss.phase_index + 1 < boss_data->phase_count) {
			if (phase_data->type == PHASE_SPELLCARD) {
				boss.state = BossState::WaitingEnd;
				boss.wait_timer = BOSS_SPELL_END_TIME;
			} else {
				boss.phase_index++;
				StartBossPhase(boss);
			}
			//PlaySound("se_enemy_die.wav");
		} else {
			for (Pickup& pickup : pickups) {
				pickup.homing_target = MAKE_INSTANCE_ID(0, TYPE_PLAYER);
			}

			if (boss_data->type == BOSS_BOSS) {
				//PlaySound("se_boss_die.wav");
				//ScreenShake(6.0f, 120.0f);
			} else {
				//PlaySound("se_enemy_die.wav");
			}

			return false;
		}

		return true;
	}

	void Stage::PhysicsUpdate(float delta) {
		auto& game = Game::GetInstance();
		auto& scene = GameScene::GetInstance();

		// Move
		{
			for (size_t player_index = 0; player_index < game.player_count; player_index++) {
				Player& player = players[player_index];

				player.x += player.hsp * delta;
				player.y += player.vsp * delta;
			}

			for (Boss& boss : bosses) {
				MoveObject(boss, delta);
			}

			for (Enemy& enemy : enemies) {
				MoveObject(enemy, delta);
			}

			for (Bullet& bullet : bullets) {
				switch (bullet.type) {
					case ProjectileType::Lazer:
					case ProjectileType::SLazer: {
						if (bullet.lazer_timer >= bullet.lazer_time) {
							MoveObject(bullet, delta);
						}
						break;
					}
					default: {
						MoveObject(bullet, delta);
						break;
					}
				}
			}

			for (PlayerBullet& player_bullet : player_bullets) {
				MoveObject(player_bullet, delta);
			}

			for (Pickup& pickup : pickups) {
				pickup.x += pickup.hsp * delta;
				pickup.y += pickup.vsp * delta;
			}
		}

		// Collide
		{
			for (size_t player_index = 0; player_index < game.player_count; player_index++) {
				Player& player = players[player_index];
				CharacterData* char_data = GetCharacterData(game.player_character[player_index]);

				// player vs bullet
				for (auto bullet_it = bullets.begin(); bullet_it != bullets.end();) {
					Bullet& bullet = *bullet_it;
					if (PlayerVsBullet(player, char_data->graze_radius, bullet)) {
						if (player.state == PlayerState::Normal) {
							if (!(bullet.grazed_by & (1u << player_index))) {
								scene.GetGraze(player_index, 1);
								//PlaySound("se_graze.wav");
								bullet.grazed_by |= (1u << player_index);
							}
						}
					}

					if (PlayerVsBullet(player, player.radius, bullet)) {
						if (player.state == PlayerState::Normal) {
							if (player.iframes == 0.0f) {
								PlayerGetHit(player);
								bullet_it = bullets.erase(bullet_it);
								continue;
							}
						}
					}

					++bullet_it;
				}

				// player vs pickup
				for (auto pickup_it = pickups.begin(); pickup_it != pickups.end();) {
					Pickup& pickup = *pickup_it;
					if (cpml::circle_vs_circle(player.x, player.y, char_data->graze_radius, pickup.x, pickup.y, pickup.radius)) {
						if (player.state == PlayerState::Normal) {
							switch (pickup.type) {
								case PICKUP_POWER:
									scene.GetPower(player_index, 1);
									scene.GetScore(player_index, 10);
									break;
								case PICKUP_POINT:      scene.GetPoints(player_index, 1);        break;
								case PICKUP_BIGP:       scene.GetPower(player_index, 8);         break;
								case PICKUP_BOMB:       scene.GetBombs(player_index, 1);         break;
								case PICKUP_FULL_POWER: scene.GetPower(player_index, MAX_POWER); break;
								case PICKUP_1UP:        scene.GetLives(player_index, 1);         break;
								case PICKUP_SCORE:      scene.GetScore(player_index, 10);        break;
							}
							//PlaySound("se_item.wav");
							pickup_it = pickups.erase(pickup_it);
							continue;
						}
					}
					++pickup_it;
				}
			}

			for (auto boss_it = bosses.begin(); boss_it != bosses.end();) {
				Boss& boss = *boss_it;

				// boss vs bullet
				for (auto player_bullet_it = player_bullets.begin(); player_bullet_it != player_bullets.end();) {
					PlayerBullet& player_bullet = *player_bullet_it;

					if (cpml::circle_vs_circle(boss.x, boss.y, boss.radius, player_bullet.x, player_bullet.y, player_bullet.radius)) {
						if (boss.state == BossState::Normal) {
							boss.hp -= player_bullet.dmg;
							if (boss.hp <= 0.0f) {
								if (!EndBossPhase(boss)) {
									boss_it = bosses.erase(boss_it);
									goto l_boss_out;
								}
							}
						}

						//PlaySound("se_enemy_hit.wav");
						player_bullet_it = player_bullets.erase(player_bullet_it);
						continue;
					}

					++player_bullet_it;
				}

				++boss_it;

				l_boss_out:;
			}

			for (size_t enemy_idx = 0; enemy_idx < enemies.size();) {
				Enemy& enemy = enemies[enemy_idx];

				// enemy vs bullet
				for (auto player_bullet_it = player_bullets.begin(); player_bullet_it != player_bullets.end();) {
					PlayerBullet& player_bullet = *player_bullet_it;

					if (cpml::circle_vs_circle(enemy.x, enemy.y, enemy.radius, player_bullet.x, player_bullet.y, player_bullet.radius)) {
						enemy.hp -= player_bullet.dmg;
						player_bullet_it = player_bullets.erase(player_bullet_it);
						//PlaySound("se_enemy_hit.wav");
						if (enemy.hp <= 0.0f) {
							switch (enemy.drops) {
								case 1: {
									// power or point
									PickupType type = (random.range(0.0f, 1.0f) > 0.5f) ? PICKUP_POWER : PICKUP_POINT;
									DropPickup(enemy.x, enemy.y, type);
									break;
								}
								case 2: {
									// power or point at chance
									if (random.range(0.0f, 1.0f) > 0.5f) {
										PickupType type = (random.range(0.0f, 1.0f) > 0.5f) ? PICKUP_POWER : PICKUP_POINT;
										DropPickup(enemy.x, enemy.y, type);
									}
									break;
								}
							}

							CallLuaFunction(L, enemy.death_callback, enemy.full_id);

							//PlaySound("se_enemy_die.wav");
							FreeEnemy(enemy);
							enemies.erase(enemies.begin() + enemy_idx);
							goto l_enemy_out;
						}
						continue;
					}

					++player_bullet_it;
				}

				enemy_idx++;

				l_enemy_out:;
			}
		}
	}

	Player& Stage::ResetPlayer(size_t player_index, bool from_death) {
		auto& game = Game::GetInstance();

		Player& player = players[player_index];
		CharacterData* char_data = GetCharacterData(game.player_character[player_index]);

		player = {};
		instance_id_id id = (instance_id_id) player_index;
		player.full_id = MAKE_INSTANCE_ID(id, TYPE_PLAYER);
		player.x = PLAYER_STARTING_X;
		player.y = PLAYER_STARTING_Y;
		player.radius = char_data->radius;
		player.sprite = char_data->spr_idle;

		player.iframes = PLAYER_RESPAWN_IFRAMES;
		if (from_death) {
			player.state = PlayerState::Appearing;
			player.timer = PLAYER_APPEAR_TIME;
		}

		return player;
	}

	Boss& Stage::CreateBoss(int boss_index) {
		BossData* boss_data = GetBossData(boss_index);

		Boss& result = bosses.emplace_back();

		result.full_id = GenFullInstanceID(TYPE_BOSS);
		result.boss_index = boss_index;
		result.x = BOSS_STARTING_X;
		result.y = BOSS_STARTING_Y;
		result.radius = 25.0f;
		result.sprite = boss_data->spr_idle;

		StartBossPhase(result);

		LOG("Created boss %s", boss_data->name);

		return result;
	}

	Enemy& Stage::CreateEnemy() {
		Enemy& result = enemies.emplace_back();
		result.full_id = GenFullInstanceID(TYPE_ENEMY);
		return result;
	}

	Bullet& Stage::CreateBullet() {
		Bullet& result = bullets.emplace_back();
		result.full_id = GenFullInstanceID(TYPE_BULLET);
		return result;
	}

	PlayerBullet& Stage::CreatePlayerBullet() {
		PlayerBullet& result = player_bullets.emplace_back();
		return result;
	}

	Pickup& Stage::CreatePickup() {
		Pickup& result = pickups.emplace_back();
		return result;
	}

	void Stage::FreeBoss(Boss& boss) {
		LuaUnref(&boss.coroutine, L);
	}

	void Stage::FreeEnemy(Enemy& enemy) {
		LuaUnref(&enemy.coroutine, L);
		LuaUnref(&enemy.update_callback, L);
		LuaUnref(&enemy.death_callback, L);
	}

	void Stage::FreeBullet(Bullet& bullet) {
		LuaUnref(&bullet.coroutine, L);
		LuaUnref(&bullet.update_callback, L);
	}

	Object* Stage::FindObject(full_instance_id full_id) {
		object_type type = INSTANCE_ID_GET_TYPE(full_id);
		Object* result = nullptr;
		switch (type) {
			case TYPE_PLAYER: {
				instance_id_id id = INSTANCE_ID_GET_ID(full_id);
				size_t player_index = (size_t) id;
				auto& game = Game::GetInstance();
				if (player_index < game.player_count) {
					result = &players[player_index];
				}
				break;
			}
			case TYPE_BOSS: {
				result = BinarySearch(bosses, full_id);
				break;
			}
			case TYPE_ENEMY: {
				result = BinarySearch(enemies, full_id);
				break;
			}
			case TYPE_BULLET: {
				result = BinarySearch(bullets, full_id);
				break;
			}
		}
		return result;
	}

	// DRAWING

	static void DrawObject(Object& object) {
		DrawSprite(object.sprite, (int)object.frame_index,
				   object.x, object.y,
				   object.angle,
				   object.xscale, object.yscale,
				   object.color);
	}

	void Stage::Draw(float delta) {
		auto& game = Game::GetInstance();
		auto& assets = Assets::GetInstance();
		SDL_Renderer* renderer = game.renderer;

		// stage bg
		if (spellcard_bg_alpha < 1.0f) {
			stage0_draw_background(delta);
		}

		// spellcard bg
		if (spellcard_bg_alpha > 0.0f) {
			cirno_draw_spellcard_background(delta, spellcard_bg_alpha);
		}

		for (Enemy& enemy : enemies) {
			DrawObject(enemy);
		}

		for (Boss& boss : bosses) {
			DrawObject(boss);
		}

		for (size_t player_index = 0; player_index < game.player_count; player_index++) {
			Player& player = players[player_index];

			SDL_Color color = {255, 255, 255, 255};
			float xscale = 1.0f;
			float yscale = 1.0f;

			if (player.state == PlayerState::Dying || player.state == PlayerState::Appearing) {
				float f;
				if (player.state == PlayerState::Dying) {
					f = 1.0f - player.timer / PLAYER_DEATH_TIME;
				} else {
					f = player.timer / PLAYER_APPEAR_TIME;
				}

				xscale = cpml::lerp(1.0f, 0.25f, f);
				yscale = cpml::lerp(1.0f, 2.0f, f);
				color.a = (uint8_t) cpml::lerp(255.0f, 0.0f, f);
			} else {
				if (player.iframes > 0.0f) {
					if (((int)time / 4) % 2) {
						color = {128, 128, 128, 128};
					}
				}
			}

			DrawSprite(player.sprite, (int) player.frame_index,
					   player.x, player.y,
					   0.0f,
					   xscale, yscale,
					   color);

			if (player.hitbox_alpha > 0.0f) {
				Sprite* sprite = assets.FindSprite("player_hitbox");
				uint8_t a = (uint8_t) (255.0f * player.hitbox_alpha);
				float x = player.x;
				float y = player.y;
				DrawSprite(sprite, 0, x, y, -time, 1.0f, 1.0f, {255, 255, 255, a});
			}
		}

		for (Pickup& pickup : pickups) {
			DrawSprite(pickup.sprite, (int) pickup.frame_index,
					   pickup.x, pickup.y);
		}

		for (PlayerBullet& player_bullet : player_bullets) {
			SDL_Color color = {255, 255, 255, 80};
			switch (player_bullet.type) {
				case PLAYER_BULLET_REIMU_CARD: {
					DrawSprite(player_bullet.sprite, (int) player_bullet.frame_index,
							   player_bullet.x, player_bullet.y,
							   player_bullet.angle,
							   1.5f, 1.5f,
							   color);
					break;
				}
				case PLAYER_BULLET_REIMU_ORB_SHOT: {
					DrawSprite(player_bullet.sprite, (int) player_bullet.frame_index,
							   player_bullet.x, player_bullet.y,
							   player_bullet.dir,
							   1.5f, 1.5f,
							   color);
					break;
				}
			}
		}

		for (Bullet& bullet : bullets) {
			switch (bullet.type) {
				case ProjectileType::Lazer:
				case ProjectileType::SLazer: {
					float angle = bullet.dir + 90.0f;
					float xscale = (bullet.lazer_thickness + 2.0f) / 16.0f;
					float yscale = bullet.lazer_length / 16.0f;
					if (bullet.type == ProjectileType::SLazer) {
						if (bullet.lazer_timer < bullet.lazer_time) {
							xscale = 2.0f / 16.0f;
						}
					}
					DrawSprite(bullet.sprite, (int) bullet.frame_index,
							   bullet.x, bullet.y,
							   angle, xscale, yscale);
					break;
				}
				default: {
					float angle = 0.0f;
					if (bullet.flags & BULLET_FLAG_ROTATE) {
						angle = bullet.dir - 90.0f;
					}
					DrawSprite(bullet.sprite, (int) bullet.frame_index,
							   bullet.x, bullet.y,
							   angle);
					break;
				}
			}
		}

		// ui
		{
			// pickup labels
			for (Pickup& pickup : pickups) {
				if (pickup.y < 0.0f) {
					Sprite* sprite = pickup.sprite;
					int frame_index = pickup.type + PICKUP_COUNT;
					float x = pickup.x;
					float y = 8.0f;
					SDL_Color color{255, 255, 255, 192};
					DrawSprite(sprite, frame_index, x, y, 0.0f, 1.0f, 1.0f, color);
				}
			}

			if (!bosses.empty()) {
				Boss& boss = bosses.front();
				BossData* data = GetBossData(boss.boss_index);
				PhaseData* phase = GetPhaseData(data, boss.phase_index);
				Font* font = assets.FindFont("Mincho");

				// phases left
				{
					char buf[3];
					stb_snprintf(buf, sizeof(buf), "%2d", data->phase_count - boss.phase_index - 1);
					int x = 8;
					int y = 0;
					DrawText(font, buf, x, y);
				}

				// healthbar
				{
					int healthbar_x = 32 + 2;
					int healthbar_y = 6;
					int healthbar_w = PLAY_AREA_W - 64 - 4;
					int healthbar_h = 2;
					int reduced_w = (int) ((float)healthbar_w * (boss.hp / phase->hp));
					{
						SDL_Rect rect{healthbar_x, healthbar_y + 1, reduced_w, healthbar_h};
						SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
						SDL_RenderFillRect(renderer, &rect);
					}
					{
						SDL_Rect rect{healthbar_x, healthbar_y, reduced_w, healthbar_h};
						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
						SDL_RenderFillRect(renderer, &rect);
					}
				}

				// timer
				{
					char buf[3];
					stb_snprintf(buf, sizeof(buf), "%d", (int)(boss.timer / 60.0f));
					int x = PLAY_AREA_W - 2 * 15;
					int y = 0;
					if ((int)(boss.timer / 60.0f) < 10) x += 8;
					DrawText(font, buf, x, y);
				}

				// boss name
				{
					int x = 16;
					int y = 16;
					DrawText(font, data->name, x+1, y+1, {0, 0, 0, 255});
					DrawText(font, data->name, x, y, {255, 255, 255, 255});
				}

				// phase name
				if (phase->type == PHASE_SPELLCARD && boss.state != BossState::WaitingEnd) {
					int x = PLAY_AREA_W - 16 - MeasureText(font, phase->name).x;
					int y = 16;
					DrawText(font, phase->name, x+1, y+1, {0, 0, 0, 255});
					DrawText(font, phase->name, x, y, {255, 255, 255, 255});
				}
			}
		}
	}

}
