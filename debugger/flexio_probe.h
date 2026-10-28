#ifndef __FLEXIO_PROBE_H__
#define __FLEXIO_PROBE_H__

namespace debugger {

// The Q command of the teensy41-flexio image: FlexIO3 latching P10-P27 on
// an emulated ALE, with P47 wired to P34 and P53 to #RD or #WR, and no CPU
// on the bus.
void flexioProbe(const char *line);

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
