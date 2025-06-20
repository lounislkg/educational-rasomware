#include <core/find_encryption_ratio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    const char *extension;
    const char *category;
    int encryption_ratio_inverse; // ex: 3 = chiffrer 1/3 du fichier
} FileEncryptionProfile;

const FileEncryptionProfile encryption_profiles[] = {
    // Executables
    { "exe",    "Executable Windows",         20 },
    { "dll",    "Bibliothèque dynamique",     20 },
    { "elf",    "Executable Linux",           20 },
    { "so",     "Shared Object Linux",        20 },
    { "bin",    "Binaire brut",               10 },

    // Documents Office
    { "doc",    "Document Word (ancien)",     5 },
    { "docx",   "Document Word",              7 },
    { "xls",    "Excel (ancien)",             5 },
    { "xlsx",   "Feuille de calcul Excel",    7 },
    { "ppt",    "PowerPoint (ancien)",        5 },
    { "pptx",   "Présentation PowerPoint",    7 },
    { "odt",    "Document LibreOffice",       10 },
    { "pdf",    "Document PDF",               7 },

    // Archives
    { "zip",    "Archive ZIP",                4 },
    { "rar",    "Archive RAR",                4 },
    { "7z",     "Archive 7-Zip",              4 },
    { "tar",    "Archive TAR",                7 },
    { "gz",     "Compression gzip",           5 },

    // Bases de données
    { "db",     "Base de données générique",  10 },
    { "sqlite", "Base SQLite",                13 },
    { "mdb",    "Access Database",            10 },
    { "accdb",  "Access (moderne)",           10 },

    // Images
    { "jpg",    "Image JPEG",                 3 },
    { "jpeg",   "Image JPEG",                 3 },
    { "png",    "Image PNG",                  4 },
    { "bmp",    "Image Bitmap",               5 },
    { "gif",    "Image GIF",                  4 },
    { "tiff",   "Image TIFF",                 3 },
    { "webp",   "Image WebP",                 4 },

    // Vidéos
    { "mp4",    "Vidéo MP4",                  5 },
    { "mkv",    "Vidéo Matroska",             5 },
    { "avi",    "Vidéo AVI",                  4 },
    { "mov",    "Vidéo QuickTime",            4 },
    { "flv",    "Vidéo Flash",                3 },
    { "wmv",    "Vidéo Windows Media",        4 },

    // Audio
    { "mp3",    "Audio MP3",                  5 },
    { "wav",    "Audio WAV",                  5 },
    { "flac",   "Audio FLAC",                 4 },
    { "ogg",    "Audio OGG",                  4 },
    { "m4a",    "Audio MPEG-4",               4 },

    // Codes / scripts (entièrement chiffrés)
    { "c",      "Code source C",              1 },
    { "cpp",    "Code source C++",            1 },
    { "py",     "Script Python",              1 },
    { "js",     "Script JavaScript",          1 },
    { "java",   "Code Java",                  1 },
    { "sh",     "Script shell",               1 },
    { "bat",    "Script batch Windows",       1 },
    { "ps1",    "Script PowerShell",          1 },
    { "html",   "HTML",                       1 },
    { "css",    "CSS",                        1 },

    // Données structurées
    { "csv",    "Données CSV",                1 },
    { "json",   "Données JSON",               1 },
    { "xml",    "Données XML",                1 },
    { "yml",    "YAML",                       1 },
    { "ini",    "Fichier de configuration",   1 },
    

    // Systèmes / configs
    { "log",    "Fichier de log",             1 },
    { "cfg",    "Configuration",              1 },
    { "sys",    "Fichier système",            5 },

    // Images disque
    { "iso",    "Image ISO",                  5 },
    { "img",    "Image disque brute",         5 },
    { "vhd",    "Image disque virtuel",       5 },
    { "vmdk",   "Image VMware",               5 },

    // Divers
    { "bak",    "Fichier de sauvegarde",      10 },
    { "tmp",    "Fichier temporaire",         20 },
    { "swp",    "Swap temporaire",            20 },
    { "torrent", "Fichier torrent",           10 },
    { "md",     "Markdown",                   1 },
    { "txt",    "Texte brut",                 1 },
    
};

const size_t encryption_profiles_count = sizeof(encryption_profiles) / sizeof(encryption_profiles[0]);


double get_encryption_density(const char *ext) {
    for (size_t i = 0; i < encryption_profiles_count; ++i) {
        if (strcmp(encryption_profiles[i].extension, ext) == 0) {
            return encryption_profiles[i].encryption_ratio_inverse;
        }
    }
    return 1; // défaut si inconnu
}

void get_extentsion(const char *filename, char *extension, size_t max_len) {
    const char *dot = strrchr(filename, '.');
    if (dot && dot != filename) {
        strncpy(extension, dot + 1, max_len - 1);
        extension[max_len - 1] = '\0'; // Assurer la terminaison de chaîne
    } else {
        extension[0] = '\0'; // Pas d'extension
    }
    // Convertir en minuscules
    for (size_t i = 0; extension[i]; i++) {
        extension[i] = tolower(extension[i]);
    }
    printf("Extension: %s\n", extension); // Debug
}

int main(int argc, char const *argv[])
{
    get_extentsion("example.txt", "txt", 4);
    return 0;
}
