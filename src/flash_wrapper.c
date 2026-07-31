
#include "fota_config.h"
#include "F021_F2837xD_C28x.h"

#pragma CODE_SECTION(Flash_EraseBackup, ".TI.ramfunc");
#pragma CODE_SECTION(Flash_Program, ".TI.ramfunc");
#pragma CODE_SECTION(Flash_CopyBackupToActive, ".TI.ramfunc");


void Flash_EraseBackup(void) {
    Fapi_StatusType status; // 1. Declare the variable at the top!

    EALLOW;
    Flash0CtrlRegs.FPAC1.bit.PMPPWR = 1;

    // Erase Backup Banks (Sector G & H )
    // 2. Assign the return value of the function to 'status'
    status = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (Uint32 *)BACKUP_START_ADDRESS);
    while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);

    if (status != Fapi_Status_Success) {
        ESTOP0; // Software breakpoint for the debugger
        while(1);
    }

    // 3. Do the same for the second sector
    status = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (Uint32 *)0x08E000);
    while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);

    if (status != Fapi_Status_Success) {
        ESTOP0;
        while(1);
    }
    EDIS;
}


void Flash_Program(Uint32 address, Uint16 *data, Uint16 length_in_bytes, Uint16 is_final) {
    Uint16 buffer16[130]; // Expanded to prevent off-by-one overflow
    Uint16 words;
    Uint16 idx;

    words = length_in_bytes / 2;
    for (idx = 0; idx < words; idx++) {
        buffer16[idx] = (data[2*idx]) | (data[2*idx + 1] << 8);
    }


    if (is_final && (length_in_bytes & 1)) {
        buffer16[words] = data[2*words] | 0xFF00;
        words++;
    }

    EALLOW;
    Flash0CtrlRegs.FPAC1.bit.PMPPWR = 1;

    for (idx = 0; idx < words; idx += 4) {
        Uint16 words_to_program = (words - idx >= 4) ? 4 : (words - idx);
        Fapi_issueProgrammingCommand((Uint32 *)(address + idx),
                                     &buffer16[idx],
                                     words_to_program,
                                     0,
                                     0,
                                     Fapi_AutoEccGeneration);
        while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);
    }
    EDIS;
}


void Flash_CopyBackupToActive(Uint32 length_in_bytes) {
    Uint16 *src_ptr;
    Uint32 dest_addr;
    Uint32 words;
    Uint16 temp_buf[4];
    Uint32 idx;
    Uint16 k;

    src_ptr = (Uint16 *)BACKUP_START_ADDRESS;
    dest_addr = APP_START_ADDRESS;
    words = (length_in_bytes + 1) / 2;

    EALLOW;
    Flash0CtrlRegs.FPAC1.bit.PMPPWR = 1;

    // Erase Active Banks (Sector C & D)
    Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (Uint32 *)APP_START_ADDRESS);
    while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);

    Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (Uint32 *)0x086000);
    while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);

    for (idx = 0; idx < words; idx += 4) {
        Uint16 words_to_program = (words - idx >= 4) ? 4 : (words - idx);
        for(k = 0; k < words_to_program; k++) {
            temp_buf[k] = src_ptr[idx+k];
        }
        Fapi_issueProgrammingCommand((Uint32 *)dest_addr,
                                     temp_buf,
                                     words_to_program,
                                     0,
                                     0,
                                     Fapi_AutoEccGeneration);
        while(Fapi_checkFsmForReady() != Fapi_Status_FsmReady);
        dest_addr += words_to_program;
    }
    EDIS;
}


Uint32 Calculate_Checksum(Uint32 start_addr, Uint32 length_in_bytes) {
    Uint32 crc = 0xFFFFFFFF;
    Uint16 *ptr = (Uint16 *)start_addr;
    Uint32 words = length_in_bytes / 2;
    Uint32 idx;
    int j;

    for (idx = 0; idx < words; idx++) {
        Uint16 word = ptr[idx];

        // Process lower byte
        crc ^= (word & 0xFF);
        for (j = 0; j < 8; j++) crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);

        // Process upper byte
        crc ^= ((word >> 8) & 0xFF);
        for (j = 0; j < 8; j++) crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
    }

    if (length_in_bytes & 1) {
        Uint16 word = ptr[words];
        crc ^= (word & 0xFF);
        for (j = 0; j < 8; j++) crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
    }

    return crc ^ 0xFFFFFFFF;
}
