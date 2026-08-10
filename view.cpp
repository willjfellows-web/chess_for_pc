#include "view.h"



void parse_algebraic_move(view_model& vm, departure& operative, std::array<char, 16>& input, int char_input_count)
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

