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
 * @file hypotheses_verification.h
 * @author Aitor Aldoma (aldoma@acin.tuwien.ac.at), Federico Tombari, Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2017
 * @brief
 *
 */

#pragma once

#include <v4r/common/color_comparison.h>
#include <v4r/common/intrinsics.h>
#include <v4r/common/normals.h>
#include <v4r/common/rgb2cielab.h>
#include <v4r/core/macros.h>
#include <v4r/recognition/hypotheses_verification_param.h>
#include <v4r/recognition/hypotheses_verification_visualization.h>
#include <v4r/recognition/object_hypothesis.h>

#include <pcl/common/angles.h>
#include <pcl/common/common.h>
#include <pcl/common/transforms.h>
#include <pcl/octree/octree.h>
#include <pcl/search/kdtree.h>

#include <pcl/octree/octree_pointcloud_pointvector.h>
#include <pcl/octree/impl/octree_iterator.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/map.hpp>

namespace v4r {

// forward declarations
template <typename PointT>
class HV_ModelVisualizer;
template <typename PointT>
class HV_CuesVisualizer;
template <typename PointT>
class HV_PairwiseVisualizer;

class PtFitness {
 public:
  float fit_;
  size_t rm_id_;

  bool operator<(const PtFitness &other) const {
    return this->fit_ < other.fit_;
  }

  PtFitness(float fit, size_t rm_id) : fit_(fit), rm_id_(rm_id) {}
};

/**
 * \brief A hypothesis verification method for 3D Object Instance Recognition
 * \author Thomas Faeulhammer (based on the work of Federico Tombari and Aitor Aldoma)
 * \date April, 2016
 */
template <typename PointT>
class V4R_EXPORTS HypothesisVerification {
  friend class HV_ModelVisualizer<PointT>;
  friend class HV_CuesVisualizer<PointT>;
  friend class HV_PairwiseVisualizer<PointT>;

  HV_Parameter param_;

 public:
  typedef std::shared_ptr<HypothesisVerification<PointT>> Ptr;
  typedef std::shared_ptr<HypothesisVerification<PointT> const> ConstPtr;

 protected:
  typedef boost::mpl::map<boost::mpl::pair<pcl::PointXYZ, pcl::PointNormal>,
                          boost::mpl::pair<pcl::PointNormal, pcl::PointNormal>,
                          boost::mpl::pair<pcl::PointXYZRGB, pcl::PointXYZRGBNormal>,
                          boost::mpl::pair<pcl::PointXYZRGBA, pcl::PointXYZRGBNormal>,
                          boost::mpl::pair<pcl::PointXYZRGBNormal, pcl::PointXYZRGBNormal>,
                          boost::mpl::pair<pcl::PointXYZI, pcl::PointXYZINormal>,
                          boost::mpl::pair<pcl::PointXYZINormal, pcl::PointXYZINormal>>
      PointTypeAssociations;
  BOOST_MPL_ASSERT((boost::mpl::has_key<PointTypeAssociations, PointT>));

  typedef typename boost::mpl::at<PointTypeAssociations, PointT>::type PointTWithNormal;

  mutable typename HV_ModelVisualizer<PointT>::Ptr vis_model_;
  mutable typename HV_CuesVisualizer<PointT>::Ptr vis_cues_;
  mutable typename HV_PairwiseVisualizer<PointT>::Ptr vis_pairwise_;

  Intrinsics cam_;  ///< rgb camera intrinsics. Used for projection of the object hypotheses onto the image plane.
  cv::Mat_<uchar> rgb_depth_overlap_;  ///< this mask has the same size as the camera image and tells us which pixels
                                       ///< can have valid depth pixels and which ones are not seen due to the physical
                                       ///< displacement between RGB and depth sensor. Valid pixels are set to 255,
                                       ///< pixels that are outside depth camera's field of view are set to 0
  ColorTransform::Ptr colorTransf_;

  bool visualize_pairwise_cues_;  ///< visualizes the pairwise cues. Useful for debugging

  typename Source<PointT>::ConstPtr m_db_;  ///< model data base

  typename pcl::PointCloud<PointT>::ConstPtr scene_cloud_;         ///< scene point clou
  typename pcl::PointCloud<pcl::Normal>::ConstPtr scene_normals_;  ///< scene normals cloud
  typename pcl::PointCloud<PointT>::Ptr scene_cloud_downsampled_;  ///< Downsampled scene point cloud
  pcl::PointCloud<pcl::Normal>::Ptr scene_normals_downsampled_;    ///< Downsampled scene normals cloud
  std::vector<int> scene_sampled_indices_;                         ///< downsampled indices of the scene
  Eigen::VectorXi scene_indices_map_;  ///< saves relationship between indices of the input cloud and indices of the
                                       ///< downsampled input cloud

  std::vector<std::vector<typename HVRecognitionModel<PointT>::Ptr>> obj_hypotheses_groups_;
  std::vector<typename HVRecognitionModel<PointT>::Ptr>
      global_hypotheses_;  ///< all hypotheses not rejected by individual verification

  std::map<std::string, std::shared_ptr<pcl::octree::OctreePointCloudPointVector<PointT>>>
      octree_model_representation_;  ///< for each model we create an octree representation (used for computing visible
                                     /// points)
  typename pcl::search::KdTree<PointT>::Ptr kdtree_scene_;
  typename v4r::NormalEstimator<PointT>::Ptr
      model_normal_estimator_;  ///< normal estimator for object models (only used
                                /// in case not already computed in advance)

  float Lmin_ = 0.f, Lmax_ = 100.f;
  int bins_ = 50;

  Eigen::MatrixXf intersection_cost_;      ///< represents the pairwise intersection cost
  Eigen::MatrixXi smooth_region_overlap_;  ///< represents if two hypotheses explain the same smooth region
  // cached variables for speed-up
  float eps_angle_threshold_rad_;

  std::vector<std::vector<PtFitness>> scene_pts_explained_solution_;

  static std::vector<std::pair<std::string, float>>
      elapsed_time_;  ///< measurements of computation times for various components

  struct Solution {
    boost::dynamic_bitset<> solution_;
    double cost_;
  } best_solution_;  ///< costs for each possible solution

  std::set<size_t> evaluated_solutions_;
  void search();

  Eigen::MatrixXf scene_color_channels_;  ///< converted color values where each point corresponds to a row entry
  typename pcl::octree::OctreePointCloudSearch<PointT>::Ptr octree_scene_downsampled_;
  boost::function<void(const boost::dynamic_bitset<> &, double, size_t)> visualize_cues_during_logger_;

  Eigen::VectorXi scene_pt_smooth_label_id_;  ///< stores a label for each point of the (downsampled) scene. Points
                                              /// belonging to the same smooth clusters, have the same label
  int max_smooth_label_id_;

  size_t num_evaluations_;  ///< counts how many times it evaluates a solution until finished verification

  // ----- MULTI-VIEW VARIABLES------
  std::vector<typename pcl::PointCloud<PointT>::ConstPtr>
      occlusion_clouds_;  ///< scene clouds from multiple views (stored as organized point clouds)
  std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> absolute_camera_poses_;
  std::vector<boost::dynamic_bitset<>> model_is_present_in_view_;  ///< for each model this variable stores information
                                                                   /// in which view it is present (used to check for
  /// visible model points - default all true = static
  /// scene)

  boost::function<float(const Eigen::VectorXf &, const Eigen::VectorXf &)> color_dist_f_;

  void refinePose(HVRecognitionModel<PointT> &rm) const;

  cv::Mat img_boundary_distance_;  ///< saves for each pixel how far it is away from the boundary (taking into account
                                   /// extrinsics of the camera)

  /**
   * @brief computeVisiblePoints first renders the model cloud in the given pose onto the image plane and checks via
   * z-buffering
   * for each model point if it is visible or self-occluded. The visible model cloud is then compared to the scene cloud
   * for occlusion
   * caused by the input scene
   * @param rm recongition model
   */
  void computeModelOcclusionByScene(HVRecognitionModel<PointT> &rm)
      const;  ///< computes the visible points of the model in the given pose and the provided depth map(s) of the scene

  /**
   * @brief computeVisibleOctreeNodes the visible point computation so far misses out points that are not visible just
   * because of the discretization from the image back-projection
   * for each pixel only one point falling on that pixel can be said to be visible with the approach so far.
   * Particularly in high-resolution models it is however possible that multiple points project on the same pixel and
   * they should be also visible.
   * if there is at least one visible point computed from the approach above. If so, the leaf node is counted visible
   * and all its containing points
   * are also set as visible.
   * @param rm recognition model
   */
  void computeVisibleOctreeNodes(HVRecognitionModel<PointT> &rm);

  void downsampleSceneCloud();  ///< downsamples the scene cloud

  void removeSceneNans();  ///< removes nan values from the downsampled scene cloud

  bool removeNanNormals(HVRecognitionModel<PointT> &recog_model)
      const;  ///< remove all points from visible cloud and normals which are not-a-number

  void removeModelsWithLowVisibility();  ///< remove recognition models where there are not enough visible points (>95%
                                         /// occluded)

  void computePairwiseIntersection();  ///< computes the overlap of two visible points when projected to camera view

  void computeSmoothRegionOverlap();  ///< computes if two hypotheses explain the same smooth region

  void computeModelFitness(HVRecognitionModel<PointT> &rm) const;

  void initialize();

  double evaluateSolution(const boost::dynamic_bitset<> &solution, bool &violates_smooth_region_check);

  void optimize();

  /**
   * @brief check if provided input is okay
   */
  void checkInput();

  /**
   * @brief this iterates through all poses and rejects hypotheses that are very similar.
   * Similar means that they describe the same object identity, their centroids align and the rotation is (almost) the
   * same.
   */
  void removeRedundantPoses();

  /**
   * @brief isOutlier remove hypotheses with a lot of outliers. Returns true if hypothesis is rejected.
   * @param rm
   * @return
   */
  bool isOutlier(const HVRecognitionModel<PointT> &rm) const;

  void cleanUp() {
    octree_scene_downsampled_.reset();
    occlusion_clouds_.clear();
    absolute_camera_poses_.clear();
    scene_sampled_indices_.clear();
    model_is_present_in_view_.clear();
    scene_cloud_downsampled_.reset();
    scene_cloud_.reset();
    intersection_cost_.resize(0, 0);
    obj_hypotheses_groups_.clear();
    scene_pt_smooth_label_id_.resize(0);
    scene_color_channels_.resize(0, 0);
    scene_pts_explained_solution_.clear();
    kdtree_scene_.reset();
  }

  /**
   * @brief extractEuclideanClustersSmooth
   */
  void extractEuclideanClustersSmooth();

  /**
   * @brief getFitness
   * @param c
   * @return
   */
  float getFitness(const ModelSceneCorrespondence &c) const {
    float fit_3d = scoreXYZNormalized(c);
    float fit_normal = modelSceneNormalsCostTerm(c);

    if (param_.ignore_color_even_if_exists_) {
      if (fit_3d < std::numeric_limits<float>::epsilon() || fit_normal < std::numeric_limits<float>::epsilon())
        return 0.f;

      float sum_weights = param_.w_xyz_ + param_.w_normals_;

      return expf((param_.w_xyz_ * log(fit_3d) + param_.w_normals_ * log(fit_normal)) /
                  sum_weights);  // weighted geometric means
    } else {
      float fit_color = scoreColorNormalized(c);

      if (fit_3d < std::numeric_limits<float>::epsilon() || fit_color < std::numeric_limits<float>::epsilon() ||
          fit_normal < std::numeric_limits<float>::epsilon())
        return 0.f;

      float sum_weights = param_.w_xyz_ + param_.w_color_ + param_.w_normals_;
      return expf(
          (param_.w_xyz_ * log(fit_3d) + param_.w_color_ * log(fit_color) + param_.w_normals_ * log(fit_normal)) /
          sum_weights);  // weighted geometric mean
    }
  }

  inline float scoreColor(float dist_color) const {
    return (1.f - tanhf((dist_color - param_.inlier_threshold_color_) / param_.sigma_color_));
  }

  inline float scoreXYZ(float dist_xyz) const {
    return (1.f - tanhf((dist_xyz - param_.inlier_threshold_xyz_) / param_.sigma_xyz_));
  }

  /**
   * @brief modelSceneColorCostTerm
   * @param model scene correspondence
   * @return
   */
  float scoreColorNormalized(const ModelSceneCorrespondence &c) const {
    //        return exp (- c.color_distance_/param_.color_sigma_ab_ );
    //        std::cout << c.color_distance_ << std::endl;
    //        return std::min(1.f, std::max(0.f, 1.f - c.color_distance_/param_.color_sigma_ab_));
    return scoreColor(c.color_distance_) * OneOver_distColor0_;
  }

  /**
   * @brief modelScene3DDistCostTerm
   * @param model scene correspondence
   * @return distance in centimeter
   */
  float scoreXYZNormalized(const ModelSceneCorrespondence &c) const {
    //        if ( c.dist_3D_ < param_.inliers_threshold_ )
    //            return 1.f;
    //        else
    return scoreXYZ(c.dist_3D_) * OneOver_distXYZ0_;
  }

  inline float scoreNormals(float dotp) const {
    return (1.f +
            tanh((dotp - param_.inlier_threshold_normals_dotp_) / param_.sigma_normals_));  /// TODO: Speed up with LUT
  }

  /**
   * @brief modelSceneNormalsCostTerm
   * @param model scene correspondence
   * @return angle between corresponding surface normals (fliped such that they are pointing into the same direction)
   */
  float modelSceneNormalsCostTerm(const ModelSceneCorrespondence &c) const {
    return scoreNormals(c.normals_dotp_) * OneOver_distNorm0_;
  }

  //    void
  //    computeLOffset( HVRecognitionModel<PointT> &rm ) const;

  float customColorDistance(const Eigen::VectorXf &color_a, const Eigen::VectorXf &color_b);

  /**
   * @brief customRegionGrowing constraint function which decides if two points are to be merged as one "smooth" cluster
   * @param seed_pt
   * @param candidate_pt
   * @param squared_distance
   * @return
   */
  bool customRegionGrowing(const PointTWithNormal &seed_pt, const PointTWithNormal &candidate_pt,
                           float squared_distance) const;

  // pre-computed variables for speed-up
  float OneOver_distNorm0_;
  float OneOver_distColor0_;
  float OneOver_distXYZ0_;
  float search_radius_;

 public:
  HypothesisVerification(const Intrinsics &cam, const HV_Parameter &p = HV_Parameter())
  : param_(p), cam_(cam), OneOver_distNorm0_(1.f / scoreNormals(1.f)), OneOver_distColor0_(1.f / scoreColor(0.f)),
    OneOver_distXYZ0_(1.f / scoreXYZ(0.f)), search_radius_(2. * param_.resolution_mm_ / 1000.) {
    max_smooth_label_id_ = 0;
    colorTransf_.reset(new RGB2CIELAB);

    switch (param_.color_comparison_method_) {
      case ColorComparisonMethod::CIE76:
        color_dist_f_ = computeCIE76;
        break;

      case ColorComparisonMethod::CIE94:
        color_dist_f_ = computeCIE94_DEFAULT;
        break;

      case ColorComparisonMethod::CIEDE2000:
        color_dist_f_ = computeCIEDE2000;
        break;

      case ColorComparisonMethod::CUSTOM:
        color_dist_f_ = boost::bind(&HypothesisVerification::customColorDistance, this, _1, _2);
        break;

      default:
        throw std::runtime_error("Color comparison method not defined!");
    }

    eps_angle_threshold_rad_ = pcl::deg2rad(param_.eps_angle_threshold_deg_);

    std::vector<std::string> dummy;
    model_normal_estimator_ = v4r::initNormalEstimator<PointT>(param_.normal_method_, dummy);
  }

  /**
   *  \brief Sets the scene cloud
   *  \param scene_cloud Point cloud representing the scene
   */
  void setSceneCloud(const typename pcl::PointCloud<PointT>::ConstPtr &scene_cloud) {
    scene_cloud_ = scene_cloud;
  }

  /**
   *  \brief Sets the scene normals cloud
   *  \param normals Point cloud representing the scene normals
   */
  void setNormals(const pcl::PointCloud<pcl::Normal>::ConstPtr &normals) {
    scene_normals_ = normals;
  }

  /**
   * @brief set Occlusion Clouds And Absolute Camera Poses (used for multi-view recognition)
   * @param occlusion clouds
   * @param absolute camera poses
   */
  void setOcclusionCloudsAndAbsoluteCameraPoses(
      const std::vector<typename pcl::PointCloud<PointT>::ConstPtr> &occ_clouds,
      const std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> &absolute_camera_poses) {
    for (size_t i = 0; i < occ_clouds.size(); i++)
      CHECK(occ_clouds[i]->isOrganized()) << "Occlusion clouds need to be organized!";

    occlusion_clouds_ = occ_clouds;
    absolute_camera_poses_ = absolute_camera_poses;
  }

  /**
   * @brief for each model this variable stores information in which view it is present
   * @param presence in model and view
   */
  void setVisibleCloudsForModels(const std::vector<boost::dynamic_bitset<>> &model_is_present_in_view) {
    model_is_present_in_view_ = model_is_present_in_view;
  }

  /**
   * @brief setHypotheses
   * @param ohs
   */
  void setHypotheses(std::vector<ObjectHypothesesGroup> &ohs);

  /**
   * @brief setModelDatabase
   * @param m_db model database
   */
  void setModelDatabase(const typename Source<PointT>::ConstPtr &m_db) {
    m_db_ = m_db;
  }

  void setRGBDepthOverlap(const cv::Mat_<uchar> &rgb_depth_overlap) {
    rgb_depth_overlap_ = rgb_depth_overlap;
  }

  /**
   * @brief visualizeModelCues visualizes the model cues during computation. Useful for debugging
   * @param vis_params visualization parameters
   */
  void visualizeModelCues(
      const PCLVisualizationParams::ConstPtr &vis_params = std::make_shared<PCLVisualizationParams>()) {
    vis_model_.reset(new HV_ModelVisualizer<PointT>(vis_params));
  }

  /**
   * @brief visualizeCues visualizes the cues during the computation and shows cost and number of evaluations. Useful
   * for debugging
   * @param vis_params visualization parameters
   */
  void visualizeCues(const PCLVisualizationParams::ConstPtr &vis_params = std::make_shared<PCLVisualizationParams>()) {
    vis_cues_.reset(new HV_CuesVisualizer<PointT>(vis_params));
  }

  /**
   * @brief visualizePairwiseCues visualizes the pairwise intersection of two hypotheses during computation. Useful for
   * debugging
   * @param vis_params visualization parameters
   */
  void visualizePairwiseCues(
      const PCLVisualizationParams::ConstPtr &vis_params = std::make_shared<PCLVisualizationParams>()) {
    vis_pairwise_.reset(new HV_PairwiseVisualizer<PointT>(vis_params));
  }

  /**
   *  \brief Function that performs the hypotheses verification
   *  This function modifies the values of mask_ and needs to be called after both scene and model have been added
   */
  void verify();

  /**
   * @brief getElapsedTimes
   * @return compuation time measurements for various components
   */
  std::vector<std::pair<std::string, float>> getElapsedTimes() const {
    return elapsed_time_;
  }

  /**
   * @brief setCamera set the camera used for z-buffering
   * @param cam camera parameters
   */
  void setCameraIntrinsics(const Intrinsics &cam) {
    cam_ = cam;
  }
};
}  // namespace v4r
