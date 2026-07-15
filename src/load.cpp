#include "load.h"
#include <cstring>

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
std::array<Texture2D, empty> load_textures(int square_len)
{
	const char* images_dir = "./Images";
		// images_dir MUST calculate the beginning index of piece names for parsing
	std::string single_backslash = "\\";
	const int images_dir_character_count = strlen(images_dir) + single_backslash.length(); 

	std::array<Texture2D, empty> textures;
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
		textures[piece] = texture;
	}

	return textures;
} // end load texture
