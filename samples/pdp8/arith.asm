;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
ACIA    = 07
        include "mc6850.inc"

        *ORG_RESET-1
        initialize
        jmp     I .-1

        *PAGE0
I_putchar,      putchar
I_putspace,     putspace
I_newline,      newline
I_add24,        add24
I_sub24,        sub24
I_neg24,        neg24
I_mul24,        mul24
I_div24,        div24
I_umul24,       umul24
I_udiv16,       udiv16
I_print_int24,  print_int24
I_set_VAVB,     set_VAVB
I_expr,         expr
I_answer,       answer
I_comp,         comp

arith_ptr,      0       / arithmetic routine work
R0,                     / 24-bit arithmetic accumulator
R0L,            0
R0H,            0
R1,                     / 24-bit arithmetic operand 1
R1L,            0
R1H,            0
R2,                     / 24-bit arithmetic operand 2
R2L,            0
R2H,            0

VA,
VAL,            0
VAH,            0
VB,
VBL,            0
VBH,            0

        page    1
initialize,
        // Initialize ACIA
        cla
        tad     ACIA_config
        iot     ACIA ACIA_control
        tad     RX_NO_TX_NO
        iot     ACIA ACIA_control
        jms     I I_arith
        hlt
I_arith,        arith

ACIA_config,    CDS_RESET_gc
RX_NO_TX_NO,    WSB_8N1_gc

/// Print space
/// @clobber AC
putspace,       .-.
        cla
        tad     char_space
        jms     putchar
        jmp     I putspace
char_space,     040             / char ' '

/// Print new line
/// @clobber AC
newline,        .-.
        cla
        tad     char_CR
        jms     putchar
        cla
        tad     char_LF
        jms     putchar
        jmp I   newline
char_CR,        015             / Carriage return
char_LF,        012             / Line feed

/// Print character
/// @param AC character
putchar,        .-.
        dca     putchar_char    / Save character
putchar_loop,
        iot     ACIA ACIA_status
        and     bit_transmit
        sna                     / Skip if AC != 0
        jmp     putchar_loop    / AC=0
        cla
        tad     putchar_char    / Restore character
        iot     ACIA ACIA_transmit
        tad     putchar_char    / Restore character
        jmp     I putchar
putchar_char,   0
bit_transmit,   TDRE_bm

        decimal
lo,     function val,val & 07777B
hi,     function val,(val >> 12) & 07777B

        page

/// Set 2 24-bit values to VA and VB
/// @param (PC+1:PC+2) value for VA
/// @param (PC+3:PC+4) value for VB
/// @return AC=0
set_VAVB,       .-.
        cla
        tad     I set_VAVB      / AC=low(value)
        isz     set_VAVB
        dca     VAL             / low(VA)
        tad     I set_VAVB      / AC=high(value)
        isz     set_VAVB
        dca     VAH             / high(VA)
        tad     I set_VAVB      / AC=low(value)
        isz     set_VAVB
        dca     VBL             / low(VB)
        tad     I set_VAVB      / AC=high(value)
        isz     set_VAVB
        dca     VBH             / high(VB)
        jmp     I set_VAVB      / return, AC=0

char_add,      2BH              / char '+'

arith_end,
        jmp     I arith
arith,          .-.
        jms     I I_set_VAVB
        lo(18000)
        hi(18000)
        lo(28000)
        hi(28000)
        tad     char_add
        jms     I I_expr
        jms     I I_add24
        VB
        jms     I I_answer      / 46000

        jms     I I_set_VAVB
        lo(18000)
        hi(18000)
        lo(-18000)
        hi(-18000)
        tad     char_add
        jms     I I_expr
        jms     I I_add24
        VB
        jms     I I_answer      / 0

        jms     I I_set_VAVB
        lo(-18000)
        hi(-18000)
        lo(-18000)
        hi(-18000)
        tad     char_add
        jms     I I_expr
        jms     I I_add24
        VB
        jms     I I_answer      / -36000

        jmp     .+2
char_sub,     2DH               / char '-'

        jms     I I_set_VAVB
        lo(-18000)
        hi(-18000)
        lo(-28000)
        hi(-28000)
        tad     char_sub
        jms     I I_expr
        jms     I I_sub24
        VB
        jms     I I_answer      / 10000

        jms     I I_set_VAVB
        lo(18000)
        hi(18000)
        lo(-18000)
        hi(-18000)
        tad     char_sub
        jms     I I_expr
        jms     I I_sub24
        VB
        jms     I I_answer      / 36000

        jms     I I_set_VAVB
        lo(-28000)
        hi(-28000)
        lo(-18000)
        hi(-18000)
        tad     char_sub
        jms     I I_expr
        jms     I I_sub24
        VB
        jms     I I_answer      / -10000

        jmp     I I_arith_mul
I_arith_mul,    arith_mul

        page
char_mul,     2AH               / char '*'

arith_mul,
        jms     I I_set_VAVB
        lo(100)
        hi(100)
        lo(300)
        hi(300)
        tad     char_mul
        jms     I I_expr
        jms     I I_mul24
        VB
        jms     I I_answer      / 30000

        jms     I I_set_VAVB
        lo(200)
        hi(200)
        lo(300)
        hi(300)
        tad     char_mul
        jms     I I_expr
        jms     I I_mul24
        VB
        jms     I I_answer      / 60000

        jms     I I_set_VAVB
        lo(300)
        hi(300)
        lo(-200)
        hi(-200)
        tad     char_mul
        jms     I I_expr
        jms     I I_mul24
        VB
        jms     I I_answer      / -60000

        jms     I I_set_VAVB
        lo(100)
        hi(100)
        lo(-300)
        hi(-300)
        tad     char_mul
        jms     I I_expr
        jms     I I_mul24
        VB
        jms     I I_answer      / -30000

        jms     I I_set_VAVB
        lo(-200)
        hi(-200)
        lo(-100)
        hi(-100)
        tad     char_mul
        jms     I I_expr
        jms     I I_mul24
        VB
        jms     I I_answer      / 20000

        jmp     .+2
char_div,     2FH               / char '/'

        jms     I I_set_VAVB
        lo(30000)
        hi(30000)
        lo(100)
        hi(100)
        tad     char_div
        jms     I I_expr
        jms     I I_div24
        VB
        jms     I I_answer      / 300

        jms     I I_set_VAVB
        lo(-200)
        hi(-200)
        lo(100)
        hi(100)
        tad     char_div
        jms     I I_expr
        jms     I I_div24
        VB
        jms     I I_answer      / -2

        jms     I I_set_VAVB
        lo(-30000)
        hi(-30000)
        lo(-200)
        hi(-200)
        tad     char_div
        jms     I I_expr
        jms     I I_div24
        VB
        jms     I I_answer      / 150

        jms     I I_set_VAVB
        lo(-30000)
        hi(-30000)
        lo(78)
        hi(78)
        tad     char_div
        jms     I I_expr
        jms     I I_div24
        VB
        jms     I I_answer      / -384

        hlt                     /  @@@@@@@@

        jms     I I_set_VAVB
        lo(5000)
        hi(5000)
        lo(4000)
        hi(4000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(5000)
        hi(5000)
        lo(5000)
        hi(5000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(4000)
        hi(4000)
        lo(5000)
        hi(5000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-5000)
        hi(-5000)
        lo(-4000)
        hi(-4000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-5000)
        hi(-5000)
        lo(-5000)
        hi(-5000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-4000)
        hi(-4000)
        lo(-5000)
        hi(-5000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(32700)
        hi(32700)
        lo(32600)
        hi(32600)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(32700)
        hi(32700)
        lo(32700)
        hi(32700)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(32600)
        hi(32600)
        lo(32700)
        hi(32700)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-32700)
        hi(-32700)
        lo(-32600)
        hi(-32600)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-32700)
        hi(-32700)
        lo(-32700)
        hi(-32700)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-32600)
        hi(-32600)
        lo(-32700)
        hi(-32700)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(18000)
        hi(18000)
        lo(-28000)
        hi(-28000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-28000)
        hi(-28000)
        lo(-28000)
        hi(-28000)
        jms     I I_comp

        jms     I I_set_VAVB
        lo(-28000)
        hi(-28000)
        lo(18000)
        hi(18000)
        jms     I I_comp

        jmp     I I_arith_end
I_arith_end,    arith_end

/// Print expression; "A op B"
/// @param VA value of A
/// @param VB value of B
/// @param AC op character
/// @return R0 VA
/// @return AC=0
/// @clobber R0 R1 R2
expr,           .-.
        dca     expr_op         / save op
        tad     VAL
        dca     R0L
        tad     VAH
        dca     R0H
        jms     I I_print_int24 / print VA
        jms     I I_putspace    / print ' '
        cla
        tad     expr_op         / restore op
        jms     I I_putchar     / print 'op'
        jms     I I_putspace    / print ' '
        cla
        tad     VBL
        dca     R0L
        tad     VBH
        dca     R0H
        jms     I I_print_int24 / print VB
        tad     VAL
        dca     R0L
        tad     VAH
        dca     R0H
        jmp     I expr          / return, AC=0
expr_op,

/// Print answer; " = C\n"
/// @param R0 value of C
/// @clobber R0 R1 R2 AC
answer,         .-.
        jms     I I_putspace    / print ' '
        cla
        tad     char_equal
        jms     I I_putchar     / print '='
        jms     I I_putspace    / print ' '
        jms     I I_print_int24 / print R0
        jms     I I_newline     / print '\n'
        jmp     I answer        / return
char_equal,     3DH             / char '='

/// Print comparison; "A rel B\n"
/// @param VA value of A
/// @param VB value of B
comp,           .-.
        cla
        tad     VAL
        dca     R0L
        tad     VAH
        dca     R0H
        jms     I I_sub24       / R0=VA-VB, AC=0
        VB
        tad     R0H             / high(VA-VB)
        spa                     / skip if R0>=0
        jmp     comp_lt         / if VA<VB
        sza                     / skip if high(VA-VB) == 0
        jmp     comp_gt         / if VA>VB
        tad     R0L             / low(VA-VB)
        sza                     / skip if low(VA-VB) == 0
        jmp     comp_gt
comp_eq,
        tad     char_equal,
        jmp     comp_putchar
char_lt,        3CH             / char '<'
comp_lt,
        cla
        tad     char_lt
        jmp     comp_putchar
char_gt,        3EH             / char '>'
comp_gt,
        cla
        tad     char_gt
comp_putchar,
        jms     I I_putchar     / print 'rel'
        jmp     I comp          / return

        page

        include "arith.inc"

        page

/// Signed 24-bit multiply; R0 *= source
/// @param R0 multiplicand
/// @param (PC+1) multiplier source address
/// @return R0 product
/// @return AC=0
/// @clobber R1 R2
mul24_sign,     0
mul24,          .-.
        cla
        dca     mul24_sign      / clear sign
        tad     I mul24         / source address
        isz     mul24           / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     I arith_ptr     / low(source)
        dca     R2L             / low(multiplier)
        isz     arith_ptr       / advance to high(source)
        tad     I arith_ptr     / high(source)
        dca     R2H             / high(multiplier)
        tad     R2H
        sma cla                 / skip if multiplier<0, AC=0
        jmp     mul24_multiplicand
        isz     mul24_sign      / sign++
        jms     I I_neg24       / negate multiplier
        R2
mul24_multiplicand,
        tad     R0L
        dca     R1L             / low(multiplicand)
        tad     R0H
        dca     R1H             / high(multiplicand)
        tad     R1H
        sma                     / skip if multiplicand<0
        jmp     mul24_multiply
        isz     mul24_sign      / sign++
        jms     I I_neg24       / negate multiplicand
        R1
mul24_multiply,
        jms     I I_umul24      / R0 = R1 * R2
        tad     mul24_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     mul24_return
        jms     I I_neg24
        R0                      / negate product
mul24_return,
        jmp     I mul24         / return

/// Signed 24-bit division; quotient = dividend / divisor ... remainder
/// @param R0 dividend
/// @param (PC+1) signed 24-bit divisor address
/// @return R0 quotient
/// @return R1 remainder
/// @return AC=0
/// @clobber R2
div24_sign,     0
div24,          .-.
        cla
        dca     div24_sign      / clear sign
        tad     I div24
        isz     div24
        dca     arith_ptr
        tad     I arith_ptr     / AC=divisor
        sma                     / Skip if divisor < 0
        jmp     div24_divisor_plus
        isz     div24_sign      / sign++
        cia                     / negate divisor
div24_divisor_plus,
        dca     R2L             / R2L=|divisor|
        dca     R2H             / R2H=0
        tad     R0L
        dca     R1L             / R1=dividend
        tad     R0H
        dca     R1H
        tad     R1H
        sma                     / skip if R0H<0
        jmp     div24_divide
        isz     div24_sign      / sign++
        jms     I I_neg24       / negate dividend
        R1
div24_divide,
        jms     I I_udiv16      / R0 = R1 / R2 ... R1
        tad     div24_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     div24_return
        jms     I I_neg24
        R0                      / negate R0, AC=0
div24_return,
        jmp     I div24         / return, AC=0
