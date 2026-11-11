#include <FT6336.h>

// ************************* 核心修改区 *************************
// 直接修改以下宏定义的数值，即可更换对应引脚
#define TOUCH_FT6336_SDA 15
#define TOUCH_FT6336_SCL 16 
#define TOUCH_FT6336_INT -1
#define TOUCH_FT6336_RST 18
// *************************************************************

#define TOUCH_FT6336
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 320
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 480

int touch_last_x = 0, touch_last_y = 0;
unsigned short int width=0, height=0, rotation,min_x=0,max_x=0,min_y=0,max_y=0;

// 引脚宏定义会自动传递到FT6336初始化，无需修改此行
FT6336 ts = FT6336(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

void touch_init(unsigned short int w, unsigned short int h,unsigned char r)
{
  width = w; 
  height = h;
  switch (r){
    case ROTATION_NORMAL:
    case ROTATION_INVERTED:
      min_x = TOUCH_MAP_X1;
      max_x = TOUCH_MAP_X2;
      min_y = TOUCH_MAP_Y1;
      max_y = TOUCH_MAP_Y2;
      break;
    case ROTATION_LEFT:
    case ROTATION_RIGHT:
      min_x = TOUCH_MAP_Y1;
      max_x = TOUCH_MAP_Y2;
      min_y = TOUCH_MAP_X1;
      max_y = TOUCH_MAP_X2;
      break;
    default:
      break;
  }
  ts.begin();
  ts.setRotation(r);
}

bool touch_touched(void)
{
   ts.read();
  if (ts.isTouched)
  {
    touch_last_x = map(ts.points[0].x, min_x, max_x, 0, width - 1);
    touch_last_y = map(ts.points[0].y, min_y, max_y, 0, height - 1);
    return true;
  }
  else
  {
    return false;
  }
}

bool touch_has_signal(void)
{
  return true;
}

bool touch_released(void)
{
  return true;
}