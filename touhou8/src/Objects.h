#pragma once

#include "Sprite.h"

#include <lua.hpp>

// first byte is object type, then id
#define _TYPE_PART_SHIFT 28u
#define _ID_PART_MASK 0x0FFF'FFFFu

#define MAKE_INSTANCE_ID(id, type)    ((full_instance_id) (((instance_id_id) (id)) | ((type) << _TYPE_PART_SHIFT)))
#define INSTANCE_ID_GET_TYPE(full_id) ((object_type) ((full_id) >> _TYPE_PART_SHIFT))
#define INSTANCE_ID_GET_ID(full_id)   ((instance_id_id) ((full_id) & _ID_PART_MASK))

#define NULL_INSTANCE_ID ((full_instance_id)(-1))

namespace th {

	typedef uint32_t full_instance_id;
	typedef uint32_t instance_id_id; // don't know how to name this to not confuse with full_instance_id

	enum object_type : uint32_t {
		TYPE_PLAYER,
		TYPE_BOSS,
		TYPE_ENEMY,
		TYPE_BULLET,

		TYPE_COUNT
	};

	enum class PlayerState : uint8_t {
		Normal,
		Dying,
		Appearing
	};

	enum class BossState : uint8_t {
		Normal,
		WaitingStart,
		WaitingEnd
	};

	enum PlayerBulletType : uint8_t {
		PLAYER_BULLET_REIMU_CARD,
		PLAYER_BULLET_REIMU_ORB_SHOT
	};

	enum class ProjectileType : uint8_t {
		Bullet,
		Rect,
		Lazer,
		SLazer
	};

	enum PickupType : uint8_t {
		PICKUP_POWER,
		PICKUP_POINT,
		PICKUP_BIGP,
		PICKUP_BOMB,
		PICKUP_FULL_POWER,
		PICKUP_1UP,
		PICKUP_SCORE,
		PICKUP_CHERRY,

		PICKUP_COUNT
	};

	// flags

	enum ObjectFlags {
		OBJECT_FLAG_DEAD = 1
	};

	enum BulletFlags {
		BULLET_FLAG_ROTATE = 1 << 8
	};



	struct ReimuData {
		float fire_timer;
		int fire_queue;
		float orb_x[2];
		float orb_y[2];
	};

	struct MarisaData {

	};



	struct Object {
		full_instance_id full_id;
		uint32_t flags;

		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;

		Sprite* sprite;
		float frame_index;
		float xscale = 1.0f;
		float yscale = 1.0f;
		float angle;
		SDL_Color color{255, 255, 255, 255};
	};

	struct Player : Object {
		float hsp;
		float vsp;

		PlayerState state;
		bool is_focused;
		float iframes;
		float timer;
		float bomb_timer;
		float hitbox_alpha;
		float facing = 1.0f;

		union {
			ReimuData reimu;
			MarisaData marisa;
		};
	};

	struct Boss : Object {
		float hp;
		int boss_index;
		int phase_index;
		float timer;
		float wait_timer;
		BossState state;
		float facing = 1.0f;

		int coroutine = LUA_REFNIL;
	};

	struct PlayerBullet {
		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		Sprite* sprite;
		float frame_index;
		float dmg;
		PlayerBulletType type;
		union {
			float angle;
		};
	};

	struct Bullet : Object {
		ProjectileType type;
		union {
			struct {
				float lazer_length;
				float lazer_target_length;
				float lazer_thickness;
				float lazer_time;
				float lazer_timer;
			};
		};

		float lifetime;
		float lifespan = 60.0f * 60.0f;
		uint32_t grazed_by;

		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
	};

	struct Enemy : Object {
		float hp;
		int drops;

		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
		int death_callback = LUA_REFNIL;
	};

	struct Pickup {
		float x;
		float y;
		float hsp;
		float vsp;
		float radius;
		Sprite* sprite;
		float frame_index;
		PickupType type;
		full_instance_id homing_target = NULL_INSTANCE_ID;
	};

}
