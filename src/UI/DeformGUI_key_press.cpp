//
// Created by Zhongshi Jiang on 3/22/17.
//
#include "DeformGUI.h"
#include "../ReWeightedARAP.h"
#include "../StateManager.h"
#include "../util/triangle_utils.h"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
// #include <nanogui/formhelper.h>
#include <igl/slim.h>
#include <igl/file_dialog_save.h>
#include <igl/file_dialog_open.h>
#include <igl/boundary_loop.h>
#include <igl/colon.h>
#include <igl/unproject_onto_mesh.h>
#include <igl/unproject.h>
#include <igl/project.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/writeOBJ.h>
#include <igl/doublearea.h>
#include <igl/triangle_triangle_adjacency.h>

#include "../Newton/SchaeferNewtonParametrizer.h"

#include <igl/Timer.h>
#include <igl/flip_avoiding_line_search.h>
#include <igl/cat.h>

namespace igl {
namespace slim {
double compute_energy(igl::SLIMData &s, Eigen::MatrixXd &V_new);
};
};

void Write_TriMesh_Obj(Eigen::MatrixXd w_uv, Eigen::MatrixXi surface_F, const std::string& filename)
{
    FILE *file = fopen(filename.c_str(), "w");
    if (!file) {
        puts("failed to create file");
        exit(-1);
    }
    for (int i = 0; i < w_uv.rows(); ++i) {
        fprintf(file, "v %le %le %le\n", w_uv(i, 0), w_uv(i, 1), w_uv(i, 2));
    }
    for (int i = 0; i < surface_F.rows(); ++i) {
        fprintf(file, "f %d %d %d\n", surface_F(i, 0) + 1, surface_F(i, 1) + 1, surface_F(i, 2) + 1);
    }
    fclose(file);
}

bool DeformGUI::key_press(unsigned int key, int mod, std::string filename) {
  using namespace Eigen;
  using namespace std;
  ScafData &sd = s_.scaf_data;
  auto ws_solver = s_.ws_solver;
  bool adjust_frame = false;
  bool use_square_frame = false;
  switch (s_.demo_type){
    case DemoType::PACKING:
      adjust_frame = true;
      use_square_frame = true;
      break;
     case DemoType::FLOW:
      adjust_frame = false;
      use_square_frame = false;
      break;
     case DemoType::PARAM:
      adjust_frame = true;
      use_square_frame = false;
      break;
    }

  static int frame = 0;
  std::string out_folder = filename.substr(3, filename.size() - 7);
  printf("!!!!!!!!!!!!!!!!!!!! %s\n", out_folder.c_str());
  if (mkdir(out_folder.c_str(), 0777) == -1)
    cout << "Error :  " << strerror(errno) << endl;
  else
    cout << "Directory created";
  printf("==================== %d %d\n", sd.m_T.rows(), sd.m_V.rows());
  Eigen::MatrixXd old_w_uv = sd.w_uv;
  Write_TriMesh_Obj(sd.w_uv, sd.surface_F, out_folder + std::string("/") + std::to_string(++frame) + std::string(".obj"));

  static double last_mesh_energy = ws_solver->compute_energy(sd.w_uv, false)/
      sd.mesh_measure - 2*d_.dim;
  static double accumulated_time = 0;
  if (key == ' ') {   /// Iterates.

    for (int i = 0; i < s_.	inner_iters; i++) {
      std::cout << "=============" << std::endl;
      std::cout << "Iteration:" << s_.iter_count++ << '\t';
      igl::Timer timer;

      timer.start();
      if(s_.optimize_scaffold) {
        d_.mesh_improve(use_square_frame, adjust_frame);
        if(!s_.use_newton) ws_solver->after_mesh_improve();
      } else {
        d_.automatic_expand_frame(2,3);
        ws_solver->after_mesh_improve();
      }
      if(s_.auto_weight)
         d_.set_scaffold_factor(
          (last_mesh_energy)*sd.mesh_measure /(sd.sf_num) / 100.0);

      if(!s_.use_newton) {
        sd.energy = ws_solver->perform_iteration(sd.w_uv);
      } else {
        // no precomputation.
        SchaeferNewtonParametrizer snp(sd);

        Eigen::MatrixXd Vo = sd.w_uv;
        MatrixXi w_T;
        igl::cat(1,sd.m_T,sd.s_T,w_T);
        snp.newton_iteration(w_T, Vo);

        auto whole_E =
            [&](Eigen::MatrixXd &uv) { return snp.evaluate_energy(w_T, uv);};

//        sd.w_uv = Vo;
        sd.energy = igl::flip_avoiding_line_search(w_T, sd.w_uv, Vo,
                                                      whole_E, -1)
            / d_.mesh_measure;
      }

      double current_mesh_energy =
          ws_solver->compute_energy(sd.w_uv, false) / sd.mesh_measure- 2*d_.dim;
      double mesh_energy_decrease = last_mesh_energy - current_mesh_energy;
      double iter_timing = timer.getElapsedTime();
      accumulated_time += iter_timing;
      cout << "Timing = " << iter_timing << "Total = "<<accumulated_time<< endl;
      cout << "Energy After:"
           << sd.energy - 2*d_.dim
           << "\tMesh Energy:"
           << current_mesh_energy
           << "\tEnergy Decrease"
           << mesh_energy_decrease
           << endl;
      cout << "V_num: "<<d_.v_num<<" F_num: "<<d_.f_num<<endl;
      last_mesh_energy = current_mesh_energy;

    }
    double inf_norm = 0;
    for (int i = 0; i < sd.m_V.rows(); ++i)
      for (int d = 0; d < d_.dim; ++d)
        inf_norm = std::max(std::abs(sd.w_uv(i, d) - old_w_uv(i, d)), inf_norm);
    printf("[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]] %.10f\n", inf_norm);
    if (inf_norm < 0.6 * 1e-5)
      exit(0);
  }

  switch (key) {
    case '0':
    case ' ':v_.data().clear();
      v_.data().set_mesh(sd.w_uv, sd.surface_F);
      v_.data().set_face_based(true);
      break;
    default:return false;
  }

  scaffold_coloring();
  return true;
}
