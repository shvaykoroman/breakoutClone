#include <stdint.h>

#define PI 3.14159265359f

#define global static 
#define local_persist static
#define internal static


typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64; 

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64; 

typedef float    f32;
typedef double   f64;

#define KILOBYTES(x) (1024LL*x)
#define MEGABYTES(x) (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x) (MEGABYTES(x) * 1024LL)

#define assert(expr); if(!(expr)) {*(s32*)0 = 0;}
#define arrayCount(array) (sizeof(array) / sizeof(array[0])) 

#define MAX_BRICKS 200

struct Game_Framebuffer
{
  s32 width;
  s32 height;
  s32 stride;

  void *memory;
};

struct Button_Status
{
  bool isDown;
  bool changed;
};

struct Keyboard
{
  union
  {
    struct
    {
      Button_Status buttonUp;
      Button_Status buttonDown;
      Button_Status buttonLeft;
      Button_Status buttonRight;
      Button_Status buttonArrowLeft;
      Button_Status buttonArrowRight;
      Button_Status buttonArrowUp;
      Button_Status buttonArrowDown;
      
    };
    Button_Status buttons[8];
  };  
};
struct Input
{
  f32 dtForFrame;
  Keyboard controller;
};

struct Game_Memory
{
  void *permanentStorage;
  s64 permanentStorageSize;

  void *transientStorage;
  s64 transientStorageSize;
  
};

typedef struct v2
{  
  union
  {
    f32 vec2[2];
    struct
    {
      f32 x;
      f32 y;
    };
  };
}v2;

#define v2(x,y) v2_create(x,y)

v2 v2_create(f32 x, f32 y)
{
  v2 result;

  result = {x,y};
  
  return result;
}

typedef struct v3
{  
  union
  {
    struct
    {
      f32 r;
      f32 g;
      f32 b;
    };
    struct
    {
      f32 x;
      f32 y;
      f32 z;
    };
  };
}v3;

#define v3(x,y,z) v3_create(x,y,z)

v3 v3_create(f32 x, f32 y, f32 z)
{
  v3 result;

  result = {x,y,z};
  
  return result;
}

inline v2
operator+(v2 x, v2 y)
{
  v2 result = v2(0,0);

  result.x = x.x+y.x;
  result.y = x.y+y.y;
  
  return result;
}

inline v2
operator-(v2 x, v2 y)
{
  v2 result = v2(0,0);

  result.x = x.x-y.x;
  result.y = x.y-y.y;

  return result;
}

inline v2
operator*(v2 vec,f32 value)
{
  v2 result = v2(0,0);

  result.x = vec.x * value;
  result.y = vec.y * value;
  
  return result;
}

inline f32
dotProduct(v2 vecA, v2 vecB)
{
  f32 result = 0;

  result = (vecA.x*vecB.x) + (vecA.y * vecB.y);
  
  return result;
}

#include <math.h>

inline v2
normalizeVector(v2 x)
{
  v2 result = v2(0.0f, 0.0f);
  f64 length = sqrt(x.x * x.x + x.y * x.y);
  result.x = (f32)(x.x / length);
  result.y = (f32)(x.y / length);
  return result;  
}


struct File_content
{
  u32  size;
  void *memory;
};

struct Game_sound_output
{
  s16 *samples;
  u32 samplesToOutput;
  u32 samplesPerSec;
};


struct Loaded_sound
{
  u32 sampleCount;
  u32 channelCount;
  s16 *samples[2];
};

struct Playing_sound
{
  f32 volume[2];
  u32 samplesPlayed;
  Loaded_sound loadedSound;
  Playing_sound *next;  
};


struct Brick
{
  v3 color;
  v2 pos;   

  bool destroyed;
};


struct Level
{
  u32 width;
  u32 height;
  u32 bricksCount;
  Brick bricks[MAX_BRICKS];
  
  u8 *map;  
};


struct Memory_arena
{
  u8 *base;
  size_t size;
  size_t used;

  s32 tempCount;
};

struct Transient_state
{
  Memory_arena transArena;
  bool isInit;
};


struct Temp_memory
{
  Memory_arena *arena;
  size_t used;
};


struct Ball
{
  v2 pos;
  v2 size;
  v2 velocity;
};  

struct Player
{
  v3 color;
  v2 pos;
  v2 size;
};

enum Powerup_type
{
 powerup_increasingPaddleSize,
 powerup_doublePoints,
 powerup_addingBalls
};


struct Powerup
{
  v2 startPos;
  
  Powerup_type type;
};


struct Game_State
{
  Memory_arena levelArena;
  Memory_arena transArena;
  
  f32 brickWidth;
  f32 brickHeight;
  f32 ballWidth;
  f32 ballHeight;
  s32 bricksCount;
  
  Level currentLevel;

  u32 nextPowerup;
  Powerup powerup;


  Loaded_sound bloop;
  Playing_sound *firstPlayingSound;
  Playing_sound *firstFreePlayingSound;
  
  bool isInit;
};


File_content readFile(char *filename);
void gameUpdateAndRender(Game_Framebuffer *framebuffer, Input *input,Game_Memory *gameMemory); 
void gameGetSoundSamples(Game_Memory *gameMemory, Game_sound_output *gameSoundBuffer);
