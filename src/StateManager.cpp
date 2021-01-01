//
// Created by Zhongshi Jiang on 1/20/17.
//

#include "StateManager.h"
#include "util/triangle_utils.h"
#include "util/tet_utils.h"

#include <igl/serialize.h>
#include <igl/boundary_loop.h>
#include <igl/cat.h>
#include <igl/slice_into.h>
#include <igl/local_basis.h>
#include <igl/read_triangle_mesh.h>
#include <igl/polar_svd.h>
#include <igl/write_triangle_mesh.h>
#include <igl/flip_avoiding_line_search.h>
#include <igl/adjacency_matrix.h>

#include <igl/slice.h>
#include <igl/colon.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/harmonic.h>
#include <igl/flipped_triangles.h>
#include <igl/euler_characteristic.h>
#include <igl/is_edge_manifold.h>
#include <igl/doublearea.h>
#include <igl/squared_edge_lengths.h>
#include <igl/PI.h>
#include <igl/readMESH.h>
#include <igl/unique.h>
#include <igl/remove_duplicates.h>
#include <igl/face_occurrences.h>
#include <igl/matrix_to_list.h>
#include <igl/boundary_facets.h>
#include <igl/sort.h>
#include <igl/readOFF.h>

#include <igl/serialize.h>

SERIALIZE_TYPE(ScafData,
SERIALIZE_MEMBER(bnd_sizes)
SERIALIZE_MEMBER(component_sizes)
SERIALIZE_MEMBER(inner_scaf_tets) 
SERIALIZE_MEMBER(dim)
SERIALIZE_MEMBER(frame_ids)
SERIALIZE_MEMBER(internal_bnd)
SERIALIZE_MEMBER(m_M)
SERIALIZE_MEMBER(m_T)
SERIALIZE_MEMBER(m_V)
SERIALIZE_MEMBER(mesh_measure)
SERIALIZE_MEMBER(proximal_p)
SERIALIZE_MEMBER(rect_frame_V)
SERIALIZE_MEMBER(s_M)
SERIALIZE_MEMBER(s_T)
SERIALIZE_MEMBER(scaffold_factor)
SERIALIZE_MEMBER(soft_cons)
SERIALIZE_MEMBER(soft_const_p)
SERIALIZE_MEMBER(surface_F)
SERIALIZE_MEMBER(w_uv)
)

void flow_initializer(std::string, std::string, Eigen::MatrixXd & mTV, Eigen::MatrixXi &mTT,
                         Eigen::MatrixXd &wTV, Eigen::MatrixXi &sTT,
                         Eigen::VectorXi& frame, Eigen::MatrixXi&surf_F, int&, std::string cube_file);

void parameterization_init(std::string filename, Eigen::MatrixXd& V_ref,
                            Eigen::MatrixXi &F_ref,
                           Eigen::MatrixXd& V_all, Eigen::MatrixXi &F_scaf,
                           Eigen::VectorXi &frame_id, Eigen::MatrixXi&disp_F);
//void bars_stack_construction(ScafData& d_);

void tight_packing_init(std::string, ScafData&);
StateManager::StateManager(DemoType demo_type, std::string filename, std::string target_file, std::string cube_file):
model_file(filename),
demo_type(demo_type),
iter_count(0) {
  using namespace Eigen;
//
  MatrixXd V, V0, V1;
  MatrixXi F, T0, T1;

  Eigen::VectorXi frame;
  Eigen::MatrixXi surf;

  switch (demo_type) {
    case DemoType::PACKING :tight_packing_init(filename, scaf_data);
      break;
    case DemoType::PARAM :read_mesh_with_uv_seam(filename, V0, T0);
      this->scaf_data.add_new_patch(V0, T0, RowVector2d(0, 0));
      this->scaf_data.mesh_improve(false, true);
      break;
    case DemoType::BARS:
        assert(false);
//      bars_stack_construction(scaf_data);
      break;
    case DemoType::FLOW:
    {
      int scaf_inner_tets = -1; 
    flow_initializer(model_file, target_file, V0,T0, V1, T1, frame, surf, scaf_inner_tets, cube_file);
    assert(scaf_inner_tets != -1); 
   
    this->scaf_data = ScafData(V0, T0, V1, T1); 
    this->scaf_data.frame_ids = frame; 
    this->scaf_data.surface_F = surf; 
    this->scaf_data.inner_scaf_tets = scaf_inner_tets; 
    break;
  }
  }

  ws_solver.reset(new ReWeightedARAP(scaf_data));
  ws_solver->pre_calc();
}

void StateManager::load(std::string ser_file)
{
  igl::deserialize(scaf_data, "scaf_data", ser_file);
  igl::deserialize(iter_count, "it", ser_file);
  igl::deserialize(model_file, "model", ser_file);

  scaf_data.update_scaffold();
  ws_solver.reset(new ReWeightedARAP(scaf_data));
  ws_solver->pre_calc();
}

void StateManager::save(std::string ser_file)
{
  igl::serialize(model_file, "model", ser_file);
  igl::serialize(iter_count, "it", ser_file);
  igl::serialize(scaf_data, "scaf_data", ser_file);

}

std::ostream& operator<<(std::ostream& os, const StateManager& sm)
{
  os << "RegScaf" << sm.optimize_scaffold <<'\t'
     << "PredRef" << sm.predict_reference <<std::endl;
  return os;
}
