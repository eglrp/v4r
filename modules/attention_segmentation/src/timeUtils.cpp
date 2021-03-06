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

#include "v4r/attention_segmentation/timeUtils.h"

#include <stdio.h>
#include <unistd.h>

namespace v4r {

TimeEstimationClass::TimeEstimationClass(clockid_t clockID) {
  isCounterStarted = false;
  isCounterWorkComplete = false;

  setClockID(clockID);
}

void TimeEstimationClass::setClockID(clockid_t clockID) {
  switch (clockID) {
#ifdef _POSIX_MONOTONIC_CLOCK
    case CLOCK_MONOTONIC:
    case CLOCK_MONOTONIC_RAW:
#endif
#ifdef _POSIX_CPUTIME
    case CLOCK_PROCESS_CPUTIME_ID:
#endif
#ifdef _POSIX_THREAD_CPUTIME
    case CLOCK_THREAD_CPUTIME_ID:
#endif
    case CLOCK_REALTIME_COARSE:
    case CLOCK_MONOTONIC_COARSE:
      this->clockID = clockID;
      break;
    default:
      this->clockID = CLOCK_REALTIME;
  }

  //   printf("ClockID %d\n",this->clockID);
}

void TimeEstimationClass::countingStart() {
  if (!isCounterStarted) {
    isCounterStarted = true;
    isCounterWorkComplete = false;

    clock_gettime(clockID, &startTime);
  } else
    perror("WARNING: Counting is working already!\n");
}

void TimeEstimationClass::countingEnd() {
  if (isCounterStarted) {
    clock_gettime(clockID, &endTime);

    isCounterStarted = false;
    isCounterWorkComplete = true;
  } else
    perror("WARNING: It's not possible to stop something that wasn't started!\n");
}

unsigned long long TimeEstimationClass::getRealNanosecondsCount(timespec time) {
  return (unsigned long long)time.tv_sec * 1000000000 + time.tv_nsec;
}

unsigned long long TimeEstimationClass::getCurrentTimeInNanoseconds() {
  timespec currentTime;
  clock_gettime(clockID, &currentTime);

  return getRealNanosecondsCount(currentTime);
}

unsigned long TimeEstimationClass::getCurrentTimeInSeconds() {
  timespec currentTime;
  clock_gettime(clockID, &currentTime);

  return (unsigned long)currentTime.tv_sec;
}

unsigned long long TimeEstimationClass::getWorkTimeInNanoseconds() {
  if (isCounterWorkComplete) {
    return ((endTime.tv_sec - startTime.tv_sec) * 1000000000 + (endTime.tv_nsec - startTime.tv_nsec));
    // return getRealNanosecondsCount(endTime) - getRealNanosecondsCount(startTime);
  } else
    perror("ERROR: Please first start and then stop counter!\n");

  return 0;
}

unsigned long TimeEstimationClass::getWorkTimeInSeconds() {
  if (isCounterWorkComplete)
    return (unsigned long)endTime.tv_sec - startTime.tv_sec;
  else
    perror("ERROR: Please first start and then stop counter!\n");

  return 0;
}

// TimeEstimation::TimeEstimation()
// {
// }
//
// TimeEstimation::~TimeEstimation()
// {
// }
//
// void TimeEstimation::init()
// {
//   clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);
// }
//
// double TimeEstimation::measureTimeDifference(std::string name, bool print)
// {
//   clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_time);
//   if(end_time.tv_nsec < start_time.tv_nsec)
//   {
//     int nsec = (start_time.tv_nsec - end_time.tv_nsec) / 1000000000 + 1;
//     start_time.tv_nsec -= 1000000000 * nsec;
//     start_time.tv_sec += nsec;
//   }
//   if(end_time.tv_nsec - start_time.tv_nsec > 1000000000)
//   {
//     int nsec = (start_time.tv_nsec - end_time.tv_nsec) / 1000000000;
//     start_time.tv_nsec += 1000000000 * nsec;
//     start_time.tv_sec -= nsec;
//   }
//   time_diff =  (double)(end_time.tv_sec - start_time.tv_sec) +
//                (double)(end_time.tv_nsec - start_time.tv_nsec)/1000000000.;
//
//   if(print)
//   {
//     printf("[%s] Runtime : %4.3f\n", name.c_str(), time_diff);
//   }
//
//   return(time_diff);
//
// }

}  // namespace v4r
