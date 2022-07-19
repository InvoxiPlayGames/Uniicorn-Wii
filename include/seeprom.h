#ifndef SEEPROM_H_
#define SEEPROM_H_

// referenced at 17-Jul-2022 https://wiibrew.org/wiki/Hardware/SEEPROM

typedef struct _Starlet_SEEPROM {
    union {
        struct {
            unsigned int ms_id;       // 0x00
            unsigned int ca_id;       // 0x04
            unsigned int ng_key_id;   // 0x08
            unsigned char ng_sig[60]; // 0x0C
            struct { // 0x48
                union {
                    struct {
                        unsigned char boot2version;
                        unsigned char unknown1;
                        unsigned char unknown2;
                        unsigned char pad;
                        unsigned int update_tag;
                    } __attribute__((packed));
                    unsigned short data[4];
                };
                unsigned short checksum; // sum of data[] elements?
            } boot2_counters[2];
            struct {
                union {
                    unsigned int nand_gen; // matches offset 0x8 in nand SFFS blocks
                    unsigned short data[2];
                } __attribute__((packed));
                unsigned short checksum;  // sum of data[] elements?
            } nand_counters[3];           // current slot rotates on each write
            unsigned char pad0[6];        // 0x6E
            unsigned char korean_key[16]; // 0x74
            unsigned char pad1[116];      // 0x84
            unsigned short prng_seed[2];  // 0xF8 unsigned int with lo word stored first, incremented every time IOS starts.
                                          // Used with the PRNG key to setup IOS's PRNG (syscalls 73/74 etc.)
            unsigned char pad2[4];        // 0xFC
        } parsed;
        unsigned short words[0x80];
    };
} Starlet_SEEPROM;

#endif // SEEPROM_H_