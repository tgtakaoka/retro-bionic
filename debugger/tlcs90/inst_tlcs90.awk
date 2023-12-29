#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 0: prefetched instruction
# 2-4: part of instruction or prefix
# 7~9: instruction following prefix
# N: prefetch next instruction
# n: dummy prefetch next instruction
# m: dummy prefetch next instructionfor jump true
# i: new instruction address
# j: 8-bit relative branch (next+2)
# k: 16-bit relative branch (next+3:2)
# J: 16-bit absolute jump address (3:2 or r:R)
# V: next instruction fetch from vector (0000_0000_0xxx_x000B)
# d: dummy cycle, no read nor write
# R: read 1 byte
# W: write 1 byte, the same address if R or E is preceeded
# r: read 1 byte at address R+1 or r+1
# w: write 1 byte at address W+1
# E: read 1 byte from direct page (0FFxxH)
# F: write 1 byte to direct page (0FFxxH)
#
# Interrupt sequence
# 0:n:d:d:V:d:w:W:W:W:V

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;

    # GENERATE_CYCLES=1
    CYCLES[1]="0N";
    CYCLES[2]="02ENW@02N";
    CYCLES[3]="02N";
    CYCLES[4]="023N";
    CYCLES[5]="02j@02N";
    CYCLES[6]="023J";
    CYCLES[7]="023k";
    CYCLES[8]="023wWJ";
    CYCLES[9]="023wWk";
    CYCLES[10]="0nRrJ";
    CYCLES[11]="0nRrrrJ";
    CYCLES[12]="02EN";
    CYCLES[13]="02NF";
    CYCLES[14]="023FN";
    CYCLES[15]="0234NFw";
    CYCLES[16]="02ErN";
    CYCLES[17]="02NFw";
    CYCLES[18]="0NwW";
    CYCLES[19]="0nRrN";
    CYCLES[20]="02ENW";
    CYCLES[21]="02ErNFw";
    CYCLES[22]="02nj@02N";
    CYCLES[23]="02nj";
    CYCLES[24]="0";
    CYCLES[25]="023";
    CYCLES[26]="02";
    CYCLES[27]="0nVwWWWV";
    CYCLES[28]="7nN";
    CYCLES[29]="7N";
    CYCLES[30]="7nRWN";
    CYCLES[31]="7nRWi@7nRWN";
    CYCLES[32]="7nRN";
    CYCLES[33]="7nRi@7nRN";
    CYCLES[34]="78N";
    CYCLES[35]="7nRrJ@7N";
    CYCLES[36]="7nRrJ";
    CYCLES[37]="7RNW";
    CYCLES[38]="7RN";
    CYCLES[39]="7RrN";
    CYCLES[40]="7RrWwN";
    CYCLES[41]="7RrNWw";
    CYCLES[42]="7NW";
    CYCLES[43]="78WN";
    CYCLES[44]="789NWw";
    CYCLES[45]="7NWw";
    CYCLES[46]="78RNW";
    CYCLES[47]="78RN";
    CYCLES[48]="7mi@7N";
    CYCLES[49]="7ni";
    CYCLES[50]="7nwWi@7N";
    CYCLES[51]="7nwWi";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

function bus_cycles(sequence, tmp, c, cycles) {
    if (sequence == "-")
        return "-";
    delete tmp;
    split(sequence, tmp, ":");
    cycles = "";
    for (c in tmp) {
        if (tmp[c] ~ /d/)
            continue;
        cycles = cycles tmp[c];
    }
    return cycles;
}

function bus_length(cycles) {
    if (cycles == "-")
        return 0;
    return length(cycles);
}

function CYCLES_index(sequence, s) {
    for (s in CYCLES) {
        if (CYCLES[s] == sequence)
            return s;
    }
    return 0;
}

function pretty_print(op, nemo, opr, len, bus, true_seq, false_seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s %-4s %-7s %s ", op, nemo, opr, len);
    printf("%-17s ", bus);
    printf("%-21s %s\n", true_seq, false_seq);
}

function add_CYCLES(bus) {
    CYCLES[length(CYCLES)+1] = bus;
    printf("    CYCLES[%d]=\"%s\";\n", length(CYCLES), bus);
}

function append_CYCLES(bus) {
    if (bus_length(bus) != 0 && CYCLES_index(bus) == 0)
        add_CYCLES(bus);
}

function generate_CYCLES(bus) {
    if (GENERATE_CYCLES == 0)
        return 0;
    if (GENERATE_CYCLES == 1)
        append_CYCLES(bus);
}

function generate_TABLE() {
    printf("constexpr const char *const SEQUENCES[/*seq*/] = {\n");
    printf("        \"%s\",  // %2d\n", "", 0);
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        \"%s\",  // %2d\n", cyc, c);
    }
    printf("};\n");
}

function generate_ENTRY(op, nemo, opr, len, bus, true_seq, false_seq, cycle) {
    if (GENERATE_TABLE == 0)
        return;
    cycle = CYCLES_index(bus);
    printf("       %d,  // %s: %-4s %-7s; %s\n",
           cycle, op, nemo, opr, bus);
}

BEGIN {
    pretty_print("op", "mnem", "operand", "#", "bus", "true_seq", "false_seq");
    pretty_print("--", "----", "-------", "-", "---", "--------", "---------");
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    mne = $2;
    opr = $3;
    len = $4;
    true_seq = $6;
    false_seq = $7;

    if (GENERATE_TABLE && opc == "00") {
        if (match(FILENAME, /tlcs90-(PAGE[0-3]).txt/, a) == 0) {
            printf("Unknown file: %s\n", FILENAME) > "/dev/stderr"
            exit;
        }
        if (FILENAME == "tlcs90-PAGE0.txt")
            generate_TABLE();
        page = a[1];
        printf("\nconstexpr uint8_t %s[] = {\n", page);
    }

    true_bus = bus_cycles(true_seq);
    false_bus = bus_cycles(false_seq);
    bus = true_bus;
    if (false_bus != "-")
        bus = bus "@" false_bus;

    pretty_print(opc, mne, opr, len, bus, true_seq, false_seq);
    generate_CYCLES(bus);
    generate_ENTRY(opc, mne, opr, len, bus, true_seq, false_seq);

    if (GENERATE_TABLE && opc == "FF")
        printf("};\n");
}
