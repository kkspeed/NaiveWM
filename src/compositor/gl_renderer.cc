#include "compositor/gl_renderer.h"

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "base/logging.h"

namespace naive {
namespace compositor {

namespace {

const GLchar* kVertexQuadShader =
    "#version 320 es\n"
    "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
    "layout(location = 1) in vec2 vertexUV;\n"
    "out vec2 UV;\n"
    "uniform mat4 MVP;\n"
    "void main() {\n"
    "  gl_Position = MVP * vec4(vertexPosition_modelspace, 1);\n"
    "  UV = vertexUV;\n"
    "}\n";

const GLchar* kFragmentQuadShader =
    "#version 320 es\n"
    "precision mediump float;\n"
    "in vec2 UV;\n"
    "out vec4 color;\n"
    "uniform sampler2D myTextureSampler;\n"
    "void main() {\n"
    "  color = texture(myTextureSampler, UV).bgra;\n"
    "}\n";

const GLchar* kSolidQuadVertexShader =
    "#version 320 es\n"
    "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
    "uniform mat4 MVP;\n"
    "void main() {\n"
    "  gl_Position = MVP * vec4(vertexPosition_modelspace, 1);\n"
    "}\n";

const GLchar* kSolidQuadFragmentShader =
    "#version 320 es\n"
    "precision mediump float;\n"
    "out vec4 color;\n"
    "uniform vec4 fill_color;\n"
    "void main() {\n"
    "  color = fill_color;\n"
    "}\n";

GLuint MakeShaders(const char* vertex, const char* fragment) {
  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

  GLint result = GL_FALSE;
  int info_log_length;

  LOG_INFO << "Compiling vertex shader.";
  glShaderSource(vertex_shader_id, 1, &vertex, nullptr);
  glCompileShader(vertex_shader_id);

  glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
  glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

  if (info_log_length > 0) {
    // TODO: should probably use std::string
    std::vector<char> vertex_shader_error_message(info_log_length + 1);
    glGetShaderInfoLog(vertex_shader_id, info_log_length, nullptr,
                       &vertex_shader_error_message[0]);
    LOG_FATAL << &vertex_shader_error_message[0];
  }

  LOG_INFO << "Compiling fragment shader.";
  glShaderSource(fragment_shader_id, 1, &fragment, nullptr);
  glCompileShader(fragment_shader_id);
  glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
  glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

  if (info_log_length > 0) {
    // TODO: should probably use std::string
    std::vector<char> fragment_shader_error_message(info_log_length + 1);
    glGetShaderInfoLog(fragment_shader_id, info_log_length, nullptr,
                       &fragment_shader_error_message[0]);
    LOG_FATAL << &fragment_shader_error_message[0];
  }

  LOG_INFO << "Linking shader";
  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);

  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

  if (info_log_length > 0) {
    std::vector<char> program_error_message(info_log_length + 1);
    glGetProgramInfoLog(program_id,
                        info_log_length,
                        nullptr,
                        &program_error_message[0]);
    LOG_FATAL << &program_error_message[0];
  }
  glDetachShader(program_id, vertex_shader_id);
  glDetachShader(program_id, fragment_shader_id);

  glDeleteShader(vertex_shader_id);
  glDeleteShader(fragment_shader_id);

  return program_id;
}

}  // namespace

GlRenderer::GlRenderer(int32_t width, int32_t height)
  : screen_width_(width), screen_height_(height) {
  shader_program_ = MakeShaders(kVertexQuadShader, kFragmentQuadShader);
  matrix_id_ = glGetUniformLocation(shader_program_, "MVP");
  texture_id_ = glGetUniformLocation(shader_program_, "myTextureSampler");

  solid_shader_program_ = MakeShaders(kSolidQuadVertexShader, kSolidQuadFragmentShader);
  solid_mvp_ = glGetUniformLocation(solid_shader_program_, "MVP");
  fill_color_ = glGetUniformLocation(solid_shader_program_, "fill_color");

  glm::mat4 projection = glm::ortho(0.0f, (float) width, (float) height, 0.0f, 0.1f, 100.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  glm::mat4 model = glm::mat4(1.0f);
  mvp_ = projection * view * model;

  glGenBuffers(1, &vertex_buffer_);
  glGenBuffers(1, &uvbuffer_);
  glGenVertexArrays(1, &vertex_buffer_);
  glBindVertexArray(vertex_array_id_);
}

GlRenderer::~GlRenderer() {
  glDeleteBuffers(1, &vertex_buffer_);
  glDeleteBuffers(1, &uvbuffer_);
  glDeleteVertexArrays(1, &vertex_array_id_);
}

void GlRenderer::DrawTextureQuad(GLint coords[], GLfloat texture_coords[], GLuint texture) {
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLint), coords, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_);
  glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), texture_coords, GL_STATIC_DRAW);

  glUseProgram(shader_program_);
  glUniformMatrix4fv(matrix_id_, 1, GL_FALSE, &mvp_[0][0]);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(texture_id_, 0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glVertexAttribPointer(0, 2, GL_INT, GL_FALSE, 0, (void*) 0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
}

void GlRenderer::DrawSolidQuad(GLint* coords, float r, float g, float b, bool fill) {
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLint), coords, GL_STATIC_DRAW);
  glUseProgram(solid_shader_program_);
  const GLfloat color[] = {r, g, b, 1.0f};
  glUniform4fv(fill_color_, 1, color);
  glUniformMatrix4fv(solid_mvp_, 1, GL_FALSE, &mvp_[0][0]);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glVertexAttribPointer(0, 2, GL_INT, GL_FALSE, 0, (void*) 0);
  if (fill)
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  else
    glDrawArrays(GL_LINE_LOOP, 0, 4);
  glDisableVertexAttribArray(0);
}

}  // namespace compositor
}  // namespace naive
