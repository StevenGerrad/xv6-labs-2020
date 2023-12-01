#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 没怎么写过这个，不知道怎么改进，参考https://xiaodongfan.com/6-S081-Lab1.html
// XXX: 标记的地方要及时关闭文件描述符，确实是要回忆下，注意是每个进程有个fd表
// TODO：但是要注意的是为什么没有及时关闭会差这么大
void pipeline(int *last){
    // int last[2];
    int prime;

    // read(last[0], &prime, sizeof(prime));
    // if(prime == 0) exit(1);
    int ret;
    // XXX：这个经验很有用，包括对pipe等函数的判断
    if ((ret = read(last[0], &prime, sizeof(int))) != sizeof(int)) {
        exit(0);
    }

    // fprintf(1, "%d:prime %d\n", getpid(), prime);
    fprintf(1, "prime %d\n", prime);
    int item;

    int flow = 0;
    int next[2];
    int status;
    int pid;
    while(read(last[0], &item, sizeof(item))){
        if(item % prime == 0){
            continue;
        }
        // if(getpid() > 35) break;
        if(flow == 0){
            flow = 1;

            // pipe(next);
            if (pipe(next) < 0) {
                fprintf(2, "%d pipe p1 err\n", getpid());
                exit(1);
            }

            pid = fork();
            if(pid == 0){
                close(last[0]);     // XXX
                close(next[1]);     // XXX
                pipeline(next);
                break;  // 这个break似乎没有意义
            } else {
                close(next[0]);     // XXX
            }
        }
        // fprintf(1, "%d:flow %d\n", getpid(), item);
        write(next[1], &item, sizeof(item));
    }
    close(last[0]);
    if(flow){
        close(next[1]);
        wait(&status);
    }
    exit(0);
}


int
main(int argc, char *argv[])
{
    int n = 35;     // n must be larger than 2
    close(0);
    int next[2];
    pipe(next);
    int status;

    
    if(fork() != 0){
        close(next[0]);     // XXX
        for(int i=2;i<n;i++){
            write(next[1], &i, sizeof(i));
        }
        close(next[1]);
        wait(&status);
    } else{
        close(next[1]);     // XXX
        pipeline(next);
    }

    // debug：会不会是由于主进程提前关闭造成的？
    // sleep(10);
    
    
    exit(0);
}