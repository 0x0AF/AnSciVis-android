// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
// License    : MIT (see the file LICENSE)
// Maintainer : Andreas-C. Bernstein <andreas.bernstein@uni-weimar.de>
// Stability  : experimental
//
// Window
// -----------------------------------------------------------------------------

#include "window.hpp"
#include <cstring>
#include <iostream>

Window::Window(glm::ivec2 const &windowsize) : m_window(nullptr), m_size(windowsize),
                                               m_mousePosition(),
                                               m_mouseButtonFlags(0), m_keypressed() {
    std::fill(std::begin(m_keypressed), std::end(m_keypressed), false);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);

    m_window = SDL_CreateWindow(nullptr, 0, 0, current.w, current.h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);

    char *err = (char *) SDL_GetError();

    if (m_window) {
        assert(m_window != nullptr);
        SDL_SetWindowData(m_window, WIN_DATA, this);
        m_glcontext = SDL_GL_CreateContext(m_window);

        err = (char *) SDL_GetError();
//        printf("%s\n", SDL_GetError());

        // equivalent (sort of) is discarding in fragment shader
//        glEnable(GL_ALPHA_TEST);
//        glAlphaFunc(GL_NOTEQUAL, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // TODO: implement an equivalent
//        glEnable(GL_POINT_SMOOTH);
//        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
//        glPointSize(5.0f);
//        glEnable(GL_POINT_SMOOTH);

        glLineWidth(2.0f);
        glEnable(GL_LINE_SMOOTH);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glEnable(GL_DEPTH_TEST);
    }
}

Window::~Window() {
    if (m_window) {
        SDL_GL_DeleteContext(m_glcontext);
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

void Window::cursorPositionCallback(SDL_Window *win, double x, double y) {
    Window *w = reinterpret_cast<Window *>(SDL_GetWindowData(win, WIN_DATA));
    assert(w);

    w->m_mousePosition = glm::ivec2(x, y);
}

void Window::mouseButtonCallback(SDL_Window *win, int button, int act, int mods) {
    Window *w = reinterpret_cast<Window *>(SDL_GetWindowData(win, WIN_DATA));
    assert(w);

    if (SDL_PRESSED == act) {
        switch (button) {
            case SDL_BUTTON_LEFT:
                w->m_mouseButtonFlags |= Window::MOUSE_BUTTON_LEFT;
                break;
            case SDL_BUTTON_MIDDLE:
                w->m_mouseButtonFlags |= Window::MOUSE_BUTTON_MIDDLE;
                break;
            case SDL_BUTTON_RIGHT:
                w->m_mouseButtonFlags |= Window::MOUSE_BUTTON_RIGHT;
                break;
            default:
                break;
        }
    } else if (act == SDL_RELEASED) {
        switch (button) {
            case SDL_BUTTON_LEFT:
                w->m_mouseButtonFlags &= ~Window::MOUSE_BUTTON_LEFT;
                break;
            case SDL_BUTTON_MIDDLE:
                w->m_mouseButtonFlags &= ~Window::MOUSE_BUTTON_MIDDLE;
                break;
            case SDL_BUTTON_RIGHT:
                w->m_mouseButtonFlags &= ~Window::MOUSE_BUTTON_RIGHT;
                break;
            default:
                break;
        }
    }
}

void Window::keyCallback(SDL_Window *win, int key, int scancode, int act, int mods) {
    Window *w = reinterpret_cast<Window *>(SDL_GetWindowData(win, WIN_DATA));
    assert(w);
    w->m_keypressed[key] = act == SDL_PRESSED;
}

glm::vec2 Window::mousePosition() const {
    return glm::vec2(m_mousePosition.x / float(m_size.x), 1.0f -
                                                          m_mousePosition.y / float(m_size.y));
}

void Window::stop() { this->shouldClose = true; }

void Window::update() { SDL_GL_SwapWindow(m_window); }

glm::ivec2 Window::windowSize() const {
    glm::ivec2 size(0);
    SDL_GetWindowSize(m_window, &size.x, &size.y);
    return size;
}

void Window::resize(glm::ivec2 const &windowsize) {
    SDL_SetWindowSize(m_window, windowsize.x, windowsize.y);
}
