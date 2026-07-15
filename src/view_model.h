#pragma once
#include "model.h";
#include "raylib.h";
#include <array>
#include <vector>


/// <summary>coordinate of board cell displayed on screen. Agnostic of sides</summary>
struct tile
{
	int row{}, col{};
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
	model model;
	// for visible_piece_list, we want piece enum yet substitute with sq_id becuase squares are unique id for searches. 
	std::array<square_coord, 32> visible_piece_list;	// all pieces on board and not being animated (ex. pick up piece) 
	std::array<Texture2D, empty> textures;
	std::array<Vector2, MAX_QUEEN_MOVES> legals_list{};	// tiles of legal moves to highlight
	RenderTexture2D board;		// checkerboard
	Rectangle board_coord{};
	int piece_count{};		// count of visible pieces
	int legals_count{};		// legal moves to highlight
	int piece_length{};		// width/height of pieces
	int is_from_white{};		// true when view of board, white -> black == bottom -> top
	float to_compliment_coord_x{};
	float to_compliment_coord_y{}; 

	inline void show_last_hid_piece() { piece_count++; } // increments visible count
	void hide_pickup_piece(departure& operative);
	// updates view and model on move.
	void player_drops_piece(departure& operative, square_coord& destination);
	void back_move(departure& operative);

	void instantiate_legal_move_coords(BB moves);
	// draws squares into render texture
	void draw_board_grid_texture();
	void new_game(int is_from_white);

	/// <summary>
	/// sets pieces at starting position for white. Uses GPU
	/// </summary>
	/// <param name="textures">all pieces and squares to draw to VRAM</param>
	/// <param name="piece_length">size of pieces</param>
	/// <param name="isFromWhite">rotation of board_view, from the player who's pawns move bottom to top. Default black</param>
	view_model(std::array<Texture2D, empty> textures, int piece_length, int screen_width, int screen_height, int is_from_white);
	~view_model();

};

void mouse_to_square_coord(view_model& vm, Vector2 mouse_coord, square_coord& out_sq_coord);
Vector2 square_to_coord(view_model& vm, sq_id square);
