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

#include <v4r/features/FeatureDetector_K_HARRIS.h>
#include <v4r/tracking/ObjectTrackerMono.h>
#include <boost/thread.hpp>
//#include "v4r/KeypointTools/ScopeTime.hpp"
#include <v4r/keypoints/impl/invPose.hpp>
#include <v4r/reconstruction/impl/projectPointToImage.hpp>

namespace v4r {

using namespace std;

/************************************************************************************
 * Constructor/Destructor
 */
ObjectTrackerMono::ObjectTrackerMono(const ObjectTrackerMono::Parameter &p)
: param(p), conf(0.), conf_cnt(0), not_conf_cnt(1000) {
  view.reset(new ObjectView(0));
  FeatureDetector::Ptr estDesc(new FeatureDetector_KD_FAST_IMGD(param.det_param));
  FeatureDetector::Ptr det = estDesc;  //(new FeatureDetector_K_HARRIS());// = estDesc;
  kpDetector.reset(new KeypointPoseDetector(param.kd_param, det, estDesc));
  projTracker.reset(new ProjLKPoseTrackerR2(param.kt_param));
  lkTracker.reset(new LKPoseTracker(param.lk_param));
  kpRecognizer.reset(new KeypointObjectRecognizerR2(param.or_param, det, estDesc));
}

ObjectTrackerMono::~ObjectTrackerMono() {}

/**
 * viewPointChange
 */
double ObjectTrackerMono::viewPointChange(const Eigen::Vector3f &pt, const Eigen::Matrix4f &inv_pose1,
                                          const Eigen::Matrix4f &inv_pose2) {
  Eigen::Vector3f v1 = (inv_pose1.block<3, 1>(0, 3) - pt).normalized();
  Eigen::Vector3f v2 = (inv_pose2.block<3, 1>(0, 3) - pt).normalized();

  float a = v1.dot(v2);

  if (a > 0.9999)
    a = 0;
  else
    a = acos(a);

  return a;
}

/**
 * @brief ObjectTrackerMono::updateView
 * @param model
 * @param pose
 * @param view
 */
void ObjectTrackerMono::updateView(const Eigen::Matrix4f &pose, const Object &_model, ObjectView::Ptr &_view) {
  double angle, min_angle = DBL_MAX;
  int idx = -1;
  Eigen::Matrix4f inv_pose, inv_pose2;

  invPose(pose, inv_pose);

  for (unsigned i = 0; i < _model.views.size(); i++) {
    const ObjectView &ov = *_model.views[i];
    invPose(_model.cameras[ov.camera_id], inv_pose2);
    angle = viewPointChange(ov.center, inv_pose2, inv_pose);

    if (angle < min_angle) {
      min_angle = angle;
      idx = (int)i;
    }
  }

  if (idx != _view->idx && idx != -1) {
    _view = _model.views[idx];

    kpDetector->setModel(_view);
    projTracker->setModel(_view, _model.cameras[_view->camera_id]);
    lkTracker->setModel(_view);
  }
  //--
  // cout<<idx<<endl;
}

/**
 * @brief ObjectTrackerMono::reinit
 * @param im
 * @param pose
 * @param view
 * @return confidence value
 */
double ObjectTrackerMono::reinit(const cv::Mat_<unsigned char> &im, Eigen::Matrix4f &pose, ObjectView::Ptr &_view) {
  //--
  //  std::vector< std::pair<int, int> > view_rank;
  //  std::vector< cv::KeyPoint > keys;
  //  cv::Mat descs;

  //  ::FeatureDetector::Ptr detector(new FeaturekpDetector_KD_FAST_IMGD(param.det_param));
  //  ::FeatureDetector::Ptr descEstimator = detector;

  //  detector->detect(im, keys);
  //  descEstimator->extract(im, keys, descs);

  //  { ::ScopeTime t("cbMatcher->queryViewRank");
  //  cbMatcher->queryViewRank(descs, view_rank);
  //  }

  //  for (unsigned i=0; i<view_rank.size(); i++)
  //    cout<<view_rank[i].first<<" ";
  //  cout<<endl;

  //  if (view_rank.size()>0)
  //  { view = model->views[view_rank[0].first]; cout<<"view: "<<view_rank[0].first; }
  //  else return 0.;
  // --

  // use object recognizer
  if (model->haveCodebook() && param.use_codebook) {
    int view_idx;
    double _conf = kpRecognizer->detect(im, pose, view_idx);

    if (view_idx != -1) {
      _view = model->views[view_idx];
      kpDetector->setModel(_view);
      projTracker->setModel(_view, model->cameras[_view->camera_id]);
      lkTracker->setModel(_view);
    }

    return _conf;
  } else {
    // random sample views and try to reinit
    _view = model->views[rand() % model->views.size()];

    kpDetector->setModel(_view);
    projTracker->setModel(_view, model->cameras[_view->camera_id]);
    lkTracker->setModel(_view);

    return kpDetector->detect(im, pose);
  }
}

/***************************************************************************************/

/**
 * @brief ObjectTrackerMono::reset
 */
void ObjectTrackerMono::reset() {
  conf = 0;
  conf_cnt = 0;
  not_conf_cnt = 1000;
  view.reset(new ObjectView(0));
}

/**
 * @brief ObjectTrackerMono::track
 * @param image input image
 * @param pose estimated pose
 * @param conf confidence value
 * @return
 */
bool ObjectTrackerMono::track(const cv::Mat &image, Eigen::Matrix4f &pose, double &out_conf) {
  if (model.get() == 0 || model->views.size() == 0)
    throw std::runtime_error("[ObjectTrackerMono::track] No model available!");

  //::ScopeTime t("tracking");
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray, cv::COLOR_RGB2GRAY);
  else
    image.copyTo(im_gray);

  if (!dbg.empty())
    projTracker->dbg = dbg;
  // if (!dbg.empty()) kpDetector->dbg = dbg;
  // if (!dbg.empty()) lkTracker->dbg = dbg;
  if (!dbg.empty())
    kpRecognizer->dbg = dbg;

  // do refinement
  if (not_conf_cnt >= param.min_not_conf_cnt) {
    conf = reinit(im_gray, pose, view);
  }

  if (conf > 0.001) {
    if (param.do_inc_pyr_lk && conf > param.conf_reinit)
      /*conf =*/lkTracker->detectIncremental(im_gray, pose);
    conf = projTracker->detect(im_gray, pose);
  }

  if (conf > param.conf_reinit) {
    lkTracker->setLastFrame(im_gray, pose);
  }

  // compute confidence
  if (conf >= param.min_conf)
    conf_cnt++;
  else
    conf_cnt = 0;

  if (conf < param.min_conf)
    not_conf_cnt++;
  else
    not_conf_cnt = 0;

  out_conf = conf;

  // update pose
  if (conf_cnt >= param.min_conf_cnt) {
    updateView(pose, *model, view);
    return true;
  }

  return false;
}

/**
 * setCameraParameter
 */
void ObjectTrackerMono::setCameraParameter(const cv::Mat &_intrinsic, const cv::Mat &_dist_coeffs) {
  dist_coeffs = cv::Mat_<double>();

  if (_intrinsic.type() != CV_64F)
    _intrinsic.convertTo(intrinsic, CV_64F);
  else
    _intrinsic.copyTo(intrinsic);

  if (!_dist_coeffs.empty()) {
    dist_coeffs = cv::Mat_<double>::zeros(1, 8);
    for (int row_id = 0; row_id < _dist_coeffs.rows; row_id++) {
      for (int col_id = 0; col_id < _dist_coeffs.cols; col_id++) {
        dist_coeffs(0, row_id * dist_coeffs.rows + col_id) = _dist_coeffs.at<double>(row_id, col_id);
      }
    }
  }

  kpDetector->setCameraParameter(intrinsic, dist_coeffs);
  lkTracker->setCameraParameter(intrinsic, dist_coeffs);
  projTracker->setTargetCameraParameter(intrinsic, dist_coeffs);
  kpRecognizer->setCameraParameter(intrinsic, dist_coeffs);
}

/**
 * setObjectCameraParameter
 * TODO: We should store the camera parameter of the learning camera
 */
void ObjectTrackerMono::setObjectCameraParameter(const cv::Mat &_intrinsic, const cv::Mat &_dist_coeffs) {
  cv::Mat_<double> intrinsicc;
  cv::Mat_<double> dist_coeffss = cv::Mat_<double>();

  if (_intrinsic.type() != CV_64F)
    _intrinsic.convertTo(intrinsicc, CV_64F);
  else
    _intrinsic.copyTo(intrinsicc);

  if (!_dist_coeffs.empty()) {
    dist_coeffss = cv::Mat_<double>::zeros(1, 8);
    for (int i = 0; i < _dist_coeffs.cols * _dist_coeffs.rows; i++)
      dist_coeffss(0, i) = _dist_coeffs.at<double>(0, i);
  }

  projTracker->setSourceCameraParameter(intrinsicc, dist_coeffss);
}

/**
 * @brief ObjectTrackerMono::setObjectModel
 * @param _model
 */
void ObjectTrackerMono::setObjectModel(const Object::Ptr &_model) {
  reset();

  model = _model;

  // set camera parameter
  if (model->camera_parameter.size() == 1) {
    std::vector<double> &_param = model->camera_parameter.back();
    cv::Mat_<double> cam(cv::Mat_<double>::eye(3, 3));
    cv::Mat_<double> dcoeffs;

    if (_param.size() == 9) {
      dcoeffs = cv::Mat_<double>::zeros(1, 5);
      dcoeffs(0, 0) = _param[4];
      dcoeffs(0, 1) = _param[5];
      dcoeffs(0, 4) = _param[6];
      dcoeffs(0, 2) = _param[7];
      dcoeffs(0, 3) = _param[8];
    }

    if (_param.size() == 4 || _param.size() == 9) {
      cam(0, 0) = _param[0];
      cam(1, 1) = _param[1];
      cam(0, 2) = _param[2];
      cam(1, 2) = _param[3];

      setObjectCameraParameter(cam, dcoeffs);
    }
  }

  if (model->haveCodebook() && param.use_codebook) {
    kpRecognizer->setModel(model);
  }
}
}  // namespace v4r
