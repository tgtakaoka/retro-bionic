REM ASCII art Mandelbrot set: samples/arith/fmandel.bas in CBASIC.
FOR Y% = -12 TO 12
    FOR X% = -49 TO 29
        CA = X% * 0.0458
        CB = Y% * 0.08333
        A = CA
        B = CB
        FOR I% = 0 TO 15
            T = A * A - B * B + CA
            B = 2 * A * B + CB
            A = T
            IF (A * A + B * B) > 4 THEN GOTO 180
        NEXT I%
        PRINT " ";
        GOTO 200
180     IF I% > 9 THEN I% = I% + 7
        PRINT CHR$(48 + I%);
200 NEXT X%
    PRINT
NEXT Y%
END
