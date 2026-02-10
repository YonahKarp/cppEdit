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

static inline unsigned char to_lower(unsigned char c) {
    return static_cast<unsigned char>(std::tolower(c));
}

static bool case_insensitive_match(const char* text, int text_pos, const char* query, int query_len) {
    for (int i = 0; i < query_len; i++) {
        if (to_lower(static_cast<unsigned char>(text[text_pos + i])) != 
            to_lower(static_cast<unsigned char>(query[i]))) {
            return false;
        }
    }
    return true;
}

static void build_bad_char_table(const char* query, int query_len, int* bad_char) {
    for (int i = 0; i < 256; i++) {
        bad_char[i] = query_len;
    }
    for (int i = 0; i < query_len - 1; i++) {
        unsigned char c = to_lower(static_cast<unsigned char>(query[i]));
        bad_char[c] = query_len - 1 - i;
    }
}

static void boyer_moore_search(SearchState& search, const char* text, int text_len) {
    int query_len = search.query_len;
    
    int bad_char[256];
    build_bad_char_table(search.query, query_len, bad_char);
    
    int i = 0;
    while (i <= text_len - query_len) {
        int j = query_len - 1;
        
        while (j >= 0 && to_lower(static_cast<unsigned char>(text[i + j])) == 
                         to_lower(static_cast<unsigned char>(search.query[j]))) {
            j--;
        }
        
        if (j < 0) {
            SearchMatch match;
            match.start = i;
            match.end = i + query_len;
            search.matches.push_back(match);
            i++;
        } else {
            unsigned char c = to_lower(static_cast<unsigned char>(text[i + query_len - 1]));
            int shift = bad_char[c];
            i += (shift > 0) ? shift : 1;
        }
    }
}

static void naive_search(SearchState& search, const char* text, int text_len) {
    for (int i = 0; i <= text_len - search.query_len; i++) {
        if (case_insensitive_match(text, i, search.query, search.query_len)) {
            SearchMatch match;
            match.start = i;
            match.end = i + search.query_len;
            search.matches.push_back(match);
        }
    }
}

constexpr int BOYER_MOORE_THRESHOLD = 4096;
constexpr int MIN_PATTERN_LEN_FOR_BM = 3;

void perform_search(SearchState& search, const char* text, int text_len) {
    search.matches.clear();
    search.current_match_index = -1;
    
    if (search.query_len == 0 || text_len == 0) {
        return;
    }
    
    if (text_len >= BOYER_MOORE_THRESHOLD && search.query_len >= MIN_PATTERN_LEN_FOR_BM) {
        boyer_moore_search(search, text, text_len);
    } else {
        naive_search(search, text, text_len);
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
