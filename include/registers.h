#ifndef REGISTERS_H_
#define REGISTERS_H_

// referenced at 16-Jul-2022 across https://wiibrew.org/wiki/Category:Starlet_Hardware

// masks
#define REG_PPCMASK 0xc0000000
#define REG_AHBMASK 0x00800000
// hollywood registers
#define REG_HW_SZ   0x400
#define REG_HW      0x0d000000
// nand interface
#define REG_NAND_SZ 0x1c
#define REG_NAND    0x0d010000
// aes engine
#define REG_AES_SZ  0x14
#define REG_AES     0x0d020000
// sha engine
#define REG_SHA_SZ  0x1c
#define REG_SHA     0x0d030000
// ehci interface
#define REG_EHCI_SZ 0x100
#define REG_EHCI    0x0d040000
// OHCI0 interface
#define REG_OHC0_SZ 0x200
#define REG_OHC0    0x0d050000
// OHCI1 interface
#define REG_OHC1_SZ 0x200
#define REG_OHC1    0x0d060000
// SD interface
#define REG_SD_SZ   0x200
#define REG_SD      0x0d070000
// exi bus
#define REG_EXI_SZ  0x80
#define REG_EXI     0x0d006800
// audio interface
#define REG_AI_SZ   0x20
#define REG_AI      0x0d006c00
// drive interface
#define REG_DI_SZ   0x40
#define REG_DI      0x0d006000
// graphics
#define REG_GX_SZ   0x100
#define REG_GX      0x0c001000
// video interface
#define REG_VI_SZ   0x100
#define REG_VI      0x0c002000
// idk what this is??? wtf??? it may be AHB related
// touched by boot1
#define REG_WTF_SZ  0x4000
#define REG_WTF     0x0d0b0000
// memory interface
#define REG_MEMI_SZ 0x80
#define REG_MEMI    0x0d0b4000
// memory controller
#define REG_MEMC_SZ 0xcc
#define REG_MEMC    0x0d0b4200

#endif // REGISTERS_H_