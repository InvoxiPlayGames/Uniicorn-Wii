#include <uniicorn.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <registers_exi.h>

#define EXI_printf(x) printf("[EXI] " x "\n")
#define EXI_printfv(x, ...) printf("[EXI] " x "\n", __VA_ARGS__)

uint8_t EXI_PPCBootVector[0x40] = { 0 };

uint64_t EXI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    EXI_printfv("Tried to read register 0x%llx", offset);
    return 0;
}
void EXI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    uint32_t realval = SWAP_32(value);
    EXI_printfv("Unimplemented register write 0x%llx = 0x%08llx", offset, SWAP_32(value));
}
