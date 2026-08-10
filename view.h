#pragma once
#include "view_model.h"
#include <stdlib.h>
#include <cstring>


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


struct ui_gameloop
{
	int text_size = 40;
	int right_hand_ui; 
	Rectangle new_game_btn_coord; 
	const char* new_game_btn_txt = "new game";
	Rectangle swap_sides_btn_coord;
	const char* swap_sides_btn_txt = "flip";
	Rectangle back_move_btn_coord;
	const char* back_move_btn_txt = "back";
	Rectangle console_coord;

	ui_gameloop(int screen_width, int screen_height, int piece_length) :
	text_size{ 40 },
	right_hand_ui { 18 * screen_width / 32 },
	new_game_btn_coord { 20*screen_width/32, screen_height / 2, 200, 100 },
	new_game_btn_txt { "new game" },
	swap_sides_btn_coord { 26*screen_width/32, screen_height / 2, 120, 120 },
	swap_sides_btn_txt { "flip" },
	back_move_btn_coord { 20*screen_width/32, 20*screen_height / 32, 100, 50 },
	back_move_btn_txt { "back" },
	console_coord{ screen_width*22/32, screen_height*10/32, 300, 50 }
	{
	}
};

struct promotion 
{
	#define MAX_PROMOTE_OPTION 4
	square_coord coord;
	Rectangle ribbon;
	int selected_tile_y{};
	int max_selection{};
	int min_selection{};
	int is_opposite_color_promoting{};
	float ribbon_direction{};
	promotion(float piece_length) : ribbon{ 0, 0, piece_length, piece_length*MAX_PROMOTE_OPTION } {}
};

void parse_algebraic_move(view_model& vm, departure& operative, std::array<char, 9>& input, int char_input_count);
