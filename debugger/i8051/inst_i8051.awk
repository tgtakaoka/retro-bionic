#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

function pretty_print(opc, mne, opr, len, bus) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-10s  %s  %s\n", opc, mne, opr, len, bus);
}

function generate_TABLE(opc, mne, opr, len, bus) {
    if (GENERATE_TABLE == 0)
        return;
    if (mne == "-") {
        printf("        0,          // %s:\n", opc);
    } else {
        printf("        E(%d, %d),  // %s: %-5s %s\n", len, bus, opc, mne, opr);
    }
}

BEGIN {
    pretty_print("op", "mnemo", "operand   ", "#", "~");
    pretty_print("--", "-----", "----------", "-", "-");
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
    bus = $5;

    pretty_print(opc, mne, opr, len, bus);
    generate_TABLE(opc, mne, opr, len, bus);
}
