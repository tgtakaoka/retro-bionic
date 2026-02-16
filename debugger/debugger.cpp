/**
 *  The Controller can accept commands represented by one letter.
 *
 *  R - reset CPU.
 *  d - dump data memory. addr [length]
 *  D - disassemble
 *  A - assemble
 *  m - read/write data memory
 *  M - read/write program memory
 *  p - dump program memory. addr [length]
 *  P - show/set write protect area
 *  B - set break point
 *  b - show/clear break point
 *  r - print CPU registers
 *  = - set CPU register
 *  S - step one instruction with printing bus cycles
 *  G - Go, continuously run CPU
 *  g - Go until address
 *  F - list files in SD card
 *  L - load S-record file
 *  U - upload Intel Hex/Motorola S-record
 *  I - show/select I/O device
 *  W - write identity EEPROM
 *  ? - print version
 */

#include "debugger.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"

#if defined(ENABLE_SDCARD)
#include <SD.h>
#endif

namespace debugger {

struct Debugger Debugger;
libcli::Cli cli;
#if defined(ENABLE_LOGGER)
libcli::Cli logger;
#endif

using State = libcli::State;

namespace {

constexpr char USAGE[] =
        "R:eset r:egs =:setReg p/d:ump M/m:emory"
#ifdef WITH_DISASSEMBLER
        " D:is"
#endif
#ifdef WITH_ASSEMBLER
        " A:sm"
#endif
#if defined(ENABLE_SDCARD)
        " S:tep G/g:o b/B:reak"
        " F:iles L:oad"
#endif
        " U:pload I:o";
constexpr char PROTECT[] = " P:rotect";

void usage() {
    cli.println();
    cli.print("* Bionic");
    Identity::printIdentity();
    cli.print(" * ");
    cli.println(VERSION_TEXT);
    cli.print(USAGE);
    if (Debugger.target().hasProtectArea())
        cli.print(PROTECT);
    cli.println();
}

void commandHandler(char c, uintptr_t) {
    Debugger.exec(c);
}

void printPrompt() {
    cli.print("> ");
    cli.readLetter(commandHandler, 0);
}

uint32_t last_addr;
uint16_t last_length;
constexpr auto MEMORY_LEN = 16;
uint16_t mem_buffer[MEMORY_LEN];
static constexpr uintptr_t DATA_MEMORY(int index) {
    return static_cast<uintptr_t>(index);
}
static constexpr uintptr_t PROG_MEMORY(int index) {
    return static_cast<uintptr_t>(index + MEMORY_LEN);
}
static constexpr int TO_INDEX(uintptr_t extra) {
    return static_cast<int>(extra % MEMORY_LEN);
}
constexpr auto DATA_ADDR = PROG_MEMORY(MEMORY_LEN);
constexpr auto DATA_LEN = DATA_ADDR + 1;
constexpr auto PROG_ADDR = DATA_LEN + 1;
constexpr auto PROG_LEN = PROG_ADDR + 1;
constexpr auto PROG_END = PROG_LEN + 1;

char str_buffer[40];

void handleDump(uint32_t value, uintptr_t extra, State state) {
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_DELETE) {
        cli.backspace();
        if (extra == DATA_LEN) {
            const auto maxData = Debugger.target().maxData();
            cli.readNum(handleDump, DATA_ADDR, radix, maxData, last_addr);
        } else {
            const auto maxAddr = Debugger.target().maxAddr();
            cli.readNum(handleDump, PROG_ADDR, radix, maxAddr, last_addr);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (extra == DATA_ADDR || extra == PROG_ADDR) {
            last_addr = value;
            if (state == State::CLI_SPACE) {
                extra = (extra == DATA_ADDR) ? DATA_LEN : PROG_LEN;
                cli.readNum(handleDump, extra, radix, UINT16_MAX);
                return;
            }
            last_length = 16;
        } else {
            last_length = value;
        }
        cli.println();
        const auto prog = extra >= PROG_ADDR;
        Debugger.target().dumpMemory(last_addr, last_length, prog);
        last_addr += last_length;
    }
    printPrompt();
}

#ifdef WITH_DISASSEMBLER
void handleDisassemble(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_LEN) {
            cli.backspace();
            cli.readNum(
                    handleDisassemble, PROG_ADDR, radix, maxAddr, last_addr);
        }
        return;
    }
    if (extra == PROG_ADDR) {
        last_addr = value;
        if (state == State::CLI_SPACE) {
            cli.readDec(handleDisassemble, MEMORY_LEN, UINT16_MAX);
            return;
        }
        value = 20;
    }
    cli.println();
    last_addr = Debugger.target().disassemble(last_addr, value);
cancel:
    printPrompt();
}
#endif

void handleMemory(uint32_t value, uintptr_t extra, State state) {
    const auto radix = Debugger.target().inputRadix();
    const auto unit = Debugger.target().addressUnit();
    const auto bits = Debugger.target().opCodeWidth();
    const auto maxValue =
            (unit == 1) ? UINT8_MAX : (bits == 12 ? 07777 : UINT16_MAX);
    if (state == State::CLI_DELETE) {
        if (extra == DATA_ADDR || extra == PROG_ADDR)
            return;
        cli.backspace();
        auto index = TO_INDEX(extra);
        if (index == 0) {
            if (extra >= PROG_MEMORY(0)) {
                const auto maxAddr = Debugger.target().maxAddr();
                cli.readNum(handleMemory, PROG_ADDR, radix, maxAddr, last_addr);
            } else {
                const auto maxData = Debugger.target().maxData();
                cli.readNum(handleMemory, DATA_ADDR, radix, maxData, last_addr);
            }
        } else {
            index--;
            const auto prev = (extra >= PROG_MEMORY(index))
                                      ? PROG_MEMORY(index)
                                      : DATA_MEMORY(index);
            cli.readNum(handleMemory, prev, radix, maxValue, mem_buffer[index]);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (extra == DATA_ADDR) {
            last_addr = value;
            cli.readNum(handleMemory, DATA_MEMORY(0), radix, maxValue);
            return;
        }
        if (extra == PROG_ADDR) {
            last_addr = value;
            cli.readNum(handleMemory, PROG_MEMORY(0), radix, maxValue);
            return;
        }
        auto index = TO_INDEX(extra);
        mem_buffer[index++] = value;
        if (state == State::CLI_SPACE) {
            if (index < MEMORY_LEN) {
                cli.readNum(handleMemory, extra + 1, radix, maxValue);
                return;
            }
        }
        cli.println();
        const auto prog = extra >= PROG_MEMORY(0);
        Debugger.target().writeMemory(last_addr, mem_buffer, index, prog);
        Debugger.target().dumpMemory(last_addr, index, prog);
        last_addr += index;
    }
    printPrompt();
}

void printAddr(uint32_t addr) {
    const auto bits = Debugger.target().addressWidth();
    const auto radix = Debugger.target().inputRadix();
    cli.printNum(addr, radix, Debugger::numDigits(bits, radix));
}

#ifdef WITH_ASSEMBLER
void handleAssembleLine(char *line, uintptr_t, State state) {
    if (state == State::CLI_CANCEL || *line == 0) {
        cli.println("end");
        printPrompt();
        return;
    }
    cli.println();
    last_addr = Debugger.target().assemble(last_addr, line);
    printAddr(last_addr);
    cli.print("? ");
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}

void handleAssembler(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    if (state == State::CLI_CANCEL) {
        printPrompt();
        return;
    }
    cli.println();
    printAddr(last_addr = value);
    cli.print("? ");
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}

#endif

void handleGoUntil(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    if (state == State::CLI_SPACE || state == State::CLI_NEWLINE) {
        cli.println();
        Debugger.breakPoints().setTemp(value);
        Debugger.go();
    }
    printPrompt();
}

#if defined(ENABLE_SDCARD)
void listDirectory(File dir) {
    const auto path = dir.name();
    const auto root = strcmp(path, "/") == 0;
    while (true) {
        auto entry = dir.openNextFile();
        if (!entry)
            break;
        if (!root) {
            cli.print(path);
            cli.print('/');
        }
        const auto name = entry.name();
        if (entry.isDirectory()) {
            cli.printStr(name);
            cli.println('/');
        } else {
            cli.printStr(name, -20);
            cli.printlnDec(entry.size(), 6);
        }
        entry.close();
    }
}

void handleFileListing(char *line, uintptr_t extra, State state) {
    if (state == State::CLI_CANCEL) {
        printPrompt();
        return;
    }
    cli.println();
    SD.begin(BUILTIN_SDCARD);
    const auto path = *line ? line : "/";
    auto dir = SD.open(path);
    listDirectory(dir);
    dir.close();
    printPrompt();
}
#endif

uint8_t toInt(const char c) {
    return isDigit(c) ? c - '0' : c - 'A' + 10;
}

uint8_t toInt8Hex(const char *text) {
    return (toInt(text[0]) << 4) | toInt(text[1]);
}

uint16_t toInt16Hex(const char *text) {
    return ((uint16_t)toInt8Hex(text) << 8) | toInt8Hex(text + 2);
}

uint32_t toInt24Hex(const char *text) {
    return ((uint32_t)toInt16Hex(text) << 8) | toInt8Hex(text + 4);
}

uint32_t toInt32Hex(const char *text) {
    return ((uint32_t)toInt24Hex(text) << 8) | toInt8Hex(text + 6);
}

int loadIHexRecord(const char *line, uint32_t &addr) {
    const auto num = toInt8Hex(line + 1);
    const auto offset = toInt16Hex(line + 3);
    const auto type = toInt8Hex(line + 7);
    if (type == 4) {
        addr = static_cast<uint32_t>(offset) << 16;
    } else if (type == 0) {
        addr &= ~UINT16_C(0xFFFF);
        addr |= offset;
        uint8_t buffer[64];
        for (int i = 0; i < num; i++) {
            buffer[i] = toInt8Hex(line + i * 2 + 9);
        }
        Debugger.target().loadCode(addr, buffer, num);
        const auto unit = Debugger.target().addressUnit();
        printAddr(addr / unit);
        cli.print(':');
        cli.printDec(num / unit, 2);
    }
    return num;
}

int loadS19Record(const char *line, uint32_t &addr) {
    const int num = toInt8Hex(line + 2) - 3;
    switch (line[1]) {
    case '1':
        addr = toInt16Hex(line + 4);
        line += 8;
        break;
    case '2':
        addr = toInt24Hex(line + 4);
        line += 10;
        break;
    case '3':
        addr = toInt32Hex(line + 4);
        line += 12;
        break;
    default:
        return 0;
    }
    uint8_t buffer[64];
    for (int i = 0; i < num; i++) {
        buffer[i] = toInt8Hex(line + i * 2);
    }
    Debugger.target().loadCode(addr, buffer, num);
    const auto unit = Debugger.target().addressUnit();
    printAddr(addr / unit);
    cli.print(':');
    cli.printDec(num / unit, 2);
    return num;
}

#if defined(ENABLE_SDCARD)
void handleLoadFile(char *line, uintptr_t, State state) {
    if (state == State::CLI_CANCEL) {
        printPrompt();
        return;
    }
    cli.println();
    if (*line) {
        SD.begin(BUILTIN_SDCARD);
        File file = SD.open(line);
        if (!file) {
            cli.print(line);
            cli.println(" not found");
        } else {
            uint16_t size = 0;
            char buffer[80];
            char *p = buffer;
            uint32_t addr;
            while (file.available() > 0) {
                const char c = file.read();
                if (c == '\n') {
                    *p = 0;
                    if (*buffer == 'S') {
                        cli.print(buffer);
                        cli.print(' ');
                        size += loadS19Record(buffer, addr);
                        cli.println();
                    } else if (*buffer == ':') {
                        cli.print(buffer);
                        cli.print(' ');
                        size += loadIHexRecord(buffer, addr);
                        cli.println();
                    }
                    p = buffer;
                } else if (c != '\r' && p < buffer + sizeof(buffer) - 1) {
                    *p++ = c;
                }
            }
            file.close();
            cli.println();
            cli.print(size);
            cli.println(" bytes loaded");
        }
    }
    printPrompt();
}
#endif

struct UploadContext {
    uint32_t size;
    uint32_t addr;
    char buffer[80];
    uintptr_t extra() { return reinterpret_cast<uintptr_t>(this); }
    static UploadContext *context(uintptr_t extra) {
        return reinterpret_cast<UploadContext *>(extra);
    }
} upload_context;

void handleUploadFile(char *line, uintptr_t extra, State state) {
    UploadContext *context = UploadContext::context(extra);
    if (state == State::CLI_CANCEL) {
        cli.print(context->size);
        cli.println(" bytes uploaded");
        printPrompt();
        return;
    }
    const auto c = context->buffer[0];
    if (c == 'S') {
        context->size += loadS19Record(context->buffer, context->addr);
        cli.println();
    } else if (c == ':') {
        context->size += loadIHexRecord(context->buffer, context->addr);
        cli.println();
    }
    cli.readLine(handleUploadFile, context->extra(), context->buffer,
            sizeof(context->buffer));
}

void handleRegisterValue(uint32_t, uintptr_t, State);

void handleSetRegister(char *word, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    uint8_t reg;
    uint32_t max;
    const auto radix = Debugger.target().inputRadix();
    if ((reg = Debugger.target().validRegister(word, max)) != 0) {
        cli.print('?');
        cli.readNum(handleRegisterValue, reg, radix, max);
        return;
    }
    Debugger.target().helpRegisters();
    printPrompt();
}

void handleRegisterValue(uint32_t value, uintptr_t reg, State state) {
    if (state == State::CLI_DELETE) {
        cli.backspace(2);  // ' ', '?'
        cli.readWord(
                handleSetRegister, 0, str_buffer, sizeof(str_buffer), true);
        return;
    }
    if (state != State::CLI_CANCEL) {
        const auto dis = Debugger.target().setRegister(reg, value);
        cli.println();
        Debugger.target().printRegisters(dis);
    }
    printPrompt();
}

void handleIo(char *, uintptr_t, State);

void handleIoBase(uint32_t val, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        auto &dev = *reinterpret_cast<Device *>(extra);
        if (state == State::CLI_DELETE) {
            cli.backspace();
            strcpy(str_buffer, dev.name());
            cli.readWord(handleIo, 0, str_buffer, sizeof(str_buffer), true);
            return;
        }
        dev.setBaseAddr(val);
        cli.println();
        Debugger.target().printDevices();
    }
    printPrompt();
}

void handleIo(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        const auto nulldev = Devs::nullDevice();
        auto dev = Debugger.target().parseDevice(line);
        if (state == State::CLI_SPACE) {
            if (dev != nulldev) {
                const auto maxData = Debugger.target().maxData();
                const auto radix = Debugger.target().inputRadix();
                cli.readNum(handleIoBase, reinterpret_cast<uintptr_t>(dev),
                        radix, maxData);
                return;
            }
        }
        cli.println();
        if (dev != nulldev) {
            Debugger.target().enableDevice(dev);
            Debugger.target().printDevices();
        }
    }
    printPrompt();
}

void handleProtectArea(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_DELETE) {
        if (extra == PROG_END) {
            cli.backspace();
            cli.readNum(
                    handleProtectArea, PROG_ADDR, radix, maxAddr, last_addr);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (state == State::CLI_SPACE && extra == PROG_ADDR) {
            last_addr = value;
            cli.readNum(handleProtectArea, PROG_END, radix, maxAddr);
            return;
        }
        if (extra == PROG_END)
            Debugger.target().setProtectArea(last_addr, value);
        cli.println();
        Debugger.target().printProtectArea();
    }
    printPrompt();
}

void handleSetBreak(uint32_t value, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        if (Debugger.breakPoints().set(value)) {
            cli.println("set");
        } else {
            cli.println("full");
        }
        Debugger.breakPoints().print();
    }
    printPrompt();
}

void handleClearBreak(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        const auto index = atoi(str_buffer);
        if (Debugger.breakPoints().clear(index)) {
            cli.println(" cleared");
            Debugger.breakPoints().print();
        } else {
            cli.println();
        }
    }
    printPrompt();
}

void handleWriteIdentity(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        cli.println();
        if (line[0] == '+')
            Identity::writeIdentity(line + 1);
    }
    printPrompt();
}

}  // namespace

void Debugger::go() {
    if (_breakPoints.on(target().nextIp())) {
        // step over break point
        if (!target().step(false))
            return;
        // Stop at consecutive break point
        if (_breakPoints.on(target().nextIp())) {
            target().printRegisters();
            return;
        }
    }
    target().run();
    target().printRegisters();
#if defined(ENABLE_LOGGER)
    while (Logger.available()) {
        const char c = Logger.read();
        if (c == '\n' || c == '\r') {
            cli.println();
        } else {
            cli.print(c);
        }
    }
#endif
}

void Debugger::exec(char c) {
    const auto maxData = target().maxData();
    const auto maxAddr = target().maxAddr();
    const auto radix = target().inputRadix();
    switch (c) {
    case 'R':
        cli.println("Reset");
        target().reset();
        if (_verbose)
            target().printCycles();
        target().printRegisters();
        break;
    case 'd':
        cli.print("Data dump? ");
        cli.readNum(handleDump, DATA_ADDR, radix, maxData);
        return;
    case 'p':
        cli.print("Program dump? ");
        cli.readNum(handleDump, PROG_ADDR, radix, maxAddr);
        return;
#ifdef WITH_DISASSEMBLER
    case 'D':
        cli.print("Disassemble? ");
        cli.readNum(handleDisassemble, PROG_ADDR, radix, maxAddr);
        return;
#endif
#ifdef WITH_ASSEMBLER
    case 'A':
        cli.print("Assemble? ");
        cli.readNum(handleAssembler, 0, radix, maxAddr);
        return;
#endif
    case 'm':
        cli.print("Data Memory? ");
        cli.readNum(handleMemory, DATA_ADDR, radix, maxData);
        return;
    case 'M':
        cli.print("Program Memory? ");
        cli.readNum(handleMemory, PROG_ADDR, radix, maxAddr);
        return;
    case 'P':
        if (target().printProtectArea()) {
            cli.print("  Protect area? ");
            cli.readNum(handleProtectArea, PROG_ADDR, radix, maxAddr);
            return;
        } else {
            cli.println("No Protect area");
        }
        break;
    case 'B':
        cli.print("Break addr? ");
        cli.readNum(handleSetBreak, 0, radix, maxAddr);
        return;
    case 'b':
        cli.println("Break points");
        if (breakPoints().print()) {
            cli.print("clear? ");
            cli.readLine(handleClearBreak, 0, str_buffer, sizeof(str_buffer));
            return;
        }
        break;
    case 'r':
        cli.println("Registers");
        target().printRegisters();
        break;
    case '=':
        cli.print("Set register? ");
        cli.readWord(handleSetRegister, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'S':
        cli.println("Step");
        if (_verbose) {
            target().step(false);
            target().printCycles();
        } else {
            target().step(true);
        }
        target().printRegisters();
        break;
    case 'G':
        cli.println("Go");
        go();
        break;
    case 'g':
        cli.print("Go until? ");
        cli.readNum(handleGoUntil, 0, radix, maxAddr);
        return;
#if defined(ENABLE_SDCARD)
    case 'F':
        cli.print("Files? ");
        cli.readLine(handleFileListing, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'L':
        cli.print("Load? ");
        cli.readLine(handleLoadFile, 0, str_buffer, sizeof(str_buffer));
        return;
#endif
    case 'U':
        cli.println("Upload waiting...");
        upload_context.size = 0;
        cli.readLine(handleUploadFile, upload_context.extra(),
                upload_context.buffer, sizeof(upload_context.buffer));
        return;
    case 'I':
        cli.println("I/O Devices");
        target().printDevices();
        cli.print("which? ");
        cli.readWord(handleIo, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'W':
        cli.print("Write identity? ");
        cli.readLine(handleWriteIdentity, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'V':
        _verbose = !_verbose;
        cli.print("Verbose ");
        cli.println(_verbose ? "ON" : "OFF");
        break;
    case '?':
        usage();
        break;
    case '\r':
        cli.println();
    case '\n':
        break;
    default:
        return;
    }
    printPrompt();
}

void Debugger::begin(Target *target) {
    _target = target;
    target->begin();
    usage();
    target->printRegisters();
    printPrompt();
}

void Debugger::loop() {
    cli.loop();
    target().idle();
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
