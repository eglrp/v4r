/****************************************************************************
**
** Copyright (C) 2017 TU Wien, ACIN, Vision 4 Robotics (V4R) group
** Contact: v4r.acin.tuwien.ac.at
**
** This file is part of V4R
**
** V4R is distributed under dual licenses - GPLv3 or closed source.
**
** GNU General Public License Usage
** V4R is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** V4R is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** Please review the following information to ensure the GNU General Public
** License requirements will be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**
** Commercial License Usage
** If GPL is not suitable for your project, you must purchase a commercial
** license to use V4R. Licensees holding valid commercial V4R licenses may
** use this file in accordance with the commercial license agreement
** provided with the Software or, alternatively, in accordance with the
** terms contained in a written agreement between you and TU Wien, ACIN, V4R.
** For licensing terms and conditions please contact office<at>acin.tuwien.ac.at.
**
**
** The copyright holder additionally grants the author(s) of the file the right
** to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of their contributions without any restrictions.
**
****************************************************************************/

/**
 * @file incremental_object_learning.h
 * @author Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2016
 * @brief
 *
 */

#pragma once
#include <pcl/common/common.h>
#include <pcl/search/kdtree.h>
#include <pcl/search/octree.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <v4r/config.h>

#include <v4r/common/PointTypes.h>
#include <v4r/common/intrinsics.h>
#include <v4r/common/normals.h>
#include <v4r/core/macros.h>
#include <v4r/keypoints/ClusterNormalsToPlanes.h>
#include <v4r/object_modelling/model_view.h>
#include <v4r/registration/noise_model_based_cloud_integration.h>

#include <boost/filesystem.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace v4r {
namespace object_modelling {

struct CamConnect {
  Eigen::Matrix4f transformation_;
  float edge_weight;
  std::string model_name_;
  size_t source_id_, target_id_;

  explicit CamConnect(float w) : edge_weight(w) {}
  CamConnect() : edge_weight(0.f) {}
  bool operator<(const CamConnect &e) const {
    return edge_weight < e.edge_weight;
  }
  bool operator<=(const CamConnect &e) const {
    return edge_weight <= e.edge_weight;
  }
  bool operator>(const CamConnect &e) const {
    return edge_weight > e.edge_weight;
  }
  bool operator>=(const CamConnect &e) const {
    return edge_weight >= e.edge_weight;
  }
};

/**
 * @brief Dynamic object modelling over a sequence of point clouds. The seed is given by
 * the user by means of an initial object mask (indices of the point cloud belonging to the object).
 * The method projects the incrementally learnt object cloud to each view by a transformation given
 * from the camera pose and looks for nearest neighbors. After some filtering, these points are then
 * used for growing the object over the points in the current view. The final model is obtained
 * by a noise-model based cloud integration of all the points labelled as objects.
 *
 * @author Thomas Faeulhammer
 * @date July 2015
 * @ref Robotics and Automation Letters 2016, Faeulhammer et al
 */
class V4R_EXPORTS IOL {
 public:
  struct Parameter {
    double radius_;
    double eps_angle_;
    double dist_threshold_growing_;
    double voxel_resolution_;
    double seed_resolution_;
    double ratio_supervoxel_;
    double chop_z_;  ///< cut-off distance in meters
    bool do_erosion_;
    bool transfer_indices_from_latest_frame_only_;
    size_t min_points_for_transferring_;
    NormalEstimatorType normal_method_;
    bool do_mst_refinement_;
    double ratio_cluster_obj_supported_;
    double ratio_cluster_occluded_;
    Parameter()
    : radius_(0.005f), eps_angle_(0.95f), dist_threshold_growing_(0.05f), voxel_resolution_(0.005f),
      seed_resolution_(0.03f), ratio_supervoxel_(0.25f), chop_z_(std::numeric_limits<double>::quiet_NaN()),
      do_erosion_(true), transfer_indices_from_latest_frame_only_(false), min_points_for_transferring_(10),
      normal_method_(NormalEstimatorType::PCL_DEFAULT), do_mst_refinement_(true), ratio_cluster_obj_supported_(0.25),
      ratio_cluster_occluded_(0.75) {}

  } param_;

 protected:
  typedef pcl::PointXYZRGB PointT;

 public:
  struct {
    int meanK_ = 10;
    double std_mul_ = 2.0f;
  } sor_params_;

  v4r::ClusterNormalsToPlanes::Parameter p_param_;
  v4r::NMBasedCloudIntegrationParameter nm_int_param_;

 protected:
  typedef boost::property<boost::edge_weight_t, CamConnect> EdgeWeightProperty;
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, size_t, EdgeWeightProperty> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;
  typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;

  std::vector<pcl::PointCloud<PointT>::ConstPtr>
      keyframes_used_;  ///< all keyframes containing the object with sufficient number of points
  std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>>
      cameras_used_;  ///< camera pose belonging to the keyframes containing the object with sufficient number of points
  std::vector<std::vector<size_t>> object_indices_clouds_used_;  ///< indices of the object in all keyframes containing
                                                                 /// the object with sufficient number of points

  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_normals_oriented_;

  Graph gs_;
  Intrinsics cam_;

  pcl::PointCloud<PointT>::Ptr big_cloud_;
  pcl::PointCloud<PointT>::Ptr big_cloud_segmented_;
  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr big_cloud_segmented_refined_;
  pcl::visualization::PCLVisualizer::Ptr vis_, vis_reconstructed_, vis_seg_;
  std::vector<int> vis_reconstructed_viewpoint_;
  std::vector<int> vis_viewpoint_;

  std::vector<modelView> grph_;
  pcl::octree::OctreePointCloudSearch<PointT> octree_;

  void computeAbsolutePoses(const Graph &grph,
                            std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> &absolute_poses);

  void computeAbsolutePosesRecursive(
      const Graph &grph, const Vertex start, const Eigen::Matrix4f &accum,
      std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> &absolute_poses,
      std::vector<bool> &hop_list);

  /**
   * @brief checks if there sufficient points of the plane are labelled as object
   * @param Plane Model
   * @return true if plane has enough labelled object points
   */
  bool plane_has_object(const modelView::SuperPlane &sp) const {
    const size_t num_pts = sp.within_chop_z_indices.size();
    const size_t num_obj = sp.object_indices.size();

    return num_pts > 0 && static_cast<double>(num_obj) / num_pts > param_.ratio_cluster_obj_supported_;
  }

  /**
   * @brief checks if there sufficient points of the plane that are visible in the first view
   * @param Plane Model
   * @return true if plane is visibly enough in first view point
   */
  bool plane_is_visible(const modelView::SuperPlane &sp) const {
    const size_t num_pts = sp.within_chop_z_indices.size();
    const size_t num_vis = sp.visible_indices.size();

    return num_pts > 0 && static_cast<double>(num_vis) / num_pts > param_.ratio_cluster_occluded_;
  }

  bool plane_is_filtered(const modelView::SuperPlane &sp) const {
    return plane_is_visible(sp) && !plane_has_object(sp);
  }

  /**
   * @brief checks if two planes can be merged based on orientation
   * @param First plane model
   * @param Second plane model
   * @return true if planes are similar
   */
  bool merging_planes_reasonable(const modelView::SuperPlane &sp1, const modelView::SuperPlane &sp2) const;

  /**
   * @brief sets pixels in given mask belonging to invalid points to false
   * @param input cloud
   * @param mask
   */
  void remove_nan_points(const pcl::PointCloud<PointT> &cloud, boost::dynamic_bitset<> &mask) {
    assert(mask.size() == cloud.size());
    for (size_t i = 0; i < mask.size(); i++) {
      if (mask[i] && !pcl::isFinite(cloud.points[i]))
        mask.reset(i);
    }
  }

  /**
   * @brief given a point cloud and a normal cloud, this function computes points belonging to a table
   *  (optional: computes smooth clusters for points not belonging to table)
   * @param[in] cloud The input cloud from which smooth clusters / planes are being calculated
   * @param[in] normals The normal cloud corresponding to the input cloud
   * @param[out] planes The resulting smooth clusters and planes
   */
  void extractPlanePoints(const pcl::PointCloud<PointT>::ConstPtr &cloud,
                          const pcl::PointCloud<pcl::Normal>::ConstPtr &normals,
                          std::vector<v4r::ClusterNormalsToPlanes::Plane::Ptr> &planes);

  /**
   * @brief given a set of clusters, this function returns the clusters which have less than ratio% object points or
   * ratio_occ% occlusion points
   * Points further away than param_.chop_z_ are neglected
   * @param[in] planes - set of input clusters
   * @param[in] object_mask - binary mask of object pixels
   * @param[in] occlusion_mask - binary mask of pixels which are neither object nor background (e.g. pixels that are
   * occluded when transferred into the labelled frame)
   * @param[in] cloud - point cloud of the scene
   * @param[out] planes_not_on_object - output set of clusters
   * @param ratio - threshold percentage when a cluster is considered as belonging to an object
   * @param ratio_occ - threshold percentage when a cluster is considered as being occluded
   */
  void computePlaneProperties(const std::vector<v4r::ClusterNormalsToPlanes::Plane::Ptr> &planes,
                              const boost::dynamic_bitset<> &object_mask, const boost::dynamic_bitset<> &occlusion_mask,
                              const pcl::PointCloud<PointT>::ConstPtr &cloud,
                              std::vector<modelView::SuperPlane> &super_planes) const;

  /**
   * @brief transfers object points nearest neighbor search in dest frame.
   * @param[in] object_points points which are transferred and looked for in search cloud
   * @param[in] octree search space for transferred points
   * @param[out] obj_mask nearest neighbors points within a specified radius highlighted (true) in object mask
   */
  void nnSearch(const pcl::PointCloud<PointT> &object_points, pcl::octree::OctreePointCloudSearch<PointT> &octree,
                boost::dynamic_bitset<> &obj_mask);

  /**
   * @brief Nearest Neighbor Search for points transferred into search cloud
   * @param transferred object points
   * @param[in] point cloud to be checked for proximity to transferred points
   * @param[out] mask with nearest neighbors points within a specified radius highlighted (true) in object mask
   */
  void nnSearch(const pcl::PointCloud<PointT> &object_points, const pcl::PointCloud<PointT>::ConstPtr &search_cloud,
                boost::dynamic_bitset<> &obj_mask);

  /**
   * @brief extracts smooth Euclidean clusters of a given point cloud
   * @param input cloud
   * @param normals_
   * @param initial_mask
   * @param bg_mask
   * @return
   */
  boost::dynamic_bitset<> extractEuclideanClustersSmooth(const pcl::PointCloud<PointT>::ConstPtr &cloud,
                                                         const pcl::PointCloud<pcl::Normal> &normals_,
                                                         const boost::dynamic_bitset<> &initial_mask,
                                                         const boost::dynamic_bitset<> &bg_mask) const;

  void updatePointNormalsFromSuperVoxels(const pcl::PointCloud<PointT>::Ptr &cloud,
                                         pcl::PointCloud<pcl::Normal>::Ptr &normals_,
                                         const boost::dynamic_bitset<> &obj_mask,
                                         boost::dynamic_bitset<> &obj_mask_out);

  boost::dynamic_bitset<> erodeIndices(const boost::dynamic_bitset<> &obj_mask, const pcl::PointCloud<PointT> &cloud);

  typename v4r::NormalEstimator<PointT>::Ptr ne_;  //< Normal estimator

 public:
  IOL(const Parameter &p = Parameter()) : octree_(0.005f) {
    param_ = p;
    // Parameters for smooth clustering / plane segmentation
    p_param_.thrAngle = 45;
    p_param_.inlDist = 0.05;
    p_param_.minPoints = 5000;  // minimum number for a plane to be segmented
    p_param_.least_squares_refinement = true;
    p_param_.smooth_clustering = false;
    p_param_.thrAngleSmooth = 30;
    p_param_.inlDistSmooth = 0.02;
    p_param_.minPointsSmooth = 50;  // minimum number for a segment other than a plane

    nm_int_param_.min_points_per_voxel_ = 1;
    nm_int_param_.octree_resolution_ = 0.002f;
    nm_int_param_.average_ = false;

    cam_ = Intrinsics::PrimeSense();

    big_cloud_.reset(new pcl::PointCloud<PointT>);
    big_cloud_segmented_.reset(new pcl::PointCloud<PointT>);
    big_cloud_segmented_refined_.reset(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
    vis_.reset();
    vis_reconstructed_.reset();
    cloud_normals_oriented_.reset(new pcl::PointCloud<pcl::PointXYZRGBNormal>());

    std::vector<std::string> dummy_params = {};
    ne_ = v4r::initNormalEstimator<PointT>(param_.normal_method_, dummy_params);
  }

  /**
   * @brief saves the learned model to disk
   * @param[in] directory where to save the object model
   * @param[in] name of the object model
   * @return
   */
  bool save_model(const std::string &models_dir = "/tmp/dynamic_models/",
                  const std::string &model_name = "new_dynamic_model", bool save_individual_views = true);

  bool write_model_to_disk(const std::string &models_dir = "/tmp/dynamic_models/",
                           const std::string &model_name = "new_dynamic_model", bool save_individual_views = true);

  bool learn_object(const pcl::PointCloud<PointT> &cloud,
                    const Eigen::Matrix4f &camera_pose = Eigen::Matrix4f::Identity(),
                    const std::vector<size_t> &initial_indices = std::vector<size_t>());

  void initSIFT();

  /**
   * @brief clears the memory from the currently learned object.
   * Needs to be called before learning a new object model.
   */
  void clear() {
    big_cloud_->points.clear();
    big_cloud_segmented_->points.clear();
    big_cloud_segmented_refined_->points.clear();
    grph_.clear();
    gs_.clearing_graph();
    gs_.clear();
    vis_viewpoint_.clear();
    vis_reconstructed_viewpoint_.clear();
  }

  /**
   * @brief This shows the learned object together with all the intermediate steps using pcl viewer
   */
  void visualize();
  void visualize_clusters();

  /**
   * @brief This creates images of all intermediate steps of the object learning and writes them to disk
   * @param[in] path - folder where to write the files to
   */
  void writeImagesToDisk(const std::string &path = std::string("/tmp/dol_images/"));

  /**
   * @brief transforms each keyframe to global coordinate system using given camera
   *  pose and does segmentation based on the computed object indices to create
   *  reconstructed point cloud of scene and object model
   */
  void createBigCloud();

  /**
   * @brief prints parameters to the given output stream
   * @param output stream
   */
  void printParams(std::ostream &ostr = std::cout) const;
};
}  // namespace object_modelling
}  // namespace v4r
