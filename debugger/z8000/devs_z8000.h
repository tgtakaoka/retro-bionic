#ifndef __DEVS_Z8000_H__
#define __DEVS_Z8000_H__

#include "z80/devs_z80.h"

// The Z80's USART in the I/O space; a byte transfer rides AD0-AD7.
#undef USART_BASE
#define USART_BASE 0x0140

#endif /* __DEVS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
