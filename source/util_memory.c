#include <uniicorn.h>

// memory allocations on the host
void * SRAM_Buffer;
void * MEM1_Buffer;
void * MEM2_Buffer;

#define MEM_printfv(x, ...) printf("[MEM] " x "\n", __VA_ARGS__)

// convert a physical address of the emulator to a host address
void * MEM_EmuToHost(uint64_t emu) {
    if (emu >= SRAM_BASE && emu < SRAM_BASE + SRAM_SIZE)
        return SRAM_Buffer + (emu - SRAM_BASE);
    if (emu >= SRAM_MIRROR && emu < SRAM_MIRROR + SRAM_SIZE)
        return SRAM_Buffer + (emu - SRAM_MIRROR);
    if (emu >= MEM1_BASE && emu < MEM1_BASE + MEM1_SIZE)
        return MEM1_Buffer + (emu - MEM1_BASE);
    if (emu >= MEM2_BASE && emu < MEM2_BASE + MEM2_SIZE)
        return MEM2_Buffer + (emu - MEM2_BASE);
    return NULL;
}

// allocate the SRAM/MEM1/MEM2 buffers
int MEM_MakeAllocations() {
    SRAM_Buffer = malloc(SRAM_SIZE);
    MEM_printfv("Allocated SRAM (0x%x) = %p", SRAM_SIZE, SRAM_Buffer);
    MEM1_Buffer = malloc(MEM1_SIZE);
    MEM_printfv("Allocated MEM1 (0x%x) = %p", MEM1_SIZE, MEM1_Buffer);
    MEM2_Buffer = malloc(MEM2_SIZE);
    MEM_printfv("Allocated MEM2 (0x%x) = %p", MEM2_SIZE, MEM2_Buffer);
    return 0;
}