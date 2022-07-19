#include <uniicorn.h>
#include <string.h>

// memory allocations on the host
void * BOOT0_Buffer;
void * SRAM_Buffer;
void * MEM1_Buffer;
void * MEM2_Buffer;

#define MEM_printfv(x, ...) printf("[MEM] " x "\n", __VA_ARGS__)

// current state of SRAM mirroring
int MEM_ARM_SRAMState = 0;

// convert a physical address of the emulator to a host address
// TODO: find out if external devices have access to mirrored/boot0 memory space
// TODO: add checking to abide by the currently set SRNPROT rules
void * MEM_EmuToHost(uint64_t emu, MEM_Source src) {
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

// sets up the mirrored SRAM + boot0 mappings on the Starlet
// referenced at 19-Jul-2022 https://wiibrew.org/wiki/Starlet/Main_Memory
void MEM_ARM_SetSRAM(bool iouen, bool boot0) {
    uc_mem_unmap(ARM_unicorn, WEIRD1_BASE, WEIRD_SIZE);
    uc_mem_unmap(ARM_unicorn, WEIRD2_BASE, WEIRD_SIZE);
    uc_mem_unmap(ARM_unicorn, SRAM_BASE, SRAM_SIZE);
    uc_mem_unmap(ARM_unicorn, SRAM_MIRROR, SRAM_SIZE);
    // try unmapping the weirder ones
    uc_mem_unmap(ARM_unicorn, SRAM_BASE, SRAM_B_SIZE);
    uc_mem_unmap(ARM_unicorn, SRAM_MIRROR, SRAM_B_SIZE);
    uc_mem_unmap(ARM_unicorn, SRAM_BASE + SRAM_A_SIZE, SRAM_B_SIZE);
    uc_mem_unmap(ARM_unicorn, SRAM_MIRROR + SRAM_A_SIZE, SRAM_B_SIZE);
    MEM_printfv("Updating mappings (boot0: %i, iouen: %i)", boot0, iouen);
    if (boot0) { 
        if (!iouen) { // bootup state
            // the SRAM 0d40/fff0 state is normal
            uc_mem_map_ptr(ARM_unicorn, SRAM_BASE, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
            uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
            // fffe is mapped to the start of SRAM ("SRAM A")
            uc_mem_map_ptr(ARM_unicorn, WEIRD1_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer);
            // ffff is mapped to boot0
            uc_mem_map_ptr(ARM_unicorn, WEIRD2_BASE, WEIRD_SIZE, UC_PROT_READ | UC_PROT_EXEC, BOOT0_Buffer);
            MEM_ARM_SRAMState = 0;
        } else {
            // 0d40/fff0 is mapped to boot0????
            uc_mem_map_ptr(ARM_unicorn, SRAM_BASE, SRAM_SIZE, UC_PROT_READ | UC_PROT_EXEC, BOOT0_Buffer);
            uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR, SRAM_SIZE, UC_PROT_READ | UC_PROT_EXEC, BOOT0_Buffer);
            // fffe is mapped to boot0
            uc_mem_map_ptr(ARM_unicorn, WEIRD1_BASE, WEIRD_SIZE, UC_PROT_READ | UC_PROT_EXEC, BOOT0_Buffer);
            // ffff is mapped to the start of SRAM ("SRAM A")
            uc_mem_map_ptr(ARM_unicorn, WEIRD2_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer);
            MEM_ARM_SRAMState = 1;
        }
    } else {
        if (!iouen) {
            // the SRAM 0d40/fff0 state is normal
            uc_mem_map_ptr(ARM_unicorn, SRAM_BASE, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
            uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
            // fffe is mapped to the entire SRAM directly
            uc_mem_map_ptr(ARM_unicorn, WEIRD1_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer);
            uc_mem_map_ptr(ARM_unicorn, WEIRD2_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer + WEIRD_SIZE);
            MEM_ARM_SRAMState = 2;
        } else {
            // what the hell
            // 0d40/fff0 is mapped to the end of SRAM ("SRAM B")
            uc_mem_map_ptr(ARM_unicorn, SRAM_BASE, SRAM_B_SIZE, UC_PROT_ALL, SRAM_Buffer + SRAM_A_SIZE);
            uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR, SRAM_B_SIZE, UC_PROT_ALL, SRAM_Buffer + SRAM_A_SIZE);
            // then 0d40/0d41 is mapped to the start?
            uc_mem_map_ptr(ARM_unicorn, SRAM_BASE + SRAM_A_SIZE, SRAM_B_SIZE, UC_PROT_ALL, SRAM_Buffer);
            uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR + SRAM_A_SIZE, SRAM_B_SIZE, UC_PROT_ALL, SRAM_Buffer);
            // fffe is mapped to the end of SRAM
            uc_mem_map_ptr(ARM_unicorn, WEIRD1_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer + SRAM_A_SIZE);
            // ffff is mapped to the start of SRAM ("SRAM A")
            uc_mem_map_ptr(ARM_unicorn, WEIRD2_BASE, WEIRD_SIZE, UC_PROT_ALL, SRAM_Buffer);
            MEM_ARM_SRAMState = 4;
        }
    }
}

// set the current boot0 pointer to where-ever the file is loaded
void MEM_SetBoot0(void *boot0_ptr) {
    memcpy(BOOT0_Buffer, boot0_ptr, BOOT0_SIZE);
}

// allocate the SRAM/MEM1/MEM2 buffers
void MEM_MakeAllocations() {
    BOOT0_Buffer = malloc(BOOT0_SIZE);
    SRAM_Buffer = malloc(SRAM_SIZE);
    MEM1_Buffer = malloc(MEM1_SIZE);
    MEM2_Buffer = malloc(MEM2_SIZE);
}