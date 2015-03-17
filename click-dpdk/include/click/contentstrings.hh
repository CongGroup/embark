#ifndef CONTENT_STRINGS_H
#define CONTENT_STRINGS_H

#include "paste.hh"
#include "bn.h"

typedef bn::Fp12 paired_value;

/**
 * Content strings may have hex values in them, so we need
 * to convert the written-string format to the actual bit-values
 * we're supposed to match again.
 * For example, a string might read: "alphabet|255|" -- |255| in snort-land
 * means there's a byte ff at the end (not an alphanumeric letter, but legit).
 * A content string represents this in bytes. Note also that
 * the content string struct needs to have a strlen field -- \0 as a terminator
 * DOESN'T WORK since 00 is a legit hex value we might have within the string.
 **/
enum string_type{
  //For generating ciphers
  PLAINTEXT, //This is, plaintext, byte by byte, what we read in (text, no cipher)
  SNORT_CS,  //This is a Snort CS -- we read in something like foo|255|, so we needed to convert last byte ( text no cipher);
  //For dealing with ciphers
  USER_ENC,  //This is an encrypted string we received from the user (cipher no text)
  TTP_TOKEN, //This is an encrypted string we received from the TTP (`McAfee') (text and cipher)
  COMBO_TOKEN, //This is an encrypted string we constructed from a user key and the TTP Token (cipher no text)
  //For fast lookup
  CACHE_TOKEN //This is a string and the partially encrypted value for caching.
};

typedef struct content_string{
  char* s;
  string_type type;
  uint32_t strlen;
  uint32_t bytestream_offset; //If PLAINTEXT or USER_ENC
  paired_value* cipher;
} content_string_t;

//Do ops on head only
typedef struct cs_list_node{
  content_string_t* str;
  struct cs_list_node* next;
}cs_list_node_t;

content_string_t* cs_get_from_snort_string(char* s);
void cs_free(content_string_t* cs);
bool cs_are_equal(content_string_t* a, content_string_t* b);
void cs_list_node_free(cs_list_node_t* cshn);
#endif
