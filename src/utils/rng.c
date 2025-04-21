#include "utils/rng.h"

uint64_t seed = 145278953;

uint64_t generateRandomSeed()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    uintptr_t ptr = (uintptr_t)&tv;
    seed = tv.tv_sec ^ tv.tv_usec ^ getpid() ^ ptr;
    return seed;
}

void generateRandomKey(state_t key)
{
    // Generate a random key of 128 bits (16 bytes)
    // For educational purpose, we will generate our own key
    // In a real implementation, you would use a secure and audited random number generator
    uint64_t seed = generateRandomSeed();
    for (size_t i = 0; i < 16; ++i)
    {
        seed ^= (seed >> 13);
        seed ^= (seed << 17);
        seed ^= (seed >> 5);
        key[i / 4][i % 4] = (uint8_t)(seed & 0xFF);
    }
}

// Génère un byte aléatoire non nul
uint8_t rand_nonzero_byte()
{
    uint8_t b;
    // On génère un byte aléatoire non nul
    do
    {
        seed ^= (seed >> 13);
        seed ^= (seed << 17);
        seed ^= (seed >> 5);
        b = (uint8_t)(seed & 0xFF);
    } while (b == 0x00);
    // printf("rand_nonzero_byte: %02X\n", b); // Debug print
    return b;
}