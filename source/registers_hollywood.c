#include <uniicorn.h>
#include <string.h>
#include <registers_hollywood.h>

#define HW_printf(x) printf("[HW] " x "\n")
#define HW_printfv(x, ...) printf("[HW] " x "\n", __VA_ARGS__)

uint32_t HW_SRNPROT_State = 0;
uint32_t HW_OTP_Data = 0xffffffff;

uint32_t HW_State_GPIO_En = 0x00000000;
uint32_t HW_State_GPIO_Dir = 0x00000000;
uint32_t HW_State_GPIO = 0x00000000;

uint32_t HW_State_Timer = 0x00000000;

Starlet_OTP HW_CurrentOTP;
Starlet_SEEPROM HW_CurrentSEEPROM;
bool HW_OTPImported = false;
bool HW_SEEPROMImported = false;

bool HW_HasKeys() {
    return HW_OTPImported && HW_SEEPROMImported;
}

void HW_IncrementTimer() {
    HW_State_Timer++;
}

void HW_ImportKeys(Starlet_OTP *otp, Starlet_SEEPROM *seeprom) {
    if (otp != NULL && !HW_OTPImported) {
        memcpy(&HW_CurrentOTP, otp, sizeof(Starlet_OTP));
        HW_printfv("Imported OTP! Console ID: %08x", SWAP_32(HW_CurrentOTP.parsed.ng_id));
        HW_OTPImported = true;
    }
    if (seeprom != NULL && !HW_SEEPROMImported) {
        memcpy(&HW_CurrentSEEPROM, seeprom, sizeof(Starlet_SEEPROM));
        HW_printfv("Imported SEEPROM! NG Key ID: %08x", SWAP_32(HW_CurrentSEEPROM.ng_key_id));
        HW_SEEPROMImported = true;
    }
}

// TODO: properly implement all of these, this is a big hack just to get boot1 to boot
uint32_t HW_Register_Cache[0x100] = { 0 };

uint64_t HW_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    if (offset == HW_SRNPROT)
        return SWAP_32(HW_SRNPROT_State);
    if (offset == HW_OTPDATA)
        return HW_OTP_Data; // already swapped from the memcpy... i hate little endian
    if (offset == HW_GPIO_ENABLE)
        return SWAP_32(HW_State_GPIO_En);
    if (offset == HW_GPIO_DIR)
        return SWAP_32(HW_State_GPIO_Dir);
    if (offset == HW_GPIO_OUT)
        return SWAP_32(HW_State_GPIO & HW_State_GPIO_Dir);
    if (offset == HW_GPIO_IN)
        return SWAP_32(HW_State_GPIO & ~HW_State_GPIO_Dir);
    if (offset == HW_TIMER)
        return SWAP_32(HW_State_Timer);
    if (offset == HW_VERSION)
        return SWAP_32(0x00000010); // ES2.0 ?
    HW_printfv("Unknown register read: 0x%llx", offset);
    return SWAP_32(HW_Register_Cache[offset / 4]);
}

void HW_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    // byteswap the value, for some reason we have to
    uint32_t value32 = SWAP_32(value);

    if (offset == HW_SRNPROT) {
        HW_printfv("SRNPROT = %x", value32);
        HW_SRNPROT_State = value32;
        return;
    } else if (offset == HW_OTPCMD) {
        if (!GET_BIT(value32, 31)) {
            HW_printf("OTP comamnd issued that was not read.");
            return;
        }
        int otpword = value32 & 0x1f;
        memcpy(&HW_OTP_Data, &(HW_CurrentOTP.words[otpword]), 0x4);
        HW_printfv("Reading word 0x%x from OTP (0x%08x)", otpword, SWAP_32(HW_OTP_Data));
        return;
    } else if (offset == HW_GPIO_ENABLE) {
        HW_State_GPIO_En = value32;
        return;
    } else if (offset == HW_GPIO_DIR) {
        HW_State_GPIO_Dir = value32;
        return;
    } else if (offset == HW_GPIO_OUT) {
        uint32_t masked = (value32 & HW_State_GPIO_En) & HW_State_GPIO_Dir;
        if ((masked & GPIO_DEBUGMASK) != (HW_State_GPIO & GPIO_DEBUGMASK)) {
            unsigned char newDebug = ((masked & GPIO_DEBUGMASK) >> 16) & 0xFF;
            HW_printfv("GPIO DEBUG: %02x (%c)", newDebug, newDebug);
        }
        // this is bad, it doesn't take into account disabled and input pins
        HW_State_GPIO = masked;
        return;
    } else if (offset == HW_SPARE0) {
        // TODO: figure out WTF this is doing, for now just set the values in HW_BOOT0 that boot1 is happy with
        if (GET_BIT(value32, 16)) {
            HW_Register_Cache[HW_BOOT0 / 4] = 0;
        } else {
            HW_Register_Cache[HW_BOOT0 / 4] = 9;
        }
    }
    HW_Register_Cache[offset / 4] = value32;
    HW_printfv("Register write: 0x%llx = 0x%08x", offset, value32);
}