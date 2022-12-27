/*
   \work\tools\source\bmptool\bmptool.c

   James Tidman

   Last Known Address:
   
		bmp2asc@tidmanfamily.com

   Created:
   	04/15/99 - JT
   	
   Modified:         
   

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <windows.h>


BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

// defines
#ifndef TRUE
	#define TRUE	(1==1)
	#define FALSE	(1==0)
#endif
            
#define UC unsigned char	// 8 bit values
#define SC signed char
#define UI unsigned int		// 16 bit values
#define SI int
#define UL unsigned long	// 32 bit values
#define SL long

// global variables
FILE *in, *out;

// functions            

main (int argc, char *argv[])
{
UI h, c, d, dsave;
UC *cmap, *bmap;

UI bytewidth;
UL bytewidthl;

UC currentcolor, newcolor;

SI startXloc = 5348, startYloc = 50;
SI level = 26, width = 1;
SI polycount = 0;


printf("BMP2ASC: Used to convert images into Pads PCB format.\n");
printf("Copyright(c) 1999 James Tidman (jtidman@@flash.net).\n");
printf("V0.01 \nFreely distributable in unmodified form.\n");
printf("No warranty express or implied in the use of this program.\n");
printf("If you like it, or have questions, drop me an email.\n\n");


if (argc != 7)
	{
	printf("Usage:\n");
	printf("BMP2ASC inputfilename outputfilename integerscale levelnumber integerXoffset integerYoffset\n");
	return -1;
	}

if ((in = fopen(argv[1], "rb")) == NULL)
	{
	printf("Unable to open input file <%s>.\n", argv[1]);
	return -2;
	}

if ((out = fopen(argv[2], "wb")) == NULL)
	{
	printf("Unable to open output file <%s>.\n", argv[1]);
	return -2;
	}

width = atoi(argv[3]);
level = atoi(argv[4]);
startXloc = atoi(argv[5]);
startYloc = atoi(argv[6]);


//////////////////////////////////////////////////////////////////////////////
//
// read bitmap header
//
//

// read header structures
fread(&bfh, sizeof(bfh), 1, in);

fread(&bih, sizeof(bih), 1, in);

// verify we're dealing with 3.0 BMP format
if ((bih.biSize != 40) && (bfh.bfType != 0x424d))
	{
	printf("Not Windows 3.0 format!\n");
	exit(-1);
	}

// verify that its a black and white (1bit per pixel) image.
if ((bih.biPlanes != 1) && (bih.biBitCount != 1))
	{
	printf("Not a black and white (1 bit per pixel) image!\n");
	exit(-1);
	}


// load the pallete (unused)
if (bih.biClrUsed == 0)
	{
	bih.biClrUsed = 1 << bih.biBitCount;
	}

if ((cmap = malloc((size_t)bih.biClrUsed * 4)) == NULL)
	{
	printf("Not enough memory.\n");
	exit(-2);
	}

fread(cmap, (size_t)bih.biClrUsed * 4, 1, in);


// print .ASC file header
fprintf(out, "!PADS-POWERPCB-V2.1-MILS! DESIGN DATABASE ASCII FILE 1.0\n");
fprintf(out, "*LINES*      LINES ITEMS\n\n");

fprintf(out, "*REMARK*  NAME  TYPE  XLOC YLOC PIECES TEXT\n");
fprintf(out, "*REMARK*  PIECETYPE  CORNERS WIDTH LEVEL PINNUM\n");
fprintf(out, "*REMARK*  XLOC YLOC BEGINANGLE DELTAANGLE\n");
fprintf(out, "*REMARK*  XLOC YLOC ORI  LEVEL  HEIGHT WIDTH  MIRRORED\n\n");



// calculate byte width
bytewidthl = (UL)bfh.bfSize - ((UL)sizeof(bfh) + (UL)sizeof(bih) + (UL)(bih.biClrUsed * 4));
bytewidthl /= (UL)bih.biHeight;
bytewidth = (UI)bytewidthl;
//bytewidth = ((UI)bih.biWidth * (UI)bih.biBitCount) >> 4;
//if (((UI)bih.biWidth * (UI)bih.biBitCount) & 0x0F) bytewidth+=2;	// round up
//bytewidth<<=1;

//////////////////////////////////////////////////////////////////////////////
//
// read in bitmap lines
//
//

// allocate memory for 1 line of storage
if ((bmap = malloc((size_t)bytewidth)) == NULL) {
	printf("Not enough memory.\n");
	exit(-2);
	}

// loop on and process each line
for (h = 0; h < (UI)bih.biHeight; h++) {

                                                                    
	// read in line	
	c = fread(bmap, 1, (size_t)bytewidth, in);
	
	if (c != bytewidth) {
		printf("Error reading file.\n");
		exit(-3);
		}

	// scan until the end of the bitmap is found
	currentcolor = 2;
	for (d = 0; d <= (UI)bih.biWidth; d++) {
		
		// check for end of line
		if (d == (UI)bih.biWidth) {
			if (currentcolor == 1) { 
				// end line
				fprintf(out, "%d     0\n", ((d - dsave - 1) * width)+1);
			}
		
		} else {		          
			// if current color != to color in bitmap start/end new line          
			newcolor = (bmap[d >> 3] & (0x80 >> (d & 0x07))) >> (7 - (d & 0x07));
			if ( newcolor != currentcolor) {
				if (newcolor == 0)  {
				
					// end line
					if (currentcolor != 2) {
						fprintf(out, "%d     0\n", ((d - dsave - 1) * width)+1);
					}
					
				} else {
				   // start line
				   fprintf(out, "JTPOLY%8.8d          LINES    %d    %d   1\n", 
				   			polycount++, startXloc + (d * width), startYloc + (h * width));
					fprintf(out, "OPEN   2   %d  %d\n", width, level);
					fprintf(out, "0      0\n");
					dsave = d;
				}
	      }
		currentcolor = newcolor;
		}
	}	// end of loop on d
}	// end of loop on h
	
	
// print .ASC file trailer
fprintf(out, "\n*END*     OF ASCII OUTPUT FILE\n\n");


// cleanup	
fcloseall();

free(bmap);	

free(cmap);

}
