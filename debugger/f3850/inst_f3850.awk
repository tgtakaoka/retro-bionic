#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

function pretty_print(opc, nem, opr, len, clk) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-4s  %-5s  %s  %s\n", opc, nem, opr, len, clk);
}

function generate_TABLE(op, nem, opr, len, clk) {
    if (GENERATE_TABLE == 0)
        return;
    printf("        E(%s, %d),  // %s: %-4s %s\n", len, clk,
               op, nem, opr);
}

BEGIN {
    pretty_print("op", "nemo", "opr",   "#", "~");
    pretty_print("--", "----", "-----", "-", "-");
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
    clk = $5;

    pretty_print(opc, nem, opr, len, clk);
    generate_TABLE(opc, nem, opr, len, clk);
}
