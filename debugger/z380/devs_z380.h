#ifndef __DEVS_Z380_H__
#define __DEVS_Z380_H__

#include "z80/devs_z80.h"

// The Z80's USART, above FFH where only a 16- or 32-bit I/O address
// reaches it.
#undef USART_BASE
#define USART_BASE 0x0140

#endif /* __DEVS_Z380_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
