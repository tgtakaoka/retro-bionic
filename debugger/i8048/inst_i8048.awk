#!/usr/bin/gawk -f
# -*- mode: awk; -*-

# FLAGS
# 1: i8039
# 2: i80C39
# 4: MSM80C39
# 8: x80x48

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

function pretty_print(opc, mne, opr, len, cyc, flg) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-7s  %s  %s  %s\n", opc, mne, opr, len, cyc, flg);
}

function generate_TABLE(opc, mne, opr, len, cyc, flg) {
    if (GENERATE_TABLE == 0)
        return;
    if (mne == "-") {
        printf("        0,          // %s:\n", opc);
    } else {
        printf("        E(%d, %d, %d),  // %s: %-5s %s\n", len, cyc, flg, opc, mne, opr);
    }
}
BEGIN {
    pretty_print("op", "mnemo", "operand", "#", "~", "F");
    pretty_print("--", "-----", "-------", "-", "-", "-");
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
    mne = $2;
    opr = $3;
    len = $4;
    cyc = $5;
    flg = $6;

    pretty_print(opc, mne, opr, len, cyc, flg);
    generate_TABLE(opc, mne, opr, len, cyc, flg);
}
