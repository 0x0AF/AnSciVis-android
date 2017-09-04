// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
//                  2015 Sebastian Thiele
// License    : MIT (see the file LICENSE)
// Maintainer : Sebastian Thiele <sebastian.thiele@uni-weimar.de>
// Stability  : experimantal exercise
//
// scivis exercise Example
// -----------------------------------------------------------------------------

#define _USE_MATH_DEFINES

#include <../framework/fensterchen.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream> // std::stringstream
#include <stdexcept>
#include <string>

/// GLM INCLUDES
#define GLM_FORCE_RADIANS

#include <../glm/gtc/matrix_access.hpp>
#include <../glm/gtc/matrix_transform.hpp>
#include <../glm/gtc/type_ptr.hpp>
#include <../glm/gtx/norm.hpp>
#include <../glm/gtx/transform.hpp>

/// PROJECT INCLUDES
#include <../framework/transfer_function.hpp>
#include <../framework/turntable.hpp>
#include <../framework/utils.hpp>
#include <../framework/volume_loader_raw.hpp>
#include <../imgui/imgui.h>

#define STB_IMAGE_IMPLEMENTATION

#include <../stb-image/stb_image.h> // stb_image.h for PNG loading

#define OFFSETOF(TYPE, ELEMENT) ((size_t) & (((TYPE *)0)->ELEMENT))

/// IMGUI INCLUDES
#include <../imgui/imgui_impl_sdl_gles2.h>

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

#undef PI
const float PI = 3.14159265358979323846f;

#ifdef INT_MAX
#define IM_INT_MAX INT_MAX
#else
#define IM_INT_MAX 2147483647
#endif

const std::string g_file_vertex_shader("shaders/volume.vert");
const std::string g_file_fragment_shader("shaders/volume.frag");

GLuint loadShaders(std::string const &vs, std::string const &fs,
                   const int task_nbr, const int enable_lighting,
                   const int enable_shadowing, const int enable_opeacity_cor) {
  std::string v = readFile(vs);
  std::string f = readFile(fs);

  std::stringstream ss1;
  ss1 << task_nbr;

  int index = (int)f.find("#define TASK");
  f.replace(index + 13, 2, ss1.str());

  std::stringstream ss2;
  ss2 << enable_opeacity_cor;

  index = (int)f.find("#define ENABLE_OPACITY_CORRECTION");
  f.replace(index + 34, 1, ss2.str());

  std::stringstream ss3;
  ss3 << enable_lighting;

  index = (int)f.find("#define ENABLE_LIGHTNING");
  f.replace(index + 25, 1, ss3.str());

  std::stringstream ss4;
  ss4 << enable_shadowing;

  index = (int)f.find("#define ENABLE_SHADOWING");
  f.replace(index + 25, 1, ss4.str());

  // std::cout << f << std::endl;

  return createProgram(v, f);
}

struct RaycasterConf {
  /// SETUP VOLUME RAYCASTER HERE
  // set the volume file
  std::string g_file_string = "data/head_w256_h256_d225_c1_b8.raw";

  // set the sampling distance for the ray traversal
  float g_sampling_distance = 0.01f;
  float g_sampling_distance_fact = 8.0f;
  float g_sampling_distance_fact_move = 10.0f;
  float g_sampling_distance_fact_ref = 1.0f;

  float g_iso_value = 0.2f;

  // set the light position and color for shading
  // set the light position and color for shading
  glm::vec3 g_light_pos = glm::vec3(-4.0, -4.0, -4.0);
  glm::vec3 g_ambient_light_color = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 g_diffuse_light_color = glm::vec3(0.5f, 0.5f, 0.5f);
  glm::vec3 g_specula_light_color = glm::vec3(0.5f, 0.5f, 0.5f);
  float g_ref_coef = 12.0;

  // set backgorund color here
  glm::vec3 g_background_color = glm::vec3(0.5f, 0.3f, 0.3f); // debug
  //    glm::vec3 g_background_color = glm::vec3(0.08f, 0.08f, 0.08f); // grey

  glm::ivec2 g_window_res = glm::ivec2(320, 200);

  std::string g_error_message;
  bool g_reload_shader_error = false;

  int g_current_tf_data_value = 0;
  GLuint g_transfer_texture;
  bool g_transfer_dirty = true;
  bool g_redraw_tf = true;
  bool g_lighting_toggle = false;
  bool g_shadow_toggle = false;
  bool g_opacity_correction_toggle = false;

  // imgui variables
  bool g_show_gui = true;
  bool mousePressed[2] = {false, false};

  bool g_show_transfer_function_in_window = false;
  glm::vec2 g_transfer_function_pos = glm::vec2(0.0f);
  glm::vec2 g_transfer_function_size = glm::vec2(0.0f);

  // imgui values
  bool g_over_gui = false;
  bool g_reload_shader = false;
  bool g_reload_shader_pressed = false;
  bool g_show_transfer_function = false;

  int g_task_chosen = 10;
  int g_task_chosen_old = g_task_chosen;

  bool g_pause = false;

  glm::ivec3 g_vol_dimensions;
  glm::vec3 g_max_volume_bounds;
  unsigned g_channel_size = 0;
  unsigned g_channel_count = 0;
  GLuint g_volume_texture = 0;

  int g_bilinear_interpolation = false;

  bool first_frame = true;
};

struct Manipulator {
  Manipulator() : m_turntable() {}

  glm::mat4 matrix() {
    int32_t current_millis = SDL_GetTicks();
    glm::vec2 m_zero = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_delta =
        glm::vec2(velocity * (current_millis - last_millis), 0.0f);
    m_turntable.rotate(m_zero, m_delta);
    last_millis = current_millis;
    return m_turntable.matrix();
  }

private:
  Turntable m_turntable;
  int32_t last_millis;
  float velocity = 0.001;
};

bool read_volume(std::string &volume_string, RaycasterConf &conf,
                 Volume_loader_raw &g_volume_loader,
                 volume_data_type &g_volume_data, Cube &g_cube) {
  // init volume g_volume_loader
  // Volume_loader_raw g_volume_loader;
  // read volume dimensions
  conf.g_vol_dimensions = g_volume_loader.get_dimensions(conf.g_file_string);

  conf.g_sampling_distance = 1.0f / glm::max(glm::max(conf.g_vol_dimensions.x,
                                                      conf.g_vol_dimensions.y),
                                             conf.g_vol_dimensions.z);

  unsigned max_dim =
      std::max(std::max(conf.g_vol_dimensions.x, conf.g_vol_dimensions.y),
               conf.g_vol_dimensions.z);

  // calculating max volume bounds of volume (0.0 .. 1.0)
  conf.g_max_volume_bounds =
      glm::vec3(conf.g_vol_dimensions) / glm::vec3((float)max_dim);

  // loading volume file data
  g_volume_data = g_volume_loader.load_volume(conf.g_file_string);
  conf.g_channel_size =
      g_volume_loader.get_bit_per_channel(conf.g_file_string) / 8;
  conf.g_channel_count = g_volume_loader.get_channel_count(conf.g_file_string);

  // setting up proxy geometry
  g_cube.freeVAO();
  g_cube = Cube(glm::vec3(0.0, 0.0, 0.0), conf.g_max_volume_bounds);

  glActiveTexture(GL_TEXTURE0);
  conf.g_volume_texture = createTexture3D(
      conf.g_vol_dimensions.x, conf.g_vol_dimensions.y, conf.g_vol_dimensions.z,
      conf.g_channel_size, conf.g_channel_count, (char *)&g_volume_data[0]);

  return 0 != conf.g_volume_texture;
}

void UpdateImGui(RaycasterConf &conf, Window &g_win) {
  ImGuiIO &io = ImGui::GetIO();

  int w, h;
  int display_w, display_h;
  SDL_GetWindowSize(g_win.getSDLwindow(), &w, &h);
  SDL_GL_GetDrawableSize(g_win.getSDLwindow(), &display_w, &display_h);
  io.DisplaySize = ImVec2((float)display_w, (float)display_h);

  static double time = 0.0f;
  const double current_time = SDL_GetTicks();
  io.DeltaTime = (float)(current_time - time);
  time = current_time;

  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  mouse_x *= (float)display_w / w;
  mouse_y *= (float)display_h / h;
  io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
  io.MouseDown[0] = conf.mousePressed[0] ||
                    SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);
  io.MouseDown[1] =
      conf.mousePressed[1] ||
      SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT);

  ImGui_ImplSdlGLES2_NewFrame();
}

void showGUI(RaycasterConf &conf, Window &g_win,
             Transfer_function &g_transfer_fun,
             Volume_loader_raw &g_volume_loader,
             volume_data_type &g_volume_data, Cube &g_cube) {
  ImGui::Begin("Volume Settings", &conf.g_show_gui, ImVec2(300, 500));
  static float f;
  conf.g_over_gui = ImGui::IsMouseHoveringAnyWindow();

  // Calculate and show frame rate
  static ImVector<float> ms_per_frame;
  if (ms_per_frame.empty()) {
    ms_per_frame.resize(400);
    memset(&ms_per_frame.front(), 0, ms_per_frame.size() * sizeof(float));
  }
  static int ms_per_frame_idx = 0;
  static float ms_per_frame_accum = 0.0f;
  if (!conf.g_pause) {
    ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
    ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
    ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];

    ms_per_frame_idx = (ms_per_frame_idx + 1) % ms_per_frame.size();
  }
  const float ms_per_frame_avg = ms_per_frame_accum / 120;

  if (ImGui::CollapsingHeader("Task", 0, true, true)) {
    if (ImGui::TreeNode("Introduction")) {
      ImGui::RadioButton("Max Intensity Projection", &conf.g_task_chosen, 10);
      ImGui::RadioButton("Average Intensity Projection", &conf.g_task_chosen,
                         11);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Iso Surface Rendering")) {
      ImGui::RadioButton("Inaccurate", &conf.g_task_chosen, 12);
      ImGui::RadioButton("Binary Search", &conf.g_task_chosen, 13);
      ImGui::SliderFloat("Iso Value", &conf.g_iso_value, 0.0f, 1.0f, "%.8f",
                         1.0f);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Direct Volume Rendering")) {
      ImGui::RadioButton("Compositing", &conf.g_task_chosen, 31);
      ImGui::TreePop();
    }

    conf.g_reload_shader ^= ImGui::Checkbox("1", &conf.g_lighting_toggle);
    ImGui::SameLine();
    conf.g_task_chosen == 31 || conf.g_task_chosen == 12 ||
            conf.g_task_chosen == 13
        ? ImGui::Text("Enable Lighting")
        : ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.2f, 0.5f), "Enable Lighting");

    conf.g_reload_shader ^= ImGui::Checkbox("2", &conf.g_shadow_toggle);
    ImGui::SameLine();
    conf.g_task_chosen == 31 || conf.g_task_chosen == 12 ||
            conf.g_task_chosen == 13
        ? ImGui::Text("Enable Shadows")
        : ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.2f, 0.5f), "Enable Shadows");

    conf.g_reload_shader ^=
        ImGui::Checkbox("3", &conf.g_opacity_correction_toggle);
    ImGui::SameLine();
    conf.g_task_chosen == 31
        ? ImGui::Text("Opacity Correction")
        : ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.2f, 0.5f),
                             "Opacity Correction");

    if (conf.g_task_chosen != conf.g_task_chosen_old) {
      conf.g_reload_shader = true;
      conf.g_task_chosen_old = conf.g_task_chosen;
    }
  }

  if (ImGui::CollapsingHeader("Load Volumes", 0, true, false)) {
    bool load_volume_1 = false;
    bool load_volume_2 = false;
    bool load_volume_3 = false;

    ImGui::Text("Volumes");
    load_volume_1 ^= ImGui::Button("Load Volume Head");
    load_volume_2 ^= ImGui::Button("Load Volume Engine");
    load_volume_3 ^= ImGui::Button("Load Volume Bucky");

    if (load_volume_1) {
      conf.g_file_string = "data/head_w256_h256_d225_c1_b8.raw";
      read_volume(conf.g_file_string, conf, g_volume_loader, g_volume_data,
                  g_cube);
    }
    if (load_volume_2) {
      conf.g_file_string = "data/Engine_w256_h256_d256_c1_b8.raw";
      read_volume(conf.g_file_string, conf, g_volume_loader, g_volume_data,
                  g_cube);
    }

    if (load_volume_3) {
      conf.g_file_string = "data/Bucky_uncertainty_data_w32_h32_d32_c1_b8.raw";
      read_volume(conf.g_file_string, conf, g_volume_loader, g_volume_data,
                  g_cube);
    }
  }

  if (ImGui::CollapsingHeader("Lighting Settings")) {
    ImGui::SliderFloat3("Position Light", &conf.g_light_pos[0], -10.0f, 10.0f);

    ImGui::ColorEdit3("Ambient Color", &conf.g_ambient_light_color[0]);
    ImGui::ColorEdit3("Diffuse Color", &conf.g_diffuse_light_color[0]);
    ImGui::ColorEdit3("Specular Color", &conf.g_specula_light_color[0]);

    ImGui::SliderFloat("Reflection Coefficient kd", &conf.g_ref_coef, 0.0f,
                       20.0f, "%.5f", 1.0f);
  }

  if (ImGui::CollapsingHeader("Quality Settings")) {
    ImGui::Text("Interpolation");
    ImGui::RadioButton("Nearest Neighbour", &conf.g_bilinear_interpolation, 0);
    ImGui::RadioButton("Bilinear", &conf.g_bilinear_interpolation, 1);

    ImGui::Text("Slamping Size");
    ImGui::SliderFloat("sampling factor", &conf.g_sampling_distance_fact, 0.1f,
                       10.0f, "%.5f", 4.0f);
    ImGui::SliderFloat("sampling factor move",
                       &conf.g_sampling_distance_fact_move, 0.1f, 10.0f, "%.5f",
                       4.0f);
    ImGui::SliderFloat("reference sampling factor",
                       &conf.g_sampling_distance_fact_ref, 0.1f, 10.0f, "%.5f",
                       4.0f);
  }

  if (ImGui::CollapsingHeader("Shader", 0, true, true)) {
    static ImVec4 text_color(1.0, 1.0, 1.0, 1.0);

    if (conf.g_reload_shader_error) {
      text_color = ImVec4(1.0, 0.0, 0.0, 1.0);
      ImGui::TextColored(text_color, "Shader Error");
    } else {
      text_color = ImVec4(0.0, 1.0, 0.0, 1.0);
      ImGui::TextColored(text_color, "Shader Ok");
    }

    ImGui::TextWrapped("%s", conf.g_error_message.c_str());

    conf.g_reload_shader ^= ImGui::Button("Reload Shader");
  }

  if (ImGui::CollapsingHeader("Timing")) {
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                ms_per_frame_avg, 1000.0f / ms_per_frame_avg);

    float min = *std::min_element(ms_per_frame.begin(), ms_per_frame.end());
    float max = *std::max_element(ms_per_frame.begin(), ms_per_frame.end());

    if (max - min < 10.0f) {
      float mid = (max + min) * 0.5f;
      min = mid - 5.0f;
      max = mid + 5.0f;
    }

    static size_t values_offset = 0;

    char buf[50];
    sprintf(buf, "avg %f", ms_per_frame_avg);
    ImGui::PlotLines("Frame Times", &ms_per_frame.front(),
                     (int)ms_per_frame.size(), (int)values_offset, buf,
                     min - max * 0.1f, max * 1.1f, ImVec2(0, 70));

    ImGui::SameLine();
    ImGui::Checkbox("pause", &conf.g_pause);
  }

  if (ImGui::CollapsingHeader("Window options")) {
    if (ImGui::TreeNode("Window Size")) {
      const char *items[] = {"640x480",   "720x576",   "1280x720",
                             "1920x1080", "1920x1200", "2048x1536"};
      static int item2 = -1;
      bool press =
          ImGui::Combo("Window Size", &item2, items, IM_ARRAYSIZE(items));

      if (press) {
        glm::ivec2 win_re_size = glm::ivec2(640, 480);

        switch (item2) {
        case 0:
          win_re_size = glm::ivec2(640, 480);
          break;
        case 1:
          win_re_size = glm::ivec2(720, 576);
          break;
        case 2:
          win_re_size = glm::ivec2(1280, 720);
          break;
        case 3:
          win_re_size = glm::ivec2(1920, 1080);
          break;
        case 4:
          win_re_size = glm::ivec2(1920, 1200);
          break;
        case 5:
          win_re_size = glm::ivec2(1920, 1536);
          break;
        default:
          break;
        }
        g_win.resize(win_re_size);
      }

      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Background Color")) {
      ImGui::ColorEdit3("BC", &conf.g_background_color[0]);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Style Editor")) {
      ImGui::ShowStyleEditor();
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Logging")) {
      ImGui::LogButtons();
      ImGui::TreePop();
    }
  }

  ImGui::End();
  conf.g_show_transfer_function =
      ImGui::Begin("Transfer Function Window",
                   &conf.g_show_transfer_function_in_window, ImVec2(300, 500));

  conf.g_transfer_function_pos.x = ImGui::GetItemRectMin().x;
  conf.g_transfer_function_pos.y = ImGui::GetItemRectMin().y;

  conf.g_transfer_function_size.x =
      ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
  conf.g_transfer_function_size.y =
      ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;

  static unsigned byte_size = 255;

  static ImVector<float> A;
  if (A.empty()) {
    A.resize(byte_size);
  }

  if (conf.g_redraw_tf) {
    conf.g_redraw_tf = false;

    image_data_type color_con =
        g_transfer_fun.get_RGBA_transfer_function_buffer();

    for (unsigned i = 0; i != byte_size; ++i) {
      A[i] = color_con[i * 4 + 3];
    }
  }

  ImGui::PlotLines("", &A.front(), (int)A.size(), (int)0, "", 0.0, 255.0,
                   ImVec2(0, 70));

  conf.g_transfer_function_pos.x = ImGui::GetItemRectMin().x;
  conf.g_transfer_function_pos.y =
      ImGui::GetIO().DisplaySize.y - ImGui::GetItemRectMin().y - 70;

  conf.g_transfer_function_size.x =
      ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
  conf.g_transfer_function_size.y =
      ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;

  ImGui::SameLine();
  ImGui::Text("Color:RGB Plot: Alpha");

  static int data_value = 0;
  ImGui::SliderInt("Data Value", &data_value, 0, 255);
  static float col[4] = {0.4f, 0.7f, 0.0f, 0.5f};
  ImGui::ColorEdit4("color", col);
  bool add_entry_to_tf = false;
  add_entry_to_tf ^= ImGui::Button("Add entry");
  ImGui::SameLine();

  bool reset_tf = false;
  reset_tf ^= ImGui::Button("Reset");

  if (reset_tf) {
    g_transfer_fun.reset();
    conf.g_transfer_dirty = true;
    conf.g_redraw_tf = true;
  }

  if (add_entry_to_tf) {
    conf.g_current_tf_data_value = data_value;
    g_transfer_fun.add((unsigned)data_value,
                       glm::vec4(col[0], col[1], col[2], col[3]));
    conf.g_transfer_dirty = true;
    conf.g_redraw_tf = true;
  }

  if (ImGui::CollapsingHeader("Manipulate Values")) {
    Transfer_function::container_type con =
        g_transfer_fun.get_piecewise_container();

    bool delete_entry_from_tf = false;

    static std::vector<int> g_c_data_value;

    if (g_c_data_value.size() != con.size())
      g_c_data_value.resize(con.size());

    int i = 0;

    for (Transfer_function::container_type::iterator c = con.begin();
         c != con.end(); ++c) {
      int c_data_value = c->first;
      glm::vec4 c_color_value = c->second;

      g_c_data_value[i] = c_data_value;

      std::stringstream ss;
      if (c->first < 10)
        ss << c->first << "  ";
      else if (c->first < 100)
        ss << c->first << " ";
      else
        ss << c->first;

      bool change_value = false;

      std::ostringstream is;
      is << i;
      change_value ^=
          ImGui::SliderInt(is.str().c_str(), &g_c_data_value[i], 0, 255);
      ImGui::SameLine();

      if (change_value) {
        if (con.find(g_c_data_value[i]) == con.end()) {
          g_transfer_fun.remove(c_data_value);
          g_transfer_fun.add((unsigned)g_c_data_value[i], c_color_value);
          conf.g_current_tf_data_value = g_c_data_value[i];
          conf.g_transfer_dirty = true;
          conf.g_redraw_tf = true;
        }
      }

      // delete
      bool delete_entry_from_tf = false;
      delete_entry_from_tf ^=
          ImGui::Button(std::string("Delete: ").append(ss.str()).c_str());

      if (delete_entry_from_tf) {
        conf.g_current_tf_data_value = c_data_value;
        g_transfer_fun.remove(conf.g_current_tf_data_value);
        conf.g_transfer_dirty = true;
        conf.g_redraw_tf = true;
      }

      static float n_col[4] = {0.4f, 0.7f, 0.0f, 0.5f};
      memcpy(&n_col, &c_color_value, sizeof(float) * 4);

      bool change_color = false;
      change_color ^= ImGui::ColorEdit4(ss.str().c_str(), n_col);

      if (change_color) {
        g_transfer_fun.add((unsigned)g_c_data_value[i],
                           glm::vec4(n_col[0], n_col[1], n_col[2], n_col[3]));
        conf.g_current_tf_data_value = g_c_data_value[i];
        conf.g_transfer_dirty = true;
        conf.g_redraw_tf = true;
      }

      ImGui::Separator();

      ++i;
    }
  }

  if (ImGui::CollapsingHeader("Transfer Function - Save/Load", 0, true,
                              false)) {
    ImGui::Text("Transferfunctions");
    bool load_tf_1 = false;
    bool load_tf_2 = false;
    bool load_tf_3 = false;
    bool load_tf_4 = false;
    bool load_tf_5 = false;
    bool load_tf_6 = false;
    bool save_tf_1 = false;
    bool save_tf_2 = false;
    bool save_tf_3 = false;
    bool save_tf_4 = false;
    bool save_tf_5 = false;
    bool save_tf_6 = false;

    save_tf_1 ^= ImGui::Button("Save TF1");
    ImGui::SameLine();
    load_tf_1 ^= ImGui::Button("Load TF1");
    save_tf_2 ^= ImGui::Button("Save TF2");
    ImGui::SameLine();
    load_tf_2 ^= ImGui::Button("Load TF2");
    save_tf_3 ^= ImGui::Button("Save TF3");
    ImGui::SameLine();
    load_tf_3 ^= ImGui::Button("Load TF3");
    save_tf_4 ^= ImGui::Button("Save TF4");
    ImGui::SameLine();
    load_tf_4 ^= ImGui::Button("Load TF4");
    save_tf_5 ^= ImGui::Button("Save TF5");
    ImGui::SameLine();
    load_tf_5 ^= ImGui::Button("Load TF5");
    save_tf_6 ^= ImGui::Button("Save TF6");
    ImGui::SameLine();
    load_tf_6 ^= ImGui::Button("Load TF6");

    if (save_tf_1 || save_tf_2 || save_tf_3 || save_tf_4 || save_tf_5 ||
        save_tf_6) {
      Transfer_function::container_type con =
          g_transfer_fun.get_piecewise_container();
      std::vector<Transfer_function::element_type> save_vect;

      for (Transfer_function::container_type::iterator c = con.begin();
           c != con.end(); ++c) {
        save_vect.push_back(*c);
      }

      std::ofstream tf_file;

      if (save_tf_1) {
        tf_file.open("TF1", std::ios::out | std::ofstream::binary);
      }
      if (save_tf_2) {
        tf_file.open("TF2", std::ios::out | std::ofstream::binary);
      }
      if (save_tf_3) {
        tf_file.open("TF3", std::ios::out | std::ofstream::binary);
      }
      if (save_tf_4) {
        tf_file.open("TF4", std::ios::out | std::ofstream::binary);
      }
      if (save_tf_5) {
        tf_file.open("TF5", std::ios::out | std::ofstream::binary);
      }
      if (save_tf_6) {
        tf_file.open("TF6", std::ios::out | std::ofstream::binary);
      }

      // std::copy(save_vect.begin(), save_vect.end(),
      // std::ostreambuf_iterator<char>(tf_file));
      tf_file.write((char *)&save_vect[0],
                    sizeof(Transfer_function::element_type) * save_vect.size());
      tf_file.close();
    }

    if (load_tf_1 || load_tf_2 || load_tf_3 || load_tf_4 || load_tf_5 ||
        load_tf_6) {
      Transfer_function::container_type con =
          g_transfer_fun.get_piecewise_container();
      std::vector<Transfer_function::element_type> load_vect;

      std::ifstream tf_file;

      if (load_tf_1) {
        tf_file.open("TF1", std::ios::in | std::ifstream::binary);
      }
      if (load_tf_2) {
        tf_file.open("TF2", std::ios::in | std::ifstream::binary);
      }
      if (load_tf_3) {
        tf_file.open("TF3", std::ios::in | std::ifstream::binary);
      }
      if (load_tf_4) {
        tf_file.open("TF4", std::ios::in | std::ifstream::binary);
      }
      if (load_tf_5) {
        tf_file.open("TF5", std::ios::in | std::ifstream::binary);
      }
      if (load_tf_6) {
        tf_file.open("TF6", std::ios::in | std::ifstream::binary);
      }

      if (tf_file.good()) {
        tf_file.seekg(0, tf_file.end);

        size_t size = tf_file.tellg();
        unsigned elements =
            (int)size / (unsigned)sizeof(Transfer_function::element_type);
        tf_file.seekg(0);

        load_vect.resize(elements);
        tf_file.read((char *)&load_vect[0], size);

        g_transfer_fun.reset();
        conf.g_transfer_dirty = true;
        for (std::vector<Transfer_function::element_type>::iterator c =
                 load_vect.begin();
             c != load_vect.end(); ++c) {
          g_transfer_fun.add(c->first, c->second);
        }
      }

      tf_file.close();
    }
  }

  ImGui::End();
}

int main(int argc, char **argv) {
  RaycasterConf conf;

  Window g_win(conf.g_window_res);
  GLuint g_volume_program(0);

  Transfer_function g_transfer_fun;
  Volume_loader_raw g_volume_loader;
  volume_data_type g_volume_data;
  Cube g_cube;

  ImGui_ImplSdlGLES2_Init(g_win.getSDLwindow());

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowPadding.x = 10.0f;
  style.WindowPadding.y = 10.0f;
  style.WindowRounding = 16.0f;
  style.ChildWindowRounding = 16.0f;
  style.FramePadding.x = 10.0f;
  style.FramePadding.y = 10.0f;
  style.FrameRounding = 16.0f;
  style.ItemSpacing.x = 10.0f;
  style.ItemSpacing.y = 10.0f;
  style.ItemInnerSpacing.x = 10.0f;
  style.ItemInnerSpacing.y = 10.0f;
  style.TouchExtraPadding.x = 10.0f;
  style.TouchExtraPadding.y = 10.0f;
  style.ScrollbarSize = 30.0f;
  style.ScrollbarRounding = 16.0f;
  style.GrabMinSize = 20.0f;
  style.GrabRounding = 16.0f;

  const GLubyte *renderer = glGetString(GL_RENDERER);
  const GLubyte *vendor = glGetString(GL_VENDOR);
  const GLubyte *version = glGetString(GL_VERSION);
  const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

  std::stringstream ss;
  ss << "\n-------------------------------------------------------------\n";
  ss << "GL Vendor    : " << vendor;
  ss << "\nGL GLRenderer : " << renderer;
  ss << "\nGL Version   : " << version;
  ss << "\nGLSL Version : " << glslVersion;
  ss << "\n-------------------------------------------------------------\n";
  SDL_Log("%s", ss.str().c_str());

  g_transfer_fun.reset();

  g_transfer_fun.add(0.0f, glm::vec4(0.0, 0.0, 0.0, 0.0));
  g_transfer_fun.add(1.0f, glm::vec4(1.0, 1.0, 1.0, 1.0));
  conf.g_transfer_dirty = true;

  bool check = read_volume(conf.g_file_string, conf, g_volume_loader,
                           g_volume_data, g_cube);

  glActiveTexture(GL_TEXTURE1);
  conf.g_transfer_texture = createTexture2D(
      255u, 1u, (char *)&g_transfer_fun.get_RGBA_transfer_function_buffer()[0]);

  try {
    g_volume_program =
        loadShaders(g_file_vertex_shader, g_file_fragment_shader,
                    conf.g_task_chosen, conf.g_lighting_toggle,
                    conf.g_shadow_toggle, conf.g_opacity_correction_toggle);
  } catch (std::logic_error &e) {
    std::stringstream ss;
    ss << e.what() << std::endl;
    conf.g_error_message = ss.str();
    conf.g_reload_shader_error = true;
  }

  Manipulator manipulator;

  while (!g_win.shouldClose) {
    float sampling_fact = conf.g_sampling_distance_fact;
    if (!conf.first_frame > 0.0) {
      // exit window with escape
      if (ImGui::IsKeyPressed(SDLK_ESCAPE)) {
        g_win.stop();
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_LEFT)) {
        conf.g_light_pos.x -= 0.5f;
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_RIGHT)) {
        conf.g_light_pos.x += 0.5f;
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_UP)) {
        conf.g_light_pos.z -= 0.5f;
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_DOWN)) {
        conf.g_light_pos.z += 0.5f;
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_MINUS) ||
          ImGui::IsKeyPressed(SDL_SCANCODE_KP_MINUS)) {
        conf.g_iso_value -= 0.002f;
        conf.g_iso_value = std::max(conf.g_iso_value, 0.0f);
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_EQUALS) ||
          ImGui::IsKeyPressed(SDL_SCANCODE_KP_PLUS)) {
        conf.g_iso_value += 0.002f;
        conf.g_iso_value = std::min(conf.g_iso_value, 1.0f);
      }

      if (ImGui::IsKeyPressed(SDLK_d)) {
        conf.g_sampling_distance -= 0.0001f;
        conf.g_sampling_distance = std::max(conf.g_sampling_distance, 0.0001f);
      }

      if (ImGui::IsKeyPressed(SDLK_s)) {
        conf.g_sampling_distance += 0.0001f;
        conf.g_sampling_distance = std::min(conf.g_sampling_distance, 0.2f);
      }

      if (ImGui::IsKeyPressed(SDLK_r)) {
        conf.g_reload_shader = true;
      }

      if (ImGui::IsMouseDown(SDL_BUTTON_LEFT) ||
          ImGui::IsMouseDown(SDL_BUTTON_MIDDLE) ||
          ImGui::IsMouseDown(SDL_BUTTON_RIGHT)) {
        sampling_fact = conf.g_sampling_distance_fact_move;
      }
    }

    /// reload shader if key R ist pressed
    if (conf.g_reload_shader) {
      GLuint newProgram(0);
      try {
        // std::cout << "Reload shaders" << std::endl;
        newProgram =
            loadShaders(g_file_vertex_shader, g_file_fragment_shader,
                        conf.g_task_chosen, conf.g_lighting_toggle,
                        conf.g_shadow_toggle, conf.g_opacity_correction_toggle);
        conf.g_error_message = "";
      } catch (std::logic_error &e) {
        // std::cerr << e.what() << std::endl;
        std::stringstream ss;
        ss << e.what() << std::endl;
        conf.g_error_message = ss.str();
        conf.g_reload_shader_error = true;
        newProgram = 0;
      }
      if (0 != newProgram) {
        glDeleteProgram(g_volume_program);
        g_volume_program = newProgram;
        conf.g_reload_shader_error = false;
      } else {
        conf.g_reload_shader_error = true;
      }
    }

    if (ImGui::IsKeyPressed(SDLK_r)) {
      if (!conf.g_reload_shader_pressed) {
        conf.g_reload_shader = true;
        conf.g_reload_shader_pressed = true;
      } else {
        conf.g_reload_shader = false;
      }
    } else {
      conf.g_reload_shader = false;
      conf.g_reload_shader_pressed = false;
    }

    if (conf.g_transfer_dirty && !conf.first_frame) {
      conf.g_transfer_dirty = false;

      static unsigned byte_size = 255;

      image_data_type color_con =
          g_transfer_fun.get_RGBA_transfer_function_buffer();

      glActiveTexture(GL_TEXTURE1);
      updateTexture2D(
          conf.g_transfer_texture, 255u, 1u,
          (char *)&g_transfer_fun.get_RGBA_transfer_function_buffer()[0]);
    }

    if (conf.g_bilinear_interpolation) {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glm::ivec2 size = g_win.windowSize();
    glViewport(0, 0, size.x, size.y);
    glClearColor(conf.g_background_color.x, conf.g_background_color.y,
                 conf.g_background_color.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float fovy = 45.0f;
    float aspect = (float)size.x / (float)size.y;
    float zNear = 0.025f, zFar = 10.0f;
    glm::mat4 projection = glm::perspective(fovy, aspect, zNear, zFar);

    glm::vec3 translate_rot =
        conf.g_max_volume_bounds * glm::vec3(-0.5f, -0.5f, -0.5f);
    glm::vec3 translate_pos =
        conf.g_max_volume_bounds * glm::vec3(+0.5f, -0.0f, -0.0f);

    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 1.5f);
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(eye, target, up);

    glm::mat4 turntable_matrix = manipulator.matrix();

    glm::mat4 model_view =
        view * glm::translate(translate_pos) * turntable_matrix *
        glm::rotate(0.5f * float(M_PI), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(0.5f * float(M_PI), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::translate(translate_rot);

    glm::vec4 camera_translate = glm::column(glm::inverse(model_view), 3);
    glm::vec3 camera_location =
        glm::vec3(camera_translate.x, camera_translate.y, camera_translate.z);

    camera_location /= glm::vec3(camera_translate.w);

    glm::vec4 light_location = glm::vec4(conf.g_light_pos, 1.0f) * model_view;

    glBindTexture(GL_TEXTURE_2D, conf.g_transfer_texture);

    glUseProgram(g_volume_program);

    glUniform1i(glGetUniformLocation(g_volume_program, "volume_texture"), 0);
    glUniform1i(glGetUniformLocation(g_volume_program, "transfer_texture"), 1);

    glUniform3fv(glGetUniformLocation(g_volume_program, "camera_location"), 1,
                 glm::value_ptr(camera_location));
    glUniform1f(glGetUniformLocation(g_volume_program, "sampling_distance"),
                conf.g_sampling_distance * sampling_fact);
    glUniform1f(glGetUniformLocation(g_volume_program, "sampling_distance_ref"),
                conf.g_sampling_distance_fact_ref);
    glUniform1f(glGetUniformLocation(g_volume_program, "iso_value"),
                conf.g_iso_value);
    glUniform3fv(glGetUniformLocation(g_volume_program, "max_bounds"), 1,
                 glm::value_ptr(conf.g_max_volume_bounds));
    glUniform3iv(glGetUniformLocation(g_volume_program, "volume_dimensions"), 1,
                 glm::value_ptr(conf.g_vol_dimensions));
    glUniform3fv(glGetUniformLocation(g_volume_program, "light_position"), 1,
                 glm::value_ptr(conf.g_light_pos));
    glUniform3fv(glGetUniformLocation(g_volume_program, "light_ambient_color"),
                 1, glm::value_ptr(conf.g_ambient_light_color));
    glUniform3fv(glGetUniformLocation(g_volume_program, "light_diffuse_color"),
                 1, glm::value_ptr(conf.g_diffuse_light_color));
    glUniform3fv(glGetUniformLocation(g_volume_program, "light_specular_color"),
                 1, glm::value_ptr(conf.g_specula_light_color));
    glUniform1f(glGetUniformLocation(g_volume_program, "light_ref_coef"),
                conf.g_ref_coef);

    glUniformMatrix4fv(glGetUniformLocation(g_volume_program, "Projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(g_volume_program, "Modelview"), 1,
                       GL_FALSE, glm::value_ptr(model_view));
    if (!conf.g_pause)
      g_cube.draw();
    glUseProgram(0);

    ImGuiIO &io = ImGui::GetIO();
    io.MouseWheel = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSdlGLES2_ProcessEvent(&event);

      switch (event.type) {
      case SDL_FINGERMOTION:
      case SDL_MOUSEMOTION:
        g_win.cursorPositionCallback(g_win.getSDLwindow(), event.motion.x,
                                     event.motion.y);
        break;
      case SDL_FINGERDOWN:
        conf.mousePressed[0] = true;
        g_win.mouseButtonCallback(g_win.getSDLwindow(), SDL_BUTTON_LEFT,
                                  SDL_PRESSED, 0);
        break;
      case SDL_FINGERUP:
        conf.mousePressed[0] = false;
        g_win.mouseButtonCallback(g_win.getSDLwindow(), SDL_BUTTON_LEFT,
                                  SDL_RELEASED, 0);
        break;
      case SDL_MOUSEBUTTONDOWN:
        conf.mousePressed[0] = true;
        g_win.mouseButtonCallback(g_win.getSDLwindow(), event.button.button,
                                  SDL_PRESSED, 0);
        break;
      case SDL_MOUSEBUTTONUP:
        conf.mousePressed[0] = false;
        g_win.mouseButtonCallback(g_win.getSDLwindow(), event.button.button,
                                  SDL_RELEASED, 0);
        break;
      case SDL_KEYDOWN:
        g_win.keyCallback(g_win.getSDLwindow(), event.key.keysym.scancode,
                          event.key.keysym.scancode, SDL_PRESSED, 0);
        break;
      case SDL_KEYUP:
        g_win.keyCallback(g_win.getSDLwindow(), event.key.keysym.scancode,
                          event.key.keysym.scancode, SDL_RELEASED, 0);
        break;
      default:
        break;
      }

      if (event.type == SDL_QUIT)
        g_win.shouldClose = true;
    }
    UpdateImGui(conf, g_win);
    showGUI(conf, g_win, g_transfer_fun, g_volume_loader, g_volume_data,
            g_cube);

    // Rendering
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui::Render();
    // IMGUI ROUTINE end

    if (conf.g_show_transfer_function)
      g_transfer_fun.draw_texture(conf.g_transfer_function_pos,
                                  conf.g_transfer_function_size,
                                  conf.g_transfer_texture);
    glBindTexture(GL_TEXTURE_2D, 0);

    g_win.update();
    if (conf.first_frame) {
      conf.first_frame = false;
    }
  }

  ImGui_ImplSdlGLES2_Shutdown();

  return 0;
}
