
#include "fota_config.h"

void CAN_Init(void) {
EALLOW;
ClkCfgRegs.CLKSRCCTL2.bit.CANBBCLKSEL = 0;
CpuSysRegs.PCLKCR10.bit.CAN_B = 1;

GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;
GpioCtrlRegs.GPADIR.bit.GPIO17 = 0;
GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;
GpioCtrlRegs.GPAPUD.bit.GPIO17 = 0;
GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;
GpioCtrlRegs.GPAGMUX1.bit.GPIO12 = 0;
GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;
GpioCtrlRegs.GPAGMUX2.bit.GPIO17 = 0;
GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 2;

CanbRegs.CAN_CTL.bit.Init = 1;
CanbRegs.CAN_CTL.bit.CCE = 1;
CanbRegs.CAN_CTL.bit.IE0 = 1;
CanbRegs.CAN_BTR.bit.BRP = 19;
CanbRegs.CAN_BTR.bit.TSEG1 = 13;
CanbRegs.CAN_BTR.bit.TSEG2 = 4;
CanbRegs.CAN_CTL.bit.Test = 0;
CanbRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 1;
CanbRegs.CAN_CTL.bit.Init = 0;
while(CanbRegs.CAN_CTL.bit.Init != 0);

// Setup RX Mailbox 2
while(CanbRegs.CAN_IF2CMD.bit.Busy);
CanbRegs.CAN_IF2CMD.all = 0;
CanbRegs.CAN_IF2CMD.bit.DIR = 1;
CanbRegs.CAN_IF2CMD.bit.Arb = 1;
CanbRegs.CAN_IF2CMD.bit.Control = 1;
CanbRegs.CAN_IF2ARB.all = 0;
CanbRegs.CAN_IF2ARB.bit.Xtd = 0;
CanbRegs.CAN_IF2ARB.bit.Dir = 0;
CanbRegs.CAN_IF2ARB.bit.ID = CAN_STD_ID(UDS_RX_ID);
CanbRegs.CAN_IF2ARB.bit.MsgVal = 1;
CanbRegs.CAN_IF2MCTL.all = 0;
CanbRegs.CAN_IF2MCTL.bit.RxIE = 1;
CanbRegs.CAN_IF2MCTL.bit.EoB = 1;
CanbRegs.CAN_IF2CMD.bit.MSG_NUM = 2;
while(CanbRegs.CAN_IF2CMD.bit.Busy);
EDIS;


}

/* STREAMING_CHUNK:Sending frame via C2000 pointer registers */
void CAN_Send(const CanFrame *f) {
while(CanbRegs.CAN_IF1CMD.bit.Busy);
CanbRegs.CAN_IF1CMD.all = 0;
CanbRegs.CAN_IF1CMD.bit.DIR = 1;

CanbRegs.CAN_IF1CMD.bit.DATA_A = 1;
CanbRegs.CAN_IF1CMD.bit.DATA_B = 1;
CanbRegs.CAN_IF1CMD.bit.Control = 1;
CanbRegs.CAN_IF1CMD.bit.Arb = 1;

CanbRegs.CAN_IF1ARB.all = 0;
CanbRegs.CAN_IF1ARB.bit.Xtd = 0;
CanbRegs.CAN_IF1ARB.bit.Dir = 1;
CanbRegs.CAN_IF1ARB.bit.ID = CAN_STD_ID(f->id);
CanbRegs.CAN_IF1ARB.bit.MsgVal = 1;

CanbRegs.CAN_IF1MCTL.all = 0;
CanbRegs.CAN_IF1MCTL.bit.EoB = 1;
CanbRegs.CAN_IF1MCTL.bit.TxRqst = 1;
CanbRegs.CAN_IF1MCTL.bit.DLC = f->dlc;

CanbRegs.CAN_IF1DATA.all = ((uint32_t)f->data[3] << 24) | ((uint32_t)f->data[2] << 16) | ((uint32_t)f->data[1] << 8) | f->data[0];
CanbRegs.CAN_IF1DATB.all = ((uint32_t)f->data[7] << 24) | ((uint32_t)f->data[6] << 16) | ((uint32_t)f->data[5] << 8) | f->data[4];

CanbRegs.CAN_IF1CMD.bit.MSG_NUM = 1;
while(CanbRegs.CAN_IF1CMD.bit.Busy);


}


Uint16 CAN_Receive(CanFrame *f) {
CanbRegs.CAN_IF2CMD.all = 0;
CanbRegs.CAN_IF2CMD.bit.DIR = 0;

CanbRegs.CAN_IF2CMD.bit.DATA_A = 1;

CanbRegs.CAN_IF2CMD.bit.DATA_B = 1;
CanbRegs.CAN_IF2CMD.bit.Control = 1;
CanbRegs.CAN_IF2CMD.bit.MSG_NUM = 2;

while(CanbRegs.CAN_IF2CMD.bit.Busy);

f->dlc = CanbRegs.CAN_IF2MCTL.bit.DLC;
f->data[0] = CanbRegs.CAN_IF2DATA.bit.Data_0;
f->data[1] = CanbRegs.CAN_IF2DATA.bit.Data_1;
f->data[2] = CanbRegs.CAN_IF2DATA.bit.Data_2;
f->data[3] = CanbRegs.CAN_IF2DATA.bit.Data_3;
f->data[4] = CanbRegs.CAN_IF2DATB.bit.Data_4;
f->data[5] = CanbRegs.CAN_IF2DATB.bit.Data_5;
f->data[6] = CanbRegs.CAN_IF2DATB.bit.Data_6;
f->data[7] = CanbRegs.CAN_IF2DATB.bit.Data_7;
return 1;


}
