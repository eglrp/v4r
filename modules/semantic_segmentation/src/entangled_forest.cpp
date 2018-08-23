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
 * @file   entangled_forest.cpp
 * @author Daniel Wolf (wolf@acin.tuwien.ac.at)
 * @date   2017
 * @brief  .
 *
 */

#include <iostream>
#include <vector>

#include <v4r/semantic_segmentation/entangled_forest.h>

namespace v4r {
EntangledForest::EntangledForest() {
  // initialize random generator once
  mNTrees = 5;
  mMaxDepth = 8;
  mSampledSplitFunctionParameters = 100;
  mSampledSplitFunctionThresholds = 100;
  mMinInformationGain = 0.02;
  mMinPointsForSplit = 5;
  mBaggingRatio = 0.5;

  std::random_device rd;
  mRandomGenerator = std::mt19937(rd());
}

EntangledForest::EntangledForest(int nTrees, int maxDepth, float baggingRatio, int sampledSplitFunctionParameters,
                                 int sampledSplitFunctionThresholds, float minInformationGain, int minPointsForSplit) {
  // initialize random generator once
  this->mNTrees = nTrees;
  this->mMaxDepth = maxDepth;
  this->mSampledSplitFunctionParameters = sampledSplitFunctionParameters;
  this->mSampledSplitFunctionThresholds = sampledSplitFunctionThresholds;
  this->mMinInformationGain = minInformationGain;
  this->mMinPointsForSplit = minPointsForSplit;
  this->mBaggingRatio = baggingRatio;

  std::random_device rd;
  mRandomGenerator = std::mt19937(rd());
}

void EntangledForest::SoftClassify(EntangledForestData *data, std::vector<std::vector<double>> &result, int maxDepth,
                                   int useNTrees) {
  data->SetLabelMap(mLabelMap);

  int nclusters = data->GetNrOfClusters(0);
  result.resize(nclusters, std::vector<double>(mLabelMap.size(), 0.0));

  if (useNTrees <= 0 || useNTrees > (int)mTrees.size())
    useNTrees = mTrees.size();

  // initialize cluster node index arrays
  data->AddTreesToClusterNodeIdx(useNTrees);

  // get label distribution for every tree
  for (int i = 0; i < useNTrees; ++i) {
    std::vector<std::vector<double>> treeResult;
    mTrees[i]->Classify(data, treeResult, maxDepth);

    for (int j = 0; j < nclusters; ++j) {
      std::transform(treeResult[j].begin(), treeResult[j].end(), result[j].begin(), result[j].begin(),
                     std::plus<double>());
    }
  }

  for (int j = 0; j < nclusters; ++j) {
    std::transform(result[j].begin(), result[j].end(), result[j].begin(),
                   std::bind2nd(std::divides<double>(), useNTrees));
  }
}

void EntangledForest::Classify(EntangledForestData *data, std::vector<int> &result, int maxDepth, int useNTrees) {
  std::vector<std::vector<double>> softResult;
  SoftClassify(data, softResult, maxDepth, useNTrees);
  GetHardClassificationResult(softResult, result);
}

void EntangledForest::GetHardClassificationResult(std::vector<std::vector<double>> &softResult,
                                                  std::vector<int> &result) {
  int nclusters = softResult.size();
  result.resize(nclusters, 0);

  std::vector<int> labelList;

  for (std::map<int, int>::iterator it = mLabelMap.begin(); it != mLabelMap.end(); ++it) {
    labelList.push_back(it->first);
  }

#ifdef NDEBUG
#pragma omp parallel for
#endif
  for (int i = 0; i < nclusters; ++i) {
    std::vector<double>::iterator max_prob = std::max_element(softResult[i].begin(), softResult[i].end());
    result[i] = labelList[std::distance(softResult[i].begin(), max_prob)];
  }
}

void EntangledForest::saveMatlab(std::string filename) {
  mTrees[0]->saveMatlab(filename);
}

void EntangledForest::Merge(EntangledForest &f) {
  for (unsigned int t = 0; t < f.mTrees.size(); ++t) {
    EntangledForestTree *tr = new EntangledForestTree(&mRandomGenerator);
    f.mTrees[t]->Clone(tr);
    // adapt tree index
    tr->SetTreeIndex(mTrees.size());
    mTrees.push_back(tr);
  }

  mNTrees = mTrees.size();
}

void EntangledForest::updateLeafs(EntangledForestData *trainingData, int updateDepth, double updateWeight,
                                  bool tryUniformBags) {
  trainingData->SetLabelMap(mLabelMap);

  // initialize cluster node index arrays
  trainingData->AddTreesToClusterNodeIdx(mNTrees);

  // first try, no bagging, use all data
  trainingData->GenerateBags(&mRandomGenerator, 1.0, mNTrees, tryUniformBags);

  // update every tree
  for (unsigned int i = 0; i < mTrees.size(); i++) {
    trainingData->LoadBag(i);
    LOG_INFO("Update tree " << i + 1 << " of " << mTrees.size());
    mTrees[i]->UpdateLeafs(trainingData, updateDepth, updateWeight);
  }

  std::cout << "### Update DONE ###" << std::endl;
}

void EntangledForest::correctTreeIndices() {
  for (unsigned int t = 0; t < mTrees.size(); ++t)
    mTrees[t]->SetTreeIndex(t);
}

void EntangledForest::Train(EntangledForestData *trainingData, bool tryUniformBags, int verbosityLevel) {
  mLabelMap = trainingData->GetLabelMap();
  mLabelNames = trainingData->GetLabelNames();

  if (verbosityLevel > 0) {
    LOG_INFO("Train forest with " << mNTrees << " trees...");

    if (verbosityLevel > 1) {
      // List all labels the forest is trained for
      LOG_PLAIN(mLabelMap.size() << " available:");

      for (std::map<int, int>::iterator it = mLabelMap.begin(); it != mLabelMap.end(); ++it) {
        LOG_PLAIN(it->second << " -> " << it->first);
      }
    }
  }

  // initialize cluster node index arrays
  trainingData->AddTreesToClusterNodeIdx(mNTrees);

  trainingData->GenerateBags(&mRandomGenerator, mBaggingRatio, mNTrees, tryUniformBags);

  // train every tree independently
  for (int i = 0; i < mNTrees; i++) {
    trainingData->LoadBag(i);
    LOG_INFO("Train tree " << i + 1 << " of " << mNTrees);
    mTrees.push_back(new EntangledForestTree(&mRandomGenerator, i));
    mTrees[mTrees.size() - 1]->Train(trainingData, mMaxDepth, mSampledSplitFunctionParameters,
                                     mSampledSplitFunctionThresholds, mMinInformationGain, mMinPointsForSplit);

    if (i != mNTrees - 1) {
      // backup if forest is not complete yet
      this->SaveToBinaryFile("tmp_" + std::to_string(i + 1) + "_tree.ef");
    }
  }

  if (verbosityLevel > 0) {
    std::cout << "### TRAINING DONE ###" << std::endl;
  }
}

void EntangledForest::SaveToFile(std::string filename) {
  // save tree to file
  std::ofstream ofs(filename.c_str());
  boost::archive::text_oarchive oa(ofs);
  oa << *this;
  ofs.close();
}

// TODO: Doesn't work so far, boost throws exception?!?
void EntangledForest::SaveToBinaryFile(std::string filename) {
  // save tree to file
  std::ofstream ofs(filename.c_str(), std::ios::binary);
  boost::archive::binary_oarchive oa(ofs);
  oa << *this;
  ofs.close();
}

void EntangledForest::LoadFromFile(std::string filename, EntangledForest &f) {
  // load tree from file
  std::ifstream ifs(filename.c_str());
  boost::archive::text_iarchive ia(ifs);

  ia >> f;
  ifs.close();

  f.UpdateRandomGenerator();
}

// TODO: Doesn't work so far, boost throws exception?!?
void EntangledForest::LoadFromBinaryFile(std::string filename, EntangledForest &f) {
  // load tree from file
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  boost::archive::binary_iarchive ia(ifs);

  ia >> f;
  ifs.close();

  f.UpdateRandomGenerator();
}

std::map<int, int> EntangledForest::GetLabelMap() {
  return mLabelMap;
}

int EntangledForest::GetNrOfLabels() {
  return mLabelMap.size();
}

std::vector<std::string> &EntangledForest::GetLabelNames() {
  return mLabelNames;
}

bool EntangledForest::SetLabelNames(std::vector<std::string> &names) {
  if (names.size() != mLabelMap.size()) {
    LOG_ERROR("Number of passed label names (" << names.size() << ") does not match number of learned labels ("
                                               << mLabelMap.size() << ")! Ignore call.");
    return false;
  } else {
    mLabelNames = names;
    return true;
  }
}

EntangledForestTree *EntangledForest::GetTree(int idx) {
  if (idx < (int)mTrees.size()) {
    return mTrees[idx];
  } else {
    return nullptr;
  }
}

int EntangledForest::GetNrOfTrees() {
  return mTrees.size();
}

void EntangledForest::UpdateRandomGenerator() {
  for (auto t : mTrees) {
    t->SetRandomGenerator(&mRandomGenerator);
  }
}

EntangledForest::~EntangledForest() {
  for (unsigned int i = 0; i < mTrees.size(); ++i) {
    delete mTrees[i];
  }
}
}  // namespace v4r
