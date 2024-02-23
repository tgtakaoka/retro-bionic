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
    CYCLES[20]="123xN";
    CYCLES[21]="123xN@123xxk/123xk";
    CYCLES[22]="12xN";
    CYCLES[23]="1NxwWN";
    CYCLES[24]="1NxRrN";
    CYCLES[25]="1NxwWWWWWWWWWWWxVvxJ/1NxwWWWWWWWWWWWWWXVvxJ";
    CYCLES[26]="1NN/1N";
    CYCLES[27]="123xN/123N";
    CYCLES[28]="123N";
    CYCLES[29]="12xRrxN/12RrN";
    CYCLES[30]="12xRrN/12RrN";
    CYCLES[31]="12xWwN/12WwN";
    CYCLES[32]="12YRrxN/12YRrN";
    CYCLES[33]="12YRrN";
    CYCLES[34]="12YWwN";
    CYCLES[35]="123xAaxN/123AaN";
    CYCLES[36]="123xAaN/123AaN";
    CYCLES[37]="123xBbN/123BbN";
    CYCLES[38]="12xRrrrN/12RrrrN";
    CYCLES[39]="12xWwwwN/12WwwwN";
    CYCLES[40]="12YRrrrN";
    CYCLES[41]="12YWwwwN";
    CYCLES[42]="123xAaaaN/123AaaaN";
    CYCLES[43]="123xBbbbN/123BbbbN";
    CYCLES[44]="123xRxN/123RxN";
    CYCLES[45]="123xRxWN/123RxWN";
    CYCLES[46]="12xxxTN";
    CYCLES[47]="12XxN";
    CYCLES[48]="12N";
    CYCLES[49]="12NZN";
    CYCLES[50]="123NZN";
    CYCLES[51]="12xRN/12RN";
    CYCLES[52]="12xWN/12WN";
    CYCLES[53]="12xRZN";
    CYCLES[54]="12xRrZN";
    CYCLES[55]="12YRN";
    CYCLES[56]="12YWN";
    CYCLES[57]="12YRZN";
    CYCLES[58]="12YRrZN";
    CYCLES[59]="123xAN/123AN";
    CYCLES[60]="123xBN/123BN";
    CYCLES[61]="123xRZN";
    CYCLES[62]="123xRrZN";
    CYCLES[63]="12xRxWN/12RxWN";
    CYCLES[64]="123RxWN";
    CYCLES[65]="123RN";
    CYCLES[66]="12xRxxN/12RxN";
    CYCLES[67]="12xi/12i";
    CYCLES[68]="1NXXN";
    CYCLES[69]="1NxxN";
    CYCLES[70]="123xxk/123xk";
    CYCLES[71]="123xxXxwWk/123xxwWk";
    CYCLES[72]="12NN";
    CYCLES[73]="12xxxxxxN/12xxxN";
    CYCLES[74]="12xxxxN/1NxxN";
    CYCLES[75]="12xj";
    CYCLES[76]="12YxN";
    CYCLES[77]="12xxXQN/12xXQN";
    CYCLES[78]="12xxPXi/12xPXi";
    CYCLES[79]="1NRrxJ/1NRrJ";
    CYCLES[80]="1NxN/1N";
    CYCLES[81]="1NRrrrrrrrrrrrXJ@1NRrrXJ/1NRrrrrrrrrrrrrrXJ";
    CYCLES[82]="12NxwWWWWWWWWWWWN/12NxwWWWWWWWWWWWWWN";
    CYCLES[83]="1NZN";
    CYCLES[84]="12YRxWN";
    CYCLES[85]="123YRxWN";
    CYCLES[86]="123YRN";
    CYCLES[87]="12YRxxN/12YRxN";
    CYCLES[88]="12Yi";
    CYCLES[89]="123xAxBN/123RxWN";
    CYCLES[90]="1234AxBN";
    CYCLES[91]="1234AN";
    CYCLES[92]="123xAxxN/123RxN";
    CYCLES[93]="123xJ/123J";
    CYCLES[94]="12xXxwWj/12xxwWj";
    CYCLES[95]="12xXxwWi/12xXwWi";
    CYCLES[96]="12YXxwWi/12YXwWi";
    CYCLES[97]="123xWN/123WN";
    CYCLES[98]="123xAxwWJ/123xAwWJ";
    CYCLES[99]="123xWwN/123WwN";
    CYCLES[100]="12345N";

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
