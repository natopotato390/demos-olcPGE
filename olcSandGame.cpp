#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <random>
#include <chrono>

#define MATERIAL_COUNT 7

const olc::Pixel mat_col_bank[MATERIAL_COUNT]	// Default material colors
{ 
	{ 50, 50, 50 }, 
	{ 255, 191, 123 },
	{ 0, 0, 160 },
	{ 30, 30, 30 },
	{ 90, 0, 0 },
	{ 0, 0, 0 },
	{ 50, 0, 50 }
};

const std::string mat_name_bank[MATERIAL_COUNT]
{
	"",
	"Sand",
	"Water",
	"Wall",
	"Spawner",
	"Eater",
	"Corrupt"
};

std::uniform_real_distribution<double> unif(0, 1);
std::mt19937_64 rng;

enum material_id
{
	MAT_NONE,
	MAT_SAND,
	MAT_WATER,
	MAT_WALL,
	MAT_SPAWNER,
	MAT_EATER,
	MAT_CORRUPTION
} material_id;

struct particle
{
	int mat_id;
	// float life_time; Unused
	// olc::vi2d vel;
	olc::Pixel col;
	bool is_updated;

	particle(int id) 
		: mat_id(id), col(compute_mat_color(id))
	{
	}

	particle(int id, olc::Pixel color) : particle(id) 
	{
		col = color;
	}

private:
	olc::Pixel compute_mat_color(int mat_id)
	{
		switch (mat_id)
		{
		case MAT_WALL: return color_random_brightness(mat_col_bank[mat_id], 0.3f);
		case MAT_SAND: return color_random_brightness(mat_col_bank[mat_id], 0.6f);
		case MAT_CORRUPTION: return color_random_brightness(mat_col_bank[mat_id], 2.0f);
		case MAT_WATER:
		case MAT_NONE: 
		case MAT_SPAWNER:
		case MAT_EATER: return mat_col_bank[mat_id];
		default: return { 255,0,255 };
		}
	}

	olc::Pixel color_random_brightness(olc::Pixel sCol, float range)
	{
		sCol *= static_cast<float> (((rand()) / (static_cast <float> (RAND_MAX / range * 2) - range) + 1.0f));
		return sCol;
	}
};

struct hsv	// TODO
{
	double h;
	double s;
	double v;
};

class spr_button
{

public:
	olc::vi2d pos;
	olc::vi2d size;

	bool bHeld;
	bool bPressed;
	bool bReleased;
	bool bHover;

private:
	olc::Sprite* sprSheet;
	int sprWidth;

	// Sprite sheet 3 sprites in this order:
	// sprNormal;
	// sprHover;
	// sprPressed;

public:
	spr_button(olc::Sprite* spr)
	{

	}

	void update(olc::PixelGameEngine* p)
	{
		bHover = (point_is_in_rect(p->GetMousePos(), pos, size));
	}

	void draw(olc::PixelGameEngine* p)
	{
	}

private:
	bool point_is_in_rect(olc::vi2d p, olc::vi2d pos, olc::vi2d size)
	{
		return (p.x >= pos.x && p.y >= pos.y && p.x < pos.x + size.x && p.y < pos.y + size.y);
	}
};

class Program : public olc::PixelGameEngine
{
public:
	Program()
	{
		sAppName = "olcSandGame";
	}

	int mapWidth = 360;
	int mapHeight = 360;

	std::vector<particle> vMap;

	float mapUpdateTimer = 0.0f;
	float mapUpdateRate = 0.01f;
	double corruptionSpread = 0.05;

	int brushMaterial = 1;
	int brushSize = 2;
	bool brushIsSquare = 1;
	
	bool isRunning = 1;
	bool mapHasUpdated = 0;

	olc::vi2d curMousePos;
	olc::vi2d oldMousePos;

	olc::vi2d mapPos = { 140,0 };
	olc::vi2d mouseWorldPos;

	int mouse_info_offset = 10;

public:
	bool OnUserCreate() override
	{
		for (int i = 0; i < mapWidth * mapHeight; i++)
		{
				vMap.push_back((unif(rng) < 0.1) ? particle(MAT_SAND) : particle(MAT_NONE));
		}

		uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
		rng.seed(ss);
		// initialize a uniform distribution between 0 and 1


		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		if (GetKey(olc::ESCAPE).bPressed) exit(0);
		// UPDATE =====================================================================
		mapUpdateTimer += fElapsedTime;

		update_map_input(fElapsedTime);

		update_map();

		// DRAW =======================================================================
		Clear(olc::Pixel(255,0,255));
		SetPixelMode(olc::Pixel::ALPHA);

		// Map
		int i = 0;
		for (int x = mapPos.x; x < mapPos.x + mapWidth; x++)		// Update from bottom to top
			for (int y = mapPos.y; y < mapPos.y + mapHeight; y++)
			{
				Draw(x, y, vMap[i].col);
				i++;
			}


		// Brush
		if (brushIsSquare) {
			if (brushSize == 0) Draw(GetMousePos(), { 255,255,255,100 });
			else DrawAlphaRect(GetMouseX() - brushSize, GetMouseY() - brushSize, (brushSize * 2) - 1, (brushSize * 2) - 1, { 255,255,255,100 }); }
		else
			DrawCircle(GetMousePos(), brushSize, { 255,255,255,100 });

		// UI
		FillRect(0, 0, 139, ScreenHeight(), {70,70,70});
		FillRect(501, 0, 139, ScreenHeight(), {70,70,70});

		DrawLine(139, 0, 139, ScreenHeight(), (isRunning) ? olc::Pixel(190, 37, 37) : olc::Pixel(255, 255, 255));
		DrawLine(500, 0, 500, ScreenHeight(), (isRunning) ? olc::Pixel(190, 37, 37) : olc::Pixel(255, 255, 255));

		// Info strings
		DrawStringShaded(3, 3, "Brush: " + mat_name_bank[brushMaterial]);
		DrawStringShaded(3, 12, "Size: " + std::to_string(brushSize+1));
		DrawStringShaded(3, 21, "Shape: ");
		DrawStringShaded(58, 21, (brushIsSquare) ? "Square" : "Circle");

		DrawStringShaded(538, 20, "CONTROLS");
		DrawLine(530, 31, 610, 31);
		DrawLine(531, 32, 611, 32, olc::BLACK);

		DrawStringShaded(513, 45, "LMB: Draw");
		DrawStringShaded(513, 55, "RMB: Erase");

		DrawStringShaded(513, 75, "SHIFT: Change");
		DrawStringShaded(513, 85, "brush SHAPE");

		DrawStringShaded(513, 105, "Scroll: Change");
		DrawStringShaded(513, 115, "brush SIZE");

		DrawStringShaded(513, 135, "1: Wall");
		DrawStringShaded(513, 145, "2: Sand");
		DrawStringShaded(513, 155, "3: Water");
		DrawStringShaded(513, 165, "4: Spawner");
		DrawStringShaded(513, 175, "5: Eater");
		DrawStringShaded(513, 185, "6: Corrupt");

		DrawStringShaded(513, 205, "SPACE: Pause");
		DrawStringShaded(513, 215, "simulation");

		if (!isRunning)
			DrawStringShaded(40, 330, "PAUSED");



		DrawStringShaded(542, 320, "Made by");
		DrawStringShaded(518, 330, "Nate Robinson");


		mouse_info_offset = (GetMouseY() < ScreenHeight() - 20) ? 10 : -10;

		if (in_bounds(mouseWorldPos.x, mouseWorldPos.y))
			DrawStringShaded(GetMouseX() + 10, GetMouseY() + mouse_info_offset, mat_name_bank[vMap[xy_to_i(mouseWorldPos.x, mouseWorldPos.y)].mat_id]);
			


		return true;
	}

	#pragma region INPUT

	void update_map_input(float fElapsedTime)
	{
		curMousePos = GetMousePos();
		mouseWorldPos = GetMousePos() - mapPos;

		brushSize += (GetMouseWheel() / 120);
		brushSize = std::clamp(brushSize, 0, 360);

		if (GetKey(olc::SHIFT).bPressed) brushIsSquare = !brushIsSquare;
		if (GetKey(olc::SPACE).bPressed) isRunning = !isRunning;

		if (GetKey(olc::K1).bPressed) brushMaterial = 3;	// Wall 
		if (GetKey(olc::K2).bPressed) brushMaterial = 1;	// Sand
		if (GetKey(olc::K3).bPressed) brushMaterial = 2;	// Water
		if (GetKey(olc::K4).bPressed) brushMaterial = 4;	// Spawner
		if (GetKey(olc::K5).bPressed) brushMaterial = 5;	// Eater
		if (GetKey(olc::K6).bPressed) brushMaterial = 6;	// Corruption


		if (GetMouse(0).bPressed || GetMouse(1).bPressed || curMousePos != oldMousePos || mapHasUpdated)
		{
			if (GetMouse(1).bHeld && in_bounds(mouseWorldPos.x, mouseWorldPos.y))
				erase_map(mouseWorldPos, brushSize);

			if (GetMouse(0).bHeld && in_bounds(mouseWorldPos.x, mouseWorldPos.y))
				fill_map(mouseWorldPos, brushSize, brushMaterial);

			mapHasUpdated = false;
		}
		
		oldMousePos = curMousePos;
	}

	#pragma endregion

	#pragma region BRUSH FUNCTIONS

	void fill_map(olc::vi2d pos, int radius, int material_id)	// BAD REPEATING CODE
	{
		if (radius == 0)
		{
			if (!in_bounds(pos.x, pos.y)) return;
			switch (material_id)
			{
			case MAT_SAND:
			case MAT_WATER:
			case MAT_WALL:
			case MAT_CORRUPTION:
				if (!is_solid(xy_to_i(pos.x, pos.y)))
					vMap[xy_to_i(pos.x, pos.y)] = material_id;
				break;
			case MAT_SPAWNER:
			case MAT_EATER:
				vMap[xy_to_i(pos.x, pos.y)] = material_id;
				break;
			}
		}

		for (int x = pos.x - radius; x < pos.x + radius; x++)
			for (int y = pos.y - radius; y < pos.y + radius; y++)
			{
				if (brushIsSquare) { if (!in_bounds(x, y)) continue; }
				else { if (!in_bounds(x, y) || dist_points(pos, olc::vi2d(x, y)) > radius) continue; }

				switch (material_id)
				{
				case MAT_SAND:
				case MAT_WATER:
				case MAT_CORRUPTION:
					if (!is_solid(xy_to_i(x, y)))
						if (isRunning)
						{
							if (unif(rng) < (1.0 / radius) * 0.5)
								vMap[xy_to_i(x, y)] = material_id;
						}
						else
							vMap[xy_to_i(x, y)] = material_id;
					break;
				case MAT_WALL:
				case MAT_SPAWNER:
				case MAT_EATER:
					if (vMap[xy_to_i(x, y)].mat_id != MAT_WALL)
					vMap[xy_to_i(x, y)] = material_id;
					break;
				}
				
			}
	}

	void erase_map(olc::vi2d pos, int radius)
	{
		if (radius == 0)
		{
			if (!in_bounds(pos.x, pos.y)) return;
			vMap[xy_to_i(pos.x, pos.y)] = MAT_NONE;
		}

		for (int x = pos.x - radius; x < pos.x + radius; x++)
			for (int y = pos.y - radius; y < pos.y + radius; y++)
			{
				if (brushIsSquare) { if (!in_bounds(x, y)) continue; }
				else { if (!in_bounds(x, y) || dist_points(pos, olc::vi2d(x, y)) > radius) continue; }

				vMap[xy_to_i(x, y)] = MAT_NONE;
			}
	}

	#pragma endregion

	#pragma region PARTICLE UPDATE FUNCTIONS

	void update_map()
	{
		// Map
		if (mapUpdateTimer > mapUpdateRate && isRunning)
		{
			for (int i = 0; i < vMap.size(); i++)
			{
				vMap[i].is_updated = false;
			}

			for (int y = mapHeight - 1; y >= 0; y--)		// Update from bottom to top
			{
				for (int x = 0; x < mapWidth; x++)
				{
					switch (vMap[xy_to_i(x, y)].mat_id)
					{
					case MAT_NONE:
						break;
					case MAT_SAND: update_sand(x, y); break;
					case MAT_WATER: update_water(x, y); break;
					case MAT_SPAWNER: update_spawner(x, y); break;
					case MAT_EATER: update_eater(x, y); break;
					case MAT_WALL: break;
					case MAT_CORRUPTION: update_corruption(x, y); break;
					default: break;
					}
				}
			}
			mapUpdateTimer = 0.0f;
			mapHasUpdated = true;
		}
	}

	void update_sand(int x, int y)
	{
		int cur_idx = xy_to_i(x, y);
		int b_idx = xy_to_i(x, y + 1);
		int br_idx = xy_to_i(x + 1, y + 1);
		int bl_idx = xy_to_i(x - 1, y + 1);
		int r_idx = xy_to_i(x + 1, y);
		int l_idx = xy_to_i(x - 1, y);



		if (in_bounds(x, y + 1) && is_empty_or_water(b_idx))
		{
			particle temp = get_particle_at(x, y + 1);
			vMap[b_idx] = vMap[cur_idx];
			vMap[cur_idx] = temp;		// Swap the 2 particles
		}
		else if (unif(rng) < 0.5)
		{
			if (in_bounds(x + 1, y + 1) && is_empty_or_water(br_idx) && is_empty_or_water(r_idx))
			{
				particle temp = get_particle_at(x + 1, y + 1);
				vMap[br_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
			}
		}
		else
		{
			if (in_bounds(x - 1, y + 1) && is_empty_or_water(bl_idx) && is_empty_or_water(l_idx))
			{
				particle temp = get_particle_at(x - 1, y + 1);
				vMap[bl_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
			}
		}

	}

	void update_water(int x, int y)
	{
		int cur_idx = xy_to_i(x, y);
		int b_idx = xy_to_i(x, y + 1);
		int br_idx = xy_to_i(x + 1, y + 1);
		int bl_idx = xy_to_i(x - 1, y + 1);
		int r_idx = xy_to_i(x + 1, y);
		int l_idx = xy_to_i(x - 1, y);

		if (in_bounds(x, y + 1) && is_empty(b_idx))
		{
			particle temp = get_particle_at(x, y + 1);
			vMap[b_idx] = vMap[cur_idx];
			vMap[cur_idx] = temp;		// Swap the 2 particles
		}
		else if (unif(rng) < 0.5)
		{
			if (in_bounds(x + 1, y + 1) && is_empty(br_idx) && is_empty(r_idx))
			{
				particle temp = get_particle_at(x + 1, y + 1);
				vMap[br_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
				return;
			}
		}
		else
		{
			if (in_bounds(x - 1, y + 1) && is_empty(bl_idx) && is_empty(l_idx))
			{
				particle temp = get_particle_at(x - 1, y + 1);
				vMap[bl_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
				return;
			}
		}
		if (unif(rng) < 0.5)
		{
			if (in_bounds(x + 1, y) && is_empty(r_idx) && !vMap[cur_idx].is_updated)
			{
				particle temp = get_particle_at(x + 1, y);
				vMap[r_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
				vMap[r_idx].is_updated = true;

			}
		}
		else
		{
			if (in_bounds(x - 1, y) && is_empty(l_idx) && !vMap[cur_idx].is_updated)
			{
				particle temp = get_particle_at(x - 1, y);
				vMap[l_idx] = vMap[cur_idx];
				vMap[cur_idx] = temp;
				vMap[l_idx].is_updated = true;
			}
		}
	}

	void update_spawner(int x, int y)
	{
		int a_idx = xy_to_i(x, y - 1);
		int b_idx = xy_to_i(x, y + 1);

		if(is_non_solid(a_idx) && vMap[b_idx].mat_id == MAT_NONE)
			if (unif(rng) < 0.1)
				vMap[b_idx] = particle(vMap[a_idx].mat_id);
	}

	void update_eater(int x, int y)
	{
		int a_idx = xy_to_i(x, y - 1);

		if (is_non_solid(a_idx))
			vMap[a_idx] = MAT_NONE;
	}

	void update_corruption(int x, int y)
	{
		int cur_idx = xy_to_i(x, y);

		int b_idx = xy_to_i(x, y + 1);
		int br_idx = xy_to_i(x + 1, y + 1);
		int bl_idx = xy_to_i(x - 1, y + 1);
		int r_idx = xy_to_i(x + 1, y);
		int l_idx = xy_to_i(x - 1, y);
		int tr_idx = xy_to_i(x + 1, y - 1);
		int tl_idx = xy_to_i(x - 1, y - 1);
		int t_idx = xy_to_i(x - 1, y - 1);

		int spread_idx = (int)(unif(rng) * 7.0);

		int spread[8] = { b_idx, br_idx, bl_idx, r_idx, l_idx, tr_idx, tl_idx, t_idx };

		if(in_bounds(spread[spread_idx]) && is_corruptable(spread[spread_idx]) == MAT_SAND && unif(rng) < corruptionSpread)
			vMap[spread[spread_idx]] = particle(MAT_CORRUPTION);


	}

	#pragma endregion

	#pragma region HELPER FUNCTIONS

	bool is_empty(int i) { return vMap[i].mat_id == MAT_NONE; }
	bool is_empty_or_water(int i) { return (vMap[i].mat_id == MAT_NONE || vMap[i].mat_id == MAT_WATER); }
	bool is_non_solid(int i) { return (vMap[i].mat_id == MAT_SAND || vMap[i].mat_id == MAT_WATER); }
	bool is_solid(int i) { return (vMap[i].mat_id == MAT_WALL || vMap[i].mat_id == MAT_SPAWNER || vMap[i].mat_id == MAT_EATER); }
	bool is_corruptable(int i) { return (vMap[i].mat_id == MAT_SAND || vMap[i].mat_id == MAT_CORRUPTION); }



	bool in_bounds(int x, int y)
	{
		return !(x < 0 || x > mapWidth - 1 || y < 0 || y > mapHeight - 1);
	}
	bool in_bounds(int i)
	{
		return (i >= 0 && i < mapWidth * mapHeight);
	}

	particle get_particle_at(int x, int y)
	{
		return vMap[xy_to_i(x, y)];
	}

	int xy_to_i(int x, int y) { return x * mapWidth + y; }
	olc::vi2d i_to_xy(int i) { return olc::vi2d(i / mapWidth, i % mapHeight); }

	float dist_points(olc::vi2d a, olc::vi2d b)
	{
		return (b - a).mag();
	}

	void DrawAlphaRect(int32_t x, int32_t y, int32_t w, int32_t h, olc::Pixel p) // Fixes bright corners when drawing rects with alpha
	{
		DrawLine(x + 1, y, x + w - 1, y, p);
		DrawLine(x + w, y, x + w, y + h, p);
		DrawLine(x + w - 1, y + h, x + 1, y + h, p);
		DrawLine(x, y + h, x, y, p);
	}

	void DrawStringShaded(const olc::vi2d pos, const std::string str, olc::Pixel p = olc::WHITE) 
	{
		DrawString(pos + olc::vi2d(1, 1), str, olc::BLACK);
		DrawString(pos, str, p);
	}

	void DrawStringShaded(int x, int y, const std::string str, olc::Pixel p = olc::WHITE)
	{
		DrawString(x + 1, y + 1, str, olc::BLACK);
		DrawString(x, y, str, p);
	}


	#pragma endregion

};

int main()
{
	Program game;
	if (game.Construct(640, 360, 2, 2, false))
		game.Start();

	return 0;
}