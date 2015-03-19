#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct trienode_st{
  struct trienode_st* mom;
  struct trienode_st* babies[256];

  //Whatever you want. Type safety smype shafety.
  void* related_data;

  uint8_t depth;
  unsigned char* label;
  uint32_t labellen;
}TrieNode;

//TODO: add memory pool to aunsigned char* calls to malloc

class TrieSet{
  public:
    TrieSet();
    ~TrieSet();
    
    bool contains(unsigned char* label, uint32_t labellen);
    void* get_related_data(unsigned char* label, uint32_t labellen);
    bool if_not_contains_then_insert(unsigned char* label, uint32_t labellen, void* related_data);

  private:
    TrieNode* getNode(unsigned char* label, uint32_t labellen);
    TrieNode root;
};
