#include "texture.h"
#include <iostream>
#include <string.h>

using namespace ogle;

Texture::Texture()
    : Name(0)
    , ComponentCount(4)
    , Width(0)
    , Height(0)
    , Target(GL_TEXTURE_2D)
    , InternalFormat(GL_RGBA16F)
    , Format(GL_RGBA)
    , Type(GL_FLOAT)
    , MinFilter(GL_NEAREST)
    , MagFilter(GL_NEAREST)
    , WrapS(GL_CLAMP_TO_EDGE)
    , WrapT(GL_CLAMP_TO_EDGE)
{

}

Texture::~Texture()
{

}

void Texture::create()
{
    glGenTextures(1, &Name);
    glBindTexture(Target, Name);

    // Nvidia has a bug that if these were intergal textures
    // GL_LINEAR cannot be used and must be GL_NEAREST
    glTexParameteri( Target, GL_TEXTURE_MIN_FILTER, MinFilter );
    glTexParameteri( Target, GL_TEXTURE_MAG_FILTER, MagFilter );
    glTexParameteri( Target, GL_TEXTURE_WRAP_S, WrapS );
    glTexParameteri( Target, GL_TEXTURE_WRAP_T, WrapT );
    glTexImage2D(
        Target, 0,
        InternalFormat,
        Width,
        Height,
        0,
        Format,
        Type,
        0
    );

    glClearTexImage(Name, 0, Format, Type, 0);

    if (glGetError() != GL_NONE) assert(0);

    glBindTexture(Target, 0);
}

void Texture::create(unsigned int width, unsigned int height)
{
    Width = width;
    Height = height;
    create();
}

void Texture::bindAsTexture(int stage)
{
    glActiveTexture(GL_TEXTURE0+stage);
    glBindTexture(Target, Name);
}

void Texture::unbindTexture(int stage)
{
    glActiveTexture(GL_TEXTURE0+stage);
    glBindTexture(Target, 0);
}

void Texture::bindAsImage(int stage, unsigned int access)
{
    glBindImageTexture(stage, Name, 0, GL_FALSE, 0, access, InternalFormat);
}

void Texture::clear()
{
    glClearTexImage(Name, 0, Format, Type, 0);
}

void Texture::shutdown()
{
    glDeleteTextures(1, &Name);
    Name = 0;
}
