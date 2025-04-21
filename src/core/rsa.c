#include "core/rsa.h"

// see right to left binary method for modular exponentiation
// https://cryptography.fandom.com/wiki/Modular_exponentiation
/* O(log n) */
static void pow_mod_faster(struct bn *a, struct bn *b, struct bn *n, struct bn *res)
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

// Fait un padding PKCS#1 v1.5 pour RSA encryption
static int rsa_pad_v15(uint8_t *output, size_t k, const uint8_t *msg, size_t msg_len)
{
    if (msg_len > k - 11)
        return -1; // trop gros pour rentrer

    output[0] = 0x00;
    output[1] = 0x02;

    size_t ps_len = k - msg_len - 3;
    for (size_t i = 0; i < ps_len; ++i)
    {
        output[2 + i] = rand_nonzero_byte();
    }

    output[2 + ps_len] = 0x00;
    memcpy(output + 3 + ps_len, msg, msg_len);

    return 0;
}

int rsa(uint8_t message[16], char buff[256])
{
    generateRandomSeed(); // Initialize the random seed
    uint8_t padded[RSA_SIZE];
    size_t msg_len = 16; // 16 bytes

    if(rsa_pad_v15(padded, RSA_SIZE, message, msg_len) == -1)
    {
        printf("Message too long for RSA padding\n");
        return -1;
    }
    // n is the modulus, e is the public key
    struct bn n, e;
    bignum_init(&n);
    bignum_init(&e);
    // c is the ciphertext, m is the plaintext
    struct bn c, m;
    bignum_init(&c);
    bignum_init(&m);

    uint8_t rsa_exponent[3] = {0x01, 0x00, 0x01}; // 65537 */
    char *rsa_modulus_hex = "c814a57a2fe60c2d8252eac3b3324546787a8957d720511bf613d237bffbe0954757ee781e5c0d60295c170eaa0a819abe24336b1e5cf6d46c2a6a4c10901b3a478f575a343d43ce96c0f7052da3e5d420703c2b42aacca9d70851bd9bfe4be8c7d03f3e484b00edc95afc8f9c83a67b98c7eb4acee0ce15dbf9960655e7b7c7";
    bignum_from_int(&e, 65537);
    bignum_from_string(&n, rsa_modulus_hex, 128);
    
    // Connvert padded to a char array of size RSA_SIZE
    char padded_str[RSA_SIZE * 2] = {'X'}; // +1 for null terminator
    for (int i = 0; i < 256; i++)
    {
        // Convert to char
        snprintf(&padded_str[i * 2], 3, "%02X", padded[i]);
    }

    // Convert the message to an bignum
    // m = 0x00 || 0x02 || PS || 0x00 || message
    bignum_from_string(&m, padded_str, 32);
    
    // Encrypt the message using RSA
    // c = m^e mod n = pow_mod(m, e, n)
    // c = m^e % n
    pow_mod_faster(&m, &e, &n, &c);

    // Print the ciphertext
    bignum_to_string(&c, buff, 256);
    
    return 0;
}
/* 
int main()
{
    uint8_t message[] = {
        0xAE, 0xFE, 0x12, 0x83,
        0x4A, 0xE3, 0xFE, 0x2A,
        0xE3, 0x2A, 0xA3, 0x4E,
        0xCB, 0xB5, 0x65, 0xBE};
    char buff[256];
    rsa(message, buff);
    int i = 0;
    while (buff[i] != 0)
    {
        printf("%c", buff[i]);
        i += 1;
    }
    printf("\n");
}  */