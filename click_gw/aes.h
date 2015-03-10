#ifndef _AES_
#define _AES_

#ifdef  __cplusplus
extern "C" {
#endif

#include <xmmintrin.h>              /* SSE instructions and _mm_malloc */
#include <emmintrin.h>              /* SSE2 instructions               */

typedef __m128i block;
typedef struct { __m128i rd_key[15]; int rounds; } AES_KEY;

#define ROUNDS(ctx) ((ctx)->rounds)

#define EXPAND_ASSIST(v1,v2,v3,v4,shuff_const,aes_const)                    \
    v2 = _mm_aeskeygenassist_si128(v4,aes_const);                           \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),              \
                                         _mm_castsi128_ps(v1), 16));        \
    v1 = _mm_xor_si128(v1,v3);                                              \
    v3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(v3),              \
                                         _mm_castsi128_ps(v1), 140));       \
    v1 = _mm_xor_si128(v1,v3);                                              \
    v2 = _mm_shuffle_epi32(v2,shuff_const);                                 \
    v1 = _mm_xor_si128(v1,v2)

#define EXPAND192_STEP(idx,aes_const)                                       \
    EXPAND_ASSIST(x0,x1,x2,x3,85,aes_const);                                \
    x3 = _mm_xor_si128(x3,_mm_slli_si128 (x3, 4));                          \
    x3 = _mm_xor_si128(x3,_mm_shuffle_epi32(x0, 255));                      \
    kp[idx] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(tmp),        \
                                              _mm_castsi128_ps(x0), 68));   \
    kp[idx+1] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x0),       \
                                                _mm_castsi128_ps(x3), 78)); \
    EXPAND_ASSIST(x0,x1,x2,x3,85,(aes_const*2));                            \
    x3 = _mm_xor_si128(x3,_mm_slli_si128 (x3, 4));                          \
    x3 = _mm_xor_si128(x3,_mm_shuffle_epi32(x0, 255));                      \
    kp[idx+2] = x0; tmp = x3


inline int AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
inline void AES_set_decrypt_key_fast(AES_KEY *dkey, const AES_KEY *ekey);
inline int AES_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);

inline void AES_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
inline void AES_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
inline void AES_ecb_encrypt_blks(block *blks, unsigned nblks, AES_KEY *key);
inline void AES_ecb_decrypt_blks(block *blks, unsigned nblks, AES_KEY *key);

#ifdef  __cplusplus
}
#endif

#endif
