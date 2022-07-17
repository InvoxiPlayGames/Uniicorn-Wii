#ifndef MEMORY_H_
#define MEMORY_H_

// memory allocation sizes
// 128KB SRAM, 24MB MEM1, 64MB MEM2
#define SRAM_SIZE 128 * 1024
#define MEM1_SIZE 24 * 1024 * 1024
#define MEM2_SIZE 64 * 1024 * 1024
// memory addresses
#define BOOT0_BASE  0xffff0000
#define SRAM_BASE   0x0d400000
#define SRAM_MIRROR 0xfff00000
#define MEM1_BASE   0x00000000
#define MEM2_BASE   0x10000000
#define MEM_VRTMASK 0x80000000

// IOS SRAM buffer
extern void * SRAM_Buffer;
// MEM1 buffer
extern void * MEM1_Buffer;
// MEM2 buffer
extern void * MEM2_Buffer;

int MEM_MakeAllocations();
void * MEM_EmuToHost(uint64_t emu);

#endif // MEMORY_H_