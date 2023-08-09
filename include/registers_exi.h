#ifndef REGISTERS_EXI_H_
#define REGISTERS_EXI_H_

// referenced at 3-Aug-2022 https://wiibrew.org/wiki/Hardware/External_Interface

#define EXI_PARAM       0x00
#define EXI_DMA_ADDR    0x04
#define EXI_DMA_LEN     0x08
#define EXI_CONTROL     0x0C
#define EXI_IMM_DATA    0x10

#define EXI_BOOT_VECTOR     0x40
#define EXI_BOOT_VECTOR_END 0x80

#define EXI_CHANNEL_COUNT 3
#define EXI_CHANNEL_SIZE  0x14

#endif
