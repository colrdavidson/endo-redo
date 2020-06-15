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

#define get_rna_seq(rna) (((uint64_t)((rna)[6])) << 48 | ((uint64_t)((rna)[5])) << 40 | ((uint64_t)((rna)[4])) << 32 | ((uint64_t)((rna)[3])) << 24 | ((uint64_t)((rna)[2])) << 16 | ((uint64_t)((rna)[1])) << 8 | ((uint64_t)((rna)[0])))

enum {
	ADD_COLOR_BLACK       = get_rna_seq("PIPIIIC"),
	ADD_COLOR_RED         = get_rna_seq("PIPIIIP"),
	ADD_COLOR_GREEN       = get_rna_seq("PIPIICC"),
	ADD_COLOR_YELLOW      = get_rna_seq("PIPIICF"),
	ADD_COLOR_BLUE        = get_rna_seq("PIPIICP"),
	ADD_COLOR_MAGENTA     = get_rna_seq("PIPIIFC"),
	ADD_COLOR_CYAN        = get_rna_seq("PIPIIFF"),
	ADD_COLOR_WHITE       = get_rna_seq("PIPIIPC"),
	ADD_COLOR_TRANSPARENT = get_rna_seq("PIPIIPF"),
	ADD_COLOR_OPAQUE      = get_rna_seq("PIPIIPP"),
	BUCKET                = get_rna_seq("PIIPICP"),
	MOVE                  = get_rna_seq("PIIIIIP"),
	TURN_CCLOCKWISE       = get_rna_seq("PCCCCCP"),
	TURN_CLOCKWISE        = get_rna_seq("PFFFFFP"),
	MARK        		  = get_rna_seq("PCCIFFP"),
	LINE        		  = get_rna_seq("PFFICCP"),
	FILL        		  = get_rna_seq("PIIPIIP"),
	ADD_BITMAP       	  = get_rna_seq("PCCPFFP"),
	COMPOSE       	      = get_rna_seq("PFFPCCP"),
	CLIP       	      	  = get_rna_seq("PFFICCF"),
};

typedef enum {
	DIR_N,
	DIR_S,
	DIR_W,
	DIR_E,
} dir_t;

typedef struct {
	int pos_x;
	int pos_y;
	int mark_x;
	int mark_y;
	dir_t dir;
} fuun_state_t;

char *process_rna(char *rna_buffer, size_t rna_size, uint8_t *img, size_t img_size) {
	char *rna = rna_buffer;

	fuun_state_t state = {0};
	state.dir = DIR_E;

	for (int i = 0; i < rna_size;) {
		rna = rna_buffer + i;
		uint64_t rna_val = get_rna_seq(rna);

		switch (rna_val) {
			case ADD_COLOR_BLACK:
			case ADD_COLOR_RED:
			case ADD_COLOR_GREEN:
			case ADD_COLOR_YELLOW:
			case ADD_COLOR_BLUE:
			case ADD_COLOR_MAGENTA:
			case ADD_COLOR_CYAN:
			case ADD_COLOR_WHITE:
			case ADD_COLOR_TRANSPARENT:
			case ADD_COLOR_OPAQUE:
			case BUCKET:
			case FILL:
			case ADD_BITMAP:
			case COMPOSE:
			case CLIP: {
				printf("%.*s\n", 7, rna);
				exit(0);
			} break;
			case LINE: {
				printf("Running Line: %.*s\n", 7, rna);
				// Do something interesting here
			} break;
			case MARK: {
				printf("Running Mark: %.*s\n", 7, rna);
				state.mark_x = state.pos_x;
				state.mark_y = state.pos_y;
			} break;
			case TURN_CCLOCKWISE: {
				printf("Running Turn Counter Clockwise: %.*s\n", 7, rna);
				switch (state.dir) {
					case DIR_N: {
						state.dir = DIR_W;
					} break;
					case DIR_S: {
						state.dir = DIR_E;
					} break;
					case DIR_W: {
						state.dir = DIR_S;
					} break;
					case DIR_E: {
						state.dir = DIR_N;
					} break;
					default: {
						panic("Unhandled direction: %d\n!", state.dir);
					}
				}
			} break;
			case TURN_CLOCKWISE: {
				printf("Running Turn Clockwise: %.*s\n", 7, rna);
				switch (state.dir) {
					case DIR_N: {
						state.dir = DIR_E;
					} break;
					case DIR_S: {
						state.dir = DIR_W;
					} break;
					case DIR_W: {
						state.dir = DIR_N;
					} break;
					case DIR_E: {
						state.dir = DIR_S;
					} break;
					default: {
						panic("Unhandled direction: %d\n!", state.dir);
					}
				}
			} break;
			case MOVE: {
				printf("Running Move: %.*s\n", 7, rna);
				switch (state.dir) {
					case DIR_N: {
						state.pos_y += 1;
					} break;
					case DIR_S: {
						state.pos_y -= 1;
					} break;
					case DIR_W: {
						state.pos_x -= 1;
					} break;
					case DIR_E: {
						state.pos_x += 1;
					} break;
					default: {
						panic("Unhandled direction: %d\n!", state.dir);
					}
				}

			} break;
			default: { };
		}

		i += 7;
	}

	exit(0);
}

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


	char *new_img = process_rna(dna_buffer, dna_file_size, img, width * height * channels);

	int ret = stbi_write_png(dump_filename, width, height, channels, new_img, 0);
	if (!ret) {
		panic("failed to write dump file!\n");
	}

	return 0;
}
