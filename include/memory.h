#ifndef MEMORY_H_
#define MEMORY_H_

// memory allocation sizes
// 96KB(split?) SRAM, 24MB MEM1, 64MB MEM2
#define SRAM_A_SIZE 64 * 1024
#define SRAM_B_SIZE 32 * 1024
#define SRAM_SIZE   SRAM_A_SIZE + SRAM_B_SIZE
#define MEM1_SIZE   24 * 1024 * 1024
#define MEM2_SIZE   64 * 1024 * 1024
#define WEIRD_SIZE  64 * 1024
#define BOOT0_SIZE  4 * 1024
// memory addresses
#define SRAM_BASE   0x0d400000
#define SRAM_MIRROR 0xfff00000
#define WEIRD1_BASE 0xfffe0000
#define WEIRD2_BASE 0xffff0000
#define MEM1_BASE   0x00000000
#define MEM2_BASE   0x10000000
#define MEM_VRTMASK 0x80000000
#define BOOT0_ENTRY 0xffff0000

// BOOT0 buffer
extern void * BOOT0_Buffer;
// IOS SRAM buffer
extern void * SRAM_Buffer;
// MEM1 buffer
extern void * MEM1_Buffer;
// MEM2 buffer
extern void * MEM2_Buffer;

typedef enum _MEM_Source {
    MEM_SOURCE_STARLET,
    MEM_SOURCE_BROADWAY,
    MEM_SOURCE_NAND,
    MEM_SOURCE_AES,
    MEM_SOURCE_SHA
} MEM_Source;

void MEM_MakeAllocations();
void MEM_ARM_SetSRAM(bool iouen, bool boot0, bool mmu);
void MEM_SetBoot0(void *boot0_ptr);
void * MEM_EmuToHost(uint64_t emu, MEM_Source src);
void MEM_FreeAllocations();
bool MEM_AllocationsValid();
int MEM_ARM_SRAMState;

#endif // MEMORY_H_