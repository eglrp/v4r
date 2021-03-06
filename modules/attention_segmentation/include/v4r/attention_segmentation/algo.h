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

#ifndef EPALGO_H
#define EPALGO_H

#include <v4r/core/macros.h>
#include "v4r/attention_segmentation/eputils_headers.h"

namespace v4r {
// ep:begin: revision at 17-07-2014
V4R_EXPORTS void filterGaussian(cv::Mat &input, cv::Mat &output, cv::Mat &mask);
V4R_EXPORTS void buildDepthPyramid(cv::Mat &image, std::vector<cv::Mat> &pyramid, cv::Mat &mask,
                                   unsigned int levelNumber);
V4R_EXPORTS int createPointCloudPyramid(std::vector<cv::Mat> &pyramidX, std::vector<cv::Mat> &pyramidY,
                                        std::vector<cv::Mat> &pyramidZ, std::vector<cv::Mat> &pyramidIndices,
                                        std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> &pyramidCloud);
V4R_EXPORTS void createNormalPyramid(std::vector<cv::Mat> &pyramidNx, std::vector<cv::Mat> &pyramidNy,
                                     std::vector<cv::Mat> &pyramidNz, std::vector<cv::Mat> &pyramidIndices,
                                     std::vector<pcl::PointCloud<pcl::Normal>::Ptr> &pyramidNormal);
V4R_EXPORTS void createIndicesPyramid(std::vector<cv::Mat> &pyramidIndices,
                                      std::vector<pcl::PointIndices::Ptr> &pyramidIndiceSets);
V4R_EXPORTS void upscaleImage(cv::Mat &input, cv::Mat &output, unsigned int width, unsigned int height);
V4R_EXPORTS void downscaleImage(cv::Mat &input, cv::Mat &output, unsigned int width, unsigned int height);
V4R_EXPORTS void scaleImage(std::vector<cv::Mat> &inputPyramid, cv::Mat &input, cv::Mat &output, int inLevel,
                            int outLevel);
// ep:end: revision at 17-07-2014

/**
 * checks if point in the polygon
 * */
bool inPoly(std::vector<cv::Point> &poly, cv::Point p);
/**
 * creates polygon map
 * */
V4R_EXPORTS void buildPolygonMap(cv::Mat &polygonMap, std::vector<std::vector<cv::Point>> &polygons);
/**
 * builds contour map
 * */
V4R_EXPORTS void buildCountourMap(cv::Mat &polygonMap, std::vector<std::vector<cv::Point>> &polygons,
                                  cv::Scalar color = cv::Scalar(255, 255, 255));
/**
 * adds noise to the image
 * */
V4R_EXPORTS void addNoise(cv::Mat &image, cv::Mat &nImage, cv::RNG &rng, float min, float max);
/**
 * returns number of the point in the cammulative distribution for
 * given x value
 * */
long commulativeFunctionArgValue(float x, std::vector<float> &A);
/**
 * calculates normal pdf of the image given mean and stddev
 * */
V4R_EXPORTS void normPDF(cv::Mat &mat, float mean, float stddev, cv::Mat &res);
float normPDF(float x, float mean, float stddev);
float normPDF(std::vector<float> x, std::vector<float> mean, cv::Mat stddev);
/**
 * normalizes distribution
 * */
V4R_EXPORTS void normalizeDist(std::vector<float> &dist);
/**
 * calculates mean
 * */
float getMean(std::vector<float> dist, float total_num = 1);
/**
 * calculates std
 * */
float getStd(std::vector<float> dist, float mean, float total_num = 1);
/**
 * creates contours from masks
 * */
V4R_EXPORTS void createContoursFromMasks(std::vector<cv::Mat> &masks, std::vector<std::vector<cv::Point>> &contours);
/**
 * checks transitions from 0 to 1 in the neighbourhood
 * */
uchar Num01Transitions(cv::Mat s, int j, int i);
/**
 * removes extra connections according to M-connectivity
 * */
V4R_EXPORTS void MConnectivity(cv::Mat &s, uchar *element);
/**
 * extracts skeleton
 * */
V4R_EXPORTS void Skeleton(cv::Mat a, cv::Mat &s);
/**
 * calculates distance between two points using eucledian normal
 * */
V4R_EXPORTS float calculateDistance(cv::Point center, cv::Point point);
/**
 * calculates gaussian distance
 * */
float calculateDistance(cv::Point center, cv::Point point, float sigma);
/**
 * calculate object center
 * */
V4R_EXPORTS void calculateObjectCenter(cv::Mat mask, cv::Point &center);
V4R_EXPORTS void calculateObjectCenter(std::vector<cv::Point> contour, cv::Mat mask, cv::Point &center);
/**
 * calculate neigbors in 2D
 * */
V4R_EXPORTS void get2DNeighbors(const cv::Mat &patches, cv::Mat &neighbors, int patchesNumber);
/**
 * calculate neigbors in 3D
 * */
#ifndef NOT_USE_PCL

template <typename T>
V4R_EXPORTS void get3DNeighbors(const cv::Mat &patches, cv::Mat &neighbors, int patchesNumber,
                                typename pcl::PointCloud<T>::Ptr cloud, double z_max_dist) {
  neighbors = cv::Mat_<bool>(patchesNumber, patchesNumber);
  neighbors.setTo(false);

  int width = patches.cols;
  int height = patches.rows;

  //@ep TODO: uncomment?
  //   int dr[4] = {-1,-1, 0, 1};
  //   int dc[4] = { 0,-1,-1,-1};

  int dr[4] = {-1, 0, -1};
  int dc[4] = {0, -1, -1};

  for (int r = 1; r < patches.rows - 1; r++) {
    for (int c = 1; c < patches.cols - 1; c++) {
      // if the patch exist
      if (patches.at<int>(r, c) != -1) {
        int patchIdx = patches.at<int>(r, c);

        //@ep: why we did not use 1,-1 shift???
        for (int i = 0; i < 3; ++i)  //@ep: TODO 3->4
        {
          int nr = r + dr[i];
          int nc = c + dc[i];

          int currentPatchIdx = patches.at<int>(nr, nc);
          if (currentPatchIdx == -1)
            continue;

          if (patchIdx != currentPatchIdx) {
            int idx0 = r * width + c;
            int idx1 = nr * width + nc;
            // @ep:: are we wure that this should be distance only in z direction, and not Euclidean distance???
            double dis = fabs(cloud->points.at(idx0).z - cloud->points.at(idx1).z);
            if (dis < z_max_dist) {
              neighbors.at<bool>(currentPatchIdx, patchIdx) = true;
              neighbors.at<bool>(patchIdx, currentPatchIdx) = true;
            }
          }
        }
      }
    }
  }
}

#endif

}  // namespace v4r

#endif  // EPALGO_H
