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
	}
	else {
		printf("Usage: shpmerge name\n");
	}

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
