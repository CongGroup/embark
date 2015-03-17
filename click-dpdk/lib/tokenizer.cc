// justine@eecs.berkeley.edu
#include "tokenizer.hh"
#include "ahocorasick/ahocorasick.h"
#include "paste.hh"

//*
// Read in the search dividers.
// Why read in the file twice? Once to count number of words, then once to read the words?
//  -- Variable-length datastructures are annoying
//  -- So many bugs in the RC3 setup when we passed the number of dividers in as a separate param
//  -- The file is short so who the hell cares
//*
AC_ALPHABET_t** load_search_file(uint32_t* dc, char* filename, bool inc_newline){
  uint32_t dividerscount = 0; //One for the newline, which we're hardcoding in.
  FILE* dividersfile = fopen(filename, "r");

  if(dividersfile == 0){
    std::cerr << "Failed to load tokens divider!" << std::endl;
    exit(2);
  }

  char ch;

  while (EOF != (ch=getc(dividersfile))) if ('\n' == ch) ++dividerscount;

  fclose(dividersfile);
  AC_ALPHABET_t** dividers = (char**) malloc(sizeof(AC_ALPHABET_t*) * dividerscount + 1);

  //Reopen and read the words now 
  dividersfile = fopen(filename, "r");
  char stringbuffer[MAX_TOKEN_SIZE + 1]; //Divider can't be bigger than token, lol
  uint32_t divideridx = 0;

  if(inc_newline){
    //Hardcoding the newline as a divider
    char* hardcodednewline = (char*) malloc(sizeof(char) * 2);
    hardcodednewline[0] = '\n';
    hardcodednewline[1] = '\0';
    dividers[0] = hardcodednewline;
    divideridx++;
    dividerscount++;
  }
  uint32_t stridx = 0;
  while (EOF != (ch=getc(dividersfile))){
    if ('\n' == ch){
      //Full word. Add null terminator and put it in the list.
      if(stridx == 0) continue;
      stringbuffer[stridx] = '\0';
      char* str = (char*) malloc(sizeof(char) * (stridx + 1));
      memcpy(str, stringbuffer, sizeof(char) * (stridx + 1));
      dividers[divideridx] = (AC_ALPHABET_t*) str;

      //Reset indices for next line.
      ++divideridx;
      stridx = 0;
    } 
    else if(MAX_TOKEN_SIZE + 1 == stridx){
      std::cerr << "Improperly formated dividers file. Error on line " << divideridx << std::endl;
      exit(2);
    }else{
      stringbuffer[stridx] = ch;
      ++stridx;
    }
  }
  *dc = dividerscount;
  return dividers;
}

//**
//Once you find a divider to create dividers, we need to remember where it was
//so we can search back and create the variable-length dividers. 
//**
match_history_t* mh_init(){
  match_history_t* history = (match_history_t*) malloc(sizeof(struct match_history));
  memset(history->matches, 0, sizeof(uint32_t) * MAX_TOKEN_SIZE);

  //Initialize to MAX divider size to account for split input string buffer (cur/prev)
  for(uint32_t i = 0; i < MAX_TOKEN_SIZE; i++) history->matches[i] = MAX_TOKEN_SIZE;

  match_history_t * c = history;
  memset(c->dividers, 0, sizeof(bool) * 256);
  c->dividers['"'] = true;
  c->dividers['.'] = true;
  c->dividers['`'] = true;
  c->dividers['~'] = true;
  c->dividers['\''] = true;
  c->dividers['/'] = true;
  c->dividers['<'] = true;
  c->dividers['>'] = true;
  c->dividers['?'] = true;
  c->dividers['='] = true;
  c->dividers['-'] = true;
  c->dividers['_'] = true;
  c->dividers[':'] = true;
  c->dividers[';'] = true;
  c->dividers['\\'] = true;
  c->dividers['$'] = true;
  c->dividers['#'] = true;
  c->dividers['*'] = true;
  c->dividers['+'] = true;
  c->dividers['('] = true;
  c->dividers[')'] = true;
  c->dividers['.'] = true;
  c->dividers['{'] = true;
  c->dividers['}'] = true;
  c->dividers['['] = true;
  c->dividers[']'] = true;
  c->dividers['%'] = true;
  c->dividers['&'] = true;
  c->dividers['^'] = true;
  c->dividers['@'] = true;
  


  memset(history->buff, 'a', sizeof(char) * MAX_TOKEN_SIZE * 2 + 1);
  memset(history->bufftok, 0, sizeof(bool) * MAX_TOKEN_SIZE * 2 + 1); 
  history->total_strlen = 0;
  history->idx = 1;
  history->bufferidx = 0;
}

//Wraps suck, part 1.
uint32_t mh_prev_idx(uint32_t m){
  return (m + MAX_TOKEN_SIZE - 1) % MAX_TOKEN_SIZE;
}

//Wraps suck, part 2.
uint32_t mh_next_idx(uint32_t m){
  return (m + 1) % MAX_TOKEN_SIZE;
}

//Insertion needs to be in-order and this is a bit uggo, 
//but the array is short.
void mh_append(match_history_t* mh, uint32_t match){
  if(mh->matches[mh_prev_idx(mh->idx)] < match){
    mh->matches[mh->idx] = match;
    mh->idx = mh_next_idx(mh->idx);
  }else if(mh->matches[mh_prev_idx(mh->idx)] > match){
    //Need to go back and insert -- bump array forward.
    uint32_t where_to_insert = mh_prev_idx(mh->idx);
    while(mh->matches[where_to_insert] > match){
      where_to_insert = mh_prev_idx(where_to_insert);
      //std::cout << "BLAH " << match << " " << where_to_insert << " " << mh->matches[where_to_insert] << std::endl;
    }
    if(mh->matches[where_to_insert] != match){
      //Need to bump forward. Ignore if equal, don't need redundant elts.
      for(uint32_t i = mh_prev_idx(mh->idx); i != where_to_insert; i = mh_prev_idx(i)){
        //std::cout << "BLAH" << std::endl;
        mh->matches[mh_next_idx(i)] = mh->matches[i];
      }
      mh->matches[mh_next_idx(where_to_insert)] = match;
      mh->idx = mh_next_idx(mh->idx);
    }
  }//else it's equal, and we don't have to insert the duplicate value
}

void mh_free(match_history_t* mh){
  free(mh);
}

AC_AUTOMATA_t* get_default_automata(){
  uint32_t dividerscount;
  AC_ALPHABET_t** dividers = load_search_file(&dividerscount,(char*)  DIVIDER_FILE_LOCATION, true);

  //Define AC variables:
  AC_AUTOMATA_t   *atm;
  AC_PATTERN_t    tmp_pattern;
  AC_TEXT_t       tmp_text;

  // Get a new automata
  atm = ac_automata_init ();
  for (uint32_t i=0; i<dividerscount; i++)
  {
    tmp_pattern.astring = dividers[i];
    tmp_pattern.rep.number = i+1; // optional
    tmp_pattern.length = strlen(tmp_pattern.astring);
    ac_automata_add (atm, &tmp_pattern);
  }
  ac_automata_finalize (atm);

  //Free the strings we read in.
  for(uint32_t i = 0; i < dividerscount; i++){
    free(dividers[i]);
  }
  free(dividers);

  return atm;
}

cs_list_node_t* get_user_tokens_from_buffer(char* text, uint16_t textlen, AC_AUTOMATA_t* atm, match_history_t* history){
  cs_list_node_t* ln = 0;

  int32_t remainder = textlen;
  char* buff = history->buff;
  while(remainder > 0){
    char* cur = &buff[MAX_TOKEN_SIZE];
    uint16_t copylen = remainder < MAX_TOKEN_SIZE ? remainder : MAX_TOKEN_SIZE;
    //std::cout << "copylen = " << copylen << std::endl;
    memcpy(cur, &text[textlen - remainder], copylen);
    cur[copylen] = '\0';
    //Set the input text
    AC_TEXT_t       tmp_text;
    tmp_text.astring = (AC_ALPHABET_t*) cur;  
    tmp_text.length = copylen;
    ac_automata_settext (atm, &tmp_text, 1); //1 means contiguous with previous.
    //find
    AC_MATCH_t * matchp;

    //Originially, I had thought that dividers might be multi-character. Now they're single character, which 
    //is fine, but means we don't actually need to use this AHO thingy. Oh well.
    while ((matchp = ac_automata_findnext(atm))){
      //There may be multiple dividers if substrings. Need inclusive left,
      //inclusive right, inclusive, and exclusive.
      //std::cerr << " Match found at " << matchp->position << std::endl;
      uint32_t matchspot = (matchp->position - history->total_strlen) + MAX_TOKEN_SIZE -1 ;
      if(matchp->position == 1) continue;
      uint32_t previdx = mh_prev_idx(history->idx);
      uint32_t base_matchspot = matchspot;

      for (int32_t j = -1; j < (int32_t) 1; j++){
        if(j != -1){ //Exclusive on the right
          matchspot = base_matchspot - 1;
        }else{ //Inclusive on the right
          matchspot = base_matchspot;
        }
        previdx = mh_prev_idx(history->idx);
        //std::cout << "previdx: " << previdx << "(" << history->matches[previdx] << ")" <<  std::endl;
        while(history->matches[previdx] != 0 && (matchspot + history->total_strlen) - history->matches[previdx] <= MAX_TOKEN_SIZE){
          if(matchspot - history->matches[previdx] == 0){
            //Edge case! When we do the exclusive search for a divider, if it came right after a previous
            //divider, we may wind up printing the same indices as if we had done the inclusive
            //search for the previous divider -- so we should skip this case altogether.
            break;
          }
          if((matchspot + history->total_strlen - history->matches[previdx] + 1)>= MIN_TOKEN_SIZE){
            //So many calls to malloc! Remove these later.
            cs_list_node_t* cln = (cs_list_node_t*) malloc(sizeof(struct cs_list_node));
            content_string_t* str = (content_string_t*) malloc(sizeof(struct content_string));
            str->type = PLAINTEXT;
            str->bytestream_offset = history->matches[previdx];
            str->strlen = matchspot + history->total_strlen - history->matches[previdx] + 1;
            str->s = (char*) malloc(sizeof(char) * (str->strlen + 1));
            //std::cout << "going from " << history->matches[previdx] - history->total_strlen << " to " << matchspot << std::endl;
            char* copyfrom =  &buff[(history->matches[previdx] - history->total_strlen) % (MAX_TOKEN_SIZE * 2)];
            memcpy(str->s, copyfrom, str->strlen);
            str->s[str->strlen] = '\0';
            cln->str = str;
            //std::cout << "New string: " << str->s << std::endl;
            cln->next = ln;
            ln = cln;
          }
          uint32_t prevprev = previdx;
          previdx = mh_prev_idx(previdx);

          if(history->matches[previdx] >= history->matches[prevprev]) break; //Check for wraparound
        }
      }

      //Update the indices for the next run
      mh_append(history, base_matchspot + 1 + history->total_strlen); //Exclusive
      mh_append(history, base_matchspot + history->total_strlen); //Inclusive

      /*for (int32_t j=0; j < matchp->match_num; j++){
        std::cout << "beep" << std::endl;
        mh_append(history, base_matchspot + 1 - strlen(matchp->patterns[j].astring));
        std::cout << "bop" << std::endl;
        }*/

    }
    history->bufferidx = (history->bufferidx == 1) ? 0 : 1;
    memcpy(history->buff, history->buff + copylen, MAX_TOKEN_SIZE * sizeof(char));
    history->total_strlen += copylen;
    remainder = remainder - copylen;

  }
  return ln;
}

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

cs_list_node_t* get_user_tokens_from_buffer_easy(char* text, uint16_t textlen, match_history_t* history){
  cs_list_node_t* ln = 0;

  int32_t remainder = textlen;
  char* buff = history->buff;
  while(remainder > 0){
    unsigned char* cur = (unsigned char*) &buff[MAX_TOKEN_SIZE];
    uint16_t copylen = remainder < MAX_TOKEN_SIZE ? remainder : MAX_TOKEN_SIZE;
    //std::cout << "copylen = " << copylen << std::endl;
    memcpy(cur, &text[textlen - remainder], copylen);
    cur[copylen] = '\0';

    //Originially, I had thought that dividers might be multi-character. Now they're single character, which 
    //is fine, but means we don't actually need to use this AHO thingy. Oh well.
    int idx = 0;

    while(idx < MAX_TOKEN_SIZE && !is_divider(cur[idx]))// || (cur[idx] >= 'a' && cur[idx] <= 'z') || (cur[idx] >= '0' || cur[idx] <= '9')))
    {
      idx++;
    }
    while(idx < copylen){
      //There may be multiple dividers if substrings. Need inclusive left,
      //inclusive right, inclusive, and exclusive.
      //std::cerr << " Match found at " << matchp->position << std::endl;

      uint32_t matchspot = (idx) + MAX_TOKEN_SIZE ;
      uint32_t previdx = mh_prev_idx(history->idx);
      uint32_t base_matchspot = matchspot;
      idx++;
      if(history->total_strlen == 0 && idx == 1) continue;
      for (int32_t j = -1; j < (int32_t) 1; j++){
        if(j != -1){ //Exclusive on the right
          matchspot = base_matchspot - 1;
        }else{ //Inclusive on the right
          matchspot = base_matchspot;
        }
        previdx = mh_prev_idx(history->idx);
        //std::cout << "previdx: " << previdx << "(" << history->matches[previdx] << ")" <<  std::endl;
        while(history->matches[previdx] != 0 && (matchspot + history->total_strlen) - history->matches[previdx] <= MAX_TOKEN_SIZE){
          //if(history->matches[previdx] != 0 && (matchspot + history->total_strlen) - history->matches[previdx] <= MAX_TOKEN_SIZE){
          if(matchspot - history->matches[previdx] == 0){
            //Edge case! When we do the exclusive search for a divider, if it came right after a previous
            //divider, we may wind up printing the same indices as if we had done the inclusive
            //search for the previous divider -- so we should skip this case altogether.
            break;
          }
          if((matchspot + history->total_strlen - history->matches[previdx] + 1)>= MIN_TOKEN_SIZE){
            //So many calls to malloc! Remove these later.
            cs_list_node_t* cln = (cs_list_node_t*) malloc(sizeof(struct cs_list_node));
            content_string_t* str = (content_string_t*) malloc(sizeof(struct content_string));
            str->type = PLAINTEXT;
            str->bytestream_offset = history->matches[previdx];
            str->strlen = matchspot + history->total_strlen - history->matches[previdx] + 1;
            str->s = (char*) malloc(sizeof(char) * (str->strlen + 1));
            //std::cout << "going from " << history->matches[previdx] - history->total_strlen << " to " << matchspot << std::endl;
            char* copyfrom =  &buff[(history->matches[previdx] - history->total_strlen) % (MAX_TOKEN_SIZE * 2)];
            memcpy(str->s, copyfrom, str->strlen);
            str->s[str->strlen] = '\0';
            cln->str = str;
            //std::cout << "New string: " << str->s << std::endl;
            cln->next = ln;
            ln = cln;
          }
          uint32_t prevprev = previdx;
          previdx = mh_prev_idx(previdx);

          if(history->matches[previdx] >= history->matches[prevprev]) break; //Check for wraparound
        }
        }

        //Update the indices for the next run
        mh_append(history, base_matchspot + 1 + history->total_strlen); //Exclusive
        mh_append(history, base_matchspot + history->total_strlen); //Inclusive

        /*for (int32_t j=0; j < matchp->match_num; j++){
          std::cout << "beep" << std::endl;
          mh_append(history, base_matchspot + 1 - strlen(matchp->patterns[j].astring));
          std::cout << "bop" << std::endl;
          }*/

        while(idx < MAX_TOKEN_SIZE && (!is_divider(cur[idx])))// || (cur[idx] >= 'a' && cur[idx] <= 'z') || (cur[idx] >= '0' || cur[idx] <= '9')))
        {
          idx++;
        }
      }
      history->bufferidx = (history->bufferidx == 1) ? 0 : 1;
      memcpy(history->buff, history->buff + copylen, MAX_TOKEN_SIZE * sizeof(char));
      history->total_strlen += copylen;
      remainder = remainder - copylen;

    }
    return ln;
  }


  cs_list_node_t* get_user_tokens_from_buffer_easy_2(char* text, uint16_t textlen, match_history_t* history){
    bool all_tokens_table[textlen + MAX_TOKEN_SIZE][textlen + MAX_TOKEN_SIZE];
    memset(all_tokens_table, 0, (textlen + MAX_TOKEN_SIZE) * (textlen + MAX_TOKEN_SIZE));


    //Copy history into tokens table.
    for(int i = 1; i < MAX_TOKEN_SIZE; i++){
      if(is_divider(history->buff[i])){
        for(int j = 0; j < i; j++){
          if(is_divider(text[i])){
            all_tokens_table[i][j + MAX_TOKEN_SIZE] = true;
            all_tokens_table[i][j + MAX_TOKEN_SIZE - 1] = true;
            all_tokens_table[i + 1][j + MAX_TOKEN_SIZE] = true;
            all_tokens_table[i + 1][j + MAX_TOKEN_SIZE - 1] = true;
          }
        }
      }
    }


    int idx = 0;
    while(idx < textlen && !is_divider(text[idx]))
    {
      idx++;
    }
    while(idx < textlen){
      //std::cout << "HEY000 " << idx << std::endl;
      //There may be multiple dividers if substrings. Need inclusive left,
      //inclusive right, inclusive, and exclusive.
      int idx2 = idx + MIN_TOKEN_SIZE;
      while(idx2 - idx < MAX_TOKEN_SIZE && idx2 < textlen){
        //std::cout << "Trying " << idx2 << std::endl;
        if(is_divider(text[idx2])){
          //Offset by MAX_TOKEN_SIZE to leave room for history
          //std::cout << "Want token from " << idx << " to " << idx2 << std::endl;
          all_tokens_table[idx + MAX_TOKEN_SIZE][idx2 + MAX_TOKEN_SIZE] = true;
          all_tokens_table[idx + 1 + MAX_TOKEN_SIZE][idx2 + MAX_TOKEN_SIZE] = true;
          all_tokens_table[idx + MAX_TOKEN_SIZE][idx2 - 1 + MAX_TOKEN_SIZE] = true;
          all_tokens_table[idx + 1 + MAX_TOKEN_SIZE][idx2 - 1 + MAX_TOKEN_SIZE] = true;
        }
        idx2++;
      }
      idx++;
      while(idx < textlen && (!is_divider(text[idx])))
      {
        idx++;
      }
    }

    //std::cout << "Minimizing..." << std::endl;
    minimize_transmitted_tokens((bool*) all_tokens_table, textlen + MAX_TOKEN_SIZE);
    //std::cout << "Done minimizing" << std::endl;
    cs_list_node* ln = 0;
    for(int i = 0; i < textlen + MAX_TOKEN_SIZE; i++){
      for(int j = i + MIN_TOKEN_SIZE; j < i + MAX_TOKEN_SIZE; j++){
        if(j < MAX_TOKEN_SIZE) continue;
        if(all_tokens_table[i][j]){
          if(i < MAX_TOKEN_SIZE){
            std::cout << " THIS IS A STUB " << std::endl;
            //SOMETHING COMPLICATED TODO
          }else{
            //std::cout << "Checking " << i << " " << j  << std::endl;
            if(all_tokens_table[i][j]){
              cs_list_node_t* cln = (cs_list_node_t*) malloc(sizeof(struct cs_list_node));
              content_string_t* str = (content_string_t*) malloc(sizeof(struct content_string));
              str->type = PLAINTEXT;
              str->bytestream_offset = i;
              str->strlen = j - i + 1;
              str->s = (char*) malloc(sizeof(char) * (str->strlen + 1));
              memcpy(str->s, text + i - MAX_TOKEN_SIZE, str->strlen);
              str->s[str->strlen] = '\0';
              cln->str = str;
              //std::cout << "New string: " << str->s << std::endl;
              cln->next = ln;
              ln = cln;
            }
          }
        }
      }
    }
    memcpy(history->buff, text - MAX_TOKEN_SIZE - 1, MAX_TOKEN_SIZE * sizeof(char));
    return ln;
  }


  //This is comedic.
  cs_list_node_t* get_user_tokens_from_buffer_easy_3(char* text, uint16_t textlen, match_history_t* history){
    bool tokenize[textlen + MAX_TOKEN_SIZE];
    memset(tokenize, 0, sizeof(bool) * (textlen + MAX_TOKEN_SIZE));
    memcpy(tokenize, history->bufftok, MAX_TOKEN_SIZE * sizeof(bool));

    for(int roundsize = MIN_TOKEN_SIZE; roundsize <= MAX_TOKEN_SIZE; roundsize++){
      for(int startidx = MAX_TOKEN_SIZE - roundsize + 1; startidx < textlen + MAX_TOKEN_SIZE - roundsize + 1; startidx++){
        int endidx = startidx + roundsize - 1; //All this is inclusive...
        
        char inclusiveleft = (startidx >= MAX_TOKEN_SIZE) ? text[startidx - MAX_TOKEN_SIZE] : history->buff[startidx];
        char exclusiveleft = 'a';
        if(startidx != 0) exclusiveleft = (startidx - 1 >= MAX_TOKEN_SIZE) ? text[startidx - 1 -  MAX_TOKEN_SIZE] : history->buff[startidx - 1];
      //The the newstuf
      //The the newstuf
        //std::cout << "Testing from " << startidx << " to " << (endidx + MAX_TOKEN_SIZE) << "(" << inclusiveleft << "," << text[endidx - MAX_TOKEN_SIZE] << ")" << std::endl;
        if((history->dividers[inclusiveleft] && history->dividers[text[endidx - MAX_TOKEN_SIZE]]) ||
           (startidx - 1 >= 0 && history->dividers[exclusiveleft] && history->dividers[text[endidx - MAX_TOKEN_SIZE]]) ||
           (endidx + 1 <= textlen && history->dividers[inclusiveleft] && history->dividers[(text[endidx + 1 - MAX_TOKEN_SIZE])]) ||
           (startidx - 1 >= 0 && endidx + 1 <= textlen && history->dividers[exclusiveleft] && history->dividers[(text[endidx + 1 - MAX_TOKEN_SIZE])]))
        {
          //std::cout << "Tokenizing from " << startidx << " to " << endidx << " (" << roundsize << ")" << std::endl;
          //Need to have tokens here.
          //std::cout << "--Marking initially: " << startidx << " " << (endidx) << std::endl;
          tokenize[startidx] = true;
          tokenize[endidx - MIN_TOKEN_SIZE + 1] = true;
          int upto = startidx + MIN_TOKEN_SIZE - 1;
          for(int j = startidx + 1; upto < endidx && j <= endidx + 1 - MIN_TOKEN_SIZE; j++){
            if(tokenize[j]){
              upto = j + MIN_TOKEN_SIZE - 1;
            }
            if(upto == j - 1){
              //Need to fill in the missing bit
              tokenize[j] = true;
              upto = j + MIN_TOKEN_SIZE - 1;
              //std::cout << "--Need to tokenize " << j << " to cover " << startidx << "," <<endidx << std::endl;
            }
          }
        }
      }
    }

    cs_list_node* ln = 0;
    for(int i = 0; i < textlen + MAX_TOKEN_SIZE; i++){
      if(tokenize[i]){
        cs_list_node_t* cln = (cs_list_node_t*) malloc(sizeof(struct cs_list_node));
        content_string_t* str = (content_string_t*) malloc(sizeof(struct content_string));
        str->type = PLAINTEXT;
        str->bytestream_offset = i;
        str->strlen = MIN_TOKEN_SIZE;
        str->s = (char*) malloc(sizeof(char) * (str->strlen + 1));
        if(i >= MAX_TOKEN_SIZE){
          memcpy(str->s, text + i - MAX_TOKEN_SIZE, str->strlen);
        }else if((i + MIN_TOKEN_SIZE - 1) < MAX_TOKEN_SIZE){
          if(!history->bufftok[i]){//Not already sent
            memcpy(str->s, history->buff + i, str->strlen);
          }else{
            free(cln);
            free(str);
            continue;
          }
        }else{
          int inhistory = MAX_TOKEN_SIZE - i;
          int infuture = MIN_TOKEN_SIZE - inhistory;
          //printf("%c|%c", history->buff[MAX_TOKEN_SIZE - 1], text[0]);
          memcpy(str->s, history->buff + i, inhistory);
          memcpy(str->s + inhistory, text, infuture);
        }
        str->s[str->strlen] = '\0';
        cln->str = str;
        //std::cout << "New string: " << str->s << std::endl;
        cln->next = ln;
        ln = cln;
      }
    }

    memcpy(history->buff, text + textlen - MAX_TOKEN_SIZE + 1, MAX_TOKEN_SIZE * sizeof(char));
    memcpy(history->bufftok, tokenize + textlen, MAX_TOKEN_SIZE * sizeof(bool));
    
    //For the wrap
    return ln;
  }

  cs_list_node_t* get_user_tokens_from_file(FILE* datafile){
    unsigned int i;

    //Where have we found matches in the past?
    match_history_t* history = mh_init();
    AC_AUTOMATA_t* atm = get_default_automata();


    char cur[MAX_TOKEN_SIZE + 1];
    cur[MAX_TOKEN_SIZE] = '\0';
    uint32_t fillidx = 0;
    char ch = 'a';
    cs_list_node_t* toreturn = 0;
    while(ch != EOF){
      while (fillidx < MAX_TOKEN_SIZE && EOF != (ch=getc(datafile))){
        if(ALLOW_NEWLINES || (ch != '\n' && ch != ' ')){
          cur[fillidx] = ch;
          fillidx++;
        }else if (fillidx != 0 && cur[fillidx-1] != ' '){ //Don't want multiple spaces back to back
          cur[fillidx] = ' ';
          fillidx++;
        }
      }
      cur[fillidx] = '\0'; 
      cs_list_node_t* ln = get_user_tokens_from_buffer(cur, fillidx, atm, history);

      fillidx = 0;
      if(ln != 0){
        cs_list_node_t* tail = ln;
        while(tail->next != 0){
          tail = tail->next;
        }
        tail->next = toreturn;
        toreturn = ln;
      }
    }
    //FREE THIS:
    //AC_ALPHABET_t** dividers = load_search_file(&dividerscount);

    ac_automata_release (atm);
    //free(atm);
    mh_free(history);
    return toreturn;  
  }
