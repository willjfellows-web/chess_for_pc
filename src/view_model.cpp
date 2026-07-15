#include "view_model.h";


// ----------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------

// update visibles list to match state of model.mailbox
void reset_visibles_list(view_model* vm, int is_from_white)
{
	vm->piece_count = 0;
	// const int to_compliment{ ROWS - 1 };
	int ranks_with_pieces_count{ 4 };
	int ranks_with_pieces[] = { 0, 1, 6, 7 };
	if (is_from_white)
	{
		for (int rank_id{}; rank_id < ranks_with_pieces_count; rank_id++)
			for (int file{}; file < COLS; file++)
			{
				int rank = ranks_with_pieces[rank_id];
				square_coord& new_piece = vm->visible_piece_list[vm->piece_count++];
				new_piece.square = ROWS * rank + file;

				// piece coord == top left corner of square
				Vector2& coord = new_piece.coord;
				float y = rank*vm->piece_length + vm->board_coord.y;
				float x = file*vm->piece_length + vm->board_coord.x;
				coord.y = vm->to_compliment_coord_y - y;
				coord.x = x;

			}
	}
	else
	{
		for (int rank_id{}; rank_id < ranks_with_pieces_count; rank_id++)
			for (int file{}; file < COLS; file++)
			{
				int rank = ranks_with_pieces[rank_id];
				square_coord& new_piece = vm->visible_piece_list[vm->piece_count++];
				new_piece.square = ROWS * rank + file;

				// piece coord == top left corner of square
				Vector2& coord = new_piece.coord;
				float y = rank*vm->piece_length + vm->board_coord.y;
				float x = file*vm->piece_length + vm->board_coord.x; 
				coord.y = y;
				coord.x = vm->to_compliment_coord_x - x;
			}
	}
} // end reset visible piece list

void view_model::new_game(int is_from_white)
{
	this->is_from_white = is_from_white;
	model.reset_score_sheet();
	model.set_defualt_board();
	reset_visibles_list(this, is_from_white);
	// departure reset during gameloop
}

view_model::view_model(
	std::array<Texture2D, empty> textures
	, int piece_length
	, int screen_width
	, int screen_height
	, int is_from_white) 
	: is_from_white{ is_from_white }
	, piece_length{ piece_length }
	, textures{ textures }
	, board_coord{ (float)screen_width / 32, (float)screen_height / 32, (float)piece_length * COLS, (float)piece_length * ROWS }
	// extra board coord for piece.coords already possessing a board coord during main.flip_piece() 
	, to_compliment_coord_y { (ROWS - 1)*piece_length + board_coord.y + board_coord.y } 
	, to_compliment_coord_x { (ROWS - 1)*piece_length + board_coord.x + board_coord.x }
{
	// cache checkerboard as texture
	board = LoadRenderTexture(piece_length * ROWS, piece_length * COLS);
	draw_board_grid_texture();
	new_game(is_from_white);
} // end set starting position constructor


view_model::~view_model()
{
	UnloadRenderTexture(board);
	for (Texture2D& texture : textures)
		UnloadTexture(texture);
}


// ----------------------------------------------------------------------
// Data Conversions
// ----------------------------------------------------------------------

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
	// need rows and cols to calc sq_id
	int row = (mouse_coord.y - vm.board_coord.y)/vm.piece_length;
	int col = (mouse_coord.x - vm.board_coord.x)/vm.piece_length;

	// offset with grid lines
	out_sq_coord.coord.y = row * vm.piece_length + vm.board_coord.y;
	out_sq_coord.coord.x = col * vm.piece_length + vm.board_coord.x;

	// square index 
	tile rank_file = synch_board_rotation(vm.is_from_white, row, col);
	int rank = rank_file.row;
	int file = rank_file.col;
	out_sq_coord.square = rank * ROWS + file;
}

Vector2 square_to_coord(view_model& vm, sq_id square)
{
	const int to_compliment = ROWS - 1;

	int rank = get_rank(square);
	int file = get_file(square);
	tile tile = synch_board_rotation(vm.is_from_white, rank, file);

	Vector2 coord;
	coord.x = vm.piece_length * tile.col + vm.board_coord.x;
	coord.y = vm.piece_length * tile.row + vm.board_coord.y;
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
	// remove and forget captured piece
	captured = vm.visible_piece_list[--vm.piece_count];
}

void place_castle(view_model& vm, departure& operative, square_coord& destination)
{
	int king_from = operative.from;
	int king_to = destination.square;
	int white_to_move = vm.model.score_sheet.size() % 2;
	piece king = (piece)(black_king + white_to_move);
	vm.model.move_piece(king, king_from, king_to);
	// king coordinates already updated

	// Rook
	// Model
	// beginning square of color's setup rank.
	const unsigned black_to_move = white_to_move ^ 1;
	// if white	0
	// if black	last_row_start
	int home_rank_start = -black_to_move & (COLS * (ROWS - 1)); 
	sq_id rook_from{ home_rank_start }; 
	sq_id rook_to{ home_rank_start };
	unsigned is_castle_kingside = king_to > king_from;
	rook_from += is_castle_kingside ? h : a;
	rook_to += is_castle_kingside ? f : d;
	piece rook = (piece)(black_rook + white_to_move);
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
	int white_to_move = vm.model.score_sheet.size() % 2;
	piece king = (piece)(black_king + white_to_move);
	vm.model.move_piece(king, king_expel, king_home);
	// king coordinates already updated

	// Rook
	// Model
	// beginning square of color's setup rank.
	const unsigned black_to_move = white_to_move ^ 1;
	int home_rank_start = black_to_move * (COLS * (ROWS - 1)); 
	sq_id rook_home{ home_rank_start }; 
	sq_id rook_expel{ home_rank_start };
	unsigned is_castle_kingside = king_home > king_expel;
	rook_home += is_castle_kingside ? a : h;
	rook_expel += is_castle_kingside ? d : f;
	piece rook = (piece)(black_rook + white_to_move);
	vm.model.move_piece(rook, rook_expel, rook_home);

	// View coordinates
	square_coord& rook_start = get_visible_square(vm.visible_piece_list, rook_expel);
	rook_start.square = rook_home;
	rook_start.coord = square_to_coord(vm, rook_home);
}

void place_en_passant(view_model& vm, departure& operative, square_coord& destination)
{
	int white_to_move = vm.model.score_sheet.size() % 2;
	piece pawn = (piece)(black_pawn + white_to_move);
	vm.model.move_piece(pawn, operative.from, destination.square);
	// pawn coordinates already updated

	// Ambushed pawn
	sq_id ambushed{ destination.square };
	int black_to_move = white_to_move ^ 1;
	ambushed += (black_to_move - white_to_move) * COLS;
	piece passer = (piece)(black_pawn + black_to_move);
	vm.model.toggle_nonempty_piece(passer, ambushed);

	// remove pawn
	square_coord& ambushed_pawn = get_visible_square(vm.visible_piece_list, ambushed);
	ambushed_pawn = vm.visible_piece_list[--vm.piece_count];
}

void place_reverse_en_passant(view_model& vm, half_move& previous)
{
	int white_to_move = vm.model.score_sheet.size() % 2;
	piece pawn = (piece)(black_pawn + white_to_move);
	vm.model.move_piece(pawn, previous.to, previous.from);
	// pawn coordinates already updated

	// Ambushed pawn
	sq_id ambushed{ previous.to };
	int black_to_move = white_to_move ^ 1;
	// if black, ambushed = destination north one
	// if white, ambushed = destination south one
	ambushed += (black_to_move - white_to_move) * COLS;
	piece passer = (piece)(black_pawn + black_to_move);
	vm.model.toggle_nonempty_piece(passer, ambushed);

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

void view_model::player_drops_piece(departure& operative, square_coord& destination)
{
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

	piece captured = model.get(destination.square);

	if (operative.trigger_effect == none_secondary_effect)
	{
		if (captured == empty)
			model.move_piece(operative.piece, operative.from, destination.square);
		else
			place_capture(*this, operative, destination);
	}
	else
	{
		bool is_triggered_second_effect = false;
		for (sq_id sq{}; sq < operative.trigger_count; sq++)
			is_triggered_second_effect |= destination.square == operative.trigger_ids[sq];

		if (!is_triggered_second_effect)
		{
			if (captured == empty)
				model.move_piece(operative.piece, operative.from, destination.square);
			else
				place_capture(*this, operative, destination);
		}
		else if (operative.trigger_effect == castle)
			place_castle(*this, operative, destination);
		else if (operative.trigger_effect == en_passant)
			place_en_passant(*this, operative, destination);
		// promotion handled from promotion ribbon menu
	}
}

void place_reverse_move_capture(view_model& vm, half_move& previous)
{
	piece migrant = vm.model.get(previous.to);

	if (previous.captured == empty)
	{
		vm.model.move_piece(migrant, previous.to, previous.from);
		// view already reversed
	}
	else
	{
		vm.model.reverse_capture_piece(migrant, previous.from, previous.to, previous.captured);
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
	int white_to_move = vm.model.score_sheet.size() % 2;
	piece pawn = (piece)(black_pawn + white_to_move);
	if (previous.captured == empty)
	{
		vm.model.reverse_move_promote(pawn, previous.from, previous.to, advancement);
		// view already updated
	}
	else // undo capture
	{
		vm.model.reverse_capture_promote(pawn, previous, advancement);
		// index of last capture
		int disgarded_index{ vm.piece_count };  
		square_coord& recycle_piece = vm.visible_piece_list[disgarded_index];
		recycle_piece.square = previous.to;
		recycle_piece.coord = square_to_coord(vm, previous.to);
		// make piece visible
		vm.piece_count++;
	}
}

void view_model::back_move(departure& operative)
{
	int zero_move{ 1 };
	if (model.score_sheet.size() == zero_move)
		return;

	half_move& previous = model.score_sheet.back();
	sq_id expel = previous.to;
	model.score_sheet.pop_back(); // set perspective to previous player

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

	print_mailbox(model);

	if (previous.effect == none_secondary_effect)
		place_reverse_move_capture(*this, previous);
	else if (previous.effect == castle)
		place_reverse_castle(*this, previous);
	else if (previous.effect == en_passant)
		place_reverse_en_passant(*this, previous);
	else // promote
	{
		// minimium previous.effect value is queen
		int selection = previous.effect - promote_queen;
		int white_to_move = model.score_sheet.size() % 2;
		// enum piece follows [ colorB, colorW, colorB, colorW, ...] pattern 
		// double selection to find piece type
		int selection_at_color = white_to_move + selection + selection;
		piece advancement = (piece)(black_queen + selection_at_color);
		place_reverse_promote(*this, previous, advancement);
	}

	// research pins, checks
	model.instantiate_pins_checks(operative);
}

// ----------------------------------------------------------------------
// Drawing
// ----------------------------------------------------------------------

void view_model::draw_board_grid_texture()
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
				int is_black_square = row % 2 ^ col % 2; 
				DrawTexture(textures[black_square + is_black_square], square.x, square.y, WHITE);
			}
	EndTextureMode();
}
