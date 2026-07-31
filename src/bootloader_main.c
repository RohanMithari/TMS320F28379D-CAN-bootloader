
#include "fota_config.h"
#include "F021_F2837xD_C28x.h"
#pragma CODE_SECTION(canb_isr, ".TI.ramfunc");
__interrupt void canb_isr(void);

extern uint16_t RamfuncsLoadStart;
extern uint16_t RamfuncsLoadEnd;
extern uint16_t RamfuncsRunStart;


__interrupt void canb_isr(void) {
Uint16 lastInterruptMailbox;
CanFrame rx;

lastInterruptMailbox = CanbRegs.CAN_INT.bit.INT0ID;
if(lastInterruptMailbox == 2) {
    if(CAN_Receive(&rx)) {
        // ISR delegates UDS logic, eliminating blocking code inside interrupts!
        UDS_HandleFrame(&rx);
    }
}
CanbRegs.CAN_GLB_INT_CLR.bit.INT0_FLG_CLR = 1;
PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;


}



void main(void) {
    uint32_t boot_delay = 0;
    Fapi_StatusType init_status; // Variable to capture flash API status

   //1. it handles the PLL
    InitSysCtrl();
    DisableDog();
    // 2. Initialize basic GPIOs
    InitGpio();

    DINT; // Disable global interrupts during hardware setup

    // 3. Move RAM functions  to RAM

    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);

    // 4. Initialize Flash API and CAPTURE the status
    EALLOW;
    Flash0CtrlRegs.FPAC1.bit.PMPPWR = 1;

    init_status = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 200);
    if (init_status != Fapi_Status_Success) {
        ESTOP0;   // Debugger will halt here if initialization fails
        while(1); // Infinite spin to prevent silent failure
    }

    init_status = Fapi_setActiveFlashBank(Fapi_FlashBank0);
    if (init_status != Fapi_Status_Success) {
        ESTOP0;
        while(1);
    }
    EDIS;

InitPieCtrl();
InitPieVectTable();

EALLOW;
PieVectTable.CANB0_INT = &canb_isr;

GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;
GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;
GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;
GpioCtrlRegs.GPCPUD.bit.GPIO73 = 0;
GpioCtrlRegs.GPCDIR.bit.GPIO73 = 1;
GpioDataRegs.GPCCLEAR.bit.GPIO73 = 1;
EDIS;

CAN_Init();

DINT;
PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
PieCtrlRegs.PIEIER9.bit.INTx7 = 1;
IER |= M_INT9;
EINT;

// BOOT WINDOW
while(boot_delay < 2000000) {
    if(Fota_IsActive()) break;
    if(boot_delay % 100000 == 0) GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
    DELAY_US(1);
    boot_delay++;
}

if(Fota_IsActive() == 0) {
    jump_to_application();
}

// MAIN FOTA PROCESSING LOOP
while(1) {
    GpioDataRegs.GPASET.bit.GPIO31 = 1;


    UDS_ProcessPendingTasks();
}


}

void jump_to_application(void) {

void (*AppEntry)(void);

DINT;
PieCtrlRegs.PIECTRL.bit.ENPIE = 0;
PieCtrlRegs.PIEACK.all = 0xFFFF;
IER = 0x0000; IFR = 0x0000;

AppEntry = (void (*)(void))APP_START_ADDRESS;
AppEntry();


}

//void DisableDog(void) { EALLOW; WdRegs.SCSR.bit.WDOVERRIDE = 0; WdRegs.WDCR.all = 0x0068; EDIS; }

void InitSysClock_200MHz_INTOSC(void) {
volatile uint32_t delayCount;

EALLOW;
Flash0CtrlRegs.FRDCNTL.bit.RWAIT = 3;
Flash0CtrlRegs.FRD_INTF_CTRL.bit.DATA_CACHE_EN = 1;
Flash0CtrlRegs.FRD_INTF_CTRL.bit.PREFETCH_EN = 1;

ClkCfgRegs.CLKSRCCTL1.bit.OSCCLKSRCSEL = 0;
ClkCfgRegs.SYSPLLCTL1.bit.PLLCLKEN = 0;
for(delayCount=0; delayCount<120; delayCount++);
ClkCfgRegs.SYSCLKDIVSEL.bit.PLLSYSCLKDIV = 0;
for(delayCount=0; delayCount<5; delayCount++) {
    ClkCfgRegs.SYSPLLCTL1.bit.PLLEN = 0;
    ClkCfgRegs.SYSPLLMULT.all = 20;
    while(ClkCfgRegs.SYSPLLSTS.bit.LOCKS != 1);
}
ClkCfgRegs.SYSCLKDIVSEL.bit.PLLSYSCLKDIV = 1;
ClkCfgRegs.SYSPLLCTL1.bit.PLLCLKEN = 1;
ClkCfgRegs.SYSCLKDIVSEL.bit.PLLSYSCLKDIV = 0;
EDIS;


}
