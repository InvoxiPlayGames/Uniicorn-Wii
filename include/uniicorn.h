#include <unicorn/unicorn.h>
#include "memory.h"
#include "registers.h"
#include "cpu.h"
#include "interfaces.h"
#include "nand.h"
#include "otp.h"
#include "seeprom.h"

// common macros
#define SWAP_32(i) ((i >> 24) & 0xff) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i << 24) & 0xff000000)
#define SWAP_16(i) ((i << 8) & 0xff | (i >> 8))

#define BIT(i) (1 << i)
#define GET_BIT(num, i) ((num & (1 << i)) > 0)
#define SET_BIT(num, i) num |= 1 << i
#define CLEAR_BIT(num, i) num &= ~(1 << i)

#define ALIGN_SIZE(i) (i % 0x400 != 0) ? i + (0x400 - (i % 0x400)) : i 

void hexdump(unsigned char* chars, int size);