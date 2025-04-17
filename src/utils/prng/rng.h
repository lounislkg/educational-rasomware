#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

typedef uint8_t state_t[4][4];

uint64_t generateRandomSeed();

void generateRandomKey(state_t key);
uint8_t rand_nonzero_byte();
