#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Usage: sixfive <filename>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        fprintf(2, "sixfive: cannot open %s\n", argv[1]);
        exit(1);
    }

    char c;
    char buf[32];
    int idx = 0;
    char *seps = " -\r\t\n./,";

    // 初始化缓冲区
    memset(buf, 0, sizeof(buf));

    while (read(fd, &c, 1) > 0) {
        // 如果是数字，存入缓冲区
        if (c >= '0' && c <= '9') {
            if (idx < 31) {
                buf[idx++] = c;
            }
        } 
        // 如果是分隔符，结算当前数字序列
        else if (strchr(seps, c) != 0) {
            if (idx > 0) {
                buf[idx] = '\0';
                int val = atoi(buf);
                if (val % 5 == 0 || val % 6 == 0) {
                    printf("%d\n", val);
                }
                // 重置缓冲区和索引
                idx = 0;
                memset(buf, 0, sizeof(buf));
            }
        }
        // 如果是其他字符，直接忽略，不影响数字解析
    }

    // 处理文件末尾可能存在的最后一个数字
    if (idx > 0) {
        buf[idx] = '\0';
        int val = atoi(buf);
        if (val % 5 == 0 || val % 6 == 0) {
            printf("%d\n", val);
        }
    }

    close(fd);
    exit(0);
}