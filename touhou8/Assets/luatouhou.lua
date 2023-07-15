math.random = nil
math.randomseed = nil

sin = math.sin
cos = math.cos
rad = math.rad
deg = math.deg

function dsin(x) return sin(rad(x)) end
function dcos(x) return cos(rad(x)) end

function lengthdir_x(l, d) return l *  dcos(d) end
function lengthdir_y(l, d) return l * -dsin(d) end

min = math.min
max = math.max

function clamp(x, _min, _max) return min(max(x, _min), _max) end

function approach(a, b, n) return a + clamp(b - a, -n, n) end

function sqr(x) return x * x end

sqrt = math.sqrt

function point_direction(x1, y1, x2, y2) return deg(math.atan(y1 - y2, x2 - x1))  end
function point_distance(x1, y1, x2, y2)  return sqrt(sqr(x2 - x1) + sqr(y2 - y1)) end

function lerp(a, b, f) return a + (b - a) * f end

function round(x) return x + (2^52 + 2^51) - (2^52 + 2^51) end

function wrap(a, b) return (a % b + b) % b end

function angle_wrap(a) return wrap(a, 360) end

function angle_difference(dest, src)
	return angle_wrap(dest - src + 180) - 180
end

abs   = math.abs
floor = math.floor
ceil  = math.ceil

function choose(...)
	local arg = {...}
	local i = 1 + floor(random(#arg))
	return arg[i]
end

function ivalues(t)
	local i=0
	return function() i=i+1 return t[i] end
end

function range(n)
	local i=0
	return function() i=i+1 if i<=n then return i end end
end

function wait(t)
	while t >= delta do
		coroutine.yield()
		t = t - delta
	end
	if t > 0 then
		_subwait(t)
	end
end

function print_table(t)
	for k, v in pairs(t) do
		print(tostring(k) .. " = " .. tostring(v))
	end
end

function seconds(t) return t*60 end

print = log

_bullet_radius = {[0]=3, 3, 4, 2, 2, 2, 2}
_bullet_rotate = {[0]=true, false, false, true, true, true, false}

-- {x, y, spd, dir, acc, bullet_type, color_index, flags=flags, script=script}
function Shoot(arg)
	local tp = arg[6]
	local color = arg[7]

	local sprite = FindSprite("bullet"..tp)
	local frame_index = color
	local radius = _bullet_radius[tp]
	local flags = arg.flags or 0
	if _bullet_rotate[tp] then
		flags = flags | BULLET_ROTATE
	end

	local result = CreateBullet(arg[1], arg[2], arg[3], arg[4], arg[5], radius, sprite, flags, arg.script)
	SetImg(result, frame_index)

	return result
end

-- {x, y, spd, dir, length, thickness, color, flags=flags, script=scrpit}
function ShootLazer(arg)
	local sprite = FindSprite("lazer")
	local flags = arg.flags or 0

	local result = CreateLazer(arg[1], arg[2], arg[3], arg[4], sprite, arg[5], arg[6], flags, arg.script)
	SetImg(result, arg[7])

	return result
end

function ShootRadial(n, dir_diff, f)
	local res = {}
	for i = 0, n - 1 do
		local mul = -(n - 1) / 2 + i
		local got = f(i)
		if type(got) == "table" then
			for b in ivalues(got) do
				SetDir(b, GetDir(b) + dir_diff * mul)
				res[#res+1] = b
			end
		elseif type(got) == "number" then
			SetDir(got, GetDir(got) + dir_diff * mul)
			res[#res+1] = got
		end
	end
	return res
end

function EntityDir(e1, e2)
	return point_direction(GetX(e1), GetY(e1), GetX(e2), GetY(e2))
end

function TargetDir(id)
	return EntityDir(id, GetTarget(id))
end

function LaunchTowardsPoint(id, target_x, target_y, acc)
	acc = abs(acc)
	local x = GetX(id)
	local y = GetY(id)
	local dist = point_distance(x, y, target_x, target_y)
	SetSpd(id, sqrt(dist * acc * 2))
	SetAcc(id, -acc)
	SetDir(id, point_direction(x, y, target_x, target_y))
end

function Wander(id)
	local target_x = random(32, PLAY_AREA_W-32)
	local target_y = random(32, BOSS_STARTING_Y*2-32)
	local x = GetX(id)
	local y = GetY(id)
	target_x = clamp(target_x, x-80, x+80)
	target_y = clamp(target_y, y-80, y+80)
	LaunchTowardsPoint(id, target_x, target_y, 0.01)
end

function GoBack(id)
	LaunchTowardsPoint(id, BOSS_STARTING_X, BOSS_STARTING_Y, 0.02)
end

function WaitTillDies(obj)
	while Exists(obj) do
		wait(1)
	end
end

PLAY_AREA_W = 384
PLAY_AREA_H = 448
BOSS_STARTING_X = 192
BOSS_STARTING_Y = 96

BULLET_BULLET  = 0
BULLET_OUTLINE = 1
BULLET_FILLED  = 2
BULLET_RICE    = 3
BULLET_KUNAI   = 4
BULLET_PELLET  = 5
BULLET_SMALL   = 6

BULLET_ROTATE = 1 << 8