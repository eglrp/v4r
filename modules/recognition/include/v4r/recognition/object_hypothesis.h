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
 * @file object_hypothesis.h
 * @author Aitor Aldoma (aldoma@acin.tuwien.ac.at), Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2017
 * @brief
 *
 */

#pragma once

#include <pcl/common/common.h>
#include <Eigen/Sparse>
#include <Eigen/StdVector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>

#include <v4r/common/pcl_serialization.h>
#include <v4r/core/macros.h>
#include <v4r/recognition/model.h>
#include <v4r/recognition/source.h>

namespace v4r {

class V4R_EXPORTS ObjectHypothesis {
 private:
  friend class boost::serialization::access;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  V4R_EXPORTS void serialize(Archive& ar, const unsigned int version) {
    (void)version;
    ar& class_id_& model_id_& transform_& pose_refinement_& confidence_& is_verified_& unique_id_;
  }

  static size_t s_counter_;  /// unique identifier to avoid transfering hypotheses multiple times when using multi-view
                             /// recognition

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  typedef boost::shared_ptr<ObjectHypothesis> Ptr;
  typedef boost::shared_ptr<ObjectHypothesis const> ConstPtr;

  pcl::Correspondences corr_;  ///< local feature matches / keypoint correspondences between model and scene (only for
                               /// visualization purposes)

  ObjectHypothesis()
  : class_id_(""), model_id_(""), pose_refinement_(Eigen::Matrix4f::Identity()), is_verified_(false),
    unique_id_(s_counter_++) {}

  ObjectHypothesis(const ObjectHypothesis& other)
  : class_id_(other.class_id_), model_id_(other.model_id_), transform_(other.transform_),
    pose_refinement_(other.pose_refinement_), is_verified_(other.is_verified_), unique_id_(other.unique_id_) {}

  std::string class_id_;             ///< category
  std::string model_id_;             ///< instance
  Eigen::Matrix4f transform_;        ///< 4x4 homogenous transformation to project model into camera coordinate system.
  Eigen::Matrix4f pose_refinement_;  ///< pose refinement (to be multiplied by transform to get refined pose)
  float confidence_;                 ///< confidence score (coming from feature matching stage)
  bool is_verified_;
  size_t unique_id_;

  virtual ~ObjectHypothesis() {}
};

class V4R_EXPORTS ModelSceneCorrespondence {
 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int file_version) {
    ar& scene_id_& model_id_& dist_3D_& color_distance_& normals_dotp_& fitness_;
  }

 public:
  int scene_id_;          /// Index of scene point.
  int model_id_;          /// Index of matching model point.
  float dist_3D_;         /// Squared distance between the corresponding points in Euclidean 3D space
  float color_distance_;  /// Distance between the corresponding points in color
  float normals_dotp_;    /// Angle in degree between surface normals
  float fitness_;         /// model fitness score

  bool operator<(const ModelSceneCorrespondence& other) const {
    return this->fitness_ > other.fitness_;
  }

  /** \brief Constructor. */
  ModelSceneCorrespondence()
  : scene_id_(-1), model_id_(-1), dist_3D_(std::numeric_limits<float>::quiet_NaN()),
    color_distance_(std::numeric_limits<float>::quiet_NaN()), normals_dotp_(M_PI / 2), fitness_(0.f) {}
};

template <typename PointT>
class V4R_EXPORTS HVRecognitionModel {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  typedef boost::shared_ptr<HVRecognitionModel> Ptr;
  typedef boost::shared_ptr<HVRecognitionModel const> ConstPtr;

  typename ObjectHypothesis::Ptr oh_;  ///< object hypothesis
                                       //    typename pcl::PointCloud<PointT>::Ptr complete_cloud_;
  size_t num_pts_full_model_;
  typename pcl::PointCloud<PointT>::Ptr visible_cloud_;
  pcl::PointCloud<pcl::Normal>::Ptr visible_cloud_normals_;
  std::vector<boost::dynamic_bitset<>> image_mask_;  ///< image mask per view (in single-view case, there will be only
                                                     /// one element in outer vector). Used to compute pairwise
  /// intersection
  //    pcl::PointCloud<pcl::Normal>::Ptr complete_cloud_normals_;
  std::vector<int> visible_indices_;  ///< visible indices computed by z-Buffering (for model self-occlusion) and
                                      /// occlusion reasoning with scene cloud
  std::vector<int> visible_indices_by_octree_;  ///< visible indices computed by creating an octree for the model and
                                                /// checking which leaf nodes are occupied by a visible point computed
  /// from the z-buffering approach
  std::vector<ModelSceneCorrespondence> model_scene_c_;  ///< correspondences between visible model points and scene
  float model_fit_;  ///< the fitness score of the visible cloud to the model scene (sum of model_scene_c correspondenes
                     /// weight divided by the number of visible points)
  boost::dynamic_bitset<> visible_pt_is_outlier_;  ///< indicates for each visible point if it is considered an outlier

  Eigen::MatrixXf pt_color_;  ///< color values for each point of the (complete) model (row_id). Width is equal to the
                              /// number of color channels
  float mean_brigthness_;     ///< average value of the L channel for all visible model points
  float mean_brigthness_scene_;  ///< average value of the L channel for all scene points close to the visible model
                                 /// points
  std::vector<int> scene_indices_in_crop_box_;  ///< indices of the scene that are occupied from the bounding box of the
                                                ///(complete) hypothesis
  float L_value_offset_;  ///< the offset being added to the computed L color values to compensate for different
                          /// lighting conditions

  Eigen::SparseVector<float>
      scene_explained_weight_;  ///< stores for each scene point how well it is explained by the visible model points

  bool rejected_due_to_low_visibility_;  ///< true if the object model rendered in the view is not visible enough
  bool is_outlier_;                      ///< true if the object model is not able to explain the scene well enough
  bool rejected_due_to_better_hypothesis_in_group_;  ///< true if there is any other object model in the same hypotheses
                                                     /// group which explains the scene better
  bool rejected_globally_;

  HVRecognitionModel(typename ObjectHypothesis::Ptr& oh)
  : num_pts_full_model_(0), L_value_offset_(0.f), rejected_due_to_low_visibility_(false), is_outlier_(false),
    rejected_due_to_better_hypothesis_in_group_(false), rejected_globally_(false) {
    oh_ = oh;
  }

  void freeSpace() {
    visible_cloud_.reset();
    visible_cloud_normals_.reset();
    visible_indices_.clear();
    image_mask_.clear();
    model_scene_c_.clear();
    pt_color_.resize(0, 0);
    scene_indices_in_crop_box_.clear();
  }

  bool isRejected() const {
    return is_outlier_ || rejected_due_to_low_visibility_ || rejected_globally_ ||
           rejected_due_to_better_hypothesis_in_group_;
  }

  /**
       * @brief does dilation and erosion on the occupancy image of the rendered point cloud
       * @param do_smoothing
       * @param smoothing_radius
       * @param do_erosion
       * @param erosion_radius
       * @param img_width
       */
  void processSilhouette(bool do_smoothing = true, int smoothing_radius = 2, bool do_erosion = true,
                         int erosion_radius = 4, int img_width = 640);

  static bool modelFitCompare(typename HVRecognitionModel<PointT>::Ptr const& a,
                              typename HVRecognitionModel<PointT>::Ptr const& b) {
    return a->model_fit_ > b->model_fit_;
  }
};

class V4R_EXPORTS ObjectHypothesesGroup {
 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    (void)version;
    ar& global_hypotheses_& ohs_;
    //        size_t nItems = ohs_.size();
    //        ar & nItems;
    //        for (const auto &oh : ohs_) {  ar & *oh; }
  }

 public:
  typedef boost::shared_ptr<ObjectHypothesesGroup> Ptr;
  typedef boost::shared_ptr<ObjectHypothesesGroup const> ConstPtr;

  ObjectHypothesesGroup() {}
  //    ObjectHypothesesGroup(const std::string &filename, const Source<PointT> &src);
  //    void save(const std::string &filename) const;

  std::vector<typename ObjectHypothesis::Ptr> ohs_;  ///< Each hypothesis can have several object model (e.g. global
                                                     /// recognizer tries to macht several object instances for a
  /// clustered point cloud segment).
  bool global_hypotheses_;  ///< if true, hypothesis was generated by global recognition pipeline. Otherwise, from local
                            /// feature matches-
};
}
