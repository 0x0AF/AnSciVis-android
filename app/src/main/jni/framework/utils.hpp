#ifndef UTILS_HPP
#define UTILS_HPP

// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
// License    : MIT (see the file LICENSE)
// Maintainer : Andreas-C. Bernstein <andreas.bernstein@uni-weimar.de>
// Stability  : experimental
//
// utils
// -----------------------------------------------------------------------------

//#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <cerrno>
#include <iostream>
#include <SDL_rwops.h>
#include <vector>

// Read a small text file.
inline std::string readFile(std::string const &file) {
    // Pre-RWops
    //    std::ifstream in(file.c_str());
    //    if (in) {
    //        std::string str((std::istreambuf_iterator<char>(in)),
    //                        std::istreambuf_iterator<char>());
    //        return str;
    //    }
    //    throw (errno);

    // TODO
    // Post-RWops
    SDL_RWops *rw;
    rw = SDL_RWFromFile(file.c_str(), "r");
    if (rw != NULL) {
        SDL_RWseek(rw, 0, RW_SEEK_SET);

        std::vector<char> data;

        data.resize(SDL_RWsize(rw));

        SDL_RWread(rw, (char *) &data.front(), 1, SDL_RWsize(rw));
        SDL_RWclose(rw);

        std::string text = std::string(data.data());

        return text;
    }
    throw (errno);
}

GLuint loadShader(GLenum type, std::string const &s);

GLuint createProgram(std::string const &v, std::string const &f);

GLuint updateTexture2D(unsigned const texture_id, unsigned const &width, unsigned const &height,
                       const char *data);

GLuint createTexture2D(unsigned const &width, unsigned const &height,
                       const char *data);

GLuint createTexture3D(unsigned const &width, unsigned const &height,
                       unsigned const &depth, unsigned const channel_size,
                       unsigned const channel_count, const char *data);

#endif // #ifndef UTILS_HPP
