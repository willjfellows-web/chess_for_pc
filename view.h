#pragma once
#include "view_model.h"
#include "records.h"

// pickup -> drop piece, promote, takebacks
void do_play_buttons(view_model* vm, departure& operative, session& session);
// new/old game, swap sides
// WARN: can destroy view model, should be vm function before end of frame
void do_gamesetting_buttons(view_model* vm, departure& operative, session& session);
void draw_board_view(view_model* vm, session& session);
void do_select_records(records* records, session& session);
