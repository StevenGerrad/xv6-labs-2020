#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p2c[2], c2p[2];
    pipe(p2c);
    pipe(c2p);
    char ping[1], pong[1];

    if(fork() == 0){
        read(p2c[0], ping, sizeof(ping));
        fprintf(1, "%d: received ping\n", getpid());
        write(c2p[1], "i", 1);
    } else {
        write(p2c[1], "i", 1);
        read(c2p[0], pong, sizeof(pong));
        fprintf(1, "%d: received pong\n", getpid());
    }
    exit(0);
}