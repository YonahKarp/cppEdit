#include "search.h"
#include <cstring>
#include <cctype>

void toggle_search(SearchState& search) {
    search.active = !search.active;
    if (!search.active) {
        close_search(search);
    }
}

void close_search(SearchState& search) {
    search.active = false;
    search.query[0] = '\0';
    search.query_len = 0;
    search.matches.clear();
    search.current_match_index = -1;
}

static bool case_insensitive_match(const char* text, int text_pos, const char* query, int query_len) {
    for (int i = 0; i < query_len; i++) {
        if (std::tolower(static_cast<unsigned char>(text[text_pos + i])) != 
            std::tolower(static_cast<unsigned char>(query[i]))) {
            return false;
        }
    }
    return true;
}

void perform_search(SearchState& search, const char* text, int text_len) {
    search.matches.clear();
    search.current_match_index = -1;
    
    if (search.query_len == 0 || text_len == 0) {
        return;
    }
    
    for (int i = 0; i <= text_len - search.query_len; i++) {
        if (case_insensitive_match(text, i, search.query, search.query_len)) {
            SearchMatch match;
            match.start = i;
            match.end = i + search.query_len;
            search.matches.push_back(match);
        }
    }
    
    if (!search.matches.empty()) {
        search.current_match_index = 0;
    }
}

int navigate_to_next_match(SearchState& search) {
    if (search.matches.empty()) {
        return -1;
    }
    
    search.current_match_index++;
    if (search.current_match_index >= static_cast<int>(search.matches.size())) {
        search.current_match_index = 0;
    }
    
    return search.matches[search.current_match_index].start;
}

int navigate_to_prev_match(SearchState& search) {
    if (search.matches.empty()) {
        return -1;
    }
    
    search.current_match_index--;
    if (search.current_match_index < 0) {
        search.current_match_index = static_cast<int>(search.matches.size()) - 1;
    }
    
    return search.matches[search.current_match_index].start;
}

bool is_position_in_match(const SearchState& search, int pos) {
    for (const auto& match : search.matches) {
        if (pos >= match.start && pos < match.end) {
            return true;
        }
    }
    return false;
}

int get_current_match_start(const SearchState& search) {
    if (search.current_match_index >= 0 && 
        search.current_match_index < static_cast<int>(search.matches.size())) {
        return search.matches[search.current_match_index].start;
    }
    return -1;
}
