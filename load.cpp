#include "load.h"
#include <cstring>

// ----------------------------------------------------------------------
// Input
// ----------------------------------------------------------------------

/// <summary>
/// checks characters equality from left to right
/// </summary>
bool str_equals_until(const char* str1, const char* str2, char delimiter)
{
	int i{};
	char c1{ str1[i] };
	char c2{ str2[i] };
	while (
		c1 != '\0' 
		&& c2 != '\0' 
		&& c1 != delimiter 
		&& c2 != delimiter
		) {

		if (c1 != c2)
			return false;
		i++;
		c1 = str1[i];
		c2 = str2[i];
	} 

	return true;
}

/// <summary>
/// parses file names to find piece id. ex) ./Images\\black_king.png -> piece::black_king.
/// </summary>
/// <param name="images_dir_count">folder name containing all images, INCLUDING trailing '/'</param>
/// <param name="image_name">full path name of image</param>
piece get_piece(int images_dir_len_count, const char* image_name)
{
	// piece name -> index for black. 
	// if white, add one to find id
	std::string names[] = { "king", "queen", "rook", "bishop", "knight", "pawn", "square" }; // ordered exactly as 'piece' enum
	constexpr int names_len = sizeof(names)/sizeof(names[0]);

	// search table
	piece found{ empty };

	// check name prefix for coloration
	const char* prefix_range_begin = &image_name[images_dir_len_count]; // image is now formated as xxxxx_name.xxx
	std::string black = "black_";
	std::string white = "white_";
	bool isWhite { 0 == strncmp(white.c_str(), prefix_range_begin, white.length()) };
	bool isBlack{ 0 == strncmp(black.c_str(), prefix_range_begin, black.length()) };
	if (isWhite || isBlack) 
	{
		const char* name_begin = &image_name[images_dir_len_count + black.length()];  // image is now name.xxx
		char extension_front = '.';
		for (int type{}; type < names_len; type++)
		{
			std::string name = names[type];
			if (str_equals_until(name.c_str(), name_begin, extension_front))
			{
				// enum piece types ordered in black_pawn, white_pawn, black_knight, white_knight ect... 
				const int color_count = 2;			// convert unique pieces to enum piece
				int id = type * color_count + isWhite;	
				found = (piece)(id); 
				break;
			}
		}
	}
	else
	{
		//else not piece, or square
		std::cout << "image " << prefix_range_begin << " not found. locate last failed image in images.back()" << "\n";
	}


	return found;
} // end try get piece

/// <summary>
/// get resized/upside-down textures from file
/// </summary>
/// <param name="square_len">size of pieces</param>
void load_textures(session& session, int square_len)
{
	const char* images_dir = "./Images";
		// images_dir MUST calculate the beginning index of piece names for parsing
	std::string single_backslash = "\\";
	const int images_dir_character_count = strlen(images_dir) + single_backslash.length(); 

	FilePathList image_paths = LoadDirectoryFiles(images_dir);
	int max_texture = empty;
	for (int i = 0; i < max_texture; i++)
	{
		char* path = image_paths.paths[i];
		piece piece = get_piece(images_dir_character_count, path);
		Image input_image = LoadImage(path);
		ImageResize(&input_image, square_len, square_len);
		Texture texture = LoadTextureFromImage(input_image);
		UnloadImage(input_image);
		session.textures[piece] = texture;
	}

	// cache checkerboard as texture
	session.board = LoadRenderTexture(square_len * ROWS, square_len * COLS);
	session.draw_board_grid_texture(square_len);
} // end load texture

// takes path, and out parameter file_size representing the size of file opened in bytes
// returns pointer to data or nullptr
void* read_file(std::ifstream& reader, std::filesystem::path filepath, uintmax_t& out_file_size)
{
	const std::ios_base::openmode no_reformat = std::ios::binary; 
	void* data = nullptr;
	reader.open(filepath, no_reformat); // open or create
	if (reader.is_open())
	{
		std::error_code code;
		out_file_size = std::filesystem::file_size(filepath, code);
		if (!code)
		{
			// don't need space for terminating null bytes (should any be written)
			data = (void*)std::malloc(out_file_size);
			// don't read null bytes
			reader.read(reinterpret_cast<char*>(data), out_file_size);
			reader.close();
		}
		else
		{
			std::cout << code.message() << '\n';
			std::cout << "file " << filepath << "ignored." << '\n';
		}
	}
	else
	{
		std::cout << "file " << filepath << " not found" << '\n';
		out_file_size = 0;
	}
	return data;
}

void load_user_data(session& session)
{
	std::filesystem::path dir = "chess_for_pc";
	if (std::filesystem::exists(dir))
	{
		std::ifstream reader;
		uintmax_t file_size_out;
		std::filesystem::path filepath;
		filepath = dir / "games.bin";
		session.mothball_games = (game_record*)read_file(reader, filepath, file_size_out);
		session.games_count = file_size_out/sizeof(game_record);
		filepath = dir / "moves.bin";
		session.mothball_moves = (bin_move*)read_file(reader, filepath, file_size_out);
		session.moves_count = file_size_out/sizeof(bin_move);
		filepath = dir / "final_position.bin";
		session.mothball_survivors = (piece_coord*)read_file(reader, filepath, file_size_out);
		session.survivors_count = file_size_out/sizeof(piece_coord);

		error("pass #2");
		for (int i{}; i < session.moves_count; i++)
		{
			std::cout << "move #" << i << ':';
			error(((bin_move*)session.mothball_moves)[i]);
		}
	}
}



// ----------------------------------------------------------------------
// Output
// ----------------------------------------------------------------------

void append_to_file(std::ofstream& writer, std::filesystem::path filepath, void* new_binary_data, int data_size)
{
	// WARN: binary mode may place null chars at end in some implimentations.
	// I don't know if ios::app accounts for null chars, yet assume it does so
	const std::ios_base::openmode no_reformat =  std::ios::binary; 
	const std::ios_base::openmode eof = std::ios::app;

	writer.open(filepath, eof | no_reformat); // open or create
	if (writer.is_open())
	{
		writer.write(reinterpret_cast<char*>(new_binary_data), data_size);
		writer.close();
	}
	else
	{
		std::cout << "file " << filepath << " not found or created" << '\n';
	}
}


void session_to_binary(session& session)
{
	// games are usually removed when the old one is cleared (ex. new_game())
	int move_three = 6;
	if (session.games.back().score_sheet.size() <= move_three)
		session.games.pop_back();
	if (session.games.empty())
		return;

	// bits to store square id
	const int square_id_max_bit = 6;
	const int element_1 = square_id_max_bit;
	const int element_2 = 2*square_id_max_bit;

	int total_survivors{};
	int total_half_moves{};
	for (int i{}; i < session.games.size(); i++)
	{
		game_record& game = session.games[i];
		total_half_moves += game.score_sheet.size();
		total_survivors += game.survivors_count;
	}
	const int new_mothball_survivors_bytes = total_survivors*sizeof(piece_coord);
	piece_coord* new_survivors_binary = (piece_coord*)malloc(new_mothball_survivors_bytes);
	const int new_mothball_moves_bytes = total_half_moves*sizeof(bin_move);
	bin_move* new_moves_binary = (bin_move*)malloc(new_mothball_moves_bytes);

	// index of last game end == index of new games begin
	// used to append new mothball_arrays atop old
	int game_end = session.moves_count;
	int survivors_end = session.survivors_count;

	error("converting games to binary");
	for (int i{}; i < session.games.size(); i++)
	{
		game_record& game = session.games[i];
		error(game);

		// game's mothball arrays
      		game.mothball_begin = game_end;
      		game.mothball_survivors_begin = survivors_end;
		game_end += game.score_sheet.size();
		survivors_end += game.survivors_count;

		// load survivors
		int survivors_bytes = game.survivors_count*sizeof(piece_coord);
		std::memcpy(new_survivors_binary, game.survivors, survivors_bytes);

		// compress moves, add to new mothball
		// moves
		// we store 16bits per move
		// 6bits = from	(value 0-63)
		// 6bits = to	(value 0-63)
		// 3bits = secondary_effect	(value 0-7)
		// 1bit = unused
		// 	storing secondary_effect is required to distinguish promotion=queen with promotion=rook/bishop
		//	storing captures as well as secondary_effect would be possible with hash,
		//		but not captures & promotion_xxx.

		bin_move* moves = &new_moves_binary[i];
		int total_half_moves = game.score_sheet.size();
		for (int move{}; move < total_half_moves; move++)
		{
			bin_move& bin = moves[move];
			half_move& defined = game.score_sheet[move]; 
			// initialize first 6bits and set remaining to 0
			bin = (bin_move)defined.from;
			// append each element to previous
			// allow off the end xor because end looks like: (elsewhere data) |= 0000;
			bin |= (bin_move)(defined.to << element_1);
			bin |= (bin_move)(defined.effect << element_2); 
		}
	}


	// write to file
	std::filesystem::path dir = "chess_for_pc";
	if (!std::filesystem::exists(dir))
		std::filesystem::create_directory(dir);
	std::filesystem::path filepath;
	std::ofstream writer;

	filepath = dir / "moves.bin";
	append_to_file(writer, filepath, new_moves_binary, new_mothball_moves_bytes);
	filepath = dir / "games.bin";
	int game_record_bytes = session.games.size()*sizeof(game_record);
	append_to_file(writer, filepath, session.games.data(), game_record_bytes);
	filepath = dir / "final_positions.bin";
	int positions_bytes = total_survivors*sizeof(piece_coord);
	append_to_file(writer, filepath, new_survivors_binary, new_mothball_survivors_bytes);
} // session_to_binary




// ----------------------------------------------------------------------
// Munipulations of loaded data
// ----------------------------------------------------------------------

void binary_to_game(session& session)
{
	// TODO: find game dimensions

	int old_mothball_bytes = session.moves_count*sizeof(half_move);
	half_move* moves = (half_move*)malloc(old_mothball_bytes);
	int square_id_max_bit = 6;
	const int element_1 = square_id_max_bit;
	const int element_2 = 2*square_id_max_bit;
	const int sqid_bit_mask = 63;
	for (int i{}; i < session.moves_count; i++)
	{
		const bin_move& bin = session.mothball_moves[i];

		half_move& defined = moves[i];
		defined.from = sqid_bit_mask&bin;
		defined.to = sqid_bit_mask&(bin >> element_1);
		defined.effect = (secondary_effect)(bin >> element_2);

		std::cout << "move #" << i << '\n';
		error((unsigned short)bin, "value");
		print_half_move(&moves[i]);
	}
	std::cout << '\n';
}
