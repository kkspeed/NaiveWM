#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

#endif  // BASE_MACROS_H_
