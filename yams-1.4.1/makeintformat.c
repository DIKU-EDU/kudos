/* Provide the printf formatting and constant generation macros since
 * they were left out from the C99 standard and are not necessarily
 * provided on all platforms (e.g. FreeBSD).
 *
 * A couple of (reasonable) assumptions are made here: 'int' is
 * assumed to be at least 32 bits, and 'long' and 'long long' are
 * assumed to be no bigger than 64 bits.
 *
 * See the end of configure.ac on how this is used.
 */

#include <stdio.h>

int main() {
    printf("#ifndef INTFORMAT_H\n#define INTFORMAT_H\n\n");

    printf("#define PRId32 \"d\"\n");
    printf("#define PRIu32 \"u\"\n");
    printf("#define PRIx32 \"x\"\n");
    printf("#define PRIx16 \"x\"\n");
    printf("#define PRIx8 \"x\"\n");

    if (sizeof(unsigned long) == 8) {
	printf("#define PRIu64 \"lu\"\n");
	printf("#define UINT64_C(c) c ## UL\n");
    }
#ifdef HAVE_LONG_LONG
    else {
	printf("#define PRIu64 \"llu\"\n");
	printf("#define UINT64_C(c) c ## ULL\n");
    }
#endif

    printf("\n#endif\n");
    return 0;
}
