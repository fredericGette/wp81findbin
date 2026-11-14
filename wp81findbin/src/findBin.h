#pragma once

int parseValueSize(CHAR *pStrValue, size_t *pValueSize);
int parseValueContent(CHAR *pStrValue, BYTE *bytes, size_t length);
int findBin(CHAR* filename, UCHAR* bytes, size_t length);