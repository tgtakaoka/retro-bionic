#ifndef __DEBUGGER_IDENTITY_H__
#define __DEBUGGER_IDENTITY_H__

namespace debugger {

struct Target;

struct Identity final {
    Identity(const char *name, Target *(*instance)(const Identity *));

    const char *name() const { return _name; }
    Target *instance() const { return _instance(this); }

    static const Identity &readIdentity();
    static void writeIdentity(const char *name);
    static void printIdentity();

    static const Identity *_head;
    static const Identity &searchIdentity(const char *name);

private:
    const char *const _name;
    Target *(*const _instance)(const Identity *);
    const Identity *const _next;
};

extern const struct Identity IdentityNull;

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
