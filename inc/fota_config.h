
#ifndef FOTA_CONFIG_H
#define FOTA_CONFIG_H

#include "F28x_Project.h"
#include <string.h>

// Memory map
#define APP_START_ADDRESS    0x084000 // Sector C
#define BACKUP_START_ADDRESS 0x08C000 // Sector F (Backup Bank)


#define CAN_STD_ID(id)   (((uint32_t)(id)) << 18)
#define UDS_RX_ID        0x7E0u // ESP32 - TI
#define UDS_TX_ID        0x7E8u // TI - ESP32

// --- UDS SERVICE IDs (SIDs) ---
#define UDS_SID_SESSION       0x10u
#define UDS_SID_ECU_RESET     0x11u
#define UDS_SID_ROUTINE       0x31u
#define UDS_SID_REQ_DOWNLOAD  0x34u
#define UDS_SID_TRANSFER      0x36u
#define UDS_SID_TRANSFER_EXIT 0x37u
#define UDS_POS_RESP          0x40u


// 1. A clean object to hold a CAN message
typedef struct {
Uint16 id;
Uint16 dlc;
Uint16 data[8];
} CanFrame;

// 2. A single object to hold the entire Bootloader State
typedef struct {
Uint16 active;
Uint32 total_size;
Uint32 flash_ptr;
Uint32 bytes_written;
Uint16 rx_buffer[264];
Uint16 rx_index;
Uint16 expected_chunk;
Uint32 expected_crc;

// Flags for the main loop to process
Uint16 erase_pending;
Uint16 flash_write_pending;
Uint16 verify_pending;
Uint16 copy_pending;
Uint16 reset_pending;


} FotaState;


// Hardware Init
void InitSysClock_200MHz_INTOSC(void);
void DisableDog(void);
void jump_to_application(void);

// CAN Driver
void CAN_Init(void);
void CAN_Send(const CanFrame *f);
Uint16 CAN_Receive(CanFrame *f);

// UDS Protocol Layer
void UDS_HandleFrame(const CanFrame *rx);
void UDS_ProcessPendingTasks(void);
Uint16 Fota_IsActive(void);

// Flash API
void Flash_EraseBackup(void);
void Flash_Program(Uint32 address, Uint16 *data, Uint16 length_in_bytes, Uint16 is_final);
void Flash_CopyBackupToActive(Uint32 length_in_bytes);
Uint32 Calculate_Checksum(Uint32 start_addr, Uint32 length_in_bytes);

#endif // FOTA_CONFIG_H
