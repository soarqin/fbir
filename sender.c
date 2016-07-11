#include "sender.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>

static sender_t* g_sender = NULL;
#define BUF_SIZE (256 * 1024)

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


void register_sender(sender_t* sender) {
    g_sender = sender;
}

#ifdef OLD_PROTOCOL
void sender_send_files(const char* addr, uint16_t port, int count, const char* files[]) {
    int i;
    char* buffer;
    buffer = (char*)malloc(BUF_SIZE);
    for (i = 0; i < count; ++i) {
        void* data = g_sender->connect(addr, port);
        if (data == NULL) {
            g_sender->event(sender_connfail, addr, port);
            return;
        }
        const char* filename = files[i];
        FILE* f = fopen(filename, "rb");
        if (f == NULL) {
            g_sender->event(sender_filemissing, filename, 0);
            return;
        }
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint64_t ss = htonll(size);
        g_sender->send(data, (const char*)&ss, 8);
        g_sender->event(sender_sending, filename, 0);
        size_t total = 0;
        while (!feof(f)) {
            size_t r = fread(buffer, 1, BUF_SIZE, f);
            g_sender->send(data, buffer, r);
            total += r;
            g_sender->progress(total, size);
        }
        fclose(f);
        g_sender->event(sender_sent, filename, 0);
        g_sender->close(data);
    }
    free(buffer);
}
#else
void sender_send_files(const char* addr, uint16_t port, int count, const char* files[]) {
    int i;
    char* buffer;
    void* data = g_sender->connect(addr, port);
    if (data == NULL) {
        g_sender->event(sender_connfail, addr, port);
        return;
    }
    int cc = htonl(count);
    g_sender->send(data, (const char*)&cc, 4);
    buffer = (char*)malloc(BUF_SIZE);
    for (i = 0; i < count; ++i) {
        const char* filename = files[i];
        FILE* f = fopen(filename, "rb");
        uint8_t token;
        if (f == NULL) {
            g_sender->event(sender_filemissing, filename, 0);
            return;
        }
        if (g_sender->receive(data, (char*)&token, 1) < 1 || token == 0) {
            g_sender->event(sender_connlost, NULL, 0);
            free(buffer);
            g_sender->close(data);
            return;
        }
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint64_t ss = htonll(size);
        g_sender->send(data, (const char*)&ss, 8);
        g_sender->event(sender_sending, filename, 0);
        size_t total = 0;
        while (!feof(f)) {
            size_t r = fread(buffer, 1, BUF_SIZE, f);
            g_sender->send(data, buffer, r);
            total += r;
            g_sender->progress(total, size);
        }
        fclose(f);
        g_sender->event(sender_sent, filename, 0);
    }
    free(buffer);
    g_sender->close(data);
}
#endif
