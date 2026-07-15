#include <iostream> // for testing
#include <vector>
#include <array>
#include "timer.h"


#ifndef MODEL_ENUMS_AND_FILES 
#define MODEL_ENUMS_AND_FILES

#define MAX_SQUARES_OF_SECONDARY_EFFECTS_PER_DEPARTURE_VIA_PROMOTION 3
#define MAX_DIRECTIONS 8 
#define MAX_QUEEN_MOVES MAX_DIRECTIONS/2*(8 - 1) 
#define ROWS 8
#define COLS 8

// white and black types must be symetrical to access with pieces[black_xxxx + isWhite]
enum piece
{
	black_king,
	white_king,

	black_queen,
	white_queen,

	black_rook,
	white_rook,

	black_bishop,
	white_bishop,

	black_knight,
	white_knight,

	black_pawn,
	white_pawn,

	black_square,	// texture, also all black pieces 
	white_square,	// texture, also all white pieces 

	empty
};

enum secondary_effect
{
	none_secondary_effect,
	castle,
	en_passant,
	// fifty_move_rule
	// threefold_repitition
	// promotion list must be last for view_model.back_move selection normalization

	promote_queen,
	promote_rook,
	promote_bishop,
	promote_knight,
};

enum victory_state
{
	safe,
	check,
	checkmate,
	stalemate,

	// dead_position,
	// threefold_repitition,
	// draw_by_agreement,
	// game_fifty_move_rule,
	// game_threefold_repitition,
	// draw_on_time,
	// win_on_time,
	// resignation
	// draw_by_resignation

	max_victory_state
};

enum opening
{
	none_opening,
};



enum file : int {
	a, b, c, d, e, f, g, h, max_file
};

// index of square on board, agnostic of placement on screen. file = sq_id%8, rank = sq_id/8
typedef int sq_id;

struct half_move
{
	sq_id from{};
	sq_id to{};
	piece captured{};
	secondary_effect effect{ none_secondary_effect };
	half_move() {}
	half_move(sq_id from, sq_id to, piece captured, secondary_effect effect) : captured{ captured }, from { from }, to{ to }, effect{ effect } {}
};


// 64 bitboard
typedef unsigned long long BB; 

// direction of sliding piece movement
enum orient 
{ 
	north,
	east,
	south,
	west,

	north_east,
	south_east,
	south_west,
	north_west,

	max_orient 
};

struct slide_instructions
{
	BB tracer_board_initial{};
	BB (*ray_trace)(int index, BB board);
	int (*bit_scan)(BB targets);
};

// the piece picked up by user
struct departure
{
	std::array<BB, MAX_DIRECTIONS> pin_threat_line; 
	std::array<sq_id, MAX_DIRECTIONS> pinned_id;

	// destinations creating secondary effects (ex. en passant captures adjacent pawn)
	std::array<sq_id, MAX_SQUARES_OF_SECONDARY_EFFECTS_PER_DEPARTURE_VIA_PROMOTION> trigger_ids{};
	BB opponent_check_evadables{};				// all opponent attack lines for king to dodge
	BB opponent_check_blockables{ ~(BB)0 };			// threat line including piece & attacks for capture/intercept. Initialize to bitwise one 
	BB squares{};						// attack for selected piece

	int trigger_count{};
	int pin_count{};

	sq_id from{};
	piece piece{ empty };
	secondary_effect trigger_effect{ none_secondary_effect };
};

class model {
public: 
	std::array<slide_instructions, MAX_DIRECTIONS> slide_instructions; // how to move rook and/or bishop
private:
	piece mailbox[64]{}; // piece type per square
	BB piece_by_type[15]; // each piece_type of each color, all_black, all_white, empty

public:
	// records history for piece events such as castle or en passant
	// zero initialize so score_sheet checks can't out-of-bounds on move 0
	std::vector<half_move> score_sheet{ half_move() };

	static const BB not_a_file{ 0xfefefefefefefefe };
	static const BB not_h_file{ 0x7f7f7f7f7f7f7f7f };

	// count of total pieces moved, not move count for players
	// turn count will initialize to one during standard game
	// int turn_count{};			


	inline piece get(int file, int rank) const
	{
		sq_id index = ROWS * rank + file; // align file to bit, rank to byte
		return mailbox[index];
	}
	inline piece get(sq_id square) const { return mailbox[square]; }
	inline BB get_BB(int piece_type) const;

	void move_piece(piece move, sq_id from, sq_id to);
	void capture_piece(piece piece, sq_id from, sq_id to);
	void reverse_capture_piece(piece migrant, sq_id restore, sq_id expel, enum piece captured);
	// executes player move, and resolves captures if any
	piece move_capture_piece(piece move, sq_id from, sq_id to);
	piece move_capture_promote(piece pawn, sq_id from, sq_id to, piece promotion);
	void reverse_move_capture_piece(half_move& undone);
	piece reverse_move_capture_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion);
	void reverse_move_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion);
	void reverse_capture_promote(piece pawn, half_move& previous, piece promotion);
	/// <param name="piece">must not be an empty, black square or white square</param>
	/// <param name="sq">must be occupied by the piece argument</param>
	void toggle_nonempty_piece(piece piece, sq_id sq);

	BB instantiate_pins_checks(departure& operative);
	victory_state pins_checks_checkmate_stalemate(departure& operative, sq_id to);
	void get_legal_moves(departure& operative);


	// initialize board with starting position
	void set_defualt_board();
	void reset_score_sheet();
	model();
};

inline int get_file(sq_id index) { return index % ROWS; }			 // files are divided by rank
inline int get_rank(sq_id index) { return index / COLS; }			// ranks are divided by file

sq_id bit_scan_forward(BB targets);
sq_id bit_scan_reverse(BB targets);

void print_bb(BB bitboard);
void print_bb(BB bitboard, const char* message);
void print_mailbox(model& model);
void error(const char* message);
void error(const char* message, bool condition);
void error(piece value);
void error(piece value, const char* message);
template <typename T> void error(T value);
template <typename T> void error(T value, bool condition);
template <typename T> void error(T print, const char* label);
template <typename T> void error(T print, const char* label, bool condition);
#endif
