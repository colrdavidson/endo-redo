#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define panic(...) do { dprintf(2, __VA_ARGS__); exit(1); } while (0);

void *emalloc(size_t size) {
	void *ret = malloc(size);
	if (ret == NULL) {
		panic("Failed to malloc (%zu)\n", size);
	}

	return ret;
}

char endo_source_filename[] = "endo.dna";

int main() {
	int ret;

	int fd = open(endo_source_filename, O_RDONLY);
	if (fd == -1) {
		panic("Failed to open file: %s\n", endo_source_filename);
	}

	off_t tmp_size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if (tmp_size == -1) {
		panic("Failed to get the size of file!\n");
	}

	size_t file_size = (size_t)tmp_size;
	char *buffer = (char *)emalloc(file_size + 1);

	int size = read(fd, buffer, file_size);
	if (size != file_size) {
		panic("Failed to read file!\n");
	}
	buffer[file_size] = '\0';

	printf("%s\n", buffer);
	return 0;
}
