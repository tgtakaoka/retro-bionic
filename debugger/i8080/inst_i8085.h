#ifndef __INST_I8085_H__
#define __INST_I8085_H__

#include "inst_i8080.h"

namespace debugger {
namespace i8085 {

struct InstI8085 : i8080::InstI8080 {
    static constexpr uint16_t ORG_TRAP = 0x0024;
};

}  // namespace i8085
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
