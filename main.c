#include "termsender.h"
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#endif

int main(int argc, char* argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: fbir <ip> <files...>\n");
        return 0;
    }
#ifdef WIN32
    {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
    }
#endif
    termsender_register();
    sender_send_files(argv[1], 5000, argc - 2, (const char**)argv + 2);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
