#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

static void *emalloc(size_t size) {
	void *ret = malloc(size);
	if (ret == NULL) {
		panic("Failed to malloc (%zu)\n", size);
	}

	return ret;
}

static void *erealloc(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (ret == NULL) {
		panic("Failed to realloc (%zu)\n", size);
	}

	return ret;
}

static void *ecalloc(size_t elems, size_t size) {
	void *ret = calloc(elems, size);
	if (ret == NULL) {
		panic("Failed to calloc (%zu, %zu)\n", elems, size);
	}

	return ret;
}

static char *read_file(char *filename, size_t *file_size) {
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


#define get_rna_seq(rna) (((uint64_t)((rna)[6])) << 48 | ((uint64_t)((rna)[5])) << 40 | ((uint64_t)((rna)[4])) << 32 | ((uint64_t)((rna)[3])) << 24 | ((uint64_t)((rna)[2])) << 16 | ((uint64_t)((rna)[1])) << 8 | ((uint64_t)((rna)[0])))

#define max(x, y) (((x) > (y)) ? (x) : (y))

typedef enum {
	ADD_COLOR_BLACK       = get_rna_seq("PIPIIIC"),
	ADD_COLOR_RED         = get_rna_seq("PIPIIIP"),
	ADD_COLOR_GREEN       = get_rna_seq("PIPIICC"),
	ADD_COLOR_YELLOW      = get_rna_seq("PIPIICF"),
	ADD_COLOR_BLUE        = get_rna_seq("PIPIICP"),
	ADD_COLOR_MAGENTA     = get_rna_seq("PIPIIFC"),
	ADD_COLOR_CYAN        = get_rna_seq("PIPIIFF"),
	ADD_COLOR_WHITE       = get_rna_seq("PIPIIPC"),
	ADD_ALPHA_TRANSPARENT = get_rna_seq("PIPIIPF"),
	ADD_ALPHA_OPAQUE      = get_rna_seq("PIPIIPP"),
	CLEAR_BUCKET          = get_rna_seq("PIIPICP"),
	MOVE                  = get_rna_seq("PIIIIIP"),
	TURN_CCLOCKWISE       = get_rna_seq("PCCCCCP"),
	TURN_CLOCKWISE        = get_rna_seq("PFFFFFP"),
	MARK        		  = get_rna_seq("PCCIFFP"),
	LINE        		  = get_rna_seq("PFFICCP"),
	FILL        		  = get_rna_seq("PIIPIIP"),
	ADD_BITMAP       	  = get_rna_seq("PCCPFFP"),
	COMPOSE       	      = get_rna_seq("PFFPCCP"),
	CLIP       	      	  = get_rna_seq("PFFICCF"),
} inst_t;

static char *get_inst_name(inst_t inst) {
	switch (inst) {
		case ADD_COLOR_BLACK:       return "Add Color: Black";
		case ADD_COLOR_RED:         return "Add Color: Red";
		case ADD_COLOR_GREEN:       return "Add Color: Green";
		case ADD_COLOR_YELLOW:      return "Add Color: Yellow";
		case ADD_COLOR_BLUE:        return "Add Color: Blue";
		case ADD_COLOR_MAGENTA:     return "Add Color: Magenta";
		case ADD_COLOR_CYAN:        return "Add Color: Cyan";
		case ADD_COLOR_WHITE:       return "Add Color: White";
		case ADD_ALPHA_TRANSPARENT: return "Add Alpha: Transparent";
		case ADD_ALPHA_OPAQUE:      return "Add Alpha: Opaque";
		case CLEAR_BUCKET:          return "Clear Bucket";
		case MOVE:                  return "Move";
		case TURN_CCLOCKWISE:       return "Turn Counter-Clockwise";
		case TURN_CLOCKWISE:        return "Turn Clockwise";
		case MARK:                  return "Set Mark";
		case LINE:                  return "Draw Line";
		case FILL:                  return "Fill";
		case ADD_BITMAP:            return "Add Bitmap";
		case COMPOSE:               return "Compose";
		case CLIP:                  return "Clip";
		default:                    return NULL;
	}
}

typedef enum {
	DIR_N,
	DIR_S,
	DIR_W,
	DIR_E,
} dir_t;

typedef enum {
	COLOR_RGB,
	COLOR_ALPHA
} color_kind_t;

typedef struct {
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
		uint32_t c;
	};
} color_t;

typedef struct {
	color_t c;
	color_kind_t type;
} color_wrap_t;

typedef struct {
	int x;
	int y;
} pos_t;

#define BITMAP_WIDTH 600
#define BITMAP_HEIGHT 600
typedef struct {
	int pos_x;
	int pos_y;
	int mark_x;
	int mark_y;
	dir_t dir;

	int max_bitmaps;
	int bitmap_size;
	color_t **bitmaps;

	int max_colors;
	int bucket_len;
	color_wrap_t *bucket;

	int max_fill_stack;
	int fill_stack_len;
	pos_t *fill_stack;
} fuun_state_t;

const color_wrap_t color_black   = {{  0,   0,   0,   0},      COLOR_RGB};
const color_wrap_t color_white   = {{255, 255, 255,   0},      COLOR_RGB};
const color_wrap_t color_red     = {{255,   0,   0,   0},      COLOR_RGB};
const color_wrap_t color_green   = {{  0, 255,   0,   0},      COLOR_RGB};
const color_wrap_t color_blue    = {{  0,   0, 255,   0},      COLOR_RGB};
const color_wrap_t color_yellow  = {{255, 255,   0,   0},      COLOR_RGB};
const color_wrap_t color_magenta = {{255,   0, 255,   0},      COLOR_RGB};
const color_wrap_t color_cyan    = {{  0, 255, 255,   0},      COLOR_RGB};
const color_wrap_t alpha_opaque       = {{  0,   0,   0, 255}, COLOR_ALPHA};
const color_wrap_t alpha_transparent  = {{  0,   0,   0, 0},   COLOR_ALPHA};

static color_t get_cur_col(fuun_state_t *state) {
	uint32_t rsum = 0;
	uint32_t gsum = 0;
	uint32_t bsum = 0;
	uint32_t asum = 0;

	int rgb_count   = 0;
	int alpha_count = 0;

	for (int i = 0; i < state->bucket_len; i++) {
		color_wrap_t col = state->bucket[i];
		switch (col.type) {
			case COLOR_RGB: {
				rsum += col.c.r;
				gsum += col.c.g;
				bsum += col.c.b;
				rgb_count++;
			} break;
			case COLOR_ALPHA: {
				asum += col.c.a;
				alpha_count++;
			} break;
			default: {
				panic("Invalid color type! %d (%d, %d, %d, %d)\n", col.type, col.c.r, col.c.g, col.c.b, col.c.a);
			}
		}
	}

	int cur_r, cur_g, cur_b, cur_a;
	if (!rgb_count) {
		cur_r = 0;
		cur_g = 0;
		cur_b = 0;
	} else {
		cur_r = rsum / rgb_count;
		cur_g = gsum / rgb_count;
		cur_b = bsum / rgb_count;
	}

	if (!alpha_count) {
		cur_a = 255;
	} else {
		cur_a = asum / alpha_count;
	}

	color_t cur_col;
	cur_col.r = (cur_r * cur_a) / 255;
	cur_col.g = (cur_g * cur_a) / 255;
	cur_col.b = (cur_b * cur_a) / 255;
	cur_col.a = cur_a;

	return cur_col;
}

static void add_color(fuun_state_t *state, color_wrap_t color) {
	if ((state->bucket_len + 1) >= state->max_colors) {
		int old_max = state->max_colors;
		state->max_colors *= 2;
		dprintf(2, "**** Resizing from %d -> %d ****\n", old_max, state->max_colors);

		state->bucket = erealloc(state->bucket, sizeof(color_wrap_t) * state->max_colors);
		memset(state->bucket + old_max, 0, sizeof(color_wrap_t) * (state->max_colors - old_max));
	}

	state->bucket[state->bucket_len++] = color;
}

static char color_buffer[20];
static char *print_color(color_t col) {
	sprintf(color_buffer, "%d, %d, %d, %d", col.r, col.g, col.b, col.a);
	return color_buffer;
}

static color_t *process_rna(char *rna_buffer, size_t rna_size) {
	char *rna = rna_buffer;

	fuun_state_t state = {0};
	state.dir = DIR_E;

	state.max_bitmaps = 10;
	state.bitmap_size = 1;
	state.bitmaps = (color_t **)emalloc(sizeof(color_t *) * state.max_bitmaps);
	for (int i = 0; i < state.max_bitmaps; i++) {
		state.bitmaps[i] = (color_t *)ecalloc(sizeof(color_t), BITMAP_WIDTH * BITMAP_HEIGHT);
	}

	state.max_colors = 200;
	state.bucket_len = 0;
	state.bucket = (color_wrap_t *)ecalloc(sizeof(color_wrap_t), state.max_colors);

	state.max_fill_stack = BITMAP_WIDTH * BITMAP_HEIGHT * 100;
	state.fill_stack_len = 0;
	state.fill_stack = (pos_t *)emalloc(sizeof(pos_t) * state.max_fill_stack);

	bool no_hit = false;

	int inst_count = 0;
	for (int i = 0; i < rna_size;) {
		rna = rna_buffer + i;
		uint64_t rna_val = get_rna_seq(rna);

/*
		//if (inst_count == 16404) {
		//if (inst_count == 16665) {
		//if (inst_count == 16666) {
		if (inst_count == 17564) {
			break;
		}
*/

		printf("(%d) Running: %s %.*s\n", inst_count, get_inst_name(rna_val), 7, rna);

		switch (rna_val) {
			case ADD_BITMAP: {
				printf("Bitmap Count: %d\n", state.bitmap_size);
				if (state.bitmap_size < state.max_bitmaps) {
					color_t *tmp_bmp_ptr = state.bitmaps[state.bitmap_size];
					memmove(state.bitmaps + 1, state.bitmaps, sizeof(color_t *) * state.bitmap_size);

					state.bitmaps[0] = tmp_bmp_ptr;
					color_t *new_bitmap = state.bitmaps[0];
					memset(new_bitmap, 0, sizeof(color_t) * BITMAP_WIDTH * BITMAP_HEIGHT);

					state.bitmap_size++;
				}
			} break;
			case COMPOSE: {
				if (state.bitmap_size < 2) {
					break;
				}

				for (int j = 0; j < BITMAP_WIDTH * BITMAP_HEIGHT; j++) {
					color_t bmp1_color = state.bitmaps[0][j];
					color_t bmp2_color = state.bitmaps[1][j];

					color_t new_color;
					new_color.r = bmp1_color.r + ((bmp2_color.r * (255 - bmp1_color.a)) / 255);
					new_color.g = bmp1_color.g + ((bmp2_color.g * (255 - bmp1_color.a)) / 255);
					new_color.b = bmp1_color.b + ((bmp2_color.b * (255 - bmp1_color.a)) / 255);
					new_color.a = bmp1_color.a + ((bmp2_color.a * (255 - bmp1_color.a)) / 255);
					state.bitmaps[1][j] = new_color;
				}

				// Do some pointer shuffling so I don't have do free/realloc memory
				color_t *tmp_bitmap_ptr = state.bitmaps[0];

				state.bitmap_size--;
				memmove(state.bitmaps, state.bitmaps + 1, sizeof(color_t *) * state.bitmap_size);
				state.bitmaps[state.bitmap_size] = tmp_bitmap_ptr;
			} break;
			case CLIP: {
				if (state.bitmap_size < 2) {
					break;
				}

				for (int j = 0; j < BITMAP_WIDTH * BITMAP_HEIGHT; j++) {
					color_t bmp1_color = state.bitmaps[0][j];
					color_t bmp2_color = state.bitmaps[1][j];

					color_t new_color;
					new_color.r = (bmp2_color.r * bmp1_color.a) / 255;
					new_color.g = (bmp2_color.g * bmp1_color.a) / 255;
					new_color.b = (bmp2_color.b * bmp1_color.a) / 255;
					new_color.a = (bmp2_color.a * bmp1_color.a) / 255;
					state.bitmaps[1][j] = new_color;
				}

				// Do some pointer shuffling so I don't have do free/realloc memory
				color_t *tmp_bitmap_ptr = state.bitmaps[0];

				state.bitmap_size--;
				memmove(state.bitmaps, state.bitmaps + 1, sizeof(color_t *) * state.bitmap_size);
				state.bitmaps[state.bitmap_size] = tmp_bitmap_ptr;
			} break;
			case ADD_ALPHA_TRANSPARENT: {
				add_color(&state, alpha_transparent);
			} break;
			case ADD_ALPHA_OPAQUE: {
				add_color(&state, alpha_opaque);
			} break;
			case ADD_COLOR_MAGENTA: {
				add_color(&state, color_magenta);
			} break;
			case ADD_COLOR_CYAN: {
				add_color(&state, color_cyan);
			} break;
			case ADD_COLOR_RED: {
				add_color(&state, color_red);
			} break;
			case ADD_COLOR_YELLOW: {
				add_color(&state, color_yellow);
			} break;
			case ADD_COLOR_BLACK: {
				add_color(&state, color_black);
			} break;
			case ADD_COLOR_GREEN: {
				add_color(&state, color_green);
			} break;
			case ADD_COLOR_WHITE: {
				add_color(&state, color_white);
			} break;
			case ADD_COLOR_BLUE: {
				add_color(&state, color_blue);
			} break;
			case FILL: {
				color_t new_color = get_cur_col(&state);
				printf("Filling with color: (%d, %d, %d, %d)\n", new_color.r, new_color.g, new_color.b, new_color.a);

				int px_idx = (state.pos_y * BITMAP_HEIGHT) + state.pos_x;
				color_t old_color = state.bitmaps[0][px_idx];

				if (new_color.c == old_color.c) {
					break;
				}

				state.fill_stack_len = 1;
				state.fill_stack[0].x = state.pos_x;
				state.fill_stack[0].y = state.pos_y;

				while (state.fill_stack_len > 0) {
					pos_t cur_pos = state.fill_stack[--state.fill_stack_len];

					color_t *cur_bitmap = state.bitmaps[0];
					int px_idx = (cur_pos.y * BITMAP_HEIGHT) + cur_pos.x;
					color_t px_color = cur_bitmap[px_idx];

					if (px_color.c == new_color.c) {
						continue;
					}

					printf("checking: (%d, %d)\n", cur_pos.x, cur_pos.y);

					cur_bitmap[px_idx] = new_color;

					if (cur_pos.x > 0) {
						state.fill_stack[state.fill_stack_len].x = cur_pos.x - 1;
						state.fill_stack[state.fill_stack_len].y = cur_pos.y;

						state.fill_stack_len++;
					}
					if (cur_pos.x < (BITMAP_WIDTH - 1)) {
						state.fill_stack[state.fill_stack_len].x = cur_pos.x + 1;
						state.fill_stack[state.fill_stack_len].y = cur_pos.y;
						state.fill_stack_len++;
					}
					if (cur_pos.y > 0) {
						state.fill_stack[state.fill_stack_len].x = cur_pos.x;
						state.fill_stack[state.fill_stack_len].y = cur_pos.y - 1;
						state.fill_stack_len++;
					}
					if (cur_pos.y < (BITMAP_HEIGHT - 1)) {
						state.fill_stack[state.fill_stack_len].x = cur_pos.x;
						state.fill_stack[state.fill_stack_len].y = cur_pos.y + 1;
						state.fill_stack_len++;
					}

					if (state.fill_stack_len > state.max_fill_stack) {
						panic("%d > %d, Yikes!\n", state.fill_stack_len, state.max_fill_stack);
					}
				}
			} break;
			case CLEAR_BUCKET: {
				memset(state.bucket, 0, sizeof(color_wrap_t) * state.bucket_len);
				state.bucket_len = 0;
			} break;
			case LINE: {

				int dx = state.mark_x - state.pos_x;
				int dy = state.mark_y - state.pos_y;
				int d = max(abs(dx), abs(dy));

				int c = ((dx * dy) <= 0) ? 1 : 0;

				int offset = (d - c) / 2;
				int x = state.pos_x * d + offset;
				int y = state.pos_y * d + offset;

				color_t cur_col = get_cur_col(&state);
				printf("Drawing line with (%s) (%d, %d) -> (%d, %d)\n", print_color(cur_col), state.pos_x, state.pos_y, state.mark_x, state.mark_y);

				for (int j = 0; j < d; j++) {
					int px_x = x / d;
					int px_y = y / d;

					color_t *cur_bitmap = state.bitmaps[0];
					int px_idx = (px_y * BITMAP_HEIGHT) + px_x;
					cur_bitmap[px_idx] = cur_col;

					x += dx;
					y += dy;
				}

				int px_x = state.mark_x;
				int px_y = state.mark_y;

				color_t *cur_bitmap = state.bitmaps[0];
				int px_idx = (px_y * BITMAP_HEIGHT) + px_x;
				cur_bitmap[px_idx] = cur_col;

				// Do something interesting here
			} break;
			case MARK: {
				state.mark_x = state.pos_x;
				state.mark_y = state.pos_y;
			} break;
			case TURN_CCLOCKWISE: {
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
				switch (state.dir) {
					case DIR_N: {
						state.pos_y = (state.pos_y + BITMAP_HEIGHT - 1) % BITMAP_HEIGHT;
					} break;
					case DIR_S: {
						state.pos_y = (state.pos_y + 1) % BITMAP_HEIGHT;
					} break;
					case DIR_W: {
						state.pos_x = (state.pos_x + BITMAP_WIDTH - 1) % BITMAP_WIDTH;
					} break;
					case DIR_E: {
						state.pos_x = (state.pos_x + 1) % BITMAP_WIDTH;
					} break;
					default: {
						panic("Unhandled direction: %d\n!", state.dir);
					}
				}
			} break;
			default: { no_hit = true;};
		}

		if (!no_hit) {
			inst_count++;
		}
		no_hit = false;

		i += 7;
	}

	printf("inst count: %d\n", inst_count);
	return state.bitmaps[0];
}

char endo_dna_filename[] = "test.rna";
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


	char *new_img = (char *)process_rna(dna_buffer, dna_file_size);

	printf("Dumping to %s\n", dump_filename);

	int ret = stbi_write_png(dump_filename, width, height, 4, new_img, width * 4);
	if (!ret) {
		panic("failed to write dump file!\n");
	}

	return 0;
}
