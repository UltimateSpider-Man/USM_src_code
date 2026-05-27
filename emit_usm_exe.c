/*
 * emit_usm_exe.c — build-time tool that writes the stock USM.exe from
 * the rawData[] byte array embedded in USM.c.
 *
 * USM.c (uploaded, 36.5 MB of ASCII-hex) declares:
 *   unsigned char rawData[5926912] = { 0x4D, 0x5A, ... };
 * which is byte-for-byte identical to the original Ultimate
 * Spider-Man USM.exe (5,926,912 bytes = 5.65 MiB).
 *
 * CMake compiles this file together with USM.c into a host-native
 * executable and runs it once during build with the target output
 * path as argv[1]. The result is <build>/USM.exe containing exactly
 * the same bytes as the stock game exe.
 *
 * Usage:
 *     emit_usm_exe <output-path>
 *
 * Example:
 *     emit_usm_exe build/USM.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char rawData[];
extern unsigned char rawData2[];
/* The size is baked into the declaration in USM.c: rawData[5926912].
 * We know the value at compile-time, so hard-code it. Alternatively
 * read sizeof(rawData) in USM.c and expose via an extern — but that
 * would require touching the uploaded file. Keeping USM.c untouched.
 */
#define RAWDATA_SIZE  5926912u   /* = 5.65 MiB = stock USM.exe size */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <output-path>\n", argv[0]);
        return 2;
    }

    FILE *f = fopen(argv[1], "wb");
    if (!f) {
        fprintf(stderr, "error: cannot open '%s' for writing\n", argv[1]);
        return 1;
    }

    /* Chunked write so huge payloads don't stress a single fwrite on
     * runtime CRTs that still have 32-bit internal counters. 1 MiB is
     * safely under every limit and keeps memory pressure flat. */
    const unsigned CHUNK = 1u << 20;
    unsigned remaining   = RAWDATA_SIZE;
    unsigned offset      = 0;
    while (remaining > 0) {
        unsigned n = (remaining > CHUNK) ? CHUNK : remaining;
        size_t wrote = fwrite(rawData + offset, 1, n, f);
        if (wrote != n) {
            fprintf(stderr,
                    "error: short write at offset %u (wrote %zu of %u)\n",
                    offset, wrote, n);
            fclose(f);
            return 1;
        }
        offset    += n;
        remaining -= n;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "error: close failed on '%s'\n", argv[1]);
        return 1;
    }

    printf("emit_usm_exe: wrote %u bytes (%.2f MiB) → %s\n",
           RAWDATA_SIZE,
           (double)RAWDATA_SIZE / (1024.0 * 1024.0),
           argv[1]);
    return 0;
}
