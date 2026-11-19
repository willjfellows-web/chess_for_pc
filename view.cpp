#include "view.h"



// always uses last game score_sheet to determine move legality
void parse_algebraic_move(view_model& vm, departure& operative, std::array<char, 16>& input, int char_input_count, session& session)
{
	// 1. [piece | piece-file | piece-rank | piece-square | pawn_square | square]
	// 2. [x]
	// 3. destination square
	// 4. [victory_status]
	// max length piece move ex. = "Ka1xa2+="
	// max length pawn promotion ex. = "b7xa8=Q+=" 


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


	// confirm we can access next characters
	const int min_algebraic_length = 2;
	if (char_input_count < min_algebraic_length)
	{
		error("too few inputs");
		return;
	}

	// token maybe Promote, Castle, piece/pawn -> coordinate, else garbage
	int white_to_move = vm.model.white_to_move;

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
					error(departure_options, "departure_options");
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
				departures = vm.model.king_moves_standard(bb_dest); // castle handled by "0-0-0"
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
					vm.model.white_pawn_departure_from_dest(operative, dest, any_promotion, session.games.back().score_sheet);
				}
				else
				{
					piece_color = black_pawn;
					vm.model.black_pawn_departure_from_dest(operative, dest, any_promotion, session.games.back().score_sheet);
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
					vm.model.white_pawn_departure_from_dest(operative, dest, any_promotion, session.games.back().score_sheet);
				}
				else
				{
					piece_color = black_pawn;
					vm.model.black_pawn_departure_from_dest(operative, dest, any_promotion, session.games.back().score_sheet);
				}
				break;
		}


		error(departures, "departures pre options");
		// apply user file/rank specification
		if (departure_options)
		{
			error(departure_options, "departure_options");
			departures &= departure_options;
		}
		// get piece(s) to depart
		departures &= vm.model.get_BB(piece_color);
		error(departures, "departures has piece");

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
					error(operative.squares, "operative.squares");

					// Place move
					// updates operative.trigger_effect before adding to score_sheet
					vm.player_drops_piece(operative, dest_coord); 

					session.games.back().score_sheet.emplace_back(operative.from, dest, operative.trigger_effect);
					// swap turn to opponent perspective 
					vm.model.white_to_move ^= 1;

					// reset departure for generating legal moves
					operative.pin_count = 0;
					operative.opponent_check_blockables = ~((BB)0);
					operative.opponent_check_evadables = 0;
					victory_state result = vm.model.pins_checks_checkmate_stalemate(operative, dest, session.games.back().score_sheet);

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

			int white_to_move{ vm.model.white_to_move };
			int black_to_move{ white_to_move ^ 1 };
			int home_rank = black_to_move * (ROWS - 1) * COLS;
			sq_id king_home_square = home_rank + e;
			sq_id castle_dest = king_home_square + move_distance;

			operative.piece = vm.model.get(king_home_square);
			// operative.squares = 0;  	// not required to assess castle rights
			operative.from = king_home_square; // the data needed to assess castle rights
			// if castle is legal, no other legal moves need consideration
			vm.model.get_any_castle(operative, session.games.back().score_sheet);

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

				session.games.back().score_sheet.emplace_back(operative.from, castle_dest, operative.trigger_effect);
				// swap turn to opponent perspective 
				vm.model.white_to_move ^= 1;

				// reset departure for generating legal moves
				operative.pin_count = 0;
				operative.opponent_check_blockables = ~((BB)0);
				operative.opponent_check_evadables = 0;
				victory_state result = vm.model.pins_checks_checkmate_stalemate(operative, castle_dest, session.games.back().score_sheet);

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


void draw_board_view(view_model* vm, session& session)
{
	ClearBackground(WHITE);

	// Draw Board
	DrawTexture(session.board.texture, vm->board_coord.x, vm->board_coord.y, WHITE);
	for (int move{}; move < vm->legals_count; move++)
		DrawTextureV(session.textures[white_square], vm->legals_list[move], vm->piece_destinations);
	for (int i{}; i < vm->piece_count; i++)
	{
		square_coord& piece = vm->visible_piece_list[i];
		// if array subscript out of range, visuals_list pointed to empty piece
		enum piece type = vm->model.get(piece.square);
		DrawTextureV(session.textures[type], piece.coord, WHITE);
	}

	// Timers
	DrawRectangleRec(vm->top_timer_coord, GRAY);
	DrawRectangleRec(vm->bottom_timer_coord, GRAY);
	DrawText(vm->white.display, vm->white_timer_coord->x, vm->white_timer_coord->y, vm->text_size, BLACK);
	DrawText(vm->black.display, vm->black_timer_coord->x, vm->black_timer_coord->y, vm->text_size, BLACK);

	// Console
	DrawRectangleRec(vm->console_coord, PINK);
	DrawText(vm->algebraic_input.data(), vm->console_coord.x, vm->console_coord.y, vm->text_size, BLACK);
	// GUI
	DrawRectangleRec(vm->new_game_btn_coord, BLUE);
	DrawText(vm->new_game_btn_txt, vm->new_game_btn_coord.x, vm->new_game_btn_coord.y, vm->text_size, BLACK);
	DrawRectangleRec(vm->old_game_btn_coord, PURPLE);
	DrawText(vm->old_game_btn_txt, vm->old_game_btn_coord.x, vm->old_game_btn_coord.y, vm->text_size, BLACK);
	DrawRectangleRec(vm->swap_sides_btn_coord, RED);
	DrawText(vm->swap_sides_btn_txt, vm->swap_sides_btn_coord.x, vm->swap_sides_btn_coord.y, vm->text_size, BLACK);
	DrawRectangleRec(vm->back_move_btn_coord, GREEN);
	DrawText(vm->back_move_btn_txt, vm->back_move_btn_coord.x, vm->back_move_btn_coord.y, vm->text_size, BLACK);
}

void do_gamesetting_buttons(view_model* vm, departure& operative, session& session)
{
	// swap sides
	if (CheckCollisionPointRec(session.mouse_coord, vm->swap_sides_btn_coord))
	{
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
		{
			session.active = no_ui;
			vm->is_from_white ^= 1;

			// how to find opposite coordinates on board
			// 1) row_#		= (piece.y - board_coord.y) / piece_length;
			// 2) row_compliment_#	= (ROWS - 1) - row_y
			// 3) row_compliment_y	= row_compliment_# * piece_length + board_coord.y;
			// 4) simplify: 
			
			for (int i{}; i < vm->piece_count; i++)
			{
				square_coord& piece = vm->visible_piece_list[i];
				piece.coord.y = vm->to_compliment_coord_y + vm->board_coord.y - piece.coord.y;
				piece.coord.x = vm->to_compliment_coord_x + vm->board_coord.x - piece.coord.x;
			}

			std::swap(vm->white_timer_coord, vm->black_timer_coord);
		}
	}

	// new_game
	else if (
		(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
		&& CheckCollisionPointRec(session.mouse_coord, vm->new_game_btn_coord)))
	{
		session.active = no_ui;
		session.page = seated;
		int mins = 10, secs = 0;
		vm->new_game(session, vm->is_from_white ^ 1, mins, secs);
		operative.opponent_check_evadables = 0;
		operative.opponent_check_blockables = ~((BB)0);
		operative.pin_count = 0;
	}

	// old_game
	else if (
		(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
		&& CheckCollisionPointRec(session.mouse_coord, vm->old_game_btn_coord)))
	{
		session.page = records; // warn struct/enum name shadow
		session.active = no_ui;
		int move_three = 6;
		if (session.games.back().score_sheet.size() > move_three)
			vm->save_last_game(session);

		// Update session draw data
		for (int txtr{}; txtr < black_square; txtr++)
		{
			Image img = LoadImageFromTexture(session.textures[txtr]);
			UnloadTexture(session.textures[txtr]);
			ImageResize(&img, records::piece_len, records::piece_len);
			session.textures[txtr] = LoadTextureFromImage(img);
			UnloadImage(img);
		}

		float new_board_size = records::piece_len*ROWS;
		Image checkerboard = LoadImageFromTexture(session.board.texture);
		UnloadRenderTexture(session.board);
		// nearest neighbor: checker board is low resolution
		ImageResizeNN(&checkerboard, new_board_size, new_board_size);
		// render texture draws upside-down
		ImageFlipVertical(&checkerboard); 
		Texture2D board_texture = LoadTextureFromImage(checkerboard);
		UnloadImage(checkerboard);
		session.board = LoadRenderTexture(new_board_size, new_board_size);
		BeginTextureMode(session.board);
		DrawTexture(board_texture, 0, 0, WHITE);
		EndTextureMode();

		// TODO: 
		// delete vm
		// create records object

		float img_size = records::piece_len*ROWS;
		for (int i{}; i < session.games_count; i++)
		{
			game_record* game = &session.mothball_games[i];
			for (int piece{}; piece < game->survivors_count; piece++)
			{
				int offset = img_size*i + session.games.size();
				session.survivors[piece].y += offset;
			}
		}
	}
} // draw_board_do_gamesetting_buttons

void do_play_buttons(view_model* vm, departure& operative, session& session)
{
	// menu select promotion option 
	if (session.active == promoting) 
	{
		if (CheckCollisionPointRec(session.mouse_coord, vm->promote_ribbon))
		{
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				// normalize mouse coord to board tile coordinates
				float relative_y = session.mouse_coord.y - vm->promote_ribbon.y;
				float relative_tile_length = relative_y/vm->piece_length;
				float selection_tile = std::trunc(relative_tile_length);
				vm->promote_selected_tile_y = vm->piece_length*selection_tile + vm->promote_ribbon.y;

				// draw
				DrawRectangleRec(vm->promote_ribbon, WHITE);
				Vector2 ribbon_start = vm->promote_coord.coord;
				// display options images
				for (int selection = vm->promote_min_selection; selection <= vm->promote_max_selection; selection += 2)
				{
					DrawTexture(session.textures[selection], ribbon_start.x, ribbon_start.y, WHITE);
					ribbon_start.y += vm->promote_ribbon_direction;
				}

				// highlight selection cursor
				DrawRectangleLines(vm->promote_ribbon.x, vm->promote_selected_tile_y, vm->piece_length, vm->piece_length, BLACK);
			}
			else
			{
				// option selected
				float round_up = 0.5;
				float relative_y = vm->promote_selected_tile_y - vm->promote_ribbon.y + round_up;
				// choose option 0-3
				int selection_tile = relative_y/vm->piece_length; 

				// bottom player ribbon displays from top down, top player from bottom up
				// rotate selected tile y for promoting player
				int to_compliment = MAX_PROMOTE_OPTION - 1;
				int selection
					= vm->is_opposite_color_promoting 
					? to_compliment - selection_tile
					: selection_tile;

				// enum piece: black_q, white_q, black_r, white_r, ect...
				// white_to_move selects color. Add selection twice to skip opposite color
				piece promoted = (piece)(black_queen + vm->model.white_to_move + 2*selection);
				vm->model.move_capture_promote(operative.piece, operative.from, vm->promote_coord.square, promoted);

				// promoting pawn already hidden from hide_pickup_piece()
				// int piece{ vm->piece_count };

				// assuming it is impossible to promote if no captures are made all game
				// let increment and decrement cancel out:
					// vm->show_last_hid_piece();	
					// visible_list[--vm->piece_count]

				// update view model
				secondary_effect advancement = (secondary_effect)(selection + promote_queen);
				session.games.back().score_sheet.emplace_back(operative.from, vm->promote_coord.square, advancement);
				// update pins, checks
				vm->model.instantiate_pins_checks(operative);

				session.active = no_ui;
				session.hot = no_ui;
				vm->legals_count = 0; // reset highlight
			} // end select promotion option
		}
		else
		{
			session.active = piece_drag;
		}
	} // end promotion


	// dropping piece outside the board is valid, even if illegal
	else if (session.active == piece_drag)
	{
		// draw piece
		Vector2 piece_center;
		piece_center.x = session.mouse_coord.x - vm->piece_length / 2.0;
		piece_center.y = session.mouse_coord.y - vm->piece_length / 2.0;
		DrawTextureV(session.textures[operative.piece], piece_center, WHITE);

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			if (operative.trigger_effect == promote_queen 
				&& CheckCollisionPointRec(session.mouse_coord, vm->board_coord))
			{
				// are we hovering over a promotion square?
				square_coord hovering; mouse_to_square_coord(*vm, session.mouse_coord, hovering);
				bool is_hovering_promotion{ false };
				for (int i{}; i < operative.trigger_count; i++)
					is_hovering_promotion |= hovering.square == operative.trigger_ids[i];
				if (is_hovering_promotion)
				{
					// display menu
					session.active = promoting;

					// bottom player ribbon displays from top down, top player from bottom up
					vm->promote_coord = hovering;
					vm->promote_ribbon.x = hovering.coord.x;
					bool is_opposite_color_promoting = vm->model.white_to_move ^ vm->is_from_white;
					vm->promote_ribbon.y 
						= is_opposite_color_promoting 
						? hovering.coord.y - (MAX_PROMOTE_OPTION - 1) * vm->piece_length
						: hovering.coord.y;
				}
			}
		}
		else 
		{
			// mouse button released
			square_coord dest; mouse_to_square_coord(*vm, session.mouse_coord, dest); // dest == destination

			session.active = no_ui;
			vm->legals_count = 0;	// reset highlight
			// if illegal move 	reset piece 
			// else			renable piece to update piece view 
			vm->show_last_hid_piece();	

			bool is_legal_move
				= CheckCollisionPointRec(session.mouse_coord, vm->board_coord)
				&& operative.squares & ((BB)1 << dest.square);
			if (is_legal_move)
			{
				// update view model
				vm->player_drops_piece(operative, dest);

				session.games.back().score_sheet.emplace_back(operative.from, dest.square, operative.trigger_effect);
				// NOTE: swap turn to opponent perspective before instantiating pins/checks
				vm->model.white_to_move ^= 1;

				// reset departure for generating legal moves
				operative.pin_count = 0;
				operative.opponent_check_blockables = ~((BB)0);
				operative.opponent_check_evadables = 0;
				victory_state ending = vm->model.pins_checks_checkmate_stalemate(operative, dest.square, session.games.back().score_sheet);

				if (ending == checkmate)
				{
					session.games.back().win = ending;
					session.games.back().result = vm->model.white_to_move ? black_win : white_win; 
					session.page = analysis;
				}
				else if (ending == stalemate)
				{
					session.games.back().win = ending;
					session.games.back().result = draw;
					session.page = analysis;
				}
			} // end piece drop
		} // end mouse up || down
	}// end piece dragginng

	// pick up new piece
	else if (session.active != piece_drag
		&& CheckCollisionPointRec(session.mouse_coord, vm->board_coord) 
		&& IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		square_coord select; mouse_to_square_coord(*vm, session.mouse_coord, select);
		operative.from = select.square;
		operative.piece = vm->model.get(operative.from);

		if (operative.piece != empty) 
		{
			session.active = piece_drag;
			vm->hide_pickup_piece(operative);

			// reset departure
			operative.squares = 0;
			operative.trigger_count = 0;
			operative.trigger_effect = none_secondary_effect;

			// always selects last game
			std::vector<half_move>& score_sheet = session.games.back().score_sheet;
			vm->model.get_legal_moves(operative, score_sheet);
			vm->instantiate_legal_move_coords(operative.squares); // if legal move count == 0, does nothing

			// prepare promoting
			if (operative.trigger_effect == promote_queen)
			{
				int white_move = vm->model.white_to_move;
				vm->promote_max_selection = black_knight + white_move;
				vm->promote_min_selection = black_queen + white_move;
				vm->is_opposite_color_promoting = white_move ^ vm->is_from_white;
				vm->promote_ribbon_direction = vm->is_opposite_color_promoting ? -vm->piece_length : vm->piece_length;
			}
		} // end validate pick-up
	} // end pick up piece

	// takeback
	else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
		&& CheckCollisionPointRec(session.mouse_coord, vm->back_move_btn_coord))
	{
		// reset departure data generating legal moves
		operative.pin_count = 0;
		operative.opponent_check_blockables = ~((BB)0);
		operative.opponent_check_evadables = 0;
		vm->back_move(operative, session);
		session.active = no_ui;
	} // end back btn

	// console
	else
	{
		char pressed_ch = GetCharPressed();
		int pressed_ky = GetKeyPressed();
		if (pressed_ky != KEY_NULL)
		{
			const char min_printable = ' ';
			if (pressed_ky == KEY_ENTER)
			{
				parse_algebraic_move(*vm, operative, vm->algebraic_input, vm->input_count, session);

				std::memset(vm->algebraic_input.data(), ' ', sizeof(char)*(vm->algebraic_input.size() - 1));
				vm->input_count = 0;
			}
			else if (pressed_ky == KEY_BACKSPACE)
			{
				if (vm->input_count > 0)
					vm->algebraic_input[--vm->input_count] = ' ';
			}
			if (pressed_ch >= min_printable)
			{
				if (vm->input_count < vm->algebraic_input.size() - 1)
					vm->algebraic_input[vm->input_count++] = pressed_ch;
			}
		}
	}
} // end do_play_buttons

void do_select_records(records* records, session& session)
{
	ClearBackground(WHITE);
	// Draw Board
	DrawTextureV(session.board.texture, records->img_begin, WHITE);

	for (int i{}; i < session.games.size(); i++)
	{
		game_record* game = &session.games[i];
		for (int piece{}; piece < game->survivors_count; piece++)
		{
			enum piece type = game->survivors[piece].piece;
			DrawTexture(session.textures[type], game->survivors[piece].x, game->survivors[piece].y, WHITE);
		}
	}

	float img_size = records::piece_len*ROWS;
	for (int i{}; i < session.games_count; i++)
	{
		game_record* game = &session.mothball_games[i];
		for (int piece{}; piece < game->survivors_count; piece++)
		{
			enum piece type = game->survivors[piece].piece;
			DrawTexture(session.textures[type], game->survivors[piece].x, game->survivors[piece].y, WHITE);
		}
	}

	// load games
	// list games
	// list pieces
};
