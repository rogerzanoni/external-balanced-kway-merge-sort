#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "errors.h"
#include "util.h"

#define print_error_and_usage(error) do \
{\
	pr_error(error);\
	print_usage(argv);\
} while (0);

void print_usage(char *argv[])
{
	printf("Usage:\n");
	printf("%s -i <bin file> -o <ordered bin file> -m <memory size>K|M|G -k <ways>\n", argv[0]);
}

typedef struct block_t {
	size_t size;
	long position;
	struct block_t *next;
} block_t;

block_t *create_block()
{
	block_t *block = (block_t *)malloc(sizeof(block_t));
	block->next = NULL;
	block->position = 0;
	block->size = 0;
	return block;
}

typedef struct {
	FILE *file;
	block_t *first_block;
	block_t *current_block;
} scratchfile_t;

void clear_blocks(scratchfile_t *file)
{
	block_t *block = file->first_block;
	while (block) {
		file->first_block = file->first_block->next;
		free(block);
		block = file->first_block;
	}
	file->first_block = NULL;
	file->current_block = NULL;
}

scratchfile_t *create_scratchfiles(int k)
{
	scratchfile_t *scratchfiles = (scratchfile_t *)malloc(k * sizeof(scratchfile_t));
	int i;

	for (i = 0; i < k; ++i) {
		scratchfiles[i].file = tmpfile();
		scratchfiles[i].first_block = NULL;
		scratchfiles[i].current_block = NULL;
		if (!scratchfiles[i].file) {
			perror(get_error(ECREATESCRATCHFILE));
			printf("Closing temporary files...\n");
			for (k = i - 1; i >= 0; --i) {
				fclose(scratchfiles[i].file);
			}
			return NULL;
		}
	}

	return scratchfiles;
}

void clear_scratchfiles(int k, scratchfile_t *files)
{
	while (k--) {
		clear_blocks(&files[k]);
		fclose(files[k].file);
	}
	free(files);
}

void quick_sort (uint32_t *array, int size) {
	int i, j, pivot;

	if (size < 2)
		return;
	pivot = array[size / 2];
	for (i = 0, j = size - 1;; i++, j--) {
		while (array[i] < pivot)
			i++;
		while (pivot < array[j])
			j--;
		if (i >= j)
			break;
		swap(array, i, j);
	}

	quick_sort(array, i);
	quick_sort(array + i, size - i);
}

void filequeue_next(uint32_t *dest, scratchfile_t *scratchfile)
{
	int nmemb_read;
	if (!scratchfile->current_block ||
	    scratchfile->current_block->position == -1 ||
	    scratchfile->current_block->position > scratchfile->current_block->size)
	    return;
	nmemb_read = fread(dest, sizeof(uint32_t), 1, scratchfile->file);
	if (nmemb_read == 1) {
		scratchfile->current_block->position += sizeof(uint32_t);
	} else
		scratchfile->current_block->position = -1;
}

bool scratchfile_finished(scratchfile_t *file)
{
	if (!file->current_block)
		return true;

	return file->current_block->size == 0 ||
	       file->current_block->position == -1 ||
	       file->current_block->position > file->current_block->size;
}

void write_data(scratchfile_t *scratchfile, uint32_t data)
{
	fwrite(&data, sizeof(uint32_t), 1, scratchfile->file);
	scratchfile->current_block->size += sizeof(uint32_t);
}

void create_runs(int k, size_t buffer_size, uint32_t *buffer, scratchfile_t *scratchfiles, FILE *infile)
{
	int nmemb_read, i = 0;
	size_t registers_per_read;

	registers_per_read = buffer_size / sizeof(uint32_t);

	while (true) {
		nmemb_read = fread(buffer, sizeof(uint32_t), registers_per_read, infile);
		if (!nmemb_read)
			break;
		quick_sort(buffer, nmemb_read);
		fwrite(buffer, sizeof(uint32_t), nmemb_read, scratchfiles[i].file);

		if (scratchfiles[i].first_block == NULL) {
			scratchfiles[i].first_block = create_block();
			scratchfiles[i].current_block = scratchfiles[i].first_block;
		} else {
			scratchfiles[i].current_block->next = create_block();
			scratchfiles[i].current_block = scratchfiles[i].current_block->next;
		}
		scratchfiles[i].current_block->size = nmemb_read * sizeof(uint32_t);

		i = (i + 1) % k;

		if (nmemb_read < registers_per_read)
			break;
	}

	while (k--) {
		scratchfiles[k].current_block = scratchfiles[k].first_block;
	}
}

void multiway_merge(int k, size_t buffer_size, uint32_t *buffer, scratchfile_t *files, FILE *outfile)
{
	bool block_finished;
	int i, j, min, outfile_idx, nmemb_read, registers_per_read;

	scratchfile_t *auxfiles = create_scratchfiles(k);

	scratchfile_t *infiles = files;
	scratchfile_t *outfiles = auxfiles;


	for (i = 0; i < k ; ++i) {
		rewind(infiles[i].file);
		filequeue_next(&buffer[i], &infiles[i]);
		outfiles[i].first_block = create_block();
		outfiles[i].current_block = outfiles[i].first_block;
	}

	outfile_idx = 0;

	block_finished = false;
	while (!block_finished) {
		block_finished = true;

		for (i = 0; i < k; ++i) {
			if (scratchfile_finished(&infiles[i]))
				continue;

			block_finished = false;
			min = i;

			for (j = 0; j < k; ++j) {
				if (i == j)
					continue;
				if (scratchfile_finished(&infiles[j]))
					continue;
				if (buffer[j] < buffer[min])
					min = j;
			}

			write_data(&outfiles[outfile_idx], buffer[min]);
			filequeue_next(&buffer[min], &infiles[min]);
		}

		int old_outfile_idx = outfile_idx;
		if (block_finished) {
			outfiles[outfile_idx].current_block->next = create_block();
			outfiles[outfile_idx].current_block = outfiles[outfile_idx].current_block->next;

			outfile_idx = (outfile_idx + 1) % k;

			for (i = 0; i < k; ++i) {
				scratchfile_block_next(infiles[i]);

				if (!scratchfile_finished(&infiles[i]))
					block_finished = false;
			}
		}

		if (block_finished) {
			int outfile_count = 0;
			for (i = 0; i < k; ++i) {
				if (scratchfile_has_data(outfiles[i]))
					++outfile_count;
			}

			if (outfile_count > 1) {
				block_finished = false;
				infiles = infiles == files ? auxfiles : files;
				outfiles = outfiles == files ? auxfiles : files;

				for (i = 0; i < k; ++i) {
					rewind(infiles[i].file);
					scratchfile_reset(infiles[i]);
					filequeue_next(&buffer[i], &infiles[i]);

					rewind(outfiles[i].file);
					ftruncate(fileno(outfiles[i].file), 0);
					clear_blocks(&outfiles[i]);
					outfiles[i].first_block = create_block();
					outfiles[i].current_block = outfiles[i].first_block;
				}
			}
		}

		if (block_finished) {
			fseek(outfiles[old_outfile_idx].file, 0L, SEEK_SET);
			registers_per_read = buffer_size / sizeof(uint32_t);
			while (true) {
				nmemb_read = fread(buffer, sizeof(uint32_t), registers_per_read, outfiles[old_outfile_idx].file);
				if (!nmemb_read)
					break;
				fwrite(buffer, sizeof(uint32_t), nmemb_read, outfile);
			}

			clear_scratchfiles(k, auxfiles);
		}
	}
}

int main(int argc, char *argv[])
{
	extern char *optarg;
	char outfile_path[PATH_SIZE] = { 0 };
	char infile_path[PATH_SIZE] = { 0 };
	int option;
	long size = 0;
	int k = 0;
	uint32_t *buffer;

	while ((option = getopt(argc, argv,"o:i:m:k:")) != -1) {
		switch (option) {
			case 'o':
				strcpy(outfile_path, optarg);
				break;
			case 'i':
				strcpy(infile_path, optarg);
				break;
			case 'm':
				size = parse_size(optarg);
				break;
			case 'k':
				k = parse_nways(optarg);
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

	if (size == 0) {
		print_error_and_usage(ENOSIZE);
		return -ENOSIZE;
	}

	if (size < 0) {
		print_error_and_usage(-size);
		return size;
	}

	if (k == 0) {
		print_error_and_usage(ENONWAYS);
		return -ENONWAYS;
	}

	if (k < 0) {
		print_error_and_usage(-k);
		return k;
	}

	scratchfile_t *scratchfiles = create_scratchfiles(k);

	if (!scratchfiles) {
		pr_error(ECREATESCRATCHFILES);
		fprintf(stderr, "Check the error messages. Aborting...\n");
		return -ECREATESCRATCHFILES;
	}

	FILE *infile = fopen(infile_path, "rb");
	if (!infile) {
		perror(get_error(EOPENINFILE));
		return -EOPENINFILE;
	}

	FILE *outfile = fopen(outfile_path, "wb");
	if (!outfile) {
		perror(get_error(EOPENOUTFILE));
		return -EOPENOUTFILE;
	}

	size_t buffer_size;
	buffer_size = (size / sizeof(uint32_t)) * sizeof(uint32_t);
	buffer = (uint32_t *)malloc(buffer_size);

	create_runs(k, buffer_size, buffer, scratchfiles, infile);

	multiway_merge(k, buffer_size, buffer, scratchfiles, outfile);

	fclose(outfile);
	fclose(infile);

	free(buffer);

	clear_scratchfiles(k, scratchfiles);

	return 0;
}
