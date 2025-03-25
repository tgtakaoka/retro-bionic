#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 0: prefetched instruction code
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# R: read 1 byte (high(addr))
# r: read 1 byte at address R+1 or r+1 (low(addr))
# W: write 1 byte, equals to R if exists or w-1
# w: write 1 byte at address W+1
# A: read 1 byte at address |addr|
# D: read 1 byte from direct page (00|disp|)
# B: write 1 byte at address addr
# E: write 1 byte to direct page (00xx), equals to D if exists
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# d: read 1 byte from any address
#
# Interrupt sequence
# 1:R:d:w:W:W:W:W:V:v:J

function parseSeq(seq, r, i, c) {
    if (seq == "" || seq == "-")
        return "-";
    r = "";
    for (i = 1; i <= length(seq); i++) {
        c = substr(seq, i, 1);
        if (c == ":" || c == "V") {
            ;
        } else if (c == "N" || c == "J" || c == "j" || c == "i") {
            c = "N";
        } else if (c == "W" || c == "w" || c == "B" || c == "E") {
            c = "w";
        } else {
            c = "r";
        }
        r = r c;
    }
    return r;
}

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;

    # GENERATE_CYCLES=1
    CYCLES[1]="rrrrrN";
    CYCLES[2]="rrrwN";
    CYCLES[3]="rrrN";
    CYCLES[4]="rrwwN";
    CYCLES[5]="rrrrwN";
    CYCLES[6]="rrwN";
    CYCLES[7]="rN";
    CYCLES[8]="rrrrN";
    CYCLES[9]="rNrrrr";
    CYCLES[10]="rNNNrrrr";
    CYCLES[11]="rrNrw";
    CYCLES[12]="rNrr";
    CYCLES[13]="rrNr";
    CYCLES[14]="rrNw";
    CYCLES[15]="rNrw";
    CYCLES[16]="rNr";
    CYCLES[17]="rNw";
    CYCLES[18]="rrrrrrrN";
    CYCLES[19]="rrwwwwwVrN";
    CYCLES[20]="rNN";
    CYCLES[21]="rrN";
    CYCLES[22]="rrrwwN";
    CYCLES[23]="rrrNr";
    CYCLES[24]="rrrNw";
    CYCLES[25]="rrrwwrN";
    CYCLES[26]="rrwwrN";
    CYCLES[27]="rrrNrw";
    CYCLES[28]="rrrrrrN";
    CYCLES[29]="rrrrrwN";
    CYCLES[30]="rrrrNr";
    CYCLES[31]="rrrrNw";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

function pretty_print(opc, nem, opr, len, clk, seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-8s  %s  %s  %s\n", opc, nem, opr, len, clk, seq);
}

function extract_number(str) {
    if (str == "-")
        return 0;
    sub(/[/+]+/, "", str);
    return str;
}

function inst_len(seq,  len, tmp, i, c) {
    len = 0;
    if (seq != "-") {
        split(seq, tmp, ":");
        for (i in tmp) {
            c = tmp[i];
            if (c ~ /[12345]/ && c >= len)
                len = c;
        }
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
        cyc = parseSeq(seq);
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
    printf("constexpr const char *const SEQUENCES[/*seq*/] = {\n");
    printf("        \"%s\",  // %2d\n", "", 0);
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        \"%s\",  // %2d\n", cyc, c);
    }
    printf("};\n");
}

function generate_ENTRY(opc, nem, opr, clk, len, seq, cyc) {
    if (GENERATE_TABLE == 0)
        return;
    cyc = parseSeq(seq);
    gsub(":", "", cyc);
    if (clk > 0 && clk != length(cyc) - 1)
        printf("@@@@ ERROR\n");
    printf("        %d,   // %s: %-5s %-8s %s %s %s\n",
           CYCLES_index(cyc), opc, nem, opr, len, clk, seq);
}

BEGIN {
    pretty_print("op", "nemo",  "operand", "#", "~", "bus sequence");
    pretty_print("--", "-----", "-------", "-", "-", "------------");
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    nem = $2;
    opr = $3;
    len = $4;
    clk = $5;
    seq = ($6 == "") ? "-" : $6;

    if (GENERATE_TABLE && opc == "00") {
        if (match(FILENAME, /mc68hc08-(P[A-Z0-9]+).txt/, a) == 0) {
            printf("Unknown file: %s\n", FILENAME) > "/dev/stderr"
            exit;
        }
        if (FILENAME == "mc68hc08-P00.txt")
            generate_TABLE();
        page = a[1];
        printf("\nconstexpr uint8_t %s_TABLE[] = {\n", page);
    }

    # if (inst_len(seq) != extract_number(len)) {
    #     printf("@@ instruction length (#) mismatches with sequence\n");
    #     print $0;
    # }
    # if (clock(seq) != extract_number(clk)) {
    #     printf("@@ instruction clock (~) mismatches with sequence\n");
    #     print $0;
    # }

    pretty_print(opc, nem, opr, clk, len, seq);
    generate_CYCLES(seq);
    generate_ENTRY(opc, nem, opr, clk, len, seq);

    if (GENERATE_TABLE && opc == "FF")
        printf("};\n");
}
