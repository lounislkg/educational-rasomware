#include "core/my_aes.h"

/* // Example state for testing purposes
// Will be replaced with the input stream
state_t state = {
    {0x23, 0x45, 0x67, 0x89},
    {0xAB, 0xCD, 0xEF, 0x01},
    {0x09, 0x54, 0x67, 0x89},
    {0xCB, 0xA3, 0xEF, 0x10}}; */

// Sbox table for SubBytes transformation
// TODO: Edit it to use a more discret Sbox table
static const uint8_t sbox[256] = {
    // 0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

// Rcon table for KeySchedule (this table works for a maximum of 10 keys)
static const uint8_t Rcon[11] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

// Function to print the state matrix
void printState(state_t state)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            printf("0x%X ", state[i][j]);
        }
        printf("\n");
    }
}

// Rotate the word to the left
static void RotWord(uint8_t *word)
{
    uint8_t temp = *word;
    for (int i = 0; i < 3; i++)
    {
        word[i] = word[i + 1];
    }
    word[3] = temp;
}

static void XOR(uint8_t *word1, uint8_t *word2)
{
    for (int i = 0; i < 4; i++)
    {
        word1[i] ^= word2[i];
    }
}

// Generate a random AES key of 128 bits
static void KeyExpansion(state_t key, state_t new_key, int round)
{
    // Fill new_key with 0
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            new_key[i][j] = 0;
        }
    }
    // Generate the new key from the existing key

    uint8_t word[4];

    word[0] = key[0][3];
    word[1] = key[1][3];
    word[2] = key[2][3];
    word[3] = key[3][3];
    // Rotate the word
    RotWord(word);

    // Apply the S-box to the word
    for (int j = 0; j < 4; j++)
    {
        word[j] = sbox[word[j]];
    }

    // build the RCON word
    // Warning : With this method you can't generate more than 16 keys
    uint8_t rcon[4] = {Rcon[round], 0, 0, 0};
    // XOR the word with the RCON word if we are in the first column
    XOR(word, rcon);

    // build the column 4 column before in the original key
    uint8_t prev_key_word[4] = {key[0][0], key[1][0], key[2][0], key[3][0]};

    // XOR the word with the previous key
    XOR(word, prev_key_word);

    // Fill the new key with the new word
    for (int j = 0; j < 4; j++)
    {
        new_key[j][0] = word[j];
    }

    // Fill the rest of the new key with only a XOR with the previous key (3rd remaining columns)
    for (int i = 1; i < 4; i++)
    {
        // No need to modify the word, just use the one that is already in the new key

        // Copy the word from the same column in the prev key
        uint8_t prev_key_word[4] = {key[0][i], key[1][i], key[2][i], key[3][i]};
        // XOR the word with the previous key
        XOR(word, prev_key_word);
        // Fill the new key with the new word
        for (int j = 0; j < 4; j++)
        {
            new_key[j][i] = word[j];
        }
    }
}

// Generate 10 keys from the base key
void GenerateTenKeys(state_t key, state_t *keys)
{
    state_t new_key = {{0}};     // Initialize new_key to zero
    state_t pending_key = {{0}}; // Initialize pending_key to zero
    // Copy the base key to the pending key
    for (int j = 0; j < 4; j++)
    {
        for (int k = 0; k < 4; k++)
        {
            pending_key[j][k] = key[j][k];
        }
    }
    // Generate 10 keys from the base key
    for (int i = 0; i < 10; i++)
    {
        KeyExpansion(pending_key, new_key, i);
        // Copy the new key to the keys
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                keys[i][j][k] = new_key[j][k];
            }
        }
        // Copy the new key to the pending key
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                pending_key[j][k] = new_key[j][k];
            }
        }
    }
}

// SubBytes
static void SubByte(state_t byte)
{
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            byte[c][r] = sbox[byte[c][r]]; // Substitute the byte using the S-box
        }
    }
}

// ShittRows function
static void ShiftRows(state_t state, state_t new_state)
{
    // Fill new_state with 0
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            new_state[i][j] = 0;
        }
    }
    for (int i = 0; i < 4; i++)
    {
        // Shift the row to the left by i positions
        for (int j = 0; j < 4; j++)
        {
            new_state[i][j] = state[i][(j + i) % 4];
        }
    }
}

static uint8_t xtime(uint8_t x)
{
    return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

// MixColumns function mixes the columns of the state matrix
static void MixColumns(state_t *state)
{
    uint8_t i;
    uint8_t Tmp, Tm, t;
    for (i = 0; i < 4; ++i)
    {
        t = (*state)[i][0];
        Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3];
        Tm = (*state)[i][0] ^ (*state)[i][1];
        Tm = xtime(Tm);
        (*state)[i][0] ^= Tm ^ Tmp;
        Tm = (*state)[i][1] ^ (*state)[i][2];
        Tm = xtime(Tm);
        (*state)[i][1] ^= Tm ^ Tmp;
        Tm = (*state)[i][2] ^ (*state)[i][3];
        Tm = xtime(Tm);
        (*state)[i][2] ^= Tm ^ Tmp;
        Tm = (*state)[i][3] ^ t;
        Tm = xtime(Tm);
        (*state)[i][3] ^= Tm ^ Tmp;
    }
}

// Function to Add round key to the state
static void AddRoundKey(state_t *state, const state_t RoundKey)
{
    for (int i = 0; i < 4; i++)
    {
        // Colonne i
        (*state)[0][i] ^= RoundKey[0][i]; // XOR the round key with the state
        (*state)[1][i] ^= RoundKey[1][i];
        (*state)[2][i] ^= RoundKey[2][i];
        (*state)[3][i] ^= RoundKey[3][i];
    }
}

// Function that handle the rounds (ten round for AES-128)
static void Round(state_t state, state_t key)
{
    // Call the SubByte function
    SubByte(state);

    // Call the ShiftRows function
    state_t new_state = {{0}}; // Initialize new_state to zero
    ShiftRows(state, new_state);

    // Call the MixColumns function
    MixColumns(&new_state);

    // Call the AddRoundKey function
    AddRoundKey(&new_state, key);
}

int aes(state_t keys[10], state_t dataState)
{
    /* // Extract the state from the command line arguments
    state_t *state = dataState; // Initialize all values to zero
    printf("Argv : %s\n", bytes);
    if (bytes == NULL || keys == NULL)
    {
        printf("Usage:\n%s <16 hex values>\n", bytes);
        return 1;
    }
    for (int i = 0; i < 16; i++)
    {
        char buff[2] = {bytes[i * 2], bytes[i * 2 + 1]}; // Create a string of 2 chars
        sscanf(buff, "%2hhx", &state[i / 4][i % 4]);
    } */
    state_t state = {{0}}; // Initialize all values to zero
    // Fill the state with the dataState
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            state[i][j] = dataState[i][j];
        }
    }
    
   /*  // Print the initial state
    printf("Initial State:\n");
    printState(state);
 */
    // Call the Round function 10 times for the 10 rounds
    for (int i = 0; i < 10; i++)
    {
        Round(state, keys[i]);
    }

    // Last round without MixColumns
    SubByte(state);
    state_t new_state = {{0}}; // Initialize new_state to zero
    ShiftRows(state, new_state);
    AddRoundKey(&new_state, keys[9]);
/* 
    printf("Final State:\n");
    printState(new_state);
 */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            dataState[i][j] = new_state[i][j];
        }
        
    }
    
    

    // Check if there's something to free

    return 0;
}
/* 
int main()
{
    generateRandomSeed(); // Initialize the random seed
    // Generate a random key of 128 bits (16 bytes)
    state_t key;
    generateRandomKey(key);

    printf("Random Key:\n");
    printState(key);

    // Generate 10 keys from the base key
    state_t keys[10] = {{{0}}}; // Initialize all keys to zero (the base_key is added in the function)
    // Generate the keys (can't be more than 10 keys because of the Rcon table)
    GenerateTenKeys(key, keys);

    aes(keys, "23456789ABCDEF0109546789CBA3EF10");

    // Send the key to the server
    char buff[257] = {'X'}; // 256 bytes + 1 for the null terminator

    uint8_t key_elongate[16] = {0}; // 16 bytes
    // Convert the key to a char array of size 16 bytes
    for (int i = 0; i < 16; i++)
    {
        // Convert to char
        key_elongate[i] = key[i / 4][i % 4];
    }

    // Encrypt the message using RSA
    rsa(key_elongate, buff);
    int i = 0;
    printf("Encrypted Key:\n'");
    while (buff[i] != 0)
    {
        printf("%c", buff[i]);
        i += 1;
    }
    printf("'\n");
    printf("Encrypted Key length: %d\n", i);

    // Send the key to the server (16 bytes)
    if (SendKey(buff) == 0)
    {
        return 0;
    }
    else
    {
        printf("Send failed\n");
        return 1;
    }
} */