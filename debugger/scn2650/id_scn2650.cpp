#include "identity.h"

#include "pins_scn2650.h"

namespace debugger {
namespace scn2650 {

Pins *instance() {
    return new PinsScn2650();
}

TARGET_OVERLAY(scn2650);

const struct Identity SCN2650{"SCN2650", instance, overlay};

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
