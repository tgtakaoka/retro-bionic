#ifndef __SIGNALS_I8051_H__
#define __SIGNALS_I8051_H__

#include "sigs.h"

namespace debugger {
namespace i8051 {

struct Signals final : Sigs<Signals> {
    void getAddress();
    void getControl();
    void getData();
    void setData() const;
    static void outputMode();
    static void inputMode();

    void print() const override;

    bool read() const;
    bool write() const;
    bool fetch() const override;
    void clearFetch();

private:
    uint8_t _cntl;
};

}  // namespace i8051
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
