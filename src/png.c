#include <GL/gl.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include "lwi_private.h"

static unsigned
png_bytes_per_pixel(png_infop info_ptr)
{
    switch (info_ptr->color_type) {
        case PNG_COLOR_TYPE_GRAY:
            return LWI_IMAGE_FORMAT_GRAY;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            return LWI_IMAGE_FORMAT_GRAY_ALPHA;
        case PNG_COLOR_TYPE_RGB:
            return LWI_IMAGE_FORMAT_RGB;
        case PNG_COLOR_TYPE_RGBA:
            return LWI_IMAGE_FORMAT_RGBA;
        default:
            return LWI_IMAGE_FORMAT_NONE;
    }
}

EXPORT int
lwi_image_read_png(lwi_image_t *img, const char *filename)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_byte header[8];

    FILE *fp = fopen(filename, "rb");
    if (!fp)
        goto err0;

    fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp(header, 0, sizeof(header)))
        goto err1;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto err1;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        goto err2;

    if (setjmp(png_jmpbuf(png_ptr)))
        goto err2;

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sizeof(header));
    png_read_info(png_ptr, info_ptr);
    png_read_update_info(png_ptr, info_ptr);

    const unsigned width = info_ptr->width;
    const unsigned height = info_ptr->height;
    const unsigned bytes_per_pixel = png_bytes_per_pixel(info_ptr);
    if (!bytes_per_pixel)
        goto err2;

    const unsigned max_size = 0xFFFFFFFF / bytes_per_pixel;
    const unsigned size = width * height;
    if (size < width || size < height || size > max_size)
        goto err2;

    png_bytep *row_ptrs = malloc(sizeof(png_bytep) * height);
    if (!row_ptrs)
        goto err2;

    png_bytep image_data = malloc(width * height * bytes_per_pixel);
    if (!image_data)
        goto err3;

    for (unsigned y = 0; y < height; y++)
        row_ptrs[y] = image_data + (y * width * bytes_per_pixel);

    if (setjmp(png_jmpbuf(png_ptr)))
        goto err4;

    png_read_image(png_ptr, row_ptrs);
    png_read_end(png_ptr, NULL);

    img->pixel_data = image_data;
    img->format = bytes_per_pixel;
    img->width = width;
    img->height = height;
    img->stride = width;

    free(row_ptrs);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return 0;

err4:
    free(image_data);
err3:
    free(row_ptrs);
err2:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
err1:
    fclose(fp);
err0:
    return 1;
}

EXPORT GLuint
lwi_gl_texture_read_png(const char *filename)
{
    lwi_image_t img;

    if (lwi_image_read_png(&img, filename) != 0)
        return 0;

    GLuint id = lwi_gl_texture_from_image(&img);
    free(img.pixel_data);
    return id;
}
