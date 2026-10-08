# Bus cycle sequence legend

Every target's opcode tables (`debugger/<arch>/*.txt`) describe what an
instruction does on the bus as a sequence of the tokens below, one
character each. `MatchWalker` (`match_walker.h`) walks a captured ring of
bus cycles with them, and each target's `Arch` decodes an instruction into
its sequence, length, target and effective address.

A token stands for one cycle on an 8-bit bus. On a wider bus, data tokens
count bytes: a data cycle that moves two bytes meets two tokens where the
second goes on from the first (`Rr`, `Ww`, `II`, `OO`), else one with its
first byte. A word below the last write is below it by the word.

## Instruction bytes

| Token | Cycle |
|---|---|
| `1` | the read of the instruction's first byte, at its address; no cycle when the instruction before fetched it ahead |
| `2` | a read of its next byte, where the fetch stream has got to |
| `n` | a read where the fetch stream has got to that doesn't advance it: a dummy read of the next instruction |

The instruction's length is the count of `1` and `2`, plus whatever its
decoder adds (prefixes, a DDIR's immediate bytes).

## The next instruction's fetch

The last of these tokens in a sequence names where the next instruction is
fetched. At its end, it looks at that cycle without taking it: the cycle is
the next instruction's `1`. Before the end, the CPU fetched the next
instruction ahead: the cycle is this one's, and the next instruction's
`1` takes none. A sequence without any goes on wherever the next read is.

| Token | Fetch |
|---|---|
| `N` | at the next address, after the instruction |
| `J` | at the target the decoder worked out |
| `P` | at the value the instruction's reads put together, plus the CPU's bias: a return, a vector |
| `?` | anywhere |

A conditional transfer is two alternatives: `…J@…N`.

## A prefetch queue

| Token | Meaning |
|---|---|
| `~` | from here the CPU's queue may fetch ahead: before each later token, a read where the fetch stream has got to |

What the queue holds, up to the CPU's size for it, are the next
instructions' first bytes: their `1` and `2` take no cycle. The next
instruction's `N` is then the queue's head; a `J`, `P` or `?` loses what
it holds. Where the ring ends inside an instruction that took nothing of
its own but its bytes, it isn't walked.

A fetch that brings a word gives an instruction byte token its byte, and
the queue the rest.

Where a CPU stalls, it may fetch again the last byte the stream fetched,
anywhere in an instruction. The ring may also begin inside an instruction
whose first bytes were fetched before it, as far back as the CPU says.
Where the ring's first fetches may be the queue's, its first instructions
count only from one that moves data, transfers or takes an interrupt.

## Data

| Token | Cycle |
|---|---|
| `R` | a memory read at any address; it starts a value |
| `r` | a memory read at the address after the last read; it continues the value |
| `W` | a memory write: the first anywhere, a later one at the address below the last write, or at a read's address |
| `w` | a memory write at the address after the last write |
| `A` | a memory read at the effective address the decoder worked out; it starts a value |
| `B` | a memory write at that effective address |
| `V` | a read in the CPU's vector table; it starts a value |
| `I` `O` | an I/O read, an I/O write |
| `X` | any memory read, taken as is |
| `x` | the CPU's dummy read |
| `!` | an interrupt acknowledge |
| `h` | a halt cycle |
| `-` | a cycle with no valid address |

A value assembles little-endian, or big-endian on a CPU that says so.

## Structure

| Token | Meaning |
|---|---|
| `@` | separates alternatives, tried in order; a later instruction that fails sends the walk back to try the next |
| `{…}` | the enclosed tokens, repeated zero or more times; at most 16 characters, not nested |
| `+1` `-1` `+2` `-2` `0` | after a data token in `{…}`: its address steps by that much from the same token's last iteration |
| `.` | after a data token in `{…}`: any address |
| `/` | ends the sequence: what follows is a variant its decoder picks instead |
| `[…]` | each kind's data tokens in order, but the kinds and the enclosed transfer in any, the queue fetching ahead between them; a data cycle meets as many tokens as it moves bytes. A read at the transfer's target is the transfer, or a data token's where the rest can't be done that way |

## Where a walk starts

The walk tries each read of the ring as a start, the earliest first, and
keeps the first that walks to the end; a CPU may limit how far in the
starts are. At the ring's first cycle it also tries each byte a fetch
brings, and an instruction the fetch stream began before the ring, as
far back as the CPU says. A start other than the ring's first cycle, or
one before it, must be confirmed by a next fetch seen inside the ring;
so must the first where the ring's last cycle is a next fetch. Where the
ring's first fetches may be the queue's, its instructions count from the
first that moves data, transfers or takes an interrupt.

Where the ring may begin inside a wait, a CPU's resume sequence, how a
wait ends, is tried first at the ring's first cycle: the wait's cycles,
then the interrupt's vector and the handler's fetch.

A walk gives up after a budget of steps, and where none reaches the
stop, a walk to the ring's end gets one of its own.

## The stop

Where the PC the CPU stopped at is known, a walk that ends there is the
one: at it, or before instructions that go on to it, all but the last of
them fetched, or inside the one at it. Those the stream fetched ran with
no cycles of their own, and are walked, up to the one at the stop. Where
none does, any walk that reaches the ring's end.

An MMU between the PC and the bus is the CPU's: it says which fetches are
at an address, and an instruction's first fetch then says where its page
is for the rest.

## Interrupts

A CPU's interrupt sequence is tried where no instruction fits. It starts
with the cycle that fetched the interrupted opcode: an `X` marks no fetch
there, a `1` marks it and may have been fetched ahead; or with a `~`,
where a queue may have fetched it and more. The ring may end
inside one only after its `V`: before, it is any reads. Its first write
pushes the PC: where the CPU would have gone on, or for a trap the
instruction it aborts.

Placeholders that only a decoder sees, expanded before the walk: hd6309
postbyte `#`, pulls `<`, pushes `>`; Z380 `a`, an address byte a `DDIR`
immediate widens.
