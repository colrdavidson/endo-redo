#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define panic(...) do { dprintf(2, __VA_ARGS__); exit(1); } while (0);

void *emalloc(size_t size) {
	void *ret = malloc(size);
	if (ret == NULL) {
		panic("Failed to malloc (%zu)\n", size);
	}

	return ret;
}

char *read_file(char *filename, size_t *file_size) {
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		panic("Failed to open file: %s\n", filename);
	}

	off_t tmp_size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if (tmp_size == -1) {
		panic("Failed to get the size of file!\n");
	}

	size_t size = (size_t)tmp_size;
	char *buffer = (char *)emalloc(size + 1);

	int read_size = read(fd, buffer, size);
	if (read_size != size) {
		panic("Failed to read file!\n");
	}

	*file_size = size;
	return buffer;
}

char endo_dna_filename[] = "endo.dna";
char endo_img_filename[] = "source.png";
char dump_filename[] = "dump.png";

int main() {
	size_t dna_file_size;
	char *dna_buffer = read_file(endo_dna_filename, &dna_file_size);
	dna_buffer[dna_file_size] = '\0';

	size_t img_file_size;
	uint8_t *img_buffer = (uint8_t *)read_file(endo_img_filename, &img_file_size);

	int width;
	int height;
	int channels;
	uint8_t *img = stbi_load_from_memory(img_buffer, img_file_size, &width, &height, &channels, 0);

	printf("%d, %d, %d\n", img[0], img[1], img[2]);

	int ret = stbi_write_png(dump_filename, width, height, channels, img, 0);
	if (!ret) {
		panic("failed to write dump file!\n");
	}

	//printf("%s\n", dna_buffer);
	return 0;
}
