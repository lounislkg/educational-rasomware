#include "utils/bn.h"
#include <stdio.h>
#include "utils/rng.h"

#define RSA_SIZE 128 // 1024 bits

int rsa(uint8_t message[16], char buff[256]);
