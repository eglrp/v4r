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
 * @file params.h
 * @author Johann Prankl (prankl@acin.tuwien.ac.at), Aitor Aldoma (aldoma@acin.tuwien.ac.at)
 * @date 2015
 * @brief
 *
 */

#ifndef _GRAB_PCD_PARAMS_H_
#define _GRAB_PCD_PARAMS_H_

#ifndef Q_MOC_RUN
#include <v4r/reconstruction/KeypointSlamRGBD2.h>
#include <Eigen/Dense>
#include <QDialog>
#include <opencv2/core/core.hpp>
#endif

namespace Ui {
class Params;
}

class RGBDCameraParameter {
 public:
  double f[2];
  double c[2];
  int width, height;
  double range[2];
  RGBDCameraParameter() {
    f[0] = 525;
    f[1] = 525;
    c[0] = 320;
    c[1] = 240;
    width = 640;
    height = 480;
    range[0] = 0.1;
    range[1] = 3.14;
  }
};

class CamaraTrackerParameter {
 public:
  bool log_point_clouds;
  bool create_prev_cloud;
  double min_delta_angle;
  double min_delta_distance;
  double prev_voxegrid_size;
  double prev_z_cutoff;
  CamaraTrackerParameter()
  : log_point_clouds(true), create_prev_cloud(true), min_delta_angle(20.), min_delta_distance(1.),
    prev_voxegrid_size(0.01), prev_z_cutoff(2.) {}
};

class BundleAdjustmentParameter {
 public:
  double dist_cam_add_projections;
  BundleAdjustmentParameter() : dist_cam_add_projections(0.1) {}
};

class SegmentationParameter {
 public:
  double inl_dist_plane;
  double thr_angle;
  double min_points_plane;
  SegmentationParameter() : inl_dist_plane(0.01), thr_angle(45), min_points_plane(5000) {}
};

class ObjectModelling {
 public:
  double vx_size_object;
  double edge_radius_px;

  ObjectModelling() : vx_size_object(0.001), edge_radius_px(3) {}
};

/**
 * @brief The Params class
 */
class Params : public QDialog {
  Q_OBJECT

 public:
  //! Constructor.
  explicit Params(QWidget *parent = nullptr);

  //! Destructor.
  ~Params();

  void apply_params();
  void apply_cam_params();
  std::string get_rgbd_path();
  std::string get_object_name();
  void set_object_name(const QString &txt);

 signals:

  void cam_params_changed(const RGBDCameraParameter &cam);
  void cam_tracker_params_changed(const CamaraTrackerParameter &param);
  void rgbd_path_changed();
  void bundle_adjustment_parameter_changed(const BundleAdjustmentParameter &param);
  void segmentation_parameter_changed(const SegmentationParameter &param);
  void object_modelling_parameter_changed(const ObjectModelling &param);
  void set_roi_params(const double &_bbox_scale_xy, const double &_bbox_scale_height, const double &_seg_offs);
  void set_segmentation_params(bool use_roi_segm, const double &offs, bool _use_dense_mv,
                               const double &_edge_radius_px);
  void set_cb_param(bool create_cb, float rnn_thr);

 private slots:

  void on_okButton_clicked();
  void on_pushFindRGBDPath_pressed();

  void on_applyButton_clicked();

 private:
  Ui::Params *ui;

  RGBDCameraParameter cam_params;
  CamaraTrackerParameter cam_tracker_params;
  BundleAdjustmentParameter ba_params;
  SegmentationParameter seg_params;
  ObjectModelling om_params;
};

#endif  // PARAMS_H
