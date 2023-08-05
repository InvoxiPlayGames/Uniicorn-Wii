#ifndef REGISTERS_EXI_H_
#define REGISTERS_EXI_H_

// referenced at 3-Aug-2022 https://wiibrew.org/wiki/Hardware/External_Interface

#define EXI_CSR         0x00
#define EXI_MAR         0x04
#define EXI_LENGTH      0x08
#define EXI_CR          0x0C
#define EXI_DATA        0x10
#define EXI_BOOT_VECTOR 0x40

#define EXI_CHANNEL_COUNT 3
#define EXI_CHANNEL_SIZE 0x14

#endif
