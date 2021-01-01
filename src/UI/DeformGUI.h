//
// Created by Zhongshi Jiang on 1/25/17.
//

#ifndef SCAFFOLD_TEST_DEFORMGUI_H
#define SCAFFOLD_TEST_DEFORMGUI_H
#include <Eigen/Dense>
#include <igl/slim.h>
#include "../StateManager.h"

namespace igl {
class SLIMData;
namespace opengl {namespace glfw{
class Viewer;
namespace imgui{
class ImGuiMenu;
}
}}
}
class StateManager;
class ScafData;

class DeformGUI {
 public:
  DeformGUI(igl::opengl::glfw::Viewer &,StateManager &);

  bool key_press(unsigned int key, int mod, std::string filename=std::string(""));

 private:
  bool mouse_down(int button, int mod);
  bool mouse_up(int button, int mod);
  bool mouse_move(int mouse_x, int mouse_y);
  bool render_to_png(const int width, const int height,
                const std::string png_file);

  bool extended_menu();
  void show_box();

  void scaffold_coloring();

  // picking
  int picked_vert_; //assume mesh stay constant throughout;
  bool dragging_ = false;
  float down_z_;
  Eigen::RowVectorXd offset_picking; // clicked - nearest vert

  // members
  ScafData& d_;
  igl::opengl::glfw::Viewer& v_;
  StateManager &s_;

  // coloring and overlay.
  Eigen::MatrixXd mesh_color;
  bool show_interior_boundary = false;

  // solving
//  bool first_solve;
  igl::SLIMData Data;
  int inner_iters = 1;
  bool smart_flip;
  bool use_newton = false;
  bool auto_weight = true;
};

#endif //SCAFFOLD_TEST_DEFORMGUI_H
