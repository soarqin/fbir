#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sender {
    void* (*connect)(const char* addr, uint16_t port);
    void (*close)(void* sender);
    int (*receive)(void* sender, char* buffer, int length);
    int (*send)(void* sender, const char* buffer, int length);
    void (*info)(const char* format, ...);
    void (*error)(const char* format, ...);
} sender_t;

extern uint64_t htonll(uint64_t value);
extern void register_sender(sender_t* sender);
extern void sender_send_files(const char* addr, uint16_t port, int count, const char* files[]);

#ifdef __cplusplus
}
#endif

