#include <uniicorn.h>
#include <stdio.h>
#include <string.h>
#include <registers_nand.h>

#define NAND_CTRL    0x00
#define NAND_CONFIG  0x04
#define NAND_ADDR1   0x08
#define NAND_ADDR2   0x0c
#define NAND_DATABUF 0x10
#define NAND_ECCBUF  0x14
#define NAND_UNK     0x18

#define NAND_printf(x) printf("[NAND] " x "\n")
#define NAND_printfv(x, ...) printf("[NAND] " x "\n", __VA_ARGS__)

FILE *nand;

uint32_t NAND_dest_address = 0;
uint32_t NAND_ecc_address = 0;

uint32_t NAND_read_page = 0;
uint32_t NAND_read_offset = 0;

// chip id of HY27UF084G2M
static unsigned char NAND_ChipID[4] = { 0xad, 0xdc, 0x80, 0x95 };

uint32_t NAND_config = 0;

static uint8_t NAND_parity(uint8_t x) {
	uint8_t y = 0;

	while (x) {
		y ^= (x & 1);
		x >>= 1;
	}

	return y;
}

void NAND_calc_ecc(uint8_t *data, uint8_t *ecc) {
	uint8_t a[12][2];
	int i, j;
	uint32_t a0, a1;
	uint8_t x;

	memset(a, 0, sizeof a);
	for (i = 0; i < 512; i++) {
		x = data[i];
		for (j = 0; j < 9; j++)
			a[3+j][(i >> j) & 1] ^= x;
	}

	x = a[3][0] ^ a[3][1];
	a[0][0] = x & 0x55;
	a[0][1] = x & 0xaa;
	a[1][0] = x & 0x33;
	a[1][1] = x & 0xcc;
	a[2][0] = x & 0x0f;
	a[2][1] = x & 0xf0;

	for (j = 0; j < 12; j++) {
		a[j][0] = NAND_parity(a[j][0]);
		a[j][1] = NAND_parity(a[j][1]);
	}

	a0 = a1 = 0;
	for (j = 0; j < 12; j++) {
		a0 |= a[j][0] << j;
		a1 |= a[j][1] << j;
	}

	ecc[0] = a0;
	ecc[1] = a0 >> 8;
	ecc[2] = a1;
	ecc[3] = a1 >> 8;
}

uint64_t NAND_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    if (offset == NAND_CONFIG) {
        uint32_t cfg = NAND_config;
        if (nand != NULL) // if the nand is mounted, set enable bit
            SET_BIT(cfg, 27);
        return SWAP_32(cfg);
    } else if (offset == NAND_DATABUF) {
        return SWAP_32(NAND_dest_address);
    } else if (offset == NAND_ECCBUF) {
        return SWAP_32(NAND_ecc_address);
    } else if (offset == NAND_ADDR1) {
        return SWAP_32(NAND_read_offset);
    } else if (offset == NAND_ADDR2) {
        return SWAP_32(NAND_read_page);
    } else if (offset == NAND_CTRL) {
        return 0; // TODO: this
    }
    NAND_printfv("Tried to read register 0x%llx", offset);
    return 0;
}
void NAND_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    uint32_t realval = SWAP_32(value);
    if (offset == NAND_CONFIG) {
        bool enableNAND = GET_BIT(realval, 27);
        if (enableNAND && nand == NULL) {
            nand = fopen("files/nand.bin", "r+");
            if (nand == NULL) {
                NAND_printf("Failed to open NAND, stopping emulator");
                uc_emu_stop(uc);
                return;
            }
            // getting file size
            fseek(nand, 0, SEEK_END);
            int filesize = ftell(nand);
            rewind(nand);
            NAND_printfv("Successfully loaded NAND. (0x%x bytes)", filesize);
            // if we don't have keys already, and we're a bootmii nand
            // import the otp/seeprom data into the hollywood registers
            if (!HW_HasKeys() && filesize == sizeof(BootMii_NAND)) {
                BootMii_Keys keys;
                fseek(nand, sizeof(NAND_Data), SEEK_SET);
                int readbytes = fread(&keys, sizeof(BootMii_Keys), 1, nand);
                rewind(nand);
                HW_ImportKeys(&keys.otp, &keys.seeprom);
            }
        } else if (nand != NULL) {
            NAND_printf("Closing NAND");
            fclose(nand);
            nand = NULL;
        }
        NAND_config = realval;
        CLEAR_BIT(NAND_config, 27); // don't save enable bit
        return;
    } else if (offset == NAND_CTRL) {
        if (!GET_BIT(realval, 31)) {
            NAND_printf("Reset NAND interface");
            NAND_dest_address = 0;
            NAND_ecc_address = 0;
            NAND_read_offset = 0;
            NAND_read_page = 0;
            NAND_config = 0;
            return;
        }
        unsigned char command = (realval >> 16) & 0xFF; 
        int datalen = realval & 0xFFF;
        if (datalen == 0x840 && command == CMD_READ_CYCLE2) {
            NAND_printfv("Reading 0x%x from page 0x%x (data:0x%08x ecc:0x%08x)", datalen, NAND_read_page, NAND_dest_address, NAND_ecc_address);
            int offset = NAND_read_page * sizeof(NAND_Page);
            NAND_Page tempread;
            fseek(nand, offset, SEEK_SET);
            int readbytes = fread(&tempread, 1, sizeof(NAND_Page), nand);
            // TODO: make sure the memory is mapped valid.
            //       figure out what real hardware does if it isn't.
            void *data_real = MEM_EmuToHost(NAND_dest_address, MEM_SOURCE_NAND);
            memcpy(data_real, tempread.data, sizeof(tempread.data));
            void *ecc_real = MEM_EmuToHost(NAND_ecc_address, MEM_SOURCE_NAND);
            memcpy(ecc_real, tempread.ecc, sizeof(tempread.ecc));
            void *calc_real = MEM_EmuToHost(NAND_ecc_address ^ 0x40, MEM_SOURCE_NAND);
            for (int i = 0; i < 4; i++) {
                NAND_calc_ecc(data_real + (i * 0x200), calc_real + (i * 0x4));
            }
            return;
        }
        NAND_printfv("TODO: command 0x%02x, data length 0x%x (page:0x%x data:0x%08x ecc:0x%08x)", command, datalen, NAND_read_page, NAND_dest_address, NAND_ecc_address);
        return;
    } else if (offset == NAND_ADDR2) {
        NAND_read_page = realval & 0xffffff;
        return;
    } else if (offset == NAND_ADDR1) {
        NAND_read_offset = realval & 0xffff;
        return;
    } else if (offset == NAND_DATABUF) {
        NAND_dest_address = realval;
        return;
    } else if (offset == NAND_ECCBUF) {
        NAND_ecc_address = realval;
        return;
    }
    NAND_printfv("Unimplemented register write 0x%llx = 0x%08llx", offset, SWAP_32(value));
}