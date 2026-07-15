#pragma once
#include "view_model.h"


enum ui_element
{
	no_ui,

		// friendly: reserved active slot
	// engine_view_switch,

		// frail competitor: active slot, last in wins
	opponent_DDL,
	piece_drag,
	piece_select,
	swap_side_btn, 

		// competitors: active slot, highest wins
	promoting,
	move_back_btn,

		// override: cancel all
	new_game_btn,
	//past_games_btn,
};

