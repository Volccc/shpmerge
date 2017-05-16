//
//  main.c
//  shpmerge
//
//  Created by Vladimir Borisov on 12/05/2017.
//  Copyright © 2017 Home. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "bool.h"
#include "shapefil.h"

#ifndef _WIN32
#define _MAX_PATH 2048
#endif

#define TYPE_FIELD "TYPE"
#define MINCAT_FIELD "MIN_CAT"

#define ISOBATHS_NAME "Isobaths"
#define MAJOR_NAME "Major Contours"
#define MINOR_NAME "Minor Contours"

#define MAX_CONGIG_DEPTH 100

int resultCount;

static int colorDef[MAX_CONGIG_DEPTH] = {
	// 0	1		2		3
	0x400,	0x400,	0x401,	0x401,
	// 4	5		6		7
	0x402,	0x402,	0x403,	0x403,
	// 8	9		10		11
	0x404,	0x404,	0x405, 0x405,
	// 12	13		14		15
	0x406,	0x406,	0x407, 0x407,
	// 16	17		18		19
	0x408,	0x408,	0x409,	0x409,
	// 20	21		22		23
	0x40A,	0x40A,	0x40B,	0x40B,
	// 24	25
	0x40C,	0x40D
};

#define TYPE_FIELD "TYPE"
#define MINCAT_FIELD "MIN_CAT"

int getAreaType(int d) {
	if (d < 0 || d > MAX_CONGIG_DEPTH) {
		return 0x400;
	}
	return colorDef[d];
}

int getDepType(const char* buf)
{
	int dep;
	int ns = sscanf(buf, "%d", &dep);
	if (ns != 1) {
		printf("Wrong MIN_CAT!!!");
	}
	else {
		return getAreaType(dep);
	}
	
	return 0x400;
}

BOOL proceedType(DBFHandle handle) {
	if (handle == NULL) {
		return FALSE;
	}
	int count = DBFGetRecordCount(handle);
	
	int typeIndex = DBFGetFieldIndex(handle, TYPE_FIELD);
	int mincatIndex = DBFGetFieldIndex(handle, MINCAT_FIELD);
	
	int newtype = 0;
	const char* sMinCat;
	
	for (int i = 0; i < count; i++) {
		sMinCat = DBFReadStringAttribute(handle, i, mincatIndex);
		newtype = getDepType(sMinCat);
		
		DBFWriteIntegerAttribute(handle, i, typeIndex, newtype);
//		printf("Category: %s; New type: %d (0x%x)\n", sMinCat, newtype, newtype);
	}
	DBFClose(handle);
	printf("TYPE corrected\n");
	return TRUE;
}

BOOL checkType(DBFHandle handle) {
	int index;
 
	index = DBFGetFieldIndex(handle, TYPE_FIELD);
	if (index >= 0) {
		// RM1, nothing to do
		return 1;
	}
	index = DBFAddField(handle, TYPE_FIELD, FTInteger, 11, 0);
	if (index >= 0) {
		return 1;
	}
	
	return 0;
}

DBFHandle openDb() {
	char* outName = malloc(_MAX_PATH);
	sprintf(outName, "merged_%s.dbf", ISOBATHS_NAME);

	DBFHandle handle = DBFOpen(outName, "rb+");
	if (handle == NULL) {
		fprintf(stderr, "Can't open %s\n", outName);
	}
	else if (!checkType(handle)) {
		DBFClose(handle);
		fprintf(stderr, "Unsupported format of %s\n", outName);
		handle = NULL;
	};
	free(outName);
	
	return handle;
}

void readConfig(const char* strExec) {
	char* ptr;
	int defaultColor = 0x40D;
	
	ptr = (char*)strExec;
	do {
		ptr = strstr(ptr, "shpmerge") + 7;
	} while (strlen(ptr) > 4);
	*ptr = 0;
	char* fname = malloc(_MAX_PATH);
	sprintf(fname, "%s.cfg", strExec);
	
	FILE* fd = fopen(fname, "r");
	if (!fd) {
		for (int i = 0; i < MAX_CONGIG_DEPTH; ++i) {
			if (colorDef[i] == 0) {
				colorDef[i] = defaultColor;
			}
		}
		return;
	}
	
	char buf[256];
	
	memset(colorDef, 0, MAX_CONGIG_DEPTH * sizeof(int));
	
	while (fgets(buf, 256, fd)) {
		char* ps = buf;
		while (isspace(*ps)) {
			++ps;
		}
		if (*ps == ';') {
			continue;
		}
		int dep, color;
		if (sscanf(ps, "%d", &dep) != 1) {
			continue;
		}
		ptr = strstr(ps, ":") + 1;
		if (sscanf(ptr, "%x", &color) != 1) {
			continue;
		};
		if (dep == 100) {
			defaultColor = color;
		}
		else {
			colorDef[dep] = color;
		}
	}
	for (int i = 0; i < MAX_CONGIG_DEPTH; ++i) {
		if (colorDef[i] == 0) {
			colorDef[i] = defaultColor;
		}
	}
}

void proceedDBF(DBFHandle iDBF, DBFHandle oDBF) {
	int recordCount, fieldCount, outCount;
	DBFFieldType fType;
	char	cTitle[32];
	int		nWidth, nDecimals;

	recordCount = DBFGetRecordCount(iDBF);
	fieldCount = DBFGetFieldCount(iDBF);
	
	for (int iRecord = 0; iRecord < recordCount; ++iRecord) {
		outCount = DBFGetRecordCount(oDBF);
		for (int i = 0; i < fieldCount; ++i) {
			fType = DBFGetFieldInfo(iDBF, i, cTitle, &nWidth, &nDecimals);
			switch(fType) {
				case FTString:
					if (outCount == 0) {
						DBFAddField(oDBF, cTitle, FTString, nWidth, nDecimals);
					}
					DBFWriteStringAttribute(oDBF, outCount, i, (char *)DBFReadStringAttribute(iDBF, iRecord, i));
					break;
					
				case FTInteger:
					if (outCount == 0) {
						DBFAddField(oDBF, cTitle, FTInteger, nWidth, nDecimals);
					}
					DBFWriteIntegerAttribute(oDBF, outCount, i, (int)DBFReadIntegerAttribute(iDBF, iRecord, i));
					break;
					
				case FTDouble:
					if (outCount == 0) {
						DBFAddField(oDBF, cTitle, FTDouble, nWidth, nDecimals);
					}
					DBFWriteDoubleAttribute(oDBF, outCount, i, (double)DBFReadDoubleAttribute(iDBF, iRecord, i));
					break;

				default:
					break;
//				case FTLogical:
//					if (outCount == 0) {
//						DBFAddField(oDBF, cTitle, FTLogical, nWidth, nDecimals);
//					}
//					break;
//				case FTInvalid:
//					if (outCount == 0) {
//						DBFAddField(oDBF, cTitle, FTInvalid, nWidth, nDecimals);
//					}
//					break;
			}
		}
	}
	DBFClose(iDBF);
}

BOOL mergeDB(const char* filename, const char* outname) {
	DBFHandle iDBF;
	DBFHandle oDBF;
	BOOL retval = TRUE;
	
	char *mergedName = malloc(_MAX_PATH);
	sprintf(mergedName, "%s.dbf", filename);
	oDBF = DBFCreate(outname);
	
	iDBF = DBFOpen(mergedName, "rb");
	if (iDBF == NULL) {
		retval = FALSE;
	}
	else {
		proceedDBF(iDBF, oDBF);
		// Try to add files, the same name with 2,3, etc at end
		for (int n = 2; ;++n) {
			sprintf(mergedName, "%s%d.dbf", filename, n);
			iDBF = DBFOpen(mergedName, "rb");
			if (iDBF == NULL) {
				break;
			}
			proceedDBF(iDBF, oDBF);
		}
	}
	
	DBFClose(oDBF);
	return TRUE;
}

BOOL mergeShapes(const char* filename, const char* outname) {
	SHPHandle iSHP;
	SHPHandle oSHP;
	SHPObject	*shape;
	int pnEntities, pnShapeType;
	BOOL retval = TRUE;
	
	char *mergedName = malloc(_MAX_PATH);
	sprintf(mergedName, "%s.shp", filename);
	
	iSHP = SHPOpen(mergedName, "rb");
	
	if (iSHP == NULL) {
		retval = FALSE;
	}
	else {
		SHPGetInfo(iSHP, &pnEntities, &pnShapeType, NULL, NULL);
		oSHP = SHPCreate(outname, pnShapeType);
		if (oSHP == NULL) {
			retval = FALSE;
		}
		else {
			for (int i = 0; i < pnEntities; i++) {
				shape = SHPReadObject(iSHP, i);
				SHPWriteObject(oSHP, -1, shape);
				SHPDestroyObject(shape);
			}
			SHPClose(iSHP);
			
			// Try to add files, the same name with 2,3, etc at end
			for (int n = 2; ;++n) {
				sprintf(mergedName, "%s%d.shp", filename, n);
				iSHP = SHPOpen(mergedName, "rb");
				if (iSHP == NULL) {
					break;
				}
				resultCount = n;
				SHPGetInfo(iSHP, &pnEntities, &pnShapeType, NULL, NULL);
				for (int i = 0; i < pnEntities; i++) {
					shape = SHPReadObject(iSHP, i);
					SHPWriteObject(oSHP, -1, shape);
					SHPDestroyObject(shape);
				}
				SHPClose(iSHP);
			}
		}
		SHPClose(oSHP);
	}
	
	free(mergedName);
	return retval;
	
}

#define BUF_SIZE 1024

BOOL copyPRJ(const char* filename, char* outname) {
	FILE *inputFd;
	FILE *outputFd;
	int numRead;
	char buf[BUF_SIZE];
 
	char *mergedName = malloc(_MAX_PATH);
	sprintf(mergedName, "%s.prj", filename);
	sprintf(outname, "%s.prj", outname);

	inputFd = fopen(mergedName, "rb");
	if (inputFd == NULL) {
		return FALSE;
	}

	outputFd = fopen(outname, "wb");
	if (outputFd == NULL) {
		return FALSE;
	}
 
	while ((numRead = (int)fread(buf, 1, BUF_SIZE, inputFd)) > 0) {
		fwrite(buf, numRead, 1, outputFd);
	}
	fclose(inputFd);
	fclose(outputFd);
	
	return TRUE;
}

BOOL mergeFiles(const char* filename, char* outname) {
	if (!mergeShapes(filename, outname)) {
		fprintf(stderr, "Merge shapes error\n");
		return FALSE;
	}
	if (!mergeDB(filename, outname)) {
		fprintf(stderr, "Merge database error\n");
		return FALSE;
	}
	if (!copyPRJ(filename, outname)) {
		fprintf(stderr, "Merge prj error\n");
		return FALSE;
	}
	
	return TRUE;
}

BOOL proceed(const char* filename, const char* suffix) {
	char* mergedName;
	char* outName;
	
	mergedName = malloc(_MAX_PATH);
	sprintf(mergedName, "%s_%s", filename, suffix);
	
	outName = malloc(_MAX_PATH);
	sprintf(outName, "merged_%s", suffix);
	
	BOOL ok = mergeFiles(mergedName, outName);
	
	free(mergedName);
	free(outName);
	return ok;
}

BOOL proceedIsobaths(const char* filename) {
	return proceed(filename, ISOBATHS_NAME);
}

BOOL proceedMajor(const char* filename) {
	return proceed(filename, MAJOR_NAME);
}

BOOL proceedMinor(const char* filename) {
	return proceed(filename, MINOR_NAME);
}

int main(int argc, const char * argv[]) {
	BOOL ok = (argc > 1);
	if (ok) {
		resultCount = 0;
		if (!proceedIsobaths(argv[1])) {
			fprintf(stderr, "Can't proceed isobaths %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		if (!proceedMajor(argv[1])) {
			fprintf(stderr, "Can't proceed Major contours %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		if (!proceedMinor(argv[1])) {
			fprintf(stderr, "Can't proceed Minor contours %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		// TYPE correction
		DBFHandle handle = openDb();
		if (!proceedType(handle)) {
			fprintf(stderr, "Can't correct TYPE\n");
			return EXIT_FAILURE;
		}
	}
	else {
		printf("Usage: shpmerge name\n");
	}

	fprintf(stderr, "%d shapes were merged\n", resultCount);
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
