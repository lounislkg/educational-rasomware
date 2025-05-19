# Posix Version (POSIX.1-2008) /!\ the resulting executable is only for Windows
# Download a recent version of gcc here : https://github.com/niXman/mingw-builds-binaries/releases 
# x86_64-15.1.0-release-win32-seh-ucrt-rt_v12-rev0.7z (recommended)
# Compilateur et options
CC = x86_64-w64-mingw32-gcc
CFLAGS = -O0 -g -I./include
LDFLAGS = -lws2_32 -lntdll -lkernel32
TARGET = test.exe

# Fichiers sources C
SRC_C = \
  ./src/core/my_aes.c \
  ./src/core/rsa.c \
  ./src/utils/rng.c \
  ./src/utils/bn.c \
  ./src/core/api_client.c \
  ./src/core/encrypter.c \
  ./src/core/aes_wrapper.c \
  ./src/core/list_files.c \
  ./main.c

# Fichiers objets assembleur (déjà compilés, ou à assembler à part)
ASM_OBJ = \
  ./src/core/assembly/NtCreateFile.o \
  ./src/core/assembly/NtMapViewOfSection.o \
  ./src/core/assembly/NtCreateSection.o

# Règle par défaut
all: $(TARGET)

# Compilation de l'exécutable
$(TARGET): $(SRC_C) $(ASM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Nettoyage des fichiers générés
clean:
	rm -f $(TARGET)

.PHONY: all clean

