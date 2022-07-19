#include <uniicorn.h>
#include <string.h>
#include <aes.h>

#define AES_CTRL 0x00
#define AES_SRC  0x04
#define AES_DEST 0x08
#define AES_KEY  0x0c
#define AES_IV   0x10

#define AES_BLOCKSIZE 0x10
#define AES_KEYSIZE   0x10

#define AES_printf(x) printf("[AES] " x "\n")
#define AES_printfv(x, ...) printf("[AES] " x "\n", __VA_ARGS__)

unsigned char AES_key_buffer[AES_KEYSIZE] = { 0 };
unsigned char AES_iv_buffer[AES_KEYSIZE] = { 0 };

int AES_key_fifo_state = 0;
int AES_iv_fifo_state = 0;

bool AES_enable_encryption = true;
bool AES_should_decrypt = false;

uint32_t AES_src_address = 0;
uint32_t AES_dest_address = 0;

struct AES_ctx AES_context;

uint64_t AES_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    if (offset == AES_CTRL) {
        uint32_t response = 0;
        if (AES_enable_encryption)
            SET_BIT(response, 28);
        if (AES_should_decrypt)
            SET_BIT(response, 27);
        AES_printfv("Read status register. 0x%08x", response);
        return SWAP_32(response);
    } else if (offset == AES_SRC) {
        AES_printfv("Read source address. 0x%08x", AES_src_address);
        return AES_src_address;
    } else if (offset == AES_DEST) {
        AES_printfv("Read destination address. 0x%08x", AES_dest_address);
        return AES_dest_address;
    } else {
        AES_printfv("Tried to read register 0x%llx", offset);
        return 0;
    }
}

void AES_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    uint32_t realval = SWAP_32(value);
    if (offset == AES_CTRL) {
        AES_enable_encryption = GET_BIT(realval, 28);
        AES_should_decrypt = GET_BIT(realval, 27);
        if (GET_BIT(realval, 31) == 0) {
            AES_printf("Resetting engine.");
            memset(AES_key_buffer, 0, AES_KEYSIZE);
            memset(AES_iv_buffer, 0, AES_KEYSIZE);
            memset(&AES_context, 0, sizeof(AES_context));
            AES_key_fifo_state = 0;
            AES_iv_fifo_state = 0;
        } else {
            int blockcount = (realval & 0xFFF) + 1;
            uint32_t aessize = blockcount * AES_BLOCKSIZE;
            bool should_chain = GET_BIT(realval, 12);
            if (AES_should_decrypt)
                AES_printfv("Crypt %i 0x%x bytes (%i blocks), chain %i, from 0x%08x", AES_should_decrypt, aessize, blockcount, should_chain, AES_src_address);
            if (!should_chain)
                AES_init_ctx_iv(&AES_context, AES_key_buffer, AES_iv_buffer);
            // TODO: make sure the memory is mapped valid.
            //       figure out what real hardware does if it isn't.
            void *src = MEM_EmuToHost(AES_src_address, MEM_SOURCE_AES);
            void *dest = MEM_EmuToHost(AES_dest_address, MEM_SOURCE_AES);
            void *decbuffer = malloc(aessize);
            memcpy(decbuffer, src, aessize);
            if (AES_should_decrypt && AES_enable_encryption) {
                AES_CBC_decrypt_buffer(&AES_context, decbuffer, aessize);
            } else if (AES_enable_encryption) {
                AES_CBC_encrypt_buffer(&AES_context, decbuffer, aessize);
            }
            memcpy(dest, decbuffer, aessize);
            free(decbuffer);
            AES_src_address += aessize;
            AES_dest_address += aessize;
        }
    } else if (offset == AES_KEY) {
        // for memcpy's sake its actually beneficial to use the provided value
        memcpy(AES_key_buffer + (AES_key_fifo_state * 4), &value, 4);
        AES_key_fifo_state++;
        if (AES_key_fifo_state == 4) {
            printf("[AES] Key set: ");
            for (int i = 0; i < AES_KEYSIZE; i++) {
                printf("%02x", AES_key_buffer[i]);
            }
            printf("\n");
            AES_key_fifo_state = 0;
        }
    } else if (offset == AES_IV) {
        // for memcpy's sake its actually beneficial to use the provided value
        memcpy(AES_iv_buffer + (AES_iv_fifo_state * 4), &value, 4);
        AES_iv_fifo_state++;
        if (AES_iv_fifo_state == 4) {
            printf("[AES] IV set: ");
            for (int i = 0; i < AES_KEYSIZE; i++) {
                printf("%02x", AES_iv_buffer[i]);
            }
            printf("\n");
            AES_iv_fifo_state = 0;
        }
    } else if (offset == AES_SRC) {
        AES_src_address = realval;
        AES_printfv("Source set: 0x%08x", AES_src_address);
    } else if (offset == AES_DEST) {
        AES_dest_address = realval;
        AES_printfv("Destination set: 0x%08x", AES_dest_address);
    } else {
        AES_printfv("Unimplemented register write 0x%llx = 0x%08llx", offset, SWAP_32(value));
    }
}