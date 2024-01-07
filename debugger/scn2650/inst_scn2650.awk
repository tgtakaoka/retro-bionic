#!/usr/bin/gawk -f
# -*- mode: awk; -*-

BEGIN {
    PRETTY_PRINT = 0;
    GENERATE_TABLE = 1;
}

function pretty_print(opc, nem, opr, len, clk, bus, indir) {
    if (PRETTY_PRINT == 0)
        return;
    printf("%-2s  %-4s  %-7s  %s  %s  %2s  %4s\n",
           opc, nem, opr, len, clk, bus, indir);
}

function generate_TABLE(opc, nem, opr, len, clk, bus, indir) {
    if (GENERATE_TABLE == 0)
        return;
    printf("        %s(%d, %d),  // %s: %-4s %s\n",
           indir == 0 ? "E" : "I",
           len, bus, opc, nem, opr);
}

BEGIN {
    pretty_print("op", "nemo", "opr", "#", "~", "bus", "indir");
    pretty_print("--", "----", "-------", "-", "-", "---", "-----");
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
    bus = $6;
    indir = $7;

    pretty_print(opc, nem, opr, len, clk, bus, indir);
    generate_TABLE(opc, nem, opr, len, clk, bus, indir);
}
