#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define BUFSIZE (32 * 1024)

int running;
int sock;

#ifndef WIN32
#define closesocket close
#endif

void onsig(int sig) {
    fprintf(stderr, "Got interrupt signal\n");
    running = 0;
    closesocket(sock);
}

uint64_t htonll(uint64_t value) {
    int num = 42;
    if (*(char *)&num == 42) {
        uint32_t high_part = htonl((uint32_t)(value >> 32));
        uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
        return (((uint64_t)low_part) << 32) | high_part;
    } else {
        return value;
    }
}

int main(int argc, char* argv[]) {
    struct sockaddr_in name;

    signal(SIGABRT, onsig);
    signal(SIGINT, onsig);
#ifndef WIN32
    signal(SIGBREAK, onsig);
#endif
#ifdef WIN32
    {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
    }
#endif
    sock = socket(AF_INET, SOCK_STREAM, 0);
    name.sin_family = AF_INET;
    name.sin_port = htons (5000);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (const struct sockaddr *) &name, sizeof (name)) < 0) {
        fprintf(stderr, "Unable to bind\n");
        WSACleanup();
        return -1;
    }
    if (listen (sock, 16) < 0) {
        fprintf(stderr, "Unable to listen\n");
        WSACleanup();
        return -1;
    }
    running = 1;
    while (running) {
        int i, count;
        struct sockaddr_in rname;
        int rlen = sizeof (rname);
        int client = accept(sock, (struct sockaddr*)&rname, &rlen);
        if (client <= 0) {
            if (running)
                fprintf(stderr, "Unable to accept\n");
            break;
        }
        if (recv(client, (char*)&count, 4, 0) < 0) {
            if (running)
                fprintf(stderr, "Failed to receive count\n");
            break;
        }
        count = htonl(count);
        fprintf (stdout, "File count: %d\n", count);
        for (i = 0; i < count; ++i) {
            char buf[BUFSIZE];
            char s = 1;
            uint64_t fsize, trans;
            if (send(client, &s, 1, 0) < 0) {
                if (running)
                    fprintf(stderr, "Failed to send sig\n");
                goto end;
            }
            if (recv(client, (char*)&fsize, 8, 0) < 0) {
                if (running)
                    fprintf(stderr, "Failed to receive file size\n");
                goto end;
            }
            fsize = htonll(fsize);
            fprintf (stdout, "  File %d: 0/%llu", i + 1, fsize);
            trans = 0ULL;
            while (trans < fsize) {
                int c = recv(client, buf, BUFSIZE, 0);
                if (c < 0) {
                    if (running)
                        fprintf(stderr, "Failed to receive file content\n");
                    goto end;
                }
                trans += (uint64_t)c;
                fprintf (stdout, "\r  File %d: %lld/%lld", i + 1, trans, fsize);
            }
            fprintf (stdout, "\n");
        }
        closesocket(client);
    }
end:
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
