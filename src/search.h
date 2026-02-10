#pragma once

#include <string>
#include <vector>

constexpr int SEARCH_BUFFER_SIZE = 256;

struct SearchMatch {
    int start;
    int end;
};

struct SearchState {
    bool active = false;
    char query[SEARCH_BUFFER_SIZE] = "";
    int query_len = 0;
    
    std::vector<SearchMatch> matches;
    int current_match_index = -1;
};

void toggle_search(SearchState& search);
void close_search(SearchState& search);

void perform_search(SearchState& search, const char* text, int text_len);

int navigate_to_next_match(SearchState& search);
int navigate_to_prev_match(SearchState& search);

bool is_position_in_match(const SearchState& search, int pos);
int get_current_match_start(const SearchState& search);
