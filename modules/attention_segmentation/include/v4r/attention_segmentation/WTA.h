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
 * @brief this implementation was inspired by http://www.saliencytoolbox.net
 */

#ifndef WTA_HPP
#define WTA_HPP

#include <v4r/core/macros.h>
#include "v4r/attention_segmentation/headers.h"

namespace v4r {

struct LIF {
  float timeStep;   // time step for integration (in sec).
  float Eleak;      // leak potential (in V).
  float Eexc;       // potential for excitatory channels (positive, in V).
  float Einh;       // potential for inhibitory channels (negative, in V).
  float Gleak;      // leak conductivity (in S).
  float Gexc;       // conductivity of excitatory channels (in S).
  cv::Mat Ginh;     // conductivity of inhibitory channels (in S).
  float GinhDecay;  // time constant for decay of inhibitory conductivity (in S).
  float Ginput;     // input conductivity (in S).
  float Vthresh;    // threshold potential for firing (in V).
  float C;          // capacity (in F).
  float time;       // current time (in sec).
  cv::Mat V;        // current membrane potential (in V) - can be an array for several neurons.
  cv::Mat I;        // current input current (in A) - can be an array for several neurons.
  float DoesFire;   // neuron can (1) or cannot (0) fire.
};

struct WTA {
  LIF sm;     // LIF neuron field for input from the saliency map.
  LIF exc;    // excitatory LIF neurons field.
  LIF inhib;  // inhibitory inter-neuron.
};

struct Params {
  float IORdecay;
  float smOutputRange;
  float noiseAmpl;
  float noiseConst;
  bool useRandom;
  float foaSize;
  int mapLevel;
  bool useCentering;
  bool useMorphologyOpenning;
};

void defaultLeakyIntFire(LIF& lif);
V4R_EXPORTS void defaultParams(Params& params);
void initializeWTA(WTA& wta, const cv::Mat& salmap, Params& salParams);
void evolveLeakyIntFire(LIF& lif, float t, cv::Mat& spikes);
void evolveWTA(WTA& wta, cv::Point& winner);
bool fastSegmentMap(cv::Mat& resultMap, cv::Mat& map, cv::Point& seedPoint);
bool SecondRound(cv::Point winner, cv::Mat& existing_points);

bool estimateShape(cv::Mat& binMap, cv::Mat& segmentedMap, cv::Mat& shapeMap, cv::Mat& salmap, cv::Point& winner,
                   Params& params, cv::Mat& image);
void applyIOR(WTA& wta, cv::Point& winner, Params& params);
void diskIOR(WTA& wta, cv::Point& winner, Params& params);
// void shapeIOR(WTA& wta, cv::Point& winner, Params& params, cv::Mat& binaryMap, cv::Mat& binMap);
void winnerToImgCoords(cv::Point& win2, cv::Point& winner, Params& params, cv::Mat& img, const cv::Mat& _salmap);
void plotSalientLocation(cv::Point& win2, cv::Point& lastWinner, cv::Mat& img, int pointNumber);
V4R_EXPORTS int CalculateWTA(cv::Mat& img, cv::Mat& _salmap, std::vector<cv::Point>& attented_points,
                             int AttentionPointsNumber, Params& params);
void PrintImage(const cv::Mat& img);
void UpdateWinner(cv::Mat& salmap, cv::Point& winner, Params& params);
void UpdateWinner2(cv::Mat& salmap, cv::Point& winner);

}  // namespace v4r

#endif  // WTA_HPP
