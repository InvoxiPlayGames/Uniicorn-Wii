#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <sha1.h>

#define NAND_PAGE_SIZE 0x800
#define NAND_ECC_SIZE 0x40

#define BOOT1_PAGE_SIZE 0x2F

unsigned char page_buffer[NAND_PAGE_SIZE];
unsigned char ecc_buffer[NAND_ECC_SIZE];
unsigned char boot1_buffer[NAND_PAGE_SIZE * BOOT1_PAGE_SIZE];

static unsigned char boot1_key[0x10] = { 0x92,0x58,0xa7,0x52,0x64,0x96,0x0d,0x82,0x67,0x6f,0x90,0x44,0x56,0x88,0x2a,0x73 };
static unsigned char boot1_iv[0x10] = { 0x00 };

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s /path/to/nand.bin\n", argv[0]);
        return -1;
    }
    
    FILE *nand = fopen(argv[1], "r");
    if (nand == NULL) {
        printf("failed to open nand\n");
        return -1;
    }
    
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, boot1_key, boot1_iv);

    SHA1_CTX sha;
    SHA1_Init(&sha);

    for (int i = 0; i < BOOT1_PAGE_SIZE; i++) {
        //printf("reading page 0x%x 0x%x\n", i, (NAND_PAGE_SIZE * i));
        fread(page_buffer, 1, NAND_PAGE_SIZE, nand);
        fread(ecc_buffer, 1, NAND_ECC_SIZE, nand);
        // todo: calculate ECC if needed
        //printf("decrypting page 0x%x\n", i);
        AES_CBC_decrypt_buffer(&ctx, page_buffer, NAND_PAGE_SIZE);
        memcpy(boot1_buffer + (NAND_PAGE_SIZE * i), page_buffer, NAND_PAGE_SIZE);
        SHA1_Update(&sha, page_buffer, NAND_PAGE_SIZE);
    }
    fclose(nand);

    // boot0 does not finalise the hash by adding padding, so we shall not for this hash
    printf("OTP hash: %08x%08x%08x%08x%08x\n", sha.state[0], sha.state[1], sha.state[2], sha.state[3], sha.state[4]);

    printf("saving boot1\n");
    FILE *boot1 = fopen("boot1.bin", "w+");
    fwrite(boot1_buffer, 1, sizeof(boot1_buffer), boot1);
    fclose(boot1);
    return 0;
}