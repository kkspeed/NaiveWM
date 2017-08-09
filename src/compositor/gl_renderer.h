#ifndef COMPOSITOR_GL_RENDERER_H_
#define COMPOSITOR_GL_RENDERER_H_

#include "compositor/compositor_view.h"

#include <GLES3/gl3.h>
#include <cstdint>
#include <glm/glm.hpp>

namespace naive {
namespace compositor {

class GlRenderer {
 public:
  explicit GlRenderer(int32_t width, int32_t height);
  ~GlRenderer();

  void DrawTextureQuad(GLint coords[],
                       GLfloat texture_coords[],
                       GLuint texture);
  void DrawSolidQuad(GLint* coords, float r, float g, float b, bool fill);

 private:
  int32_t screen_width_;
  int32_t screen_height_;
  GLuint shader_program_;
  GLuint solid_shader_program_;
  GLuint vertex_buffer_, uvbuffer_;
  GLuint vertex_array_id_;

  GLint matrix_id_, texture_id_, fill_color_, solid_mvp_;

  glm::mat4 mvp_;
};

}  // namespace compositor
}  // namespace naive

#endif  // COMPOSITOR_GL_RENDERER_H_
