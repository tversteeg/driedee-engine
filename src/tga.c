/*
 * tga.c -- tga texture loader
 * last modification: dec. 15, 2005
 *
 * Copyright (c) 2005 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * gcc -Wall -ansi -L/usr/X11R6/lib -lGL -lGLU -lglut tga.c -o tga
 */

#include "tga.h"

#include <stdio.h>
#include <stdlib.h>


/* OpenGL texture info */
typedef struct
{
  GLsizei width;
  GLsizei height;

  GLenum format;
  GLint	internalFormat;
  GLuint id;

  GLubyte *texels;

} gl_texture_t;


#pragma pack(push, 1)
/* tga header */
typedef struct
{
  GLubyte id_lenght;          /* size of image id */
  GLubyte colormap_type;      /* 1 is has a colormap */
  GLubyte image_type;         /* compression type */

  short	cm_first_entry;       /* colormap origin */
  short	cm_length;            /* colormap length */
  GLubyte cm_size;            /* colormap size */

  short	x_origin;             /* bottom left x coord origin */
  short	y_origin;             /* bottom left y coord origin */

  short	width;                /* picture width (in pixels) */
  short	height;               /* picture height (in pixels) */

  GLubyte pixel_depth;        /* bits per pixel: 8, 16, 24 or 32 */
  GLubyte image_descriptor;   /* 24 bits = 0x00; 32 bits = 0x80 */

} tga_header_t;
#pragma pack(pop)


/* texture id for exemple */
GLuint texId = 0;


void
GetTextureInfo (tga_header_t *header, gl_texture_t *texinfo)
{
  texinfo->width = header->width;
  texinfo->height = header->height;

  switch (header->image_type)
    {
    case 3:  /* grayscale 8 bits */
    case 11: /* grayscale 8 bits (RLE) */
      {
	if (header->pixel_depth == 8)
	  {
	    texinfo->format = GL_LUMINANCE;
	    texinfo->internalFormat = 1;
	  }
	else /* 16 bits */
	  {
	    texinfo->format = GL_LUMINANCE_ALPHA;
	    texinfo->internalFormat = 2;
	  }

	break;
      }

    case 1:  /* 8 bits color index */
    case 2:  /* BGR 16-24-32 bits */
    case 9:  /* 8 bits color index (RLE) */
    case 10: /* BGR 16-24-32 bits (RLE) */
      {
	/* 8 bits and 16 bits images will be converted to 24 bits */
	if (header->pixel_depth <= 24)
	  {
	    texinfo->format = GL_RGB;
	    texinfo->internalFormat = 3;
	  }
	else /* 32 bits */
	  {
	    texinfo->format = GL_RGBA;
	    texinfo->internalFormat = 4;
	  }

	break;
      }
    }
}


void
ReadTGA8bits (FILE *fp, GLubyte *colormap, gl_texture_t *texinfo)
{
  int i;
  GLubyte color;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read index color byte */
      color = (GLubyte)fgetc (fp);

      /* convert to RGB 24 bits */
      texinfo->texels[(i * 3) + 2] = colormap[(color * 3) + 0];
      texinfo->texels[(i * 3) + 1] = colormap[(color * 3) + 1];
      texinfo->texels[(i * 3) + 0] = colormap[(color * 3) + 2];
    }
}


void
ReadTGA16bits (FILE *fp, gl_texture_t *texinfo)
{
  int i;
  unsigned short color;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read color word */
      color = fgetc (fp) + (fgetc (fp) << 8);

      /* convert BGR to RGB */
      texinfo->texels[(i * 3) + 0] = (GLubyte)(((color & 0x7C00) >> 10) << 3);
      texinfo->texels[(i * 3) + 1] = (GLubyte)(((color & 0x03E0) >>  5) << 3);
      texinfo->texels[(i * 3) + 2] = (GLubyte)(((color & 0x001F) >>  0) << 3);
    }
}


void
ReadTGA24bits (FILE *fp, gl_texture_t *texinfo)
{
  int i;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read and convert BGR to RGB */
      texinfo->texels[(i * 3) + 2] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 3) + 1] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 3) + 0] = (GLubyte)fgetc (fp);
    }
}


void
ReadTGA32bits (FILE *fp, gl_texture_t *texinfo)
{
  int i;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read and convert BGRA to RGBA */
      texinfo->texels[(i * 4) + 2] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 4) + 1] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 4) + 0] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 4) + 3] = (GLubyte)fgetc (fp);
    }
}


void
ReadTGAgray8bits (FILE *fp, gl_texture_t *texinfo)
{
  int i;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read grayscale color byte */
      texinfo->texels[i] = (GLubyte)fgetc (fp);
    }
}


void
ReadTGAgray16bits (FILE *fp, gl_texture_t *texinfo)
{
  int i;

  for (i = 0; i < texinfo->width * texinfo->height; ++i)
    {
      /* read grayscale color + alpha channel bytes */
      texinfo->texels[(i * 2) + 0] = (GLubyte)fgetc (fp);
      texinfo->texels[(i * 2) + 1] = (GLubyte)fgetc (fp);
    }
}


void
ReadTGA8bitsRLE (FILE *fp, GLubyte *colormap, gl_texture_t *texinfo)
{
  int i, size;
  GLubyte color;
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height) * 3)
    {
      /* read first byte */
      packet_header = (GLubyte)fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  color = (GLubyte)fgetc (fp);

	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = colormap[(color * 3) + 2];
	      ptr[1] = colormap[(color * 3) + 1];
	      ptr[2] = colormap[(color * 3) + 0];
	    }
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      color = (GLubyte)fgetc (fp);

	      ptr[0] = colormap[(color * 3) + 2];
	      ptr[1] = colormap[(color * 3) + 1];
	      ptr[2] = colormap[(color * 3) + 0];
	    }
	}
    }
}


void
ReadTGA16bitsRLE (FILE *fp, gl_texture_t *texinfo)
{
  int i, size;
  unsigned short color;
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height) * 3)
    {
      /* read first byte */
      packet_header = fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  color = fgetc (fp) + (fgetc (fp) << 8);

	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = (GLubyte)(((color & 0x7C00) >> 10) << 3);
	      ptr[1] = (GLubyte)(((color & 0x03E0) >>  5) << 3);
	      ptr[2] = (GLubyte)(((color & 0x001F) >>  0) << 3);
	    }
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      color = fgetc (fp) + (fgetc (fp) << 8);

	      ptr[0] = (GLubyte)(((color & 0x7C00) >> 10) << 3);
	      ptr[1] = (GLubyte)(((color & 0x03E0) >>  5) << 3);
	      ptr[2] = (GLubyte)(((color & 0x001F) >>  0) << 3);
	    }
	}
    }
}


void
ReadTGA24bitsRLE (FILE *fp, gl_texture_t *texinfo)
{
  int i, size;
  GLubyte rgb[3];
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height) * 3)
    {
      /* read first byte */
      packet_header = (GLubyte)fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  fread (rgb, sizeof (GLubyte), 3, fp);

	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = rgb[2];
	      ptr[1] = rgb[1];
	      ptr[2] = rgb[0];
	    }
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[2] = (GLubyte)fgetc (fp);
	      ptr[1] = (GLubyte)fgetc (fp);
	      ptr[0] = (GLubyte)fgetc (fp);
	    }
	}
    }
}


void
ReadTGA32bitsRLE (FILE *fp, gl_texture_t *texinfo)
{
  int i, size;
  GLubyte rgba[4];
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height) * 4)
    {
      /* read first byte */
      packet_header = (GLubyte)fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  fread (rgba, sizeof (GLubyte), 4, fp);

	  for (i = 0; i < size; ++i, ptr += 4)
	    {
	      ptr[0] = rgba[2];
	      ptr[1] = rgba[1];
	      ptr[2] = rgba[0];
	      ptr[3] = rgba[3];
	    }
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr += 4)
	    {
	      ptr[2] = (GLubyte)fgetc (fp);
	      ptr[1] = (GLubyte)fgetc (fp);
	      ptr[0] = (GLubyte)fgetc (fp);
	      ptr[3] = (GLubyte)fgetc (fp);
	    }
	}
    }
}


void
ReadTGAgray8bitsRLE (FILE *fp, gl_texture_t *texinfo)
{
  int i, size;
  GLubyte color;
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height))
    {
      /* read first byte */
      packet_header = (GLubyte)fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  color = (GLubyte)fgetc (fp);

	  for (i = 0; i < size; ++i, ptr++)
	    *ptr = color;
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr++)
	    *ptr = (GLubyte)fgetc (fp);
	}
    }
}


void
ReadTGAgray16bitsRLE (FILE *fp, gl_texture_t *texinfo)
{
  int i, size;
  GLubyte color, alpha;
  GLubyte packet_header;
  GLubyte *ptr = texinfo->texels;

  while (ptr < texinfo->texels + (texinfo->width * texinfo->height) * 2)
    {
      /* read first byte */
      packet_header = (GLubyte)fgetc (fp);
      size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  /* run-length packet */
	  color = (GLubyte)fgetc (fp);
	  alpha = (GLubyte)fgetc (fp);

	  for (i = 0; i < size; ++i, ptr += 2)
	    {
	      ptr[0] = color;
	      ptr[1] = alpha;
	    }
	}
      else
	{
	  /* non run-length packet */
	  for (i = 0; i < size; ++i, ptr += 2)
	    {
	      ptr[0] = (GLubyte)fgetc (fp);
	      ptr[1] = (GLubyte)fgetc (fp);
	    }
	}
    }
}


gl_texture_t *
ReadTGAFile (const char *filename)
{
  FILE *fp;
  gl_texture_t *texinfo;
  tga_header_t header;
  GLubyte *colormap = NULL;

  fp = fopen (filename, "rb");
  if (!fp)
    {
      fprintf (stderr, "error: couldn't open \"%s\"!\n", filename);
      return NULL;
    }

  /* read header */
  fread (&header, sizeof (tga_header_t), 1, fp);

  texinfo = (gl_texture_t *)malloc (sizeof (gl_texture_t));
  GetTextureInfo (&header, texinfo);
  fseek (fp, header.id_lenght, SEEK_CUR);

  /* memory allocation */
  texinfo->texels = (GLubyte *)malloc (sizeof (GLubyte) *
	texinfo->width * texinfo->height * texinfo->internalFormat);
  if (!texinfo->texels)
    {
      free (texinfo);
      return NULL;
    }

  /* read color map */
  if (header.colormap_type)
    {
      /* NOTE: color map is stored in BGR format */
      colormap = (GLubyte *)malloc (sizeof (GLubyte)
	* header.cm_length * (header.cm_size >> 3));
      fread (colormap, sizeof (GLubyte), header.cm_length
	* (header.cm_size >> 3), fp);
    }

  /* read image data */
  switch (header.image_type)
    {
    case 0:
      /* no data */
      break;

    case 1:
      /* uncompressed 8 bits color index */
      ReadTGA8bits (fp, colormap, texinfo);
      break;

    case 2:
      /* uncompressed 16-24-32 bits */
      switch (header.pixel_depth)
	{
	case 16:
	  ReadTGA16bits (fp, texinfo);
	  break;

	case 24:
	  ReadTGA24bits (fp, texinfo);
	  break;

	case 32:
	  ReadTGA32bits (fp, texinfo);
	  break;
	}

      break;

    case 3:
      /* uncompressed 8 or 16 bits grayscale */
      if (header.pixel_depth == 8)
	ReadTGAgray8bits (fp, texinfo);
      else /* 16 */
	ReadTGAgray16bits (fp, texinfo);

      break;

    case 9:
      /* RLE compressed 8 bits color index */
      ReadTGA8bitsRLE (fp, colormap, texinfo);
      break;

    case 10:
      /* RLE compressed 16-24-32 bits */
      switch (header.pixel_depth)
	{
	case 16:
	  ReadTGA16bitsRLE (fp, texinfo);
	  break;

	case 24:
	  ReadTGA24bitsRLE (fp, texinfo);
	  break;

	case 32:
	  ReadTGA32bitsRLE (fp, texinfo);
	  break;
	}

      break;

    case 11:
      /* RLE compressed 8 or 16 bits grayscale */
      if (header.pixel_depth == 8)
	ReadTGAgray8bitsRLE (fp, texinfo);
      else /* 16 */
	ReadTGAgray16bitsRLE (fp, texinfo);

      break;

    default:
      /* image type is not correct */
      fprintf (stderr, "error: unknown TGA image type %i!\n", header.image_type);
      free (texinfo->texels);
      free (texinfo);
      texinfo = NULL;
      break;
    }

  /* no longer need colormap data */
  if (colormap)
    free (colormap);

  fclose (fp);
  return texinfo;
}


GLuint
loadTGATexture (const char *filename)
{
  gl_texture_t *tga_tex = NULL;
  GLuint tex_id = 0;

  tga_tex = ReadTGAFile (filename);
  
  if (tga_tex && tga_tex->texels)
    {
      /* generate texture */
      glGenTextures (1, &tga_tex->id);
      glBindTexture (GL_TEXTURE_2D, tga_tex->id);

      /* setup some parameters for texture filters and mipmapping */
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*_MIPMAP_LINEAR*/);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


      glTexImage2D (GL_TEXTURE_2D, 0, tga_tex->internalFormat,
		    tga_tex->width, tga_tex->height, 0, tga_tex->format,
		    GL_UNSIGNED_BYTE, tga_tex->texels);

      /*
      gluBuild2DMipmaps (GL_TEXTURE_2D, tga_tex->internalFormat,
			 tga_tex->width, tga_tex->height,
			 tga_tex->format, GL_UNSIGNED_BYTE, tga_tex->texels);
      */

      tex_id = tga_tex->id;

      /* OpenGL has its own copy of texture data */
      free (tga_tex->texels);
      free (tga_tex);
    }

  return tex_id;
}
