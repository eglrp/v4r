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
 * @file IMKRecognizerIO.cpp
 * @author Johann Prankl (prankl@acin.tuwien.ac.at)
 * @date 2016
 * @brief
 *
 */

#include <pcl/common/common.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <v4r/io/filesystem.h>
#include <v4r/recognition/IMKRecognizerIO.h>
#include <boost/algorithm/string.hpp>

namespace v4r {

using namespace std;

/************************ PRIVATE ****************************/

/**
 * @brief IMKRecognizerIO::generateDir
 * @param dir
 * @param object_names
 * @param full_dir
 */
void IMKRecognizerIO::generateDir(const std::string &dir, const std::vector<std::string> &object_names,
                                  std::string &full_dir) {
  std::vector<std::string> sorted_names = object_names;
  std::sort(sorted_names.begin(), sorted_names.end());
  full_dir = dir + std::string("/IMK_");

  for (int i = 0; i < (int)sorted_names.size(); i++) {
    full_dir += sorted_names[i];
    if (i < ((int)sorted_names.size()) - 1)
      full_dir += std::string("_");
  }
}

/**
 * @brief IMKRecognizerIO::generateName
 * @param dir
 * @param object_names
 * @param full_name
 */
void IMKRecognizerIO::generateName(const std::string &dir, const std::vector<std::string> &object_names,
                                   std::string &full_name) {
  std::vector<std::string> sorted_names = object_names;
  std::sort(sorted_names.begin(), sorted_names.end());
  full_name = dir + std::string("/imk_");
  for (int i = 0; i < (int)sorted_names.size(); i++) {
    full_name += sorted_names[i];
    if (i < ((int)sorted_names.size()) - 1)
      full_name += std::string("_");
  }
  full_name += std::string(".bin");
}

/************************ PUBLIC *****************************/

/**
 * write
 */
void IMKRecognizerIO::write(const std::string &dir, const std::vector<std::string> &object_names,
                            const std::vector<IMKView> &object_models, const CodebookMatcher &cb,
                            const std::string &codebookFilename) {
  //  std::string full_dir;
  //  generateDir(dir, object_names, full_dir);
  //  boost::filesystem::create_directories(full_dir);

  std::string full_name;
  if (!codebookFilename.empty()) {
    full_name = dir + "/" + codebookFilename;
  } else {
    generateName(dir, object_names, full_name);
  }

  cv::Mat cb_centers = cb.getDescriptors();
  std::vector<std::vector<std::pair<int, int>>> cb_entries = cb.getEntries();
  //  std::ofstream ofs((full_dir+std::string("/imk_recognizer_model.bin")).c_str());
  std::ofstream ofs(full_name.c_str());

  boost::archive::binary_oarchive oa(ofs);
  oa << object_names;
  oa << object_models;
  oa << cb_centers;
  oa << cb_entries;
}

/**
 * read
 */
bool IMKRecognizerIO::read(const std::string &dir, std::vector<std::string> &object_names,
                           std::vector<IMKView> &object_models, CodebookMatcher &cb,
                           const std::string &codebookFilename) {
  //  std::string full_dir;
  //  generateDir(dir, object_names, full_dir);
  //  std::ifstream ifs((full_dir+std::string("/imk_recognizer_model.bin")).c_str());
  std::string full_name;
  if (!codebookFilename.empty()) {
    full_name = dir + "/" + codebookFilename;
  } else {
    generateName(dir, object_names, full_name);
  }
  std::ifstream ifs(full_name.c_str());
  if (ifs.is_open()) {
    //    cout<<(full_dir+std::string("/imk_recognizer_model.bin"))<<endl;
    cout << full_name << endl;
    cv::Mat cb_centers;
    std::vector<std::vector<std::pair<int, int>>> cb_entries;
    boost::archive::binary_iarchive ia(ifs);
    ia >> object_names;
    ia >> object_models;
    ia >> cb_centers;
    ia >> cb_entries;
    cb.setCodebook(cb_centers, cb_entries);
    return true;
  }
  return false;
}

}  // namespace v4r
