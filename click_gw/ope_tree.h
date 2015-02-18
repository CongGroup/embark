#ifndef OPETREE_H_
#define OPETREE_H_

#include "tree/ScapegoatTree.h"
#include <limits>
#include <random>
#include <arpa/inet.h>

typedef unsigned __int128 uint128_t;

inline uint128_t hton128(uint128_t n)
{
    uint32_t h0 = n & 0xffffffff;
    uint32_t h1 = (n >> 32) & 0xffffffff;
    uint32_t h2 = (n >> 64) & 0xffffffff;
    uint32_t h3 = (n >> 96) & 0xffffffff;
    uint128_t n0 = htonl(h0);
    uint128_t n1 = htonl(h1);
    uint128_t n2 = htonl(h2);
    uint128_t n3 = htonl(h3);
    return (n3) | (n2 << 32) | (n1 << 64) | (n0 << 96);
}

inline uint128_t ntoh128(uint128_t n)
{
    uint32_t n0 = n & 0xffffffff;
    uint32_t n1 = (n >> 32) & 0xffffffff;
    uint32_t n2 = (n >> 64) & 0xffffffff;
    uint32_t n3 = (n >> 96) & 0xffffffff;
    uint128_t h0 = ntohl(n0);
    uint128_t h1 = ntohl(n1);
    uint128_t h2 = ntohl(n2);
    uint128_t h3 = ntohl(n3);
    return (h3) | (h2 << 32) | (h1 << 64) | (h0 << 96);
}

static inline 
uint128_t random128(std::random_device& engine)
{
    uint128_t x0 = engine();
    uint128_t x1 = engine();
    uint128_t x2 = engine();
    uint128_t x3 = engine();
    return x0 | (x1 << 32) | (x2 << 64) | (x3 << 96);
}

// TODO: Not exactly uniformed dist
static inline 
uint128_t random_bound(std::random_device& engine, const uint128_t& low, const uint128_t& high)
{
    return (random128(engine) % (high - low)) + low;
}

template <class Node, class EncType>
class OPETree : public ods::ScapegoatTree<Node, EncType> {
protected:
    using ods::BinaryTree<Node>::nil;
    using ods::BinaryTree<Node>::r;

    std::random_device generator;

    uint128_t max_ciphertext_less_equal_than(const EncType& v) const;
    uint128_t min_ciphertext_greater_than(const EncType& v) const;

public:
    std::pair<uint128_t, uint128_t> ciphertext_range(const EncType& v) const;
    uint128_t generate_ciphertext(const EncType& v);
};

template<class EncType>
class OPETree1 : public OPETree<ods::BSTNode1<EncType>, EncType> { };

template <class Node, class EncType>
uint128_t
OPETree<Node, EncType>::generate_ciphertext(const EncType& v)
{
    uint128_t low = max_ciphertext_less_equal_than(v),
        high = min_ciphertext_greater_than(v);
    return random_bound(generator, low, high);
}

template <class Node, class EncType>
std::pair<uint128_t, uint128_t> 
OPETree<Node, EncType>::ciphertext_range(const EncType& v) const
{
    uint128_t low = max_ciphertext_less_equal_than(v),
        high = min_ciphertext_greater_than(v);
    return std::make_pair(low, high);
}

template <class Node, class EncType>
uint128_t 
OPETree<Node, EncType>::max_ciphertext_less_equal_than(const EncType& v) const
{
    uint128_t current_path = 0, path = 0;
    size_t current_path_length = 0, path_length = 0;
    Node *current = r, *max = nil;

    while (current != nil) {
        if (current->x <= v) {
            max = current;
            path = current_path;
            path_length = current_path_length;
            
            current = current->right;
            current_path = (current_path << 1) | 1;
            ++current_path_length;
        } else {
            current = current->left;
            current_path = current_path << 1;
            ++current_path_length;
        }
    }

    if (max == nil) {
        return std::numeric_limits<uint128_t>::min();
    } else {
        path = (path << 1) | 1;
        ++path_length;
        path <<= (128 - path_length);
        return path;
    }
}

template <class Node, class EncType>
uint128_t 
OPETree<Node, EncType>::min_ciphertext_greater_than(const EncType& v) const
{
    uint128_t current_path = 0, path = 0;
    size_t current_path_length = 0, path_length = 0;
    Node *current = r, *min = nil;

    while (current != nil) {
        if (current->x > v) {
            min = current;
            path = current_path;
            path_length = current_path_length;
            
            current = current->left;
            current_path = (current_path << 1);
            ++current_path_length;
        } else {
            current = current->right;
            current_path = (current_path << 1) | 1;
            ++current_path_length;
        }
    }

    if (min == nil) {
        return ~0;
    } else {
        path = (path << 1) | 1;
        ++path_length;
        path <<= (128 - path_length);
        return path;
    }
}

#endif
