/*  shrink & expand consecutive-zero blocks in a binary file
 *   A tiny utility that “shrinks” long continuous 0-byte areas in
 *       a binary file, and later “expands” (restores) them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>


/* Threshold: a run of ≥ CONTINUE_ZERO consecutive zeros triggers shrinking */
#define CONTINUE_ZERO  8

#define MAGIC          "SZR0"
#define VERSION        1
#define BUF_SIZE       65536          /* 64 KiB                                */

#pragma pack(push,1)
typedef struct {
    char     magic[4];      /* "SZR0" */
    uint16_t version;       /* =1      */
    uint16_t rec_cnt;        /* how many zero records follow                  */
    uint32_t original_sz;   /* original (un-shrunk) file size                */
} Header;

typedef struct {
    uint32_t off;           /* offset in original file                       */
    uint32_t len;           /* length of the zero block                      */
} ZeroRec;
#pragma pack(pop)

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*==========================================================================*/
/* Step-1 : Scan input file, collect every eligible zero segment            */
/*==========================================================================*/

static ZeroRec *scan_zero_blocks(FILE *fp, uint64_t *out_cnt, uint64_t *out_size)
{
    uint64_t capacity = 128;                   /* initial vector size  */
    ZeroRec *arr = malloc(capacity * sizeof *arr);
    if (!arr) die("malloc");

    uint64_t cnt = 0; /* number of records found  */
    uint64_t file_off = 0; /* current offset in file   */
    int in_zero = 0; /* inside a zero run?       */
    uint64_t zero_start = 0, zero_len = 0;

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == 0) {                          /* 0x00 byte                */
            if (!in_zero) {
                in_zero   = 1;
                zero_start = file_off;
                zero_len   = 1;
            } else {
                ++zero_len;
            }
        } else {            /* non-0x00 byte            */
            if (in_zero && zero_len >= CONTINUE_ZERO) {
        /* store this zero block                                     */
                if (cnt == capacity) {
                    capacity *= 2;
                    arr = realloc(arr, capacity * sizeof *arr);
                    if (!arr) die("realloc");
                }
                arr[cnt].off = zero_start;
                arr[cnt].len = zero_len;
                ++cnt;
            }
            in_zero = 0;
        }
        ++file_off;
    }

        /* Handle the tailing zeros   */
    if (in_zero && zero_len >= CONTINUE_ZERO) {
        if (cnt == capacity) {
            capacity *= 2;
            arr = realloc(arr, capacity * sizeof *arr);
            if (!arr) die("realloc");
        }
        arr[cnt].off = zero_start;
        arr[cnt].len = zero_len;
        ++cnt;
    }

    *out_cnt  = cnt;
    *out_size = file_off;
    return arr;
}

/*==========================================================================*/
/* shrink_file()  —  compress                                               */
/*==========================================================================*/

static int shrink_file(const char *in_name, const char *out_name)
{
    FILE *fin  = fopen(in_name, "rb");
    if (!fin) die("fopen input");

    /* Pass-1  : detect zero runs                                            */
    uint64_t rec_cnt, orig_sz;
    ZeroRec *recs = scan_zero_blocks(fin, &rec_cnt, &orig_sz);

  /* Pass-2  : copy data while skipping stored zero runs                   */
    rewind(fin);
    FILE *fout = fopen(out_name, "wb");
    if (!fout) die("fopen output");

   /* Write provisional header first                                        */
    Header hdr;
    memcpy(hdr.magic, MAGIC, 4);
    hdr.version      = VERSION;
    hdr.original_sz  = orig_sz;
    hdr.rec_cnt      = (uint16_t)rec_cnt;
    if (fwrite(&hdr, sizeof hdr, 1, fout) != 1) die("write header");

    /* Write record list immediately after header                            */
    if (rec_cnt && fwrite(recs, sizeof(ZeroRec), rec_cnt, fout) != rec_cnt)
        die("write rec");

     /* Copy real data, omitting zero areas                                   */
    uint64_t next_idx = 0;                 
    uint64_t file_off = 0;
    uint8_t  buf[BUF_SIZE];

    while (file_off < orig_sz) {
        uint64_t stop = orig_sz;
        if (next_idx < rec_cnt)
            stop = recs[next_idx].off;     

        uint64_t left = stop - file_off;
        while (left) {
            size_t chunk = (left > BUF_SIZE) ? BUF_SIZE : (size_t)left;
            if (fread(buf, 1, chunk, fin) != chunk) die("read while copy");
            if (fwrite(buf, 1, chunk, fout) != chunk) die("write while copy");

            file_off += chunk;
            left     -= chunk;
        }

     /* Skip the zero segment in input                                    */
        if (next_idx < rec_cnt) {
            uint64_t zero_len = recs[next_idx].len;
            if (fseek(fin, (long)zero_len, SEEK_CUR) != 0) die("fseek skip zero");
            file_off += zero_len;
            ++next_idx;
        }
    }

    fclose(fin);
    fclose(fout);
    free(recs);
    return 0;
}

/*==========================================================================*/
/* expand_file()  —  restore                                                */
/*==========================================================================*/

static int expand_file(const char *in_name, const char *out_name)
{
    FILE *fin = fopen(in_name, "rb");
    if (!fin) die("fopen in");

    Header hdr;
    if (fread(&hdr, sizeof hdr, 1, fin) != 1) die("read header");
    if (memcmp(hdr.magic, MAGIC, 4) || hdr.version != VERSION) {
        fprintf(stderr, "Not a shrinked file or version mismatch\n");
        return -1;
    }

    uint64_t rec_cnt = hdr.rec_cnt;
    ZeroRec *recs = NULL;
    if (rec_cnt) {
        recs = malloc(rec_cnt * sizeof *recs);
        if (!recs) die("malloc recs");
        if (fread(recs, sizeof *recs, rec_cnt, fin) != rec_cnt)
            die("read recs");
    }

    FILE *fout = fopen(out_name, "wb");
    if (!fout) die("fopen out");

    uint64_t idx = 0;    
    uint64_t file_off = 0;
    uint8_t  buf[BUF_SIZE];

    while (file_off < hdr.original_sz) {
        uint64_t next_zero_off = hdr.original_sz;
        uint64_t next_zero_len = 0;

        if (idx < rec_cnt) {
            next_zero_off = recs[idx].off;
            next_zero_len = recs[idx].len;
        }

    /* Copy “normal” data up to next zero block                          */
        uint64_t left = next_zero_off - file_off;
        while (left) {
            size_t chunk = (left > BUF_SIZE) ? BUF_SIZE : (size_t)left;
            size_t rd = fread(buf, 1, chunk, fin);
            if (rd != chunk) die("read data stream");
            if (fwrite(buf, 1, rd, fout) != rd) die("write out");
            file_off += rd;
            left     -= rd;
        }

    /* Insert zeros back                                                 */
        if (idx < rec_cnt) {
            memset(buf, 0, BUF_SIZE);
            uint64_t zleft = next_zero_len;
            while (zleft) {
                size_t chunk = (zleft > BUF_SIZE) ? BUF_SIZE : (size_t)zleft;
                if (fwrite(buf, 1, chunk, fout) != chunk) die("write zeros");
                zleft   -= chunk;
            }
            file_off += next_zero_len;
            ++idx;
        }
    }

    fclose(fin);
    fclose(fout);
    free(recs);
    return 0;
}

/*==========================================================================*/
/* CLI entry                                                                 */
/*==========================================================================*/
/* Usage demonstration:
 *   shrk -c in.bin  out.szr   (compress/shrink)
 *   shrk -x in.szr  out.bin   (expand/restore)
 */
int main(int argc, char *argv[])
{
    if (argc != 4 || (strcmp(argv[1], "-c") && strcmp(argv[1], "-x"))) {
        fprintf(stderr,
                "Usage:\n"
                "  %s -c <input.bin> <output.szr>   (compress)\n"
                "  %s -x <input.szr> <output.bin>   (expand)\n",
                argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-c") == 0) {
        return shrink_file(argv[2], argv[3]);
    } else {
        return expand_file(argv[2], argv[3]);
    }
}
