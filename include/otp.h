#ifndef OTP_H_
#define OTP_H_

// referenced at 16-Jul-2022 https://wiibrew.org/wiki/Hardware/OTP

typedef struct _Starlet_OTP {
    union {
        struct {
            unsigned char boot1_hash[0x14]; // 0x0-0x4
            unsigned char common_key[0x10]; // 0x5-0x8
            unsigned int ng_id;             // 0x9
            union {
                struct {
                    unsigned char ng_key[0x20];  // 0xA-0x11
                    unsigned char padding[0x10]; // 0x12-0x15
                } ng_key;
                struct {
                    unsigned char padding[0x1C];   // 0xA-0x10
                    unsigned char nand_hmac[0x14]; // 0x11-0x15
                } nand_hmac;
            };                            // 0xA-0x15
            unsigned char nand_key[0x10]; // 0x16-0x19
            unsigned char rng_seed[0x10]; // 0x1A-0x1D
            unsigned char unknown[0x8];   // 0x1E-0x1F
        } parsed;
        unsigned int words[0x20];
    };
} Starlet_OTP;

#endif // OTP_H_