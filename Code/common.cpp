#include <stdio.h>
#include <assert.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int loadTexture(GLuint tex, const char *filename)
{
    int w, h, c;
    uint8_t *data = stbi_load(filename, &w, &h, &c, STBI_rgb);
    if (!data)
    {
        printf("file %s missing\n", filename);
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, w,h);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, w,h, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return 1;
}
