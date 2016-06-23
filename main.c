#include "sender.h"

#include "termsender.h"
#include "sender.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: fbir <ip> <files...>\n");
        return 0;
    }
    termsender_register();
    sender_send_files(argv[1], 5000, argc - 2, (const char**)argv + 2);
    return 0;
}
