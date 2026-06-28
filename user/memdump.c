#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void
memdump(char *fmt, char *data)
{
  // Your code here.
  for (int i = 0;fmt[i] != '\0'; i++)
  {    /* code */
    char type = fmt[i]; //接收模式

    if(type == 'c'){
      char val = *(char*)data; //打印一个字节
      printf("%c ", val);
      data++;
    }
    else if (type == 'h')
    {
      /* code */
      short val = *(short*)data;//打印2个字节
      printf("%d ", val);
      data += 2;
    }
    else if (type == 'i')
    {
      /* code */
      int val = *(int*)data;
      printf("%d ", val);
      data += 4;
    }
    else if (type == 'p')
    {
      /* code */
      long val = *(long*)data; 
      printf("%p ", val); // 或者用 %lx 打印十六进制地址
      data += 8;
    }
    else if (type =='S')
    {
      /* code */
      printf("%s ", data);
      data += (strlen(data) + 1);
    }
    else if (type == 's')
    {
      /* code */
      char *s = *(char**)data;
      printf("%s", s);
      data += 8;
    }
    printf("\n");
  }
}
