// wp81findbin.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "getopt.h"
#include "findBin.h"

BOOL verbose;

void removeLastFilename(CHAR *pPathWithFilename)
{
	for (DWORD i = strlen(pPathWithFilename); i > 0; i--) 
	{
		if (pPathWithFilename[i] == '\\')
		{
			pPathWithFilename[i] = '\0';
			break;
		}
	}
}

static void usage(char *programName)
{
	printf("%s - Searches for a sequence of bytes in files.\n"
		"Usage:\n", programName);
	printf("\t%s [options] \"value\" filename\n", programName);
	printf("options:\n"
		"\t-h, --help               Show help options\n"
		"\t-v, --verbose            Increase verbosity\n");
}

static const struct option main_options[] = {
	{ "help",      no_argument,       NULL, 'h' },
	{ "verbose",   no_argument,       NULL, 'v' },
	{}
};

int main(int argc, char* argv[])
{
	int exit_status = EXIT_SUCCESS;
	CHAR* pFilename = NULL;
	CHAR* pStrValue = NULL;
	BYTE* pBytes = NULL;
	size_t length = 0;
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	CHAR* pPath = (CHAR*)malloc(MAX_PATH);
	DWORD pathLength;

	for (;;) {
		int opt;

		opt = getopt_long(argc, argv,
			"hv",
			main_options, NULL);

		if (opt < 0) {
			// no more option to parse
			break;
		}

		switch (opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'v':
			printf("Verbose mode\n");
			verbose = TRUE;
			break;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (argc > 2 && argv[argc - 1][0] != '-' && argv[argc - 2][0] != '-')
	{
		pStrValue = argv[argc - 2];
		pFilename = argv[argc - 1];
	}
	else
	{
		printf("No value or filename.\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (verbose) printf("filename=%s\n", pFilename);
	if (verbose) printf("value=%s\n", pStrValue);

	exit_status = parseValueSize(pStrValue, &length);
	pBytes = (BYTE*)malloc(length);
	exit_status = parseValueContent(pStrValue, pBytes, length);

	memcpy(pPath, pFilename, strlen(pFilename) + 1);
	removeLastFilename(pPath);
	pathLength = strlen(pPath);
	pPath[pathLength] = '\\';
	pPath[pathLength+1] = '\0';
	pathLength++;
	if (verbose) printf("path=%s\n", pPath);

	hFind = FindFirstFileA(pFilename, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		printf("Invalid filename: %s\n", pFilename);
		exit_status = EXIT_FAILURE;
	}
	do
	{
		// 0x101C5B9F, 0x41C4C6F7, 0xAA695B81, 0xA9A0ECC1
		// 9F 5B 1C 10 F7 C6 C4 41 81 5B 69 AA C1 EC A0 A9

		// 0xBB431756, 0x46C34878, 0x95ECFCA8, 0x4DF52DFF
		// 56 17 43 BB 78 48 C3 46 A8 FC EC 95 FF 2D F5 4D

		// 1B4A6595-CFCD-4E08-92FB-C1906D04498C
		// 95 65 4A 1B CD CF 08 4E 92 FB C1 90 6D 04 49 8C

		if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Add the filename to the path
			memcpy(pPath + pathLength, FindFileData.cFileName, strlen(FindFileData.cFileName) + 1);
			if (verbose)
			{
				printf("Checking %s\n", pPath);
			}
			else
			{
				printf(".");
			}
			exit_status = findBin(pPath, pBytes, length);
		}		
	} while (FindNextFileA(hFind, &FindFileData) != 0);

	FindClose(hFind);
	free(pBytes);
	free(pPath);

	return exit_status;
}

