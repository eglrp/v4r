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
 * @file visibility_reasoning.h
 * @author Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at), Aitor Aldoma (aldoma@acin.tuwien.ac.at)
 * @date 2013
 * @brief
 *
 */

#pragma once

#include <pcl/common/common.h>
#include <v4r/common/intrinsics.h>
#include <v4r/core/macros.h>

namespace v4r {

/**
 * visibility_reasoning.h
 *
 *  @date Mar 19, 2013
 *  @author Aitor Aldoma, Thomas Faeulhammer
 */
template <typename PointT>
class V4R_EXPORTS VisibilityReasoning {
  typedef typename pcl::PointCloud<PointT>::Ptr PointCloudPtr;
  Intrinsics cam_;
  float tss_;
  int fsv_used_;

 public:
  VisibilityReasoning(const Intrinsics &cam) {
    cam_ = cam;
    tss_ = 0.01f;
    fsv_used_ = 0;
  }

  int getFSVUsedPoints() {
    return fsv_used_;
  }

  int computeRangeDifferencesWhereObserved(const typename pcl::PointCloud<PointT>::ConstPtr &im1,
                                           const typename pcl::PointCloud<PointT>::ConstPtr &im2,
                                           std::vector<float> &range_diff);

  int computeRangeDifferencesWhereObservedWithIndicesBack(const typename pcl::PointCloud<PointT>::ConstPtr &im1,
                                                          const typename pcl::PointCloud<PointT>::ConstPtr &im2,
                                                          std::vector<float> &range_diff, std::vector<int> &indices);

  float computeFSV(const typename pcl::PointCloud<PointT>::ConstPtr &im1,
                   const typename pcl::PointCloud<PointT>::ConstPtr &im2,
                   Eigen::Matrix4f pose_2_to_1 = Eigen::Matrix4f::Identity());

  float computeFSVWithNormals(const typename pcl::PointCloud<PointT>::ConstPtr &im1,
                              const typename pcl::PointCloud<PointT>::ConstPtr &im2,
                              pcl::PointCloud<pcl::Normal>::Ptr &normals);

  float computeOSV(const typename pcl::PointCloud<PointT>::ConstPtr &im1,
                   const typename pcl::PointCloud<PointT>::ConstPtr &im2,
                   Eigen::Matrix4f pose_2_to_1 = Eigen::Matrix4f::Identity());

  void setThresholdTSS(float t) {
    tss_ = t;
  }

  float computeFocalLength(int width, int height, const typename pcl::PointCloud<PointT>::ConstPtr &cloud);

  static void computeRangeImage(int width, int height, float fl,
                                const typename pcl::PointCloud<PointT>::ConstPtr &cloud,
                                typename pcl::PointCloud<PointT>::Ptr &range_image);
};
}  // namespace v4r
