#pragma once

#define LUA_REG_SLOT_TIMER (void*)0

namespace th {

	static void LuaUnref(int* ref, lua_State* L) {
		if (*ref != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, *ref);
			*ref = LUA_REFNIL;
		}
	}

	static bool CallLuaFunction(lua_State* L, int ref, full_instance_id full_id) {
		if (ref == LUA_REFNIL) {
			return true;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		if (!lua_isfunction(L, -1)) {
			LOG("CallLuaFunction: not a function");
			lua_pop(L, 1);
			return false;
		}

		lua_pushinteger(L, full_id);
		int res = lua_pcall(L, 1, 0, 0);
		if (res != LUA_OK) {
			LOG("CallLuaFunction:\n%s", lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		return true;
	}

	static void UpdateCoroutine(lua_State* L, int* coroutine, full_instance_id full_id) {
		if (*coroutine == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, *coroutine);
		if (!lua_isthread(L, -1)) {
			LOG("UpdateCoroutine: not a thread");
			lua_settop(L, 0);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
			return;
		}

		lua_State* NL = lua_tothread(L, -1);
		lua_pop(L, 1);

		if (!lua_isyieldable(NL)) {
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
			return;
		}

		lua_pushinteger(NL, full_id);
		int nres;
		int res = lua_resume(NL, L, 1, &nres);
		if (res == LUA_OK) {
			lua_pop(NL, nres);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
		} else if (res != LUA_YIELD) {
			const char* err = lua_tostring(NL, -1);
			LOG("  %s", err);
			lua_settop(NL, 0);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
		}
	}

	static int CreateCoroutine(lua_State* L, lua_State* main_L, const char* debug_name = nullptr) {
		if (lua_gettop(L) < 1) {
			LOG("CreateCoroutine: stack is empty");
			return LUA_REFNIL;
		}

		if (!lua_isfunction(L, -1)) {
			const char* tpname = luaL_typename(L, -1);
			if (debug_name) {
				LOG("%s is %s", debug_name, tpname);
			} else {
				LOG("got %s", tpname);
			}
			lua_pop(L, 1);
			return LUA_REFNIL;
		}

		lua_State* NL = lua_newthread(main_L);
		int coroutine = luaL_ref(main_L, LUA_REGISTRYINDEX);
		lua_xmove(L, NL, 1);

		lua_pushnumber(NL, (lua_Number) 0.0);
		lua_rawsetp(NL, LUA_REGISTRYINDEX, LUA_REG_SLOT_TIMER);

		return coroutine;
	}

	static void _lua_register(lua_State* L, const char* name, lua_CFunction func) {
		lua_register(L, name, func);
	}

	static void lua_checkargc(lua_State* L, int min, int max) {
		int top = lua_gettop(L);
		if (top < min || top > max) {
			if (min == max) {
				luaL_error(L, "expected %d args got %d", min, top);
			} else if (top < min) {
				luaL_error(L, "too little args (at least %d)", min);
			} else {
				luaL_error(L, "too much args (max %d)", max);
			}
		}
	}



	template <typename T>
	static void LuaPush(lua_State* L, T value);

	template <>
	static void LuaPush<float>(lua_State* L, float value) { lua_pushnumber(L, value); }

	template <>
	static void LuaPush<void*>(lua_State* L, void* value) { lua_pushlightuserdata(L, value); }

	template <typename T>
	static T LuaGet(lua_State* L, int idx);

	template <>
	static float LuaGet<float>(lua_State* L, int idx) { return (float) luaL_checknumber(L, idx); }

	template <>
	static void* LuaGet<void*>(lua_State* L, int idx) { return lua_touserdata(L, idx); }

	template <
		typename T,
		T (*GetFromObj)(Object* object)
	> static int lua_GetObjectVar(lua_State* L) {
		Stage& stage = Stage::GetInstance();

		lua_checkargc(L, 1, 1);
		full_instance_id full_id = (full_instance_id) luaL_checkinteger(L, 1);
		T result{};
		Object* object = stage.FindObject(full_id);
		if (object) result = GetFromObj(object);
		LuaPush<T>(L, result);
		return 1;
	}

	template <
		typename T,
		void (*SetForObj)(Object* object, T value)
	> static int lua_SetObjectVar(lua_State* L) {
		Stage& stage = Stage::GetInstance();

		lua_checkargc(L, 2, 2);
		full_instance_id full_id = (full_instance_id) luaL_checkinteger(L, 1);
		T value = LuaGet<T>(L, 2);
		Object* object = stage.FindObject(full_id);
		if (object) SetForObj(object, value);
		return 0;
	}

	static float GetXFromObject(Object* object) { return object->x; }
	static float GetYFromObject(Object* object) { return object->y; }
	static float GetSpdFromObject(Object* object) { return object->spd; }
	static float GetDirFromObject(Object* object) { return object->dir; }
	static float GetAccFromObject(Object* object) { return object->acc; }

	static void* GetSprFromObject(Object* object) { return object->sprite; }
	static float GetImgFromObject(Object* object) { return object->frame_index; }

	static void SetXForObject(Object* object, float value) { object->x = value; }
	static void SetYForObject(Object* object, float value) { object->y = value; }
	static void SetSpdForObject(Object* object, float value) { object->spd = value; }
	static void SetDirForObject(Object* object, float value) { object->dir = cpml::angle_wrap(value); }
	static void SetAccForObject(Object* object, float value) { object->acc = value; }

	static void SetSprForObject(Object* object, void* value) { object->sprite = (Sprite*) value; }
	static void SetImgForObject(Object* object, float value) { object->frame_index = value; }



	static int lua_random(lua_State* L) {
		lua_checkargc(L, 0, 2);

		float a = 0.0f;
		float b = 1.0f;

		int argc = lua_gettop(L);
		if (argc > 0) {
			b = (float) luaL_checknumber(L, 1);
			if (argc > 1) {
				a = b;
				b = (float) luaL_checknumber(L, 2);
			}
		}

		Stage& stage = Stage::GetInstance();
		float r = stage.random.range(a, b);

		lua_pushnumber(L, (lua_Number) r);
		return 1;
	}

	static int lua__subwait(lua_State* L) {
		lua_checkargc(L, 1, 1);
		float t = (float) luaL_checknumber(L, 1);

		lua_rawgetp(L, LUA_REGISTRYINDEX, LUA_REG_SLOT_TIMER);
		float subtime = (float) lua_tonumber(L, -1);
		lua_pop(L, 1);

		subtime += t;

		lua_pushnumber(L, (lua_Number) subtime);
		lua_rawsetp(L, LUA_REGISTRYINDEX, LUA_REG_SLOT_TIMER);

		if (subtime >= CORO_DELTA) {
			subtime -= CORO_DELTA;

			lua_pushnumber(L, (lua_Number) subtime);
			lua_rawsetp(L, LUA_REGISTRYINDEX, LUA_REG_SLOT_TIMER);

			lua_yield(L, 0);
		}

		return 0;
	}

	static int lua_log(lua_State* L) {
		int n = lua_gettop(L);
		for (int i = 1; i <= n; i++) {
			const char* str = luaL_tolstring(L, i, nullptr);
			lua_pop(L, 1);
			if (i > 1) {
				LOG_NO_LINE(" ");
			}
			LOG_NO_LINE("%s", str);
		}
		LOG("");
		return 0;
	}

	static int lua_FindSprite(lua_State* L) {
		Assets& assets = Assets::GetInstance();

		lua_checkargc(L, 1, 1);
		const char* name = luaL_checkstring(L, 1);
		Sprite* result = assets.FindSprite(name);
		lua_pushlightuserdata(L, result);
		return 1;
	}

	static int lua_GetTime(lua_State* L) {
		lua_checkargc(L, 0, 0);
		Stage& stage = Stage::GetInstance();
		lua_pushnumber(L, (lua_Number) stage.time);
		return 1;
	}

	static int lua_GetTarget(lua_State* L) {
		lua_checkargc(L, 1, 1);
		full_instance_id result = MAKE_INSTANCE_ID(0, TYPE_PLAYER);
		lua_pushinteger(L, result);
		return 1;
	}

	static int lua_CreateBoss(lua_State* L) {
		auto& stage = Stage::GetInstance();
		lua_checkargc(L, 1, 1);

		int boss_index = (int) luaL_checkinteger(L, 1);
		Boss& result = stage.CreateBoss(boss_index);

		lua_pushinteger(L, result.full_id);
		return 1;
	}

	static int lua_CreateBullet(lua_State* L) {
		auto& stage = Stage::GetInstance();

		lua_checkargc(L, 8, 9);

		Bullet& result = stage.CreateBullet();

		int i = 1;
		result.x   = (float) luaL_checknumber(L, i++);
		result.y   = (float) luaL_checknumber(L, i++);
		result.spd = (float) luaL_checknumber(L, i++);
		result.dir = (float) luaL_checknumber(L, i++);
		result.acc = (float) luaL_checknumber(L, i++);
		result.radius = (float) luaL_checknumber(L, i++);
		result.sprite = (Sprite*) lua_touserdata(L, i++);
		result.flags = (uint32_t) luaL_checkinteger(L, i++);

		result.dir = cpml::angle_wrap(result.dir);

		if (!lua_isnoneornil(L, i)) {
			if (lua_isfunction(L, i)) {
				lua_copy(L, i, -1);
				result.coroutine = CreateCoroutine(L, stage.L);
			} else {
				LOG("CreateBullet: 9th arg (script) is not a function");
			}
		}
		i++;

		lua_pushinteger(L, result.full_id);
		return 1;
	}

	static int lua_CreateLazer(lua_State* L) {
		auto& stage = Stage::GetInstance();

		lua_checkargc(L, 8, 9);

		Bullet& result = stage.CreateBullet();

		int i = 1;
		result.x   = (float) luaL_checknumber(L, i++);
		result.y   = (float) luaL_checknumber(L, i++);
		result.spd = (float) luaL_checknumber(L, i++);
		result.dir = (float) luaL_checknumber(L, i++);
		result.sprite = (Sprite*) lua_touserdata(L, i++);
		result.lazer_target_length = (float) luaL_checknumber(L, i++);
		result.lazer_thickness = (float) luaL_checknumber(L, i++);
		result.flags = (uint32_t) luaL_checkinteger(L, i++);

		result.dir = cpml::angle_wrap(result.dir);

		if (result.spd < 0.01f) result.spd = 0.01f;
		if (result.lazer_target_length < 1.0f) result.lazer_target_length = 1.0f;

		result.lazer_time = result.lazer_target_length / result.spd;
		result.type = ProjectileType::Lazer;

		if (!lua_isnoneornil(L, i)) {
			if (lua_isfunction(L, i)) {
				lua_copy(L, i, -1);
				result.coroutine = CreateCoroutine(L, stage.L);
			} else {
				LOG("CreateLazer: 9th arg (script) is not a function");
			}
		}
		i++;

		//PlaySound("se_lazer.wav");
		lua_pushinteger(L, result.full_id);
		return 1;
	}



	// static void *l_alloc (void *ud, void *ptr, size_t osize,
	// 					  size_t nsize) {
	// 	if (nsize == 0) {
	// 		free(ptr);
	// 		return NULL;
	// 	} else if (!ptr) {
	// 		static int i = 0;
	// 		printf("[%d] allocated %zu bytes\n", i++, nsize);
	// 		return malloc(nsize);
	// 	} else {
	// 		return realloc(ptr, nsize);
	// 	}
	// }

	static void *l_alloc (void *ud, void *ptr, size_t osize,
						  size_t nsize) {
		Loki::SmallObjAllocator* allocator = (Loki::SmallObjAllocator*) ud;
		if (nsize == 0) {
			if (ptr) allocator->Deallocate(ptr, osize);
			return NULL;
		} else if (!ptr) {
			// static int i = 0;
			// printf("[%d] allocated %zu bytes\n", i++, nsize);
			return allocator->Allocate(nsize, false);
		} else {
			void* newptr = allocator->Allocate(nsize, false);
			memcpy(newptr, ptr, std::min(osize, nsize));
			allocator->Deallocate(ptr, osize);
			return newptr;
		}
	}

	void Stage::InitLua() {
		// L = luaL_newstate();
		// L = lua_newstate(l_alloc, NULL);
		L = lua_newstate(l_alloc, &lua_allocator);

		{
			luaL_Reg loadedlibs[] = {
				{LUA_GNAME, luaopen_base},
				{LUA_COLIBNAME, luaopen_coroutine},
				{LUA_TABLIBNAME, luaopen_table},
				{LUA_STRLIBNAME, luaopen_string},
				{LUA_MATHLIBNAME, luaopen_math},
				{LUA_UTF8LIBNAME, luaopen_utf8},
				{NULL, NULL}
			};

			const luaL_Reg *lib;
			/* "require" functions from 'loadedlibs' and set results to global table */
			for (lib = loadedlibs; lib->func; lib++) {
				luaL_requiref(L, lib->name, lib->func, 1);
				lua_pop(L, 1);  /* remove lib */
			}
		}

		{
			lua_pushnil(L);
			lua_setglobal(L, "dofile");
			lua_pushnil(L);
			lua_setglobal(L, "load");
			lua_pushnil(L);
			lua_setglobal(L, "loadfile");
			lua_pushnil(L);
			lua_setglobal(L, "print");
		}

		{
			_lua_register(L, "random", lua_random);
			_lua_register(L, "_subwait", lua__subwait);
			_lua_register(L, "log", lua_log);
			_lua_register(L, "FindSprite", lua_FindSprite);
			_lua_register(L, "GetTime", lua_GetTime);
			_lua_register(L, "GetTarget", lua_GetTarget);

			_lua_register(L, "CreateBoss", lua_CreateBoss);
			_lua_register(L, "CreateBullet", lua_CreateBullet);
			_lua_register(L, "CreateLazer", lua_CreateLazer);

			_lua_register(L, "GetX", lua_GetObjectVar<float, GetXFromObject>);
			_lua_register(L, "GetY", lua_GetObjectVar<float, GetYFromObject>);
			_lua_register(L, "GetSpd", lua_GetObjectVar<float, GetSpdFromObject>);
			_lua_register(L, "GetDir", lua_GetObjectVar<float, GetDirFromObject>);
			_lua_register(L, "GetAcc", lua_GetObjectVar<float, GetAccFromObject>);

			_lua_register(L, "GetSpr", lua_GetObjectVar<void*, GetSprFromObject>);
			_lua_register(L, "GetImg", lua_GetObjectVar<float, GetImgFromObject>);

			_lua_register(L, "SetX", lua_SetObjectVar<float, SetXForObject>);
			_lua_register(L, "SetY", lua_SetObjectVar<float, SetYForObject>);
			_lua_register(L, "SetSpd", lua_SetObjectVar<float, SetSpdForObject>);
			_lua_register(L, "SetDir", lua_SetObjectVar<float, SetDirForObject>);
			_lua_register(L, "SetAcc", lua_SetObjectVar<float, SetAccForObject>);

			_lua_register(L, "SetSpr", lua_SetObjectVar<void*, SetSprForObject>);
			_lua_register(L, "SetImg", lua_SetObjectVar<float, SetImgForObject>);
		}

		{
			auto& assets = Assets::GetInstance();

			auto& scripts = assets.GetScripts();

			for (auto it = scripts.begin(); it != scripts.end(); ++it) {
				const Script& script = it->second;

				//if (!(script->stage_index == game.stage_index || script->stage_index == -1)) {
				//	continue;
				//}

				const char* debug_name = it->first.c_str();

				if (luaL_loadbuffer(L, script.data, script.size, debug_name) != LUA_OK) {
					LOG("luaL_loadbuffer failed\n%s", lua_tostring(L, -1));
					lua_settop(L, 0);
					continue;
				}

				if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
					LOG("error while running script\n%s", lua_tostring(L, -1));
					lua_settop(L, 0);
					continue;
				}
			}
		}
	}

	void Stage::CallCoroutines() {
		UpdateCoroutine(L, &coroutine, -1);

		for (size_t i = 0, n = bosses.size(); i < n; i++) {
			Boss& boss = bosses[i];
			UpdateCoroutine(L, &boss.coroutine, boss.full_id);
		}

		for (size_t i = 0, n = enemies.size(); i < n; i++) {
			Enemy& enemy = enemies[i];
			UpdateCoroutine(L, &enemy.coroutine, enemy.full_id);
		}

		for (size_t i = 0, n = bullets.size(); i < n; i++) {
			Bullet& bullet = bullets[i];
			UpdateCoroutine(L, &bullet.coroutine, bullet.full_id);
		}
	}

}
