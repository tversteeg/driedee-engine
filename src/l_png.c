#include "l_png.h"

#include <png.h>

bool getSizePng(const char *file, unsigned int *width, unsigned int *height)
{
	FILE *fp = fopen(file, "rb");
	if(!fp){
		printf("Couldn't open file: %s\n", file);
		return false;
	}

	unsigned char header[8];
	fread(header, 1, sizeof(header), fp);
	if(png_sig_cmp(header, 0, sizeof(header))){
		printf("File supplied is not a valid PNG: %s\n", file);
		return false;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png){
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if(!info){
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		return false;
	}

	if(setjmp(png_jmpbuf(png))){
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		return false;
	}

	png_init_io(png, fp);
	png_set_sig_bytes(png, sizeof(header));

	png_read_info(png, info);

	*width = png_get_image_width(png, info);
	*height = png_get_image_height(png, info);

	return true;
}

bool loadPng(texture_t *tex, const char *file)
{
	FILE *fp = fopen(file, "rb");
	if(!fp){
		printf("Couldn't open file: %s\n", file);
		return false;
	}

	unsigned char header[8];
	fread(header, 1, sizeof(header), fp);
	if(png_sig_cmp(header, 0, sizeof(header))){
		printf("File supplied is not a valid PNG: %s\n", file);
		return false;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png){
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if(!info){
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		return false;
	}

	if(setjmp(png_jmpbuf(png))){
		printf("Error during PNG initialization\n");
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		return false;
	}

	png_init_io(png, fp);
	png_set_sig_bytes(png, sizeof(header));

	png_read_info(png, info);

	unsigned int width = png_get_image_width(png, info);
	unsigned int height = png_get_image_height(png, info);

	png_byte bitdepth = png_get_bit_depth(png, info);
	if(bitdepth < 8){
		png_set_packing(png);
	}else if(bitdepth == 16){
		png_set_strip_16(png);
	}

	png_byte colortype = png_get_color_type(png, info);

	png_read_update_info(png, info);

	if(setjmp(png_jmpbuf(png))){
		printf("Error during PNG reading\n");
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		return false;
	}

	png_bytep *rows = (png_bytep*)malloc(sizeof(png_bytep) * height);
	unsigned int y;
	for(y = 0; y < height; y++){
		rows[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, rows);

	fclose(fp);

	if(colortype == PNG_COLOR_TYPE_RGB){
		for(y = 0; y < height; y++){
			png_byte *row = rows[y];
			unsigned int x;
			for(x = 0; x < width; x++){
				png_byte *color = row + x * 3;
				setPixel(tex, x, y, (pixel_t){color[0], color[1], color[2], 255});
			}
		}
	}else if(colortype == PNG_COLOR_TYPE_RGBA){
		for(y = 0; y < height; y++){
			png_byte *row = rows[y];
			unsigned int x;
			for(x = 0; x < width; x++){
				png_byte *color = row + (x << 2);
				setPixel(tex, x, y, (pixel_t){color[0], color[1], color[2], color[3]});
			}
		}
	}

	for(y = 0; y < height; y++){
		free(rows[y]);
	}
	free(rows);

	png_destroy_read_struct(&png, &info, NULL);

	if(colortype != PNG_COLOR_TYPE_RGB && colortype != PNG_COLOR_TYPE_RGBA){
		printf("Unrecognized PNG colortype: %d\n", colortype);
		return false;
	}

	return true;
}
