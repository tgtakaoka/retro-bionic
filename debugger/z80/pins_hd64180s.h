#ifndef __PINS_HD64180S_H__
#define __PINS_HD64180S_H__

#include "pins_z180.h"

namespace debugger {
namespace hd64180s {

struct PinsHD64180S final : z180::PinsZ180 {
    PinsHD64180S() : PinsZ180() {};

protected:
    void configureCpu() override;
};

}  // namespace hd64180s
}  // namespace debugger
#endif /* __PINS_HD64180S_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
