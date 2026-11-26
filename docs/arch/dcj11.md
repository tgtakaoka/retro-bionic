# DCJ11

Part of [Development style](../development-style.md): read General first.

**Bus.** A cycle is four CLK1 periods, T0-T3; a stretched one adds T4
pairs until #CONT, sampled at T4's falling edges, ends it, and #SCTL
rises three edges after the sample. The address, AIO3-AIO0 and BS1-BS0
are valid while #ALE is low: DAL21-DAL16, BS and AIO reach the Teensy
through two 74HCS153s on P30-P33, selected one select bit at a time,
and resting at AIO, so a NIO costs one port read. **[doc][bench]**

**Clock.** The DCJ11 is static, and CLK1 is bit-banged: the debugger
makes every edge, reset, step and run alike, from one script per cycle,
counting from the fall #ALE falls after. The address is read with the
clock parked after the next rise, where #ALE is low with #STRB high, and
interrupts stay on, as one only lengthens a half. A NIO ends at the
eighth edge. #MISS is held high, so a memory read, fetches among them,
is not stretched either: its data goes out in the fourth period's low
half and T3's rise latches it, held 60ns after; the table's 20ns broke
one V6 compile in three, and the data is let go before the CPU drives
the next address. Every other cycle stretches to eight periods, #CONT
at the first sample point: a stretched read's data goes out as #BUFCTL
falls the second time, with #ALE high, and DV latches it before the
sample; a write's is read with #SCTL low; #CONT and DV are held until
#SCTL rises. A read the bus times out takes ten. The halves are coded
24ns and measure 32ns and up on the pin, a period of 72ns and up, in
captures sampled every 8ns, against the table's 28 and 67; coded 28, the
table's minimum, measured 40ns and 96ns in this build, and 32ns and 80ns
in another with the same edge code. **[doc][bench]**

**#ABORT.** The CPU's own, an MMU or address error, falls two edges
before the cycle's #ALE and holds through its first part: it is read
with the address, and a write it covers is not stored. An abort from
outside is taken in the stretch, sampled at T4's falling edges (User's
Guide, 2.6.3). #BUFCTL is low twice in a read: from #STRB's fall to
#ALE's rise, with the address still out, then high again, then low for
the data. Read data driven on the first met the CPU's own on DAL until
the second, and with 177777 against an address of zeros the CPU aborted
the cycle and trapped through 4: most often in the first register
restore after power-up, when the FP11's accumulators hold 000000 177777
000000 177777. Only a stretched read's data waits for the second;
a memory read's goes out after the first, where the address has left
(User's Guide, Figure 3-3). **[doc][bench]**

**Bus timeout.** A read of 760000-760077, the I/O page's first words,
or of 770200-770376, the UNIBUS map's registers, is aborted: the
debugger pulls #ABORT low in the cycle's stretch, as an abort from
outside has to come (DCJ11 User's Guide, 2.6.3), and lets it go as
#SCTL rises. The CPU traps through 4 with the timeout bit in its error
register; Unix sizes its memory by that trap, and 2.11BSD's boot finds
no UNIBUS map, which a read of 0 would turn on in MMR3 and so put every
DMA address through the map. The maintenance register at 177750, which
the DCJ11 leaves to its board and reads with the system-register bank
select (BS 01, as 17777740-17777750 are; User's Guide, Table 2-1), reads
000020, a KDJ11-A module id, which that boot reports as an 11/73. The
rest of the I/O page outside its devices still reads 0. **[doc][code]**

**Disks.** The RK11 and the MSCP controller both interrupt at level 5
on IRQ1: `DevsPdp11::vector(5)` asks each whether it requests, and the
line is let go only when neither does. A transfer is done whole inside
the register access that starts it, the RK11's GO write or the MSCP
poll's service, so the kernel sees it complete as the interrupt comes;
the DCJ11 is static and waits, and the line clock drops the ticks it
would have made meanwhile, so the kernel sees no time pass. `Mscp` is the controller without the
board: it reads and writes the host's memory and drives through
`MscpHost`, so `test/test_mscp` runs 2.11BSD's init handshakes and
packets against arrays on the host, and a flash tests the bus, not the
protocol. `Rq` is its board side: the `Device`, the SD card images and
the interrupt. **[doc][code]**

**Entries and power-up.** The power-up configuration read is answered
with 001017: POK, power-up mode 3 (start at 1000) and a kernel HALT that
traps to 4. #PWRF halts through vector 24, BPT and the trace trap through
14; the debugger answers either with 017000 and 340. A vector read has
no bus code of its own, and a program reads those words as data (the
Unix boot block copies itself through them) or handles BPT itself. The
vector at 14 is answered only in a step, for a BPT at a break point, or
while it points at itself, as the MC6800's SWI vector does to halt to
the debugger; the one at 24 only while #PWRF is held. The one at 4 is
answered when a HALT's fetch comes just before it, past the
configuration read the HALT makes; a kernel's HALT then
stops there, and one in user or supervisor mode, which traps through 4
too, goes on to the program's vector, the debugger pushing its PS and
PC and running an RTI. 017000 is in kernel I page 0, which a kernel
maps whole; page 7 would not do, as 2.11BSD maps its overlays there and
some end below 173004. With the memory management on, the fetch lies
wherever page 0 maps it, so the debugger parks on the first fetch after
the frame, by its bus address, and its save sequence jumps back to
017000. Each save reads
MMR0 and MMR3, and with the mapping on the kernel data space's eight
PARs (KDPAR when MMR3 enables kernel D space, else KIPAR), which carry
the restore's RTT frame below SP and the FP11's loads from 177000 to
their bus addresses; `test/test_pdp11_mmu` checks that translation.
HALT, the #EVENT and
IRQ lines and #MISS come from a 74HCT259 that shares the 153s' selects.
`RegsPdp11` saves the FP11's FPS and AC0-AC5; its FD and FL bits pick
how the disassembler spells FP11 instructions, as single and double (or
short and long integer) forms share opcodes. Without a trap frame it
reads the PS with `MFPS`, a byte, so the J11's PS<15:8> (modes and
register set) needs a per-chip hook. **[doc][code]**

**Memory.** The emulated memory is 22 bits wide, in EXTMEM: a cycle's
word is found by DAL21-DAL00. The I/O page, which BS marks, has only its
devices: elsewhere it reads 0. With the MMU off a program's 160000-177777
is that page, so the CPU starts at 1000, in memory, and a sample's
restart jump at 173000 loads into memory it does not reach. **[code]**

**Bus reset.** The general-purpose write 014, a RESET instruction or
the power-up microroutine, resets the devices. **[doc][code]**

**Line clock.** The KW11-L's LKS is at 177546: MONITOR comes every
60th of a second of a run, and with the interrupt enable set it pulls
#EVENT low, level 6 through vector 100 with no IACK. The CPU answers
with the general-purpose write 100, which lets #EVENT go. A tick missed
while the CPU was halted is not made up. Only the DCJ11 has the clock.
**[doc][code]**

**Switches.** The switch register at 177570 reads 173030 at power-up,
which starts Unix single-user; `I SWR value` sets it, and any setting
but 0 lets the Unix kernel print. **[doc][code]**

**Disk.** The RK11's registers are at 177400, and its request is IRQ1,
level 5 with vector 220: an IACK carries its level on DAL3-DAL0 (DCJ11
User's Guide, Table 3-4), which picks the disk's vector or the
console's. A transfer is done whole in the write that sets GO, with the
CPU held in that cycle's stretch, between the image on the SD card and
the memory at RKBA on the 18-bit bus; READY and the interrupt come at
the next service. A byte write goes into its half of the register's
word; the other devices' high bytes hold nothing a program writes, and
a high byte reaches them at its odd address. A write check compares
the memory with the disk and a read check only reads; a write lock
holds until a drive reset or a bus reset. The sector counter steps on
each read of RKDS. A disk comes with its bootstrap: `L` of a file from
the card mounts the slots of the image of the same name with `.dsk`, as
far as the file reaches, and nothing is mounted before, so no disk is
written by chance. The drives are the shared disk layer's units, one
file handle among them, flushed once a transfer. **[doc][code]**

**Stack limit.** A kernel stack reference below 400, a trap's push
among them, takes a yellow stack trap through vector 4 (DCJ11 User's
Guide, 1.8), and a reset keeps SP (Figure 8-2), so after power-up SP may
lie below it, or in the I/O page, where `G`'s restore cannot stack the
PC and PS and the CPU ends in a HALT loop at 1730xx. So `R` moves such
an SP to 1000, and a step's trace frame, which would trap there, is
refused while SP is below 400. **[doc][bench]**
