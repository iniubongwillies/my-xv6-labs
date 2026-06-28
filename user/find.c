#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void find(char *path, char *target) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 只有目录才需要遍历
    if (st.type == T_DIR) {
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            fprintf(2, "find: path too long\n");
            close(fd);
            return;
        }
        
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0) continue;
            
            // 忽略 "." 和 ".." 防止无限递归
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

            // 拼接路径
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if (stat(buf, &st) < 0) {
                fprintf(2, "find: cannot stat %s\n", buf);
                continue;
            }

            // 如果是目录，递归调用 find
            if (st.type == T_DIR) {
                find(buf, target);
            } 
            // 如果是文件，对比文件名
            else if (st.type == T_FILE) {
                if (strcmp(de.name, target) == 0) {
                    printf("%s\n", buf);
                }
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(2, "Usage: find <dir> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}