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

#ifndef KP_CLUSTERING_RNN_HH
#define KP_CLUSTERING_RNN_HH

#include <float.h>
#include <v4r/common/Clustering.h>
#include <v4r/core/macros.h>
#include <stdexcept>
#include <string>
#include <v4r/common/impl/DataMatrix2D.hpp>
#include <vector>

namespace v4r {

/**
 * ClusteringRNN
 */
class V4R_EXPORTS ClusteringRNN : public Clustering {
 public:
  class Parameter {
   public:
    float dist_thr;

    Parameter(float _dist_thr = 0.4) : dist_thr(_dist_thr) {}
  };

 private:
  std::vector<Cluster::Ptr> clusters;

  int getNearestNeighbour(const Cluster &cluster, const std::vector<Cluster::Ptr> &clusters, float &sim);
  void agglomerate(const Cluster &src, Cluster &dst);

  void initDataStructure(const DataMatrix2Df &samples, std::vector<Cluster::Ptr> &data);

 public:
  Parameter param;
  bool dbg;

  ClusteringRNN(const Parameter &_param = Parameter(), bool _dbg = true);
  ~ClusteringRNN();

  virtual void cluster(const DataMatrix2Df &samples);
  virtual void getClusters(std::vector<std::vector<int>> &_clusters);
  virtual void getCenters(DataMatrix2Df &_centers);
};

/************************** INLINE METHODES ******************************/
}

#endif
