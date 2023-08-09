#include <uniicorn.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <registers_exi.h>

#define EXI_printf(x) printf("[EXI] " x "\n")
#define EXI_printfv(x, ...) printf("[EXI] " x "\n", __VA_ARGS__)

uint8_t EXI_PPCBootVector[EXI_BOOT_VECTOR_END - EXI_BOOT_VECTOR] = { 0 };

// emulates a USB Gecko device in Slot B (channel 1, device 0) - clock isn't cared for
static char gecko_out_buffer[0x1000] = { 0 };
static int gecko_out_read = 0;
static bool gecko_enabled = false;
static uint32_t gecko_exi_data_buf = 0;
static uint32_t gecko_last_control = 0;

void EXI_FlushGeckoText() {
    if (gecko_out_read > 0) {
        EXI_printfv("Gecko: %s", gecko_out_buffer);
        gecko_out_read = 0;
        memset(gecko_out_buffer, 0, sizeof(gecko_out_buffer));
    }
}

uint64_t EXI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    // exi boot vector memory space
    if (offset >= EXI_BOOT_VECTOR && offset <= EXI_BOOT_VECTOR_END) {
        int real_offset = offset - EXI_BOOT_VECTOR;
        void *source = EXI_PPCBootVector + real_offset;
        uint32_t read_value = 0; // use the LE one for the memcpy
        memcpy(&read_value, source, sizeof(read_value));
        return read_value;
    }

    int channel = offset / EXI_CHANNEL_SIZE;
    int cmd = offset - (channel * EXI_CHANNEL_SIZE);
    //if (channel < 0 || channel > 2) {
    if (channel != 1) { // we only support channel 1 right now
        EXI_printfv("Invalid EXI channel %i, trying to read reg 0x%llx", channel, offset);
        return 0;
    }
    switch (cmd) {
        case EXI_CONTROL:
            // return control val but with last bit unset (action complete)
            return SWAP_32((uint32_t)(gecko_last_control & ~0x1));
        case EXI_IMM_DATA:
            return SWAP_32(gecko_exi_data_buf);
        default:
            EXI_printfv("Unimplemented EXI read 0x%x on channel %i", cmd, channel);
            return 0;
    }
    return 0;
}

void EXI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    // exi boot vector memory space
    if (offset >= EXI_BOOT_VECTOR && offset <= EXI_BOOT_VECTOR_END) {
        int real_offset = offset - EXI_BOOT_VECTOR;
        void *target = EXI_PPCBootVector + real_offset;
        uint32_t value32 = (uint32_t)value; // use the LE one for the memcpy
        memcpy(target, &value32, sizeof(value32));
    }

    uint32_t realval = SWAP_32(value);
    int channel = offset / EXI_CHANNEL_SIZE;
    int cmd = offset - (channel * EXI_CHANNEL_SIZE);
    //if (channel < 0 || channel > 2) {
    if (channel != 1) { // we only support channel 1 right now
        EXI_printfv("Invalid EXI channel %i, trying to write reg 0x%llx val %08x", channel, offset, realval);
        return;
    }
    switch (cmd) {
        case EXI_PARAM: {
            char is_gecko = (GET_BIT(realval, 7) && !GET_BIT(realval, 8) && !GET_BIT(realval, 9));
            int device_speed = (realval >> 4) & 0b111;
            gecko_enabled = is_gecko && device_speed == 5;
            return;
        }
        case EXI_IMM_DATA: {
            if (gecko_enabled)
                gecko_exi_data_buf = realval;
            else
                EXI_printfv("Tried to set EXI imm val %08x when gecko was deselected", realval);
            return;
        }
        case EXI_CONTROL:
            if (gecko_enabled) {
                gecko_last_control = realval;
                // mask off the highest nibble of the data buffer
                char command = (gecko_exi_data_buf >> 28) & 0xF;
                EXI_printfv("0x%08x 0x%08x 0x%x", gecko_last_control, gecko_exi_data_buf, command);
                if (command == 0xB) { // send 8-bit byte
                    char byte = (gecko_exi_data_buf >> 20) & 0x7F;
                    if (byte == '\n' || gecko_out_read >= sizeof(gecko_out_buffer)) { // once we reach a newline, print it to stdout
                        EXI_FlushGeckoText();
                    } else {
                        gecko_out_buffer[gecko_out_read++] = byte;
                    }
                    // set the bit saying a byte was sent
                    // TODO: verify this with hardware
                    gecko_exi_data_buf = 0x08000000;
                } else if (command == 0xC) { // asking if ready to send byte
                    gecko_exi_data_buf = 0x04000000;
                } else if (command == 0x9) { // identification
                    gecko_exi_data_buf = 0x04700000; 
                } else if (command == 0xA || command == 0xD) { // recvbyte/checkrecv
                    gecko_exi_data_buf = 0x00000000; // not ready
                } else if (command == 0x0) { // no idea, some function does this and expects nothing back
                    gecko_exi_data_buf = 0x00000000;
                } else {
                    EXI_printfv("Unknown Gecko command 0x%x (0x%08x)", command, realval);
                    gecko_exi_data_buf = 0;
                }
            } else
                EXI_printfv("Tried to send EXI control command %08x when gecko was deselected", realval);
            return;
        default:
            EXI_printfv("Unimplemented EXI write 0x%x on channel %i (val %08x)", cmd, channel, realval);
            return;
    }
}
