/*
 * mydeflate.c — simple zlib compressor / decompressor with
 *               runtime-selectable windowBits and memLevel.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define CHUNK 16384              /* 16 KiB I/O buffer */

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  Compress:   %s -w <8..15> -m <1..9> -c <input> <output>\n"
        "  Decompress: %s -w <8..15> -m <1..9> -x <input> <output>\n",
        prog, prog);
}

enum {MODE_NONE, MODE_COMPRESS, MODE_DECOMPRESS};

/* ------------------------------------------------------------
 * Parse command line.
 * Return 0 on success, −1 on any error.
* -----------------------------------------------------------*/
static int parse_args(int argc, char **argv,
                      int *mode, int *wbits, int *mlevel,
                      const char **infile, const char **outfile)
{
    *mode   = MODE_NONE;
    *wbits  = 15;   /* zlib default */
    *mlevel = 8;

    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            *wbits = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-m") && i + 1 < argc) {
            *mlevel = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-c")) {
            *mode = MODE_COMPRESS;
        } else if (!strcmp(argv[i], "-x")) {
            *mode = MODE_DECOMPRESS;
        } else {
            return -1;
        }
        ++i;
    }
    if (argc - i != 2) return -1;
    if (*mode == MODE_NONE)   return -1;
    if (*wbits  < 8 || *wbits  > 15) return -1;
    if (*mlevel < 1 || *mlevel > 9)  return -1;

    *infile  = argv[i];
    *outfile = argv[i + 1];
    return 0;
}

/* ------------------------------------------------------------
* Compress <in> to <out> with given windowBits / memLevel.
* -----------------------------------------------------------*/
static int do_compress(FILE *in, FILE *out, int wbits, int mlevel)
{
    z_stream strm;
    unsigned char in_buf[CHUNK], out_buf[CHUNK];
    int ret, flush;

    memset(&strm, 0, sizeof(strm));
    ret = deflateInit2(&strm,
                       Z_DEFAULT_COMPRESSION,
                       Z_DEFLATED,
                       wbits,
                       mlevel,
                       Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) return ret;

    do {
        strm.avail_in = fread(in_buf, 1, CHUNK, in);
        if (ferror(in)) { deflateEnd(&strm); return Z_ERRNO; }

        strm.next_in = in_buf;
        flush = feof(in) ? Z_FINISH : Z_NO_FLUSH;

        do {
            strm.next_out  = out_buf;
            strm.avail_out = CHUNK;

            ret = deflate(&strm, flush);
            if (ret == Z_STREAM_ERROR) { deflateEnd(&strm); return ret; }

            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out_buf, 1, have, out) != have || ferror(out)) {
                deflateEnd(&strm); return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
    } while (flush != Z_FINISH);

    deflateEnd(&strm);
    return Z_OK;
}

/* ------------------------------------------------------------
 * Decompress <in> to <out>.  memLevel is ignored.
 * -----------------------------------------------------------*/
static int do_decompress(FILE *in, FILE *out, int wbits)
{
    z_stream strm;
    unsigned char in_buf[CHUNK], out_buf[CHUNK];
    int ret;

    memset(&strm, 0, sizeof(strm));
    ret = inflateInit2(&strm, wbits);
    if (ret != Z_OK) return ret;

    do {
        strm.avail_in = fread(in_buf, 1, CHUNK, in);
        if (ferror(in)) { inflateEnd(&strm); return Z_ERRNO; }

        if (strm.avail_in == 0) break;
        strm.next_in = in_buf;

        do {
            strm.next_out  = out_buf;
            strm.avail_out = CHUNK;

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR ||
                ret == Z_MEM_ERROR) {
                inflateEnd(&strm); return ret;
            }

            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out_buf, 1, have, out) != have || ferror(out)) {
                inflateEnd(&strm); return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int main(int argc, char **argv)
{
    int mode, wbits, mlevel;
    const char *in_path, *out_path;

    if (parse_args(argc, argv, &mode, &wbits, &mlevel, &in_path, &out_path)) {
        usage(argv[0]);
        return 1;
    }

    FILE *in  = fopen(in_path,  "rb");
    if (!in) { perror(in_path); return 2; }

    FILE *out = fopen(out_path, "wb");
    if (!out) { perror(out_path); fclose(in); return 3; }

    int zret = (mode == MODE_COMPRESS)
               ? do_compress  (in, out, wbits, mlevel)
               : do_decompress(in, out, wbits);

    fclose(in);
    fclose(out);

    if (zret != Z_OK) {
        fprintf(stderr, "%s failed: zlib error %d\n",
                mode == MODE_COMPRESS ? "Compression" : "Decompression", zret);
        return 4;
    }
    return 0;
}
