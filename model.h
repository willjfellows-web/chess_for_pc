#include <iostream> // for testing
#include <vector>
#include <array>
#include <cstring>
#include "timer.h"

#ifndef MODEL_ENUMS_AND_FILES 
#define MODEL_ENUMS_AND_FILES

#define MAX_SQUARES_OF_SECONDARY_EFFECTS_PER_DEPARTURE_VIA_PROMOTION 3
#define MAX_DIRECTIONS 8 
#define MAX_THREAT_LINE_LENGTH (ROWS - 1)
#define MAX_QUEEN_MOVES MAX_DIRECTIONS/2*MAX_THREAT_LINE_LENGTH 
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
	// promotion list must be last for view_model.back_move selection normalization

	promote_queen,
	promote_rook,
	promote_bishop,
	promote_knight,
};

enum victory_state : unsigned short
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
	draw_on_time,
	win_on_time,
	// resignation
	// draw_by_resignation

	max_victory_state
};

enum opening : unsigned short
{
	none_opening
};

// index of square on board, agnostic of placement on screen. file = sq_id%8, rank = sq_id/8
typedef int sq_id;

struct half_move
{
	sq_id from{};
	sq_id to{};
	secondary_effect effect{ none_secondary_effect };
	half_move() {}
	half_move(sq_id from, sq_id to, secondary_effect effect) : from { from }, to{ to }, effect{ effect } {}
};



enum file : int {
	a, b, c, d, e, f, g, h, max_file
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

class model 
{
public: 
	std::array<slide_instructions, MAX_DIRECTIONS> slide_instructions; // how to move rook and/or bishop
private:
	piece mailbox[64]{}; // piece type per square
	BB piece_by_type[15]; // each piece_type of each color, all_black, all_white, empty

public:
	void print_mailbox();
	static const BB not_a_file{ 0xfefefefefefefefe };// implimentation detail
	static const BB not_h_file{ 0x7f7f7f7f7f7f7f7f };// implimentation detail
	int white_to_move{ 1 };


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
	// move_capture executes player move, and resolves captures if any
	piece move_capture_piece(piece move, sq_id from, sq_id to);
	piece move_capture_promote(piece pawn, sq_id from, sq_id to, piece promotion);
	void reverse_move_capture_piece(half_move& undone, piece original_occupant);
	piece reverse_move_capture_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion, piece captured);
	void move_promote(piece pawn, sq_id from, sq_id to, piece promotion);
	void capture_promote(piece pawn, sq_id from, sq_id to, piece promotion);
	void reverse_move_promote(piece pawn, sq_id reaquire, sq_id expel, piece promotion);
	void reverse_capture_promote(piece pawn, half_move& previous, piece promotion, piece captured);
	/// flips sq index in bitboards/mailbox to empty
	/// <param name="piece">must not be an empty, black square or white square</param>
	/// <param name="sq">must be occupied by the piece argument</param>
	void empty_occupied_square(piece piece, sq_id sq);
	/// flips sq index in bitboards/mailbox to piece
	void set_empty_square(piece piece, sq_id sq);


	// returns adjacent squares/diagonals. doesn't castle, escape check, or countermand any caller "always block check" instructions
	BB king_moves_standard(BB kings); 
	// if all castle rules pass, instantiates castle secondary_effect, flips castle destination BB squares, and appends to secondary effect trigger_ids. 
	// argument operative.from must be instantiated
	void get_any_castle(departure& operative, std::vector<half_move>& score_sheet); 
	BB knight_moves(BB knights);
	void prepare_bishop_sliding(sq_id square_index); // instantiates model.slide_instructions
	// returns aggregated attack squares for a given range of directions in slide_instructions
	// DEPENDENT on prepare_bishop_sliding() if diagonals used 
	BB execute_slide(int start, int end, int square_index, BB occupied);
	void get_legal_moves(departure& operative, std::vector<half_move>& score_sheet);
	// instantiate pins, evadables/blockables from opponent against defender
	BB instantiate_pins_checks(departure& operative);
	// return victory state from opponent against defender
	victory_state pins_checks_checkmate_stalemate(departure& operative, sq_id to, std::vector<half_move>& score_sheet);
	// for elapsed clock, return draw or win by time
	victory_state is_time_insufficient_material();

	// white/black_pawn_departure_from_dest:
	// instantiates operative.squares with potential departures
	// as well as the trigger_ids of any secondary effects
	void black_pawn_departure_from_dest(departure& operative, sq_id dest, secondary_effect promotion, std::vector<half_move>& score_sheet);
	void white_pawn_departure_from_dest(departure& operative, sq_id dest, secondary_effect promotion, std::vector<half_move>& score_sheet);

	// initialize board with starting position
	void set_default_board();
	void set_default_mailbox();
	void reset_score_sheet();
	model();
};


inline int get_file(sq_id index) { return index % ROWS; }			 // files are divided by rank
inline int get_rank(sq_id index) { return index / COLS; }			// ranks are divided by file

sq_id bit_scan_forward(BB targets);
sq_id bit_scan_reverse(BB targets);

void error(BB bitboard);
void error(BB bitboard, const char* message);
void error(model& model);
void error(const char* message);
void error(const char* message, bool condition);
void error(const half_move* move);
void print_half_move(const half_move* move);
void error(piece value);
void error(piece value, const char* message);
void error(secondary_effect effect);
template <typename T> void error(T value) { std::cout << value << '\n'; }
template <typename T> void error(T value, bool condition) { if (condition) std::cout << value << '\n'; }
template <typename T> void error(T value, const char* label) { std::cout << label << ": " << value << '\n'; }
template <typename T> void error(T value, const char* label, bool condition) { if (condition) std::cout << label << ": " << value << '\n'; }
#endif
