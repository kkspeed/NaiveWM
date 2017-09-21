#ifndef WM_MANAGE_SPLIT_EXEC_H_
#define WM_MANAGE_SPLIT_EXEC_H_

#include <cstdint>
#include <string>

#include "base/geometry.h"

namespace naive {
namespace wm {

class SplitExec {
 public:
  SplitExec(const std::string& split_string,
            const base::geometry::Rect& rect,
            int32_t size);

  base::geometry::Rect NextRect(int32_t i);

 private:
  float GetPartitionPercentage();
  void GotoLastMatchParen();

  std::string split_string_;
  base::geometry::Rect workspace_;
  int32_t size_;

  int32_t index_ = 0;
};

}  // namespace wm
}  // namespace naive

#endif  // WM_MANAGE_SPLIT_EXEC_H_
