// justine@eecs.berkeley.edu

#include <click/config.h>
#include <click/packet.hh>
#include <click/packet_anno.hh>
#include <click/glue.hh>
#include <click/sync.hh>
#include <click/tokenizer.hh>
#include <click/paste.hh>


//*
// Read in the search dividers.
// Why read in the file twice? Once to count number of words, then once to read the words?
//  -- Variable-length datastructures are annoying
//  -- So many bugs in the RC3 setup when we passed the number of dividers in as a separate param
//  -- The file is short so who the hell cares
//*

//**
//Once you find a divider to create dividers, we need to remember where it was
//so we can search back and create the variable-length dividers. 
//**
inline bool is_divider(char x){
  if(x > 'a' && x < 'z') return false;
  return x == '"' ||
    x == '.' ||
    x == '`' ||
    x == '~' ||
    x == '\'' ||
    x == '/' ||
    x == '<' ||
    x == '>' ||
    x == '?' ||
    x == '=' ||
    x == '-' ||
    x == '_' ||
    x == ':' ||
    x == ';' ||
    x == '\\' ||
    x == '$' ||
    x == '#' ||
    x == '*' ||
    x == '+' ||
    x == '(' ||
    x == ')' ||
    x == '.' ||
    x == '{' ||
    x == '}' ||
    x == '[' ||
    x == ']' ||
    x == '%' ||
    x == '&' ||
    x == '^' ||
    x == '*' ||
    x == '@';
}

void minimize_transmitted_tokens(bool* tokentable, uint32_t length){
  //tokentable[x][y] means x connects forward to y.
  bool transitive_closure[length][length];
  memset(transitive_closure, 0, sizeof(bool) * length * length);
  for(int i = length - 1; i--; i >= 0){
    for(int j = length - 1; j--; j >= 0){
      transitive_closure[i][j] = *(tokentable + (i * length + j));
    }
  }
  //std::cout << "Started tclosure" << std::endl;

  //Move backwards. Everything who connects forward "to me"
  //connects to everything I connect forward. By working backwards
  //we can do this in one pass.
  for(int i = length - 1; i > 0; i --){
    //std::cout << "BOO " << i << std::endl;
    for(int j = i - 1; j--; j >= 0){
      if(transitive_closure[i][j]){
        for(int k = length - 1; k--; k>j){
          transitive_closure[j][k] = transitive_closure[i][k] | transitive_closure[j][k];
        }
      }
      if(i == j) transitive_closure[i][j] = false;
    }
  }

  //std::cout << "Generated tclosure" << std::endl;
  //Now remove all the unnecessary bits from the original table.
  for(int i = 0; i < length; i++){
    for(int j = i + 1; j < i + MAX_TOKEN_SIZE && j < length; j++){
      if(tokentable + (i * length + j)){ //Is there any finer grained path?
        for(int k = i + 1; k < j; k++){
          if(transitive_closure[i][k] && transitive_closure[k][j]){
            //std::cout << "Deleting " << i << "," << j << " because " << i << "," << k << " and " << k << "," << j << std::endl;
            *(tokentable + (i * length + j)) = false;
            break;
          }
        }
      }
    }
  }

  //And we're done.
}

//This is comedic.

cs_list_node_t* get_user_tokens_from_buffer_easy_3(char* text, uint16_t textlen, bool* dividers, cs_list_node** freenodes){

  bool tokenize[textlen + MAX_TOKEN_SIZE];
  memset(tokenize, 0, sizeof(bool) * (textlen + MAX_TOKEN_SIZE));

  for(int roundsize = MIN_TOKEN_SIZE; roundsize <= MAX_TOKEN_SIZE; roundsize++){
    for(int startidx = 0; startidx < textlen - roundsize + 1; startidx++){
      int endidx = startidx + roundsize - 1; //All this is inclusive...
      char inclusiveright = text[endidx];
      char exclusiveright = text[endidx + 1];
      char inclusiveleft = text[startidx];
      char exclusiveleft = 'a';
      if(startidx != 0) text[startidx - 1];
      //The the newstuf
      //The the newstuf
      //std::cout << "Testing from " << startidx << " to " << (endidx + MAX_TOKEN_SIZE) << "(" << inclusiveleft << "," << text[endidx - MAX_TOKEN_SIZE] << ")" << std::endl;

      if(dividers[inclusiveleft] || dividers[exclusiveleft]){
        if(dividers[inclusiveright] || dividers[exclusiveright]){
          //std::cout << "Tokenizing from " << startidx << " to " << endidx << " (" << roundsize << ")" << std::endl;
          //Need to have tokens here.
          //std::cout << "--Marking initially: " << startidx << " " << (endidx) << std::endl;
          
          tokenize[startidx] = true;
          tokenize[endidx] = true;
          int upto = startidx + MIN_TOKEN_SIZE - 1;
          for(int j = startidx + 1; upto < endidx && j <= endidx + 1; j++){
            if(tokenize[j]){
              upto = j + MIN_TOKEN_SIZE - 1;
            }
            if(upto == j - 1){
              //Need to fill in the missing bit
              tokenize[j] = true;
              upto = j + MIN_TOKEN_SIZE - 1;
            }
          }
        }
      }
    }
  }

  cs_list_node* ln = 0;
  for(int i = 0; i < textlen + MAX_TOKEN_SIZE; i++){
    if(tokenize[i]){
      cs_list_node_t* cln = *(freenodes);
      *(freenodes) = cln->next;
      cln->next = ln;
      content_string_t* str = cln->str;
      str->type = PLAINTEXT;
      str->bytestream_offset = i;
      str->strlen = MIN_TOKEN_SIZE;
      memset(str->s, 0, 32);
      memcpy(str->s, text + i, str->strlen);
      //std::cout << "New string: " << str->s << " " << cln << std::endl;
      ln = cln;
    }
  }

  //std::cout << "Returning " << ln << std::endl;
  //For the wrap
  return ln;
}

