#ifndef PLANE_HPP
#define PLANE_HPP

// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
// License    : MIT (see the file LICENSE)
// Maintainer : Andreas-C. Bernstein <andreas.bernstein@uni-weimar.de>
// Stability  : experimental
//
// Cube
// -----------------------------------------------------------------------------

//#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#define GLM_FORCE_RADIANS
#include <../glm/vec2.hpp>
#include <../glm/vec3.hpp>

class Plane
{
public:
  struct Vertex
  {
    Vertex()
      : position(0.0, 0.0, 0.0)
      , texCoord(0.0, 0.0)
    {}

    Vertex(glm::vec3 pos, glm::vec2 tc)
      : position(pos)
      , texCoord(tc)
    {}

    glm::vec3 position;
    glm::vec2 texCoord;
  };
    
  Plane(glm::vec2 min = glm::vec2(-0.0f), glm::vec2 max = glm::vec2(1.0f));
  void draw() const;

private:
    GLuint m_vao;
    GLuint m_ibo;
};

#endif // PLANE_HPP
