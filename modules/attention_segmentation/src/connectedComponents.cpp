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

#include "v4r/attention_segmentation/connectedComponents.h"

namespace v4r {

ConnectedComponent::ConnectedComponent() {
  points.clear();
  saliency_values.clear();
  average_saliency = 0;
}

void extractConnectedComponents(cv::Mat map, std::vector<ConnectedComponent> &connected_components, float th) {
  assert(map.type() == CV_32FC1);
  assert((th >= 0) && (th <= 1));

  cv::Mat map_copy;
  map.copyTo(map_copy);
  for (int i = 0; i < map_copy.rows; ++i) {
    for (int j = 0; j < map_copy.cols; ++j) {
      if (map_copy.at<float>(i, j) > th) {
        ConnectedComponent new_component;

        new_component.points.push_back(cv::Point(j, i));
        new_component.saliency_values.push_back(map_copy.at<float>(i, j));
        new_component.average_saliency = map_copy.at<float>(i, j);

        map_copy.at<float>(i, j) = 0;

        std::vector<cv::Point> queue;
        queue.push_back(cv::Point(j, i));

        while (queue.size()) {
          cv::Point cur_point = queue.back();
          queue.pop_back();

          for (int p = 0; p < 8; ++p) {
            int new_x = cur_point.x + dx8[p];
            int new_y = cur_point.y + dy8[p];

            if ((new_x < 0) || (new_y < 0) || (new_x >= map_copy.cols) || (new_y >= map_copy.rows))
              continue;

            if (map_copy.at<float>(new_y, new_x) > th) {
              new_component.points.push_back(cv::Point(new_x, new_y));
              new_component.saliency_values.push_back(map_copy.at<float>(new_y, new_x));
              new_component.average_saliency += map_copy.at<float>(new_y, new_x);

              map_copy.at<float>(new_y, new_x) = 0;

              queue.push_back(cv::Point(new_x, new_y));
            }
          }
        }

        new_component.average_saliency /= new_component.points.size();

        if (new_component.average_saliency > th) {
          connected_components.push_back(new_component);
        }
      }
    }
  }
}

void extractConnectedComponents(cv::Mat map, std::vector<ConnectedComponent> &connected_components,
                                cv::Point attention_point, float th) {
  assert(map.type() == CV_32FC1);
  assert((th >= 0) && (th <= 1));

  cv::Mat map_copy;
  map.copyTo(map_copy);
  int i = attention_point.y;
  int j = attention_point.x;
  if (map_copy.at<float>(i, j) > th) {
    ConnectedComponent new_component;

    new_component.points.push_back(cv::Point(j, i));
    new_component.saliency_values.push_back(map_copy.at<float>(i, j));
    new_component.average_saliency = map_copy.at<float>(i, j);

    map_copy.at<float>(i, j) = 0;

    std::vector<cv::Point> queue;
    queue.push_back(cv::Point(j, i));

    while (queue.size()) {
      cv::Point cur_point = queue.back();
      queue.pop_back();

      for (int p = 0; p < 8; ++p) {
        int new_x = cur_point.x + dx8[p];
        int new_y = cur_point.y + dy8[p];

        if ((new_x < 0) || (new_y < 0) || (new_x >= map_copy.cols) || (new_y >= map_copy.rows))
          continue;

        if (map_copy.at<float>(new_y, new_x) > th) {
          new_component.points.push_back(cv::Point(new_x, new_y));
          new_component.saliency_values.push_back(map_copy.at<float>(new_y, new_x));
          new_component.average_saliency += map_copy.at<float>(new_y, new_x);

          map_copy.at<float>(new_y, new_x) = 0;

          queue.push_back(cv::Point(new_x, new_y));
        }
      }
    }

    new_component.average_saliency /= new_component.points.size();

    if (new_component.average_saliency > th) {
      connected_components.push_back(new_component);
    }
  }
}

void drawConnectedComponent(ConnectedComponent component, cv::Mat &image, cv::Scalar color) {
  int nchannels = image.channels();
  for (unsigned j = 0; j < component.points.size(); ++j) {
    int x = component.points.at(j).x;
    int y = component.points.at(j).y;

    for (int i = 0; i < nchannels; ++i) {
      image.at<uchar>(y, nchannels * x + i) = color(i);
    }
  }
}

void drawConnectedComponents(std::vector<ConnectedComponent> components, cv::Mat &image, cv::Scalar color) {
  for (unsigned int i = 0; i < components.size(); ++i) {
    drawConnectedComponent(components.at(i), image, color);
  }
}

}  // namespace v4r
