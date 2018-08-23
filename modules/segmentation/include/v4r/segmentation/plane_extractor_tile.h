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
 * @file plane_extractor_tile.h
 * @author Simon Schreiberhuber (schreiberhuber@acin.tuwien.ac.at), Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2017
 * @brief
 *
 */

#pragma once
#include <opencv/cv.h>
#include <v4r/segmentation/plane_extractor.h>

namespace v4r {
class PlaneExtractorTileParameter {
 public:
  int minNrPatches_;  ///< The minimum number of blocks that are allowed to spawn a plane
  int patchDim_;      ///< Patches are made of pixel squares that have exactly these side length
  float
      minBlockInlierRatio_;  ///< The minimum ratio of points that have to be in a patch before it would get discarded.
  bool pointwiseNormalCheck_;  ///< Activating this allowes to reduce a lot of calculations and improve speed by a lot
  float maxDistance_;          ///< A maximum distance at which no plane segmentation is possible anymore
  float maxStepSize_;    ///< maxStepSize to calculate normals there is a maximum step size which allowes to correctly
                         /// distinguish between 2 adjacent plane segments.
  float maxInlierDist_;  ///< The maximum distance a point is allowed to be out of his plane
  bool useVariableThresholds_;  ///< useVariableThresholds
  float maxInlierBlockDist_;    ///< The maximum distance two adjacent patches are allowed to be out of plane
  bool doZTest_;                ///< Only the closest possible points get added to a plane

  PlaneExtractorTileParameter()
  : minNrPatches_(5), patchDim_(10), minBlockInlierRatio_(0.95),           // 0.9f
    pointwiseNormalCheck_(false), maxDistance_(4.f), maxStepSize_(0.05f),  // 0.1f
    maxInlierDist_(0.01f), useVariableThresholds_(true), maxInlierBlockDist_(0.005f), doZTest_(true) {}

  /**
   * @brief init parameters
   * @param command_line_arguments (according to Boost program options library)
   * @return unused parameters (given parameters that were not used in this initialization call)
   */
  std::vector<std::string> init(int argc, char **argv) {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    return init(arguments);
  }

  /**
   * @brief init parameters
   * @param command_line_arguments (according to Boost program options library)
   * @return unused parameters (given parameters that were not used in this initialization call)
   */
  std::vector<std::string> init(const std::vector<std::string> &command_line_arguments) {
    po::options_description desc("Plane Extractor Tile-based Parameter\n=====================\n");
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("plane_extractor_minNrPatches", po::value<int>(&minNrPatches_)->default_value(minNrPatches_),
                       "The minimum number of blocks that are allowed to spawn a plane");
    desc.add_options()("plane_extractor_patchDim", po::value<int>(&patchDim_)->default_value(patchDim_),
                       "The minimum number of blocks that are allowed to spawn a plane");
    desc.add_options()("plane_extractor_minBlockInlierRatio",
                       po::value<float>(&minBlockInlierRatio_)->default_value(minBlockInlierRatio_),
                       "The minimum ratio of points that have to be in a patch before it would get discarded");
    desc.add_options()("plane_extractor_pointwiseNormalCheck",
                       po::value<bool>(&pointwiseNormalCheck_)->default_value(pointwiseNormalCheck_),
                       "Activating this allows to reduce a lot of calculations and improves speed.");
    desc.add_options()("plane_extractor_maxDistance", po::value<float>(&maxDistance_)->default_value(maxDistance_),
                       "A maximum distance at which no plane segmentation is possible anymore.");
    desc.add_options()(
        "plane_extractor_maxStepSize", po::value<float>(&maxStepSize_)->default_value(maxStepSize_),
        " maxStepSize to calculate normals there is a maximum step size which allowes to correctly distinguish between "
        "2 adjacent plane segments.");
    desc.add_options()("plane_extractor_maxInlierDist",
                       po::value<float>(&maxInlierDist_)->default_value(maxInlierDist_),
                       "The maximum distance a point is allowed to be out of his plane");
    desc.add_options()("plane_extractor_useVariableThresholds",
                       po::value<bool>(&useVariableThresholds_)->default_value(useVariableThresholds_),
                       " useVariableThresholds");
    desc.add_options()("plane_extractor_maxInlierBlockDist",
                       po::value<float>(&maxInlierBlockDist_)->default_value(maxInlierBlockDist_),
                       "The maximum distance two adjacent patches are allowed to be out of plane");
    desc.add_options()("plane_extractor_doZTest", po::value<bool>(&doZTest_)->default_value(doZTest_),
                       "Only the closest possible points get added to a plane");
    po::variables_map vm;
    po::parsed_options parsed =
        po::command_line_parser(command_line_arguments).options(desc).allow_unregistered().run();
    std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed.options, po::include_positional);
    po::store(parsed, vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      to_pass_further.push_back("-h");
    }
    try {
      po::notify(vm);
    } catch (std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl << std::endl << desc << std::endl;
    }
    return to_pass_further;
  }
};

template <typename PointT>
class V4R_EXPORTS PlaneExtractorTile : public PlaneExtractor<PointT> {
 protected:
  using PlaneExtractor<PointT>::cloud_;
  using PlaneExtractor<PointT>::normal_cloud_;
  using PlaneExtractor<PointT>::all_planes_;
  using PlaneExtractor<PointT>::plane_inliers_;

  PlaneExtractorTileParameter param_;

 public:
  PlaneExtractorTile(const PlaneExtractorTileParameter &p = PlaneExtractorTileParameter());

  virtual bool getRequiresNormals() const override {
    return param_.pointwiseNormalCheck_;
  }

 private:
  /**
   * @brief The PlaneMatrix struct
   * structure to store a symmetrical 3 by 3 matrix
   */
  struct PlaneMatrix {
    Eigen::Vector3d sum;
    double xx;
    double xy;
    double xz;
    double yy;
    double yz;
    double zz;
    int nrPoints;

    PlaneMatrix() : sum(Eigen::Vector3d::Zero()), xx(0), xy(0), xz(0), yy(0), yz(0), zz(0), nrPoints(0) {}

    inline PlaneMatrix operator+(const PlaneMatrix &b) {
      PlaneMatrix a;
      a.sum = sum + b.sum;
      a.xx = xx + b.xx;
      a.xy = xy + b.xy;
      a.xz = xz + b.xz;
      a.yy = yy + b.yy;
      a.yz = yz + b.yz;
      a.zz = zz + b.zz;
      a.nrPoints = nrPoints + b.nrPoints;
      return a;
    }
    inline void operator+=(const PlaneMatrix &b) {
      sum = sum + b.sum;
      xx = xx + b.xx;
      xy = xy + b.xy;
      xz = xz + b.xz;
      yy = yy + b.yy;
      yz = yz + b.yz;
      zz = zz + b.zz;
      nrPoints = nrPoints + b.nrPoints;
    }
  };

  /**
   * @brief The PlaneSegment struct
   * plane segment
   */
  struct PlaneSegment {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    Eigen::Vector4f plane;
    int nrInliers;
  };

  // maybe supply this with a list of additional
  struct Plane {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    Eigen::Vector4f plane;
    int nrElements;
  };

  std::vector<PlaneMatrix> matrices;
  std::vector<Plane> planeList;
  std::vector<PlaneMatrix> planeMatrices;
  std::vector<std::vector<PlaneSegment>> planes;
  std::vector<std::vector<Eigen::Vector3f>> centerPoints;
  cv::Mat patchIds;

  // big todo for speed: switch to Vector4f elements
  // http://eigen.tuxfamily.org/index.php?title=FAQ#Vectorization

  Eigen::Vector4f calcPlaneFromMatrix(const PlaneMatrix &mat) const;

  void replace(int from, int to, int maxIndex);

  cv::Mat getDebugImage(bool doNormalTest);

  int minAbsBlockInlier;
  int colsOfPatches;  ///< The dimensions of the downsampled image of patches
  int rowsOfPatches;  ///< The dimensions of the downsampled image of patches
  int maxId;          ///< The highest used id of

  int allocateMemory();

  void calculatePlaneSegments(bool doNormalTest);

  // TODO: this uses up too much time... get rid of it
  void rawPatchClustering();

  void postProcessing1Direction(const int offsets[][2], bool doNormalTest, bool reverse, bool zTest);
  void postProcessing(bool doNormalTest, bool zTest);

  /**
   * @brief zBuffer
   * This buffer contains the distance of a point to the assumed plane.
   * Only used when doZTest is set to true
   */
  cv::Mat zBuffer;

  /**
   * @brief thresholdsBuffer
   * Stores the thresholds for the according patches:
   * channel1 maxBlockDistance
   * channel2 minCosBlockAngle
   * channel3 maxInlierDistance
   * channel4 minCosAngle
   */
  std::vector<std::vector<Eigen::Vector4f, Eigen::aligned_allocator<Eigen::Vector4f>>> thresholdsBuffer;

  float maxBlockAngle;     ///< The maximum angle that is allowed between two adjacent blocks to be able to connect them
  float minCosBlockAngle;  ///< The cos of this block angle
  float (*minCosBlockAngleFunc)(float depth) = nullptr;  ///< minCosBlockAngleFunc
  float maxAngle;     ///< The angle the normal vector of a pointer is never allowed to be off of a patch before getting
                      /// discarded.
  float minCosAngle;  ///< The cos of this angle
  float (*minCosAngleFunc)(float depth) = nullptr;         ///< minCosAngleFunc
  float (*maxInlierDistFunc)(float depth) = nullptr;       ///< maxInlierDistFunc
  float (*maxInlierBlockDistFunc)(float depth) = nullptr;  ///< inlierBlockDistanceFunc

  // Some parameters for maximum
  cv::Mat segmentation;
  std::vector<Plane> resultingPlanes;

  cv::Mat debug;

  /**
   * @brief generateColorCodedTexture
   * @return
   * a rgb image of the segmentation result
   */
  cv::Mat generateColorCodedTexture() const;

  /**
   * @brief generateColorCodedTextureDebug
   * @return
   */
  cv::Mat generateColorCodedTextureDebug() const;

 public:
  void setMaxAngle(float angle) {
    maxAngle = angle;
    minCosAngle = cos(maxAngle);
  }

  void setMaxBlockAngle(float angle) {
    maxBlockAngle = angle;
    minCosBlockAngle = cos(maxBlockAngle);
  }

  void compute() override;

  typedef std::shared_ptr<PlaneExtractorTile<PointT>> Ptr;
  typedef std::shared_ptr<PlaneExtractorTile<PointT> const> ConstPtr;
};
}  // namespace v4r
