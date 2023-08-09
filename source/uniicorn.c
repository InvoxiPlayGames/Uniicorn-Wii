#include <uniicorn.h>
#include <string.h>

void hexdump(unsigned char* chars, int size) {
    for (int i = 0; i < size; i++)
        printf("%02x", chars[i]);
    printf("\n");
}

/*
    Supported command line arguments:
    --boot0-nocheck - patches boot0 to ignore boot1 hash from OTP
    --bmem [path] - sets in-memory armboot for BootMii to load
*/

int main(int argc, char **argv, char **envp)
{
    // allocate memory
    MEM_MakeAllocations();
    if (!MEM_AllocationsValid()) {
        fprintf(stderr, "failed to initialise memory buffers - out of memory?\n");
        return -1;
    }

    // parse command line arguments
    bool patch_boot0_hash = false;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            // bootmii memory load argument
            if (strcmp(argv[i], "--bmem") == 0 && argc > i + 1) {
                char *bmem_file = argv[++i];
                printf("loading %s into BootMii memory load buffer...", bmem_file);
                FILE *fp = fopen(bmem_file, "rb");
                if (fp == NULL) {
                    printf("failed.\n");
                    return -1;
                }
                size_t r = fread(&MEM1_Buffer[0x150F100], 1, 0x10000, fp);
                fclose(fp);
                printf("%zu bytes.\n", r);
                // set the values in MEM1 that BootMii's loader will read from
                *(uint32_t *)(&MEM1_Buffer[0x150F000]) = SWAP_32((uint32_t)0x424D454D);
                *(uint32_t *)(&MEM1_Buffer[0x150F004]) = SWAP_32((uint32_t)0x0150F100);
                i++;
            }
            // boot0 hash check argument
            else if (strcmp(argv[i], "--boot0-nocheck") == 0) patch_boot0_hash = true;
        }
    }

    // load boot0 and OTP keys

    // read boot0 from file
    printf("loading boot0...\n");
    FILE *b0 = fopen("files/boot0.bin", "r");
    if (b0 == NULL) {
        printf("Failed to open boot0!\n");
        return -1;
    }
    fread(BOOT0_Buffer, sizeof(char), BOOT0_SIZE, b0);
    fclose(b0);
    // patches the mov r2, #1 at "otp_hash_not_empty" to mov r2, #0
    // so boot0 thinks the OTP hash was empty and doesn't check against it
    if (patch_boot0_hash) ((char *)BOOT0_Buffer)[0x4cb] = 0;

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

    // run the ARM CPU
    // TODO: Completely redesign how everything exists here
    //       GUI, threads, the works
    int arm_ret = ARM_Main();
    // free memory
    MEM_FreeAllocations();
    return arm_ret;
}