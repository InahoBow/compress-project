#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/*  CSV format (text mode)  - A-phase power (pa), voltage (ua), current (ia)
*  Format:
*	   ,pa,ua,ia
*	   0,2107.267332,223.9977763,14.38752741
*	   1,2109.770845,224.0341007,14.40099749
*	   2,2109.128518,224.0855046,14.41061483
*/
#define TXT_RAW_FILE "pui.org.csv"

/* Encoding flag for convert_according_to_bytebit()
*/
#define IS_RAW 3
#define IS_DIFF 2
#define IS_BYTE 1
#define IS_BIT 0

/*  Binary file containing <lines> records of P/U/I (complete structure)
 */
#define BIN_INPUT_FILE "pui_input.b"
#define BIN_INPUT_FILE_P "p_input.b"
#define BIN_INPUT_FILE_U "u_input.b"
#define BIN_INPUT_FILE_I "i_input.b"

/* Byte-plane result	*/
#define BYTE_RESULT_FILE "out/byte.res"
#define BYTE_RESULT_FILE_P "out/byte_p.res"
#define BYTE_RESULT_FILE_U "out/byte_u.res"
#define BYTE_RESULT_FILE_I "out/byte_i.res"


/* Bit-plane result	*/
#define BIT_RESULT_FILE "out/bit.res"
#define BIT_RESULT_FILE_P "out/bit_p.res"
#define BIT_RESULT_FILE_U "out/bit_u.res"
#define BIT_RESULT_FILE_I "out/bit_i.res"

/* Raw (no rearrange) result                                               */
#define RAW_RESULT_FILE_U "out/raw_u.res"
#define RAW_RESULT_FILE_I "out/raw_i.res"
#define RAW_RESULT_FILE_P "out/raw_p.res"

/*   Diff result (plain & byte/bit)                                          */
#define DIFF_RESULT_FILE_P "out/diff_p.res"
#define DIFF_RESULT_FILE_U "out/diff_u.res"
#define DIFF_RESULT_FILE_I "out/diff_i.res"
#define DIFF_BYTE_RESULT_FILE_P "out/diff_byte_p.res"
#define DIFF_BIT_RESULT_FILE_P "out/diff_bit_p.res"
#define DIFF_BYTE_RESULT_FILE_U "out/diff_byte_u.res"
#define DIFF_BIT_RESULT_FILE_U "out/diff_bit_u.res"
#define DIFF_BYTE_RESULT_FILE_I "out/diff_byte_i.res"
#define DIFF_BIT_RESULT_FILE_I "out/diff_bit_i.res"

/* File for round-trip verify (bit → byte)                                 */
#define BYTE_REVERSE_FILE "byte_reverse.b"


#define PRINT_SAMPLE_LINES 10 /* how many lines to show for demo      */
#define BYTE_CONVERSION 0
#define BIT_CONVERSION 1

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;


typedef struct struct_raw_pui
{
    int index;
    double p;
	double u;
	double i;
}RAW_PUI;

typedef struct struct_bin_pui
{
  	DWORD p;     /* power   scaled by 10   -> 4 bytes                     */
	WORD u; /* voltage scaled by 10   -> 2 bytes                     */
	DWORD i;/* current scaled by 1000 -> 4 bytes                     */
}__attribute__ ((packed)) BIN_PUI;


void usage()
{
	printf("Usage:\n");
	printf("\t ./pre_reassemble\n");
	printf("\t ./pre_reassemble <lines>\n");
}

/*------------------------------------------------------------------------
 * double2long() - quantize double by <factor>
 *------------------------------------------------------------------------*/
long double2long(double value, int factor) {
    return (long)(value * factor);
}

/*------------------------------------------------------------------------
 * raw2bin() - convert one RAW_PUI record into BIN_PUI
 *------------------------------------------------------------------------*/
void raw2bin(RAW_PUI * raw, BIN_PUI * pui)
{
	if(!raw || !pui){
		printf("null pointer %s\n", __FUNCTION__);
		return ;
		}
	pui->p=double2long(raw->p, 10);
	pui->u=double2long(raw->u, 10);
	pui->i=double2long(raw->i, 1000);
}
/*------------------------------------------------------------------------
 * prepare_binary_pui_file()
 *  Read <max_lines> from CSV and write binary files:
 *      a) whole struct  -> BIN_INPUT_FILE
 *      b) p channel     -> BIN_INPUT_FILE_P
 *      c) u channel     -> BIN_INPUT_FILE_U
 *      d) i channel     -> BIN_INPUT_FILE_I
 *------------------------------------------------------------------------*/
int prepare_binary_pui_file(char * rawfile, char * binfile, int max_lines)
{
	int ret, lines=0;
	char raw_file[512]="";
	char bin_file[512]="";
	char buf[512]="";
	RAW_PUI readPUI;
	BIN_PUI binPUI;
    FILE * fp=NULL, *fpw=NULL, *fpw_u=NULL,*fpw_p=NULL,*fpw_i=NULL;

	memset(&readPUI, 0x0, sizeof(readPUI));
	memset(&binPUI, 0x0, sizeof(binPUI));

	if(!rawfile || !binfile){
		printf("%s, invalid parameters\n", __FUNCTION__);
		return -1;
	}
	strncpy(raw_file, rawfile,sizeof(raw_file)-1);
	raw_file[sizeof(raw_file)-1]=0x0;
	strncpy(bin_file, binfile,sizeof(bin_file)-1);
	bin_file[sizeof(bin_file)-1]=0x0;
	
	printf("%s, prepare %s from %s\n", __FUNCTION__, bin_file, raw_file);
    fp=fopen(raw_file, "r");
    if(!fp){
        printf("%s failed, open %s!!!\n", __FUNCTION__, raw_file);
        return -1;
        }
    fpw=fopen(bin_file, "wb");
    if(!fpw){
        printf("%s failed, open %s!!!\n", __FUNCTION__, bin_file);
        ret=-1;
        goto err;
        }

    fpw_u=fopen(BIN_INPUT_FILE_U, "wb");
    if(!fpw_u){
        printf("%s failed, open %s!!!\n", __FUNCTION__, BIN_INPUT_FILE_U);
        ret=-1;
        goto err;
        }
    fpw_p=fopen(BIN_INPUT_FILE_P, "wb");
    if(!fpw_u){
        printf("%s failed, open %s!!!\n", __FUNCTION__, BIN_INPUT_FILE_P);
        ret=-1;
        goto err;
        }

    fpw_i=fopen(BIN_INPUT_FILE_I, "wb");
    if(!fpw_u){
        printf("%s failed, open %s!!!\n", __FUNCTION__, BIN_INPUT_FILE_I);
        ret=-1;
        goto err;
        }
    while(1){
        if(NULL == fgets(buf,sizeof(buf),fp)){
                printf("fgets end of file %s\n", raw_file);
           break;
            }
		
		if (sscanf(buf, "%d,%lf,%lf,%lf", &readPUI.index, &readPUI.p, &readPUI.u, &readPUI.i) != 4) {
			printf("Invalid line format: %s\n", buf);
			continue; 
		}
		if(lines++<max_lines){
			raw2bin(&readPUI, &binPUI);
			fwrite(&binPUI, sizeof(binPUI), 1, fpw);
			fwrite(&binPUI.p, sizeof(binPUI.p), 1, fpw_p);
			fwrite(&binPUI.u, sizeof(binPUI.u), 1, fpw_u);
			fwrite(&binPUI.i, sizeof(binPUI.i), 1, fpw_i);
		
			if(readPUI.index<PRINT_SAMPLE_LINES){
				printf("read %s\n", buf);
				printf("index %d, p %f, u %f, i %f\n", readPUI.index, readPUI.p,readPUI.u, readPUI.i);;
				printf("\tp %u, u %u, i %u\n", binPUI.p, binPUI.u, binPUI.i);
				}
			}
        }


	
		ret=0;
	err:
		if(fp) fclose(fp);
		if(fpw) fclose(fpw);
		if(fpw_u) fclose(fpw_u);
		if(fpw_p) fclose(fpw_p);
		if(fpw_i) fclose(fpw_i);
		return ret;
	
}

/*------------------------------------------------------------------------
 * get_byte_from_puis2()
 *  Fetch the <index>-th byte from a memory block which is treated as
 *      consecutive BIN_PUI (or WORD/DWORD) records.
 *------------------------------------------------------------------------*/
BYTE get_byte_from_puis2(int index, void *puis, int puis_len)
{
	BYTE * byte;
	if(index>=puis_len||!puis){
		printf("%s, invalid index %d or puis %p\n", __FUNCTION__, index, puis);
		return -1;
		}
	byte=(BYTE*)puis;
	return byte[index];
}


int read_puis(char * binfile, int lines, BYTE ** puis, int puis_size)
	{
		int ret;
		FILE * fp=NULL;
		if(!binfile){
			printf("%s, invalid parameter\n", __FUNCTION__);
			return -1;
			}
		*puis=malloc(puis_size*lines);
		if(!puis){
			printf("%s, malloc failed\n", __FUNCTION__);
			return -1;
			}
		fp=fopen(binfile, "r");
		if(!fp){
			printf("%s failed, open %s!!!\n", __FUNCTION__, binfile);
			ret=-2;
			goto err;
			}
		ret=fread (*puis, puis_size, lines, fp);
		printf("%s, ret %d\n", __FUNCTION__, ret);
		ret=0;
		err:
		if(fp) fclose(fp);
		return ret;
	}

/******************************************************************************
 *  convert_according_to_diff()
 * write raw or diff buffer directly (no rearrangement)
 ******************************************************************************/
int convert_according_to_diff(char * wfile, int lines, BYTE* puis, int puis_size)
{
	int bytes_len;
    FILE *fpw=NULL;

	bytes_len=puis_size*lines;

    fpw=fopen(wfile, "wb");
    if(!fpw){
        printf("%s failed, open %s!!!\n", __FUNCTION__, wfile);
        return -1;
     }

	fwrite(puis, sizeof(BYTE), bytes_len,                 fpw);
	fclose(fpw);
	return 0;

}

/******************************************************************************
 *  convert_according_to_byte2()
 * Byte-plane interleave
 *      Output order =   B0(P1,P2,…Pn)  B1(P1,P2,…Pn) …  (for generic stream)
 ******************************************************************************/
int convert_according_to_byte2(char * wfile, int lines, BYTE* puis, int puis_size)
{
	int i, j, count=0, ret, bytes_len;
	BYTE * bytes=NULL;
    FILE *fpw=NULL;

	bytes_len=puis_size*lines;
	bytes=malloc(bytes_len);
	if(!bytes){
		printf("%s, malloc failed\n", __FUNCTION__);
		ret=-1;
		goto err;
		}
  for(j=0;j<puis_size;j++){
		for(i=0;i<lines;i++){
			bytes[count++]=get_byte_from_puis2(j, (void *)&puis[i*puis_size], puis_size);
			}
		}

	printf("%d, %d, %d\n", i*lines+j, bytes_len, count);
    fpw=fopen(wfile, "wb");
    if(!fpw){
        printf("%s failed, open %s!!!\n", __FUNCTION__, wfile);
        ret=-1;
        goto err;
        }

	fwrite(bytes, sizeof(BYTE), bytes_len,                 fpw);
	ret=0;
	err:
	if(bytes) free(bytes);
	if(fpw) fclose(fpw);
	return ret;

}

/* get_byte_from_bytes()
 * See classical “bit-plane” transform: take every bit-plane sequentially */
int get_byte_from_bytes(BYTE* bytes, int bytes_len, int index)
{
	int i, bit_val;
	int    which_bit, which_byte;
	BYTE byte;

	int start_bit=index*8;
	which_bit=start_bit/bytes_len;
	which_byte=start_bit%bytes_len;
	byte=0;
	for(i=0;i<8;i++){
		bit_val=(bytes[which_byte+i]& (1 << (7 - which_bit))) ? 1 : 0;
		byte=byte|bit_val<<(7-i); 
		}
	return byte;
}

/* inverse of above but operate on “bit-plane arranged” buffer                */
int get_byte_from_bits(BYTE* bits, int bytes_len, int index)
{
    if (!bits || bytes_len <= 0 || index < 0 || index >= bytes_len) {
        return -1; 
    }

	int i, bit_val, round;
	BYTE byte;

	round=bytes_len/8;
	int byte_of_1st_bit, bit_of_1st_bit;
	byte_of_1st_bit=index/8;
	bit_of_1st_bit=index%8;
	byte=0;
	for(i=0;i<8;i++){
		bit_val=(bits[byte_of_1st_bit+i*round]& (1 << (7 - bit_of_1st_bit))) ? 1 : 0;
		byte=byte|bit_val<<(7-i); 
		}
	
    return byte;
}


/* bytes → bits (bit-plane), keeps length                                    */
int bytes2bits(BYTE *bytes, BYTE *bits, int bytes_len) {
	int i;
	if(!bytes || !bits || bytes_len <= 0 || bytes_len%8!=0){
		printf("%s, invalid bytes_len %d\n", __FUNCTION__, bytes_len);
		return -1;
		}
	for(i=0;i<bytes_len;i++){
		bits[i]=get_byte_from_bytes(bytes, bytes_len, i);
		}


    return 0; // 
}

/* bits → bytes (reverse)                                                    */
int bits2bytes(BYTE *bits, BYTE *bytes_reversed, int bytes_len) {
	int i;
	
    if (!bits || !bytes_reversed || bytes_len <= 0 || bytes_len % 8 != 0) {
        return -1; 
    }

	for(i=0;i<bytes_len;i++){
		bytes_reversed[i]=get_byte_from_bits(bits, bytes_len, i);
		}

    return 0; 
}


/******************************************************************************
 *  convert_according_to_bit2()
 *  Bit-plane interleave (require lines %8 ==0)
 ******************************************************************************/
int convert_according_to_bit2(char * wfile, int lines, BYTE* puis, int puis_size)
{
	int i, j, count=0, ret, bytes_len;
	BYTE * bytes=NULL;
	BYTE * bits=NULL;
	BYTE * bytes_back=NULL;
    FILE *fpw=NULL, *fpw_back=NULL;
	if(lines%8!=0){
		printf("%s, lines should be mod by 8\n", __FUNCTION__);
		return -1;
		}
	bytes_len=puis_size*lines;
	bytes=malloc(bytes_len);
	bits=malloc(bytes_len);
	bytes_back=malloc(bytes_len);
	if(!puis || !bytes || !bits || !bytes_back){
		printf("%s, malloc failed\n", __FUNCTION__);
		ret=-1;
		goto err;
		}
  for(j=0;j<puis_size;j++){
		for(i=0;i<lines;i++){
			bytes[count++]=get_byte_from_puis2(j, (void *)&puis[i*puis_size], puis_size);
			}
		}

	printf("**%d, %d, %d\n", i*lines+j, bytes_len, count);
	bytes2bits(bytes, bits, bytes_len);
	bits2bytes(bits, bytes_back, bytes_len);
    fpw=fopen(wfile, "wb");
    if(!fpw){
        printf("%s failed, open %s!!!\n", __FUNCTION__, wfile);
        ret=-1;
        goto err;
        }

	fwrite(bits, sizeof(BYTE), bytes_len,                 fpw);
	
	if (ferror(fpw)) {
	    printf("Error writing to file\n");
	    ret = -1;
	    goto err;
	}

	fpw_back=fopen(BYTE_REVERSE_FILE, "wb");
    if(!fpw_back){
        printf("%s failed, open %s!!!\n", __FUNCTION__, BYTE_REVERSE_FILE);
        ret=-1;
        goto err;
        }

	fwrite(bytes_back, sizeof(BYTE), bytes_len,                 fpw_back);
	if (ferror(fpw_back)) {
	    printf("Error writing to backfile\n");
	    ret = -1;
	    goto err;
	}
	
	ret=0;
	err:
	if(bytes) free(bytes);
	if(bits) free(bits);
	if(fpw) fclose(fpw);
	if(fpw_back) fclose(fpw_back);
	return ret;
		
}
 /*------------------------------------------------------------------------
 * puis_diff()
 *  EN: First-order difference (plain).   Sign bit is stored in MSB.
 *       lines     : number of elements
 *       puis      : byte stream (WORD or DWORD array)
 *       puis_size : 2 (WORD) or 4 (DWORD)
 *------------------------------------------------------------------------*/
int puis_diff(int lines, BYTE *puis, int puis_size)
{
    if (!puis || lines <= 0)               return -1;
    if (puis_size != 2 && puis_size != 4)  return -2;

    if (puis_size == 2)            
    {
        WORD *p = (WORD *)puis;
        WORD  prev = p[0];                     

        for (int i = 1; i < lines; ++i)
        {
            WORD  curr = p[i];
            int   diff = (int)curr - (int)prev;
            prev  = curr;     

            unsigned mag = (diff >= 0) ? (unsigned) diff
                                       : (unsigned)(-diff); 
            if (mag > 0x7FFF) mag = 0x7FFF;
            p[i] = (diff < 0) ? (WORD)(mag | 0x8000u)  
                               : (WORD)(mag);          
        }
    }
    else          
    {
        DWORD *p = (DWORD *)puis;
        DWORD  prev = p[0];

        for (int i = 1; i < lines; ++i)
        {
            DWORD  curr = p[i];
            long diff = (long)curr - (long)prev; 
            prev   = curr;

            DWORD mag = (diff >= 0) ? (DWORD) diff
                                       : (DWORD)(-diff);
            if (mag > 0x7FFFFFFF) mag = 0x7FFFFFFF;

            p[i] = (diff < 0) ? (mag | 0x80000000u)
                               : (mag);                   
        }
    }
    FILE *fpw=NULL;

	printf("%s,11 write diff.res\n", __FUNCTION__);
	
    fpw=fopen("diff.res", "wb");
    if(!fpw){
        printf("%s failed, open %s!!!\n", __FUNCTION__, "diff.res");
        return -1;
        }
	fwrite(puis, puis_size, lines, fpw);
	fclose(fpw);

    return 0;
}
/////////////////////

static inline WORD zigzag16(int32_t v)
{
    /*  [-32768,32767] → [0,65535] */
    return (WORD)(((uint32_t)v << 1) ^ (uint32_t)(v >> 15));
}

static inline DWORD zigzag32(int64_t v)
{
    /*  [-2^31,2^31-1] → [0,0xFFFFFFFF] */
    return (DWORD)(((uint64_t)v << 1) ^ (uint64_t)(v >> 31));
}

/*------------------------------------------------------------------------
 * puis_diff_zigzag()
 *  First-order difference + ZigZag mapping
 *------------------------------------------------------------------------*/ 
int puis_diff_zigzag(int lines, BYTE *puis, int puis_size)
{
    if (!puis || lines <= 0)               return -1;
    if (puis_size != 2 && puis_size != 4)  return -2;

    if (puis_size == 2)          /* ---------- WORD ---------- */
    {
        WORD *p   = (WORD *)puis;
        WORD  prev = p[0];  

        for (int i = 1; i < lines; ++i)
        {
            WORD curr = p[i];
            int32_t diff = (int32_t)curr - (int32_t)prev;  
            prev = curr;

            
            if (diff >  32767)  diff =  32767;
            if (diff < -32768)  diff = -32768;

            p[i] = zigzag16(diff);
        }
    }
    else                         /* ---------- DWORD ---------- */
    {
        DWORD *p   = (DWORD *)puis;
        DWORD  prev = p[0];

        for (int i = 1; i < lines; ++i)
        {
            DWORD curr = p[i];
            int64_t diff = (int64_t)curr - (int64_t)prev;  
            prev = curr;

            
            if (diff >  2147483647LL) diff =  2147483647LL;
            if (diff < -2147483648LL) diff = -2147483648LL;

            p[i] = zigzag32(diff);
        }
    }

printf("%s, write diff.res\n", __FUNCTION__);
    FILE *fp = fopen("diff.res", "wb");
    if (!fp)
    {
        fprintf(stderr, "open diff.res failed!\n");
        return -3;
    }
    fwrite(puis, puis_size, lines, fp);
    fclose(fp);

    return 0;
}

/******************************************************************************
 *  Top level splitter: convert_according_to_bytebit()
 ******************************************************************************/
int convert_according_to_bytebit(char * wfile, int lines, BYTE* puis, int puis_size, int factor, int isbyte)
{
	int i;
	BYTE * seg_puis=NULL;
	char filename[512]="";
	if(lines%factor!=0){
		printf("%s failed, lines %d, factor %d\n", __FUNCTION__, lines, factor);
		return -1;
		}
	int seg_lines=lines/factor;
	for(i=0;i<factor;i++){
		seg_puis=&puis[i*seg_lines*puis_size];
		snprintf(filename, sizeof(filename)-1, "%s.%02d", wfile, i);
		switch(isbyte){
			case IS_BYTE:
				convert_according_to_byte2(filename, seg_lines, seg_puis, puis_size);
				break;
			case IS_BIT:
				convert_according_to_bit2(filename, seg_lines, seg_puis, puis_size);
				break;
			case IS_DIFF:
				convert_according_to_diff(filename, seg_lines, seg_puis, puis_size);
				break;
			case IS_RAW:
				convert_according_to_diff(filename, seg_lines, seg_puis, puis_size);
				break;
			default:
				printf("invalid isbyte %d\n", isbyte);
				break;
			}
/*
		if(isbyte==IS_BYTE)
			convert_according_to_byte2(filename, seg_lines, seg_puis, puis_size);
		else
			convert_according_to_bit2(filename, seg_lines, seg_puis, puis_size);
*/
		}
	return 0;
}


int pirnt_binary_pui_file(char * binfile)
{
	int index=0;
	BIN_PUI ulPUI;
    FILE * fp;
	if(!binfile){
		printf("%s, invalid parameter\n", __FUNCTION__);
		return -1;
		}
    fp=fopen(binfile, "r");
    if(!fp){
        printf("%s failed, open %s!!!\n", __FUNCTION__, binfile);
        return -1;
        }
	while(fread (&ulPUI, sizeof (ulPUI), 1, fp)>0){
		if(index<PRINT_SAMPLE_LINES)
			printf("index %d, p %u, u %u, i %u\n", index, ulPUI.p, ulPUI.u, ulPUI.i);
		index++;
		}
	fclose(fp);	
	printf("total %d lines\n", index);
	return 0;
}

int test()
{
	int i;
	// bytes: 10101010 01010101 11111111 00000000, 01010101 10101010 00000000 11111111 
	// bits: 10100101 01101001 10100101 01101001, 10100101 01101001 10100101 01101001
	// bits: A5,69,A5,69,A5,69,A5,69
	// bytes_reversed: 10101010 01010101 11111111 00000000, 01010101 10101010 00000000 11111111
    BYTE bytes[8] = {0xAA, 0x55, 0xFF, 0x00, 0x55, 0xAA, 0x00, 0xFF};
    BYTE bits[8] = {0};   
    BYTE bytes_reversed[8] = {0};             
	bytes2bits(bytes, bits, sizeof(bytes));
	bits2bytes(bits, bytes_reversed, sizeof(bits));
	printf("Bytes array:\n");
	   for (i = 0; i < sizeof(bytes); i++) {
		   printf("0x%02X ", bytes[i]);
	   }
	   printf("\n");
	printf("Bits array:\n");
	   for (i = 0; i < sizeof(bits); i++) {
		   printf("0x%02X ", bits[i]);
	   }
	   printf("\n");

	printf("reversed array:\n");
	   for (i = 0; i < sizeof(bytes_reversed); i++) {
		   printf("0x%02X ", bytes_reversed[i]);
	   }
	   printf("\n");
	   return 0;
}





int main(int argc, char * argv[])
{
	int ret, lines;


//	test(); return 0;

	if(argc != 1 && argc != 2){
		usage();
		return -1;
		}
	if(argc==1)
		lines=102400;
	else
		lines=atoi(argv[1]);


	ret=prepare_binary_pui_file(TXT_RAW_FILE, BIN_INPUT_FILE, lines);
	if(ret!=0){
		printf("FATAL error, %s failed\n",__FUNCTION__);
		return ret;
		}
	//////// bin pui file is OK ///////////
	pirnt_binary_pui_file(BIN_INPUT_FILE);
	printf("struct %ld, ul %ld, ui %ld, us %ld\n", sizeof(BIN_PUI), sizeof(unsigned long), sizeof(unsigned int), sizeof(unsigned short));
	printf("Now start processing...\n");



typedef enum {
	TEST_PUIS =0,
	TEST_P,
	TEST_U,
	TEST_I,
	TEST_MAX
}TEST_ITEM;
char *test_case[TEST_MAX]={"test puis", "test power", "test voltage", "test current"};
int unit_size[TEST_MAX]={10, 4, 2, 4};

BYTE *puis=NULL, *puis_p=NULL, *puis_u=NULL, *puis_i=NULL;

#define SEGMENTS 10

TEST_ITEM t=TEST_I;
char buf[512];
sprintf(buf, "echo \"####  %s, segments %d, segsize %d*%d  ####\"> out/readme", 
						test_case[t], SEGMENTS, unit_size[t], lines/SEGMENTS);
system(buf);
switch(t){
	case TEST_PUIS:
		read_puis(BIN_INPUT_FILE, lines, &puis, sizeof(BIN_PUI));
		convert_according_to_bytebit(BYTE_RESULT_FILE, lines, puis, sizeof(BIN_PUI), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(BIT_RESULT_FILE, lines, puis, sizeof(BIN_PUI), SEGMENTS, IS_BIT);
		if(puis) free(puis);
		break;
	case TEST_P:
		read_puis(BIN_INPUT_FILE_P, lines, &puis_p, sizeof(DWORD));
		convert_according_to_bytebit(RAW_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_RAW);
		convert_according_to_bytebit(BYTE_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(BIT_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_BIT);
		puis_diff_zigzag(lines, puis_p, sizeof(DWORD));
		convert_according_to_bytebit(DIFF_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_DIFF);
		convert_according_to_bytebit(DIFF_BYTE_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(DIFF_BIT_RESULT_FILE_P, lines, puis_p, sizeof(DWORD), SEGMENTS, IS_BIT);
		if(puis_p) free(puis_p);
		break;
	case TEST_U:
		read_puis(BIN_INPUT_FILE_U, lines, &puis_u, sizeof(WORD));
		convert_according_to_bytebit(RAW_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_RAW);
		convert_according_to_bytebit(BYTE_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(BIT_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_BIT);
		puis_diff(lines, puis_u, sizeof(WORD));
		convert_according_to_bytebit(DIFF_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_DIFF);
		convert_according_to_bytebit(DIFF_BYTE_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(DIFF_BIT_RESULT_FILE_U, lines, puis_u, sizeof(WORD), SEGMENTS, IS_BIT);
		if(puis_u) free(puis_u);
		break;
	case TEST_I:
		read_puis(BIN_INPUT_FILE_I, lines, &puis_i, sizeof(DWORD));
		convert_according_to_bytebit(RAW_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_RAW);
		convert_according_to_bytebit(BYTE_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(BIT_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_BIT);
		puis_diff(lines, puis_i, sizeof(DWORD));
		convert_according_to_bytebit(DIFF_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_DIFF);
		convert_according_to_bytebit(DIFF_BYTE_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_BYTE);
		convert_according_to_bytebit(DIFF_BIT_RESULT_FILE_I, lines, puis_i, sizeof(DWORD), SEGMENTS, IS_BIT);
		if(puis_i) free(puis_i);
		break;
	default:
		break;
}
	return 0;
}

