#include "raylib.h"

struct records
{
	static constexpr Vector2 img_begin{ 600, 100 };
	static constexpr float piece_len{32};
	float scroll_distance{}; // down > up
	unsigned selection{};
	static constexpr int load_size{8};
	int loaded_offset{};
};
