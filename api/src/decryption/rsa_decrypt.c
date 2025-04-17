#include "bn.h"
#include <stdio.h>

// see right to left binary method for modular exponentiation
// https://cryptography.fandom.com/wiki/Modular_exponentiation
/* O(log n) */
void pow_mod_faster(struct bn *a, struct bn *b, struct bn *n, struct bn *res)
{
    bignum_from_int(res, 1); /* r = 1 */

    struct bn tmpa;
    struct bn tmpb;
    struct bn tmp;
    bignum_assign(&tmpa, a);
    bignum_assign(&tmpb, b);

    while (1)
    {
        if (tmpb.array[0] & 1) /* if (b % 2) */
        {
            bignum_mul(res, &tmpa, &tmp); /*   r = r * a % m */
            bignum_mod(&tmp, n, res);
        }
        bignum_rshift(&tmpb, &tmp, 1); /* b /= 2 */
        bignum_assign(&tmpb, &tmp);

        if (bignum_is_zero(&tmpb))
            break;

        bignum_mul(&tmpa, &tmpa, &tmp);
        bignum_mod(&tmp, n, &tmpa);
    }
}



int main(int argc, char *argv[])
{
    
    // d is the private key, n is the modulus, c is the ciphertext, m is the plaintext
    // the private key is somewhere in the range od 0 and infinity GL
    struct bn d, n;
    bignum_init(&n);
    bignum_init(&d);
    struct bn c, m;
    bignum_init(&c);
    bignum_init(&m);

    char *rsa_modulus_hex = "c814a57a2fe60c2d8252eac3b3324546787a8957d720511bf613d237bffbe0954757ee781e5c0d60295c170eaa0a819abe24336b1e5cf6d46c2a6a4c10901b3a478f575a343d43ce96c0f7052da3e5d420703c2b42aacca9d70851bd9bfe4be8c7d03f3e484b00edc95afc8f9c83a67b98c7eb4acee0ce15dbf9960655e7b7c7";
    bignum_from_string(&n, rsa_modulus_hex, 128);
    pow_mod_faster(&c, &d, &n, &m);
}
