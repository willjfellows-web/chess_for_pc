#pragma once
#include <filesystem>
#include <fstream>
#include "view_model.h"

/// <summary>
/// get resized/upside-down textures from file
/// </summary>
/// <param name="square_len">size of pieces</param>
void load_textures(session& session, int square_len);
void load_user_data(session& session);
void session_to_binary(session& session);
void binary_to_game(session& session); // decompress session.mothball_moves
