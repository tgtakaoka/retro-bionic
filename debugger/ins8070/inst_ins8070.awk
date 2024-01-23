#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 1: instruction code (next=&1)
# 2: instruction operands (disp, low(addr), ++next)
# 3: instruction operands (high(addr), ++next)
# R: read 1 byte (low(addr))
# r: read 1 byte at address R+1 (high(addr))
# Q: read 1 byte at PC-relative (next+disp+1)
# q: read 1 byet at PC-relative (Q+1)
# W: write 1 byte, equals to R if exists
# w: write 1 byte at address W+1
# X: write 1 byte at PC-relative (next+disp+1), equals to Q if exists
# x: write 1 byet at PC-relative (X+1)
# E: read 1 byte from direct page (FFxx)
# e: read 1 byte at address (E+1)
# F: write 1 byte to direct page (FFxx), equals to E if exists
# f: write 1 byte at address F+1
# V: read 1 byte from address 0020-003E (low(addr))
# v: read 1 byte from address V+1 (high(addr))
# S: read 1+ byte sequencially next address (S+1)
# @: separator for internal RAM access cycles
# N: next sequential instruction address (next)
# n: next unknown instruction
# s: next instruction address of SSM (next or next+2)
# A: 16-bit absolute address (addr)
# B: 8-bit relative branch (next or next+disp)
# P: poped from stack address (addr)

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;

    # GENERATE_CYCLES=1
    CYCLES[1]="1N";
    CYCLES[2]="1wWN@1N";
    CYCLES[3]="1WN@1N";
    CYCLES[4]="1wWVvA@1VvA";
    CYCLES[5]="12wW3A@123A";
    CYCLES[6]="12wW3N@123N";
    CYCLES[7]="123A";
    CYCLES[8]="123N";
    CYCLES[9]="12B";
    CYCLES[10]="1Ss@1s";
    CYCLES[11]="1RN@1N";
    CYCLES[12]="12N";
    CYCLES[13]="1RrN@1N";
    CYCLES[14]="1n";
    CYCLES[15]="1RrP@1n";
    CYCLES[16]="12n";
    CYCLES[17]="12QqN@12N";
    CYCLES[18]="12RrN@12N";
    CYCLES[19]="12EeN@12N";
    CYCLES[20]="12XxN@12N";
    CYCLES[21]="12WwN@12N";
    CYCLES[22]="12FfN@12N";
    CYCLES[23]="12QXN@12N";
    CYCLES[24]="12RWN@12N";
    CYCLES[25]="12EFN@12N";
    CYCLES[26]="12QN@12N";
    CYCLES[27]="12RN@12N";
    CYCLES[28]="12EN@12N";
    CYCLES[29]="12XN@12N";
    CYCLES[30]="12WN@12N";
    CYCLES[31]="12FN@12N";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

function pretty_print(opc, nem, opr, mod, clk, len, cyc, seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s %-4s %-8s %-4s %-6s %s ", opc, nem, opr, mod, clk, len);
    printf("%-11s %s\n", cyc, seq);
}

function inst_len(cyc, s, i, c) {
    i = 0;
    for (s = 1; s <= length(cyc); ++s) {
        c = substr(cyc, s, 1);
        if (c == "@")
            break;
        if (c ~ /[123]/)
            ++i;
    }
    return i;
}

function bus_cycles(cyc, s, i, c) {
    i = 0;
    for (s = 1; s <= length(cyc); ++s) {
        c = substr(cyc, s, 1);
        if (c == "@")
            break;
        if (c ~ /[123RrQqWwXxEeFfVvS]/)
            ++i;
    }
    return i;
}

function external_cycles(cyc, s, i, c) {
    i = 0;
    for (s = 1; s <= length(cyc); ++s) {
        c = substr(cyc, s, 1);
        if (c == "@")
            break;
        if (c ~ /[123Vv]/)
            ++i;
    }
    return i;
}

function seq_cycles(seq, tmp, i, c, cyc, no_bus) {
    if (seq == "-")
        return "-";
    delete tmp;
    split(seq, tmp, ":");
    cyc = "";
    no_bus = "";
    for (i in tmp) {
        c = tmp[i];
        cyc = cyc c;
        if (c ~ /[123VvNnsABP]/) {
            if (c == "P")
                c = "n";
            no_bus = no_bus c;
        }
    }
    if (cyc != no_bus)
        cyc = cyc "@" no_bus;
    return cyc;
}

function seq_length(seq) {
    if (seq == "-")
        return 0;
    return length(seq);
}

function CYCLES_index(cyc, i) {
    for (i in CYCLES) {
        if (CYCLES[i] == cyc)
            return i;
    }
    return 0;
}

function add_CYCLES(cyc, n) {
    n = length(CYCLES);
    CYCLES[++n] = cyc;
    printf("    CYCLES[%d]=\"%s\";\n", n, cyc);
}

function append_CYCLES(seq, cyc) {
    if (seq_length(seq) != 0) {
        cyc = seq_cycles(seq);
        if (CYCLES_index(cyc) == 0) {
            add_CYCLES(cyc);
            return 1;
        }
    }
    return 0;
}

function generate_CYCLES(seq) {
    if (GENERATE_CYCLES == 1)
        append_CYCLES(seq);
}

function generate_TABLE(op, nem, opr, mod, len, cyc, seq) {
    if (GENERATE_TABLE == 0)
        return;
    cycle = CYCLES_index(cyc);
    mode = (mod == "-" ? "M_NO" : "M_"mod);
    if (cycle == 0) {
        printf("        0,          // %s: %-4s %-8s ; %s\n",
               op, nem, opr, seq);
    } else {
        printf("        E(%s, %d),  // %s: %-4s %-8s ; %s\n", mode, cycle,
               op, nem, opr, seq);
    }
}

function generate_INSTLENS() {
    if (GENERATE_TABLE == 0)
        return;
    printf("constexpr const char *const BUS_SEQUENCES[/*seq*/] = {\n");
    printf("        \"%s\",  // %2d\n", "", 0);
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        \"%s\",  // %2d\n", cyc, c);
    }
    printf("};\n\n");

    printf("constexpr uint8_t INST_LENS[] = {\n");
    printf("        %d, // %2d: %s\n", 0, 0, "-");
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        %d, // %2d: %s\n", inst_len(cyc), c, cyc);
    }
    printf("};\n\n");
}

function generate_BUSCYCLES() {
    if (GENERATE_TABLE == 0)
        return;
    printf("constexpr uint8_t BUS_CYCLES[] = {\n");
    printf("        %d, // %2d: %s\n", 0, 0, "-");
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        %d, // %2d: %s\n", bus_cycles(cyc), c, cyc);
    }
    printf("};\n\n");
    printf("constexpr uint8_t EXTERNAL_CYCLES[] = {\n");
    printf("        %d, // %2d: %s\n", 0, 0, "-");
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        %d, // %2d: %s\n", external_cycles(cyc), c, cyc);
    }
    printf("};\n\n");
}

BEGIN {
    pretty_print("op", "nemo", "operand", "mode", "clock", "#",
                 "cycle", "sequence");
    pretty_print("--", "----", "--------", "----", "------", "-",
                 "------", "--------");
    generate_INSTLENS();
    generate_BUSCYCLES();
    if (GENERATE_TABLE)
        printf("constexpr uint8_t INST_TABLE[] = {\n");
}
END {
    if (GENERATE_TABLE)
        printf("};\n");
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    nem = $2;
    opr = $3;
    mod = $4;
    clk = $5;
    len = $6;
    cyc = $7;
    seq = $8;

    if (inst_len(cyc) > 0 && inst_len(cyc) != len) {
        printf("@@ instruction length (#) and cycle are mismatched\n");
        print $0;
        exit;
    }

    pretty_print(opc, nem, opr, mod, clk, len, cyc, seq);
    generate_CYCLES(seq);
    generate_TABLE(opc, nem, opr, mod, len, cyc, seq);
}
