#include <uniicorn.h>
#include <string.h>
#include <sha1.h>

#define SHA_CTRL 0x00
#define SHA_SRC  0x04
#define SHA_H0   0x08
#define SHA_H1   0x0c
#define SHA_H2   0x10
#define SHA_H3   0x14
#define SHA_H4   0x18

#define SHA_BLOCKSIZE 0x40

#define SHA_printf(x) printf("[SHA] " x "\n")
#define SHA_printfv(x, ...) printf("[SHA] " x "\n", __VA_ARGS__)

uint32_t SHA_src_address = 0;

SHA1_CTX SHA1_context;

uint64_t SHA_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    if (offset == SHA_CTRL) {
        SHA_printf("Read status register.");
        return 0;
    } else if (offset == SHA_SRC) {
        SHA_printfv("Read source address. 0x%08x", SHA_src_address);
        return SHA_src_address;
    } else if (offset >= SHA_H0 && offset <= SHA_H4) {
        int state = (offset - SHA_H0) / 0x4;
        SHA_printfv("Read SHA state %i: 0x%08x", state, SHA1_context.state[state]);
        return SWAP_32(SHA1_context.state[state]);
    } else {
        SHA_printfv("Tried to read register 0x%llx", offset);
        return 0;
    }
}

void SHA_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    uint32_t realval = SWAP_32(value);
    if (offset == SHA_CTRL) {
        if (GET_BIT(realval, 31) == 0) {
            SHA_printf("Resetting engine.");
            memset(&SHA1_context, 0, sizeof(SHA1_CTX));
        } else {
            int blockcount = (realval & 0x1FF) + 1;
            uint32_t shasize = blockcount * SHA_BLOCKSIZE;
            SHA_printfv("Hashing 0x%x bytes (%i blocks) from 0x%08x", shasize, blockcount, SHA_src_address);
            // TODO: make sure the memory is mapped valid.
            //       figure out what real hardware does if it isn't.
            void *src = MEM_EmuToHost(SHA_src_address, MEM_SOURCE_SHA);
            // the sha1 library has a tendency to mangle the data
            // so make a buffer to hold it
            void *tempbuf = malloc(shasize);
            memcpy(tempbuf, src, shasize);
            SHA1_Update(&SHA1_context, tempbuf, shasize);
            free(tempbuf);
            SHA_src_address += shasize;
        }
    } else if (offset == SHA_SRC) {
        SHA_src_address = realval;
        SHA_printfv("Source set: 0x%08x", SHA_src_address);
    } else if (offset >= SHA_H0 && offset <= SHA_H4) {
        int state = (offset - SHA_H0) / 0x4;
        SHA_printfv("Set SHA state %i: 0x%08x", state, realval);
        SHA1_context.state[state] = realval;
    } else {
        SHA_printfv("Unimplemented register write 0x%llx = 0x%08llx", offset, SWAP_32(value));
    }
}