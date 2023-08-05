#include <uniicorn.h>
#include <string.h>

#define ARM_PSR_MODE_MASK 0b11111

#define ARM_PSR_MODE_USER 0b10000
#define ARM_PSR_MODE_IRQ 0b10010
#define ARM_PSR_MODE_SVC 0b10011
#define ARM_PSR_MODE_ABORT 0b10111
#define ARM_PSR_MODE_SYSTEM 0b11111

uc_engine *ARM_unicorn;

#define ARM_printf(x) printf("[ARM] " x "\n")
#define ARM_printfv(x, ...) printf("[ARM] " x "\n", __VA_ARGS__)

char ARM_boot0_file[0x1000];

uint64_t ARM_WTFRead(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    //ARM_printfv("[AHB?] Tried to read register 0x%llx", offset);
    return 0;
}
void ARM_WTFWrite(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    //ARM_printfv("[AHB?] Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    return;
}

void ARM_memory_invalid(uc_engine *uc, int access, uint64_t addr, unsigned size, uint64_t value, void *user_data) {
    ARM_printfv("Invalid memory access! (0x%08llx = 0x%08llx)", addr, value);
}

bool ARM_TLB_hook(uc_engine *uc, uint64_t vaddr, uc_mem_type type, uc_tlb_entry *result, void *user_data) {
    ARM_printfv("tlb hook %08llx %i", vaddr, type);
    result->paddr = vaddr;
    result->perms = UC_PROT_ALL;
    return true;
}

void ARM_code_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    HW_IncrementTimer();
    // monitor what part of the OS we are at
    if (address == 0xffff0000) {
        ARM_printf("boot0/boot2/IOS (re-)started");
        // HACK - disable enabling the MMU on boot2v4
        /*static uint8_t expect_bytes[8] = { 0xe3, 0x80, 0x00, 0x05, 0xee, 0x01, 0x0f, 0x10 };
        static uint8_t read_bytes[sizeof(expect_bytes)] = {0};
        uc_mem_read(uc, 0xffff2398, read_bytes, sizeof(expect_bytes));
        if (memcmp(expect_bytes, read_bytes, sizeof(expect_bytes)) == 0) {
            uint8_t all_zero[8] = {0}; // equivalent to a nop
            uc_mem_write(uc, 0xffff2398, all_zero, sizeof(all_zero));
            ARM_printf("BOOT2 HACK: PATCHED 0xffff2398 TO DISABLE MMU/DCACHE");
        }*/
    }
    if (address == 0xfff00000)
        ARM_printf("boot1 started");
    if (address == 0x00028710)
        ARM_printf("boot2loader started");
}

void ARM_print_state() {
    // TODO: migrate to uc_reg_read_batch
    unsigned int temp = 0;
    ARM_printf("Register state:");
    for (int i = UC_ARM_REG_R0; i <= UC_ARM_REG_R12; i++) {
        uc_reg_read(ARM_unicorn, i, &temp);
        ARM_printfv("    R%i = 0x%08x", i - UC_ARM_REG_R0, temp);
    }
    uc_reg_read(ARM_unicorn, UC_ARM_REG_PC, &temp);
    ARM_printfv("    PC = 0x%08x", temp);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_LR, &temp);
    ARM_printfv("    LR = 0x%08x", temp);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_SP, &temp);
    ARM_printfv("    SP = 0x%08x", temp);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_SPSR, &temp);
    ARM_printfv("    SPSR = 0x%08x", temp);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_CPSR, &temp);
    ARM_printfv("    CPSR = 0x%08x", temp);
}

char debug_out_buffer[0x1000] = { 0 };

void ARM_intr_hook(uc_engine *uc, uint32_t intno, void *user_data) {
    uint32_t pc = 0;
    uc_reg_read(uc, UC_ARM_REG_PC, &pc);

    // do our special handling
    if (intno == 2) { // SWI
        int r0 = 0;
        uc_reg_read(uc, UC_ARM_REG_R0, &r0);
        if (r0 == 4) { // IOS uses this for debugger output
            uint32_t r1 = 0;
            uc_reg_read(uc, UC_ARM_REG_R1, &r1);
            // fetch from memory subsystem
            char unicorn_fetched_string[0x80] = {0}; //for below hack
            char *string = MEM_EmuToHost(r1, MEM_SOURCE_STARLET);
            if (string == NULL) {
                // memory subsystem doesn't know? HACK: fetch directly from unicorn
                uc_mem_read(uc, r1, unicorn_fetched_string, sizeof(unicorn_fetched_string));
                string = unicorn_fetched_string;
            }
            snprintf(debug_out_buffer, sizeof(debug_out_buffer), "%s%s", debug_out_buffer, string);
            // if we have a newline, print the buffer out
            if (debug_out_buffer[strlen(debug_out_buffer) - 1] == '\n') {
                debug_out_buffer[strlen(debug_out_buffer) - 1] = 0x00; // strip newline (we have our own)
                ARM_printfv("Debug output: %s", debug_out_buffer);
                // flush out the debug buffer
                memset(debug_out_buffer, 0, sizeof(debug_out_buffer));
            }
        } else {
            ARM_printfv("Unknown SWI %i", r0);
            ARM_print_state();
        }
    }
    if (intno == 3) {
        MEM_ARM_SetSRAM(false, false, true);
        //uc_emu_stop(uc);
    }

    // TODO: is this correct?
    // handle the interrupt as the CPU intended
    // set the correct mode bits in the PSR
    /*uint32_t spsr = 0;
    uint32_t cpsr = 0;
    uc_reg_read(uc, UC_ARM_REG_CPSR, &spsr);
    if (intno == 3 || intno == 4) { // abort
        cpsr = (spsr & ~ARM_PSR_MODE_MASK) | ARM_PSR_MODE_ABORT;
    } else { // anything else? i think?
        cpsr = (spsr & ~ARM_PSR_MODE_MASK) | ARM_PSR_MODE_SVC;
    }
    uc_reg_write(uc, UC_ARM_REG_SPSR, &spsr);
    uc_reg_write(uc, UC_ARM_REG_CPSR, &cpsr);
    // back up the next PC into the LR
    pc += 4;
    uc_reg_write(uc, UC_ARM_REG_LR, &pc);
    // set the new PC to the interrupt vector
    uint32_t int_vec = 0xFFFF0000 + (intno * 4);
    uc_reg_write(uc, UC_ARM_REG_PC, &int_vec);
    ARM_printfv("Handled interrupt 0x%x at PC %08x to vector %08x", intno, pc -4, int_vec);
    return;*/
}

int ARM_Main() {
    uc_err err;

    // read boot0 from the file
    ARM_printf("Loading boot0");
    FILE *b0 = fopen("files/boot0.bin", "r");
    if (b0 == NULL) {
        ARM_printf("Failed to open boot0!");
        return -1;
    }
    fread(ARM_boot0_file, sizeof(char), sizeof(ARM_boot0_file), b0);
    fclose(b0);
    // skip hash check
    //ARM_boot0_file[0x4cb] = 0;

    // attempt to load OTP from otp.bin
    FILE *otp = fopen("files/otp.bin", "r");
    if (otp != NULL) {
        Starlet_OTP otpstruct;
        fread(&otpstruct, sizeof(Starlet_OTP), 1, otp);
        HW_ImportKeys(&otpstruct, NULL);
        fclose(otp);
    }
    // attempt to load SEEPROM from seeprom.bin
    FILE *seeprom = fopen("files/seeprom.bin", "r");
    if (seeprom != NULL) {
        Starlet_SEEPROM romstruct;
        fread(&romstruct, sizeof(Starlet_SEEPROM), 1, seeprom);
        HW_ImportKeys(NULL, &romstruct);
        fclose(seeprom);
    }
    // attempt to load keys from keys.bin
    FILE *keys = fopen("files/keys.bin", "r");
    if (keys != NULL) {
        BootMii_Keys keyfile;
        fread(&keyfile, sizeof(BootMii_Keys), 1, keys);
        HW_ImportKeys(&keyfile.otp, &keyfile.seeprom);
        fclose(keys);
    }

    // create the unicorn emulator object
    //ARM_printf("Creating Unicorn emulator");
    err = uc_open(UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_BIG_ENDIAN, &ARM_unicorn);
    if (err != UC_ERR_OK) {
        ARM_printfv("Creating emulator failed: %s (%u)", uc_strerror(err), err);
        return -1;
    }

    // attempt to set the CPU mode
    // TODO: figure out why UC_CPU_ARM_926 fails
    err = uc_ctl_set_cpu_model(ARM_unicorn, UC_CPU_ARM_926);
    if (err != UC_ERR_OK) {
        ARM_printfv("Setting CPU model failed: %s (%u)", uc_strerror(err), err);
        return -1;
    }

    // attempting to set the TLB mode to virtual
    // possibly will give us more control over IOS memory in the future?
    // but really, just a lil HACK to try to get boot2 to boot further
    uc_hook tlb;
    uc_ctl_tlb_mode(ARM_unicorn, UC_TLB_VIRTUAL);
    uc_hook_add(ARM_unicorn, &tlb, UC_HOOK_TLB_FILL, ARM_TLB_hook, NULL, 0xffffffff, 0x0);

    // set boot0 in the memory mapping
    MEM_SetBoot0(ARM_boot0_file);
    // set the SRAM state, iouen off, boot0 on, mmu off
    MEM_ARM_SetSRAM(false, true, false);

    // map MEM1/MEM2 into memory
    // TOOD: only map memory when boot1 properly initialises it in the memory interface
    uc_mem_map_ptr(ARM_unicorn, MEM1_BASE, MEM1_SIZE, UC_PROT_ALL, MEM1_Buffer);
    uc_mem_map_ptr(ARM_unicorn, MEM2_BASE, MEM2_SIZE, UC_PROT_ALL, MEM2_Buffer);

    // trap invalid memory accesses so we know what tries to get written to where
    uc_hook mem;
    uc_hook_add(ARM_unicorn, &mem, UC_HOOK_MEM_UNMAPPED, ARM_memory_invalid, NULL, 0xffffffff, 0x0);
    // apply a code hook so we can trace the program counter properly (and increase HW_CLOCK)
    uc_hook code;
    uc_hook_add(ARM_unicorn, &code, UC_HOOK_CODE, ARM_code_hook, NULL, 0xffffffff, 0x0);
    // apply hooks for the interrupts
    uc_hook intr;
    uc_hook_add(ARM_unicorn, &intr, UC_HOOK_INTR, ARM_intr_hook, NULL, 0xffffffff, 0x0);

    // map hollywood registers, both with the AHB bit applied and without
    uc_mmio_map(ARM_unicorn, REG_HW | REG_AHBMASK, ALIGN_SIZE(REG_HW_SZ), HW_ReadRegister, NULL, HW_WriteRegister, NULL);
    uc_mmio_map(ARM_unicorn, REG_HW, ALIGN_SIZE(REG_HW_SZ), HW_ReadRegister, NULL, HW_WriteRegister, NULL);

    // map AES registers
    uc_mmio_map(ARM_unicorn, REG_AES, ALIGN_SIZE(REG_AES_SZ), AES_ReadRegister, NULL, AES_WriteRegister, NULL);
    // map SHA registers
    uc_mmio_map(ARM_unicorn, REG_SHA, ALIGN_SIZE(REG_SHA_SZ), SHA_ReadRegister, NULL, SHA_WriteRegister, NULL);
    // map NAND registers
    uc_mmio_map(ARM_unicorn, REG_NAND, ALIGN_SIZE(REG_NAND_SZ), NAND_ReadRegister, NULL, NAND_WriteRegister, NULL);
    // map EHCI registers
    uc_mmio_map(ARM_unicorn, REG_EHCI, ALIGN_SIZE(REG_EHCI_SZ), EHCI_ReadRegister, NULL, EHCI_WriteRegister, NULL);
    // map EXI registers
    uc_mmio_map(ARM_unicorn, REG_EXI, ALIGN_SIZE(REG_EXI_SZ), EXI_ReadRegister, NULL, EXI_WriteRegister, NULL);
    // map WTF / AHB?? registers
    uc_mmio_map(ARM_unicorn, REG_WTF | REG_AHBMASK, ALIGN_SIZE(REG_WTF_SZ), ARM_WTFRead, NULL, ARM_WTFWrite, NULL);
    // map memory registers - unicorn requires alignment here, so handle MEMI and MEMC in the same register space
    uc_mmio_map(ARM_unicorn, REG_MEMI | REG_AHBMASK, ALIGN_SIZE(REG_MEMI_SZ), MEMI_ReadRegister, NULL, MEMI_WriteRegister, NULL);

    // start the emulator
    ARM_printf("Starting emulation");
    err = uc_emu_start(ARM_unicorn, BOOT0_ENTRY, 0xFFFFFFFF /* panic() = BOOT0_ENTRY + 0xdc */, 0, 0);
    if (err != UC_ERR_OK) {
        ARM_printfv("Error: %u (%s)", err, uc_strerror(err));
    }
    ARM_printf("Emulation stopped.");

    // print out the state of the registers
    ARM_print_state();

    // dump the contents of SRAM
    FILE *sramdump = fopen("sram.bin", "w+");
    fwrite(SRAM_Buffer, 1, SRAM_SIZE, sramdump);
    fclose(sramdump);
    // dump the contents of MEM1
    FILE *mem1dump = fopen("mem1.bin", "w+");
    fwrite(MEM1_Buffer, 1, MEM1_SIZE, mem1dump);
    fclose(mem1dump);
    
    return 0;
}
