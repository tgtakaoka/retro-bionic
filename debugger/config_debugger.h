#ifndef __CONFIG_DEBUGGER_H__
#define __CONFIG_DEBUGGER_H__

#define VERSION_TEXT "1.0"

#define Console Serial

#define ENABLE_LOGGER
#define Logger SerialUSB1

#define PIN_USRSW 24 /* P6.12: Halt Switch */
#define PIN_ID 34    /* P7.29: Identifity EEPROM UNI/O SCIO pin */

#define ENABLE_USRLED
#if defined(ENABLE_USRLED)
#define PIN_USRLED 25 /* P6.13: User LED */
#define PIN_DEBUG 35  /* P7.28: Debug */
#else
#define PIN_DEBUG 25 /* P6.13: Debug */
#endif

// #define WITH_ASSEMBLER
#define WITH_DISASSEMBLER

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
