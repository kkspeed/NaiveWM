#ifndef BASE_IMAGE_CODEC_H_
#define BASE_IMAGE_CODEC_H_

#include <string>
#include <vector>

namespace naive {
namespace base {

void EncodePngToFile(
    const std::string &path,
    std::vector<uint8_t> data,
    int32_t width,
    int32_t height);

}  // namespace base
}  // namespace naive

#endif  // BASE_IMAGE_CODEC_H_
