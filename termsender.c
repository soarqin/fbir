#include "termsender.h"

#include "sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void* connect_(const char* addr, uint16_t port) {
    struct hostent* he;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in rec_addr;
    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(port);
    if((he = gethostbyname(addr)) != NULL) {
        memcpy(&rec_addr.sin_addr, he->h_addr_list[0], he->h_length);
    } else
        rec_addr.sin_addr.s_addr = inet_addr(addr);
    if (connect(fd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)))
        return NULL;
    return (void*)(uintptr_t)fd;
}

void close_(void* sender) {
    int fd = (int)(uintptr_t)sender;
#ifdef WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

int receive_(void* sender, char* buffer, int length) {
    int fd = (int)(uintptr_t)sender;
    int r = recv(fd, buffer, length, 0);
    if(r < 0) {
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSATRY_AGAIN) return 0;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
        return -1;
    }
    return r;
}

int send_(void* sender, const char* buffer, int length) {
    int fd = (int)(uintptr_t)sender;
    int r = send(fd, buffer, length, 0);
    if(r < 0) {
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSATRY_AGAIN) return 0;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
        return -1;
    }
    return r;
}

void progress_(size_t curr, size_t total) {
    fprintf(stdout, "\r  Progress:%10lu/%lu", curr, total);
}

void event_(sender_event_t event, const char* sparam, int iparam) {
    switch(event) {
        case sender_connfail:
            fprintf(stderr, "Unable to connect to %s:%d.\n", sparam, iparam);
            break;
        case sender_connlost:
            fprintf(stderr, "Connection closed by remote.\n");
            break;
        case sender_filemissing:
            fprintf(stderr, "File %s does not exist.\n", sparam);
            break;
        case sender_sending:
            fprintf(stdout, "Sending %s...\n", sparam);
            break;
        case sender_sent:
            fprintf(stdout, "\nSuccessfully sent %s.\n", sparam);
            break;
        default:
            break;
    }
}

void termsender_register() {
    static sender_t s = {
        connect_,
        close_,
        receive_,
        send_,
        progress_,
        event_
    };
    register_sender(&s);
}
