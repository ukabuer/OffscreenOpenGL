/*
 * see
 * https://devblogs.nvidia.com/egl-eye-opengl-visualization-without-x-server/
 */

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#define EGL_EGLEXT_PROTOTYPES
#define MY_EGL_CHECK()                                                         \
  do {                                                                         \
    auto error = eglGetError();                                                \
    if (error != EGL_SUCCESS) {                                                \
      std::cout << "EGL error  before " << __LINE__ << " of " << __FILE__      \
                << ":  " << get_egl_error_info(error) << std::endl;            \
    }                                                                          \
  } while (0)

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fstream>
#include <glad/glad.h>
#include <iostream>

using namespace std;

namespace {
constexpr int opengl_version[] = {3, 3};

constexpr int pixel_buffer_width = 800;
constexpr int pixel_buffer_height = 600;

constexpr EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                                     EGL_PBUFFER_BIT,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_DEPTH_SIZE,
                                     8,
                                     EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_BIT,
                                     EGL_NONE};

constexpr EGLint pixel_buffer_attribs[] = {
    EGL_WIDTH, pixel_buffer_width, EGL_HEIGHT, pixel_buffer_height, EGL_NONE,
};

constexpr EGLint context_attris[] = {EGL_CONTEXT_MAJOR_VERSION,
                                     opengl_version[0],
                                     EGL_CONTEXT_MINOR_VERSION,
                                     opengl_version[1],
                                     EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                     EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                     EGL_NONE};

EGLDisplay create_display_from_device();
void render(int width, int height);
void write_to_file(int width, int height, const char *filename);
const char *get_egl_error_info(EGLint error);

} // namespace

int main(int argc, char *argv[]) {
  // 1. Initialize EGL
  auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (egl_display == EGL_NO_DISPLAY) {
    cout << "No default display, try to create a display "
            "from devices."
         << endl;
    // try EXT_platform_device, see
    // https://www.khronos.org/registry/EGL/extensions/EXT/EGL_EXT_platform_device.txt
    egl_display = create_display_from_device();
  }

  EGLint major = 0, minor = 0;
  eglInitialize(egl_display, &major, &minor);
  cout << "EGL version: " << major << "." << minor << endl;
  cout << "EGL vendor string: " << eglQueryString(egl_display, EGL_VENDOR)
       << endl;
  MY_EGL_CHECK();

  // 2. Select an appropriate configuration
  EGLint num_configs = 0;
  EGLConfig egl_config = nullptr;
  eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &num_configs);
  MY_EGL_CHECK();

  // 3. Create a surface
  auto egl_surface =
      eglCreatePbufferSurface(egl_display, egl_config, pixel_buffer_attribs);
  MY_EGL_CHECK();

  // 4. Bind the API
  eglBindAPI(EGL_OPENGL_API);
  MY_EGL_CHECK();

  // 5. Create a context and make it current
  auto egl_ctx =
      eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attris);
  MY_EGL_CHECK();

  eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_ctx);
  MY_EGL_CHECK();

  // from now on use your OpenGL context
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&eglGetProcAddress))) {
    cout << "Load OpenGL " << opengl_version[0] << "." << opengl_version[1]
         << "failed" << endl;
    return -1;
  }

  cout << "OpenGL version: " << GLVersion.major << "." << GLVersion.minor
       << endl;

  // render and save
  render(pixel_buffer_width, pixel_buffer_height);
  const char *filename = "image.ppm";
  write_to_file(pixel_buffer_width, pixel_buffer_height, filename);
  cout << "save to file: " << filename << endl;

  // 6. Terminate EGL when finished
  eglTerminate(egl_display);
  return 0;
}

namespace {

EGLDisplay create_display_from_device() {
  auto eglGetPlatformDisplayEXT =
      reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (eglGetPlatformDisplayEXT == nullptr) {
    cerr << "eglGetPlatformDisplayEXT not found" << endl;
    return EGL_NO_DISPLAY;
  }

  auto eglQueryDevicesEXT = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(
      eglGetProcAddress("eglQueryDevicesEXT"));
  if (eglQueryDevicesEXT == nullptr) {
    cerr << "eglQueryDevicesEXT not found" << endl;
    return EGL_NO_DISPLAY;
  }

  EGLint max_devices = 0, num_devices = 0;
  eglQueryDevicesEXT(0, nullptr, &max_devices);

  auto devices = new EGLDeviceEXT[max_devices];
  eglQueryDevicesEXT(max_devices, devices, &num_devices);

  EGLint major = 0, minor = 0;
  cout << "Detected " << num_devices << " devices." << endl;
  for (int i = 0; i < num_devices; ++i) {
    auto display =
        eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
    if (!eglInitialize(display, &major, &minor)) {
      cout << "  Failed to initialize EGL with device " << i << endl;
      continue;
    }

    cout << "  Device #" << i << " is used." << endl;
    return display;
  }

  return EGL_NO_DISPLAY;
}

void render(int width, int height) {
  static const char *vs_source = R"(
#version 330 core
const vec2 positions[3] = vec2[3](
  vec2(0.0f, 0.5f), vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f)
);
const vec3 colors[3] = vec3[3](
  vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)
);
out vec3 v_color;
void main() {
  gl_Position = vec4(positions[gl_VertexID], 1.0f, 1.0f);
  v_color = colors[gl_VertexID];
})";
  static const char *fs_source = R"(
#version 330 core
in vec3 v_color;
out vec4 frag_color;
void main() {
frag_color = vec4(v_color, 1.0f);
})";
  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_source, nullptr);
  glCompileShader(vs);
  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_source, nullptr);
  glCompileShader(fs);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(program);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glFlush();
}

void write_to_file(int width, int height, const char *filename) {
  auto image = new unsigned char[width * height * 3];
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

  std::ofstream file{filename};
  file << "P3" << std::endl
       << width << " " << height << std::endl
       << 255 << std::endl;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      const int offset = ((height - j) * width + i) * 3;
      file << static_cast<int32_t>(image[offset]) << " "
           << static_cast<int32_t>(image[offset + 1]) << " "
           << static_cast<int32_t>(image[offset + 2]) << std::endl;
    }
  }

  delete[] image;
}

const char *get_egl_error_info(EGLint error) {
  switch (error) {
  case EGL_NOT_INITIALIZED:
    return "EGL_NOT_INITIALIZED";
  case EGL_BAD_ACCESS:
    return "EGL_BAD_ACCESS";
  case EGL_BAD_ALLOC:
    return "EGL_BAD_ALLOC";
  case EGL_BAD_ATTRIBUTE:
    return "EGL_BAD_ATTRIBUTE";
  case EGL_BAD_CONTEXT:
    return "EGL_BAD_CONTEXT";
  case EGL_BAD_CONFIG:
    return "EGL_BAD_CONFIG";
  case EGL_BAD_CURRENT_SURFACE:
    return "EGL_BAD_CURRENT_SURFACE";
  case EGL_BAD_DISPLAY:
    return "EGL_BAD_DISPLAY";
  case EGL_BAD_SURFACE:
    return "EGL_BAD_SURFACE";
  case EGL_BAD_MATCH:
    return "EGL_BAD_MATCH";
  case EGL_BAD_PARAMETER:
    return "EGL_BAD_PARAMETER";
  case EGL_BAD_NATIVE_PIXMAP:
    return "EGL_BAD_NATIVE_PIXMAP";
  case EGL_BAD_NATIVE_WINDOW:
    return "EGL_BAD_NATIVE_WINDOW";
  case EGL_CONTEXT_LOST:
    return "EGL_CONTEXT_LOST";
  case EGL_SUCCESS:
    return "NO ERROR";
  default:
    return "UNKNOWN ERROR";
  }
}

} // namespace