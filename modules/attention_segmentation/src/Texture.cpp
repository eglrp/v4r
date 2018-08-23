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
 * @file Texture.cpp
 * @author Richtsfeld
 * @date October 2012
 * @version 0.1
 * @brief Calculate texture feature to compare surface texture.
 */

#include "v4r/attention_segmentation/Texture.h"

namespace v4r {

/************************************************************************************
 * Constructor/Destructor
 */

Texture::Texture() {
  computed = false;
  have_edges = false;
  have_indices = false;
}

Texture::~Texture() {}

void Texture::setInputEdges(cv::Mat &_edges) {
  if ((_edges.cols <= 0) || (_edges.rows <= 0)) {
    LOG(ERROR) << "Invalid image (height|width must be > 1";
    throw std::runtime_error("[Texture::setInputEdges] Invalid image (height|width must be > 1)");
  }

  if (_edges.type() != CV_8UC1) {
    LOG(ERROR) << "Invalid image type (must be 8UC1)";
    throw std::runtime_error("[Texture::setInputEdges] Invalid image type (must be 8UC1)");
  }

  edges = _edges;
  width = edges.cols;
  height = edges.rows;

  have_edges = true;
  computed = false;

  indices.reset(new pcl::PointIndices);
  for (int i = 0; i < width * height; i++) {
    indices->indices.push_back(i);
  }
}

void Texture::setIndices(pcl::PointIndices::Ptr _indices) {
  if (!have_edges) {
    LOG(ERROR) << "No edges available.";
    throw std::runtime_error("[Texture::setIndices]: Error: No edges available.");
  }

  indices = _indices;
  have_indices = true;
}

void Texture::setIndices(std::vector<int> &_indices) {
  indices.reset(new pcl::PointIndices);
  indices->indices = _indices;
}

void Texture::setIndices(cv::Rect _rect) {
  if (!have_edges) {
    LOG(ERROR) << "No edges available.";
    throw std::runtime_error("[Texture::setIndices]: Error: No edges available.");
  }

  if (_rect.y >= height) {
    _rect.y = height - 1;
  }

  if ((_rect.y + _rect.height) >= height) {
    _rect.height = height - _rect.y;
  }

  if (_rect.x >= width) {
    _rect.x = width - 1;
  }

  if ((_rect.x + _rect.width) >= width) {
    _rect.width = width - _rect.x;
  }

  VLOG(1) << "_rect = " << _rect.x << ", " << _rect.y << ", " << _rect.x + _rect.width << ", "
          << _rect.y + _rect.height;

  indices.reset(new pcl::PointIndices);
  for (int r = _rect.y; r < (_rect.y + _rect.height); r++) {
    for (int c = _rect.x; c < (_rect.x + _rect.width); c++) {
      indices->indices.push_back(r * width + c);
    }
  }

  have_indices = true;
}

void Texture::compute() {
  if (!have_edges) {
    LOG(ERROR) << "No edges available.";
    throw std::runtime_error("[Texture::compute]: Error: No edges available.");
  }

  textureRate = 0.0;

  if ((indices->indices.size()) <= 0) {
    computed = true;
    return;
  }

  int texArea = 0;
  for (unsigned int i = 0; i < indices->indices.size(); i++) {
    int x = indices->indices.at(i) % width;
    int y = indices->indices.at(i) / width;
    if (edges.at<uchar>(y, x) == 255)
      texArea++;
  }

  textureRate = (double)((double)texArea / indices->indices.size());

  computed = true;
}

double Texture::compare(Texture::Ptr t) {
  if (!computed || !(t->getComputed())) {
    LOG(ERROR) << "Texture is not computed.";
    return 0.;
  }

  return 1. - fabs(textureRate - t->getTextureRate());
}

}  // namespace v4r
