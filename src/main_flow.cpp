#include "ReWeightedARAP.h"
#include "StateManager.h"
#include "UI/DeformGUI.h"
#include "util/triangle_utils.h"
#include <igl/Timer.h>
#include <igl/opengl/glfw/Viewer.h>
#include <iostream>
#include <memory>
#include <CLI/CLI.hpp>

int main(int argc, char* argv[]) {
  CLI::App command_line{"Scaffold Mapping: Flow Example"};
  std::string filename = "";
  std::string target_file;
  std::string cube_file;
  int demo_type = 1;
  command_line.add_option("-m,--mesh", filename, "Mesh path")->check(CLI::ExistingFile);
  command_line.add_option("-t,--target", target_file, "target file")->check(CLI::ExistingFile);
  command_line.add_option("-c,--cube", cube_file, "cube file")->check(CLI::ExistingFile);
  bool show_gui = true;
  command_line.add_option("-g,--gui", show_gui, "Show GUI");
  try {
      command_line.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
      return command_line.exit(e);
  }

  StateManager s_(static_cast<DemoType>(demo_type), filename, target_file, cube_file);
    igl::opengl::glfw::Viewer v;

    DeformGUI gui(v, s_);
    if (show_gui)
        v.launch();
    else {
        while (s_.iter_count >= 0) {
            gui.key_press(' ', 0, filename);
        }
    }

  return 0;
}
