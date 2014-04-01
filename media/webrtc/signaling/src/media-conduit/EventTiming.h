/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EVENT_TIMING_H_
#define EVENT_TIMING_H_

#include <sys/time.h>

#include <iostream>

struct TimeStampedEvent {
  TimeStampedEvent(const std::string& name) :
      name_(name) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    time_ = tv.tv_sec;
    time_ *= 1000000;
    time_ += tv.tv_usec;
  };

  const std::string name_;
  uint64_t time_;
};

class TimeStamper {
 public:
    TimeStamper(const char* name, std::ostream* file) :
      name_(name),
      file_(file),
      stamps_() {
  }

  void Stamp(const char* event) {
    stamps_.push_back(new TimeStampedEvent(event));
  }

  void Dump() {
    if (!file_)
      return;

    uint64_t last = 0;
    *file_ << "TIME RESULTS FOR = " << name_ << std::endl;

    for (auto a = stamps_.begin();  a != stamps_.end(); ++a) {
      if (!last)
        last = (*a)->time_;

      *file_ << (*a)->name_ << ": " << (*a)->time_ << "("
            << (*a)->time_ - stamps_[0]->time_ << "/"
            << (*a)->time_ - last << ")" << std::endl;
      last = (*a)->time_;
    }
  }

 private:
  const std::string name_;
  std::vector<TimeStampedEvent*> stamps_;
  std::ostream* file_;
};

#endif
