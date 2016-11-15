#include <string.h>

size_t strlcpy(char *dst, const char *src, size_t dsize)
{
  int i;
  for(i = 0; i < dsize; ++i){
    dst[i] = src[i];
    if(src[i] == '\0')
      break;
  }
  return i;
}
