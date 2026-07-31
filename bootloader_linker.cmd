/* * BOOTLOADER LINKER SCRIPT

This maps the Bootloader code ONLY to Sector A.

It also prepares the RAM for the Flash API (RAMLS).
*/

MEMORY
{
PAGE 0 :
/* Bootloader lives safely in Sector A */
BEGIN           : origin = 0x080000, length = 0x000002
RESET           : origin = 0x3FFFC0, length = 0x000002
BOOT_FLASH  : origin = 0x080002, length = 0x003FFE
/* High Speed RAM for executing the Flash API later */
RAMLS0123       : origin = 0x008000, length = 0x002000


PAGE 1 :
BOOT_RSVD       : origin = 0x000002, length = 0x000120
RAMM0           : origin = 0x000122, length = 0x0002DE
RAMM1           : origin = 0x000400, length = 0x000400
}

SECTIONS
{
codestart       : > BEGIN,       PAGE = 0
.text           : > BOOT_FLASH , PAGE = 0
.cinit          : > BOOT_FLASH , PAGE = 0
.pinit          : > BOOT_FLASH , PAGE = 0
.switch         : > BOOT_FLASH , PAGE = 0
.econst         : > BOOT_FLASH , PAGE = 0
.reset           : > RESET,     PAGE = 0, TYPE = DSECT

/* This allows us to copy Flash API functions from Sector A into RAM */
.TI.ramfunc     : LOAD = BOOT_FLASH ,
                  RUN = RAMLS0123,
                  LOAD_START(_RamfuncsLoadStart),
                  LOAD_SIZE(_RamfuncsLoadSize),
                  LOAD_END(_RamfuncsLoadEnd),
                  RUN_START(_RamfuncsRunStart),
                  RUN_SIZE(_RamfuncsRunSize),
                  RUN_END(_RamfuncsRunEnd),
                  PAGE = 0
{
    -lF021_API_F2837xD_FPU32.lib
}
/* Standard Data Sections */
.stack          : > RAMM1,       PAGE = 1
.ebss           : > RAMM0,       PAGE = 1
.esysmem        : > RAMM0,       PAGE = 1


}
