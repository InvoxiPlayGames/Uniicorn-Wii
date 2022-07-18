#include <uniicorn.h>
#include <registers_memory.h>

#define MEMI_printf(x) printf("[MEMI] " x "\n")
#define MEMI_printfv(x, ...) printf("[MEMI] " x "\n", __VA_ARGS__)

#define MEMC_printf(x) printf("[MEMC] " x "\n")
#define MEMC_printfv(x, ...) printf("[MEMC] " x "\n", __VA_ARGS__)

// TODO: properly emulate a memory flush
uint16_t flushreq = 0;

uint64_t MEMI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    if (offset > (REG_MEMC - REG_MEMI)) {
        // we have to handle the memory controller and the memory interface within the same address space
        offset -= REG_MEMC - REG_MEMI;
        if (offset == MEM_AHMFLUSH_ACK)
            return flushreq;
        MEMC_printfv("Tried to read register 0x%llx", offset);
    } else {
        MEMI_printfv("Tried to read register 0x%llx", offset);
    }
    return 0;
}
void MEMI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    if (offset > (REG_MEMC - REG_MEMI)) {
        // we have to handle the memory controller and the memory interface within the same address space
        offset -= REG_MEMC - REG_MEMI;
        if (offset == MEM_AHMFLUSH)
            flushreq = (uint16_t)value;
        MEMC_printfv("Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    } else {
        MEMI_printfv("Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    }
    return;
}