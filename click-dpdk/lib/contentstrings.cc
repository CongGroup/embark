#include "contentstrings.hh"

content_string_t* cs_get_from_snort_string(char* s){
  //First, how many bytes in the final strong? <= strlen(s);
  uint32_t cs_strlen = 0;
  

  for(uint32_t i = 0; i < strlen(s); i++){
    if(s[i] == '|' && (i == 0 || s[i - 1] != '\\')){
      //Welcome to hexland. Three characters per byte.
      do{
        cs_strlen++;
        i+=3;
      }while(i < strlen(s) && s[i] != '|');
    }else{
      cs_strlen++; //One character == 1 byte.
    }
  }

  if(cs_strlen == 0){
    std::cerr << "Invalid Snort rule!" << std::endl;
    exit(2);
  }

  //Now we know how long the string should be
  char* cs = (char*) malloc(sizeof(char) * cs_strlen); //I don't know why I keep calling sizeof char
  uint32_t csidx = 0;
  for(uint32_t i = 0; i < strlen(s); i++){
    if(s[i] == '|' && (i == 0 || s[i - 1] != '\\')){
      //Welcome to hexland. Three characters per byte.
      //std::cout << "I am in hexland." << std::endl;
      do{
        //Lovely lovely scanf will interpret for us
        
        i++;
        sscanf(&(s[i]), "%2hhx", &(cs[csidx]));
        i+=2;
        csidx++;
      }while(i < strlen(s) && s[i] != '|');
    }else{
      cs[csidx] = s[i];
      csidx++;
    }
  }
  
  if(csidx != cs_strlen) std::cerr << "CREATED SNORT STRING WITH LENGTH " << csidx << " " << cs_strlen << std::endl;

  content_string_t* final_cs = (content_string_t*) malloc(sizeof(struct content_string));
  final_cs->s = cs;
  final_cs->type = SNORT_CS; 
  final_cs->strlen = cs_strlen;

  return final_cs;
}

void cs_free(content_string_t* cs){
  if(cs == 0) return;
  if(cs->s != 0){
    free(cs->s);
  }
  free(cs);
}

void cs_list_node_free(cs_list_node_t* cshn){
  if(cshn == 0) return;
  cs_free(cshn->str);
  free(cshn);
}


bool cs_are_equal(content_string_t* a, content_string_t* b){
  //TODO: Optimize this later to do comparison in 64/32/16 bit blocks?
  if(a->strlen != b->strlen) return false;
  uint32_t ln = a->strlen;
  //(Only if we're really really fine-tuning).
  for(uint32_t i = 0; i < ln; i++) if(a->s[i] != b->s[i]) return false;
  return true;
}
