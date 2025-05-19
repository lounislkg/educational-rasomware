#include "core/list_files.h"

void initHandleArray(FileDataArray *arr)
{
    arr->length = 0;
    arr->capacity = 100;
    arr->fd = malloc(10 * sizeof(FileData));
}

int addHandle(FileDataArray *arr, WIN32_FIND_DATA win32_fd, char file_path[3096])
{
    // Allocate memory for the file name and copy it
    char *copiedFileName = malloc(strlen(file_path) + 1);
    if (!copiedFileName)
    {
        fprintf(stderr, "Memory allocation error for file name\n");
        return 1;
    }
    strcpy(copiedFileName, file_path);

    FileData fd_to_add = {
        .cFileName = copiedFileName,                   // Exemple d'attribut
        .ftLastAccessTime = win32_fd.ftLastAccessTime, // Exemple d'attribut
        .nFileSizeHigh = win32_fd.nFileSizeHigh        // Exemple d'attribut
    };
    arr->length++;
    if (arr->length >= arr->capacity)
    {
        // augmenter la capacité (par exemple x2 ou +4)
        size_t newCapacity = arr->capacity + 100;

        FileData *newFd_array = realloc(arr->fd, newCapacity * sizeof(FileData));
        if (!newFd_array)
        {
            // TODO: Handle error so it won't comprize the attack if the memory can't be allocated
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            return 1;
        }
        arr->fd = newFd_array;
        arr->capacity = newCapacity;
    }

    arr->fd[arr->length] = fd_to_add;
    return 0;
}

void freeHandleArray(FileDataArray *arr)
{
    free(arr->fd);
    arr->fd = NULL;
    arr->length = 0;
    arr->capacity = 0;
}

void list_files(char *dir, FileDataArray *arr, DWORD *file_size)
{
    char o_dir[MAX_PATH]; // Original directory name to prevent from *.* being added to the path
    snprintf(o_dir, sizeof(o_dir), "%s", dir);
    strcat(dir, "\\*.*");
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(dir, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("Error finding files in directory: %s\n", dir);
        return;
    }

    do
    {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            printf("Directory: %s\n", findFileData.cFileName);
            if (findFileData.cFileName[0] == '.' && (findFileData.cFileName[1] == '\0' || (findFileData.cFileName[1] == '.' && findFileData.cFileName[2] == '\0')))
            {
                // Skip the current and parent directory entries
                continue;
            }
            // MAX_PATH is defined in windows.h and is typically 260 characters
            char new_dir[MAX_PATH];
            snprintf(new_dir, MAX_PATH, "%s\\%s", o_dir, findFileData.cFileName);
            if (strlen(dir) + strlen(new_dir) > MAX_PATH)
            {
                printf("Path too long: %s\n", new_dir);
            }
            list_files(new_dir, arr, file_size); // Recursive call to list_files
        }
        else
        {
            // printf("File: %s\n", findFileData.cFileName);
            // Add the file to the array
            char file_path[3096];
            snprintf(file_path, 3096, "%s\\%s", o_dir, findFileData.cFileName);
            if (addHandle(arr, findFileData, file_path) != 0)
            {
                fprintf(stderr, "Error adding file to array\n");
                return;
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}
