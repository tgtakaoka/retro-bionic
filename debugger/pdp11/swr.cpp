#include "swr.h"
#include "debugger.h"

namespace debugger {
namespace pdp11 {

void Swr::print() const {
    cli.print("  Value: ");
    cli.printNum(_value, 8, 6);
    cli.println();
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
