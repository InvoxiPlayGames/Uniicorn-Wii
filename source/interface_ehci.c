#include <uniicorn.h>

#define EHCI_printf(x) printf("[EHCI] " x "\n")
#define EHCI_printfv(x, ...) printf("[EHCI] " x "\n", __VA_ARGS__)

// TODO: properly emulate a memory flush
uint32_t EHCI_Register_Cache[0x40];

uint64_t EHCI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data) {
    EHCI_printfv("Tried to read register 0x%llx", offset);
    return 0;
}
void EHCI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data) {
    EHCI_printfv("Tried to write register 0x%llx = 0x%08llx", offset, SWAP_32(value));
    //EHCI_Register_Cache[offset / 4] = SWAP_32(value);
    return;
}