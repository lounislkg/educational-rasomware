// Compiler: mingw-w64-x86_64-posix-seh
// Version: 14.2.0
#ifndef UTILITAIRE_H
#define UTILITAIRE_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // ou 0x0601 si Windows 7 minimum
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib") // Lien vers la bibliothèque Winsock (utile uniquement pour MSVC. GCC/Mingw l'ignore)

#if (MINGW)
target_link_libraries(boost_asio INTERFACE ws2_32) #mswsock wsock32
#endif

int SendKey(char key[257]);

#endif // UTILITAIRE_H
