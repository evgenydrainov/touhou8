function Boss0_Phase0(id)
	local function shoot_radial_bullets()
		return ShootRadial(17, 360 / 17, function()
			return Shoot{GetX(id), GetY(id), 3.5, TargetDir(id), 0, BULLET_OUTLINE, 6}
		end)
	end

	while true do
		for i = 1, 3 do
			for i = 0, 5 do
				ShootRadial(7 - i, 5, function()
					return Shoot{GetX(id), GetY(id), lerp(4, 7.5, i / 5), TargetDir(id), 0, BULLET_PELLET, 6}
				end)
			end

			wait(60)
		end

		wait(15)

		Wander(id)

		wait(15)

		for i = 1, 3 do
			shoot_radial_bullets()

			wait(15)

			ShootRadial(17, 360 / 17, function()
				return Shoot{GetX(id), GetY(id), 4.5, TargetDir(id), -0.08, BULLET_PELLET, 15, script=function(id)
					while GetSpd(id) > 0 do wait(1) end
					SetAcc(id, 0)
					SetSpd(id, 5)
					SetDir(id, TargetDir(id))
				end}
			end)

			wait(15)

			shoot_radial_bullets()

			wait(60)

			if i == 3 then
				GoBack(id)
			else
				Wander(id)
			end
		end

		wait(60)
	end
end

function Boss0_Phase1(id)
	while true do
		for i = 0, 10 do
			local off = 100 - 5 * i

			for side = 0, 1 do
				local dir = (side == 0) and (270 - off) or (270 + off)

				for j = 0, 2 do
					local x = GetX(id)
					local y = GetY(id)
					local target_x = x + lengthdir_x(100 + 90 * j, dir)
					local target_y = y + lengthdir_y(100 + 90 * j, dir)
					local bullet = Shoot{x, y, 0, dir, 0, BULLET_PELLET, 6, script=function(id)
						wait(50)
						SetSpd(id, 2)
						SetAcc(id, 0)
						local dir = GetDir(id)
						if 90 <= dir and dir < 270 then
							SetDir(id, dir + 90)
						else
							SetDir(id, dir - 90)
						end
					end}
					LaunchTowardsPoint(bullet, target_x, target_y, 0.07)
				end
			end

			if i % 3 == 2 then
				ShootRadial(5, 20, function()
					return Shoot{GetX(id), GetY(id), 2, TargetDir(id), 0, BULLET_FILLED, 13}
				end)
			end

			wait(20)
		end
	end
end

function Boss0_Phase2(id)
	while true do
		Wander(id)

		for i = 1, 16 do
			if i % 2 == 0 then
				ShootRadial(8, 360 / 8, function()
					return Shoot{GetX(id), GetY(id), 4, TargetDir(id) + 360 / 8 / 2, 0, BULLET_OUTLINE, 6}
				end)
			else
				ShootRadial(8, 360 / 8, function()
					return Shoot{GetX(id), GetY(id), 2, TargetDir(id), 0, BULLET_SMALL, 6}
				end)
			end

			wait(10)
		end

		Wander(id)

		for i = 1, 3 do
			ShootRadial(3, 20, function()
				return ShootLazer{GetX(id), GetY(id), 3.5, TargetDir(id), 180, 2, 15}
			end)

			wait(60)
		end
	end
end

function Boss0_Phase3(id)
	while true do
		local bullets = {}

		Wander(id)

		for i = 1, 100 do
			for i = 1, 2 do
				local color = choose(2, 6, 10, 13, 14)
				bullets[#bullets+1] = Shoot{GetX(id), GetY(id), random(1, 4), random(360), 0, BULLET_OUTLINE, color}
			end

			wait(1)
		end

		wait(60)

		for b in ivalues(bullets) do
			SetSpd(b, 0)
			SetImg(b, 15)
		end

		wait(60)

		Wander(id)

		for i = 1, 5 do
			for i = 0, 4 do
				ShootRadial(4, 30, function()
					return Shoot{GetX(id), GetY(id), lerp(2, 6, i / 4), TargetDir(id), 0, BULLET_OUTLINE, 6}
				end)
			end

			wait(10)
		end

		wait(60)

		for b in ivalues(bullets) do
			SetSpd(b, 0)
			SetDir(b, random(360))
			SetAcc(b, random(0.01, 0.015))
		end

		wait(180)
	end
end

function Boss0_Phase4(id)
	while true do
		for i = 1, 30 do
			local bullets = {}

			local x = GetX(id) + random(-50, 50)
			local y = GetY(id) + random(-50, 50)

			local n = random(4, 6)
			for i = 1, n do
				bullets[i] = Shoot{x, y, random(4, 5), random(360), 0, BULLET_PELLET, 6}
			end

			wait(4)

			for i = 1, n do
				SetSpd(bullets[i], random(1, 2))
			end
		end

		Wander(id)
	end
end