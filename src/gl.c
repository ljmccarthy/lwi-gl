#include <GL/gl.h>
#include "lwi_private.h"

static const GLenum gl_formats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

EXPORT GLuint
lwi_gl_texture_from_image(lwi_image_t *img)
{
    if (!img->pixel_data || img->format <= 0 || img->format >= NELEMS(gl_formats))
        return 0;

    GLuint id[1];
    glGenTextures(1, id);
    glBindTexture(GL_TEXTURE_2D, id[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, img->format, img->width, img->height, 0,
                 gl_formats[img->format], GL_UNSIGNED_BYTE, img->pixel_data);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, id);
        return 0;
    }
    return id[0];
}
