#include "view_model.h";
#define BASE_TEN 10
#define SECS_PER_MIN 60

void error(game_record& game)
{
	std::cout << "opening: " << game.opening << '\n';
	std::cout << "win: " << game.win << '\n';
	std::cout << "result: " << game.result << '\n';
	std::cout << "clock_total_seconds: " << game.clock_total_seconds << '\n';
	std::cout << "mothball_begin: " << game.mothball_begin << '\n';
	std::cout << "date: " << (int)game.year << ' ' << (int)game.month << ' ' << (int)game.day << '\n';
}

// ----------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------

void set_clocks(view_model& vm, int min, int sec)
{
	std::div_t second_digits = std::div(sec, BASE_TEN);
	vm.black.display[4] = vm.white.display[4] = second_digits.rem + '0';
	vm.black.display[3] = vm.white.display[3] = second_digits.quot + '0';

	std::div_t minute_digits = std::div(min, BASE_TEN);
	vm.black.display[1] = vm.white.display[1] = minute_digits.rem + '0';
	vm.black.display[0] = vm.white.display[0] = minute_digits.quot + '0';
	int total_secs = min*60 + sec;
	// view::update_timer must detect decrease in seconds as soon as game starts
	vm.black.previous_total_seconds = vm.white.previous_total_seconds = total_secs; 
	float countdown = (float)total_secs;
	vm.black.countdown = vm.white.countdown = countdown;
}

void view_model::save_last_game(session& session)
{
	game_record* game = &session.games.back();
	game->score_sheet.shrink_to_fit();
	// save final position image
	game->survivors_count = piece_count;
	int loaded_position_size = piece_count*sizeof(piece_coord);
	game->survivors = (piece_coord*)malloc(loaded_position_size);

	// NOTE: records globals temp location
	const Vector2 records_img_begin = { 600, 100 };
	const float records_piece_len = 32;

	for (int piece{}; piece < piece_count; piece++)
	{
		sq_id sq = visible_piece_list[piece].square;
		enum piece survivor = model.get(sq);
		game->survivors[piece].piece = survivor;
		Vector2 coord = square_to_coord(records_img_begin, records_piece_len, this->is_from_white, sq);
		game->survivors[piece].x = coord.x;
		game->survivors[piece].y = coord.y;
	}
}

void view_model::new_game(session& session, int is_from_white, int player_minutes, int player_seconds)
{
	int move_three = 6;
	if (session.games.back().score_sheet.size() <= move_three)
		// returning to play-page always creates a new game
		session.games.pop_back();
	else
		save_last_game(session);

	model.white_to_move = 1;
	session.games.push_back(game_record());
	game_record* game = &session.games.back();
	// game->rating_black;
	// game->rating_white;
	// game->id_black;
	// game->id_white;
	// game->mothball_begin;
	// game->opening = (opening)0;
	game->clock_total_seconds = player_minutes*60 + player_seconds;
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);
	game->year = now->tm_year;
	game->month = now->tm_mon;
	game->day =  now->tm_mday;

	model.set_default_board();
	set_clocks(*this, player_minutes, player_seconds);
	// swap perspective for setup
	this->is_from_white = is_from_white;
	// swap clock display location &
	// update visibles list to match the mailbox in default setup
	piece_count = 0;
	int ranks_with_pieces_count{ 4 };
	const static int ranks_with_pieces[] = { 0, 1, 6, 7 };
	if (is_from_white)
	{
		white_timer_coord = &top_timer_coord;
		black_timer_coord = &bottom_timer_coord;

		for (int rank_id{}; rank_id < ranks_with_pieces_count; rank_id++)
			for (int file{}; file < COLS; file++)
			{
				int rank = ranks_with_pieces[rank_id];
				square_coord& new_piece = visible_piece_list[piece_count++];
				new_piece.square = ROWS * rank + file;

				// piece coord == top left corner of square
				Vector2& coord = new_piece.coord;
				float y = rank*piece_length;
				float x = file*piece_length;
				coord.y = to_compliment_coord_y - y;
				coord.x = x + board_coord.x;
			}
	}
	else
	{
		white_timer_coord = &bottom_timer_coord;
		black_timer_coord = &top_timer_coord;

		for (int rank_id{}; rank_id < ranks_with_pieces_count; rank_id++)
			for (int file{}; file < COLS; file++)
			{
				int rank = ranks_with_pieces[rank_id];
				square_coord& new_piece = visible_piece_list[piece_count++];
				new_piece.square = ROWS * rank + file;

				// piece coord == top left corner of square
				Vector2& coord = new_piece.coord;
				float y = rank*piece_length;
				float x = file*piece_length; 
				coord.y = y + board_coord.y;
				coord.x = to_compliment_coord_x - x;
			}
	}
} // end new_game

view_model::view_model(session& session, float piece_length, int screen_width, int screen_height, int is_from_white) 
	: is_from_white{ is_from_white }
	, piece_length{ piece_length }
	, board_coord{ (float)screen_width / 32, (float)110, piece_length * COLS, piece_length * ROWS }
	// extra board coord for piece.coords already possessing a board coord during main.flip_piece() 
	, to_compliment_coord_y { (ROWS - 1)*piece_length + board_coord.y } 
	, to_compliment_coord_x { (ROWS - 1)*piece_length + board_coord.x }

	, text_size{ 40 }
	, new_game_btn_coord { 20*screen_width/32, screen_height / 2, 200, 100 }
	, old_game_btn_coord { 20*screen_width/32, screen_height / 2 + 300, 200, 100 }
	, swap_sides_btn_coord { 26*screen_width/32, screen_height / 2, 120, 120 }
	, back_move_btn_coord { 20*screen_width/32, 20*screen_height / 32, 100, 50 }
	, console_coord{ screen_width*22/32, screen_height*10/32, 300, 50 }

	, promote_ribbon{ 0, 0, piece_length, piece_length*MAX_PROMOTE_OPTION }

	, top_timer_coord{ screen_width*2/32, 110 + (110 - 100)/5 + ROWS*piece_length, 150, 80  }
	, bottom_timer_coord{ screen_width/32 + COLS*piece_length - screen_width*2/32, (110 - 100)/5, 150, 80  }
{
	new_game(session, is_from_white, 10, 0);
	// instantiate console
	std::memset(algebraic_input.data(), ' ', sizeof(char)*(algebraic_input.size() - 1)); 
	algebraic_input[algebraic_input.size() - 1] = '\0';
} // end set starting position constructor




// ----------------------------------------------------------------------
// Data Conversions
// ----------------------------------------------------------------------

/// coordinate of board cell displayed on screen. Agnostic of sides
struct tile
{
	int row{}, col{};
};

tile synch_board_rotation(bool from_white, int rank, int file)
{
	const int to_compliment = ROWS - 1;
	tile rotated;
	switch (from_white)
	{
	case 0:
		rotated.row = rank;
		rotated.col = to_compliment - file;
		break;
	case 1:
		rotated.row = to_compliment - rank;
		rotated.col = file;
		break;
	}
	return rotated;
}

void mouse_to_square_coord(view_model& vm, Vector2 mouse_coord, square_coord& out_sq_coord)
{
	// convert coord to tile
	float row = std::trunc((mouse_coord.y - vm.board_coord.y)/vm.piece_length);
	float col = std::trunc((mouse_coord.x - vm.board_coord.x)/vm.piece_length);

	// get tile coordinates
	out_sq_coord.coord.y = vm.board_coord.y + row*vm.piece_length;
	out_sq_coord.coord.x = vm.board_coord.x + col*vm.piece_length;;

	// square index 
	tile rank_file = synch_board_rotation(vm.is_from_white, row, col);
	int rank = rank_file.row;
	int file = rank_file.col;
	out_sq_coord.square = rank * ROWS + file;
}


Vector2 square_to_coord(view_model& vm, sq_id square)
{
	int rank = get_rank(square);
	int file = get_file(square);
	tile tile = synch_board_rotation(vm.is_from_white, rank, file);

	Vector2 coord;
	coord.x = vm.piece_length * tile.col + vm.board_coord.x;
	coord.y = vm.piece_length * tile.row + vm.board_coord.y;
	return coord;
}

Vector2 square_to_coord(Vector2 board_coord, float piece_length, bool is_from_white, sq_id square)
{
	int rank = get_rank(square);
	int file = get_file(square);
	tile tile = synch_board_rotation(is_from_white, rank, file);

	Vector2 coord;
	coord.x = piece_length * tile.col + board_coord.x;
	coord.y = piece_length * tile.row + board_coord.y;
	return coord;
}

sq_id mouse_to_square(view_model& vm, Vector2 mouse_coord)
{
	int row = (mouse_coord.y - vm.board_coord.y) / vm.piece_length;
	int col = (mouse_coord.x - vm.board_coord.x) / vm.piece_length;
	
	tile rank_file = synch_board_rotation(vm.is_from_white, row, col);
	int rank = rank_file.row;
	int file = rank_file.col;
	sq_id square = rank * ROWS + file;
	return square;
};



// ----------------------------------------------------------------------
// Visible Piece List
// ----------------------------------------------------------------------

inline square_coord& get_visible_square(std::array<square_coord, 32>& visible_piece_list, sq_id find)
{
	int piece{};
	while (visible_piece_list[piece++].square != find)
		;
	piece--; // de post-increment
	return visible_piece_list[piece];
}

void place_capture(view_model& vm, departure& operative, square_coord& destination)
{
	vm.model.capture_piece(operative.piece, operative.from, destination.square);
	square_coord& captured = get_visible_square(vm.visible_piece_list, destination.square);
	// overwrite captured with last visible
	captured = vm.visible_piece_list[--vm.piece_count];
}


void place_castle(view_model& vm, departure& operative, square_coord& destination)
{
	int king_from = operative.from;
	int king_to = destination.square;
	piece king = (piece)(black_king + vm.model.white_to_move);
	vm.model.move_piece(king, king_from, king_to);
	// king coordinates already updated

	// Rook
	// Model
	// beginning square of color's setup rank.
	const unsigned black_to_move = vm.model.white_to_move ^ 1;
	// if white	0
	// if black	last_row_start
	int home_rank_start = -black_to_move & (COLS * (ROWS - 1)); 
	sq_id rook_from{ home_rank_start }; 
	sq_id rook_to{ home_rank_start };
	unsigned is_castle_kingside = king_to > king_from;
	rook_from += is_castle_kingside ? h : a;
	rook_to += is_castle_kingside ? f : d;
	piece rook = (piece)(black_rook + vm.model.white_to_move);
	vm.model.move_piece(rook, rook_from, rook_to);

	// View coordinates
	
	square_coord& rook_start = get_visible_square(vm.visible_piece_list, rook_from);
	rook_start.square = rook_to;
	rook_start.coord = square_to_coord(vm, rook_to);
}

void place_reverse_castle(view_model& vm, half_move& previous)
{
	int king_home = previous.from;
	int king_expel = previous.to;
	piece king = (piece)(black_king + vm.model.white_to_move);
	vm.model.move_piece(king, king_expel, king_home);
	// king coordinates already updated

	// Rook
	// Model
	// beginning square of color's setup rank.
	const unsigned black_to_move = vm.model.white_to_move ^ 1;
	int home_rank_start = black_to_move * (COLS * (ROWS - 1)); 
	sq_id rook_home{ home_rank_start }; 
	sq_id rook_expel{ home_rank_start };
	unsigned is_castle_kingside = king_home > king_expel;
	rook_home += is_castle_kingside ? a : h;
	rook_expel += is_castle_kingside ? d : f;
	piece rook = (piece)(black_rook + vm.model.white_to_move);
	vm.model.move_piece(rook, rook_expel, rook_home);

	// View coordinates
	square_coord& rook_start = get_visible_square(vm.visible_piece_list, rook_expel);
	rook_start.square = rook_home;
	rook_start.coord = square_to_coord(vm, rook_home);
}

void place_en_passant(view_model& vm, departure& operative, square_coord& destination)
{
	piece pawn = (piece)(black_pawn + vm.model.white_to_move);
	vm.model.move_piece(pawn, operative.from, destination.square);
	// pawn visibility coordinates already updated in player_drops_piece

	// Locate ambushed pawn
	sq_id ambushed{ destination.square };
	int black_to_move = vm.model.white_to_move ^ 1;
	int ambushed_direction_from_dest = black_to_move - vm.model.white_to_move;
	ambushed +=  ambushed_direction_from_dest * ROWS;
	piece color = (piece)(black_pawn + black_to_move);
	vm.model.empty_occupied_square(color, ambushed);

	// remove pawn
	square_coord& ambushed_pawn = get_visible_square(vm.visible_piece_list, ambushed);
	ambushed_pawn = vm.visible_piece_list[--vm.piece_count];
}

void place_reverse_en_passant(view_model& vm, half_move& previous)
{
	piece pawn = (piece)(black_pawn + vm.model.white_to_move);
	vm.model.move_piece(pawn, previous.to, previous.from);
	// pawn coordinates already updated

	// Ambushed pawn
	sq_id ambushed{ previous.to };
	int black_to_move = vm.model.white_to_move ^ 1;
	// if black, ambushed = destination north one
	// if white, ambushed = destination south one
	ambushed += (black_to_move - vm.model.white_to_move) * COLS;
	piece passer = (piece)(black_pawn + black_to_move);
	vm.model.set_empty_square(passer, ambushed);

	// replace ambushed
	int disgarded_index{ vm.piece_count };
	square_coord& recycle_piece = vm.visible_piece_list[disgarded_index];
	recycle_piece.square = ambushed;
	recycle_piece.coord = square_to_coord(vm, ambushed);
	// make piece visible
	vm.piece_count++;
}

void view_model::instantiate_legal_move_coords(BB moves)
{
	while (moves)
	{
		sq_id least_significant_bit = bit_scan_forward(moves);
		legals_list[legals_count++] = square_to_coord(*this, least_significant_bit);
		moves &= moves - 1;
	}
}

void view_model::hide_pickup_piece(departure& operative)
{
	square_coord& piece = get_visible_square(visible_piece_list, operative.from);
	int static_pieces_count = piece_count - 1;
	square_coord temp = visible_piece_list[static_pieces_count];
	visible_piece_list[static_pieces_count] = piece;
	piece = temp;
	piece_count = static_pieces_count;
}

void promote_capture_or_move_junction(view_model& vm, departure& operative, square_coord& dest, piece promotion)
{
	piece captured = vm.model.get(dest.square);
	if (captured == empty)
		vm.model.move_promote(operative.piece, operative.from, dest.square, promotion);
	else 
	{
		vm.model.capture_promote(operative.piece, operative.from, dest.square, promotion);

		// update view model
		square_coord& captured = get_visible_square(vm.visible_piece_list, dest.square);
		// overwrite captured with last visible
		captured = vm.visible_piece_list[--vm.piece_count];
	}
}

// will reset operative trigger effect if selected square != trigger_id
void view_model::player_drops_piece(departure& operative, square_coord& destination)
{
	// update moved piece
	// check last first because from_piece was probably dragged/dropped and placed at last
	int from_index{ piece_count - 1 }; 
	if (visible_piece_list[from_index].square == operative.from)
	{
		visible_piece_list[from_index] = destination;
	}
	else
	{
		square_coord& piece = get_visible_square(visible_piece_list, operative.from);
		piece = destination;
	}

	// Place

	secondary_effect effect = none_secondary_effect;
	for (sq_id sq{}; sq < operative.trigger_count; sq++)
		if (destination.square == operative.trigger_ids[sq])
			effect = operative.trigger_effect;

	int white_to_move = model.white_to_move;

	// promotion effects pass through here from algebraic notation
	switch(effect)
	{
		case none_secondary_effect:
			{
				piece captured = model.get(destination.square);
				if (captured == empty) 	model.move_piece(operative.piece, operative.from, destination.square);
				else 			place_capture(*this, operative, destination);
			}
			break;
		case castle:
			place_castle(*this, operative, destination);
			break;
		case en_passant:
			place_en_passant(*this, operative, destination);
			break;
		case promote_queen:
			promote_capture_or_move_junction(*this, operative, destination, (piece)(black_queen + white_to_move));
			break;
		case promote_rook:
			promote_capture_or_move_junction(*this, operative, destination, (piece)(black_rook + white_to_move));
			break;
		case promote_bishop:
			promote_capture_or_move_junction(*this, operative, destination, (piece)(black_bishop + white_to_move));
			break;
		case promote_knight:
			promote_capture_or_move_junction(*this, operative, destination, (piece)(black_knight + white_to_move));
			break;
	}
}

// TODO: erase reverse one: reverse of any number moves === a complete reset

piece get_captured() { return empty; }

void place_reverse_move_capture(view_model& vm, half_move& previous)
{
	piece migrant = vm.model.get(previous.to);

	piece captured = get_captured();
	if (captured == empty)
	{
		vm.model.move_piece(migrant, previous.to, previous.from);
		// view already reversed
	}
	else
	{
		vm.model.reverse_capture_piece(migrant, previous.from, previous.to, captured);
		// recreate piece
		int disgarded_index{ vm.piece_count };
		square_coord& recycle_piece = vm.visible_piece_list[disgarded_index];
		recycle_piece.square = previous.to;
		recycle_piece.coord = square_to_coord(vm, previous.to);
		// make piece visible
		vm.piece_count++;
	}
}

void place_reverse_promote(view_model& vm, half_move& previous, piece advancement)
{
	piece pawn = (piece)(black_pawn + vm.model.white_to_move);
	piece captured = get_captured();
	if (captured == empty)
	{
		vm.model.reverse_move_promote(pawn, previous.from, previous.to, advancement);
		// view already updated
	}
	else // undo capture
	{
		vm.model.reverse_capture_promote(pawn, previous, advancement, captured);
		// index of last capture
		int disgarded_index{ vm.piece_count };  
		square_coord& recycle_piece = vm.visible_piece_list[disgarded_index];
		recycle_piece.square = previous.to;
		recycle_piece.coord = square_to_coord(vm, previous.to);
		// make piece visible
		vm.piece_count++;
	}
}



void view_model::back_move(departure& operative, session& session)
{
	if (session.games.back().score_sheet.size() == 0)
		return;

	std::vector<half_move> score_sheet = session.games.back().score_sheet;
	half_move& previous = score_sheet.back();
	sq_id expel = previous.to;
	score_sheet.pop_back(); 
	// NOTE: set perspective to previous player before returning move
	model.white_to_move ^= 1;

	Vector2 return_coord = square_to_coord(*this, previous.from);
	// migrant_piece probably dragged/dropped then placed at last index 
	int migrant{ piece_count - 1 };
	if (visible_piece_list[migrant].square == expel)
	{
		visible_piece_list[migrant].square = previous.from;
		visible_piece_list[migrant].coord = return_coord;
	}
	else
	{
		square_coord& migrant_piece = get_visible_square(this->visible_piece_list, expel);
		migrant_piece.square = previous.from;
		migrant_piece.coord = return_coord;
	}

	model.print_mailbox();

	int white_to_move = model.white_to_move;
	switch (previous.effect)
	{
		case none_secondary_effect: place_reverse_move_capture(*this, previous); break;
		case castle: place_reverse_castle(*this, previous); break;
		case en_passant: place_reverse_en_passant(*this, previous); break;
		case promote_queen: place_reverse_promote(*this, previous, (piece)(black_queen + white_to_move)); break;
		case promote_rook: place_reverse_promote(*this, previous, (piece)(black_rook + white_to_move)); break;
		case promote_bishop: place_reverse_promote(*this, previous, (piece)(black_bishop + white_to_move)); break;
		case promote_knight: place_reverse_promote(*this, previous, (piece)(black_knight + white_to_move)); break;
	}

	// research pins, checks
	model.instantiate_pins_checks(operative);
}

// ----------------------------------------------------------------------
// Drawing
// ----------------------------------------------------------------------

void session::draw_board_grid_texture(int piece_length)
{
	BeginTextureMode(board);
		Vector2 square;
		for (int row{}; row < ROWS; row++)
			for (int col{}; col < ROWS; col++)
			{
				square.y = piece_length * row;
				square.x = piece_length * col;
				// GPU draws with negative height.
				// we draw opposite color square becuase board is asymetrical
				// ex) if black sq -> draw white sq
				int is_black_square = row % 2 ^ col % 2; 
				DrawTexture(textures[black_square + is_black_square], square.x, square.y, WHITE);
			}
	EndTextureMode();
}


void view_model::decrement_timer(session& session)
{
	struct clock& defender = model.white_to_move ? white : black; // update defender during move
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

		if (defender.countdown <= 0)
		{
			session.page = analysis;
			victory_state maybe_draw = model.is_time_insufficient_material();
			session.games.back().win = maybe_draw;
			if (maybe_draw == draw_on_time)
				session.games.back().result = draw;
			// opponent wins
			else if (model.white_to_move)
				session.games.back().result = black_win;
			else 
				session.games.back().result = white_win;
		}
	}
}
