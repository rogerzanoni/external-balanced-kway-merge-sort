#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "errors.h"
#include "util.h"

#define print_error_and_usage(error) do {\
	pr_error(error);\
	print_usage(argv);\
} while (0);

void print_usage(char *argv[])
{
	printf("Usage:\n");
	printf("%s -i <binary file> -o <text file>\n", argv[0]);
}

int main(int argc, char *argv[])
{
	extern char *optarg;
	char outfile_path[PATH_SIZE] = { 0 };
	char infile_path[PATH_SIZE] = { 0 };
	int option;
	int nmemb_read;
	uint32_t num;

	while ((option = getopt(argc, argv,"o:i:")) != -1) {
		switch (option) {
			case 'o':
				strcpy(outfile_path, optarg);
				break;
			case 'i':
				strcpy(infile_path, optarg);
				break;
		}
	}

	if (!infile_path[0]) {
		print_error_and_usage(ENOINPUTFILE);
		return -ENOINPUTFILE;
	}

	if (!outfile_path[0]) {
		print_error_and_usage(ENOOUTPUTFILE);
		return -ENOOUTPUTFILE;
	}

	if (strcmp(infile_path, outfile_path) == 0) {
		print_error_and_usage(EINFILEEQOUTFILE);
		return -EINFILEEQOUTFILE;
	}

	FILE *outfile = fopen(outfile_path, "w");
	if (!outfile) {
		perror(get_error(EOPENOUTFILE));
		return -EOPENOUTFILE;
	}

	FILE *infile = fopen(infile_path, "rb");
	if (!infile) {
		perror(get_error(EOPENINFILE));
		return -EOPENINFILE;
	}

	do {
		nmemb_read = fread(&num, sizeof(uint32_t), 1, infile);
		if (nmemb_read != 1)
			break;
		fprintf(outfile, "%" PRIu32 "\n", num);
	} while (true);

	fclose(outfile);
	fclose(infile);

	return 0;
}
