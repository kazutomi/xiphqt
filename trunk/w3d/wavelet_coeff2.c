
#include <stdint.h>

#define width 0
#define height 0
#define GND 0 
#define SETCOEFF(x)
#define INPUT_8BIT(x) 0


static
void decode_8pixel (uint32_t x, uint32_t y, uint32_t z)
{
   uint8_t byte = INPUT_8BIT(x);
   uint32_t base;

   if (byte == 0)
      return;

   base = x + y * width + z * width * height;

   if (byte & 0x01)  SETCOEFF(base);
   if (byte & 0x02)  SETCOEFF(base+1);
   if (byte & 0x04)  SETCOEFF(base+width);
   if (byte & 0x08)  SETCOEFF(base+width+1);
   if (byte & 0x10)  SETCOEFF(base+width*height);
   if (byte & 0x20)  SETCOEFF(base+width*height+1);
   if (byte & 0x40)  SETCOEFF(base+width*height+width);
   if (byte & 0x80)  SETCOEFF(base+width*height+width+1);
}



#define DECODE_CHILD_CUBES(x,y,z,level)               \
   do {                                               \
      decode_cube (2*(x), 2*(y), 2*(z), level);       \
      decode_cube (2*(x)+1, 2*(y), 2*(z), level);     \
      decode_cube (2*(x), 2*(y)+1, 2*(z), level);     \
      decode_cube (2*(x), 2*(y), 2*(z)+1, level);     \
      decode_cube (2*(x)+1, 2*(y)+1, 2*(z), level);   \
      decode_cube (2*(x), 2*(y)+1, 2*(z)+1, level);   \
      decode_cube (2*(x)+1, 2*(y), 2*(z)+1, level);   \
      decode_cube (2*(x)+1, 2*(y)+1, 2*(z)+1, level); \
   } while (0)


static
void decode_cube (uint32_t x, uint32_t y, uint32_t z, uint32_t level)
{
   uint8_t byte = INPUT_8BIT(x);

   if (byte == 0)
      return;

   ++level;
   if (level == GND)
      decode_8pixel (x, y, z);

   if (byte & 0x01)  DECODE_CHILD_CUBES(x,y,z,level);
   if (byte & 0x02)  DECODE_CHILD_CUBES(x+1,y,z,level);
   if (byte & 0x04)  DECODE_CHILD_CUBES(x,y+1,z,level);
   if (byte & 0x08)  DECODE_CHILD_CUBES(x,y,z+1,level);
   if (byte & 0x10)  DECODE_CHILD_CUBES(x+1,y+1,z,level);
   if (byte & 0x20)  DECODE_CHILD_CUBES(x,y+1,z+1,level);
   if (byte & 0x40)  DECODE_CHILD_CUBES(x+1,y,z+1,level);
   if (byte & 0x80)  DECODE_CHILD_CUBES(x+1,y+1,z+1,level);
}


