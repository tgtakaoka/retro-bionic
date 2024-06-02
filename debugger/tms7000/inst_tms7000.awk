#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

BEGIN {
    if (PRETTY_PRINT) {
        pretty_print("op", "mnemo", "operand",    "#", "bus");
        pretty_print("--", "-----", "----------", "-", "-");
    }
    if (GENERATE_TABLE) {
        printf("constexpr uint8_t INST_TABLE[] = {\n");
    }
}

function pretty_print(opc, mnemo, operand, len, bus) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-10s  %s  %s\n",
           opc, mnemo, operand, len, bus);
}

function generate_ENTRY(opc, mnemo, operand, len, bus) {
    if (GENERATE_TABLE == 0)
        return;
    if (bus == "-")
        bus = "0";
    if (len == "-") {
        printf("        0,  // %s\n",  opc);
    } else {
        printf("        E(%d, %d),  // %s: %-5s %s\n",
               len, bus, opc, mnemo, operand);
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

    pretty_print(opc, mne, opr, len, bus);
    generate_ENTRY(opc, mne, opr, len, bus);
}
