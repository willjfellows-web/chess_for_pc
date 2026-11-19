#include "model.h"



// ============================================================================================
// debugging
// ============================================================================================

void print_bb(BB bitboard)
{
	std::cout << "board #" << bitboard << '\n';
	int bit;
	int bit_state;
	for (int rank{ ROWS - 1 }; rank >= 0; rank--)
	{
		for (int file{ a }; file < max_file; file++)
		{
			bit = rank * ROWS + file;
			bit_state = (bitboard >> bit) % 2;
			switch (bit_state)
			{
			case 0: std::cout << '.'; break;
			case 1: std::cout << '1'; break;
			}
			std::cout << ' ';
		}
		std::cout << '\n';
	}
}


void print_bb(BB bitboard, const char* message)
{
	std::cout << message << '\n';
	print_bb(bitboard);
}


void print_piece(piece piece)
{
	switch (piece)
	{
	case black_king: std::cout << "bK"; break;
	case white_king: std::cout << "wK"; break;
	case black_queen: std::cout << "bQ"; break;
	case white_queen: std::cout << "wQ"; break;
	case black_rook: std::cout << "bR"; break;
	case white_rook: std::cout << "wR"; break;
	case black_bishop: std::cout << "bB"; break;
	case white_bishop: std::cout << "wB"; break;
	case black_knight: std::cout << "bN"; break;
	case white_knight: std::cout << "wN"; break;
	case black_pawn: std::cout << "bp"; break;
	case white_pawn: std::cout << "wp"; break;
	case empty: std::cout << ". "; break;
	default: std::cout << "ER" << piece; break;
	}
}

void print_secondary_effect(secondary_effect effect)
{
	switch (effect)
	{
	case none_secondary_effect: std::cout << "none_secondary_effect"; break;
	case castle: std::cout << "castle"; break;
	case en_passant: std::cout << "en_passant"; break;
	case promote_queen: std::cout << "promote_queen"; break;
	case promote_rook: std::cout << "promote_rook"; break;
	case promote_bishop: std::cout << "promote_bishop"; break;
	case promote_knight:	std::cout << "promote_knight"; break;
	}
}

void print_half_move(const half_move* move)
{
	std::cout << "departure: " << move->from << '\n'
		<< "destinate: " << move->to << '\n';
	print_secondary_effect(move->effect);
	std::cout << '\n';
}

void error(secondary_effect effect) { print_secondary_effect(effect); }
void error(const char* message) { std::cout << message << '\n'; }
void error(const char* message, bool condition) { if (condition) std::cout << message << '\n'; }
void error(piece value) { print_piece(value); std::cout << '\n'; }
void error(piece value, const char* label) { std::cout << label << ':'; print_piece(value); std::cout << '\n'; }
void error(BB bitboard) { print_bb(bitboard); }
void error(BB bitboard, const char* message) { print_bb(bitboard, message); }
void error(const half_move* move) { print_half_move(move); }

void model::print_mailbox()
{
	for (int rank{ ROWS - 1 }; rank >= 0; rank--)
	{
		for (int file{}; file < COLS; file++) 
		{
			sq_id index = ROWS*rank + file;
			piece piece = mailbox[index];
			print_piece(piece);
			std::cout << ' ';
		}
		std::cout << '\n';
	}
}

void print_mailbox(model& model)
{
	for (int rank{ ROWS - 1 }; rank >= 0; rank--)
	{
		for (int file{}; file < COLS; file++) 
		{
			piece piece = model.get(file, rank);
			print_piece(piece);
			std::cout << ' ';
		}
		std::cout << '\n';
	}
}

// ============================================================================================
// accessors
// ============================================================================================


piece model::move_capture_piece(piece piece, sq_id from, sq_id to)
{
	BB bb_from = (BB)1 << from;
	BB bb_to = (BB)1 << to;

	// push
	piece_by_type[piece] ^= bb_from | bb_to;
	piece_by_type[black_square + white_to_move] ^= bb_from | bb_to;

	// capture
	const enum piece captured = mailbox[to];

	// if capture == empty, occupy empty
	// else,				remove captured_piece bit
	piece_by_type[captured] ^= bb_to;

	// occupy empty in anycase
	piece_by_type[empty] &= ~(bb_to);
	piece_by_type[empty] ^= bb_from; // remove from starting location

	// if capture == empty, no changes
	// else,				remove opposing color bit
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] &= ~bb_to;

	mailbox[from] = empty;
	mailbox[to] = piece;

	return captured;
}

void model::capture_piece(piece piece, sq_id from, sq_id to)
{
	BB bb_from = (BB)1 << from;
	BB bb_to = (BB)1 << to;

	// defenders moves
	piece_by_type[piece] ^= bb_from | bb_to;
	piece_by_type[black_square + white_to_move] ^= bb_from | bb_to;

	// capture
	const enum piece captured = mailbox[to];
	piece_by_type[captured] ^= bb_to;
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] ^= bb_to;

	// to already occupied
	piece_by_type[empty] ^= bb_from; 

	mailbox[from] = empty;
	mailbox[to] = piece;
}

void model::reverse_capture_piece(piece migrant, sq_id restore, sq_id expel, enum piece captured)
{
	BB bb_expel = (BB)1 << expel;
	BB bb_restore = (BB)1 << restore;

	// defenders moves
	piece_by_type[migrant] ^= bb_expel | bb_restore;
	piece_by_type[black_square + white_to_move] ^= bb_expel | bb_restore;

	// uncapture
	piece_by_type[captured] ^= bb_expel;
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] ^= bb_expel;

	// expel already occupied
	piece_by_type[empty] ^= bb_restore; 

	mailbox[restore] = migrant;
	mailbox[expel] = captured;
}

void model::move_piece(piece piece, sq_id from, sq_id to)
{
	BB from_to = (BB)1 << from | (BB)1 << to;

	piece_by_type[piece] ^= from_to;
	piece_by_type[black_square + white_to_move] ^= from_to;
	piece_by_type[empty] ^= from_to;

	mailbox[from] = empty;
	mailbox[to] = piece;
}

void model::reverse_move_capture_piece(half_move& undone, piece original_occupant)
{
	piece reversing = mailbox[undone.to];

	BB bb_return = (BB)1 << undone.from;
	BB emigrate = (BB)1 << undone.to;

	// reverse piece out of square
	piece_by_type[reversing] ^= emigrate | bb_return;
	piece_by_type[black_square + white_to_move] ^= emigrate | bb_return;

	// if capture == empty, replace empty
	// else,				replace captured piece
	piece_by_type[original_occupant] ^= emigrate;

	piece_by_type[empty] ^= bb_return;  // finish reversing piece

	// if capture != empty, update opponent pieces
	int black_to_move = white_to_move ^ 1;
	int is_not_capture = !(original_occupant == empty);
	piece_by_type[black_square + black_to_move] |= is_not_capture*emigrate;

	mailbox[undone.from] = reversing;
	mailbox[undone.to] = original_occupant;
}

// using macros to continue private access without needing to declare function in header
#define TOGGLE_BITBOARD_SQ( piece, sq)\
{\
	BB bb = (BB)1 << sq;\
	piece_by_type[piece] ^= bb;\
	piece_by_type[empty] ^= bb;\
	int is_white_piece = piece % 2;\
	piece_by_type[black_square + is_white_piece] ^= bb;\
	error("enpassant capture toggled");\
}
void model::empty_occupied_square(piece piece, sq_id sq)
{
	TOGGLE_BITBOARD_SQ( piece, sq);
	mailbox[sq] = empty;
}
void model::set_empty_square(piece piece, sq_id sq)
{
	TOGGLE_BITBOARD_SQ(piece, sq);
	mailbox[sq] = piece;
}

void model::move_promote(piece pawn, sq_id from, sq_id to, piece promotion)
{
	BB bb_from = (BB)1 << from;
	piece_by_type[pawn] ^= bb_from;

	BB bb_to = (BB)1 << to;
	piece_by_type[promotion] ^= bb_to;

	BB from_to = bb_from | bb_to;
	piece_by_type[black_square + white_to_move] ^= from_to;
	piece_by_type[empty] ^= from_to;

	mailbox[from] = empty;
	mailbox[to] = promotion;
}

void model::capture_promote(piece pawn, sq_id from, sq_id to, piece promotion)
{
	BB bb_from = (BB)1 << from;
	BB bb_to = (BB)1 << to;

	// defenders moves
	piece_by_type[pawn] ^= bb_from;
	piece_by_type[black_square + white_to_move] ^= bb_from | bb_to;

	// capture
	const piece captured = mailbox[to];
	piece_by_type[captured] ^= bb_to;
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] ^= bb_to;

	// to was already occupied
	piece_by_type[empty] ^= bb_from; 

	mailbox[from] = empty;
	mailbox[to] = promotion;
}

piece model::move_capture_promote(piece pawn, sq_id from, sq_id to, piece promotion)
{
	BB bb_from = (BB)1 << from;
	BB bb_to = (BB)1 << to;

	// push
	piece_by_type[pawn] ^= bb_from;
	piece_by_type[promotion] ^= bb_to;

	// update player's pieces
	piece_by_type[black_square + white_to_move] ^= bb_from | bb_to;

	// capture
	piece captured = mailbox[to];

	// if capture == empty, occupy empty
	// else,		remove captured_piece bit
	piece_by_type[captured] ^= bb_to;

	// occupy empty in anycase
	piece_by_type[empty] &= ~bb_to;
	piece_by_type[empty] ^= bb_from; // remove from starting location

	// if capture == empty, no changes
	// else,	remove opposing color bit
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] &= ~bb_to;

	mailbox[from] = empty;
	mailbox[to] = promotion;

	return captured;
}

piece model::reverse_move_capture_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion, piece captured)
{
	BB bb_reaquire = (BB)1 << reaquire;
	BB bb_expel = (BB)1 << expel;

	// return
	piece_by_type[pawn] ^= bb_reaquire;
	piece_by_type[promotion] ^= bb_expel;

	// update player's pieces
	piece_by_type[black_square + white_to_move] ^= bb_reaquire | bb_expel;

	// capture
	piece broke_captured = mailbox[expel]; // WARN: error, expel info was destroyed in mailbox

	// if capture == empty, occupy empty
	// else,		remove captured_piece bit
	piece_by_type[broke_captured] ^= bb_expel;

	// occupy empty in anycase
	piece_by_type[empty] &= ~bb_expel;
	piece_by_type[empty] ^= bb_reaquire; // remove from starting location

	// if capture == empty, no changes
	// else,		remove opposing color bit
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] &= ~bb_expel;

	mailbox[reaquire] = empty;
	mailbox[expel] = promotion;

	return broke_captured;
}

void model::reverse_move_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion) 
{

	BB bb_reaquire = (BB)1 << reaquire;
	BB bb_expel = (BB)1 << expel;

	piece_by_type[promotion] ^= bb_expel;
	piece_by_type[pawn] ^= bb_reaquire;
	piece_by_type[black_square + white_to_move] ^= bb_reaquire | bb_expel;
	piece_by_type[empty] ^= bb_reaquire | bb_expel;

	mailbox[reaquire] = pawn;
	mailbox[expel] = empty;
}

void model::reverse_capture_promote(piece pawn, half_move& previous, piece promotion, piece captured)
{
	BB reaquire = (BB)1 << previous.from;
	BB expel = (BB)1 << previous.to;

	piece_by_type[promotion] ^= expel;
	piece_by_type[pawn] ^= reaquire;
	piece_by_type[black_square + white_to_move] ^= reaquire | expel;

	piece_by_type[captured] ^= expel;
	int black_to_move = white_to_move ^ 1;
	piece_by_type[black_square + black_to_move] ^= expel;

	// destination square previously occupied
	piece_by_type[empty] ^= reaquire;

	mailbox[previous.from] = pawn;
	mailbox[previous.to] = captured;
}

BB model::get_BB(int piece_type) const { return piece_by_type[piece_type]; }



// ============================================================================================
// Constructors
// ============================================================================================



// sets piece_color and aggreagates color bitboards.
void set_white_and_black_home_rank(BB* piece_by_type, piece black_id, BB setup)
{
	int to_white_piece = 1;
	int white_id = black_id + to_white_piece;
	piece_by_type[white_square] |= piece_by_type[white_id] = setup;
	piece_by_type[black_square] |= piece_by_type[black_id] = setup << (COLS*(ROWS - 1));
}

void model::set_default_mailbox()
{
	const static std::array<piece, 64> default_board =
		{
			white_rook,white_knight,white_bishop,white_queen,white_king,white_bishop,white_knight,white_rook,
			white_pawn,white_pawn,white_pawn,white_pawn,white_pawn,white_pawn,white_pawn,white_pawn,

			empty,empty,empty,empty,empty,empty,empty,empty,
			empty,empty,empty,empty,empty,empty,empty,empty,
			empty,empty,empty,empty,empty,empty,empty,empty,
			empty,empty,empty,empty,empty,empty,empty,empty,

			black_pawn,black_pawn,black_pawn,black_pawn,black_pawn,black_pawn,black_pawn,black_pawn,
			black_rook,black_knight,black_bishop,black_queen,black_king,black_bishop,black_knight,black_rook
		};
	
	int mailbox_byte_count = default_board.size()*sizeof(piece);
	std::memcpy(mailbox, default_board.data(), mailbox_byte_count);
}

void model::set_default_board() 
{
	set_default_mailbox();

	// bitboard
	const BB rank = (BB)255;
	// instantiate pawn_color AND color
	piece_by_type[white_pawn] = rank << ROWS;
	piece_by_type[white_square] = piece_by_type[white_pawn];
	piece_by_type[black_pawn] = rank << (6*ROWS);
	piece_by_type[black_square] = piece_by_type[black_pawn];

	BB one = (BB)1;
	BB rooks = (one << 7) | one;
	set_white_and_black_home_rank(piece_by_type, black_rook, rooks);
	BB knights = (one << 6) | (one << 1);
	set_white_and_black_home_rank(piece_by_type, black_knight, knights);
	BB bishops = (one << 5) | (one << 2);
	set_white_and_black_home_rank(piece_by_type, black_bishop, bishops);
	BB king = (one << 4);
	set_white_and_black_home_rank(piece_by_type, black_king, king);
	BB queen = (one << 3);
	set_white_and_black_home_rank(piece_by_type, black_queen, queen);

	piece_by_type[empty] = ~(piece_by_type[white_square] | piece_by_type[black_square]);
}



model::model()
{
	// rooks
	slide_instructions[north] = { 0x0101010101010100, [](int square_index, BB board) -> BB { return board << square_index; }, bit_scan_forward };
	slide_instructions[east] = { 1, [](int square_index, BB board) -> BB { return 2 * ((board << (square_index | 7)) - (board << square_index)); }, bit_scan_forward };
	slide_instructions[south] = { 0x0080808080808080, [](int square_index, BB board) -> BB { return board >> (square_index ^ 63); }, bit_scan_reverse };
	slide_instructions[west] = { 1, [](int square_index, BB board) -> BB { return (board << square_index) - (board << (square_index & 56)); }, bit_scan_reverse };

	// bishops
	BB until_sq_known = 0;
	slide_instructions[north_east] = { until_sq_known, [](int square_index, BB board) -> BB { return board & ((BB)-2 << square_index); }, bit_scan_forward };
	slide_instructions[south_west] = { until_sq_known, [](int square_index, BB board) -> BB { return board & (((BB)1 << square_index) - 1); }, bit_scan_reverse };
	slide_instructions[north_west] = { until_sq_known, [](int square_index, BB board) -> BB { return board & ((BB)-2 << square_index); }, bit_scan_forward };
	slide_instructions[south_east] = { until_sq_known, [](int square_index, BB board) -> BB { return board & (((BB)1 << square_index) - 1); }, bit_scan_reverse };
}



// ============================================================================================
// Pawn Processing
// ============================================================================================



// slide until first hit, return hit piece type
// if first hit == empty, return garbage
BB ray_trace_until_piece(sq_id square, orient orient, BB target, model& model)
{
	slide_instructions& slide = model.slide_instructions[orient];
	BB line_from = slide.ray_trace(square, slide.tracer_board_initial);
	BB targets = line_from & target;
	sq_id first_hit = slide.bit_scan(targets);
	BB line_after = slide.ray_trace(first_hit, slide.tracer_board_initial);
	return line_from ^ line_after;
}


// is_ambusher_east, is_ambusher_west: always 0 or 1
bool is_en_passant_attack_pinned(model& model, BB occupied, sq_id passer, int is_ambusher_east, int is_ambusher_west)
{
	int black_to_move = model.white_to_move ^ 1;
	BB threat_rook = model.get_BB(black_rook + black_to_move);
	BB threat_queen = model.get_BB(black_queen + black_to_move);
	BB friend_king = model.get_BB(black_king + model.white_to_move);

	// identify adjacent pawn
	sq_id ambusher_passer_delta = is_ambusher_east - is_ambusher_west;
	sq_id ambusher = passer + ambusher_passer_delta;
	// is ambusher to east or west of passer
	int west_east_delta = west - east;
	orient ambusher_ray = (orient)(east + west_east_delta*is_ambusher_west);
	orient passer_ray = (orient)(east + west_east_delta*is_ambusher_east);

	// look for pieces outside en passant pair
	BB ray_from_ambusher = ray_trace_until_piece(ambusher, ambusher_ray, occupied, model);
	BB ray_from_passer = ray_trace_until_piece(passer, passer_ray, occupied, model);
print_bb(ray_from_ambusher, "ray_from_ambusher");
print_bb(ray_from_passer, "ray_from_passer");
	bool king_found 
		= ray_from_ambusher & friend_king 
		|| ray_from_passer & friend_king;
	bool threat_found 
		= ray_from_ambusher & threat_queen 
		|| ray_from_passer & threat_queen 
		|| ray_from_ambusher & threat_rook 
		|| ray_from_passer & threat_rook; 
	error(king_found, "king_found"); 
	error(threat_found, "threat_found");
	return !(king_found && threat_found);
}

bool is_passer_targeted_after_double_push(model& model, BB pawns, BB occupied, sq_id passer)
{
	BB passing_pawn = (BB)1 << passer;
	BB ambusher_east = pawns & model::not_h_file & (passing_pawn << 1);
	BB ambusher_west = pawns & model::not_a_file & (passing_pawn >> 1);
	int is_ambusher_east = ambusher_east > 0;
	int is_ambusher_west = ambusher_west > 0;
	
	int num_adjacent_pawns = is_ambusher_east + is_ambusher_west;
	switch(num_adjacent_pawns)
	{
	case 0:
		return false;
	case 1:
		// would capturing passer expose our king?
		return is_en_passant_attack_pinned(model, occupied, passer, is_ambusher_east, is_ambusher_west);
	case 2:
		// if pawns on both sides, capture can't reveal attack
		return true;
	}
}

BB get_en_passant_white(model& model, BB pawns, std::vector<half_move>& score_sheet) 
{
	const int min_to_enpassant = 4;
	if (score_sheet.size() >= min_to_enpassant)
	{
		half_move& previous_move = score_sheet.back();
		sq_id passer = previous_move.to;
		int move_distance =  previous_move.from - previous_move.to; // black moves top down
		int double_push = ROWS*2;
		if (model.get(passer) != black_pawn 
			|| move_distance != double_push)
		{
			return 0;
		}
		else 
		{
			BB occupied = ~model.get_BB(empty);
			BB intercept_square = (BB)1 << (passer + ROWS);
			bool is_en_passant = is_passer_targeted_after_double_push(model, pawns, occupied, passer);
			return intercept_square * is_en_passant;
		}
	}
	else
	{
		return 0;
	}
}


void white_pawn_moves(departure& operative, BB occupied, model& model, std::vector<half_move>& score_sheet)
{
	BB& moves = operative.squares;
	sq_id square = operative.from;
	BB pawn = (BB)1 << square;

	// push
	moves = pawn << ROWS;
	moves &= model.get_BB(empty);
	const BB fourth_rank = 0x00000000FF000000;
	moves |= model.get_BB(empty) & (moves << ROWS) & fourth_rank;

	// attacks 
	BB right_attack = model::not_a_file & occupied & (pawn << (ROWS + 1));
	BB left_attack = model::not_h_file & occupied & (pawn << (ROWS - 1));

	std::array<sq_id, 3>& triggers = operative.trigger_ids;
	int rank = get_rank(square);
	const int candadite_queen_rank{ 6 };

	BB en_passant_ambush = get_en_passant_white(model, pawn, score_sheet); 
	if (en_passant_ambush)
	{
		half_move& previous_move = score_sheet.back();
		triggers[operative.trigger_count++] = previous_move.to + ROWS;
		moves |= en_passant_ambush;
		operative.trigger_effect = en_passant;
		 
	} // end en passant 

	else if (rank == candadite_queen_rank)
	{
		sq_id up_one = square + ROWS;
		// push
		triggers[operative.trigger_count] = up_one;
		unsigned any_push = moves > 0;
		operative.trigger_count += any_push; // +1 to save +0 to forget
		// right
		triggers[operative.trigger_count] = up_one + 1;
		unsigned is_right_attack = right_attack > 0;
		operative.trigger_count += is_right_attack; 
		// left
		triggers[operative.trigger_count] = up_one - 1;
		unsigned is_left_attack = left_attack > 0;
		operative.trigger_count += is_left_attack; 

		// promote
		// if any	promote_queen
		// else		0 aka none_secondary_effect
		unsigned any_forword_step = operative.trigger_count > 0;
		operative.trigger_effect = (secondary_effect)((-1*any_forword_step) & promote_queen);
	}

	moves |= left_attack | right_attack;
} // end white pawn moves

BB get_en_passant_black(model& model, BB pawns, std::vector<half_move>& score_sheet) 
{
	const int min_to_enpassant = 4;
	if (score_sheet.size() >= min_to_enpassant)
	{
		half_move& previous_move = score_sheet.back();
		sq_id passer = previous_move.to;
		int move_distance =  previous_move.to - previous_move.from; // white moves bottom up
		int double_push = ROWS*2;
		if (model.get(passer) != white_pawn 
			|| move_distance != double_push)
		{
			return 0;
		}
		else
		{
			BB occupied = ~model.get_BB(empty);
			BB intercept_square = (BB)1 << (passer - ROWS);
			bool is_en_passant = is_passer_targeted_after_double_push(model, pawns, occupied, passer);
			return intercept_square * is_en_passant;
		}
	}
	else
	{
		return 0;
	}
}

void black_pawn_moves(departure& operative, BB occupied, model& model, std::vector<half_move>& score_sheet)
{
	std::array<sq_id, 3>& triggers = operative.trigger_ids;
	BB& moves = operative.squares;
	sq_id square = operative.from; // not reference
	BB pawn = (BB)1 << square;

	// push one
	moves = pawn >> ROWS;
	// confirm we can continue to slide	
	moves &= model.get_BB(empty);
	const BB fith_rank = 0x000000FF00000000;
	moves |= fith_rank & model.get_BB(empty) & (moves >> ROWS);

	// attacks
	// leave seperate for individual evaluation
	BB right_attack = model::not_h_file & occupied & (pawn >> (ROWS + 1));
	BB left_attack = model::not_a_file & occupied & (pawn >> (ROWS - 1));

	// rank specific moves
	const int candadite_queen_rank{ 1 };
	const int rank = get_rank(square);

	BB en_passant_ambush = get_en_passant_black(model, pawn, score_sheet); 
	if (en_passant_ambush)
	{
		half_move& previous_move = score_sheet.back();
		triggers[operative.trigger_count++] = previous_move.from + ROWS;
		moves |= en_passant_ambush;
		operative.trigger_effect = en_passant;
	} // end en passant 

	else if (rank == candadite_queen_rank)
	{
		sq_id down_one = square - ROWS;
		// push
		triggers[operative.trigger_count] = down_one;
		unsigned any_push = moves > 0;
		operative.trigger_count += any_push; // +1 to save +0 to forget
		// right
		triggers[operative.trigger_count] = down_one + 1;
		unsigned is_right_attack = right_attack > 0;
		operative.trigger_count += is_right_attack; 
		// left
		triggers[operative.trigger_count] = down_one - 1;
		unsigned is_left_attack = left_attack > 0;
		operative.trigger_count += is_left_attack; 

		// promote
		// if any	promote_queen
		// else		0 aka none_secondary_effect
		unsigned any_forword_step = any_push | is_right_attack | is_left_attack;
		operative.trigger_effect = (secondary_effect)((-any_forword_step) & promote_queen);
	}

	moves |= right_attack | left_attack;
} // black pawn moves

void model::white_pawn_departure_from_dest(departure& operative, sq_id dest, secondary_effect promotion, std::vector<half_move>& score_sheet)
{
	// operative.squares == potential departures
	BB pawn_dest = (BB)1 << dest;
	// push only into unoccupied
	operative.squares = pawn_dest & piece_by_type[empty]; 
	operative.squares >>= ROWS;
	// double push only through unoccupied
	BB double_push_intermezzo_sq = operative.squares & piece_by_type[empty]; 
	BB second_rank = 0x000000000000FF00;
	operative.squares |= second_rank & (double_push_intermezzo_sq >> ROWS);

	// pawn attacks
	BB opponent_occupancy = piece_by_type[black_square];
	BB dest_target = pawn_dest & opponent_occupancy;
	operative.squares |= model::not_h_file & (dest_target >> (ROWS + 1));
	operative.squares |= model::not_a_file & (dest_target >> (ROWS - 1));

	int queen_rank = 7;
	if (get_rank(dest) == queen_rank)
	{
		operative.trigger_ids[operative.trigger_count++] = dest;
		if (promotion != none_secondary_effect)
			operative.trigger_effect = promotion;
		else
			error("promoting pawn with no advancement selected");
	}
	else
	{
		BB pawns = piece_by_type[white_pawn];
		BB en_passant_ambush = get_en_passant_white(*this, pawns, score_sheet); 
		if (en_passant_ambush)
		{
			half_move& previous_move = score_sheet.back();
			operative.trigger_ids[operative.trigger_count++] = previous_move.to + ROWS;
			operative.squares |= model::not_h_file & (pawn_dest >> (ROWS + 1));
			operative.squares |= model::not_a_file & (pawn_dest >> (ROWS - 1));
			operative.trigger_effect = en_passant;
			error("white_pawn_departure_from_dest enpassant confirmed & instantiated");
		} // end en passant 
	}
}

void model::black_pawn_departure_from_dest(departure& operative, sq_id dest, secondary_effect promotion, std::vector<half_move>& score_sheet)
{
	// operative.squares == potential departures
	BB pawn_dest = (BB)1 << dest;
	// push only into unoccupied
	operative.squares = pawn_dest & piece_by_type[empty]; 
	operative.squares <<= ROWS;
	// double push only through unoccupied
	BB double_push_intermezzo_sq = operative.squares & piece_by_type[empty]; 
	BB seventh_rank = 0x00FF000000000000;
	operative.squares |= seventh_rank & (double_push_intermezzo_sq << ROWS);

	// pawn attacks
	BB opponent_occupancy = piece_by_type[white_square];
	BB dest_target = pawn_dest & opponent_occupancy;
	operative.squares |= model::not_a_file & (dest_target << (ROWS + 1));
	operative.squares |= model::not_h_file & (dest_target << (ROWS - 1));

	int queen_rank = 0;
	if (get_rank(dest) == queen_rank)
	{
		operative.trigger_ids[operative.trigger_count++] = dest;
		if (promotion != none_secondary_effect)
			operative.trigger_effect = promotion;
		else
			error("promoting pawn with no advancement selected");
	}
	else
	{
		BB pawns = piece_by_type[black_pawn];
		BB en_passant_ambush = get_en_passant_black(*this, pawns, score_sheet); 
		if (en_passant_ambush)
		{
			operative.squares |= model::not_a_file & (dest << (ROWS + 1));
			operative.squares |= model::not_h_file & (dest << (ROWS - 1));
			half_move& previous_move = score_sheet.back();
			operative.trigger_ids[operative.trigger_count++] = previous_move.from + ROWS;
			operative.trigger_effect = en_passant;
			error("black_pawn_departure_from_dest enpassant confirmed & instantiated");
		} // end en passant 
	}
}


// ============================================================================================
// Piece Processing
// ============================================================================================


/**
 * bitScanReverse
 * @authors Kim Walisch, Mark Dickinson
 * @return index (0..63) of first attack collision
 */
sq_id bit_scan_reverse(BB targets)
{
	// https://www.chessprogramming.org/BitScan

	const sq_id significant_bit_finder[64]{
		0, 47,  1, 56, 48, 27,  2, 60,
	   57, 49, 41, 37, 28, 16,  3, 61,
	   54, 58, 35, 52, 50, 42, 21, 44,
	   38, 32, 29, 23, 17, 11,  4, 62,
	   46, 55, 26, 59, 40, 36, 15, 53,
	   34, 51, 20, 43, 31, 22, 10, 45,
	   25, 39, 14, 33, 19, 30,  9, 24,
	   13, 18,  8, 12,  7,  6,  5, 63
	};
	const BB debruijn_sequence = 0x03f79d71b4cb0a89;
	// find first blocker
	targets |= targets >> 1;
	targets |= targets >> 2;
	targets |= targets >> 4;
	targets |= targets >> 8;
	targets |= targets >> 16;
	targets |= targets >> 32;
	return significant_bit_finder[(targets * debruijn_sequence) >> 58];
}

/**
 * bitScanForward
 * @author Martin Läuter (1997)
 *         Charles E. Leiserson
 *         Harald Prokop
 *         Keith H. Randall
 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
 * @return index (0..63) of first attack collision
 */
sq_id bit_scan_forward(BB targets)
{
	// https://www.chessprogramming.org/BitScan

	const sq_id significant_bit_finder[64]{
		0, 47,  1, 56, 48, 27,  2, 60,
	   57, 49, 41, 37, 28, 16,  3, 61,
	   54, 58, 35, 52, 50, 42, 21, 44,
	   38, 32, 29, 23, 17, 11,  4, 62,
	   46, 55, 26, 59, 40, 36, 15, 53,
	   34, 51, 20, 43, 31, 22, 10, 45,
	   25, 39, 14, 33, 19, 30,  9, 24,
	   13, 18,  8, 12,  7,  6,  5, 63
	};
	const BB debruijn_sequence = 0x03f79d71b4cb0a89;

	return significant_bit_finder[((targets ^ (targets - 1)) * debruijn_sequence) >> 58];
}

BB bishop_slide_get_initial_board(int shift, BB slash)
{
	bool condition = shift >= 0;
	switch (condition)
	{
	case 0: return slash << -shift * 8;
	case 1: return slash >> shift * 8;
	}
}

void model::prepare_bishop_sliding(sq_id square_index)
{
	// forward slash
	BB forwardslash = 0x0102040810204080;
	int forwardslash_shift = 7 - (square_index & 7) - (square_index >> 3); 
	BB forwardslash_attacks = bishop_slide_get_initial_board(forwardslash_shift, forwardslash);

	// backslash
	const BB backslash = 0x8040201008040201; 
	int backslash_shift = (square_index & 7) - (square_index >> 3);
	BB backslash_attacks = bishop_slide_get_initial_board(backslash_shift, backslash);

	slide_instructions[north_east].tracer_board_initial = forwardslash_attacks;
	slide_instructions[south_west].tracer_board_initial = forwardslash_attacks;
	slide_instructions[north_west].tracer_board_initial = backslash_attacks;
	slide_instructions[south_east].tracer_board_initial = backslash_attacks;
}


BB model::execute_slide(int start, int end, int square_index, BB occupied)
{
	BB attack_squares{};
	for (; start < end; start++) // each slide direction
	{
		// slide in direction from start_square
		BB from_piece = slide_instructions[start].ray_trace(square_index, slide_instructions[start].tracer_board_initial);

		BB targets = from_piece & occupied;
		sq_id least_significant_bit = slide_instructions[start].bit_scan(targets);

		// slide in direction from last bit collision
		BB from_block = slide_instructions[start].ray_trace(least_significant_bit, slide_instructions[start].tracer_board_initial);

		attack_squares |= from_piece ^ from_block;
	}
	return attack_squares;
}

BB model::knight_moves(BB knights)
{
	BB east, west;
	east = (knights << 1) & model::not_a_file;
	west = (knights >> 1) & model::not_h_file;
	BB out_moves = (east | west) << (ROWS * 2);
	out_moves |= (east | west) >> (ROWS * 2);
	east = (east << 1) & model::not_a_file;
	west = (west >> 1) & model::not_h_file;
	out_moves |= (east | west) << ROWS;
	out_moves |= (east | west) >> ROWS;
	return out_moves;
}

BB model::king_moves_standard(BB bb_king)
{
	bb_king 
		|= (model::not_a_file & (bb_king << 1))
		| (model::not_h_file & (bb_king >> 1)); // left right
	bb_king 
		|= (bb_king << COLS) 
		| (bb_king >> COLS); // up down
	return bb_king;
}

void model::get_any_castle(departure& operative, std::vector<half_move>& score_sheet)
{
	BB& king_attacks = operative.squares;

	// confirm conditions for castle
	int black_to_move{ white_to_move ^ 1 };
	int home_rank = black_to_move * (ROWS - 1) * COLS;
	sq_id king_home_square = home_rank + e;
	if (operative.from == king_home_square)
	{
		bool king_side_castle = true;
		bool queen_side_castle = true;

		// in-between squares vacant and not checked?
		// because ray_trace from rook -> king would require truncating the attack line after the king, 
		// it's easier to ray_trace king -> rook and allow slide_instructions to naturally stop at board edge.
		BB occupied = ~piece_by_type[empty];
		BB occupied_for_all_but_rook = occupied ^ piece_by_type[black_rook + white_to_move];
		BB obstacles = occupied_for_all_but_rook | operative.opponent_check_evadables;
		
		// check king side vacancy
		BB to_rook = slide_instructions[east].ray_trace(king_home_square, slide_instructions[east].tracer_board_initial);
		king_side_castle = (to_rook & obstacles) == 0;

		// check queen side vacancy
		to_rook = slide_instructions[west].ray_trace(king_home_square, slide_instructions[west].tracer_board_initial);
		queen_side_castle = (to_rook & obstacles) == 0;

		// king or rook moved?
		sq_id queenside_rook = home_rank + a;
		sq_id kingside_rook = home_rank + h;
		const sq_id our_color_king = black_king + white_to_move;

		for (int i{ black_to_move }; i < score_sheet.size(); i += 2)
		{
			const half_move& move = score_sheet[i];
			if (move.from == king_home_square)
			{
				king_side_castle = false;
				queen_side_castle = false;
				break;
			}
			else if (move.from == queenside_rook)
				queen_side_castle = false;
			else if (move.from == kingside_rook)
				king_side_castle = false;
		}
		

		// evalutate castle rights
		if (king_side_castle)
		{
			operative.trigger_effect = castle;
			int move_distance = 2;
			operative.trigger_ids[operative.trigger_count++] = king_home_square + move_distance;
			king_attacks |= (BB)1 << (operative.from + move_distance);
		}
		if (queen_side_castle)
		{
			operative.trigger_effect = castle;
			int move_distance = -2;
			operative.trigger_ids[operative.trigger_count++] = king_home_square + move_distance;
			king_attacks |= (BB)1 << (operative.from + move_distance);
		}
	} // end castle check
}

void king_moves(departure& operative, model& model, std::vector<half_move>& score_sheet)
{
	BB& king_attacks = operative.squares;
	king_attacks = (BB)1 << operative.from;

	king_attacks = model.king_moves_standard(king_attacks);
	king_attacks &= ~operative.opponent_check_evadables;	// don't move into check
	model.get_any_castle(operative, score_sheet);
} // end king_moves


// given a nonempty piece, instantiate legal moves. Piece must be of correct color-turn to move!
void model::get_legal_moves(departure& operative, std::vector<half_move>& score_sheet)
{
	int white_piece_selected = operative.piece % 2;
	if (white_to_move != white_piece_selected)
		return;

	BB occupied = ~piece_by_type[empty];
	BB& move_squares = operative.squares;
	BB checks_to_block = operative.opponent_check_blockables; // bitwise (1) if king selected

	switch (operative.piece)
	{
	case black_king:
	case white_king: 
		king_moves(operative, *this, score_sheet);
		checks_to_block = -1; // bitwise (1)
		break;
	case black_queen:
	case white_queen:
		prepare_bishop_sliding(operative.from);
		move_squares = execute_slide(north, max_orient, operative.from, occupied);
		break;
	case black_rook:
	case white_rook: 
		move_squares = execute_slide(north, west + 1, operative.from, occupied);
		break;
	case black_bishop:
	case white_bishop: 
		prepare_bishop_sliding(operative.from);
		move_squares = execute_slide(north_east, max_orient, operative.from, occupied);
		break;
	case black_knight:
	case white_knight: 
		// hijack occupied to store knight position
		occupied = (BB)1 << operative.from; 
		move_squares = knight_moves(occupied);
		break;
	case black_pawn:
		black_pawn_moves(operative, occupied, *this, score_sheet);
		break;
	case white_pawn: 
		white_pawn_moves(operative, occupied, *this, score_sheet);
		break;
	}


	// Remove illegal moves
	for (int pin{}; pin < operative.pin_count; pin++)
		if (operative.from == operative.pinned_id[pin])
			move_squares &= operative.pin_threat_line[pin];

	// block any checks (if king switch, bitwise one)
	move_squares &= checks_to_block;

	BB friendly_pieces = piece_by_type[black_square + white_to_move];
	move_squares = move_squares & ~friendly_pieces;
} // end get_legal_moves



// ============================================================================================
// Checkmate/Stalemate Detection
// ============================================================================================



BB south_attacks(BB rooks, BB empty)
{
	rooks |= empty & (rooks >> 8);
	empty &= (empty >> 8);
	rooks |= empty & (rooks >> 16);
	empty &= (empty >> 16);
	rooks |= empty & (rooks >> 32);
	// no wrap around
	return rooks >> ROWS;
}

BB north_attacks(BB rooks, BB empty)
{
	rooks |= empty & (rooks << 8);
	empty &= (empty << 8);
	rooks |= empty & (rooks << 16);
	empty &= (empty << 16);
	rooks |= empty & (rooks << 32);
	// no wrap around
	return rooks << ROWS;
}

BB east_attacks(BB rooks, BB empty)
{
	empty &= model::not_a_file;
	rooks |= empty & (rooks << 1);
	empty &= (empty << 1);
	rooks |= empty & (rooks << 2);
	empty &= (empty << 2);
	rooks |= empty & (rooks << 4);
	// no A file, so no wrap around
	return rooks << 1;
}

BB west_attacks(BB rooks, BB empty)
{
	empty &= model::not_h_file;
	rooks |= empty & (rooks >> 1);
	empty &= (empty >> 1);
	rooks |= empty & (rooks >> 2);
	empty &= (empty >> 2);
	rooks |= empty & (rooks >> 4);
	// no H file, so no wrap around
	return rooks >> 1;
}


BB north_east_attacks(BB bishops, BB empty)
{
	empty &= model::not_h_file;
	bishops |= empty & (bishops << 9);
	empty &= (empty << 9);
	bishops |= empty & (bishops << 18);
	empty &= (empty << 18);
	bishops |= empty & (bishops << 36);
	return bishops << (ROWS + 1);
}

BB south_east_attacks(BB bishops, BB empty)
{
	empty &= model::not_h_file;
	bishops |= empty & (bishops >> 7);
	empty &= (empty >> 7);
	bishops |= empty & (bishops >> 14);
	empty &= (empty >> 14);
	bishops |= empty & (bishops >> 28);
	return bishops >> (ROWS - 1);
}

BB south_west_attacks(BB bishops, BB empty)
{
	empty &= model::not_a_file;
	bishops |= empty & (bishops >> 9);
	empty &= (empty >> 9);
	bishops |= empty & (bishops >> 18);
	empty &= (empty >> 18);
	bishops |= empty & (bishops >> 36);
	return bishops >> (ROWS + 1);
}

BB north_west_attacks(BB bishops, BB empty)
{
	empty &= model::not_a_file;
	bishops |= empty & (bishops << 7);
	empty &= (empty << 7);
	bishops |= empty & (bishops << 14);
	empty &= (empty << 14);
	bishops |= empty & (bishops << 28);
	return bishops << (ROWS - 1);
}

BB all_sliding_attacks(BB rook_q, BB bishop_q, BB empty)
{
	BB moves;
	moves = north_attacks(rook_q, empty);
	moves |= east_attacks(rook_q, empty);
	moves |= south_attacks(rook_q, empty);
	moves |= west_attacks(rook_q, empty);

	moves |= north_east_attacks(bishop_q, empty);
	moves |= south_east_attacks(bishop_q, empty);
	moves |= south_west_attacks(bishop_q, empty);
	moves |= north_west_attacks(bishop_q, empty);
	return moves;
}

bool any_sliding_attacks(BB rook_q, BB bishop_q, BB empty)
{
	BB east_moves, west_moves;
	east_moves |= bishop_q << (ROWS + 1);
	east_moves |= bishop_q >> (ROWS - 1);
	west_moves |= bishop_q >> (ROWS + 1);
	west_moves |= bishop_q << (ROWS - 1);

	west_moves |= rook_q << 1;
	east_moves |= rook_q >> 1;

	east_moves &= model::not_a_file;
	west_moves &= model::not_h_file;

	BB moves = east_moves | west_moves;
	moves |= rook_q << ROWS;
	moves |= rook_q >> ROWS;
	moves &= empty;
	print_bb(moves);
	return moves;
}

// instantiate evadables, blockables, pinned_id/threatline, pin_count, and return pinned squares
BB aggregate_pins_and_sliding_blockables(int start, int end, model& model, BB threat_squares, departure& operative, sq_id king_sq)
{
	BB friendly_pieces = model.get_BB(black_square + model.white_to_move);
	BB occupied = ~model.get_BB(empty);
	BB pinned_squares{};
	for (; start < end; start++)
	{
		slide_instructions& calc = model.slide_instructions[start];

		// slide in direction from start_square
		BB from_king = calc.ray_trace(king_sq, calc.tracer_board_initial);
		BB targets = from_king & occupied;
		const sq_id first_blocker = calc.bit_scan(targets);
		BB from_first_block = calc.ray_trace(first_blocker, calc.tracer_board_initial);
		BB to_first_block = from_king ^ from_first_block;


		// verify if check should be blocked
		bool is_check = to_first_block & threat_squares;
		BB to_first_check_else_nothing; 
		switch (is_check)
		{
		case 0:
			// do nothing
			to_first_check_else_nothing = operative.opponent_check_blockables; break;
		case 1:
			to_first_check_else_nothing = to_first_block; break;
		}
		// if one & only check store, else not blockable
		operative.opponent_check_blockables &= to_first_check_else_nothing; 


		// continue sliding in direction from first_block
		targets = occupied & from_first_block;
		const sq_id second_blocker = calc.bit_scan(targets);
		BB from_second_blocker = calc.ray_trace(second_blocker, calc.tracer_board_initial);
		BB to_second_block = from_first_block ^ from_second_blocker;


		BB intermediate_friend = to_first_block & friendly_pieces;
		bool is_pinned 
			= (bool)intermediate_friend
			& (bool)(to_second_block & threat_squares);  // attacker for pin

		pinned_squares ^= is_pinned*intermediate_friend;
		// from_king sufficient becuase intermediate_friend can't move overtop attacker anyway
		// pins only collected if pin_count increments
		operative.pin_threat_line[operative.pin_count] = from_king; 
		operative.pinned_id[operative.pin_count] = first_blocker;
		operative.pin_count += is_pinned; // increment if pinned 
	}
	return pinned_squares;
} // end aggregate_pins_and_sliding_blockables

BB white_pawn_blocks(model& model, BB pawns, std::vector<half_move>& score_sheet)
{
	// pawn push
	BB out_pawn_moves = model.get_BB(empty) & (pawns << ROWS); 
	BB fourth_rank = 0x00000000FF000000;
	out_pawn_moves |= model.get_BB(empty) & (out_pawn_moves << ROWS) & fourth_rank;

	// pawn attacks
	BB pawn_attacks = model::not_a_file & (pawns << (ROWS + 1));
	pawn_attacks |= model::not_h_file & (pawns << (ROWS - 1));
	BB opponent_occupancy = model.get_BB(black_square);
	out_pawn_moves |= pawn_attacks & opponent_occupancy;

	// pawn passant
	BB en_passant_ambush = get_en_passant_white(model, pawns, score_sheet); 
	out_pawn_moves |= en_passant_ambush;
	
	// we don't need promotion to block check/stalemate
	return out_pawn_moves;
}

BB black_pawn_blocks(model& model, BB pawns, std::vector<half_move>& score_sheet)
{
	// pawn push
	BB out_pawn_moves = model.get_BB(empty) & (pawns >> ROWS); 
	BB fith_rank = 0x0000000FF0000000;
	out_pawn_moves |= model.get_BB(empty) & (out_pawn_moves >> ROWS) & fith_rank;

	// pawn attacks
	BB pawn_attacks = model::not_h_file & (pawns >> (ROWS + 1));
	pawn_attacks |= model::not_a_file & (pawns >> (ROWS - 1));
	BB opponent_occupancy = model.get_BB(white_square);
	out_pawn_moves |= pawn_attacks & opponent_occupancy; 

	// pawn passant
	BB en_passant_ambush = get_en_passant_black(model, pawns, score_sheet); 
	out_pawn_moves |= en_passant_ambush;

	// we don't need promotion to block check/stalemate
	return out_pawn_moves;
}

// find all pins and attacks/checks from opponent
// return all unpinned squares
BB model::instantiate_pins_checks(departure& operative)
{
	int black_to_move = white_to_move ^ 1; // find opposite color pieces

	// sliding piece blockable checks
	const BB threat_rooks{ piece_by_type[black_queen + black_to_move] | piece_by_type[black_rook + black_to_move] };
	const BB threat_bishops = piece_by_type[black_queen + black_to_move] | piece_by_type[black_bishop + black_to_move];
	const BB friend_king = piece_by_type[black_king + white_to_move];
	sq_id king = bit_scan_forward(friend_king);

	// rook pinnings
	BB pinned_squares = aggregate_pins_and_sliding_blockables(north, west + 1, *this, threat_rooks, operative, king);
	// bishop pinnings
	prepare_bishop_sliding(king);
	pinned_squares |= aggregate_pins_and_sliding_blockables(north_east, max_orient, *this, threat_bishops, operative, king);
	// pinned pieces can't block check, ignore them
	BB unpinned_squares{ ~pinned_squares };

		
	// Nonsliding piece blockable checks
	// if any check occurs, we need all knights to assess checkmate anyway
	BB all_knights = piece_by_type[black_knight + black_to_move]; 
	BB all_knight_attacks = knight_moves(all_knights); 
	if (friend_king & all_knight_attacks) // check
	{
		// research for exact knight position
		BB knight_check{};
		knight_check = knight_moves(friend_king);
		BB offending_knight = knight_check & piece_by_type[black_knight + black_to_move];
		// blockables lost if more than one piece to block
		operative.opponent_check_blockables &= offending_knight; 
	}

	BB all_pawn_attacks{};
	// if any check occurs, we need all pawns to assess checkmate anyway
	const BB opposing_pawns = piece_by_type[black_pawn + black_to_move];
	switch (white_to_move)
	{
	case 0:
		all_pawn_attacks |= model::not_a_file & (opposing_pawns << (ROWS + 1)); // opponent white pawns
		all_pawn_attacks |= model::not_h_file & (opposing_pawns << (ROWS - 1));
		if (friend_king & all_pawn_attacks) 
		{
			// find offending_pawn -> reverse pawn attacks from king
			BB offending_pawn{};
			offending_pawn |= model::not_h_file & (friend_king >> (ROWS + 1));	// black king finds pawn attacks
			offending_pawn |= model::not_a_file & (friend_king >> (ROWS - 1));
			offending_pawn &= opposing_pawns;
			// blockables lost if more than one piece to block 
			operative.opponent_check_blockables &= offending_pawn; 
		}
		break;
	case 1:
		all_pawn_attacks |= model::not_h_file & (opposing_pawns >> (ROWS + 1)); // opponent black pawns
		all_pawn_attacks |= model::not_a_file & (opposing_pawns >> (ROWS - 1));
		if (friend_king & all_pawn_attacks) 
		{
			// find offending_pawn -> reverse pawn attacks from king
			BB offending_pawn{};
			offending_pawn |= model::not_a_file & (friend_king << (ROWS + 1)); // white king finds pawn attacks
			offending_pawn |= model::not_h_file & (friend_king << (ROWS - 1));
			offending_pawn &= opposing_pawns;
			// blockables lost if more than one piece to block 
			operative.opponent_check_blockables &= offending_pawn; 
		}
		break;
	}

	// aggregate all opponent attacks
	BB& danger_squares = operative.opponent_check_evadables;

	// king pseudo moves are acceptable because only defending king can move into check here
	danger_squares = king_moves_standard(piece_by_type[black_king + black_to_move]); 

	// remove king in sliding piece calculation for king's check escape: king can't retreat into the attack's shadow
	BB empty_kingless = piece_by_type[empty] ^ friend_king;
	BB sliding_attacks = all_sliding_attacks(threat_rooks, threat_bishops, empty_kingless);

	danger_squares |= sliding_attacks;
	danger_squares |= all_knight_attacks;
	danger_squares |= all_pawn_attacks;

	return unpinned_squares;
}


// After a move, what squares contain a threat against the opponents king, and on what squares is the opponent pinned?
victory_state model::pins_checks_checkmate_stalemate(departure& operative, sq_id to, std::vector<half_move>& score_sheet)
{
	// To find check:	 all attacks
	// To find is_stalemate: all attacks, unpinned defense, pinned defense
	// To find is_checkmate: all attacks, unpinned defense, pinned defense, squares to block check
	// don't worry about oppenent pins, all attacks are only for immediate king saftey
	// Meanwhile we can aggreagate evadables and nonsliding blockables for get_legal_moves next turn

	BB unpinned_squares = instantiate_pins_checks(operative);
	BB& danger_squares = operative.opponent_check_evadables;
	const BB friendly_pieces = piece_by_type[black_square + white_to_move];
	
	// start with king moves
	BB illegal_king_moves = danger_squares | friendly_pieces;
	BB defended_squares = king_moves_standard(piece_by_type[black_king + white_to_move]) & ~illegal_king_moves; 

	victory_state result; 
	if (piece_by_type[black_king + white_to_move] & danger_squares) // check
	{
		result = checkmate;
	
		// average 4.9 checks per game: https://trueelo.app/stats/checks
		if (defended_squares & ~danger_squares > 0) // can evade
		{
			result = safe;
		}
		else
		{
			// can pieces block/capture check?
			BB friend_rooks = piece_by_type[black_rook + white_to_move] | piece_by_type[black_queen + white_to_move];
			friend_rooks &= unpinned_squares;
			BB friend_bishops = piece_by_type[black_bishop + white_to_move] | piece_by_type[black_queen + white_to_move];
			friend_bishops &= unpinned_squares;
			defended_squares |= all_sliding_attacks(friend_rooks, friend_bishops, piece_by_type[empty]);

			BB knights = piece_by_type[black_knight + white_to_move];
			knights &= unpinned_squares;
			defended_squares |= knight_moves(knights);

			if (defended_squares & operative.opponent_check_blockables) // piece can block
			{
				result = safe;
			}
			else
			{
				// can pawns block/capture check?
				BB pawns = piece_by_type[black_pawn + white_to_move];
				pawns &= unpinned_squares;
				BB pawn_moves;
				if (white_to_move)
					pawn_moves = white_pawn_blocks(*this, pawns, score_sheet);
				else	
					pawn_moves = black_pawn_blocks(*this, pawns, score_sheet);
				defended_squares |= pawn_moves;

				if (defended_squares & operative.opponent_check_blockables)
				{
					result = safe;
				}
				// else pinned pieces cannot block/capture check
			} // end pawn block
		} // end piece block
	} // end check evasion

	else 
	{
		result = stalemate;

		if (defended_squares) 
		{
			// king moves?
			result = safe;
		}
		else
		{
			// any sliding pieces?
			BB friend_rooks = piece_by_type[black_rook + white_to_move] | piece_by_type[black_queen + white_to_move];
			friend_rooks &= unpinned_squares;
			BB friend_bishops = piece_by_type[black_bishop + white_to_move] | piece_by_type[black_queen + white_to_move];
			friend_bishops &= unpinned_squares;

			if (any_sliding_attacks(friend_rooks, friend_bishops, piece_by_type[empty]))
			{
				result = safe;
			}
			else
			{
				// any knights / pawns?
				BB knights = piece_by_type[black_knight + white_to_move];
				knights &= unpinned_squares;
				defended_squares |= knight_moves(knights);

				BB pawns = piece_by_type[black_pawn + white_to_move];
				pawns &= unpinned_squares;
				if (white_to_move)
					defended_squares |= white_pawn_blocks(*this, pawns, score_sheet);
				else	
					defended_squares |= black_pawn_blocks(*this, pawns, score_sheet);

				if (defended_squares & operative.opponent_check_blockables)
				{
					result = safe;
				}
				else
				{
					for (int pin{}; pin < operative.pin_count; pin++)
					{
						sq_id pinned_sq = operative.pinned_id[pin];
						piece pinned = mailbox[pinned_sq];
						BB occupied = ~piece_by_type[empty];
						BB placement;
						BB pin_leeway{};
						switch (pinned)
						{
							case black_queen:
							case white_queen:
								prepare_bishop_sliding(pinned_sq);
								pin_leeway = execute_slide(north, max_orient, pinned_sq, occupied);
								break;
							case black_rook:
							case white_rook:
								pin_leeway = execute_slide(north, west + 1, pinned_sq, occupied);
								break;
							case black_bishop:
							case white_bishop:
								prepare_bishop_sliding(pinned_sq);
								pin_leeway = execute_slide(north_west, max_orient, pinned_sq, occupied);
								break;
							case black_knight:
							case white_knight:
								placement = (BB)1 << pinned_sq; 
								pin_leeway = knight_moves(occupied);
								break;
							case black_pawn:
								placement = (BB)1 << pinned_sq;
								pin_leeway = black_pawn_blocks(*this, placement, score_sheet);
								break;
							case white_pawn:
								placement = (BB)1 << pinned_sq;
								pin_leeway = white_pawn_blocks(*this, placement, score_sheet);
								break;
						}

						pin_leeway &= operative.pin_threat_line[pin]; // must continue to block
						if (pin_leeway & operative.opponent_check_blockables)
							result = safe;
					} // end find any pin_leeway
				} // end knight/pawns vs stalemate
			} // end sliding pieces vs stalemate
		} // end king vs stalemate
	} // end stalemate detection

	return result;
} // pins_checks_checkmate_stalemate


victory_state model::is_time_insufficient_material()
{
	// defender lost on time, oppenent is victor to verify
	int black_to_move = white_to_move^1;
	if (piece_by_type[black_rook + black_to_move] 
		|| piece_by_type[black_queen + black_to_move]
		|| piece_by_type[black_pawn + black_to_move]
	)
	     return win_on_time;
	// TODO: pawns win, but future should verify if pawn can MOVE later

	/*
	* draw if:
	* nil_pieces
	* 1[bishop | knight]
	* 2*knight vs opponent_king + any_piece
	*/
	BB pieces = piece_by_type[black_bishop + black_to_move];
	int bishop_count{};
	while (pieces)
	{
		bishop_count++;
		pieces &= pieces - 1;
	}
	pieces = piece_by_type[black_knight + black_to_move];
	int knight_count{};
	while (pieces)
	{
		knight_count++;
		pieces &= pieces - 1;
	}

	if (bishop_count + knight_count <= 1)
	{
		return draw_on_time;
	}
	else if (knight_count == 2)
	{
		// opponent knight pair can win if defender has a piece
		bool any_friend_piece 
			= piece_by_type[black_queen + white_to_move]
			|| piece_by_type[black_rook + white_to_move]
			|| piece_by_type[black_bishop + white_to_move]
			|| piece_by_type[black_knight + white_to_move]
			|| piece_by_type[black_pawn + white_to_move];
		return any_friend_piece 
			? win_on_time 
			: draw_on_time; 
	}
	else
	{
		// sufficient knight/bishops
		return win_on_time;
	}
} // end is_time_insufficient_material
