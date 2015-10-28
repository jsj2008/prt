#include <tests/common.h>
#include <prt/shared/popcnt.h>

int main(int argc, const char *argv[]) {
  size_t c;
  int r;
  bool v;
  uint64_t alternate = 0xaaaaaaaaaaaaaaaa;
  uint64_t four_times[4] = {alternate, alternate, alternate, alternate};

  uint64_t fours = 0x4444444444444444;
  uint64_t fours_five_times[5] = {fours, fours, fours, fours, fours};

  output1("[!] " PRD_HEADER " - popcnt (x64_86) test");

  output(" [+] Popcount(fast) of 0x%lx is: %zu", alternate,
         popcnt64_fast(&alternate, 1));
  output(" [+] Popcount(fast) of buffer is: %zu", popcnt64_fast(four_times, 4));
  output(" [+] Popcount(fast) of 0x%lx is: %zu", fours,
         popcnt64_fast(&fours, 1));
  output(" [+] Popcount(fast) of buffer is: %zu",
         popcnt64_fast(fours_five_times, 5));
  /*output(" [+] Bit scan forward(20): %u", bsf64((uint64_t)1 << 20));
  output(" [+] Bit scan forward(60): %u", bsf64((uint64_t)1 << 60));
  output(" [+] Bit scan forward(16): %u", bsf64((uint64_t)1 << 16));*/

  return 0;
}
