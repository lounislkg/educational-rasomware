#include <stdint.h>
#include <stdio.h>

typedef uint8_t state_t[4][4];

void printState(state_t state);
void GenerateRandomKey(state_t key);
void GenerateTenKeys(state_t key, state_t *keys);
int aes(state_t *keys, char *message);