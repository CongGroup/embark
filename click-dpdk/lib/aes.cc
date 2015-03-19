/*
 This file is part of JustGarble.

    JustGarble is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JustGarble is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JustGarble.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <wmmintrin.h>
#include <click/aes.hh>

 void MY_AES_128_Key_Expansion(const unsigned char *userkey, void *key) {
	__m128i x0, x1, x2;
	__m128i *kp = (__m128i *) key;
	kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
	x2 = _mm_setzero_si128();
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 1);
	kp[1] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 2);
	kp[2] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 4);
	kp[3] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 8);
	kp[4] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 16);
	kp[5] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 32);
	kp[6] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 64);
	kp[7] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 128);
	kp[8] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 27);
	kp[9] = x0;
	EXPAND_ASSIST(x0, x1, x2, x0, 255, 54);
	kp[10] = x0;
}

 void MY_AES_192_Key_Expansion(const unsigned char *userkey, void *key) {
	__m128i x0, x1, x2, x3, tmp, *kp = (__m128i *) key;
	kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
	tmp = x3 = _mm_loadu_si128((__m128i *) (userkey + 16));
	x2 = _mm_setzero_si128();
	EXPAND192_STEP(1, 1);
	EXPAND192_STEP(4, 4);
	EXPAND192_STEP(7, 16);
	EXPAND192_STEP(10, 64);
}

 void MY_AES_256_Key_Expansion(const unsigned char *userkey, void *key) {
	__m128i x0, x1, x2, x3, *kp = (__m128i *) key;
	kp[0] = x0 = _mm_loadu_si128((__m128i *) userkey);
	kp[1] = x3 = _mm_loadu_si128((__m128i *) (userkey + 16));
	x2 = _mm_setzero_si128();
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 1);
	kp[2] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 1);
	kp[3] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 2);
	kp[4] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 2);
	kp[5] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 4);
	kp[6] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 4);
	kp[7] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 8);
	kp[8] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 8);
	kp[9] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 16);
	kp[10] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 16);
	kp[11] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 32);
	kp[12] = x0;
	EXPAND_ASSIST(x3, x1, x2, x0, 170, 32);
	kp[13] = x3;
	EXPAND_ASSIST(x0, x1, x2, x3, 255, 64);
	kp[14] = x0;
}


int MY_AES_set_encrypt_key(const unsigned char *userKey, const int bits,
		MY_AES_KEY *key) {
	if (bits == 128) {
		MY_AES_128_Key_Expansion(userKey, key);
	} else if (bits == 192) {
		MY_AES_192_Key_Expansion(userKey, key);
	} else if (bits == 256) {
		MY_AES_256_Key_Expansion(userKey, key);
	}
#if (OCB_KEY_LEN == 0)
	key->rounds = 6 + bits / 32;
#endif
	return 0;
}

 void MY_AES_set_decrypt_key_fast(MY_AES_KEY *dkey, const MY_AES_KEY *ekey) {
	int j = 0;
	int i = ROUNDS(ekey);
#if (OCB_KEY_LEN == 0)
	dkey->rounds = i;
#endif
	dkey->rd_key[i--] = ekey->rd_key[j++];
	while (i)
		dkey->rd_key[i--] = _mm_aesimc_si128(ekey->rd_key[j++]);
	dkey->rd_key[i] = ekey->rd_key[j];
}

 int MY_AES_set_decrypt_key(const unsigned char *userKey, const int bits,
		MY_AES_KEY *key) {
	MY_AES_KEY temp_key;
	MY_AES_set_encrypt_key(userKey, bits, &temp_key);
	MY_AES_set_decrypt_key_fast(key, &temp_key);
	return 0;
}

 void MY_AES_encrypt(const unsigned char *in, unsigned char *out,
		const MY_AES_KEY *key) {
	int j, rnds = ROUNDS(key);
	const __m128i *sched = ((__m128i *) (key->rd_key));
	__m128i tmp = _mm_load_si128((__m128i *) in);
  
	tmp = _mm_xor_si128(tmp, sched[0]);
	for (j = 1; j < rnds; j++){
		tmp = _mm_aesenc_si128(tmp, sched[j]);
  }
  tmp = _mm_aesenclast_si128(tmp, sched[j]);
	_mm_store_si128((__m128i *) out, tmp);
}

 void MY_AES_decrypt(const unsigned char *in, unsigned char *out,
		const MY_AES_KEY *key) {
	int j, rnds = ROUNDS(key);
	const __m128i *sched = ((__m128i *) (key->rd_key));
	__m128i tmp = _mm_load_si128((__m128i *) in);
	tmp = _mm_xor_si128(tmp, sched[0]);
	for (j = 1; j < rnds; j++)
		tmp = _mm_aesdec_si128(tmp, sched[j]);
	tmp = _mm_aesdeclast_si128(tmp, sched[j]);
	_mm_store_si128((__m128i *) out, tmp);
}

 void MY_AES_ecb_encrypt_blks(block *blks, unsigned nblks, MY_AES_KEY *key) {
	unsigned i, j, rnds = ROUNDS(key);
	const __m128i *sched = ((__m128i *) (key->rd_key));
	for (i = 0; i < nblks; ++i)
		blks[i] = _mm_xor_si128(blks[i], sched[0]);
	for (j = 1; j < rnds; ++j)
		for (i = 0; i < nblks; ++i)
			blks[i] = _mm_aesenc_si128(blks[i], sched[j]);
	for (i = 0; i < nblks; ++i)
		blks[i] = _mm_aesenclast_si128(blks[i], sched[j]);
}

 void MY_AES_ecb_encrypt_blks_4(block *blks, MY_AES_KEY *key) {
	unsigned i, j, rnds = ROUNDS(key);
	const __m128i *sched = ((__m128i *) (key->rd_key));
	blks[0] = _mm_xor_si128(blks[0], sched[0]);
	blks[1] = _mm_xor_si128(blks[1], sched[0]);
	blks[2] = _mm_xor_si128(blks[2], sched[0]);
	blks[3] = _mm_xor_si128(blks[3], sched[0]);

	for (j = 1; j < rnds; ++j){
		blks[0] = _mm_aesenc_si128(blks[0], sched[j]);
		blks[1] = _mm_aesenc_si128(blks[1], sched[j]);
		blks[2] = _mm_aesenc_si128(blks[2], sched[j]);
		blks[3] = _mm_aesenc_si128(blks[3], sched[j]);
	}
	blks[0] = _mm_aesenclast_si128(blks[0], sched[j]);
	blks[1] = _mm_aesenclast_si128(blks[1], sched[j]);
	blks[2] = _mm_aesenclast_si128(blks[2], sched[j]);
	blks[3] = _mm_aesenclast_si128(blks[3], sched[j]);
}


 void MY_AES_ecb_decrypt_blks(block *blks, unsigned nblks, MY_AES_KEY *key) {
	unsigned i, j, rnds = ROUNDS(key);
	const __m128i *sched = ((__m128i *) (key->rd_key));
	for (i = 0; i < nblks; ++i)
		blks[i] = _mm_xor_si128(blks[i], sched[0]);
	for (j = 1; j < rnds; ++j)
		for (i = 0; i < nblks; ++i)
			blks[i] = _mm_aesdec_si128(blks[i], sched[j]);
	for (i = 0; i < nblks; ++i)
		blks[i] = _mm_aesdeclast_si128(blks[i], sched[j]);
}
void MY_AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
                     size_t len, const MY_AES_KEY *key,
                     unsigned char *ivec, const int enc)
{

    if (enc)
        MY_CRYPTO_cbc128_encrypt(in, out, len, key, ivec,
                              (my_block128_f) MY_AES_encrypt);
    else
        MY_CRYPTO_cbc128_decrypt(in, out, len, key, ivec,
                              (my_block128_f) MY_AES_decrypt);
}

#define STRICT_ALIGNMENT 0

void MY_CRYPTO_cbc128_encrypt(const unsigned char *in, unsigned char *out,
                           size_t len, const void *key,
                           unsigned char ivec[16], my_block128_f block)
{
    size_t n;
    const unsigned char *iv = ivec;

    //assert(in && out && key && ivec);

    if (STRICT_ALIGNMENT &&
        ((size_t)in | (size_t)out | (size_t)ivec) % sizeof(size_t) != 0) {
        while (len >= 16) {
            for (n = 0; n < 16; ++n)
                out[n] = in[n] ^ iv[n];
            (*block) (out, out, key);
            iv = out;
            len -= 16;
            in += 16;
            out += 16;
        }
    } else {
        while (len >= 16) {
            for (n = 0; n < 16; n += sizeof(size_t))
                *(size_t *)(out + n) =
                    *(size_t *)(in + n) ^ *(size_t *)(iv + n);
            (*block) (out, out, key);
            iv = out;
            len -= 16;
            in += 16;
            out += 16;
        }
    }

    while (len) {
        for (n = 0; n < 16 && n < len; ++n)
            out[n] = in[n] ^ iv[n];
        for (; n < 16; ++n)
            out[n] = iv[n];
        (*block) (out, out, key);
        iv = out;
        if (len <= 16)
            break;
        len -= 16;
        in += 16;
        out += 16;
    }
    memcpy(ivec, iv, 16);
}

void MY_CRYPTO_cbc128_decrypt(const unsigned char *in, unsigned char *out,
                           size_t len, const void *key,
                           unsigned char ivec[16], my_block128_f block)
{
    size_t n;
    union {
        size_t t[16 / sizeof(size_t)];
        unsigned char c[16];
    } tmp;

    //assert(in && out && key && ivec);

    if (in != out) {
        const unsigned char *iv = ivec;

        if (STRICT_ALIGNMENT &&
            ((size_t)in | (size_t)out | (size_t)ivec) % sizeof(size_t) != 0) {
            while (len >= 16) {
                (*block) (in, out, key);
                for (n = 0; n < 16; ++n)
                    out[n] ^= iv[n];
                iv = in;
                len -= 16;
                in += 16;
                out += 16;
            }
        } else if (16 % sizeof(size_t) == 0) { /* always true */
            while (len >= 16) {
                size_t *out_t = (size_t *)out, *iv_t = (size_t *)iv;

                (*block) (in, out, key);
                for (n = 0; n < 16 / sizeof(size_t); n++)
                    out_t[n] ^= iv_t[n];
                iv = in;
                len -= 16;
                in += 16;
                out += 16;
            }
        }
        memcpy(ivec, iv, 16);
    } else {
        if (STRICT_ALIGNMENT &&
            ((size_t)in | (size_t)out | (size_t)ivec) % sizeof(size_t) != 0) {
            unsigned char c;
            while (len >= 16) {
                (*block) (in, tmp.c, key);
                for (n = 0; n < 16; ++n) {
                    c = in[n];
                    out[n] = tmp.c[n] ^ ivec[n];
                    ivec[n] = c;
                }
                len -= 16;
                in += 16;
                out += 16;
            }
        } else if (16 % sizeof(size_t) == 0) { /* always true */
            while (len >= 16) {
                size_t c, *out_t = (size_t *)out, *ivec_t = (size_t *)ivec;
                const size_t *in_t = (const size_t *)in;

                (*block) (in, tmp.c, key);
                for (n = 0; n < 16 / sizeof(size_t); n++) {
                    c = in_t[n];
                    out_t[n] = tmp.t[n] ^ ivec_t[n];
                    ivec_t[n] = c;
                }
                len -= 16;
                in += 16;
                out += 16;
            }
        }
    }

    while (len) {
        unsigned char c;
        (*block) (in, tmp.c, key);
        for (n = 0; n < 16 && n < len; ++n) {
            c = in[n];
            out[n] = tmp.c[n] ^ ivec[n];
            ivec[n] = c;
        }
        if (len <= 16) {
            for (; n < 16; ++n)
                ivec[n] = in[n];
            break;
        }
        len -= 16;
        in += 16;
        out += 16;
    }
}
