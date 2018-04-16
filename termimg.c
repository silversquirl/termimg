// Reads ANSI-escaped data on stdin, writes a PNG to stdout
// This file is licensed under the ISC license

#define _POSIX_C_SOURCE 200809l
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cairo/cairo.h>
#include <vterm.h>

enum {
	ROW = 24,
	COL = 72,
};

static cairo_status_t cairo_write_to_file(void *fp, const unsigned char *data, unsigned int len) {
	FILE *f = fp;
	if (fwrite(data, 1, len, f) != len) {
		if (ferror(f))
			fprintf(stderr, "IO error when writing Cairo data\n");
		else if (feof(f))
			fprintf(stderr, "End of file when writing Cairo data\n");
		return CAIRO_STATUS_WRITE_ERROR;
	}
	return CAIRO_STATUS_SUCCESS;
}

struct font_info {
	const char *family;
	int size;
};

int termimg_render(VTerm *vt, struct font_info fi) {
	// Font init
	// FIXME: should really use scaled fonts here. However, I couldn't get that to work so just did this hack instead
	cairo_surface_t *tmpsurf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, fi.size, fi.size);
	cairo_t *tmp = cairo_create(tmpsurf);
	cairo_surface_destroy(tmpsurf);
	cairo_select_font_face(tmp, fi.family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(tmp, fi.size);
	cairo_font_extents_t font_extents;
	cairo_font_extents(tmp, &font_extents);
	int char_w = font_extents.max_x_advance, char_h = font_extents.height;
	cairo_destroy(tmp);

	// Drawing init
	int col, row;
	vterm_get_size(vt, &row, &col);
	int w = char_w * col, h = char_h * row;
	cairo_surface_t *imgsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

	cairo_t *cr = cairo_create(imgsurf);
	cairo_select_font_face(cr, fi.family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, fi.size);

	// Actual drawing
	VTermScreen *s = vterm_obtain_screen(vt);
	for (int x = 0; x < col; ++x) {
		for (int y = 0; y < row; ++y) {
			VTermScreenCell cell;
			vterm_screen_get_cell(s, (VTermPos){ y, x }, &cell);
			if (cell.width != 1)
				fprintf(stderr, "termimg doesn't know how to handle multi-codepoint characters yet. Raise an issue on GitHub if this annoys you.\n");

			// Draw background
			cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER); // Behind the text
			cairo_set_source_rgb(cr, cell.bg.red / 255, cell.bg.green / 255, cell.bg.blue / 255);
			cairo_rectangle(cr, x * char_w, y * char_h, (x + 1) * char_w, (y + 1) * char_h);
			cairo_fill(cr);

			// Calculate character to draw
			if (!cell.chars[0]) continue; // Skip NULs

			char text[MB_CUR_MAX + 1];
			int n = wctomb(text, cell.chars[0]);
			if (n < 0) {
				fprintf(stderr, "Bad character at position (%d, %d)\n", x, y);
				continue;
			}
			text[n] = '\0'; // Add NUL terminator

			// Draw foreground
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER); // Above the background
			cairo_set_source_rgb(cr, cell.fg.red / 255., cell.fg.green / 255., cell.fg.blue / 255.);
			cairo_move_to(cr, x * char_w, (y + 1) * char_h);
			cairo_show_text(cr, text);
		}
	}

	// Cleanup
	cairo_surface_write_to_png_stream(imgsurf, cairo_write_to_file, stdout);
	cairo_destroy(cr);
	cairo_surface_destroy(imgsurf);
	return 0;
}

void usage(void) {
	puts("Usage: termimg [-f family] [-s size] >output.png");
	puts("Options:");
	puts("  -f family  Set the font family (default: monospace)");
	puts("  -s size    Set the font size (default: 12)");
}

int main(int argc, char *argv[]) {
	struct font_info fi = {
		.family = "monospace",
		.size = 12,
	};
	int ch;
	while ((ch = getopt(argc, argv, "hf:s:")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;

		case 'f':
			fi.family = optarg;
			break;

		case 's':
			fi.size = atoi(optarg);
			break;

		case '?':
			return 1;
		}
	}

	VTerm *vt = vterm_new(ROW, COL);
	vterm_set_utf8(vt, 1);
	VTermScreen *s = vterm_obtain_screen(vt);
	vterm_screen_reset(s, 1);

	// Make line feed reset the column
	vterm_input_write(vt, "\e[20h", 5);

#define BUF_SIZE 256
	char buf[BUF_SIZE];
	size_t n;
	do {
		n = fread(buf, 1, BUF_SIZE, stdin);
		vterm_input_write(vt, buf, n);
	} while (n == BUF_SIZE);
	if (ferror(stdin)) fprintf(stderr, "Error reading data\n");
	termimg_render(vt, fi);

	vterm_free(vt);
	return 0;
}
