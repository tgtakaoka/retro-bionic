#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# 0: prefetched instruction code
# R: read 1 byte (high(addr))
# r: read 1 byte at address R+1 or r+1 (low(addr))
# W: write 1 byte, equals to R if exists or w-1
# w: write 1 byte at address W+1
# A: read 1 byte at address |addr|
# a: read 1 byte ar A+1
# D: read 1 byte from direct page (00|disp|)
# d: read 1 byte at address (D+1)
# B: write 1 byte at address addr
# b: write 1 byte ar B+1
# E: write 1 byte to direct page (00xx), equals to D if exists
# e: write 1 byte at address E+1
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# X: any valid read
# x: irrelevant read access to $FFFF
# n: non VMA access
#
# Interrupt sequence
# 1:X:x:w:W:W:W:W:W:W:V:v:J

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;

    # GENERATE_CYCLES=1
    CYCLES[1]="0N";
    CYCLES[2]="0Nx";
    CYCLES[3]="0NxxN";
    CYCLES[4]="02xj";
    CYCLES[5]="0NxR";
    CYCLES[6]="0NxWN";
    CYCLES[7]="0NxRr";
    CYCLES[8]="0NxRrJ";
    CYCLES[9]="0NxRrrrrrrJ";
    CYCLES[10]="0NxwWN";
    CYCLES[11]="0Nxxxxxx";
    CYCLES[12]="0NxwWWWWWW";
    CYCLES[13]="0NxwWWWWWWVvJ";
    CYCLES[14]="02xRxWN";
    CYCLES[15]="023xRxWN";
    CYCLES[16]="023xRN";
    CYCLES[17]="02xRN";
    CYCLES[18]="02xi";
    CYCLES[19]="02xRWN";
    CYCLES[20]="023AxBN";
    CYCLES[21]="023DxEN";
    CYCLES[22]="023DN";
    CYCLES[23]="023AN";
    CYCLES[24]="023J";
    CYCLES[25]="023ABN";
    CYCLES[26]="02N";
    CYCLES[27]="023N";
    CYCLES[28]="02xwWj";
    CYCLES[29]="02DN";
    CYCLES[30]="02DdN";
    CYCLES[31]="02EN";
    CYCLES[32]="02xwWi";
    CYCLES[33]="02EeN";
    CYCLES[34]="02xRrN";
    CYCLES[35]="02xWN";
    CYCLES[36]="02xWwN";
    CYCLES[37]="023AaN";
    CYCLES[38]="023BN";
    CYCLES[39]="023xwWJ";
    CYCLES[40]="023BbN";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

function pretty_print(opc, nem, opr, clk, len, seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-4s  %-4s  %2s  %s  %s\n", opc, nem, opr, clk, len, seq);
}

function inst_len(seq,  len, tmp, i, c) {
    len = 0;
    if (seq != "-") {
        split(seq, tmp, ":");
        for (i in tmp) {
            c = tmp[i];
            if (c == "0")
                len = 1;
            if (c ~ /[123]/ && c >= len)
                len = c;
        }
    }
    return len;
}

function inst_len_cyc(cyc, len, tmp, i, c) {
    len = 0;
    split(cyc, tmp, "");
    for (i in tmp) {
        c = tmp[i];
        if (c ~ /[123]/ && c >= len)
            len = c;
    }
    return len;
}

function clock(seq,  clk, tmp) {
    clk = 0;
    if (seq != "-")
        clk = split(seq, tmp, ":") - 1;
    return clk;
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
    if (seq != "-")  {
        cyc = seq;
        gsub(":", "", cyc);
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

function generate_TABLE() {
    if (GENERATE_TABLE == 0)
        return;
    printf("constexpr const char *const SEQUENCES[/*seq*/] = {\n");
    printf("        \"%s\",  // %2d\n", "", 0);
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        \"%s\",  // %2d\n", cyc, c);
    }
    printf("};\n\n");

    # printf("constexpr uint8_t INST_LENS[] = {\n");
    # printf("        %d, // %2d: %s\n", 0, 0, "-");
    # for (c in CYCLES) {
    #     cyc = CYCLES[c];
    #     printf("        %d, // %2d: %s\n", inst_len_cyc(cyc), c, cyc);
    # }
    # printf("};\n\n");

    # printf("constexpr uint8_t CLOCKS[] = {\n");
    # printf("        %d, // %2d: %s\n", 0, 0, "-");
    # for (c in CYCLES) {
    #     cyc = CYCLES[c];
    #     printf("        %d, // %2d: %s\n", length(cyc), c, cyc);
    # }
    # printf("};\n\n");
}

function generate_ENTRY(opc, nem, opr, clk, len, seq, cyc) {
    if (GENERATE_TABLE == 0)
        return;
    cyc = seq;
    gsub(":", "", cyc);
    printf("        %d,   // %s: %-4s %-4s  %-2s:%s  %s\n",
           CYCLES_index(cyc), opc, nem, opr, clk, len, seq);
}

BEGIN {
    pretty_print("op", "nemo", "oper", " ~", "#", "sequence");
    pretty_print("--", "----", "----", "--", "-", "--------");

    if (GENERATE_TABLE) {
        generate_TABLE();
        printf("constexpr uint8_t INST_TABLE[] = {\n");
    }
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
    clk = $4;
    len = $5;
    seq = $6;

    if (inst_len(seq) != len) {
        printf("@@ instruction length (#) mismatches with sequence\n");
        print $0;
        exit;
    }
    if (clock(seq) != clk) {
        printf("@@ instruction clock (~) mismatches with sequence\n");
        print $0;
        exit;
    }

    pretty_print(opc, nem, opr, clk, len, seq);
    generate_CYCLES(seq);
    generate_ENTRY(opc, nem, opr, clk, len, seq);
}
