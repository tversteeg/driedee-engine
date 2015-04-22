#include "l_draw.h"

#include "l_colors.h"

#include <string.h>

inline void drawPixel(texture_t *tex, int x, int y, pixel_t pixel);

static inline void vline(texture_t *tex, int x, int top, int bot, pixel_t pixel)
{
	if(x < 0 || x >= tex->width){
		return;
	}

	if(top < bot){
		int tmp = top;
		top = bot;
		bot = tmp;
	}

	if(top >= tex->height){
		top = tex->height - 1;
	}
	if(bot < 0){
		bot = 0;
	}

	int y;
	for(y = bot; y <= top; y++){
		drawPixel(tex, x, y, pixel);
	}
}

static inline void hline(texture_t *tex, int y, int left, int right, pixel_t pixel)
{
	if(y < 0 || y >= tex->height){
		return;
	}

	if(right < left){
		int tmp = right;
		right = left;
		left = tmp;
	}

	if(right >= tex->width){
		right = tex->width - 1;
	}
	if(left < 0){
		left = 0;
	}

	int x;
	for(x = left; x <= right; x++){
		drawPixel(tex, x, y, pixel);
	}
}

void initTexture(texture_t *tex, unsigned int width, unsigned int height)
{
	tex->width = width;
	tex->height = height;
	tex->pixels = (pixel_t*)calloc(width * height, sizeof(pixel_t));
}

void initFont(font_t *font, unsigned int width, unsigned int height)
{
	font->totalwidth = width;
	font->size = height;
	font->pixels = (bool*)calloc(width * height, sizeof(bool));
}

void loadFont(font_t *font, char start, const bool *pixels)
{
	font->letters = font->totalwidth / font->size;
	font->start = start;
	memcpy(font->pixels, pixels, (font->totalwidth * font->size) * sizeof(bool));
}

void clearTexture(texture_t *tex, pixel_t pixel)
{
	if(pixel.r == 0 && pixel.g == 0 && pixel.b == 0 && pixel.a == 0){
		memset(tex->pixels, 0, tex->width * tex->height * sizeof(pixel_t));
	}else{
		unsigned int i, size = tex->width * tex->height;
		for(i = 0; i < size; i++){
			tex->pixels[i] = pixel;
		}
	}
}

inline bool samePixel(pixel_t p1, pixel_t p2)
{
	return p1.r == p2.r && p1.g == p2.g && p1.b == p2.b && p1.a == p2.a;
}

inline bool getPixel(texture_t *tex, pixel_t *pixel, unsigned int x, unsigned int y)
{
	if(x < tex->width && y < tex->height){
		pixel = &tex->pixels[x + y * tex->width];
		return true;
	}
	return false;
}

inline void setPixel(texture_t *tex, unsigned int x, unsigned int y, pixel_t pixel)
{
	tex->pixels[x + y * tex->width] = pixel;
}

inline void drawPixel(texture_t *tex, int x, int y, pixel_t pixel)
{
	if(x >= 0 && x < tex->width && y >= 0 && y < tex->height){
		setPixel(tex, x, y, pixel);
	}
}

void drawLine(texture_t *tex, xy_t p1, xy_t p2, pixel_t pixel)
{
	int x1 = p1.x, y1 = p1.y;
	int x2 = p2.x, y2 = p2.y;
	if(x1 == x2 && y1 == y2){
		drawPixel(tex, x1, x2, pixel);
		return;
	}
	if(x1 == x2){
		vline(tex, x1, y1, y2, pixel);
	}else if(y1 == y2){
		hline(tex, y1, x1, x2, pixel);
	}else{
		int dx = abs(x2 - x1);
		int dy = abs(y2 - y1);
		int sx = x1 < x2 ? 1 : -1;
		int sy = y1 < y2 ? 1 : -1;
		int err = (dx > dy ? dx : -dy) / 2;
		while(true){
			drawPixel(tex, x1, y1, pixel);
			if(x1 == x2 && y1 == y2){
				return;
			}
			int err2 = err;
			if(err2 > -dx) {
				err -= dy;
				x1 += sx;
			}
			if(err2 < dy) {
				err += dx;
				y1 += sy;
			}
		}
	}
}

void drawCircle(texture_t *tex, xy_t p, unsigned int radius, pixel_t pixel)
{
	int x = radius;
	int y = 0;
	int error = 1 - x;
	while(x >= y){
		drawPixel(tex, x + p.x, y + p.y, pixel);
		drawPixel(tex, y + p.x, x + p.y, pixel);
		drawPixel(tex, -x + p.x, y + p.y, pixel);
		drawPixel(tex, -y + p.x, x + p.y, pixel);
		drawPixel(tex, -x + p.x, -y + p.y, pixel);
		drawPixel(tex, -y + p.x, -x + p.y, pixel);
		drawPixel(tex, x + p.x, -y + p.y, pixel);
		drawPixel(tex, y + p.x, -x + p.y, pixel);
		y++;
		if(error < 0){
			error += 2 * y + 1;
		}else{
			x--;
			error += 2 * (y - x) + 1;
		}
	}
}

void drawRect(texture_t *tex, xy_t p, unsigned int width, unsigned int height, pixel_t pixel)
{
	unsigned int i;
	for(i = p.y; i < p.y + height; i++){
		hline(tex, i, p.x, p.x + width, pixel);
	}
}

void drawLetter(texture_t *tex, const font_t *font, char letter, int x, int y, pixel_t pixel)
{
	char todraw = letter - font->start;
	if(todraw < 0 || todraw > font->letters){
		return;
	}
	int drawpos = todraw * font->size;

	int i;
	for(i = 0; i < font->size; i++){
		int j;
		for(j = 0; j < font->size; j++){
			if(font->pixels[i + drawpos + j * font->totalwidth] == true){
				drawPixel(tex, x + i, y + j, pixel);
			}
		}
	}
}

void drawString(texture_t *tex, const font_t *font, const char *string, int x, int y, pixel_t pixel)
{
	int i;
	for(i = 0; string[i] != '\0'; i++){
		if(string[i] == '\n'){
			y += font->size;
			x -= (i + 1) * font->size;
		}else if(string[i] == '\t'){
			x += font->size * 2;
		}else{
			drawLetter(tex, font, string[i], x + i * font->size, y, pixel);
		}
	}
}

void drawTexture(texture_t *target, const texture_t *source, int x, int y)
{
	if(x >= target->width || y >= target->height || x < -source->width || y < -source->height){
		return;
	}

	//TODO handle cases where texture is (partially) out of bounds

	unsigned int i;
	for(i = 0; i < source->height; i++){
		memcpy(target->pixels + x + (y + i) * target->width, source->pixels + i * source->width, source->width * sizeof(pixel_t));
	}
}

void drawTextureSlice(texture_t *target, const texture_t *source, int x, int y, int height, double uvx)
{
	int uvcx;
	if(uvx == 0){
		uvcx = 0;
	}else{
		uvcx = (int)(source->width * uvx) % source->width;
	}
	unsigned int j;
	for(j = 0; j < height; j++){
		int yscreen = y + j;
		if(yscreen >= 0 && yscreen < target->height){
			pixel_t pixel = source->pixels[uvcx + (int)(j / (double)height * source->height) * source->height];
			setPixel(target, x, y + j, pixel);
		}
	}
}

void drawGrid(texture_t *tex, unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int gridwidth, unsigned int gridheight, pixel_t pixel)
{
	if(gridwidth <= 1 || gridheight <= 1){
		return;
	}
	int i;
	for(i = x; i < width; i += gridwidth){
		vline(tex, i, x, height, pixel);
	}
	for(i = y; i < height; i += gridheight){
		hline(tex, i, y, width, pixel);
	}
}

pixel_t strtopixel(const char *hexstr)
{
	if(hexstr == NULL){
		return COLOR_NONE;
	}
	long int number = strtol(hexstr, NULL, 16);

	pixel_t pixel;
	if(strlen(hexstr) == 8){
		pixel.r = (number & 0xFF000000) >> 24;
		pixel.g = (number & 0x00FF0000) >> 16;
		pixel.b = (number & 0x0000FF00) >> 8;
		pixel.a = number & 0x000000FF;
	}else{
		pixel.r = (number & 0xFF0000) >> 16;
		pixel.g = (number & 0x00FF00) >> 8;
		pixel.b = number & 0x0000FF;
		pixel.a = 255;
	}

	return pixel;
}
