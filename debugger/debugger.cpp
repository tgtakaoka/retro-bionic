/**
 *  The Controller can accept commands represented by one letter.
 *
 *  R - reset CPU.
 *  d - dump memory. addr [length]
 *  D - disassemble
 *  A - assemble
 *  m - write memory
 *  M - show/set ROM area
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
#include "unio_eeprom.h"

#include <SD.h>

namespace debugger {

struct Debugger Debugger;
libcli::Cli cli;
#if defined(ENABLE_LOGGER)
libcli::Cli logger;
#endif

using State = libcli::State;

namespace {

constexpr char USAGE[] =
        "R:eset r:egs =:setReg d:ump m:emory"
#ifdef WITH_DISASSEMBLER
        " D:is"
#endif
#ifdef WITH_ASSEMBLER
        " A:sm"
#endif
        " S:tep G/g:o b/B:reak F:iles L:oad U:pload I:o ro:M";

void usage() {
    cli.println();
    cli.print("* Bionic");
    Identity::printIdentity();
    cli.print(" * ");
    cli.println(VERSION_TEXT);
    cli.println(USAGE);
}

void commandHandler(char c, uintptr_t extra) {
    (void)extra;
    Debugger.exec(c);
}

void printPrompt() {
    cli.print("> ");
    cli.readLetter(commandHandler, 0);
}

uint32_t last_addr;
uint16_t last_length;
#define DUMP_ADDR 0
#define DUMP_LENGTH 1
#define DUMP_SPACE 2
#define DIS_ADDR 0
#define DIS_LENGTH 1

#define MEMORY_LEN 16
uint16_t mem_buffer[MEMORY_LEN];
#define MEMORY_DATA(i) ((uintptr_t)(i))
#define MEMORY_ADDR static_cast<uintptr_t>(MEMORY_LEN + 1)
#define MEMORY_END (MEMORY_ADDR + 1)

char str_buffer[40];

void handleDump(uint32_t value, uintptr_t extra, State state);

void handleDumpSpace(char *word, uintptr_t extra, State state) {
    const auto radix = Debugger.target().inputRadix();
    if (state != State::CLI_CANCEL) {
        if (state == State::CLI_DELETE) {
            cli.backspace();
            cli.readNum(
                    handleDump, DUMP_LENGTH, radix, UINT16_MAX, last_length);
            return;
        }
        cli.println();
        Debugger.target().dumpMemory(last_addr, last_length, word);
    }
    printPrompt();
}

void handleDump(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DUMP_LENGTH) {
            cli.backspace();
            cli.readNum(handleDump, DUMP_ADDR, radix, maxAddr, last_addr);
        }
        return;
    }
    if (extra == DUMP_LENGTH) {
        last_length = value;
        if (state == State::CLI_SPACE) {
            cli.readWord(handleDumpSpace, DUMP_SPACE, str_buffer,
                    sizeof(str_buffer));
            return;
        }
    } else if (extra == DUMP_ADDR) {
        last_addr = value;
        if (state == State::CLI_SPACE) {
            cli.readNum(handleDump, DUMP_LENGTH, radix, UINT16_MAX);
            return;
        }
        last_length = 16;
    }
    cli.println();
    Debugger.target().dumpMemory(last_addr, last_length);
    last_addr += last_length;
cancel:
    printPrompt();
}

#ifdef WITH_DISASSEMBLER
void handleDisassemble(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DIS_LENGTH) {
            cli.backspace();
            cli.readNum(handleDisassemble, DIS_ADDR, radix, maxAddr, last_addr);
        }
        return;
    }
    if (extra == DIS_ADDR) {
        last_addr = value;
        if (state == State::CLI_SPACE) {
            cli.readDec(handleDisassemble, DIS_LENGTH, UINT16_MAX);
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
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    const auto unit = Debugger.target().addressUnit();
    const auto bits = Debugger.target().opCodeWidth();
    const auto maxData =
            (unit == 1) ? UINT8_MAX : (bits == 12 ? 07777 : UINT16_MAX);
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_ADDR)
            return;
        cli.backspace();
        uint16_t index = extra;
        if (index == 0) {
            cli.readNum(handleMemory, MEMORY_ADDR, radix, maxAddr, last_addr);
        } else {
            index--;
            cli.readNum(handleMemory, MEMORY_DATA(index), radix, maxData,
                    mem_buffer[index]);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (extra == MEMORY_ADDR) {
            last_addr = value;
            cli.readNum(handleMemory, MEMORY_DATA(0), radix, maxData);
            return;
        }
        uint16_t index = extra;
        mem_buffer[index++] = value;
        if (state == State::CLI_SPACE) {
            if (index < MEMORY_LEN) {
                cli.readNum(handleMemory, MEMORY_DATA(index), radix, maxData);
                return;
            }
        }
        cli.println();
        Debugger.target().write_memory(last_addr, mem_buffer, index);
        Debugger.target().dumpMemory(last_addr, index);
        last_addr += index;
    }
    printPrompt();
}

#ifdef WITH_ASSEMBLER
void handleAssembleLine(char *line, uintptr_t extra, State state) {
    (void)extra;
    if (state == State::CLI_CANCEL || *line == 0) {
        cli.println("end");
        printPrompt();
        return;
    }
    cli.println();
    last_addr = Debugger.target().assemble(last_addr, line);
    cli.printHex(last_addr, 4);
    cli.print('?');
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}

void handleAssembler(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE)
        return;
    if (state == State::CLI_CANCEL) {
        printPrompt();
        return;
    }
    last_addr = value;
    cli.println();
    cli.printHex(last_addr, 4);
    cli.print('?');
    cli.readLine(handleAssembleLine, 0, str_buffer, sizeof(str_buffer));
}

#endif

void handleGoUntil(uint32_t value, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        Debugger.setTempBreakPoint(value);
        Debugger.go();
    }
    printPrompt();
}

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

int loadIHexRecord(const char *line) {
    const auto bits = Debugger.target().addressWidth();
    const auto unit = Debugger.target().addressUnit();
    const auto radix = Debugger.target().inputRadix();
    const auto num = toInt8Hex(line + 1);
    uint16_t addr = toInt16Hex(line + 3);
    const auto type = toInt8Hex(line + 7);
    // TODO: Support 32bit Intel Hex
    if (type == 0) {
        uint8_t buffer[num];
        for (int i = 0; i < num; i++) {
            buffer[i] = toInt8Hex(line + i * 2 + 9);
        }
        addr /= unit;
        Debugger.target().write_code(addr, buffer, num);
        cli.printNum(addr, radix, Debugger::numDigits(bits, radix));
        cli.print(':');
        cli.printDec(num / unit, 2);
    }
    return num;
}

int loadS19Record(const char *line) {
    const auto bits = Debugger.target().addressWidth();
    const auto unit = Debugger.target().addressUnit();
    const auto radix = Debugger.target().inputRadix();
    const int num = toInt8Hex(line + 2) - 3;
    uint32_t addr;
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
    addr /= unit;
    uint8_t buffer[num];
    for (int i = 0; i < num; i++) {
        buffer[i] = toInt8Hex(line + i * 2);
    }
    Debugger.target().write_code(addr, buffer, num);
    cli.printNum(addr, radix, Debugger::numDigits(bits, radix));
    cli.print(':');
    cli.printDec(num / unit, 2);
    return num;
}

void handleLoadFile(char *line, uintptr_t extra, State state) {
    (void)extra;
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
            while (file.available() > 0) {
                const char c = file.read();
                if (c == '\n') {
                    *p = 0;
                    if (*buffer == 'S') {
                        cli.print(buffer);
                        cli.print(' ');
                        size += loadS19Record(buffer);
                        cli.println();
                    } else if (*buffer == ':') {
                        cli.print(buffer);
                        cli.print(' ');
                        size += loadIHexRecord(buffer);
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

struct UploadContext {
    uint32_t size;
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
        context->size += loadS19Record(context->buffer);
        cli.println();
    } else if (c == ':') {
        context->size += loadIHexRecord(context->buffer);
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
        Debugger.target().setRegister(reg, value);
        cli.println();
        Debugger.target().printRegisters();
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
                const auto maxAddr = Debugger.target().maxAddr();
                const auto radix = Debugger.target().inputRadix();
                cli.readNum(handleIoBase, reinterpret_cast<uintptr_t>(dev),
                        radix, maxAddr);
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

void handleRomArea(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.target().maxAddr();
    const auto radix = Debugger.target().inputRadix();
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_END) {
            cli.backspace();
            cli.readNum(handleRomArea, MEMORY_END, radix, maxAddr, last_addr);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (state == State::CLI_SPACE && extra == MEMORY_ADDR) {
            last_addr = value;
            cli.readNum(handleRomArea, MEMORY_END, radix, maxAddr);
            return;
        }
        if (extra == MEMORY_END)
            Debugger.target().setRomArea(last_addr, value);
        cli.println();
        Debugger.target().printRomArea();
    }
    printPrompt();
}

void handleSetBreak(uint32_t value, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        if (Debugger.setBreakPoint(value)) {
            cli.println("set");
        } else {
            cli.println("full");
        }
        Debugger.printBreakPoints();
    }
    printPrompt();
}

void handleClearBreak(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        const auto index = atoi(str_buffer);
        if (Debugger.clearBreakPoint(index)) {
            cli.println(" cleared");
            Debugger.printBreakPoints();
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
            target().printStatus();
            return;
        }
    }
    target().run();
    target().printStatus();
}

void Debugger::exec(char c) {
    const auto maxAddr = target().maxAddr();
    const auto radix = target().inputRadix();
    switch (c) {
    case 'R':
        cli.println("Reset");
        target().reset();
        if (_verbose)
            target().printCycles();
        target().printStatus();
        break;
    case 'd':
        cli.print("Dump? ");
        cli.readNum(handleDump, DUMP_ADDR, radix, maxAddr);
        return;
#ifdef WITH_DISASSEMBLER
    case 'D':
        cli.print("Disassemble? ");
        cli.readNum(handleDisassemble, DIS_ADDR, radix, maxAddr);
        return;
#endif
#ifdef WITH_ASSEMBLER
    case 'A':
        cli.print("Assemble? ");
        cli.readNum(handleAssembler, 0, radix, maxAddr);
        return;
#endif
    case 'm':
        cli.print("Memory? ");
        cli.readNum(handleMemory, MEMORY_ADDR, radix, maxAddr);
        return;
    case 'M':
        if (target().printRomArea()) {
            cli.print("  ROM area? ");
            cli.readNum(handleRomArea, MEMORY_ADDR, radix, maxAddr);
            return;
        } else {
            cli.println("No ROM area");
        }
        break;
    case 'B':
        cli.print("Break addr? ");
        cli.readNum(handleSetBreak, 0, radix, maxAddr);
        return;
    case 'b':
        cli.println("Break points");
        if (printBreakPoints()) {
            cli.print("clear? ");
            cli.readLine(handleClearBreak, 0, str_buffer, sizeof(str_buffer));
            return;
        }
        break;
    case 'r':
        cli.println("Registers");
        target().printStatus();
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
        target().printStatus();
        break;
    case 'G':
        cli.println("Go");
        go();
        break;
    case 'g':
        cli.print("Go until? ");
        cli.readNum(handleGoUntil, 0, radix, maxAddr);
        return;
    case 'F':
        cli.print("Files? ");
        cli.readLine(handleFileListing, 0, str_buffer, sizeof(str_buffer));
        return;
    case 'L':
        cli.print("Load? ");
        cli.readLine(handleLoadFile, 0, str_buffer, sizeof(str_buffer));
        return;
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
