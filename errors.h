#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdio.h>

#define ENOSIZE 1
#define EINVSIZE 2
#define EINVSZSUFFIX 3
#define ESZNOTNUM 4
#define ENOOUTPUTFILE 5
#define ENOINPUTFILE 6
#define EOPENOUTFILE 7
#define EOPENINFILE 8
#define EINFILEEQOUTFILE 9
#define EINVNWAYS 10
#define ENONWAYS 11
#define ECREATESCRATCHFILE 12
#define ECREATESCRATCHFILES 13

static char *errors[] = {
	"dummystr",

	"Size not set.",

	"Invalid size.",

	"Invalid suffix: Use K-Kilobytes, M-Megabytes or G-Gigabytes.",

	"Size must be a number.",

	"Output file not set.",

	"Source file not set.",

	"Error opening output file.",

	"Error opening input file.",

	"Input and output files can't be the same.",

	"Invalid K",

	"K not set.",

	"Error creating temporary file."

	"Error copying temporary file."
};

static void pr_error(short error)
{
	printf("%s\n", errors[error]);
}

static char *get_error(short error)
{
	return errors[error];
}

#endif
