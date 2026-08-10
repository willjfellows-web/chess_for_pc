#pragma once
#include "model.h";
#include "raylib.h";
#include <array>
#include <vector>
#include <cmath>
#define BASE_TEN 10


struct clock
{
	float countdown{};
	int previous_total_seconds{}; // determines if display update is required
	char* display;
	clock()
	{
		int length = 5;
		display = (char*)malloc(length*sizeof(char));
		display[length/2] = ':';
		display[length] = '\0';
	}
	~clock() { delete[] display; }
};

struct square_coord
{
	Vector2 coord{};
	sq_id square{};
	square_coord(sq_id square, float x, float y) : coord{ x, y }, square{ square } {};
	square_coord() {};
};

class view_model
{
public:
	model model; 	// true state of board. used by visible_piece_list to identify piece_type
	// for drawing visible_piece_list, we want piece_type to draw, but substitute with sq_id becuase squares are unique id for searches. 
	std::array<square_coord, 32> visible_piece_list;	// all pieces on board and not being animated (ex. pick up piece) 
	std::array<Texture2D, empty> textures;
	std::array<Vector2, MAX_QUEEN_MOVES> legals_list{};	// tiles of legal moves to highlight
	RenderTexture2D board;		// checkerboard

	struct clock white{}; 
	struct clock black{};
	Rectangle top_timer_coord{};
	Rectangle bottom_timer_coord{};
	Rectangle board_coord{};
	Rectangle* white_timer_coord;
	Rectangle* black_timer_coord;

	int piece_count{};		// count of visible pieces
	int legals_count{};		// legal moves to highlight
	float piece_length{};		// width/height of pieces
	int is_from_white{};		// is board displayed as white -> black, bottom -> top
	float to_compliment_coord_x{};	// y axis at x == board_center. used for relecting piece positions
	float to_compliment_coord_y{};  // x axis at y == board_center. used for relecting piece positions

	inline void show_last_hid_piece() { piece_count++; } // increments visible count
	void hide_pickup_piece(departure& operative);
	// updates view and model on move.
	void player_drops_piece(departure& operative, square_coord& destination);
	void back_move(departure& operative);

	void instantiate_legal_move_coords(BB moves);
	// draws squares into render texture
	void draw_board_grid_texture();
	void new_game(int is_from_white, int player_minutes, int player_secs);

	/// <summary>
	/// sets pieces at starting position for white. Uses GPU
	/// </summary>
	/// <param name="textures">all pieces and squares to draw to VRAM</param>
	/// <param name="piece_length">size of pieces</param>
	/// <param name="isFromWhite">rotation of board_view, from the player who's pawns move bottom to top. Default black</param>
	view_model(std::array<Texture2D, empty> textures, float piece_length, int screen_width, int screen_height, int is_from_white);
	~view_model();
};

void mouse_to_square_coord(view_model& vm, Vector2 mouse_coord, square_coord& out_sq_coord);
Vector2 square_to_coord(view_model& vm, sq_id square);
