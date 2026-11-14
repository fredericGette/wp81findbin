// wp81findbin.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "getopt.h"
#include "findBin.h"

BOOL verbose;
BOOL recursive;

// Splits "dir\pattern" into a directory and a filename pattern.
// Without a directory part (e.g. "*.dll"), the directory is the current one.
static void splitPath(const CHAR* pPathWithFilename, CHAR* pDir, CHAR* pPattern)
{
	const CHAR* pLastSep = strrchr(pPathWithFilename, '\\');
	if (pLastSep == NULL)
	{
		strcpy_s(pDir, MAX_PATH, ".");
		strcpy_s(pPattern, MAX_PATH, pPathWithFilename);
	}
	else
	{
		size_t dirLength = pLastSep - pPathWithFilename;
		memcpy(pDir, pPathWithFilename, dirLength);
		pDir[dirLength] = '\0';
		strcpy_s(pPattern, MAX_PATH, pLastSep + 1);
	}
}

// Searches the files of pDir matching pPattern, then (if recursive) its subdirectories.
// Returns the number of files matching pPattern that were checked.
static DWORD searchDirectory(const CHAR* pDir, const CHAR* pPattern, BYTE* pBytes, size_t length, int* pExitStatus)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	CHAR path[MAX_PATH];
	DWORD nbOfFiles = 0;

	// Files of this directory matching the pattern
	if (_snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\%s", pDir, pPattern) < 0)
	{
		printf("Path too long: %s\\%s\n", pDir, pPattern);
		*pExitStatus = EXIT_FAILURE;
		return 0;
	}
	hFind = FindFirstFileA(path, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (_snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\%s", pDir, FindFileData.cFileName) < 0)
				{
					printf("Path too long: %s\\%s\n", pDir, FindFileData.cFileName);
					*pExitStatus = EXIT_FAILURE;
					continue;
				}
				nbOfFiles++;
				if (verbose)
				{
					printf("Checking %s\n", path);
				}
				else
				{
					printf(".");
				}
				if (findBin(path, pBytes, length) != EXIT_SUCCESS)
				{
					*pExitStatus = EXIT_FAILURE;
				}
			}
		} while (FindNextFileA(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}

	if (!recursive)
	{
		return nbOfFiles;
	}

	// Subdirectories (all of them, whatever the pattern)
	if (_snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\*", pDir) < 0)
	{
		return nbOfFiles;
	}
	hFind = FindFirstFileA(path, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return nbOfFiles;
	}
	do
	{
		if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) // avoid junction loops
			&& strcmp(FindFileData.cFileName, ".") != 0
			&& strcmp(FindFileData.cFileName, "..") != 0)
		{
			if (_snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\%s", pDir, FindFileData.cFileName) < 0)
			{
				printf("Path too long: %s\\%s\n", pDir, FindFileData.cFileName);
				*pExitStatus = EXIT_FAILURE;
				continue;
			}
			nbOfFiles += searchDirectory(path, pPattern, pBytes, length, pExitStatus);
		}
	} while (FindNextFileA(hFind, &FindFileData) != 0);
	FindClose(hFind);

	return nbOfFiles;
}

static void usage(char *programName)
{
	printf("%s - Searches for a sequence of bytes in files.\n"
		"Usage:\n", programName);
	printf("\t%s [options] \"value\" filename\n", programName);
	printf("options:\n"
		"\t-h, --help               Show help options\n"
		"\t-r, --recursive          Also search in subdirectories\n"
		"\t-v, --verbose            Increase verbosity\n");
}

static const struct option main_options[] = {
	{ "help",      no_argument,       NULL, 'h' },
	{ "recursive", no_argument,       NULL, 'r' },
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
	CHAR dir[MAX_PATH];
	CHAR pattern[MAX_PATH];

	for (;;) {
		int opt;

		opt = getopt_long(argc, argv,
			"hrv",
			main_options, NULL);

		if (opt < 0) {
			// no more option to parse
			break;
		}

		switch (opt) {
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'r':
			recursive = TRUE;
			break;
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

	if (parseValueSize(pStrValue, &length) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	pBytes = (BYTE*)malloc(length);
	if (parseValueContent(pStrValue, pBytes, length) != EXIT_SUCCESS)
	{
		free(pBytes);
		return EXIT_FAILURE;
	}

	if (strlen(pFilename) >= MAX_PATH)
	{
		printf("Filename too long: %s\n", pFilename);
		free(pBytes);
		return EXIT_FAILURE;
	}
	splitPath(pFilename, dir, pattern);
	if (verbose) printf("path=%s\\ pattern=%s%s\n", dir, pattern, recursive ? " (recursive)" : "");

	if (searchDirectory(dir, pattern, pBytes, length, &exit_status) == 0)
	{
		printf("No file found matching: %s\n", pFilename);
		exit_status = EXIT_FAILURE;
	}

	// Example values:
	// 0x101C5B9F, 0x41C4C6F7, 0xAA695B81, 0xA9A0ECC1
	// 9F 5B 1C 10 F7 C6 C4 41 81 5B 69 AA C1 EC A0 A9

	// 0xBB431756, 0x46C34878, 0x95ECFCA8, 0x4DF52DFF
	// 56 17 43 BB 78 48 C3 46 A8 FC EC 95 FF 2D F5 4D

	// 1B4A6595-CFCD-4E08-92FB-C1906D04498C
	// 95 65 4A 1B CD CF 08 4E 92 FB C1 90 6D 04 49 8C

	free(pBytes);

	return exit_status;
}

