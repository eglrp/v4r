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
 * @file main.cpp
 * @author Johann Prankl (prankl@acin.tuwien.ac.at)
 * @date 2017
 * @brief
 *
 */

#ifndef KP_TSF_MAPPING_HH
#define KP_TSF_MAPPING_HH

#include <float.h>
#include <pcl/io/io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <v4r/camera_tracking_and_mapping/TSFData.h>
#include <v4r/core/macros.h>
#include <v4r/features/FeatureDetector.h>
#include <v4r/keypoints/RigidTransformationRANSAC.h>
#include <v4r/recognition/RansacSolvePnPdepth.h>
#include <v4r/reconstruction/RefineProjectedPointLocationLK.h>
#include <Eigen/Dense>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <v4r/camera_tracking_and_mapping/TSFFrame.hh>
#include <v4r/camera_tracking_and_mapping/TSFOptimizeBundle.hh>
#include <v4r/common/impl/DataMatrix2D.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/tracking.hpp"

namespace v4r {

/**
 * TSFMapping
 */
class V4R_EXPORTS TSFMapping {
 public:
  /**
   * Parameter
   */
  class Parameter {
   public:
    cv::Size win_size;
    int max_level;
    cv::TermCriteria termcrit;
    float max_error;  // 100
    int max_count;
    double max_dev_vr_normal;
    double depth_error_scale;
    double max_delta_angle_loop;
    double max_cam_dist_loop;
    double max_delta_angle_eq_pose;
    double max_cam_dist_eq_pose;
    double nnr;
    bool refine_plk;
    bool detect_loops;
    int nb_tracked_frames;
    double inl_dist;
    RefineProjectedPointLocationLK::Parameter plk_param;
    v4r::RansacSolvePnPdepth::Parameter pnp;
    TSFOptimizeBundle::Parameter ba;
    Parameter()
    : win_size(cv::Size(21, 21)), max_level(2),
      termcrit(cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03)), max_error(100), max_count(500),
      max_dev_vr_normal(75), max_delta_angle_loop(30), max_cam_dist_loop(1.5), max_delta_angle_eq_pose(5),
      max_cam_dist_eq_pose(0.1), nnr(0.95), refine_plk(false), detect_loops(true), nb_tracked_frames(2),
      inl_dist(0.015),
      plk_param(RefineProjectedPointLocationLK::Parameter(5., 0.01, 0.1, 10, 15., 0.3, true, cv::Size(21, 21))),
      pnp(v4r::RansacSolvePnPdepth::Parameter(1.5, 0.01, 2000, INT_MIN, 4, 0.015)) {}
  };

 private:
  Parameter param;

  double cos_rad_max_dev_vr_normal;

  cv::Mat_<double> intrinsic;
  Eigen::Matrix3f C;

  bool run, have_thread;

  boost::thread th_obectmanagement;
  boost::thread th_init;

  TSFData *data;

  std::vector<TSFFrame::Ptr> map_frames;

  cv::Mat_<cv::Vec3b> image0, image;
  cv::Mat_<unsigned char> im_gray0, im_gray1, im_gray2;
  cv::Mat_<unsigned char> im_warped;

  std::vector<cv::Point2f> cv_points, cv_points1;
  std::vector<int> converged;
  std::vector<std::vector<cv::DMatch>> matches;

  v4r::FeatureDetector::Ptr detector;
  v4r::FeatureDetector::Ptr descEstimator;
  RefineProjectedPointLocationLK plk;
  v4r::RansacSolvePnPdepth::Ptr pnp;
  cv::FlannBasedMatcher matcher;

  TSFOptimizeBundle ba;

  void operate();

  void initKeypoints(const cv::Mat_<unsigned char> &im, TSFFrame &frame0);
  void addFeatureLinks(TSFFrame &frame0, TSFFrame &frame1, const cv::Mat_<unsigned char> &im0,
                       const cv::Mat_<unsigned char> &im1, const Eigen::Matrix4f &pose0, const Eigen::Matrix4f &pose1,
                       bool is_loop);
  void filterValidPoints3D(std::vector<cv::Point2f> &points, std::vector<Eigen::Vector3f> &points3d,
                           std::vector<Eigen::Vector3f> &normals);
  void getPoints3D(const v4r::DataMatrix2D<Surfel> &cloud, const std::vector<cv::Point2f> &points,
                   std::vector<Eigen::Vector3f> &points3d, std::vector<Eigen::Vector3f> &normals);
  void getKeys3D(const v4r::DataMatrix2D<Surfel> &cloud, const std::vector<cv::KeyPoint> &keys,
                 std::vector<Eigen::Vector3f> &keys3d);
  void filterValidKeys3D(std::vector<cv::KeyPoint> &keys, std::vector<Eigen::Vector3f> &keys3d);
  int addProjectionsPLK(const std::vector<Eigen::Vector3f> &points3d, int frame_idx,
                        const v4r::DataMatrix2D<Surfel> &sf_cloud, const Eigen::Matrix4f &inc_pose,
                        const std::vector<cv::Point2f> &cv_points, const std::vector<int> &converged,
                        std::vector<std::vector<v4r::triple<int, cv::Point2f, Eigen::Vector3f>>> &projs);
  void addLoops();
  bool refineLK(const TSFFrame &frame0, const TSFFrame &frame1, const cv::Mat_<unsigned char> &im0,
                const cv::Mat_<unsigned char> &im1, Eigen::Matrix4f &pose01, std::vector<cv::Point2f> &refined1,
                std::vector<int> &converged1);
  bool ransacPose(const std::vector<Eigen::Vector3f> &query, const std::vector<cv::KeyPoint> &query_keys,
                  const std::vector<Eigen::Vector3f> &train, const std::vector<std::vector<cv::DMatch>> &matches,
                  Eigen::Matrix4f &pose, int &nb_inls);
  void warpImage(const TSFFrame &frame1, const cv::Mat_<unsigned char> &im0, const Eigen::Matrix4f &pose,
                 cv::Mat_<unsigned char> &im_warped, Eigen::Matrix3f &H);

  inline void mapPoint(const cv::Point2f &pt_in, const Eigen::Matrix3f &H, cv::Point2f &pt_out);

 public:
  cv::Mat dbg;

  TSFMapping(const Parameter &p = Parameter());
  ~TSFMapping();

  void start();
  void stop();

  inline bool isStarted() {
    return have_thread;
  }

  inline void lock() {
    data->lock();
  }  // threaded object management, so we need to lock
  inline void unlock() {
    data->unlock();
  }

  void reset();
  void setData(TSFData *_data) {
    data = _data;
  }

  void setCameraParameter(const cv::Mat &_intrinsic);
  void setParameter(const Parameter &p);
  void setDetectors(const FeatureDetector::Ptr &_detector, const FeatureDetector::Ptr &_descEstimator);

  void optimizeMap();
  void getCameraParameter(cv::Mat &_intrinsic, cv::Mat &_dist_coeffs) const {
    ba.getCameraParameter(_intrinsic, _dist_coeffs);
  }

  void transformMap(const Eigen::Matrix4f &transform);

  inline const std::vector<std::vector<Eigen::Vector3d>> &getOptiPoints() const {
    return ba.getOptiPoints();
  }
  inline const std::vector<TSFFrame::Ptr> &getMap() const {
    return map_frames;
  }

  typedef std::shared_ptr<::v4r::TSFMapping> Ptr;
  typedef std::shared_ptr<::v4r::TSFMapping const> ConstPtr;
};

/*************************** INLINE METHODES **************************/

inline void TSFMapping::mapPoint(const cv::Point2f &pt_in, const Eigen::Matrix3f &H, cv::Point2f &pt_out) {
  pt_out.x = H(0, 0) * pt_in.x + H(0, 1) * pt_in.y + H(0, 2);
  pt_out.y = H(1, 0) * pt_in.x + H(1, 1) * pt_in.y + H(1, 2);
  float t = H(2, 0) * pt_in.x + H(2, 1) * pt_in.y + H(2, 2);
  pt_out.x /= t;
  pt_out.y /= t;
}

}  // namespace v4r

#endif
