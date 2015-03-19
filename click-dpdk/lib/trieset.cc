#include <click/trieset.hh>

TrieSet::TrieSet(){
  memset(&root, 0, sizeof(TrieNode));
}

TrieSet::~TrieSet(){
  //TODO
}

//Returns true if contains
bool TrieSet::contains(unsigned char* label, uint32_t labellen){
  TrieNode* n = getNode(label, labellen);
  if(n->labellen == 0 || n->labellen != labellen) return false;
  return strncmp((const char*) label, (const char*) n->label, labellen) == 0;
}

//Returns true if new and inserted, false if it was
//already in there.
bool TrieSet::if_not_contains_then_insert(unsigned char* label, uint32_t labellen, void* related_data=0){
  bool contains = false;
  TrieNode* cur = getNode(label, labellen);
  if(cur->labellen == labellen && strncmp((const char*) label, (const char*) cur->label, labellen) == 0){
    return false;
  }else{
    //Need to split into new nodes.
    TrieNode* mynewnode = (TrieNode*) malloc(sizeof(TrieNode));
    memset(mynewnode, 0, sizeof(TrieNode));
    mynewnode->label = (unsigned char*) malloc(labellen);
    memcpy(mynewnode->label, label, labellen);
    mynewnode->labellen = labellen;
    mynewnode->depth = cur->depth + 1;
    mynewnode->related_data = related_data;
    cur->babies[label[cur->depth]] = mynewnode;

    if(cur->labellen != 0){
      TrieNode* bumpdownnode = (TrieNode*) malloc(sizeof(TrieNode));
      memset(bumpdownnode, 0, sizeof(TrieNode));
      bumpdownnode->label = cur->label;
      cur->label = 0;
      bumpdownnode->labellen = cur->labellen;
      cur->labellen = 0;
      bumpdownnode->depth = cur->depth + 1;
      bumpdownnode->related_data = cur->related_data;
      cur->related_data = 0;
      
      cur->babies[label[cur->depth]] = bumpdownnode;
    }
  }
  return true;
}

//Returns node where that label might be
TrieNode* TrieSet::getNode(unsigned char* label, uint32_t labellen){
  int idx;
  TrieNode* cur = &root;
  for(idx = 0; idx < labellen; idx++){
     if(cur->babies[label[idx]] == 0){
       return cur;
     }else{
       cur = cur->babies[label[idx]];
     }
  }
  return cur;
}


void* TrieSet::get_related_data(unsigned char* label, uint32_t labellen){
  return getNode(label, labellen)->related_data;  
}
