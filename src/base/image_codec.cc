#include "base/image_codec.h"

#include <png.h>
#include <cstdio>
#include <cstdlib>

#include "base/logging.h"

namespace naive {
namespace base {

void EncodePngToFile(const std::string& path,
                     std::vector<uint8_t> data,
                     int32_t width,
                     int32_t height) {
  FILE* fp;
  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  size_t x, y;
  png_byte** row_pointers = nullptr;

  int pixel_size = 4;
  int depth = 8;

  fp = fopen(path.c_str(), "wb");
  if (!fp) {
    TRACE("cannot open file %s", path.c_str());
    return;
  }

  png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr) {
    TRACE("cannot create png write struct.");
    return;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == nullptr) {
    goto png_create_info_struct_failed;
  }

  /* Set up error handling. */

  if (setjmp(png_jmpbuf(png_ptr))) {
    goto png_failure;
  }

  /* Set image attributes. */

  png_set_IHDR(png_ptr, info_ptr, width, height, depth, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  row_pointers =
      static_cast<png_byte**>(png_malloc(png_ptr, height * sizeof(png_byte*)));
  for (y = 0; y < height; y++) {
    png_byte* row = static_cast<png_byte*>(
        png_malloc(png_ptr, sizeof(uint8_t) * width * pixel_size));
    // TODO: The inversion needs to passed in as a parameter.
    row_pointers[height - y - 1] = row;
    for (x = 0; x < width; x++) {
      uint8_t* pixel = data.data() + 4 * width * y + 4 * x;
      // TODO: Use uint32_t instead to fix endian problems.
      *row++ = *pixel;
      *row++ = *(pixel + 1);
      *row++ = *(pixel + 2);
      *row++ = *(pixel + 3);
    }
  }

  png_init_io(png_ptr, fp);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

  for (y = 0; y < height; y++) {
    png_free(png_ptr, row_pointers[y]);
  }
  png_free(png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
}

}  // namespace base
}  // namespace naive
