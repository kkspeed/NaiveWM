#ifndef _COMPOSITOR_BUFFER_H_
#define _COMPOSITOR_BUFFER_H_

namespace naive {

class Surface;

class Buffer {
 public:
  void SetOwningSurface(Surface* surface);
};

}

#endif  // _COMPOSITOR_BUFFER_H_
