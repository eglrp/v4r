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
 * @file Texture.h
 * @author Richtsfeld
 * @date October 2012
 * @version 0.1
 * @brief Calculate texture feature to compare surface texture.
 */

#ifndef SURFACE_TEXTURE_H
#define SURFACE_TEXTURE_H

#include <stdio.h>
#include <vector>

#include <v4r/core/macros.h>
#include <opencv2/opencv.hpp>

#include "v4r/attention_segmentation//SurfaceModel.h"

namespace v4r {

class V4R_EXPORTS Texture {
 private:
  bool computed;
  bool have_edges;
  cv::Mat edges;
  bool have_indices;
  pcl::PointIndices::Ptr indices;

  int width, height;

  double textureRate;  ///< Texture rate for each surface

 public:
  typedef std::shared_ptr<Texture> Ptr;

  Texture();
  ~Texture();

  /** Set input point cloud **/
  // sets input cloud
  void setInputEdges(cv::Mat &_edges);
  // sets indices
  void setIndices(pcl::PointIndices::Ptr _indices);
  V4R_EXPORTS void setIndices(std::vector<int> &_indices);
  void setIndices(cv::Rect rect);

  double getTextureRate() {
    return textureRate;
  };
  bool getComputed() {
    return computed;
  };

  /** Compute the texture **/
  void compute();

  /** Compare surface texture **/
  double compare(Texture::Ptr t);
};

/*************************** INLINE METHODES **************************/

}  // namespace v4r

#endif
