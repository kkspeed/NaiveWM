#include "wm/manage/split_exec.h"

#include <cassert>
#include <cstring>

#include "base/logging.h"

namespace naive {
namespace wm {

namespace {

bool IsNumber(char i) {
  return i >= '0' && i < '9';
}

}  // namespace

SplitExec::SplitExec(const std::string& split_string,
                     const base::geometry::Rect& rect,
                     int32_t size)
    : split_string_(split_string), workspace_(rect), size_(size) {}

base::geometry::Rect SplitExec::NextRect(int32_t i) {
  if (index_ >= split_string_.size() || i == size_ - 1) {
    return workspace_;
  }
  float partition = 0.0f;
  base::geometry::Rect result(workspace_);
  if (split_string_[index_] == '|') {
    partition = GetPartitionPercentage();
    result.width_ = partition * workspace_.width();
    workspace_.x_ += result.width_;
    workspace_.width_ -= result.width_;
    return result;
  }

  if (split_string_[index_] == '-') {
    partition = GetPartitionPercentage();
    result.height_ = partition * workspace_.height();
    workspace_.y_ += result.height_;
    workspace_.height_ -= result.height_;
    return result;
  }

  if (split_string_[index_] == ')' || split_string_[index_] == '(') {
    index_ += 1;
  } else if (split_string_[index_] == '+') {
    assert(index_ > 0);
    index_--;
    if (split_string_[index_] == ')') {
      GotoLastMatchParen();
    }
  }
  return NextRect(i);
}

float SplitExec::GetPartitionPercentage() {
  int32_t j = index_ + 1;
  if (j >= split_string_.size() || !IsNumber(split_string_[j])) {
    index_ = j;
    return 0.5f;
  }

  std::string tmp;
  while (j < split_string_.size() && IsNumber(split_string_[j])) {
    tmp += split_string_[j++];
  }
  index_ = j;
  return std::atoi(tmp.c_str()) / 100.0f;
}

void SplitExec::GotoLastMatchParen() {
  int cnt = 0;
  while (true) {
    assert(index_ >= 0);
    if (split_string_[index_] == ')')
      cnt++;
    if (split_string_[index_] == '(')
      cnt--;
    if (cnt == 0)
      return;
    index_--;
  }
}

}  // namespace wm
}  // namespace naive
