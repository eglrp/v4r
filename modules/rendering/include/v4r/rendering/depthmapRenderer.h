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
 * @file depthmapRenderer.h
 * @author Simon Schreiberhuber (schreiberhuber@acin.tuwien.ac.at)
 * @date Nov, 2015
 * @brief
 *
 */

#pragma once

#include <GL/glew.h>

#include <opencv2/opencv.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <v4r/core/macros.h>
#include <eigen3/Eigen/Eigen>
#include "dmRenderObject.h"

// This is for testing the offscreen rendering context:

namespace v4r {
const size_t maxMeshSize = 1000000;  // this will lead to a big ssbo^^ (16mb)
                                     /**
                                      * @brief The Renderer class
                                      * renders a depth map from a model (every model you can load via assimp)
                                      * Although technically not part of the same problem this class can give points
                                      * along a sphere. (Good for generating views to an object)
                                      */
class V4R_EXPORTS DepthmapRenderer {
 private:
  static bool glfwRunning;

  // hide the default constructor
  DepthmapRenderer();

  // pointer to the current model
  DepthmapRendererModel *model;

  // Shader for rendering all that stuff
  GLuint shaderProgram;
  GLuint projectionUniform;
  GLuint _projectionUniform;
  GLuint poseUniform;
  GLuint viewportResUniform;
  GLuint textureUniform;
  GLuint posAttribute;
  GLuint texPosAttribute;
  GLuint normalAttribute;

  // Used to give each triangle its own id
  GLuint atomicCounterBuffer;

  // For each triangle the ssbo stores the number of pixel it would have if it weren't occluded
  // and also the surface area of the triangle
  GLuint SSBO;

  // framebuffer to render the model into
  GLuint FBO;
  GLuint zBuffer;
  GLuint depthTex;
  // stores the triangle id for each pixel
  GLuint indexTex;
  GLuint colorTex;
  GLuint normalTex;

  // the obligatory VAO
  GLuint VAO;

  // Buffer for geometry
  GLuint VBO;
  GLuint IBO;

  // camera intrinsics:
  Eigen::Vector4f fxycxy;
  Eigen::Vector2i res;

  // Stores the camera pose:
  Eigen::Matrix4f pose;

  // this here is to create points as part of a sphere
  // The next two i stole from thomas mörwald
  int search_midpoint(int &index_start, int &index_end, size_t &n_vertices, int &edge_walk, std::vector<int> &midpoint,
                      std::vector<int> &start, std::vector<int> &end, std::vector<float> &vertices);
  void subdivide(size_t &n_vertices, size_t &n_edges, size_t &n_faces, std::vector<float> &vertices,
                 std::vector<int> &faces);

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  /**
   * @brief DepthmapRenderer
   * @param resx the resolution has to be fixed at the beginning of the program
   * @param resy
   */
  DepthmapRenderer(int resx, int resy);

  ~DepthmapRenderer();

  /**
   * @brief createSphere
   * @param r radius
   * @param subdivisions there are 12 points by subdividing you add a lot more to them
   * @return vector of poses around a sphere
   */
  std::vector<Eigen::Vector3f> createSphere(float r, size_t subdivisions);

  /**
   * @brief setIntrinsics
   * @param fx focal length
   * @param fy
   * @param cx center of projection
   * @param cy
   */
  void setIntrinsics(float fx, float fy, float cx, float cy);

  /**
   * @brief setModel
   * @param model
   */
  void setModel(DepthmapRendererModel *_model);

  /**
   * @brief getPoseLookingToCenterFrom
   * @param position
   * @return
   */
  static Eigen::Matrix4f getPoseLookingToCenterFrom(const Eigen::Vector3f &position);

  /**
   * @brief lookAt
   * Function to look at a point from another point.
   * To define the orientation of the camera you need a vector pointing to the top of the camera.
   * @param from
   * camera position
   * @param to
   * position the camera is looking at
   * @param up
   * show the camera where upwards is
   * @return
   */
  static Eigen::Matrix4f lookAt(const Eigen::Vector3f &from, const Eigen::Vector3f &to, const Eigen::Vector3f &up);

  /**
   * @brief setCamPose
   * @param pose
   * A 4x4 Matrix giving the pose
   */
  void setCamPose(const Eigen::Matrix4f &_pose);

  /**
   * @brief renderDepthmap
   * @param visibleSurfaceArea: Returns an estimate of how much of the models surface area
   *        is visible. (Value between 0 and 1)
   * @param color: if the geometry contains color information this cv::Mat will contain
   *        a color image after calling this method. (otherwise it will be plain black)
   * @return a depthmap
   */
  cv::Mat renderDepthmap(float &visibleSurfaceArea, cv::Mat &color, cv::Mat &normal) const;

  /**
   * @brief renderPointcloud
   * @param visibleSurfaceArea: Returns an estimate of how much of the models surface area
   *        is visible.
   * @return
   */
  pcl::PointCloud<pcl::PointXYZ> renderPointcloud(float &visibleSurfaceArea) const;

  /**
   * @brief renderPointcloudColor
   * @param visibleSurfaceArea: Returns an estimate of how much of the models surface area
   *        is visible.
   * @return
   */
  pcl::PointCloud<pcl::PointXYZRGB> renderPointcloudColor(float &visibleSurfaceArea) const;

  pcl::PointCloud<pcl::PointXYZRGBNormal> renderPointcloudColorNormal(float &visibleSurfaceArea) const;

  typedef std::shared_ptr<DepthmapRenderer> Ptr;
  typedef std::shared_ptr<DepthmapRenderer const> ConstPtr;
};
}  // namespace v4r
