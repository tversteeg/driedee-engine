#include "l_png.h"

#include <png.h>

typedef struct {
	const png_byte *data;
	const png_size_t size;
} datahandle;

typedef struct {
	const datahandle data;
	png_size_t offset;
} readdatahandle;

typedef struct {
	const png_uint_32 width;
	const png_uint_32 height;
	const int color_type;
} pnginfo;

static void readPngData(png_structp ptr, png_byte *data, png_size_t len);
static pnginfo readInfo(const png_structp ptr, const png_infop info);
static datahandle readPng(const png_structp ptr, const png_info info, const png_uint_32 height);

bool loadPng(texture_t *tex, const char *file)
{
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png == NULL){
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if(info == NULL){
		return false;
	}

	return true;
}
