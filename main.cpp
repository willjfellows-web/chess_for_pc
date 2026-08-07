// #include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raylib.h"
#include "model.h"
#include "load.h"
#include "view.h"
#include <cstring>
#define DEBUG true

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
	int selected_tile_y{};
	int max_selection{};
	int min_selection{};
	int is_opposite_color_promoting{};
	float ribbon_direction{};
	promotion(float piece_length) : ribbon{ 0, 0, piece_length, piece_length*MAX_PROMOTE_OPTION } {}
};


void parse_algebraic_move(view_model& vm, departure& operative, std::array<char, 9>& input, int char_input_count)
{
	// 1. [piece | piece-file | piece-rank | piece-square | pawn_square | square]
	// 2. [x]
	// 3. destination square
	// 4. [victory_status]
	// max length piece move ex. = "Ka1xa2+="
	// max length pawn promotion ex. = "b7xa8=Q+=" 

	// check trail
	// resolve draw offer
	//
	// check dest
	// validate dest
	// remove any one 'x'
	// bounds check
	// 	assess r/f specification
	// 	disambiguate b
	// switch piece
	// 	find potential departures
	// specify potential departures
	// filter wrong operative piece type
	// check departure ambiguity
	// filter illegal moves
	//
	// 	none: msg = out-of-bounds, continue
	// check castle
	// 	castle: q,k sides
	// 		partial: msg = illegal castle, return
	// 	none: msg = out-of-bounds, continue
	// check promotion
	// 	promote: pawn, return
	// 	none: msg = out-of-bounds, continue
	// return

	char trail = input[--char_input_count];
	char check = '+', checkmate = '#', draw_offer = '=';
	if (trail == draw_offer)
	{
		trail = input[--char_input_count];
	}
	if (trail != check && trail != checkmate)
	{
		char_input_count++; // include character in future reads
	}



	// Promote, Castle, piece/pawn -> coordinate

	// confirm we can access next characters
	const int min_algebraic_length = 2;
	if (char_input_count < min_algebraic_length)
	{
		error("too few inputs");
		return;
	}

	int white_to_move = vm.model.score_sheet.size()%2;

	// Promote
	secondary_effect any_promotion = none_secondary_effect;
	char last_char = input[--char_input_count];
	char second_last_char = input[--char_input_count];
	if (second_last_char == '=')
	{
		piece advancement;
		char advancement_key = last_char;
		// promotion, proceed with pawn move
		switch(advancement_key)
		{
			case 'q': advancement = black_queen; break;
			case 'r': advancement = black_rook; break;
			case 'b': advancement = black_bishop; break;
			case 'n': advancement = black_knight; break;
			default:
				error("not promotion. input was likely out-of-bounds destination coord or garbage");
				break;
		}
		int black_pieces_are_even = 2;
		int to_queen_zero_initialized = 1;
		int advancement_id = advancement/black_pieces_are_even - to_queen_zero_initialized;
		any_promotion = (secondary_effect)(advancement_id + promote_queen);
		error(any_promotion, "any_promotion");

		// confirm we can access next characters
		if (char_input_count < min_algebraic_length)
		{
			error("too few inputs");
			return;
		}
		// get next characters
		last_char = input[--char_input_count];
		second_last_char = input[--char_input_count];
	}

	constexpr char to_lower_mask = 'a' ^ 'A';

	// destination coords
	char rank = last_char;
	char file = second_last_char;
	if (rank >= '1' && rank <= '8'
		&& file >= 'a' && file <= 'h')
	{
		// is piece/pawn -> coordinate
		error("invalid destination range");
		int rank_num = rank - '1';
		int file_num = file - 'a';
		sq_id dest = ROWS*rank_num + file_num;
		BB bb_dest = (BB)1 << dest;

		// remove any capture specification
		if (char_input_count > 0)
		{
			char maybe_capture = input[char_input_count - 1]; // wait to skip
			char lower_capture = maybe_capture | to_lower_mask;
			if (lower_capture == 'x')
			{
				char_input_count--; // ignore
			}
		}

		// Parse departure


		enum disambiguate_b
		{
			bishop_move,
			pawn_move,
		};
		disambiguate_b disambiguate_b = bishop_move;

		// rank/file specification for departure
		BB departure_options = (BB)0;
		const BB first_file = 0x0101010101010101;
		const BB first_rank = 0x00000000000000FF;

		// this switch does three things
		// 1. parse and rank/file specification 
		// 2. decrement char_input_count until specification end
		// 3. disambiguate bishops and pawn-bfile 
		char current;
		const char bishop_key = 'b';
		switch (char_input_count)
		{
			case 0:
				disambiguate_b = pawn_move;
				break;
			case 1:
				// piece move, pawn-[file | rank]
				// ex) ka1, aa1, 1a1
				current = input[--char_input_count];
				if (current >= '1' && current <= '8')
				{
					// pawn-rank
					disambiguate_b = pawn_move;
					int specified_rank = current - '1';
					int rank_index_delta = specified_rank*ROWS;
					departure_options |= first_rank << rank_index_delta;
				}
				else if (current == bishop_key)
				{
					error("OK bishop move");
				}
				else if (current >= 'a' && current <= 'h')
				{
					// piece move, pawn-file
					disambiguate_b = pawn_move;
					int specified_file = current - 'a';
					departure_options |= first_file << specified_file;
					print_bb(departure_options, "departure_options");
				}
				else
				{
					error("OK piece move");
				}
				break;
			case 2:
				// piece-[file | rank], pawn-square
				// ex) kaa1, k1a1, a2a1
				current = input[--char_input_count];
				if (current >= '1' && current <= '8')
				{
					int specified_rank = current - '1';
					int rank_index_delta = specified_rank*ROWS;
					departure_options |= first_rank << rank_index_delta;

					current = input[--char_input_count]; 
					if (current == bishop_key)
					{
						error("OK bishop move");
					}
					else if (current >= 'a' && current <= 'h')
					{
						disambiguate_b = pawn_move;
						int specified_file = current - 'a';
						departure_options |= first_file << specified_file;
					}
					else
					{
						// OK piece move
						error("OK piece move");
					}
				}
				else if (current >= 'a' && current <= 'h')
				{
					// piece-file ex) kaa1
					int specified_file = current - 'a';
					departure_options |= first_file << specified_file;
				}
				else
				{
					error("error file specification out-of-bounds (a-h)");
				}
				break;
			case 3:
				// piece-square
				{
					current = input[--char_input_count];
					int specified_rank = current - '1';
					current = input[--char_input_count];
					int specified_file = current - 'a';
					if (specified_rank < 1 || specified_rank > 8
						|| specified_file < a || specified_file > h)
					{
						error("invalid piece departure range");
					}
					else
					{
						int rank_index_delta = specified_rank*ROWS;
						departure_options |= first_rank << rank_index_delta;
						departure_options |= first_file << specified_file;
					}
				}

				break;
			default:
				error("too many departure description characters error");
				break;
		}


		operative.trigger_effect = none_secondary_effect; 
		BB& departures = operative.squares;
		BB checks_to_block = operative.opponent_check_blockables; // bitwise (1) if king selected
		piece piece_color { empty };
		char first = input[0]; 
		first |= to_lower_mask;
		BB occupied = ~vm.model.get_BB(empty);
		switch (first)
		{
			case 'k':
				checks_to_block = (BB)-1; // don't block check with king 
				bb_dest &= ~operative.opponent_check_evadables;	// don't move into check
				departures = vm.model.king_moves_standard(bb_dest);
				piece_color = (piece)(black_king + white_to_move);
				break;
			case 'q':
				piece_color = (piece)(black_queen + white_to_move);
				vm.model.prepare_bishop_sliding(dest);
				departures = vm.model.execute_slide(north, max_orient, dest, occupied);
				break;
			case 'r':
				piece_color = (piece)(black_rook + white_to_move);
				departures = vm.model.execute_slide(north, north_east, dest, occupied);
				break;
			case 'b':
				// bishop, pawn, pawn-file, pawn-square
				if (disambiguate_b == bishop_move)
				{
					piece_color = (piece)(black_bishop + white_to_move);
					vm.model.prepare_bishop_sliding(dest);
					departures = vm.model.execute_slide(north_east, max_orient, dest, occupied);
				}
				else if (white_to_move)
				{
					piece_color = white_pawn;
					vm.model.white_pawn_departure_from_dest(operative, dest, any_promotion);
				}
				else
				{
					piece_color = black_pawn;
					vm.model.black_pawn_departure_from_dest(operative, dest, any_promotion);
				}
				break;
			case 'n':
				piece_color = (piece)(black_knight + white_to_move);
				departures = vm.model.knight_moves(bb_dest);
				break;
			default:
				// capture-pawn
				// pawn-file, pawn-rank, pawn-square
				if (white_to_move)
				{
					piece_color = white_pawn;
					vm.model.white_pawn_departure_from_dest(operative, dest, any_promotion);
				}
				else
				{
					piece_color = black_pawn;
					vm.model.black_pawn_departure_from_dest(operative, dest, any_promotion);
				}
				break;
		}


		print_bb(departures, "departures pre options");
		// apply user file/rank specification
		if (departure_options)
		{
			print_bb(departure_options, "departure_options");
			departures &= departure_options;
		}
		// get piece(s) to depart
		departures &= vm.model.get_BB(piece_color);
		print_bb(departures, "departures has piece");

		// we seek the one-and-only option
		if (departures)
		{
			sq_id index = bit_scan_forward(departures);
			BB zero_least_significant_bit = departures - 1;
			BB second_departure = departures & zero_least_significant_bit;
			if (second_departure)
			{
				// error: too many possible departures
				error("error: too many possible departures");
			}
			else
			{
				// Found departure & destination!
				operative.from = index; 
				operative.piece = vm.model.get(index);

				// Remove Illegal Moves

				piece piece = vm.model.get(operative.from);
				int white_piece_selected = piece % 2;
				if (white_to_move != white_piece_selected)
				{
					error("opponent color selected");
					return;
				}

				// destination can't open check
				for (int pin{}; pin < operative.pin_count; pin++)
					if (operative.from == operative.pinned_id[pin])
						operative.squares &= operative.pin_threat_line[pin];
				// block any checks (if king switch, bitwise one)
				bb_dest &= checks_to_block;
				// no friendly fire
				BB friendly_pieces = vm.model.get_BB(black_square + white_to_move);
				bb_dest &= ~friendly_pieces; 
				if (bb_dest)
				{
					square_coord dest_coord;
					dest_coord.square = dest;
					dest_coord.coord = square_to_coord(vm, dest);
					operative.squares = bb_dest; // share attack squares with view_model

					error(operative.piece, "operative.piece");
					error(operative.from, "operative.from");
					print_bb(operative.squares, "operative.squares");

					// Place move
					// updates operative.trigger_effect before adding to score_sheet
					vm.player_drops_piece(operative, dest_coord); 

					// swap turn to opponent perspective 
					enum piece captured = vm.model.get(dest);
					vm.model.score_sheet.emplace_back(operative.from, dest, captured, operative.trigger_effect);

					// reset departure for generating legal moves
					operative.pin_count = 0;
					operative.opponent_check_blockables = ~((BB)0);
					operative.opponent_check_evadables = 0;
					victory_state result = vm.model.pins_checks_checkmate_stalemate(operative, dest);

					if (result == checkmate)
						;
					else if (result == stalemate)
						;
				}
				else
				{
					error("error: move is illegal");
					return;
				}
			} // end move legality
		} // end has possible departure
		else
		{
			// error no departure
			error("error no departure");
		}
	}
	else
	{
		// Castle
		error("error destination coordinate out-of-bounds (a-h)");
		error("checking castle");

		const char* kingside_castle_key = "0-0";
		int kingside_castle_char_count = 3;
		if (std::strncmp(kingside_castle_key, input.data(), kingside_castle_char_count) == 0)
		{
			int move_distance; // vector describing squares king moves to castle

			const char* queenside_castle_key = "0-0-0";
			int queenside_castle_char_count = 5;
			if (std::strncmp(queenside_castle_key, input.data(), queenside_castle_char_count) == 0)
			{
				// is queenside castle
				error("is queenside castle");
				move_distance = -2;
			}
			else 
			{
				// is kingside castle
				error("is kingside castle");
				move_distance = 2;
			}

			int white_to_move{ vm.model.score_sheet.size() % 2 };
			int black_to_move{ white_to_move ^ 1 };
			int home_rank = black_to_move * (ROWS - 1) * COLS;
			sq_id king_home_square = home_rank + e;
			sq_id castle_dest = king_home_square + move_distance;

			operative.piece = vm.model.get(king_home_square);
			// operative.squares = 0;  	// not required to assess castle rights
			operative.from = king_home_square; // the data needed to assess castle rights
			// if castle is legal, no other legal moves need consideration
			vm.model.get_any_castle(operative);

			bool is_castle_legal{false};
			for (sq_id sq{}; sq < operative.trigger_count; sq++)
				is_castle_legal |= operative.trigger_ids[sq] == castle_dest;
				
			if (is_castle_legal)
			{
				square_coord dest_coord;
				dest_coord.square = castle_dest;
				dest_coord.coord = square_to_coord(vm, castle_dest);

				// Place move
				vm.player_drops_piece(operative, dest_coord); // call before adding to history so operative.trigger_effect updates

				// swap turn to opponent perspective 
				vm.model.score_sheet.emplace_back(operative.from, castle_dest, empty, operative.trigger_effect);

				// reset departure for generating legal moves
				operative.pin_count = 0;
				operative.opponent_check_blockables = ~((BB)0);
				operative.opponent_check_evadables = 0;
				victory_state result = vm.model.pins_checks_checkmate_stalemate(operative, castle_dest);

				if (result == checkmate)
					;
				else if (result == stalemate)
					;
			}
		} // end castle
		else
		{
			error("error destination coordinate out-of-bounds or garbage");
		} // end is castle 
	} // end is pawn/piece move | castle | promotion

	// TODO:
// 11. alg_notation error messages
// 12. alg_notation screen position
} // parse_algebraic_move

// TODO: 
// 1. fix place reverse capture piece crash
// 2. add timer
// 3. add boardstate hash for threefold repitition
// 4. conclude game
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
	int square_len = 64 + 32;

	std::array<Texture2D, empty> textures = load_textures(square_len);

	ui_gameloop ui_gameloop(screen_width, screen_height); // right hand ui
	view_model vm = view_model(textures, square_len, screen_width, screen_height, true);
	promotion promotion(square_len);

	const Color piece_destinations = { 200, 60, 60, 128 };
	ui_element active = no_ui;
	ui_element hot = no_ui;
	departure operative{};
	Vector2 mouse_coord;

	// 1. [piece | piece-file | piece-rank | piece-square | pawn_square]
	// 2. [x]
	// 3. destination square
	// 4. [victory_status]
	// max algebraic notation = "Ka1xa2+" = 7 characters
	std::array<char, 9> algebraic_input;
	std::memset(algebraic_input.data(), ' ', sizeof(char)*algebraic_input.size());
	int char_input_count{};
	char char_pressed;

	
	float frame_rate = 20;
	float time_passed = 1/frame_rate;
	float turn_time = 0.0;
	SetTargetFPS(frame_rate);
	while (!WindowShouldClose())
	{
		turn_time += time_passed;
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

		// Draw UI
		const static Rectangle timer_coord{ screen_height/2, screen_width/2, 100,100  };
		DrawRectangleRec(timer_coord, GRAY);
		// std::string test = std::to_string(turn_time);
		// const char* timer_text = test.data();
		// DrawText(timer_text, timer_coord.x, timer_coord.y, ui_gameloop.text_size, BLACK);

		DrawText(algebraic_input.data(), timer_coord.x, timer_coord.y, ui_gameloop.text_size, BLACK);

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

		// new_game
		else if (
			(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse_coord, ui_gameloop.new_game_btn_coord)))
		{
			active = no_ui;
			vm.new_game(vm.is_from_white ^ 1);
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
