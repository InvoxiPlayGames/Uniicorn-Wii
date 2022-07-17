#ifndef NAND_H_
#define NAND_H_

#include "otp.h"
#include "seeprom.h"

// referenced at 17-Jul-2022 https://wiibrew.org/wiki/Bootmii/NAND_dump_format#keys.bin

typedef struct _BootMii_Keys {
    unsigned char header[0x100];
    Starlet_OTP otp;
    unsigned char padding1[0x80];
    Starlet_SEEPROM seeprom;
    unsigned char padding2[0x100];
} BootMii_Keys;

// referenced at 17-Jul-2022 https://wiibrew.org/wiki/Hardware/NAND

typedef struct _NAND_Page {
    unsigned char data[0x800];
    unsigned char ecc[0x40];
} NAND_Page;

typedef struct _NAND_Cluster {
    NAND_Page pages[0x8];
} NAND_Cluster;

typedef struct _NAND_Block {
    union {
        NAND_Cluster clusters[0x8];
        NAND_Page pages[0x40];
    };
} NAND_Block;


typedef struct _NAND_Data {
    union {
        NAND_Block blocks[0x1000];
        NAND_Cluster clusters[0x8000];
        NAND_Page pages[0x40000];
    };
} NAND_Data;

typedef struct _BootMii_NAND {
    NAND_Data data;
    BootMii_Keys keys;
} BootMii_NAND;

#endif // NAND_H_