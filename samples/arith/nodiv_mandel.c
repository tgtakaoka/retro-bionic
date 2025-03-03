#include <stdio.h>
#include <stdint.h>

typedef int16_t value_t;
const value_t F = 64;
const value_t S = 128;

const value_t KA = 0.0458 * F * S;
const value_t KB = 0.0833 * F * S;

int main(int argc, char **argv) {
  for (value_t y = -12; y <= 12; ++y) {
    for (value_t x = -49; x <= 29; ++x) {
      value_t c = x * KA / S;
      value_t d = y * KB / S;
      value_t a = c;
      value_t b = d;
      value_t i = 0;
      do {
        value_t q = b / F;
        value_t s = b - q * F;
        value_t tmp = (a * a - b * b) / F + c;
        b = 2 * (a * q + a * s / F) + d;
        a = tmp;
        value_t p = a / F;
        q = b / F;
        value_t t = p * p + q * q;
        if (t > 4)
          break;
        ++i;
      } while (i < 16);
      char out = i < 10 ? i + '0' : (i < 16 ? i - 10 + 'A' : ' ');
      putchar(out);
    }
    printf("\n");
  }
  return 0;
}
