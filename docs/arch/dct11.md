# DCT11

Part of [Development style](../development-style.md): read General first.

**Entry.** A HALT instruction and the HALT input (AI7) push the PS and
the PC and fetch the restart address with PS 340; BPT and a T-bit trace
trap push the same frame and read the vector at 14. **[doc]** The
debugger answers that vector with the restart address and 340, so every
entry parks on the fetch of 173004 with the frame below SP. A BPT's PC
is the pushed PC less 2; a step's is the pushed PC. A step sets the T
bit and restores with `RTT`, which defers the trace trap past the next
instruction. A step onto a BPT or HALT is refused: the trace trap would
still be pending after its own trap. **[code]**

**Mode register.** It is read from DAL15-DAL8 and DAL1-DAL0 while #BCLR
is low, at power-up and on every `RESET` instruction, so the board drives
it at both: 153377, start 173000, 16-bit, static memory, normal
read/write so R/#WHB and R/#WLB are valid at the #RAS fall. **[doc]**

**Read before write.** Every write is preceded by a read of the same
address, except the pushes of a trap or interrupt. **[doc]** That read
would take a received character from the DL11's RBUF, so a
device read returns `peek()` and takes effect only if the next
transaction is not a write to the same word. **[code]**

**Console.** The DCT11 and DCJ11 each have a DL11 at 177560, vectors 60 and 64. A
request comes as DONE or READY rises with its interrupt enable set, or
as the enable is set, and the acknowledge that reads its vector drops
it: a level request would interrupt for as long as a program leaves the
transmitter's enable set. Received characters come a 9600 baud character
time apart: all at once, pasted text overruns a program that polls.
**[doc][code][bench]**

**Interrupts.** The DL11 requests level 4 with #VEC, and its vector is
driven on DAL7-DAL0 at the IACK. The AI lines must not change while PI
is high, so a request a device raises inside a transaction is driven
after it. **[doc][code]**

**Clock.** XTL1 is bit-banged: it needs T > 133 ns and each half > 60 ns
at the pin (User's Guide 5.4.2), and the board's 48/48 ns delays give
halves of 64 and 88 ns at least. **[doc]** A high half under about 70 ns
at the connector leaves #RAS asserted through the transactions that
follow an IACK; nothing in the manuals defines such a bus, so it is the
clock to fix, not a transaction to serve. **[bench]** The
clock rests high and the pins are read at the end of a period: #RAS is
seen in the period it falls in with the address still on DAL, and a read's
lookup rides the period that brings #CAS. **[bench]**

- The PS after power-up, as `MFPS` reads it.
