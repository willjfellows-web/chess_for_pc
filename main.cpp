// #include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raylib.h"
#include "model.h"
#include "load.h"
#include "view.h"

#define DEBUG false
#define SECS_PER_MIN 60


// TODO: 
// 1. fix place reverse capture piece crash
// 3. add boardstate hash for threefold repitition
// 4. conclude game
// 	a) determine standby mode 
// 	b) declare winner
// 	c) store game
// 8. reverse move_capture_promote duplicates promoting piece

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

	std::array<Texture2D, empty> textures = load_textures(square_len);

	ui_gameloop ui_gameloop(screen_width, screen_height, square_len); // right hand ui
	view_model vm = view_model(textures, square_len, screen_width, screen_height, true);
	promotion promotion(square_len);

	const Color piece_destinations = { 200, 60, 60, 128 };
	ui_element active = no_ui;
	ui_element hot = no_ui;
	departure operative{};
	Vector2 mouse_coord;

	// Console
	std::array<char, 16> algebraic_input;
	std::memset(algebraic_input.data(), ' ', sizeof(char)*algebraic_input.size());
	int char_input_count{};

	
	float frame_rate = 20;
	float remaining_time_max = 8;
	float time_passed = 1/frame_rate;
	float turn_time = 0.0;

	SetTargetFPS(frame_rate);
	while (!WindowShouldClose())
	{
		mouse_coord = GetMousePosition();

		BeginDrawing();
		ClearBackground(WHITE);

		// Draw Board
		DrawTexture(vm.board.texture, vm.board_coord.x, vm.board_coord.y, WHITE);
		for (int move{}; move < vm.legals_count; move++)
			DrawTextureV(vm.textures[white_square], vm.legals_list[move], piece_destinations);
		for (int i{}; i < vm.piece_count; i++)
		{
			square_coord& piece = vm.visible_piece_list[i];
			// if array subscript out of range, visuals_list pointed to empty piece
			enum piece type = vm.model.get(piece.square);
			DrawTextureV(vm.textures[type], piece.coord, WHITE);
		}

		// Timers
		int white_to_move = vm.model.score_sheet.size()%2;
		struct clock& defender = white_to_move ? vm.white : vm.black; // update defender during move
		defender.countdown -= GetFrameTime();
		// for float -> int, assume x64bit mode with instruction >= SSE2
		int total_seconds = (int)defender.countdown;
		if (defender.previous_total_seconds > total_seconds)
		{
			defender.previous_total_seconds = total_seconds;
			// quot[ient] = min, rem[ainder] = secs
			std::div_t mins_secs = std::div(total_seconds, SECS_PER_MIN); 

			// quot[ient] = second digit, rem[ainder] = first_digit
			std::div_t digit_pair = std::div(mins_secs.rem, BASE_TEN); 
			defender.display[4] = digit_pair.rem + '0';
			defender.display[3] = digit_pair.quot + '0';
			digit_pair = std::div(mins_secs.quot, BASE_TEN); 
			defender.display[1] = digit_pair.rem + '0';
			defender.display[0] = digit_pair.quot + '0';
		}

		DrawRectangleRec(vm.top_timer_coord, GRAY);
		DrawRectangleRec(vm.bottom_timer_coord, GRAY);
		DrawText(vm.white.display, vm.white_timer_coord->x, vm.white_timer_coord->y, ui_gameloop.text_size, BLACK);
		DrawText(vm.black.display, vm.black_timer_coord->x, vm.black_timer_coord->y, ui_gameloop.text_size, BLACK);


		// Console
		DrawRectangleRec(ui_gameloop.console_coord, PINK);
		DrawText(algebraic_input.data(), ui_gameloop.console_coord.x, ui_gameloop.console_coord.y, ui_gameloop.text_size, BLACK);
		// GUI
		DrawRectangleRec(ui_gameloop.new_game_btn_coord, BLUE);
		DrawText(ui_gameloop.new_game_btn_txt, ui_gameloop.new_game_btn_coord.x, ui_gameloop.new_game_btn_coord.y, ui_gameloop.text_size, BLACK);
		DrawRectangleRec(ui_gameloop.swap_sides_btn_coord, RED);
		DrawText(ui_gameloop.swap_sides_btn_txt, ui_gameloop.swap_sides_btn_coord.x, ui_gameloop.swap_sides_btn_coord.y, ui_gameloop.text_size, BLACK);
		DrawRectangleRec(ui_gameloop.back_move_btn_coord, GREEN);
		DrawText(ui_gameloop.back_move_btn_txt, ui_gameloop.back_move_btn_coord.x, ui_gameloop.back_move_btn_coord.y, ui_gameloop.text_size, BLACK);

		// back
		if (
			IsKeyPressed(KEY_LEFT)
			|| (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse_coord, ui_gameloop.back_move_btn_coord)))
		{
			// reset departure data generating legal moves
			operative.pin_count = 0;
			operative.opponent_check_blockables = ~((BB)0);
			operative.opponent_check_evadables = 0;
			vm.back_move(operative);
			active = no_ui;
		} // end back btn

		// swap sides
		else if (CheckCollisionPointRec(mouse_coord, ui_gameloop.swap_sides_btn_coord))
		{
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
			{
				active = no_ui;
				vm.is_from_white ^= 1;

				// how to find opposite coordinates on board
				// 1) row_#		= (piece.y - board_coord.y) / piece_length;
				// 2) row_compliment_#	= (ROWS - 1) - row_y
				// 3) row_compliment_y	= row_compliment_# * piece_length + board_coord.y;
				// 4) simplify: 
				
				for (int i{}; i < vm.piece_count; i++)
				{
					square_coord& piece = vm.visible_piece_list[i];
					piece.coord.y = vm.to_compliment_coord_y + vm.board_coord.y - piece.coord.y;
					piece.coord.x = vm.to_compliment_coord_x + vm.board_coord.x - piece.coord.x;
				}

				std::swap(vm.white_timer_coord, vm.black_timer_coord);
			}
		}

		// new_game
		else if (
			(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse_coord, ui_gameloop.new_game_btn_coord)))
		{
			active = no_ui;
			int player_minutes = 10;
			int player_secs = 10;
			vm.new_game(vm.is_from_white ^ 1, player_minutes, player_secs);
			operative.opponent_check_evadables = 0;
			operative.opponent_check_blockables = ~((BB)0);
			operative.pin_count = 0;
		}



		else if (active == promoting) // menu select promotion option 
		{
			if (CheckCollisionPointRec(mouse_coord, promotion.ribbon))
			{
				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
				{
					// normalize mouse coord to board tile coordinates
					float relative_y = mouse_coord.y - promotion.ribbon.y;
					float relative_tile_length = relative_y/vm.piece_length;
					float selection_tile = std::trunc(relative_tile_length);
					promotion.selected_tile_y = vm.piece_length*selection_tile + promotion.ribbon.y;

					// draw
					DrawRectangleRec(promotion.ribbon, WHITE);
					Vector2 ribbon_start = promotion.coord.coord;
					// display options images
					for (int selection = promotion.min_selection; selection <= promotion.max_selection; selection += 2)
					{
						DrawTexture(vm.textures[selection], ribbon_start.x, ribbon_start.y, WHITE);
						ribbon_start.y += promotion.ribbon_direction;
					}

					// highlight selection cursor
					DrawRectangleLines(promotion.ribbon.x, promotion.selected_tile_y, vm.piece_length, vm.piece_length, BLACK);
				}
				else
				{
					// option selected
					float round_up = 0.5;
					float relative_y = promotion.selected_tile_y - promotion.ribbon.y + round_up;
					// choose option 0-3
					int selection_tile = relative_y/vm.piece_length; 

					// bottom player ribbon displays from top down, top player from bottom up
					// rotate selected tile y for promoting player
					int to_compliment = MAX_PROMOTE_OPTION - 1;
					int selection
						= promotion.is_opposite_color_promoting 
						? to_compliment - selection_tile
						: selection_tile;

					int is_white_move = vm.model.score_sheet.size()%2;
					// enum piece: black_q, white_q, black_r, white_r, ect...
					// is_white_move selects color. Add selection twice to skip opposite color
					piece promoted = (piece)(black_queen + is_white_move + 2*selection);
					piece captured = vm.model.move_capture_promote(operative.piece, operative.from, promotion.coord.square, promoted);

					// promoting pawn already hidden from hide_pickup_piece()
					// int piece{ vm.piece_count };

					// assuming it is impossible to promote if no captures are made all game
					// let increment and decrement cancel out:
						// vm.show_last_hid_piece();	
						// visible_list[--vm.piece_count]

					// update view model
					secondary_effect advancement = (secondary_effect)(selection + promote_queen);
					vm.model.score_sheet.emplace_back(operative.from, promotion.coord.square, captured, advancement);
					// update pins, checks
					vm.model.instantiate_pins_checks(operative);

					active = no_ui;
					hot = no_ui;
					vm.legals_count = 0; // reset highlight
				} // end select promotion option
			}
			else
			{
				active = piece_drag;
			}
		} // end promotion


		// dropping piece outside the board is valid, even if illegal
		else if (active == piece_drag)
		{
			// draw piece
			Vector2 piece_center;
			piece_center.x = mouse_coord.x - vm.piece_length / 2.0;
			piece_center.y = mouse_coord.y - vm.piece_length / 2.0;
			DrawTextureV(vm.textures[operative.piece], piece_center, WHITE);

			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				if (operative.trigger_effect == promote_queen 
					&& CheckCollisionPointRec(mouse_coord, vm.board_coord))
				{
					// are we hovering over a promotion square?
					square_coord hovering; mouse_to_square_coord(vm, mouse_coord, hovering);
					bool is_hovering_promotion{ false };
					for (int i{}; i < operative.trigger_count; i++)
						is_hovering_promotion |= hovering.square == operative.trigger_ids[i];
					if (is_hovering_promotion)
					{
						// display menu
						active = promoting;

						// bottom player ribbon displays from top down, top player from bottom up
						promotion.coord = hovering;
						promotion.ribbon.x = hovering.coord.x;
						int white_to_move = vm.model.score_sheet.size() % 2;
						bool is_opposite_color_promoting = white_to_move ^ vm.is_from_white;
						promotion.ribbon.y 
							= is_opposite_color_promoting 
							? hovering.coord.y - (MAX_PROMOTE_OPTION - 1) * vm.piece_length
							: hovering.coord.y;
					}
				}
			}
			else 
			{
				// mouse button released
				square_coord dest; mouse_to_square_coord(vm, mouse_coord, dest); // dest == destination

				active = no_ui;
				vm.legals_count = 0;	// reset highlight
				// if illegal move 	reset piece 
				// else			renable piece to update piece view 
				vm.show_last_hid_piece();	

				bool is_legal_move
					= CheckCollisionPointRec(mouse_coord, vm.board_coord)
					&& operative.squares & ((BB)1 << dest.square);
				if (DEBUG || is_legal_move)
				{
					piece captured = vm.model.get(dest.square);
					// update view model
					vm.player_drops_piece(operative, dest);

					// swap turn to opponent perspective 
					vm.model.score_sheet.emplace_back(operative.from, dest.square, captured, operative.trigger_effect);

					// reset departure for generating legal moves
					operative.pin_count = 0;
					operative.opponent_check_blockables = ~((BB)0);
					operative.opponent_check_evadables = 0;
					victory_state result = vm.model.pins_checks_checkmate_stalemate(operative, dest.square);

					if (result == checkmate)
						;
					else if (result == stalemate)
						;
				} // end piece drop
			} // end mouse up || down
		}// end piece dragginng

		// pick up new piece
		else if (active != piece_drag
			&& CheckCollisionPointRec(mouse_coord, vm.board_coord) 
			&& IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			square_coord select; mouse_to_square_coord(vm, mouse_coord, select);
			operative.from = select.square;
			operative.piece = vm.model.get(operative.from);

			if (operative.piece != empty) 
			{
				active = piece_drag;
				vm.hide_pickup_piece(operative);

				// reset departure
				operative.squares = 0;
				operative.trigger_count = 0;
				operative.trigger_effect = none_secondary_effect;

				vm.model.get_legal_moves(operative);
				vm.instantiate_legal_move_coords(operative.squares); // if legal move count == 0, does nothing

				// prepare promoting
				if (operative.trigger_effect == promote_queen)
				{
					int is_white_move = vm.model.score_sheet.size()%2;
					promotion.max_selection = black_knight + is_white_move;
					promotion.min_selection = black_queen + is_white_move;
					promotion.is_opposite_color_promoting = is_white_move ^ vm.is_from_white;
					promotion.ribbon_direction = promotion.is_opposite_color_promoting ? -vm.piece_length : vm.piece_length;
				}
			} // end validate pick-up
		} // end pick up piece
		else
		{
			char pressed_ch = GetCharPressed();
			int pressed_ky = GetKeyPressed();
			if (pressed_ky != KEY_NULL)
			{
				if (pressed_ky == KEY_ENTER)
				{
					parse_algebraic_move(vm, operative, algebraic_input, char_input_count);

					std::memset(algebraic_input.data(), ' ', sizeof(char)*algebraic_input.size());
					char_input_count = 0;
				}
				else if (pressed_ky == KEY_BACKSPACE)
				{
					if (char_input_count > 0)
						algebraic_input[--char_input_count] = ' ';
				}
				if (pressed_ch >= ' ')
				{
					if (char_input_count < algebraic_input.size())
						algebraic_input[char_input_count++] = pressed_ch;
				}
			}
		}

		EndDrawing();
	} // end window should close
}
