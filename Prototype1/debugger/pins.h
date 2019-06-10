#ifndef __pins_h__
#define __pins_h__

#include <stdint.h>

class Pins {

public:
  void begin();
  void loop();
  void reset();
  void print() const;

  uint8_t dbus() { return _signals.dbus; }
  void execInst(const uint8_t *inst, uint8_t len, bool show = false);
  void captureWrites(const uint8_t *inst, uint8_t len, uint8_t *buf, uint8_t max);
  void step(bool show = false);
  bool run();
  void runStep();
  bool halt();

private:

  struct Status {
    void get();
    unsigned reset:1;
    unsigned halt:1;
    unsigned ba:1;
    unsigned bs:1;
    unsigned lic:1;
    unsigned avma:1;
    unsigned busy:1;
    unsigned rw:1;
    uint8_t  dbus;
    bool inst;
  };

  class Dbus {
  public:
    void begin();
    void set(uint8_t data);
    void output();
    void input();
    bool valid() const { return _valid; }
    void capture(bool enable);
    static uint8_t getDbus();
  private:
    void setDbus(uint8_t dir, uint8_t data);
    uint8_t _dir = INPUT;
    uint8_t _data;
    bool _valid;
    bool _capture;
  };

  void cycle();
  void setData(uint8_t data);
  void unhalt();

  bool unchanged() const;
  bool inHalt() const;
  bool vectorFetch() const;
  bool running() const;
  bool lastInstCycle() const;
  bool writeCycle() const;
  bool readCycle() const;

  uint16_t _cycle;
  Status _signals;
  Status _previous;
  Dbus _dbus;
  bool _run;
  bool _inStep;
};

extern Pins Pins;

#endif /* __pins_h__ */