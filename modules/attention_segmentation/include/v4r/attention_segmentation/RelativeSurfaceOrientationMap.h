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

#ifndef RELATIVE_SURFACE_ORIENATION_HPP
#define RELATIVE_SURFACE_ORIENATION_HPP

#include <v4r/core/macros.h>
#include "v4r/attention_segmentation/BaseMap.h"

namespace v4r {

enum OrientationTypes {
  AM_VERTICAL = 0,
  AM_HORIZONTAL,
};

/**
 * class for relative surface orientation saliency map
 * */

class V4R_EXPORTS RelativeSurfaceOrientationMap : public BaseMap {
 public:
  RelativeSurfaceOrientationMap();
  ~RelativeSurfaceOrientationMap();

  void setOrientationNormal(pcl::Normal orientation_normal_);
  bool getOrientationNormal(pcl::Normal &orientation_normal_);

  void setOrientationType(int orientationType_);
  int getOrientationType();

  void setNormalThreshold(float normal_threshold_);
  float getNormalThreshold();

  /**
   * calculates single surface height map
   * */

  virtual int calculate();

  virtual void reset();
  virtual void print();

 private:
  pcl::Normal orientation_normal;
  int orientationType;
  float normal_threshold;

  bool haveOrientationNormal;

  float getOrientationType(int orientationType_);
  void orientationMap(pcl::PointCloud<pcl::Normal>::Ptr normals_cur, pcl::PointIndices::Ptr indices_cur,
                      int image_width, int image_height, float a, float b, float c, float orientationCoefficient,
                      cv::Mat &map_cur);

 protected:
  virtual int checkParameters();
  virtual int calculatePyramidSimple();
  virtual int calculatePyramidItti();
  virtual int calculatePyramidFrintrop();

  virtual int combinePyramid(BasePyramid::Ptr pyramid);
};

}  // namespace v4r

#endif  // RELATIVE_SURFACE_ORIENATION_HPP
