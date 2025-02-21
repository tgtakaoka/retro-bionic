#include "devs_i8085.h"
#include <string.h>
#include "debugger.h"
#include "i8085_sio_handler.h"
#include "i8251.h"

namespace debugger {
namespace i8085 {

DevsI8085::DevsI8085()
    : DevsI8080()
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _sio(new I8085SioHandler())
#endif
{
}

DevsI8085::~DevsI8085() {
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sio;
#endif
}

void DevsI8085::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _sio->reset();
#endif
    DevsI8080::reset();
}

void DevsI8085::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sio->setIdle(idle);
#endif
}

Device *DevsI8085::parseDevice(const char *name) const {
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sio->name()) == 0)
        return _sio;
#endif
    return DevsI8080::parseDevice(name);
}

void DevsI8085::enableDevice(Device *dev) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sio->enable(dev == _sio);
#endif
    DevsI8080::enableDevice(dev);
}

void DevsI8085::printDevices() const {
    DevsI8080::printDevices();
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sio);
#endif
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
