#include "ahocorasick/ahocorasick.h"
#include "paste.hh"
#include "contentstrings.hh"
#ifndef TOKENIZER_H
#define TOKENIZER_H

#define DIVIDER_FILE_LOCATION "./dividers.txt"
#define ALLOW_NEWLINES 0 


/**
 * Where were the previous matches? So we can search back and create tokens.
 **/
typedef struct match_history{
  uint32_t matches[MAX_TOKEN_SIZE * 2];
  uint32_t idx;
  uint32_t bufferidx;
  uint32_t total_strlen;
  bool dividers[256];

  char buff[MAX_TOKEN_SIZE * 2 + 1];
  bool bufftok[MAX_TOKEN_SIZE * 2 + 1];
} match_history_t;

match_history_t* mh_init();
uint32_t mh_prev_idx(uint32_t m);
uint32_t mh_next_idx(uint32_t m);
void mh_append(match_history_t* mh, uint32_t match);
void mh_free(match_history_t* mh);

AC_AUTOMATA_t* get_default_automata();

//Loading the dividers that you tokenize over -- returns ptr to the buffer,
//copies in to dividerscount the number of strings
AC_ALPHABET_t** load_search_file(uint32_t* dividerscount, char* filename, bool inc_newline);
//The function that creates the tokens.
cs_list_node_t* get_user_tokens_from_buffer(char* text, uint16_t textlen, AC_AUTOMATA_t* atm, match_history_t* history);
cs_list_node_t* get_user_tokens_from_buffer_easy(char* text, uint16_t textlen, match_history_t* history);
cs_list_node_t* get_user_tokens_from_buffer_easy_2(char* text, uint16_t textlen, match_history_t* history);
cs_list_node_t* get_user_tokens_from_buffer_easy_3(char* text, uint16_t textlen, match_history_t* history);
cs_list_node_t* get_user_tokens_from_file(FILE* file);
#endif
