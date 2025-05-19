#include <stdio.h>
#define __WIN32_WINNT 0x0600 // Windows 7 and later
#include "core/aes_wrapper.h"
#include <windows.h>
#include <threadpoolapiset.h>

#define CHUNK_SIZE 4
#define CHUNK_SIZE_ELEMENT 4 // 4 octets par uint32_t * 4 = 16 octets
// chunk size  explication
// Car on travail avec des uint32_t donc ils contiennent 4 octets de mémoire :
// 16 octets par vecteur state / 4 octets par chunk  => de faire un offset qui s'incrémente de 4 par itération.

typedef struct
{
    uint32_t *ptr;
    SIZE_T size;
    state_t keys[10];
} EncryptJob;

void uint32ToUint8Array(uint32_t input, uint8_t output[4])
{
    output[0] = (uint8_t)(input & 0xFF); // little-endian / LSB (octet de poid faible)
    output[1] = (uint8_t)((input >> 8) & 0xFF);
    output[2] = (uint8_t)((input >> 16) & 0xFF);
    output[3] = (uint8_t)((input >> 24) & 0xFF); // big-endian (octet de poid fort)
}
/*
void uint8ArrayToUint32(uint8_t input[4],  out) {
    uint32_t =
} */

VOID CALLBACK EncryptWorkCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
{
    EncryptJob *job = (EncryptJob *)context;
    uint32_t *data = job->ptr;
    SIZE_T size = job->size;
    state_t *keys = job->keys; // Use the keys from the job

    state_t dataState = {{0}};
    for (int i = 0; i < 4; i++)
    {
        uint8_t block[4];
        uint32ToUint8Array(data[i], block);
        // printf("[Thread %lu] Block %d: %02X %02X %02X %02X\n", threadId, i, block[0], block[1], block[2], block[3]);
        for (int j = 0; j < 4; j++)
        {
            dataState[j][i] = block[j];
        }
    }
    // Encrypt the data using AES
    aes(keys, dataState); // Chiffrement AES
    // Write the encrypted data back to the original location
    for (int i = 0; i < 4; i++)
    {
        uint32_t encryptedBlock = 0;
        for (int j = 0; j < 4; j++)
        {
            encryptedBlock |= (dataState[j][i] << (j * 8));
        }
        // printf("[Thread %lu] Encrypted Block %d: %08X\n", threadId, i, data[i]);
        data[i] = encryptedBlock;
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

void DispatchEcryption(state_t *keys, uint32_t *vue_address, uint64_t view_size)
{
    PTP_POOL pool = CreateThreadpool(NULL);

    if (!pool)
    {
        printf("CreateThreadpool failed.\n");
        return;
    }

    SetThreadpoolThreadMaximum(pool, 4); // Par exemple 4 threads
    SetThreadpoolThreadMinimum(pool, 1);

    /*   PTP_CALLBACK_ENVIRON env = malloc(sizeof(TP_CALLBACK_ENVIRON));
      if (!env) {
          printf("Memory allocation failed : %d\n", GetLastError());
          CloseThreadpool(pool);
          return;
      }
   */
    TP_CALLBACK_ENVIRON env; // Sur la pile, pas avec malloc
    InitializeThreadpoolEnvironment(&env);

    PTP_CLEANUP_GROUP cleanupGroup = CreateThreadpoolCleanupGroup();
    if (cleanupGroup == NULL)
    {
        printf("CreateThreadpoolCleanupGroup failed.\n");
        CloseThreadpool(pool);
        DestroyThreadpoolEnvironment(&env);
        return;
    }

    // InitializeThreadpoolEnvironment(env);
    SetThreadpoolCallbackPool(&env, pool);
    SetThreadpoolCallbackCleanupGroup(&env, cleanupGroup, NULL);
    printf("View size: %llu\n", view_size);
    int i = 0;
    uint64_t num_elements = view_size / sizeof(uint32_t);
    for (SIZE_T element_offset = 0; element_offset < num_elements; element_offset += CHUNK_SIZE_ELEMENT)
    {
        // printf("Dispatching encryption for chunk at offset %llu\n", (unsigned long long)offset);
        if (element_offset + CHUNK_SIZE_ELEMENT > view_size)
        {
            break; // Ne pas dépasser la taille de la vue
        }
        EncryptJob *job = (EncryptJob *)malloc(sizeof(EncryptJob));
        if (!job)
        {
            printf("Memory allocation failed for job.\n");
            continue;
        }
        job->ptr = vue_address + element_offset;
        job->size = CHUNK_SIZE_ELEMENT;
        memcpy(job->keys, keys, sizeof(state_t) * 10); // Copier les clés
        PTP_WORK work = CreateThreadpoolWork(EncryptWorkCallback, job, &env);
        if (work)
        {
            SubmitThreadpoolWork(work);
        }
        else
        {
            printf("Falied to create work item.\n");
            free(job);
        }
    }
    
    printf("Waiting for all work to complete...\n");
    // WaitForThreadpoolWorkCallbacks(cleanupGroup, FALSE); // Attendre que tous les travaux soient terminés
    CloseThreadpoolCleanupGroupMembers(cleanupGroup, FALSE, NULL);
    CloseThreadpoolCleanupGroup(cleanupGroup);
    DestroyThreadpoolEnvironment(&env);
    CloseThreadpool(pool);

    printf("All work submitted.\n");
}
