#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "Usage: xargs command\n");
        exit(1);
    }
    // fprintf(1, "%d %s", argc, argv[0]);
    char *args[MAXARG + 1];
    int ind = 0;
    for(int i=1;i<argc;i++){
        args[ind++] = argv[i];
    }

    char buf[512];
    char *p = buf;
    while(read(0, p, 1) == 1){
        if(*p == '\n'){
            *p = '\0';
            
            if(fork() == 0){
                args[ind] = buf;
                exec(argv[1], args);
                fprintf(2, "exec %s failed\n", argv[1]);
                exit(0);
            }
            wait(0);
            p = buf;
        } else {
            p++;
        }
    }

    exit(0);
}

