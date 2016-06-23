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

void sender_send_files(const char* addr, uint16_t port, int count, const char* files[]) {
    int i;
    char* buffer;
    void* data = g_sender->connect(addr, port);
    if (data == NULL) {
        g_sender->error("Unable to connect to %s:%u.\n", addr, port);
        return;
    }
    buffer = (char*)malloc(BUF_SIZE);
    for (i = 0; i < count; ++i) {
        const char* filename = files[i];
        FILE* f = fopen(filename, "rb");
        uint8_t token;
        if (f == NULL) {
            g_sender->error("File %s does not exist.\n", filename);
            return;
        }
        if (g_sender->receive(data, (char*)&token, 1) < 1 || token == 0) {
            g_sender->error("Connection closed by remote.\n");
            free(buffer);
            g_sender->close(data);
            return;
        }
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint64_t ss = htonll(size);
        g_sender->send(data, (const char*)&ss, 8);
        g_sender->info("Sending %s...\n", filename);
        size_t total = 0;
        while (!feof(f)) {
            size_t r = fread(buffer, 1, BUF_SIZE, f);
            g_sender->send(data, buffer, r);
            total += r;
            g_sender->info("\r  Progress:%10u/%u\n", (uint32_t)total, (uint32_t)size);
        }
        fclose(f);
        g_sender->info("\nSuccessfully sent %s.\n", filename);
    }
    free(buffer);
    g_sender->close(data);
}
