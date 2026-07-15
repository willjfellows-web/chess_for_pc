// #include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raylib.h"
#include "model.h"
#include "load.h"
#include "view.h"
// #include <cstring>
#include <string>

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

	ui_gameloop(int screen_width, int screen_height) :
	text_size{ 40 },
	right_hand_ui { 18 * screen_width / 32 },
	new_game_btn_coord { 20*screen_width/32, screen_height / 2, 200, 100 },
	new_game_btn_txt { "new game" },
	swap_sides_btn_coord { 26*screen_width/32, screen_height / 2, 120, 120 },
	swap_sides_btn_txt { "flip" },
	back_move_btn_coord { 20*screen_width/32, 20*screen_height / 32, 100, 50 },
	back_move_btn_txt { "back" }
	{
	}
};

struct promotion 
{
	#define MAX_PROMOTE_OPTION 4
	square_coord coord;
	Rectangle ribbon;
	int selection{};

	promotion(int piece_length) :
	ribbon{ 0, 0, MAX_PROMOTE_OPTION * piece_length, piece_length }
	{
	}
};

// TODO: 
// 1. stop backmove from crashing
// 2. add timer
// 3. add boardstate hash for threefold repitition
// 4. conclude game

int main()
{
	// Tell the window to use vsync and work on high DPI displays
	// SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	// SearchAndSetResourceDir("resources");

	int screen_width = 1500;
	int screen_height = 900;
	InitWindow(screen_width, screen_height, "play chess"); // !ALWAYS initialize window BEFORE calling GPU
	int square_len = 64 + 32;

	std::array<Texture2D, empty> textures = load_textures(square_len);

	ui_gameloop ui_gameloop(screen_width, screen_height); // right hand ui
	view_model vm = view_model(textures, square_len, screen_width, screen_height, true);
	promotion promotion(square_len);
		
	const Color piece_destinations = { 200, 60, 60, 128 };
	ui_element active = no_ui;
	departure operative{};
	Vector2 mouse_coord;

	
	float frame_rate = 20;
	float time_passed = 1/frame_rate;
	float turn_time = 0.0;
	SetTargetFPS(frame_rate);
	while (!WindowShouldClose())
	{
		turn_time += time_passed;
		mouse_coord = GetMousePosition();


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

		else if (CheckCollisionPointRec(mouse_coord, ui_gameloop.swap_sides_btn_coord))
		{
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
			{
				active = no_ui;
				vm.is_from_white = vm.is_from_white ^ 1;

				// how to find opposite coordinates on board
				// 1) row_#		= (piece.y - board_coord.y) / piece_length;
				// 2) row_compliment_#	= (ROWS - 1) - row_y
				// 3) row_compliment_y	= row_compliment_# * piece_length + board_coord.y;
				// 4) simplify: 
				
				for (int i{}; i < vm.piece_count; i++)
				{
					square_coord& piece = vm.visible_piece_list[i];
					piece.coord.y = vm.to_compliment_coord_y - piece.coord.y;
					piece.coord.x = vm.to_compliment_coord_x - piece.coord.x;
				}
			}
		}

		else if (
			IsKeyPressed(KEY_N)
			|| (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse_coord, ui_gameloop.new_game_btn_coord)))
		{
			vm.new_game(vm.is_from_white ^ 1);
			active = no_ui;
		}

		// dropping piece outside the board is valid, even if illegal
		else if (active == piece_drag && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) // drop piece
		{
			square_coord dest; mouse_to_square_coord(vm, mouse_coord, dest); // dest == destination

			active = no_ui;
			vm.legals_count = 0;	// reset highlight
			// if illegal move 	reset piece 
			// else			renable piece to update piece view 
			vm.show_last_hid_piece();	

			bool is_legal_move
				= CheckCollisionPointRec(mouse_coord, vm.board_coord)
				&& operative.squares & ((BB)1 << dest.square);
			if (is_legal_move)
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
			}
		}// end piece drop


		else if (active == promoting) // select promotion
		{
			if (CheckCollisionPointRec(mouse_coord, promotion.ribbon))
			{

				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
					promotion.selection = (mouse_coord.x - promotion.ribbon.x) / vm.piece_length;
				else
				{

					int is_white_move = vm.model.score_sheet.size() % 2;
					// enum piece: black_q, white_q, black_r, white_r, ect...
					// is_white_move selects color. Add selection twice to skip opposite color
					piece promoted = (piece)(black_queen + is_white_move + 2*promotion.selection);
					piece captured = vm.model.move_capture_promote(operative.piece, operative.from, promotion.coord.square, promoted);

					// piece always already hidden from hide_pickup_piece()
					// int piece{ vm.piece_count };

					// assuming it is impossible to promote if no captures are made all game
					// let increment and decrement cancel out:
							// vm.show_last_hid_piece();	
							// visible_list[--vm.piece_count]

					// update view model
					secondary_effect advancement = (secondary_effect)(promotion.selection + promote_queen);
					vm.model.score_sheet.emplace_back(operative.from, promotion.coord.square, captured, advancement);

					active = no_ui;
					vm.legals_count = 0; // reset highlight
				}
			}
			else
				active = piece_drag;
		} // end promotion


		else if (active == piece_drag
			&& IsMouseButtonDown(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse_coord, vm.board_coord))  // piece in hand
		{
			if (operative.trigger_effect == promote_queen)
			{
				// are we hovering over a promotion square?
				square_coord hovering; mouse_to_square_coord(vm, mouse_coord, hovering);
				bool is_hovering_promotion{ false };
				for (int i{}; i < operative.trigger_count; i++)
					is_hovering_promotion |= hovering.square == operative.trigger_ids[i];
				if (is_hovering_promotion)
				{
					promotion.coord = hovering;
					promotion.ribbon.x = hovering.coord.x;
					promotion.ribbon.y = hovering.coord.y;
					active = promoting;
				}
			}
		}


		// pick up new piece
		else if (active != piece_drag
			&& CheckCollisionPointRec(mouse_coord, vm.board_coord) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			square_coord select;
			mouse_to_square_coord(vm, mouse_coord, select);
			operative.from = select.square;
			operative.piece = vm.model.get(operative.from);

			// reset departure
			operative.squares = 0;
			operative.trigger_count = 0;
			operative.trigger_effect = none_secondary_effect;
			vm.model.get_legal_moves(operative);

			if (operative.squares)
			{
				vm.instantiate_legal_move_coords(operative.squares);
				vm.hide_pickup_piece(operative);
				active = piece_drag;
			}
		} // end pick up piece

		BeginDrawing();

		ClearBackground(WHITE);
		DrawTexture(vm.board.texture, vm.board_coord.x, vm.board_coord.y, WHITE);

		for (int move{}; move < vm.legals_count; move++)
			DrawTextureV(vm.textures[white_square], vm.legals_list[move], piece_destinations);

		// ;
		for (int i{}; i < vm.piece_count; i++)
		{
			square_coord& piece = vm.visible_piece_list[i];
			// if array subscript out of range, visuals_list pointed to empty piece
			enum piece type = vm.model.get(piece.square);
			DrawTextureV(vm.textures[type], piece.coord, WHITE);
		}

		if (active == piece_drag)
		{
			Vector2 piece_center;
			piece_center.x = mouse_coord.x - vm.piece_length / 2;
			piece_center.y = mouse_coord.y - vm.piece_length / 2;
			DrawTextureV(vm.textures[operative.piece], piece_center, WHITE);
		}

		else if (active == promoting)
		{
			Vector2 ribbon_start = promotion.coord.coord;
			DrawRectangleRec(promotion.ribbon, WHITE);
			int is_white_move = vm.model.score_sheet.size() % 2;
			int max_selection = black_knight + is_white_move;
			for (int selection{ black_queen + is_white_move }; selection <= max_selection; selection += 2)
			{
				DrawTexture(vm.textures[selection], ribbon_start.x, ribbon_start.y, WHITE);
				ribbon_start.x += vm.piece_length;
			}
			// highlight selection cursor
			DrawRectangleLines(promotion.ribbon.x + promotion.selection * vm.piece_length, promotion.ribbon.y, vm.piece_length, vm.piece_length, BLACK);
		}

		// draw ui
		// DrawRectangleRec(timer_coord, GRAY);
		// Rectangle timer_coord{ screen_height/2, screen_width/2, 100,100  };
		// std::string test = std::to_string(turn_time);
		// const char* timer_text = test.data();
		// DrawText(timer_text, timer_coord.x, timer_coord.y, ui_gameloop.text_size, BLACK);

		DrawRectangleRec(ui_gameloop.new_game_btn_coord, BLUE);
		DrawText(ui_gameloop.new_game_btn_txt, ui_gameloop.new_game_btn_coord.x, ui_gameloop.new_game_btn_coord.y, ui_gameloop.text_size, BLACK);
		DrawRectangleRec(ui_gameloop.swap_sides_btn_coord, RED);
		DrawText(ui_gameloop.swap_sides_btn_txt, ui_gameloop.swap_sides_btn_coord.x, ui_gameloop.swap_sides_btn_coord.y, ui_gameloop.text_size, BLACK);
		DrawRectangleRec(ui_gameloop.back_move_btn_coord, GREEN);
		DrawText(ui_gameloop.back_move_btn_txt, ui_gameloop.back_move_btn_coord.x, ui_gameloop.back_move_btn_coord.y, ui_gameloop.text_size, BLACK);

		EndDrawing();
	} // end window should close
}
