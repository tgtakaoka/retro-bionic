#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 1: instruction code (next=&1+1)
# 2: instruction operands (++next)
# 3: instruction operands (++next)
# 4: instruction operands (++next)
# 5: instruction operands (++next)
# 6: instruction operands (++next)
# 7: instruction operands (++next)
# ~: prefetch bytes (++next)
# B: read byte
# R: read low byte of word
# r: read high byte of word (=&R+1)
# Y: write byte
# W: write low byte of word
# w: write high byte of word (=&W+1)
# V: read low byte of vector
# v: read high byte of vector (=&V+1)
#--- following code invalidate prefetch bytes
# j: fetch instruction from next+8-bit disp
# k: fetch instruction from next+8-bit disp (JBC/JBS/DJNZ)
# K: fetch instruction from next+11-bit disp
# J: fetch instruction from next+16-bit disp
# i: fetch instruction from unknown address
# A: fetch instruction from previous read word
# C: fetch from reset origin (2080H)
#---- separator
# @: separator for conditional
# /: separator for 8/16 bit index
#---- interrupt
# V:v:W:w:A

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;
}

BEGIN {
    # GENERATE_CYCLES=1
    CYCLES[1]="12";
    CYCLES[2]="123";
    CYCLES[3]="12~K";
    CYCLES[4]="12~WwK";
    CYCLES[5]="123~k@123";
    CYCLES[6]="1234";
    CYCLES[7]="12345";
    CYCLES[8]="1234~Rr";
    CYCLES[9]="12345Rr/123456Rr";
    CYCLES[10]="1234~Rr/1234~Rr";
    CYCLES[11]="12345~Rr/123456Rr";
    CYCLES[12]="1234~B";
    CYCLES[13]="12345B/123456B";
    CYCLES[14]="12345~B/123456B";
    CYCLES[15]="123~Rr";
    CYCLES[16]="1234~Rr/12345~Rr";
    CYCLES[17]="123~B";
    CYCLES[18]="1234~B/12345~B";
    CYCLES[19]="123~Ww";
    CYCLES[20]="1234~Ww/12345~Ww";
    CYCLES[21]="123~Y";
    CYCLES[22]="1234~Y/12345~Y";
    CYCLES[23]="12~Ww";
    CYCLES[24]="12~RrWw";
    CYCLES[25]="123~RrWw/1234~RrWw";
    CYCLES[26]="12~Rr";
    CYCLES[27]="12~j@12";
    CYCLES[28]="12~i";
    CYCLES[29]="123~J";
    CYCLES[30]="123~WwJ";
    CYCLES[31]="1~RrA";
    CYCLES[32]="1~Ww";
    CYCLES[33]="1~Rr";
    CYCLES[34]="1~VvWv";
    CYCLES[35]="1";
    CYCLES[36]="1~C";
    CYCLES[37]="123456";
    CYCLES[38]="12345~Rr";
    CYCLES[39]="123456~Rr/1234567Rr";
    CYCLES[40]="12345~B";
    CYCLES[41]="123456~B/1234567B";
    CYCLES[42]="12345~Rr/123456~Rr";
    CYCLES[43]="12345~B/123456~B";
    CYCLES[44]="1234Rr";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

BEGIN {
    if (PRETTY_PRINT) {
        pretty_print("op", "mnemo", "operands", "#",   "~", "sequence");
        pretty_print("--", "-----", "--------", "---", "-", "--------");
    }
    if (GENERATE_TABLE)
        HEADER = "";
}

function pretty_print(opc, mnemo, operand, len, bus, seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-8s  %-3s  %s  %s\n", opc, mnemo, operand, len, bus, seq);
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
    printf("constexpr const char *const SEQUENCES[/*seq*/] = {\n");
    printf("        \"%s\",  // %2d\n", "", 0);
    for (c in CYCLES) {
        cyc = CYCLES[c];
        printf("        \"%s\",  // %2d\n", cyc, c);
    }
    printf("};\n");
}

function generate_ENTRY(opc, mnemo, opr, len, seq) {
    if (GENERATE_TABLE == 0)
        return;
    cyc = seq;
    gsub(":", "", cyc);
    if (len == "-") {
        printf("        {0, 0},  // %s\n",  opc);
    } else {
        printf("        {%d, %d},  // %s: %-5s %-8s %s\n",
               len, CYCLES_index(cyc), opc, mnemo, opr, seq);
    }
}

END {
    if (GENERATE_TABLE)
        printf("};\n");
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    mne = $2;
    opr = $3;
    len = $4;
    bus = $5;
    seq = $6
    if (seq == "") seq = "-"

    if (GENERATE_TABLE && HEADER == "")
        generate_TABLE();

    if (HEADER != FILENAME) {
        if (match(FILENAME, /i8096-(PAGE[0-9A-F]+).txt/, a) == 0) {
            printf("Unknown file: %s\n", FILENAME) > "/dev/stderr"
            exit;
        }
        HEADER = FILENAME;
        if (GENERATE_TABLE)
            printf("\nconstexpr InstI8096::Table %s_TABLE[] = {\n", a[1]);
    }

    pretty_print(opc, mne, opr, len, bus, seq);
    generate_CYCLES(seq);
    generate_ENTRY(opc, mne, opr, len, seq);

    if (GENERATE_TABLE && opc == "FF")
        printf("};\n");
}
