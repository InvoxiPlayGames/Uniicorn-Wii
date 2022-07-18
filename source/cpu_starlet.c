#include <uniicorn.h>

uc_engine *ARM_unicorn;

#define ARM_printf(x) printf("[ARM] " x "\n")
#define ARM_printfv(x, ...) printf("[ARM] " x "\n", __VA_ARGS__)

char ARM_boot0_file[0x1000];

uint64_t ARM_WTFRead(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    ARM_printfv("[AHB?] Tried to read register 0x%llx", offset);
    return 0;
}
void ARM_WTFWrite(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    ARM_printfv("[AHB?] Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    return;
}

void ARM_memory_invalid(uc_engine *uc, int access, uint64_t addr, unsigned size, uint64_t value, void *user_data) {
    ARM_printfv("Invalid memory access! (0x%08llx = 0x%08llx)", addr, value);
}

void ARM_code_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
    HW_IncrementTimer();
    // monitor what part of the OS we are at
    if (address == 0xffff0000)
        ARM_printf("boot0 started");
    if (address == 0xfff00000)
        ARM_printf("boot1 started");
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
    // always skip over applying ecc correction
    //ARM_boot0_file[0x384] = 0xEA;

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
        ARM_printfv("failed %u (%s)", err, uc_strerror(err));
        return -1;
    }

    // attempt to set the CPU mode
    // TODO: figure out why this fails
    /*printf("setting CPU model\n");
    err = uc_ctl_set_cpu_model(uc, UC_CPU_ARM_926);
    if (err != UC_ERR_OK) {
        printf("failed %u (%s)\n", err, uc_strerror(err));
        return -1;
    }*/

    // map boot0 into memory
    uc_mem_map_ptr(ARM_unicorn, BOOT0_BASE, sizeof(ARM_boot0_file), UC_PROT_ALL, (void*)ARM_boot0_file);
    // map SRAM memory
    uc_mem_map_ptr(ARM_unicorn, SRAM_BASE, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
    uc_mem_map_ptr(ARM_unicorn, SRAM_MIRROR, SRAM_SIZE, UC_PROT_ALL, SRAM_Buffer);
    // TODO: properly map SRAM and BOOT0 according to HW_SRNPROT and HW_BOOT0
    //       likely required for booting into boot2 and IOS

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

    // map hollywood registers, both with the AHB bit applied and without
    uc_mmio_map(ARM_unicorn, REG_HW | REG_AHBMASK, ALIGN_SIZE(REG_HW_SZ), HW_ReadRegister, NULL, HW_WriteRegister, NULL);
    uc_mmio_map(ARM_unicorn, REG_HW, ALIGN_SIZE(REG_HW_SZ), HW_ReadRegister, NULL, HW_WriteRegister, NULL);

    // map AES registers
    uc_mmio_map(ARM_unicorn, REG_AES, ALIGN_SIZE(REG_AES_SZ), AES_ReadRegister, NULL, AES_WriteRegister, NULL);
    // map SHA registers
    uc_mmio_map(ARM_unicorn, REG_SHA, ALIGN_SIZE(REG_SHA_SZ), SHA_ReadRegister, NULL, SHA_WriteRegister, NULL);
    // map NAND registers
    uc_mmio_map(ARM_unicorn, REG_NAND, ALIGN_SIZE(REG_NAND_SZ), NAND_ReadRegister, NULL, NAND_WriteRegister, NULL);
    // map WTF / AHB?? registers
    uc_mmio_map(ARM_unicorn, REG_WTF | REG_AHBMASK, ALIGN_SIZE(REG_WTF_SZ), ARM_WTFRead, NULL, ARM_WTFWrite, NULL);
    // map memory registers - unicorn requires alignment here, so handle MEMI and MEMC in the same register space
    uc_mmio_map(ARM_unicorn, REG_MEMI | REG_AHBMASK, ALIGN_SIZE(REG_MEMI_SZ), MEMI_ReadRegister, NULL, MEMI_WriteRegister, NULL);

    // start the emulator
    ARM_printf("Starting emulation");
    err = uc_emu_start(ARM_unicorn, BOOT0_BASE, BOOT0_BASE + 0xdc /* panic() */, 0, 0);
    if (err != UC_ERR_OK) {
        ARM_printfv("Error: %u (%s)", err, uc_strerror(err));
    }
    ARM_printf("Emulation stopped.");

    // print out a few useful registers
    int pc = 0;
    int sp = 0;
    int lr = 0;
    uc_reg_read(ARM_unicorn, UC_ARM_REG_PC, &pc);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_SP, &sp);
    uc_reg_read(ARM_unicorn, UC_ARM_REG_LR, &lr);
    ARM_printfv("Final state - pc:0x%08x lr:0x%08x sp:0x%08x. SRAM dumped.", pc, lr, sp);

    // dump the contents of SRAM
    FILE *sramdump = fopen("sram.bin", "w+");
    fwrite(SRAM_Buffer, 1, SRAM_SIZE, sramdump);
    fclose(sramdump);
    
    return 0;
}