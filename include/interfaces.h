#ifndef INTERFACES_H_
#define INTERFACES_H_

#include <unicorn/unicorn.h>
#include <uniicorn.h>

uint64_t HW_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void HW_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);
void HW_ImportKeys(Starlet_OTP *otp, Starlet_SEEPROM *seeprom);
bool HW_HasKeys();
void HW_IncrementTimer();

uint64_t AES_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void AES_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t NAND_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void NAND_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t SHA_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void SHA_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t MEMI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void MEMI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t MEMC_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void MEMC_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t EHCI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void EHCI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

uint64_t EXI_ReadRegister(uc_engine *uc, uint64_t offset, unsigned size, void *user_data);
void EXI_WriteRegister(uc_engine *uc, uint64_t offset, unsigned size, uint64_t value, void *user_data);

#endif // INTERFACES_H_