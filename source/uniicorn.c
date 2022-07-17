#include <uniicorn.h>

void hexdump(unsigned char* chars, int size) {
    for (int i = 0; i < size; i++)
        printf("%02x", chars[i]);
    printf("\n");
}

int main(int argc, char **argv, char **envp)
{
  MEM_MakeAllocations();
  return ARM_Main();
}