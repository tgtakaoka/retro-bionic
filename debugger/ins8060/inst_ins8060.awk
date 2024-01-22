#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

function pretty_print(opc, nem, opr, len, rcy, wcy) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-4s  %-6s  %s  %s  %s\n",
           opc, nem, opr, len, rcy, wcy);
}

function generate_TABLE(opc, nem, opr, len, rcy, wcy) {
    if (GENERATE_TABLE == 0)
        return;
    printf("        %d,  // %s: %-4s %s\n",
           len, opc, nem, opr);
}

BEGIN {
    pretty_print("op", "nemo", "opr", "#", "r", "w");
    pretty_print("--", "----", "------", "-", "-", "-");
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
    nem = $2;
    opr = $3;
    len = $4;
    rcy = $5;
    wcy = $6;

    pretty_print(opc, nem, opr, len, rcy, wcy);
    generate_TABLE(opc, nem, opr, len, rcy, wcy);
}
