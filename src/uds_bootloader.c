
#include "fota_config.h"

static FotaState fota;

static void UDS_SendRaw(Uint16 d0, Uint16 d1, Uint16 d2, Uint16 d3, Uint16 d4, Uint16 d5, Uint16 d6, Uint16 d7) {
CanFrame tx;
tx.id = UDS_TX_ID;
tx.dlc = 8;
tx.data[0]=d0; tx.data[1]=d1; tx.data[2]=d2; tx.data[3]=d3;
tx.data[4]=d4; tx.data[5]=d5; tx.data[6]=d6; tx.data[7]=d7;
CAN_Send(&tx);
}


static void handle_session(const CanFrame *req) {
fota.active = 1;
UDS_SendRaw(0x02, UDS_SID_SESSION | UDS_POS_RESP, 0x02, 0, 0, 0, 0, 0);
}

static void handle_erase(const CanFrame *req) {
fota.erase_pending = 1;
}

static void handle_download(const CanFrame *req) {
fota.total_size = ((uint32_t)req->data[3] << 16) | ((uint32_t)req->data[4] << 8) | req->data[5];
fota.flash_ptr = BACKUP_START_ADDRESS;
fota.bytes_written = 0;
UDS_SendRaw(0x05, UDS_SID_REQ_DOWNLOAD | UDS_POS_RESP, 0x00, 0, 0, 0, 0, 0);
}

static void handle_transfer_exit(const CanFrame *req) {
fota.expected_crc = ((uint32_t)req->data[2] << 24) | ((uint32_t)req->data[3] << 16) | ((uint32_t)req->data[4] << 8) | req->data[5];
fota.verify_pending = 1;
}

static void handle_reset(const CanFrame *req) {
UDS_SendRaw(0x02, UDS_SID_ECU_RESET | UDS_POS_RESP, 0x01, 0, 0, 0, 0, 0);
fota.reset_pending = 1;
}

typedef void (*UdsHandler)(const CanFrame *req);

typedef struct {
Uint16 sid;
UdsHandler handler;
} UdsService;

static const UdsService uds_table[] = {
{ UDS_SID_SESSION,       handle_session  },
{ UDS_SID_ROUTINE,       handle_erase    },
{ UDS_SID_REQ_DOWNLOAD,  handle_download },
{ UDS_SID_TRANSFER_EXIT, handle_transfer_exit },
{ UDS_SID_ECU_RESET,     handle_reset    }
};

Uint16 Fota_IsActive(void) { return fota.active; }


void UDS_HandleFrame(const CanFrame *rx) {
Uint16 pci;
int idx;

pci = rx->data[0] & 0xF0;
if (pci == 0x00) {
    Uint16 sid = rx->data[1];
    for(idx = 0; idx < 5; idx++) {
        if(uds_table[idx].sid == sid) { uds_table[idx].handler(rx); return; }
    }
}
else if (pci == 0x10) {
    Uint16 sid = rx->data[2];
    if (sid == UDS_SID_TRANSFER) {
        fota.expected_chunk = rx->data[1];
        fota.rx_buffer[0] = rx->data[3]; fota.rx_buffer[1] = rx->data[4];
        fota.rx_buffer[2] = rx->data[5]; fota.rx_buffer[3] = rx->data[6];
        fota.rx_buffer[4] = rx->data[7];
        fota.rx_index = 5;
        UDS_SendRaw(0x30, 0x00, 0x05, 0, 0, 0, 0, 0);
    }
}
else if (pci == 0x20) {
    for(idx = 1; idx < rx->dlc; idx++) {
        if(fota.rx_index < 264) fota.rx_buffer[fota.rx_index++] = rx->data[idx];
    }
    if (fota.rx_index >= fota.expected_chunk) {
        fota.flash_write_pending = 1;
    }
}


}


void UDS_ProcessPendingTasks(void) {
    if (fota.erase_pending) {
        DINT; // Disable interrupts so CAN doesn't crash the CPU
        Flash_EraseBackup();
        EINT; // Re-enable interrupts

        fota.erase_pending = 0;
        UDS_SendRaw(0x04, UDS_SID_ROUTINE | UDS_POS_RESP, 0x01, 0, 0, 0, 0, 0);
    }

    if (fota.flash_write_pending) {
        Uint16 is_final = ((fota.bytes_written + fota.expected_chunk) >= fota.total_size) ? 1 : 0;

        DINT; // Disable interrupts
        Flash_Program(fota.flash_ptr, fota.rx_buffer, fota.expected_chunk, is_final);
        EINT; // Re-enable interrupts

        fota.flash_ptr += (fota.expected_chunk + 1) / 2;
        fota.bytes_written += fota.expected_chunk;
        fota.flash_write_pending = 0;

        UDS_SendRaw(0x01, UDS_SID_TRANSFER | UDS_POS_RESP, 0, 0, 0, 0, 0, 0);
    }

    if (fota.verify_pending) {
        Uint32 calc_crc = Calculate_Checksum(BACKUP_START_ADDRESS, fota.total_size);
        if (calc_crc == fota.expected_crc) {
            UDS_SendRaw(0x01, UDS_SID_TRANSFER_EXIT | UDS_POS_RESP, 0, 0, 0, 0, 0, 0);
            fota.copy_pending = 1;
        } else {
            UDS_SendRaw(0x03, 0x7F, (Uint16)(calc_crc >> 24), (Uint16)(calc_crc >> 16), (Uint16)(calc_crc >> 8), (Uint16)(calc_crc & 0xFF), 0, 0);
        }
        fota.verify_pending = 0;
    }

    if (fota.copy_pending) {
        DELAY_US(10000);
        DINT; // Disable interrupts
        Flash_CopyBackupToActive(fota.total_size);
        EINT; // Re-enable interrupts
        fota.copy_pending = 0;

        // Trigger reset
        EALLOW; WdRegs.WDCR.all = 0x0000; EDIS;
    }

    if (fota.reset_pending) {
        DELAY_US(10000);
        EALLOW; WdRegs.WDCR.all = 0x0000; EDIS;
    }
}
