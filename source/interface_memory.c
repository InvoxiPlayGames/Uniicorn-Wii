#include <uniicorn.h>

#define MEMI_printf(x) printf("[MEMI] " x "\n")
#define MEMI_printfv(x, ...) printf("[MEMI] " x "\n", __VA_ARGS__)

#define MEMC_printf(x) printf("[MEMC] " x "\n")
#define MEMC_printfv(x, ...) printf("[MEMC] " x "\n", __VA_ARGS__)

uint64_t MEMI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    MEMI_printfv("Tried to read register 0x%llx", offset);
    return 0;
}
void MEMI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    MEMI_printfv("Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    return;
}

uint64_t MEMC_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    MEMC_printfv("Tried to read register 0x%llx", offset);
    return 0;
}
void MEMC_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    MEMC_printfv("Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    return;
}