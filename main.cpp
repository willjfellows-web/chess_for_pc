// #include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raylib.h"
#include "load.h"
#include "view.h"

// TODO: 
// 1. replace back move with goto move
// 2. rename records enum vs struct


int main()
{
	// Tell the window to use vsync and work on high DPI displays
	// SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	// SearchAndSetResourceDir("resources");

	int screen_width = 1500;
	int screen_height = 900;
	InitWindow(screen_width, screen_height, "play chess"); // !ALWAYS initialize window BEFORE calling GPU
	int square_len = 64 + 20;

	session session;
	load_textures(session, square_len);
	load_user_data(session);

	// game list, search
	struct records records{};
	// board state, animate pieces, timers
	view_model vm = view_model(session, square_len, screen_width, screen_height, true);
	// pickup pieces
	departure operative{};


	float frame_rate = 20;
	SetTargetFPS(frame_rate);

	while (!WindowShouldClose())
	{
		session.mouse_coord = GetMousePosition();

		BeginDrawing();
		switch(session.page)
		{
			case seated:
				draw_board_view(&vm, session);
				do_play_buttons(&vm, operative, session);
				do_gamesetting_buttons(&vm, operative, session);
				if (session.games.back().score_sheet.size() == 2) 
					session.page = play;
				break;
			case play:
				vm.decrement_timer(session);
				draw_board_view(&vm, session);
				do_play_buttons(&vm, operative, session);
				do_gamesetting_buttons(&vm, operative, session);
				break;
			case analysis:
				draw_board_view(&vm, session);
				do_gamesetting_buttons(&vm, operative, session);
				break;
			case records:
				do_select_records(records, session);
				break;
		}
		EndDrawing();
	} // end window should close

	error("saving...");
	session_to_binary(session);
	error("saved.");
	
	error("======================================================================================================== program end");
	while (GetKeyPressed() != KEY_ESCAPE)
		;
}

