#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;

    BITS[0] = 0x80;
    BITS[1] = 0x40;
    BITS[2] = 0x20;
    BITS[3] = 0x10;
    BITS[4] = 0x08;
    BITS[5] = 0x04;
    BITS[6] = 0x02;
    BITS[7] = 0x01;
}

function pretty_print(opc, mne, opr, clk, len) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-7s  %2s  %s\n", opc, mne, opr, clk, len);
}

function generate_ENTRY(opc, len,  num, pos, bit) {
    if (GENERATE_TABLE == 0)
        return;
    num = strtonum("0x" opc);
    if (len != 0) {
        pos = int(num / 8);
        bit = BITS[num % 8];
        VALID[pos] = VALID[pos] + bit;
    }
}

BEGIN {
    pretty_print("op", "mnemo", "operand", " ~", "#");
    pretty_print("--", "-----", "-------", "--", "-");

    if (GENERATE_TABLE)
        printf("constexpr uint8_t INST_TABLE[] = {\n");
}
END {
    if (GENERATE_TABLE) {
        for (i = 0; i < 256 / 8; ++i) {
            if (i % 8 == 0)
                sep = "        ";
            printf("%s0x%02X", sep, VALID[i]);
            sep = ", ";
            if (i % 8 == 7)
                printf(", //  %02X~%02X\n", int(i/8)*8*8, i*8+7);
        }
        printf("};\n");
    }
}

$1 !~ /[0-9A-F][0-9A-F]/ { next; }
$1 ~ /[0-9A-F][0-9A-F]/ {
    opc = $1;
    mne = $2;
    opr = $3;
    clk = $4;
    len = $5;

    pretty_print(opc, mne, opr, clk, len);
    generate_ENTRY(opc, len);
}
