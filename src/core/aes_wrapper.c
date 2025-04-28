#define _WIN32_WINNT 0x0A00 // Windows 10 and later
#include <windows.h>
#include <threadpoolapiset.h>
#include <winbase.h> // Pour les fonctions de gestion de fichiers
#include <processthreadsapi.h> // Parfois utile pour certains contextes
#include <synchapi.h>          // Synchronisation si besoin
#include <winnt.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/aes_wrapper.h"


#define CHUNK_SIZE 16
#define VIEW_SIZE  (64 * 1024) // 64 KB

typedef struct {
    uint32_t *ptr;
    SIZE_T size;
    state_t keys[10]; 
} EncryptJob;

void uint32ToUint8Array(uint32_t input, uint8_t output[4]) {
    output[0] = (uint8_t)(input & 0xFF);
    output[1] = (uint8_t)((input >> 8) & 0xFF);
    output[2] = (uint8_t)((input >> 16) & 0xFF);
    output[3] = (uint8_t)((input >> 24) & 0xFF);
}

VOID CALLBACK EncryptWorkCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
    EncryptJob* job = (EncryptJob*)context;
    uint32_t* data = (uint32_t*)job->ptr;
    SIZE_T size = job->size;
    state_t* keys = job->keys; // Use the keys from the job

    state_t dataState;
    for (int i=0; i < 4; i++) {
        uint8_t block[4];
        uint32ToUint8Array(data[i], block);
        printf("Block %d: %02X %02X %02X %02X\n", i, block[0], block[1], block[2], block[3]);
        for (int j=0; j < 4; j++) {
            dataState[j][i] = block[j];
        }
        printState(dataState);
    }

    /* // Encrypt the data in chunks of 16 bytes
    for (SIZE_T i = 0; i < size; i += CHUNK_SIZE) {
        uint8_t block[CHUNK_SIZE];
        memcpy(block, data + i / sizeof(uint32_t), CHUNK_SIZE);
        aes(keys, block); // Chiffrement AES
        memcpy(data + i / sizeof(uint32_t), block, CHUNK_SIZE);
    } */

    free(job); // Libérer la mémoire allouée pour le travail
}

void DispatchEcryption(state_t *keys, uint32_t* vue_address)
{
    PTP_POOL pool = CreateThreadpool(NULL);
    
    if (!pool) {
        printf("CreateThreadpool failed.\n");
        return;
    }

    SetThreadpoolThreadMaximum(pool, 4); // Par exemple 4 threads
    SetThreadpoolThreadMinimum(pool, 1);

    PTP_CALLBACK_ENVIRON env = malloc(sizeof(TP_CALLBACK_ENVIRON));
    if (!env) {
        printf("Memory allocation failed : %d\n", GetLastError());
        CloseThreadpool(pool);
        return;
    }
    InitializeThreadpoolEnvironment(env);
    SetThreadpoolCallbackPool(env, pool);
    
    for (SIZE_T offset = 0; offset < VIEW_SIZE; offset += CHUNK_SIZE) {
        EncryptJob* job = (EncryptJob*)malloc(sizeof(EncryptJob));
        job->ptr = vue_address + offset;
        job->size = CHUNK_SIZE;

        PTP_WORK work = CreateThreadpoolWork(EncryptWorkCallback, job, env);
        if (work) {
            SubmitThreadpoolWork(work);
            CloseThreadpoolWork(work); // libérera automatiquement après exécution
        }
    }

    // Attention la fonction est mal paramétrée, il faut donner un work en paramètre
    WaitForThreadpoolWorkCallbacks(env, FALSE); // Attendre la fin de tous les travaux
    
    CloseThreadpool(env);
}
