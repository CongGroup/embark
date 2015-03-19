#include "paste.hh"
#include "contentstrings.hh"
#ifndef TOKENIZER_H
#define TOKENIZER_H

#define DIVIDER_FILE_LOCATION "./dividers.txt"
#define ALLOW_NEWLINES 0 


/**
 * Where were the previous matches? So we can search back and create tokens.
 **/

//Loading the dividers that you tokenize over -- returns ptr to the buffer,
//copies in to dividerscount the number of strings
//The function that creates the tokens.
//
cs_list_node_t* get_user_tokens_from_buffer_easy_3(char* text, uint16_t textlen, bool* dividers, cs_list_node** freenodes);
#endif
