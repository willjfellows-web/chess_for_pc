#pragma once
#include "model.h"
#include "raylib.h"
#include <array>
#include <vector>
#include <cmath>
#include <ctime>

enum page
{
	seated,
	play,
	analysis,
	records,

	max_page
};

enum ui_element
{
	no_ui,

		// friendly: reserved active slot
	// engine_view_switch,

		// frail competitor: active slot, last in wins
	opponent_DDL,
	piece_drag,
	promoting_pawn_drag,
	piece_select,
	swap_side_btn, 

		// competitors: active slot, highest wins
	promoting,
	move_back_btn,

		// override: cancel all
	new_game_btn,
	//past_games_btn,
};

enum result
{
	no_result,
	draw,
	black_win,
	white_win
};

struct piece_coord
{
	piece piece{};
	float x{};
	float y{};
};

struct game_record
{
	opening opening = none_opening;
	victory_state win = safe;
	enum result result = no_result;

	unsigned short rating_black{}, rating_white{}, clock_total_seconds{};
	char id_black{}, id_white{}, year{}, month{}, day{};

	unsigned mothball_survivors_begin{};
	piece_coord* survivors;
	int survivors_count{};

	unsigned mothball_begin{};
	std::vector<half_move> score_sheet;
};


typedef unsigned short bin_move; // binary halfmove
struct session
{
	// checkerboard
	std::array<Texture2D, empty> textures;
	RenderTexture2D board;

	// binary data
	game_record* mothball_games;	// mothballed games metadata, including beginning index to moves and pieces
	unsigned games_count{};
	bin_move* mothball_moves;	// halfmoves
	unsigned moves_count{};
	piece_coord* mothball_survivors;	// final position
	unsigned survivors_count{};

	// zero initialize to remove all if (empty)
	// returning to play-page always creates a new game
	std::vector<game_record> games = { game_record() };	// metadata of session games

	// state id
	page page = seated;
	// mouse drags
	Vector2 mouse_coord;
	ui_element active = no_ui;
	ui_element hot = no_ui;

	void draw_board_grid_texture(int piece_length);
};

struct clock
{
	float countdown{};
	int previous_total_seconds{}; // determines if display update is required
	char display[5];
	clock()
	{
		int length = 5;
		display[length/2] = ':';
		display[length] = '\0';
	}
};

struct square_coord
{
	sq_id square{};
	Vector2 coord{}; // vector must be second element to match piece coord alignment
	square_coord(sq_id square, float x, float y) : coord{ x, y }, square{ square } {};
	square_coord() {};
};

struct view_model
{
	// true state of board. used by visible_piece_list to identify piece_type
	model model; 	
	// for drawing visible_piece_list, we want piece_type to draw, but substitute with sq_id becuase squares are unique id for searches. 
	// all pieces on board and not being animated (ex. pick up piece) 
	std::array<square_coord, 32> visible_piece_list;
	// count of visible pieces
	int piece_count{};
	// tiles of legal moves to highlight
	std::array<Vector2, MAX_QUEEN_MOVES> legals_list{};
	// legal moves to highlight
	int legals_count{};
	// color of legal moves highlight
	const Color piece_destinations = { 200, 60, 60, 128 };
	Rectangle board_coord; 

	
	void instantiate_legal_move_coords(BB moves);
	// draws squares into render texture
	inline void show_last_hid_piece() { piece_count++; } // increments visible count
	void hide_pickup_piece(departure& operative);
	// updates view and model for move.
	void player_drops_piece(departure& operative, square_coord& destination);
	// takeback
	void back_move(departure& operative, session& session);


	struct clock white{}; 
	struct clock black{};
	Rectangle top_timer_coord{};
	Rectangle bottom_timer_coord{};
	Rectangle* white_timer_coord;
	Rectangle* black_timer_coord;

	void decrement_timer(session& session);



	// Console
	std::array<char, 16> algebraic_input;
	int input_count{};
	Rectangle console_coord;



	// promotion
	#define MAX_PROMOTE_OPTION 4
	square_coord promote_coord;		// open menu under selected square
	Rectangle promote_ribbon;		// menu
	float promote_ribbon_direction{};	// lay menu towards board center 
	int promote_selected_tile_y{};
	int promote_max_selection{};		// alter menu for black/white
	int promote_min_selection{};
	int is_opposite_color_promoting{};



	// buttons
	int text_size = 40;
	Rectangle new_game_btn_coord; 
	const char new_game_btn_txt[9] = "new game";
	Rectangle old_game_btn_coord;
	const char old_game_btn_txt[9] = "old game";
	Rectangle swap_sides_btn_coord;
	const char swap_sides_btn_txt[5] = "flip";
	Rectangle back_move_btn_coord;
	const char back_move_btn_txt[5] = "back";



	// board instantiation
	float to_compliment_coord_x{};	// y axis at x == board_center. used for relecting piece positions
	float to_compliment_coord_y{};  // x axis at y == board_center. used for relecting piece positions
	float piece_length{};		// width/height of pieces
	int is_from_white{};		// is board displayed as white -> black, bottom -> top

	// also calls save_last_game()
	void new_game(session& session, int is_from_white, int player_minutes, int player_secs);
	// deletes games <= 3 moves, instantiates survivor positions
	// WARN: using piece list, and assumes no piece animations are playing!
	void save_last_game(session& session);

	/// <summary>
	/// sets pieces at starting position for white. Uses GPU
	/// </summary>
	/// <param name="textures">all pieces and squares to draw to VRAM</param>
	/// <param name="piece_length">size of pieces</param>
	/// <param name="isFromWhite">rotation of board_view, from the player who's pawns move bottom to top. Default black</param>
	view_model(session& session, float piece_length, int screen_width, int screen_height, int is_from_white);
};

void mouse_to_square_coord(view_model& vm, Vector2 mouse_coord, square_coord& out_sq_coord);
Vector2 square_to_coord(view_model& vm, sq_id square);
Vector2 square_to_coord(Vector2 board_coord, float piece_length, bool is_from_white, sq_id square);
void error(game_record& game);
