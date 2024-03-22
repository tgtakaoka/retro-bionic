#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# 1: instruction code (next=&1)
# 2: instruction operands (disp, high(addr), ++next)
# 3: instruction operands (low(addr), ++next)
# 4: instruction operands (low(addr), ++next)
# 5: instruction operands (low(addr), ++next)
# Y: variable instruction operands cycles (1~9) based on post byte
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
# P: variable pull (0~12) based on post byte (PSHx)
# Q: variable push (0~12) based on post byte (PULx)
# T: repeated reads and writes (R/R+/R-:x:W/W+/W-, TFM)
# N: next instruction read from |next|
# J: next instruction read from |addr|
# j: next instruction read from |next| or |next|+disp
# k: next instruction read from |next| or |next|+disp16
# i: next instruction read from unknown
# V: read 1 byte from address FFF8-FFFE (high(addr))
# v: read 1 byte from address V+1 (low(addr))
# X: any valid read
# x: irrelevant read access to $FFFF
# Z: repeated irrelevant read access to $FFFF
# n: non VMA access
# @: separator for alternative sequence
#
# Interrupt sequence
# 1:X:x:w:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x:J@1:X:x:w:W:W:x:V:v:x:J/1:X:x:w:W:W:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x:J

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_CYCLES = 0;
    GENERATE_TABLE = 1;

    # GENERATE_CYCLES=1
    CYCLES[1]="Nx";
    CYCLES[2]="Nxx/Nx";
    CYCLES[3]="Nxxx/Nxx";
    CYCLES[4]="N";
    CYCLES[5]="3x";
    CYCLES[6]="34xxx/34xx";
    CYCLES[7]="NXxxx/NXx";
    CYCLES[8]="34xxxx/34xx";
    CYCLES[9]="NRrx";
    CYCLES[10]="NxxxRrx/NxxRrx";
    CYCLES[11]="NxxxRrx/NxxRrX";
    CYCLES[12]="NxRrx";
    CYCLES[13]="3xRrx";
    CYCLES[14]="34xxxRrx/34xxRrx";
    CYCLES[15]="NXxxxRrx/NXxRrx";
    CYCLES[16]="34xxxxRrx/34xxRrx";
    CYCLES[17]="34xRrx/34Rrx";
    CYCLES[18]="34x";
    CYCLES[19]="34xRrx";
    CYCLES[20]="12xRxWN/12RxWN";
    CYCLES[21]="123RxWN";
    CYCLES[22]="123RN";
    CYCLES[23]="12xRxxN/12RxN";
    CYCLES[24]="12xi/12i";
    CYCLES[25]="1NN/1N";
    CYCLES[26]="1NXXN";
    CYCLES[27]="1NxxN";
    CYCLES[28]="123xxk/123xk";
    CYCLES[29]="123xxXxwWk/123xxwWk";
    CYCLES[30]="12NN";
    CYCLES[31]="12xxxxxxN/12xxxN";
    CYCLES[32]="12xxxxN/12xxN";
    CYCLES[33]="12xj";
    CYCLES[34]="12xN";
    CYCLES[35]="12YxN";
    CYCLES[36]="12xxXQN/12xXQN";
    CYCLES[37]="12xxPXi/12xPXi";
    CYCLES[38]="1NRrxJ/1NRrJ";
    CYCLES[39]="1NxN/1N";
    CYCLES[40]="1NRrrrrrrrrrrrXJ@1NRrrXJ/1NRrrrrrrrrrrrrrXJ";
    CYCLES[41]="12NxwWWWWWWWWWWWN/12NxwWWWWWWWWWWWWWN";
    CYCLES[42]="1NZN";
    CYCLES[43]="1NxwWWWWWWWWWWWxVvxJ/1NxwWWWWWWWWWWWWWXVvxJ";
    CYCLES[44]="12YRxWN";
    CYCLES[45]="123YRxWN";
    CYCLES[46]="123YRN";
    CYCLES[47]="12YRxxN/12YRxN";
    CYCLES[48]="12Yi";
    CYCLES[49]="123xAxBN/123RxWN";
    CYCLES[50]="1234AxBN";
    CYCLES[51]="1234AN";
    CYCLES[52]="123xAxxN/123RxN";
    CYCLES[53]="123xJ/123J";
    CYCLES[54]="12N";
    CYCLES[55]="123xN/123N";
    CYCLES[56]="12xXxwWj/12xxwWj";
    CYCLES[57]="123N";
    CYCLES[58]="12xRN/12RN";
    CYCLES[59]="12xRrxN/12RrN";
    CYCLES[60]="12xWN/12WN";
    CYCLES[61]="12xXxwWi/12xXwWi";
    CYCLES[62]="12xRrN/12RrN";
    CYCLES[63]="12xWwN/12WwN";
    CYCLES[64]="12YRN";
    CYCLES[65]="12YRrxN/12YRrN";
    CYCLES[66]="12YWN";
    CYCLES[67]="12YXxwWi/12YXwWi";
    CYCLES[68]="12YRrN";
    CYCLES[69]="12YWwN";
    CYCLES[70]="123xAN/123AN";
    CYCLES[71]="123xAaxN/123AaN";
    CYCLES[72]="123xWN/123WN";
    CYCLES[73]="123xAxwWJ/123xAwWJ";
    CYCLES[74]="123xAaN/123AaN";
    CYCLES[75]="123xWwN/123WwN";
    CYCLES[76]="12345N";
    CYCLES[77]="123xBN/123BN";
    CYCLES[78]="123xBbN/123BbN";
    CYCLES[79]="123xN";
    CYCLES[80]="123xN@123xxk/123xk";
    CYCLES[81]="1NxwWN";
    CYCLES[82]="1NxRrN";
    CYCLES[83]="12xRrrrN/12RrrrN";
    CYCLES[84]="12xWwwwN/12WwwwN";
    CYCLES[85]="12YRrrrN";
    CYCLES[86]="12YWwwwN";
    CYCLES[87]="123xAaaaN/123AaaaN";
    CYCLES[88]="123xBbbbN/123BbbbN";
    CYCLES[89]="123xRxN/123RxN";
    CYCLES[90]="123xRxWN/123RxWN";
    CYCLES[91]="12xxxTN";
    CYCLES[92]="12XxN";
    CYCLES[93]="12NZN";
    CYCLES[94]="123NZN";
    CYCLES[95]="12xRZN";
    CYCLES[96]="12xRrZN";
    CYCLES[97]="12YRZN";
    CYCLES[98]="12YRrZN";
    CYCLES[99]="123xRZN";
    CYCLES[100]="123xRrZN";

    if (GENERATE_CYCLES == 1)
        delete CYCLES;
}

function print_number(n, len, num, rem) {
    len = match(n, /^[0-9]+/);
    if (len == 0) {
        printf("%2s    ", n);
    } else if (length(n) == RLENGTH) {
        printf("%2d    ", n);
    } else {
        num = substr(n, RSTART, RLENGTH);
        rem = length(n) - RLENGTH;
        printf("%2d%s", num, substr(n, RLENGTH+1));
        for (num = rem; num <= 3; ++num)
            printf(" ");
    }
}

function pretty_print(opc, nem, opr, clk, len, seq) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-9s  %-5s  ", opc, nem, opr);
    print_number(clk);
    printf(" ");
    print_number(len);
    printf(" %s\n", seq);
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

function generate_ENTRY(opc, nem, opr, clk, len, seq, cyc) {
    if (GENERATE_TABLE == 0)
        return;
    cyc = seq;
    gsub(":", "", cyc);
    printf("        %d,   // %s: %-9s %-5s ",
           CYCLES_index(cyc), opc, nem, opr);
    print_number(clk);
    print_number(len);
    printf("%s\n", seq);
}

BEGIN {
    pretty_print("op", "nemo", "oper",  " ~", " #", "sequence");
    pretty_print("--", "----", "-----", "--", "--", "--------");
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    nem = $2;
    opr = $3;
    clk = $4;
    len = $5;
    seq = ($6 == "") ? "-" : $6;

    if (GENERATE_TABLE && opc == "00") {
        if (match(FILENAME, /hd6309-([A-Z0-9]+).txt/, a) == 0) {
            printf("Unknown file: %s\n", FILENAME) > "/dev/stderr"
            exit;
        }
        if (FILENAME == "hd6309-IX.txt")
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
