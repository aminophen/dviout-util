/*
 *		check Fonts (version of AMS FONTS)
 *                         Dec 1990, written by SHIMA
 *						   Apr 1992, second version
 *						   May 1992, third  version
 *						   Sep 1992, fifth  version
 *						   May 1996, sixth  version
 *
 * Usage: chkfont [-s] [-c] [-{f|F}<font_data_file>]    dvi_file_name\n"
 *        chkfont [-s] [-c] [-{f|F}<font_data_file>] -t tfm_file_name\n"
 *        chkfont [-s] [-c] [-{f|F}<font_data_file>] -p font_file_name\n\n"
 *
 * "c" means check mode
 * "s" means silent check mode
 * "f" means user defined font data
 * "F" means user defined font data and ignore default data
 * "t" means tfm_file
 * "p" means font_file
 *
 * errorlevel is the sum of th following number
 *           1: cmbsy5-9, cmcsc8-9, cmex7-9, cmmib5-9
 *           2: new font: eu??, msam, msbm, wncy??, font_data_file
 *           4: new eufm5-10
 *           8: unknown eufm
 *          16: old font: msxm, msym, amcsc, mcyr, mcyb, font_data_file
 *          32: old eufm5-10
 *
 * If errorlevel is larger than 240,
 *         248: error in font_data_file
 *         249: cannot find font_data_file
 *         250: bad parameter
 *         251: old and new eufm
 *         252: bad dvi file
 *         253: error in dvi file
 *         254: not dvi file
 *         255: cannot find dvi file
 *
 *
 *  1 point = 65536(=2^(16)) scaled point
 *  1 inch  = 72.27 point = 2.54 cm
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef UNIX
#include <dos.h>
#ifdef MSVC
#include "msvcdir.h"
#else
#include <dir.h>
#endif
#include <io.h>
#endif
#include <string.h>

#ifdef	GCC
#include <ctype.h>
#endif

#include "dd.h"

#define	ID			2
#define ID_PTEX       3
#define	END_DVI		223
#define	MAX_NAME	16
#define	MAX_FONT	1024
#define	BUF_SIZE	1024

#define	HALF_POINT	(1L<<8)

#define	NEW_FONT	1
#define	NEW_POINT	2
#define	NEW_EUFM	4
#define	BAD_EUFM	8
#define	OLD_FONT	16
#define	OLD_EUFM	32

#define	DVI_FILE	0
#define	TFM_FILE	1
#define	PXL_FILE	2
#define	PK_FILE		3

char buf[BUF_SIZE];
char filename[MAXPATH];

/*
 * eufm: point, checksum old, checksum new
 */

struct EUFM {
	int point;	/*	point number	*/
	unsigned int oldd;	/*	old checksum	*/
	unsigned int neww;	/* 	new checksum	*/
};

/*
	 	Multiple lines for a point are allowed.
		The last line should be 0 point.
 */
struct EUFM eufm[] =
{
	5, 0x0891FD38L, 0x246A686BL,
	5, 0x0891FD38L, 0xB8C9AFD4L,
	6, 0x2F8EDF02L, 0x9793B1E1L,
	6, 0x2F8EDF02L, 0x65167BC4L,
	7, 0x37CDA321L, 0x05184980L,
	7, 0x37CDA321L, 0x58ED2602L,
	8, 0x37CCA321L, 0x2564B2F5L,
	8, 0x37CCA321L, 0x5C978225L,
	9, 0x3F977CF3L, 0x1F6C4283L,
	9, 0x3F977CF3L, 0xF833F3AFL,
	10, 0xBF989013L, 0x8F256EB2L,
	0, 0L, 0L
};

struct CHKFONT {
	char f_name[MAX_NAME];	/* font name with point */
	unsigned long oldd;	/* old checksum */
	unsigned long neww;	/* new checksum */
};

struct CHKFONT chkfont[MAX_FONT];

/*
 * old AMS FONTS
 */

char *oldfont[] =
{"msxm", "msym", "amcsc", "mcyr", "mcyb", ""};

/*
 * add CM fonts, strat point, end point
 */

struct NEWPOINT {
	char *font_name;	/* font name		*/
	int top;	/* smallest point	*/
	int end;	/* lagest point		*/
};

struct NEWPOINT newpoint[] =
{
	"cmbsy", 5, 9,
	"cmcsc", 8, 9,
	"cmex", 7, 9,
	"cmmib", 5, 9,
	"", 0, 0
};

/*
 * new AMS FONTS
 */

char *newfont[] =
{"euex", "eufb", "eurb", "eurm", "eusb", "eusm",
 "msam", "msbm",
 "wncyb", "wncyi", "wncyr", "wncysc", "wncyss",
 ""
};

struct FONT_ATR {
	/* font_def�̃p�����[�^�[ ; See "TeX:The program". */
	long k, c, s, d;
	int a, l;
	char *n;
};

int mag[] =
{1095, 1200, 1315, 1440, 1577, 1728, 1893, 2074, 2272, 2488,
 2726, 2986, 3271, 3583, 3925, 4300, 4710, 5160, 32000};

char *font_ext[] = {
	"gf", "pk", "pxl", "far", "gth", NULL
};

#define	read_byte(x)	(uchar)getc(x)

void usage(void);
char *name_link(char *, char *);
void read_post(DVIFILE_INFO *);
void get_font_list(DVIFILE_INFO *);
void font_define(int, FILE *);
int read_uint(FILE *);
long read_mit(FILE *);
long read_long(FILE *);
char *read_str(FILE *, int);
long read_n(FILE *, int);
long signed_read_n(FILE *, int);
void check_font(void);
void usage(void);
void show_dvi_data(DVIFILE_INFO *);
void get_list(char *);
void type(char *);
void tfm_define(FILE *);
void pxl_define(FILE *);
int chk_font_file(FILE *);
int check_mag(int);
long to_long(uchar *);

DVIFILE_INFO dvi_info;
struct FONT_ATR font;

int f_v = -1;	/* verbose mode */
int f_d;	/* ignore default data */
int f_t;	/* file_type */
int n_data;	/* number of font_data */
uint f_type;
long pmag;
long hppp;
long vppp;

struct FINFO {
	char f_atrib;
	uchar f_time[2];
	uchar f_date[2];
	uchar f_size[4];
	char f_name[14];
	uchar f_pos[4];
};

struct FARDIR {
	char f_name[10];
	uchar f_time[4];
	uchar f_size[4];
	uchar f_pos[4];
};

uchar gth_buf[sizeof(struct FINFO)];
long f_end;
int  f_cont;

int	strcmpl(const char *s, const char *t)
{
	int	i;

        i = strlen(t);
        if(strlen(s) < i ) return -1;
        s += strlen(s);
        while(i-- > 0){
                s--;
                if(toupper(*s) != toupper(t[i]))
		        return -1;
	}
        return 0;
}


#ifdef UNIX
char *strlwr(char *p)
{
  char *original = p;

  while (*p != '\0') {
  	*p = tolower(*p);
  	++p;
  }
  return original;
}
#endif

void main(int argc, char **argv)
{
	int i;
	char *pt;

	if (argc < 2)
		usage();
	for (i = 1; i < argc - 1; i++) {
		if (argv[i][0] != '-')
			usage();
		switch (argv[i][1]) {
		  case 'S':
		  case 's':
			  f_v = 0;
			  break;
		  case 'c':
			  f_v = 1;
			  break;
		  case 'F':
			  f_d = 1;
		  case 'f':
			  get_list(argv[i] + 2);
			  break;
		  case 'd':
			  f_t = -1;
			  break;
		  case 't':
			  f_t = 1;
			  break;
		  case 'p':
			  f_t = 2;
			  break;
		  default:
			  usage();
		}
	}

				/* check extension */
	if (!f_t){
		pt = argv[argc-1];
		i = strlen(pt);
		while(--i > 0){
			if( pt[i] == '.' ) break;
			if( pt[i] == '\\' || pt[i] == '/' || pt[i] == ':' ){
				i = 1;
			}
		}
		if (i > 0){
			pt += i+1;
			if( strcmpl(pt, "dvi") == 0 ) f_t = -1;
			else{
				if( strcmpl(pt, "tfm") == 0 ) f_t = 1;
				else{
					for(i = 0; font_ext[i]; i++){
						if( strcmpl(pt, font_ext[i]) == 0 ){
							f_t = 2;
							break;
						}
					}
				}
			}
		}
	}
	if(f_t == -1) f_t = 0;
	strcpy(filename, dvi_info.file_name = argv[argc - 1]);
	if (f_t != PXL_FILE && strchr(argv[argc - 1], '.') == NULL)
		dvi_info.file_name = name_link(argv[argc - 1],
									   (f_t) ? "tfm" : "dvi");
	if ((dvi_info.file_ptr = fopen(dvi_info.file_name, "rb")) == NULL) {
		fprintf(stderr, "Cannot find %s\n", dvi_info.file_name);
		exit(255);
	}
	if (f_t != DVI_FILE) {
		font.n = dvi_info.file_name;
		if (f_t == TFM_FILE)
			tfm_define(dvi_info.file_ptr);
		else {
			if (chk_font_file(dvi_info.file_ptr))
				pxl_define(dvi_info.file_ptr);
		}
	}
	else {
		read_post(&dvi_info);
		if (f_v)
			show_dvi_data(&dvi_info);
		get_font_list(&dvi_info);
		if (f_v != -1
			&& (f_type & (OLD_EUFM + NEW_EUFM)) == (OLD_EUFM + NEW_EUFM)) {
			fprintf(stderr, "old and new eufm fonts\n");
			exit(251);
		}
	}
	if (f_v != -1) {
		if (f_v != 0)
			printf("\n\n");
		printf("EXIT Code (errorlevel)\t\t= %d\n", f_type);
		if (f_type & OLD_EUFM)
			printf("Old eufm ");
		if (f_type & NEW_EUFM)
			printf("New eufm ");
		if (f_type & OLD_FONT)
			printf("Old font ");
		if (f_type & (NEW_FONT | NEW_POINT))
			printf("New font");
		if (f_type == 0)
			printf("No AMS Font");
	}
	printf("\n");
	exit(f_type);
}

int chk_font_file(FILE * fp)
{
	int i, f_num;
	long start;
	struct FINFO *finfo;
	struct FARDIR *fardir;

	finfo = (struct FINFO *)gth_buf;
	fardir = (struct FARDIR *)gth_buf;

	fread(gth_buf, sizeof(struct FINFO), 1, fp);

	if (strcmp(gth_buf + 8, "gather") == 0)
		goto gth;

	fseek(fp, -8L, SEEK_END);
	f_num = read_uint(fp);
	start = read_long(fp);
	if (read_uint(fp) != (int)('f' + 'o' + 'n' + 't' + 'a' + 'r' + 'c' + 'h' + 'i' + 'v' + 'e')) {
		fseek(fp, 0L, SEEK_SET);
		return (1);
	}
	/* FAR file */
	if (f_v){
		f_cont = 1;
		printf("\t\"%s\" is a FAR file which contains...\n", filename);
	}
	while (f_num-- > 0) {
		if (f_v)
			printf("\n");
		fseek(fp, start, SEEK_SET);
		fread((char *)fardir, sizeof(struct FARDIR), 1, fp);

		start = ftell(fp);
		font.n = fardir->f_name;
		f_end = to_long(fardir->f_pos) + to_long(fardir->f_size);
		fseek(fp, to_long(fardir->f_pos), SEEK_SET);
		pxl_define(fp);
		if (f_v)
			printf("\n");
	}
	return (0);

  gth:							/* GTH file */
	if (f_v){
		f_cont = 1;
		printf("\t\"%s\" is a GTH file which contains...\n", filename);
	}
	start = gth_buf[0] + (gth_buf[1] << 8) + (gth_buf[2] << 16)
                + (gth_buf[3] << 24);
	f_num = gth_buf[4] + (gth_buf[5]<< 8);
	while (f_num-- > 0) {
		if (f_v)
			printf("\n");
		fseek(fp, start, SEEK_SET);
		fread(gth_buf, sizeof(struct FINFO), 1, fp);

		start = ftell(fp);
		font.n = finfo->f_name;
		printf("%s:%d(%d)\n", 
			font.n, to_long(finfo->f_pos), to_long(finfo->f_size));
		f_end = to_long(finfo->f_pos) + to_long(finfo->f_size);
		fseek(fp, to_long(finfo->f_pos), SEEK_SET);
		pxl_define(fp);
		if (f_v)
			printf("\n");
	}
	return (0);
}

void show_dvi_data(DVIFILE_INFO *dvi)
{
	int len, x, y;
	long s_width, s_hight;

	printf("dvi file name\t\t\t= %s\n", dvi->file_name);
	fseek(dvi->file_ptr, 14L, SEEK_SET),
		len = (uchar)read_byte(dvi->file_ptr);
	printf("comment\t\t\t\t=%s\n", read_str(dvi->file_ptr, len));
	printf("factor converting to 10^{-5}m\t= %ld/%ld\n",
		   dvi->num, dvi->den);
	printf("magnification\t\t\t= %ld\n", dvi->mag);
	printf("maximum width\t\t\t= %ld scaled points = %ld points\n",
		   dvi->u, (dvi->u + HALF_POINT) >> 16);
	printf("maximum height+depth\t\t= %ld scaled points = %ld points\n",
		   dvi->l, (dvi->l + HALF_POINT) >> 16);
	printf("maximum stack depth\t\t= %d\n", dvi->stack_depth);
	printf("number of pages\t\t\t= %d\n", dvi->total_page);
	s_width = ((dvi->u >> 6) * 2540) / 7227;	/* scated 2^(-10) mm */
	s_hight = ((dvi->l >> 6) * 2540) / 7227;	/* scaled 2^(-10) mm */
	x = (s_width + 512) / 1024;
	y = (s_hight + 512) / 1024;
	printf("page size\t\t\t= %2d.%d cm x %2d.%d cm\n",
		   x / 10, x % 10, y / 10, y % 10);
	s_width = s_width * dvi->mag / 1000;
	s_hight = s_hight * dvi->mag / 1000;
	x = (s_width + 512) / 1024;
	y = (s_hight + 512) / 1024;
	printf("page size magnificated\t\t= %2d.%d cm x %2d.%d cm",
		   x / 10, x % 10, y / 10, y % 10);
}

int check_mag(int scale)
{
	int i;

	for (i = 0; scale > mag[i] + 1; i++);
	if (scale + 1 > mag[i])
		return (i + 1);
	return (-1);
}

void check_font(void)
{
	uchar point;
	char *pt;
	int ratio, cmag;
	int j;
	int f_point;

	if (f_v) {
		if (f_t == DVI_FILE)
			printf("\n\nFont %ld\n", font.k);
		printf("\tchecksum\t\t= %08lX\n", font.c);
		if (f_t != DVI_FILE) {
			printf("\tdesign size\t\t= %ld 2^{-20} points = %ld points",
				   font.d, font.d >> 20);
		}
		else {
			printf("\tdesign size\t\t=%9ld scaled points =%3ld points",
				   font.d, font.d >> 16);
		}
		switch (f_t) {

		  case (PK_FILE):
			  printf("\n\thorizontal\t\t= %ld 2^{-16} dots/point = %ld dpi",
					 hppp, ((((hppp >> 4) * 7227) >> 12) + 49) / 100);
			  printf("\n\tvertical\t\t= %ld 2^{-16} dots/point "
					 "= %ld dpi\n\t\t\t\t",
					 vppp, ((((vppp >> 4) * 7227) >> 12) + 49) / 100);
			  break;

		  case (PXL_FILE):
			  printf("\n\tmagnification\t\t= %ld (%ld dpi)\n\t\t\t\t",
					 pmag, (pmag + 2) / 5);
			  break;

		  case (TFM_FILE):
			  printf("\n\t\t\t\t");
			  break;

		  case (DVI_FILE):
			  printf("\n\tspace size\t\t=%9ld scaled points =%3ld points",
					 font.s, font.s >> 16);
			  ratio = 1000;
			  cmag = 0;
			  if (font.d > font.s)
				  ratio = (font.d * 100L + (font.s / 20L)) / (font.s / 10L);
			  else if (font.s > font.d)
				  ratio = (font.s * 100L + (font.d / 20L)) / (font.d / 10L);
			  if (ratio != 1000)
				  cmag = check_mag(ratio);
			  if (cmag > 0) {
				  printf("\n\tmagnification\t\t= %smagstep%d%s",
						 (font.d > font.s) ? "-" : "+", cmag / 2, (cmag & 1) ? ".5" : "");
			  }
			  else if (cmag < 0)
				  printf("\n\tmagnification\t\t= %d",
						 (font.d > font.s) ? ((int)((1000000L + ratio) / ratio)) : ratio);
			  printf("\n\tfont name\t\t= %s", font.n);
			  break;
		}
	}
	if (f_v == -1)
		return;
	for (j = 0; j < n_data; j++) {
		if (strcmp(chkfont[j].f_name, font.n) == 0) {
			if (chkfont[j].oldd == 0 || chkfont[j].oldd == font.c) {
				type("Old font");
				f_type |= OLD_FONT;
				return;
			}
			if (chkfont[j].neww == 0 || chkfont[j].neww == font.c) {
				type("New font");
				f_type |= NEW_FONT;
				return;
			}
		}
	}
	if (f_d)
		return;

	pt = font.n + font.a + font.l - 1;
	while (*pt >= '0' && *pt <= '9')
		pt--;
	point = atoi(++pt);
	*pt = 0;

	if (strcmp(font.n, "eufm") == 0) {
		j = f_point = 0;
		while (eufm[j].point != 0) {
			if (eufm[j].point == point) {
				f_point = 1;
				if (eufm[j].oldd == font.c) {
					f_type |= OLD_EUFM;
					type("Old AMSTeX");
					return;
				}
				else {
					if (eufm[j].neww == font.c) {
						f_type |= NEW_EUFM;
						type("AMSTeX 2.0/2.1");
						return;
					}
				}
			}
			j++;
		}
		f_type |= BAD_EUFM;
		if (f_v)
			type(
					(f_point) ? "unknown checksum" : "unknown point size");
		return;
	}
	for (j = 0; oldfont[j][0] != 0; j++) {
		if (strcmp(font.n, oldfont[j]) == 0) {
			f_type |= OLD_FONT;
			type("Old AMS Font");
			return;
		}
	}

	for (j = 0; newfont[j][0] != 0; j++) {
		if (strcmp(font.n, newfont[j]) == 0) {
			f_type |= NEW_FONT;
			type("New AMS Font");
			return;
		}
	}

	for (j = 0; newpoint[j].font_name[0] != 0; j++) {
		if (strcmp(font.n, newpoint[j].font_name) == 0) {
			if (newpoint[j].top <= point && newpoint[j].end >= point) {
				f_type |= NEW_POINT;
				type("New AMS Font");
			}
			return;
		}
	}
}

char *
    name_link(char *base, char *ext)
	/* �x�[�X���ƁC�g���q����������D
     */
{
	char *name;
	int base_len;

	base_len = strlen(base);
	name = (char *)malloc(base_len + 5);
	strcpy(name, base);
	name[base_len] = '.';
	strcpy(name + base_len + 1, ext);
	return (name);
}

void read_post(DVIFILE_INFO *dvi)
	/* POSTAMBLE����̃f�[�^�ǂݏo���D
     */
{
	long endofs;
	int code;

	if ((uchar)read_byte(dvi->file_ptr) != PRE ||
		(uchar)read_byte(dvi->file_ptr) != ID) {
	  err:fprintf(stderr, "%s is not correct DVI file\n", dvi->file_name);
		exit(254);
	}
	for (endofs = -3L; fseek(dvi->file_ptr, endofs, SEEK_END),
		 (code = (uchar)read_byte(dvi->file_ptr)) != ID; endofs--)
		/* Search id number
             */
		if (code == ID_PTEX) {
			printf("This is extended DVI file [ID = 3] for pTeX.\n");
			break;
		}
		else if (code == EOF || code != END_DVI)
			goto err;

	fseek(dvi->file_ptr, endofs - 4L, SEEK_END);
	if ((dvi->post = read_long(dvi->file_ptr)) <= 0)
		goto err;
	/* Read the position of POSTAMBLE */

	fseek(dvi->file_ptr, dvi->post, SEEK_SET);
	/* Set file-ptr at POSTAMBLE */

	if ((uchar)read_byte(dvi->file_ptr) != POST)
		goto err;

	if ((dvi->last_bop = read_long(dvi->file_ptr)) <= 0)
		goto err;

	dvi->num = read_long(dvi->file_ptr);
	dvi->den = read_long(dvi->file_ptr);
	dvi->mag = read_long(dvi->file_ptr);

	dvi->l = (SCALED_PT)read_long(dvi->file_ptr);
	dvi->u = (SCALED_PT)read_long(dvi->file_ptr);

	dvi->stack_depth = read_uint(dvi->file_ptr);
	dvi->total_page = read_uint(dvi->file_ptr);

	if (dvi->num <= 0 || dvi->den <= 0 || dvi->mag <= 0)
		goto err;

	if (dvi->stack_depth < 0 || dvi->total_page <= 0)
		goto err;
}

void get_font_list(DVIFILE_INFO *dvi)
{
	int code, count;

	fseek(dvi->file_ptr, dvi->post + 29, SEEK_SET);
	while ((code = getc(dvi->file_ptr)) != POST_POST)
		if (code >= FNT_DEF_1 && code < FNT_DEF_1 + 4) {
			font_define(code, dvi->file_ptr);
			check_font();
		}
		else if (code != NOP) {
			fprintf(stderr, "Command error code:%d\n", code);
			exit(253);
		}
}

void tfm_define(FILE * fp)
{
	int i, ch, bc, ec;
	long sum, size;
	char *s, *t, *u;

	font.n[strlen(font.n) - 4] = 0;
	s = font.n + strlen(font.n);
	while (*--s >= '0' && *s <= '9' && s >= font.n);
	t = s;
	while (*s != ':' && *s != '\\' && *s != '/' && s-- > font.n);
	font.n = s + 1;
	font.a = t - s;
	font.l = 0;

	ch = 't';
	u = "";

	if ((i=read_uint(fp)) == 11 || i == 9) {	/* lf */
		read_long(fp);
		ch = 'j';
		if( i == 9 ) u = "(tate)";
	}
	read_uint(fp);				/* lh */
	bc = read_uint(fp);			/* bc */
	ec = read_uint(fp);			/* ec */
	if (bc < 0 || bc > 256 || bc > ec || ec > 256) {
		printf("\n\"%s\" is not a tfm file\n", filename);
		exit(254);
	}
	if (f_v != 0)
		printf("\t\"%s\" is a %cfm%s file :%3d  -> %3d\n",
			font.n, ch, u, bc, ec);
	for (i = 0; i < 4; i++)
		read_long(fp);
	font.c = read_long(fp);		/* header[0] */
	font.d = read_long(fp);		/* header[1] */
	fclose(fp);
	check_font();
}

#define PK_PRE		247
#define PK_ID		89
#define	GF_ID		131
#define	PKD_ID		('p'+'k'+'d')
#define PK_POST		245
#define GF_POST		223

#define	GFW_ID		((PK_PRE<<8)+GF_ID)
#define	PKW_ID		((PK_PRE<<8)+PK_ID)
#define	PKDW_ID		((PK_PRE<<8)+PKD_ID)
#define	JXL4_0_ID	(0x0001)
#define	JXL4_1_ID	(0x8B25)

void pxl_define(FILE * fp)
{
	int i, tmp, ch, code;
	char *type, *s, *t, *u;
	long endofs;

	strlwr(font.n);
	s = u = font.n + strlen(font.n);
	ch = 0;
	while (*s != ':' && *s != '\\' && *s != '/' && *s != '.' &&
		   s-- >= font.n);
	if (*s == '.') {
		ch = *(u = s);
		*s = 0;
	}
	s = font.n + strlen(font.n);
	while (*--s >= '0' && *s <= '9' && s >= font.n);
	t = s;
	while (*s != ':' && *s != '\\' && *s != '/' && s-- > font.n);
	font.n = s + 1;
	font.a = t - s;
	font.l = 0;

	switch (tmp = read_uint(fp)) {
	  case (0):
		  switch (tmp = read_uint(fp)) {
			case (1001):
				type = "PXL1001";
				break;

			case (1002):
				type = "PXL1002";
				break;

			case (1003):
				type = "PXL1003";
				break;

			default:
				printf("\nUnknown id:0000%04X\n", tmp);
				exit(254);
		  }
		  if (f_end == 0)
			  fseek(fp, -20L, SEEK_END);
		  else
			  fseek(fp, f_end - 20, SEEK_SET);
		  font.c = read_long(fp);
		  pmag = read_long(fp);
		  font.d = read_long(fp);
		  break;

	  case (JXL4_0_ID):
		  if (read_long(fp) == JXL4_1_ID) {
			  type = "JXL4";
			  if (f_end == 0)
				  fseek(fp, -28L, SEEK_END);
			  else
				  fseek(fp, f_end - 28, SEEK_END);
			  font.c = read_long(fp);
			  pmag = read_long(fp);
			  font.d = read_long(fp);
		  }
		  else
			  goto id_err;
		  break;

	  case (GFW_ID):
		  type = "GF";
		  for (endofs = -3L; fseek(fp, endofs, SEEK_END),
			   (code = (uchar)read_byte(fp)) == GF_POST; endofs--);
		  /* Search id number	*/
		  if (code != GF_ID) {
			  fprintf(stderr, "This is not correct GF file\n");
			  exit(252);
		  }
		  fseek(fp, endofs - 4, SEEK_END);
		  endofs = read_long(fp);
		  fseek(fp, endofs + 5, SEEK_SET);
		  goto pk3;

	  case (PKDW_ID):
		  type = "PKD";
		  goto pk2;

	  case (PKW_ID):
		  type = "PK";
		pk2:tmp = read_byte(fp);
		  while (tmp--)
			  (void)read_byte(fp);
		pk3:f_t = PK_FILE;
		  font.d = read_long(fp);
		  font.c = read_long(fp);
		  hppp = read_long(fp);
		  vppp = read_long(fp);
		  break;

	  default:
		id_err:
		  printf("\nUnknown id:%04X\n", tmp);
		  exit(254);
	}
	if (f_v != 0) {
		*u = ch;
		printf("\t\"%s\" is in %s format\n", font.n, type);
		*u = 0;
	}
	check_font();
}

void font_define(int code, FILE * dvifile)
{
	int i, len, lenm;

	font.k = read_n(dvifile, code - FNT_DEF_1 + 1);
	font.c = read_long(dvifile);
	font.s = read_long(dvifile);
	font.d = read_long(dvifile);
	font.a = read_byte(dvifile);
	font.l = read_byte(dvifile);

	if (font.a == EOF || font.l == EOF) {
		fprintf(stderr, "Bad DVI file\n");
		exit(252);
	}
	font.n = read_str(dvifile, font.a + font.l);
}

int read_uint(FILE * fp)
{	   /* read signed integer */
	int d;

	d = (uchar)getc(fp);
	d = (d << 8) | (uchar)getc(fp);
	return (d);
}

long read_mit(FILE * fp)
{	   /* read three bytes as a long integer */
	long d;

	d = (uchar)getc(fp);
	if (d > 127)
		d -= 256;
	d = (d << 8) | (uchar)getc(fp);
	d = (d << 8) | (uchar)getc(fp);
	return (d);
}

long read_long(FILE * fp)
{	   /* read 4 bytes as long integer */
	long d;

	d = (uchar)getc(fp);
	if (d > 127)
		d -= 256;
	d = (d << 8) | (uchar)getc(fp);
	d = (d << 8) | (uchar)getc(fp);
	d = (d << 8) | (uchar)getc(fp);
	return (d);
}

long to_long(uchar *pt)
{	   /* read 4 bytes as long integer */
	long d;

	d = (pt[3] << 8) + pt[2];
	d = (d << 16) + (pt[1] << 8) + pt[0];
	return (d);
}


char *
    read_str(FILE * fp, int len)
{
	char *pt0, *pt;

	if ((pt = pt0 = (char *)malloc(len + 1)) != NULL) {
		while (len-- > 0)
			*pt++ = getc(fp);
		*pt = '\0';

	}
	return (pt0);
}

long read_n(FILE * fp, int n)
{
	ulong d;

	d = (uchar)getc(fp);

	switch (n) {
	  case 4:
		  d = (d << 8) | (uchar)getc(fp);
	  case 3:
		  d = (d << 8) | (uchar)getc(fp);
	  case 2:
		  d = (d << 8) | (uchar)getc(fp);
	}
	return ((long)d);
}

long signed_read_n(FILE * fp, int n)
{
	long d;

	d = (uchar)getc(fp);
	if (d > 127)
		d -= 256;
	switch (n) {
	  case 4:
		  d = (d << 8) | (uchar)getc(fp);
	  case 3:
		  d = (d << 8) | (uchar)getc(fp);
	  case 2:
		  d = (d << 8) | (uchar)getc(fp);
	}
	return (d);
}

void type(char *s)
{
	if (f_v)
		printf(" (%s) ", s);
}

void get_list(char *fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Cannot find %s.\n", fname);
		exit(249);
	}
	for (n_data = 0; n_data < MAX_FONT;) {
		if (fgets(buf, BUF_SIZE, fp) == NULL)
			return;
		if (buf[0] == '#' || buf[0] == ';')
			continue;
		if (sscanf(buf, "%15s %X %X",
				   chkfont[n_data].f_name,
				   &(chkfont[n_data].oldd),
				   &(chkfont[n_data].neww)) != 3) {
			fprintf(stderr, "error in %s[%s]\n", fname, buf);
			exit(248);
		}
		if (chkfont[n_data].oldd == chkfont[n_data].neww) {
			fprintf(stderr, "same check sum in %s[%s]\n", fname, buf);
			exit(248);
		}
		n_data++;
	}
	if (n_data >= MAX_FONT) {
		fprintf(stderr, "Too many font data in %s\n", fname);
		exit(248);
	}
}

void usage()
{
	fprintf(stderr,
		"<<< CHecK dvi/tfm/gf/pk/pkd/pxl/gth/far file and tell informations"
			" of FONTs >>>\n"
			"\t\t\t\tVer 1.7a, written by SHIMA, 1990/1992\n\n"
	  "Usage: chkfont [-s] [-c] [-{f|F}<font_data_file>] [-d] dvi_file_name\n"
	  "       chkfont [-s] [-c] [-{f|F}<font_data_file>] [-t] tfm_file_name\n"
	"       chkfont [-s] [-c] [-{f|F}<font_data_file>] [-p] font_file_name\n\n"
			"Option\t -c: check mode\n"
			"\t -s: silent check mode\n"
			"\t -f: use font_data_file (-F: and ignore default data)\n\n"
			"Input  M  to see more.  Push Return to quit");
	switch (getc(stdin)) {
	  case ('M'):
	  case ('m'):
		  goto show;
	  default:
		  exit(250);
	}
 exit(1);

  show:
	fprintf(stderr,
			"\n\nErrorlevel is the sum of the following number\n"
			"\t  1: cmbsy5-9, cmcsc8-9, cmex7-9, cmmib5-9\n"
			"\t  2: new font: eu??, msam, msbm, wncy??, font_data_file\n"
			"\t  4: new eufm5-10\n"
			"\t  8: unknown eufm\n"
		  "\t 16: old font: msxm, msym, amcsc, mcyr, mcyb, font_data_file\n"
			"\t 32: old eufm5-10\n"
			"If errorlevel is larger than 240,\n"
			"\t248: error in font_data_file\n"
			"\t249: cannot find font_data_file\n"
			"\t250: bad parameter\n"
			"\t251: old and new eufm\n"
			"\t252: bad dvi file\n"
			"\t253: error in dvi file\n"
			"\t254: not dvi/tfm file or unknown font ID\n"
			"\t255: cannot find dvi/tfm/font file\n\n"
			"\t\t<<< Example of font_data_file >>>\n"
			"# font_name old_check_sum new_check_sum  (in HEX, 0 means existence check)\n"
			"eufm5	0891FD38	246A686B\n"
			"eufm10	BF989013	8F256EB2\n"
			"# The line beginning with # or ; is ignored.\n"
		);
	exit(250);
}
