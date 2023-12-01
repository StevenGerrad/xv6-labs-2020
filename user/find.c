#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


int
cmpSuffix(char *path, char *target)
{
    char *p, *q = target;
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;
    while(*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

void ffind(char *dir, char *target){
    int fd;
    if((fd = open(dir, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", dir);
        exit(1);
    }
    
    struct stat st;
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        exit(1);
    }

    if(st.type != T_DIR){
        fprintf(2, "find: %s is not a dir\n", dir);
        exit(1);
    }
    
    
    char buf[512], *p;
    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
        // printf("find: path too long\n");
        fprintf(2, "find: path too long\n");
        exit(1);
    }
    

    struct dirent de;
    strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        
        if(stat(buf, &st) < 0){
            fprintf(1, "find: cannot stat %s\n", buf);
            continue;
        }

        if(cmpSuffix(buf, target) == 0){
            fprintf(1, "%s\n", buf);
        }
        if(st.type == T_DIR && cmpSuffix(buf, ".") != 0 && cmpSuffix(buf, "..") != 0){
            ffind(buf, target);
        }
    }

    close(fd);
}

int
main(int argc, char *argv[])
{
    // 仿照ls.c文件
    if(argc != 3){
        fprintf(2, "usage: find dir filename\n");
        exit(1);
    }
    
    
    char *dir = argv[1];
    char *target = argv[2];

    ffind(dir, target);

    exit(0);
}