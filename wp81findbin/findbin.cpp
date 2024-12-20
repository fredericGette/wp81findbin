#include "stdafx.h"
#include "findBin.h"

#define READ_BUFFER_SIZE 200

int parseValue(CHAR *pStrValue, size_t *pValueSize, BYTE *pBytes, size_t length)
{
	BYTE value = 0;
	DWORD nbOfDigit = 0;
	DWORD inputBufferLength = 0;
	for (DWORD i = 0; i < strlen(pStrValue); i++) 
	{
		char c = pStrValue[i];
		if (c >= '0' && c <= '9') 
		{
			value += c - '0';
			nbOfDigit++;
		}
		else if (c >= 'A' && c <= 'F') 
		{
			value += c - 'A' + 10;
			nbOfDigit++;
		}
		else if (c >= 'a' && c <= 'f') 
		{
			value += c - 'a' + 10;
			nbOfDigit++;
		}
		else if (nbOfDigit != 0) 
		{
			printf("Invalid input. Must be a list of bytes in hexadecimal.\n");
			return EXIT_FAILURE;
		}

		if (nbOfDigit == 1) 
		{
			value = value << 4;
		}
		else if (nbOfDigit == 2) 
		{
			if (pBytes != NULL)
			{
				pBytes[inputBufferLength] = value;
			}
			inputBufferLength++;
			value = 0;
			nbOfDigit = 0;
		}

		if (length > 0 && inputBufferLength > length) 
		{
			printf("Invalid input. Max number of bytes is %d.\n", length);
			return EXIT_FAILURE;
		}
	}
	if (nbOfDigit != 0) 
	{
		printf("Invalid input. Must be a list of bytes in hexadecimal.\n");
		return EXIT_FAILURE;
	}

	if (pValueSize != NULL)
	{
		*pValueSize = inputBufferLength;
	}

	return EXIT_SUCCESS;
}

int parseValueSize(CHAR *pStrValue, size_t *pValueSize)
{
	return parseValue(pStrValue, pValueSize, NULL, 0);
}

int parseValueContent(CHAR *pStrValue, BYTE *pBytes, size_t length)
{
	return parseValue(pStrValue, NULL, pBytes, length);
}


int search(BYTE* pReadBuffer, DWORD dwBytesRead, BYTE* pBytes, DWORD length, BYTE* pTestBuffer, DWORD* pTestBufferLength)
{
	BYTE* pHit = (BYTE*)memchr(pReadBuffer, pBytes[0], READ_BUFFER_SIZE);
	if (pHit == NULL)
	{
		return -1;
	}

	DWORD dwBytesAvailable = pReadBuffer + dwBytesRead - pHit;
	if (dwBytesAvailable >= length)
	{
		if (memcmp(pHit, pBytes, length) == 0)
		{
			// We found the bytes we were looking for.
			return 1;
		}
	}
	else
	{
		memcpy_s(pTestBuffer, length, pHit, dwBytesAvailable);
		*pTestBufferLength = dwBytesAvailable;
	}

	return 0;
}

int findBin(CHAR* pFilename, BYTE* pBytes, size_t length)
{
	if (length < 1)
	{
		printf("Sequence of bytes must have a length > 0.\n");
		return EXIT_FAILURE;
	}

	int exit_status = EXIT_SUCCESS;
	HANDLE hFile;
	BYTE* pReadBuffer = (BYTE*)malloc(READ_BUFFER_SIZE);
	DWORD dwBytesRead;
	BYTE* pTestBuffer = (BYTE*)malloc(length);
	DWORD testBufferLength = 0;
	DWORD searchResult;

	hFile = CreateFileA(pFilename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Unable to open file [%s]\n", pFilename);
		return EXIT_FAILURE;
	}

	do
	{
		if (ReadFile(hFile, pReadBuffer, READ_BUFFER_SIZE, &dwBytesRead, NULL) == FALSE)
		{
			printf("Unable to read file [%s]\n", pFilename);
			exit_status = EXIT_FAILURE;
			break;
		}

		if (testBufferLength == 0)
		{
			// No previous hit pending
			// Search in the new pReadBuffer
			searchResult = search(pReadBuffer, dwBytesRead, pBytes, length, pTestBuffer, &testBufferLength);
			if (searchResult == 1)
			{
				printf("\nFile %s matches.\n", pFilename);
				break;
			}
		}
		else if (dwBytesRead > length - testBufferLength)
		{
			// A previous hit is pending
			// Complete the pending pTestBuffer
			// And check its content against the bytes we are looking for.
			memcpy_s(pTestBuffer + testBufferLength, length - testBufferLength, pReadBuffer, length - testBufferLength);
			if (memcmp(pTestBuffer, pBytes, length) == 0)
			{
				printf("\nFile %s matches.\n", pFilename);
				break;
			}
			testBufferLength = 0;

			// We didn't find the correct bytes in the pTestBuffer
			// Search in the new pReadBuffer
			searchResult = search(pReadBuffer, dwBytesRead, pBytes, length, pTestBuffer, &testBufferLength);
			if (searchResult == 1)
			{
				printf("\nFile %s matches.\n", pFilename);
				break;
			}
		}
	}	
	while (dwBytesRead > 0);

	CloseHandle(hFile);

	return exit_status;
}