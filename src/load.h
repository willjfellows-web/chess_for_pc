#pragma once
#include "model.h"
#include "raylib.h"
#include <array>
#include <iostream> // for testing

/// <summary>
/// checks characters equality from left to right
/// </summary>
bool str_equals_until(const char* str1, const char* str2, char delimiter);


/// <summary>
/// parses file names to find piece id. ex) ./Images\\black_king.png -> piece::black_king.
/// </summary>
/// <param name="images_dir_count">folder name containing all images, INCLUDING trailing '/'</param>
/// <param name="image_name">full path name of image</param>
piece get_piece(int images_dir_len_count, const char* image_name);

/// <summary>
/// get resized/upside-down textures from file
/// </summary>
/// <param name="square_len">size of pieces</param>
std::array<Texture2D, empty> load_textures(int square_len);
