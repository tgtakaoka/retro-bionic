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
        " S:tep G:o b/B:reak F:iles L:oad U:pload I:o";
constexpr char USAGE_ROM[] = " roM";

void usage() {
    cli.println();
    cli.print("* Bionic");
    Target::printIdentity();
    cli.print(" * ");
    cli.println(F(VERSION_TEXT));
    cli.print(USAGE);
    if (Debugger.mems().hasRomArea())
        cli.print(USAGE_ROM);
    cli.println();
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

uint8_t mem_buffer[16];
#define MEMORY_DATA(i) ((uintptr_t)(i))
#define MEMORY_ADDR static_cast<uintptr_t>(sizeof(mem_buffer) + 1)
#define MEMORY_END (MEMORY_ADDR + 1)

char str_buffer[40];

void memoryDump(uint32_t addr, uint16_t len, const char *space = nullptr) {
    const auto start = addr;
    const auto end = addr + len;
    for (addr &= ~0xF; addr < end; addr += 16) {
        cli.printHex(addr, 4);
        cli.print(':');
        for (auto i = 0; i < 16; i++) {
            const auto a = addr + i;
            if (a < start || a >= end) {
                cli.print("   ");
            } else {
                const auto data = Debugger.mems().get(a, space);
                mem_buffer[i] = data;
                cli.print(' ');
                cli.printHex(data, 2);
            }
        }
        cli.print(' ');
        for (auto i = 0; i < 16; i++) {
            const auto a = addr + i;
            if (a < start || a >= end) {
                cli.print(' ');
            } else {
                const char data = mem_buffer[i];
                if (isprint(data)) {
                    cli.print(data);
                } else {
                    cli.print('.');
                }
            }
        }
        cli.println();
    }
}

void handleDump(uint32_t value, uintptr_t extra, State state);

void handleDumpSpace(char *word, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        if (state == State::CLI_DELETE) {
            cli.backspace();
            cli.readHex(handleDump, DUMP_LENGTH, UINT16_MAX, last_length);
            return;
        }
        cli.println();
        memoryDump(last_addr, last_length, word);
    }
    printPrompt();
}

void handleDump(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DUMP_LENGTH) {
            cli.backspace();
            cli.readHex(handleDump, DUMP_ADDR, Debugger.mems().maxAddr(),
                    last_addr);
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
            cli.readHex(handleDump, DUMP_LENGTH, UINT16_MAX);
            return;
        }
        last_length = 16;
    }
    cli.println();
    memoryDump(last_addr, last_length);
    last_addr += last_length;
cancel:
    printPrompt();
}

#ifdef WITH_DISASSEMBLER
void handleDisassemble(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_CANCEL)
        goto cancel;
    if (state == State::CLI_DELETE) {
        if (extra == DIS_LENGTH) {
            cli.backspace();
            cli.readHex(handleDisassemble, DIS_ADDR, Debugger.mems().maxAddr(),
                    last_addr);
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
    last_addr = Debugger.mems().disassemble(last_addr, value);
cancel:
    printPrompt();
}
#endif

void handleMemory(uint32_t value, uintptr_t extra, State state) {
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_ADDR)
            return;
        cli.backspace();
        uint16_t index = extra;
        if (index == 0) {
            cli.readHex(handleMemory, MEMORY_ADDR, Debugger.mems().maxAddr(),
                    last_addr);
        } else {
            index--;
            cli.readHex(handleMemory, MEMORY_DATA(index), UINT8_MAX,
                    mem_buffer[index]);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (extra == MEMORY_ADDR) {
            last_addr = value;
            cli.readHex(handleMemory, MEMORY_DATA(0), UINT8_MAX);
            return;
        }
        uint16_t index = extra;
        mem_buffer[index++] = value;
        if (state == State::CLI_SPACE) {
            if (index < sizeof(mem_buffer)) {
                cli.readHex(handleMemory, MEMORY_DATA(index), UINT8_MAX);
                return;
            }
        }
        cli.println();
        Debugger.mems().put(last_addr, mem_buffer, index);
        memoryDump(last_addr, index);
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
    last_addr = Debugger.mems().assemble(last_addr, line);
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
void listDirectory(File dir, const char *parent = nullptr) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry)
            break;
        if (entry.isDirectory()) {
            listDirectory(entry, entry.name());
        } else {
            if (parent) {
                cli.print(parent);
                cli.print('/');
            }
            cli.printStr(entry.name(), -20);
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
    const char *dir = (*line == 0) ? "/" : line;
    SD.begin(BUILTIN_SDCARD);
    File root = SD.open(dir);
    listDirectory(root);
    root.close();
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
    const auto num = toInt8Hex(line + 1);
    const uint16_t addr = toInt16Hex(line + 3);
    const auto type = toInt8Hex(line + 7);
    // TODO: Support 32bit Intel Hex
    if (type == 0) {
        uint8_t buffer[num];
        for (int i = 0; i < num; i++) {
            buffer[i] = toInt8Hex(line + i * 2 + 9);
        }
        Debugger.mems().put(addr, buffer, num);
        cli.printHex(addr, 4);
        cli.print(':');
        cli.printHex(num, 2);
    }
    return num;
}

int loadS19Record(const char *line) {
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
    uint8_t buffer[num];
    for (int i = 0; i < num; i++) {
        buffer[i] = toInt8Hex(line + i * 2);
    }
    Debugger.mems().put(addr, buffer, num);
    cli.printHex(addr, 4);
    cli.print(':');
    cli.printHex(num, 2);
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
    if ((reg = Debugger.regs().validRegister(word, max)) != 0) {
        cli.print('?');
        cli.readHex(handleRegisterValue, reg, max);
        return;
    }
    Debugger.regs().helpRegisters();
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
        Debugger.regs().setRegister(reg, value);
        cli.println();
        Debugger.regs().print();
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
        Debugger.devs().printDevices();
    }
    printPrompt();
}

void handleIo(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        const auto *nulldev = &Debugger.devs().nullDevice();
        auto &dev = Debugger.devs().parseDevice(line);
        if (state == State::CLI_SPACE) {
            if (&dev != nulldev) {
                cli.readHex(handleIoBase, reinterpret_cast<uintptr_t>(&dev),
                        Debugger.mems().maxAddr());
                return;
            }
        }
        if (&dev != nulldev)
            Debugger.devs().enableDevice(dev);
        cli.println();
        Debugger.devs().printDevices();
    }
    printPrompt();
}

void handleRomArea(uint32_t value, uintptr_t extra, State state) {
    const auto maxAddr = Debugger.mems().maxAddr();
    if (state == State::CLI_DELETE) {
        if (extra == MEMORY_END) {
            cli.backspace();
            cli.readHex(handleRomArea, MEMORY_END, maxAddr, last_addr);
        }
        return;
    }
    if (state != State::CLI_CANCEL) {
        if (state == State::CLI_SPACE && extra == MEMORY_ADDR) {
            last_addr = value;
            cli.readHex(handleRomArea, MEMORY_END, maxAddr);
            return;
        }
        if (extra == MEMORY_END)
            Debugger.mems().setRomArea(last_addr, value);
        cli.println();
        Debugger.mems().printRomArea();
    }
    printPrompt();
}

void handleSetBreak(uint32_t value, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        if (Debugger.pins().setBreakPoint(value)) {
            cli.print("Set");
        } else {
            cli.print("Full");
        }
        Debugger.pins().printBreakPoints();
    }
    printPrompt();
}

void handleClearBreak(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        if (str_buffer[0]) {
            const auto index = atoi(str_buffer);
            if (Debugger.pins().clearBreakPoint(index)) {
                cli.print(" Clear");
                Debugger.pins().printBreakPoints();
            }
        }
    }
    printPrompt();
}

void handleWriteIdentity(char *line, uintptr_t extra, State state) {
    if (state != State::CLI_CANCEL) {
        cli.println();
        if (line[0] == '+')
            Target::writeIdentity(line + 1);
    }
    printPrompt();
}

}  // namespace

void Debugger::exec(char c) {
    const auto maxAddr = mems().maxAddr();
    switch (c) {
    case 'R':
        cli.println("Reset");
        target().reset();
        if (_verbose)
            target().printCycles();
        goto regs;
    case 'd':
        cli.print("Dump? ");
        cli.readHex(handleDump, DUMP_ADDR, maxAddr);
        return;
#ifdef WITH_DISASSEMBLER
    case 'D':
        cli.print("Disassemble? ");
        cli.readHex(handleDisassemble, DIS_ADDR, maxAddr);
        return;
#endif
#ifdef WITH_ASSEMBLER
    case 'A':
        cli.print("Assemble? ");
        cli.readHex(handleAssembler, 0, maxAddr);
        return;
#endif
    case 'm':
        cli.print("Memory? ");
        cli.readHex(handleMemory, MEMORY_ADDR, maxAddr);
        return;
    case 'M':
        if (target().printRomArea()) {
            cli.print("  ROM area? ");
            cli.readHex(handleRomArea, MEMORY_ADDR, maxAddr);
            return;
        }
        break;
    case 'B':
        cli.print("Set break? ");
        cli.readHex(handleSetBreak, 0, maxAddr);
        return;
    case 'b':
        if (target().printBreakPoints()) {
            cli.print("Clear break? ");
            cli.readLine(handleClearBreak, 0, str_buffer, sizeof(str_buffer));
            return;
        }
        break;
    case 'r':
        cli.println("Registers");
    regs:
        target().printRegisters();
        target().disassembleNext();
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
        goto regs;
    case 'G':
        cli.println("Go");
        if (target().isOnBreakPoint()) {
            // step over break point
            if (!target().step(false))
                break;
        }
        target().run();
        goto regs;
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
        cli.println();
        target().printDevices();
        cli.print("Io? ");
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

void Debugger::begin(Target &target_) {
    _target = &target_;
    target().begin();
    usage();
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
