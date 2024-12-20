#include "inst_mc146805.h"

namespace debugger {
namespace mc146805 {

namespace {
constexpr uint8_t INST_TABLE[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0xED,  //  00~3F
        0x9B, 0xED, 0x9B, 0xED, 0x9B, 0xED, 0x9B, 0xED,  //  40~7F
        0xD0, 0x03, 0x01, 0xFD, 0xFE, 0xF6, 0xFF, 0xFF,  //  80~BF
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //  C0~FF
};
}

const struct InstMc146805 Inst;

const uint8_t *InstMc146805::table() const {
    return INST_TABLE;
}

}  // namespace mc146805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
