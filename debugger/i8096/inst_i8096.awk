#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

BEGIN {
    if (PRETTY_PRINT) {
        pretty_print("op", "mnemo", "operands", "#",   "~");
        pretty_print("--", "-----", "--------", "---", "-");
    }
    if (GENERATE_TABLE)
        HEADER = 1;
}

function pretty_print(opc, mnemo, operand, len, bus) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-5s  %-8s  %-3s  %s\n", opc, mnemo, operand, len, bus);
}

function generate_ENTRY(opc, mnemo, operand, len, bus,  var) {
    if (GENERATE_TABLE == 0)
        return;
    if (bus == "-")
        bus = "0";
    var = 0;
    if (sub("/[0-9]", "", len))
        var = 1;
    if (len == "-") {
        printf("        0,  // %s\n",  opc);
    } else {
        printf("        E(%d, %d, %d),  // %s: %-5s %s\n",
               len, var, bus, opc, mnemo, operand);
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

    if (HEADER) {
        if (match(FILENAME, /i8096-(PAGE[0-9A-F]+).txt/, a) == 0) {
            printf("Unknown file: %s\n", FILENAME) > "/dev/stderr"
            exit;
        }
        printf("constexpr uint8_t %s_TABLE[] = {\n", a[1]);
        HEADER = 0;
    }

    pretty_print(opc, mne, opr, len, bus);
    generate_ENTRY(opc, mne, opr, len, bus);
}
