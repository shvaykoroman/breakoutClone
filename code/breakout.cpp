#include <immintrin.h>

#define DEBUG 1

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

#pragma pack(push,1)
struct WAVE_header
{
  u32 RIFFID;
  u32 size;
  u32 WAVEID;  
};

#define RIFF_CODE(a,b,c,d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
enum
{ 
 WAVE_CHUNKID_fmt  = RIFF_CODE('f','m','t',' '),
 WAVE_CHUNKID_RIFF = RIFF_CODE('R','I','F','F'),
 WAVE_CHUNKID_WAVE = RIFF_CODE('W','A','V','E'),
 WAVE_CHUNKID_data = RIFF_CODE('d','a','t','a'),
};

struct WAVE_fmt
{
  u16 wFormatTag;
  u16 nChannels;
  u32 nSamplesPerSec;
  u32 nAvgBytesPerSec;
  u16 nBlockAlign;
  u16 wBitsPerSample;
  u16 cbSize;
  u16 wValidBitsPerSample;
  u32 dwChannelMask;
  s8 SubFormat[16];  
};

struct WAVE_chunk
{
  u32 ID;
  u32 size;
};
#pragma pack(pop)


struct Riff_iterator
{
  u8 *at;
  u8 *stop;
};

inline Riff_iterator
parseChunkAt(void *at, void *stop)
{
  Riff_iterator iter;

  iter.at = (u8*)at;
  iter.stop = (u8*)stop;
  
  return iter;
}

inline Riff_iterator
nextChunk(Riff_iterator iter)
{
  WAVE_chunk *chunk = (WAVE_chunk*)iter.at;
  u32 size = (chunk->size + 1) & ~1;

  iter.at += sizeof(WAVE_chunk) + size;
  return iter;
}

inline bool
isValid(Riff_iterator iter)
{
  bool result = 0;

  result = iter.at < iter.stop;
  
  return result;
}

inline void *
getChunkData(Riff_iterator iter)
{
  void *result;
  result = iter.at + sizeof(WAVE_chunk); 
  return result;
}

inline u32
getType(Riff_iterator iter)
{
  u32 result;
  WAVE_chunk *chunk = (WAVE_chunk*)iter.at;
  result = chunk->ID;
  return result;
}


inline u32
getChunkDataSize(Riff_iterator iter)
{
  u32 result;
  WAVE_chunk *chunk = (WAVE_chunk*)iter.at;
  result = chunk->size;
  return result;
}

Loaded_sound
loadWAVEFile(char *filename)
{
  Loaded_sound result = {};

  File_content fileReadResult = readFile(filename);
  if(fileReadResult.size != 0)
    {
      WAVE_header *waveHeader = (WAVE_header*)fileReadResult.memory;

      if(waveHeader->RIFFID == WAVE_CHUNKID_RIFF &&
	 waveHeader->WAVEID == WAVE_CHUNKID_WAVE)
	{
	  u32 channelCount   = 0;
	  u32 sampleDataSize = 0;
	  s16 *sampleData   = 0;
	  for(Riff_iterator iter = parseChunkAt(waveHeader+1,(u8*)(waveHeader+1) + waveHeader->size - 4);
	      isValid(iter); iter = nextChunk(iter))
	    {
	      switch(getType(iter))
		{
		case WAVE_CHUNKID_fmt:
		  {
		    WAVE_fmt *fmt = (WAVE_fmt*)getChunkData(iter);
		    assert(fmt->wFormatTag == 1); // NOTE(shvayko): only PCM
		    //assert(fmt->nSamplesPerSec == 44100);
		    channelCount = fmt->nChannels;
		  }break;
		case WAVE_CHUNKID_data:
		  {
		    sampleData = (s16*)getChunkData(iter);
		    sampleDataSize = getChunkDataSize(iter);
		  }break;
		}
	    }
	  //assert(channelCount && sampleData);
	  result.channelCount = channelCount;	  
	  result.sampleCount = sampleDataSize / (channelCount*sizeof(s16));
	  if(channelCount == 1)
	    {
	      result.samples[0] = sampleData;
	      result.samples[1] = 0;
	    }
	  else if(channelCount == 2)
	    {
	      result.samples[0] = sampleData;
	      result.samples[1] = sampleData + result.sampleCount;

	      for(u32 sampleIndex = 0; sampleIndex < result.sampleCount;sampleIndex++)	       
		{
		  u16 source = sampleData[2*sampleIndex];
		  sampleData[2*sampleIndex] = sampleData[sampleIndex];
		  sampleData[sampleIndex] = source;
		}
	    }
	  else
	    {
	      assert(!"invalid channel count");
	    }
	  result.channelCount = 1;
	}
    }
  
  return result;
}



bool
checkCollision(f32 minx1, f32 miny1, f32 maxx1, f32 maxy1,
	       f32 minx2, f32 miny2, f32 maxx2, f32 maxy2)
{
  bool result = false;

  bool intersectionX = maxx1 >= minx2 && minx1 <= maxx2;
  bool intersectionY = maxy1 >= miny2 && miny1 <= maxy2;

  result = intersectionX && intersectionY;
  
  return result;
}

void
clearBackbuffer(Game_Framebuffer *framebuffer)
{
  memset(framebuffer->memory,0,framebuffer->width * framebuffer->height * 4);
}

internal void 
drawRectangle(Game_Framebuffer *framebuffer,f32 realX, f32 realY,f32 width , f32 height, v3 color)
{
  s32 xMin = (s32)realX;
  s32 yMin = (s32)realY;
  s32 xMax = xMin + (s32)width;
  s32 yMax = yMin + (s32)height;
  
  if(xMin <= 0)
    {
      xMin = 0;
    }
  if(yMin <= 0)
    {
      yMin = 0;
    }
  if(xMax >= framebuffer->width)
    {
      xMax = framebuffer->width;
    }
  if(yMax >= framebuffer->height)
    {
      yMax = framebuffer->height;
    }
  
  u32 colorSrc = (((u32)color.r << 16) | ((u32)color.g << 8) | ((u32)color.b << 0));
  u8 *row = (u8*)framebuffer->memory + (xMin * 4) + (yMin * framebuffer->stride);
  
  for(s32 y = yMin; y < yMax; y++)
    {
      u32 *pixel = (u32*)row;  
      for(s32 x = xMin; x < xMax; x++)
	{
	  *pixel++ = colorSrc;
	}
      row += framebuffer->stride;
    }  
}


// TODO(shvayko)DELETE THIS FROM GLOBAL SCOPE! 
s32 nextBrick = 0;
global Player player;
global Ball   ball;

#define MAX_LEVEL_HEIGHT   10
#define MAX_LEVEL_WIDTH    20

internal void
gameSoundOutput(Game_sound_output *soundOutput,Game_Memory *gameMemory)
{
  Game_State *gameState = (Game_State*)gameMemory->permanentStorage;
}

internal void
loadLevel(Game_State *gameState, u8 *level)
{
  nextBrick = 0;
  gameState->currentLevel.map = level;
  for(u32 y = 0; y < MAX_LEVEL_HEIGHT; y++)
    {
      for(u32 x = 0; x < MAX_LEVEL_WIDTH; x++)
	{
	  if(gameState->currentLevel.map[y*MAX_LEVEL_WIDTH + x] != 0)
	    {
	      Brick *brick = gameState->currentLevel.bricks + nextBrick++;
	      assert(nextBrick < MAX_LEVEL_HEIGHT * MAX_LEVEL_WIDTH);
	      v3 brickColor = v3(255.0f,0.0f,255.0f);
	      if(gameState->currentLevel.map[y*MAX_LEVEL_WIDTH + x] == 1)
		{
		  brickColor = v3(255.0f,255.0f,0.0f);
		}
	      brick->color = brickColor;
	      brick->pos   = v2(x * gameState->brickWidth, y* gameState->brickHeight);
	      brick->destroyed = false;
	    }
	}
    }
  gameState->currentLevel.bricksCount = nextBrick;
      
} 

struct Memory_arena
{
  u8 *base;
  size_t size;
  size_t used;
};

internal void
initArena(Memory_arena *arena, size_t size, u8 *base)
{
  arena->base = base;
  arena->used = 0;
  arena->size = size;
}

#define pushStruct(arena, type) _pushSize(arena,sizeof(type))
#define pushArray(arena, type, num) _pushSize(arena, sizeof(type)*num)

internal void*
_pushSize(Memory_arena *arena, size_t size)
{
  void *result = (arena->base + arena->used);
  arena->used += size;
  return result;
}

struct foo
{
  s32 bar;
  s32 foobar;
  s32 lol;
};

void
gameUpdateAndRender(Game_Framebuffer *framebuffer, Input *input, Game_Memory *gameMemory)
{
  clearBackbuffer(framebuffer);
  
  u8 levelMap2[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH] =
    {
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };

  u8 *firstMap = (u8*)levelMap2;

    u8 levelMap1[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH] =
    {
     {2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
     {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
     {1,1,2,1,1,2,1,1,1,2,1,1,2,1,1,1,2,1,1,1},
     {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
     {1,1,1,2,1,1,1,1,2,1,1,1,1,2,1,1,2,1,1,1},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,1,0,0,0,2,0,0,1,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    }; 

  u8 *secondMap = (u8*)levelMap1;
  
  
  Game_State *gameState = (Game_State*)gameMemory->permanentStorage; 
  if(!gameState->isInit)
    {

      Memory_arena levelArena;
      initArena(&levelArena, gameMemory->permanentStorageSize - sizeof(*gameState),
		(u8*)gameMemory->permanentStorage + sizeof(*gameState));      
      
      foo *Foo = (foo*)pushStruct(&levelArena, foo);
      Foo->bar = 6;
      Foo->foobar = 10;
      
      gameState->brickWidth  = 64.0f;
      gameState->brickHeight = 21.0f;
      gameState->ballWidth   = 10.0f;
      gameState->ballHeight  = 10.0f;      
      gameState->currentLevel.map = firstMap;
      
      loadLevel(gameState, gameState->currentLevel.map);      
      
      player.pos.x = framebuffer->width / 2.0f;
      player.pos.y = framebuffer->height - 20.0f;
      player.color = v3(255.0f,255.0f,255.0f);
      player.size  = v2(100.0f, 20.0f);
      f32 ballStartingPositionX = 500;
      f32 ballStartingPositionY = 600;
      ball.pos = v2(ballStartingPositionX, ballStartingPositionY);
      ball.velocity = v2(0.0f, -200.0f);
      
      gameState->testSampleIndex = 0;
      gameState->testSound = loadWAVEFile("bloop_00.wav");
      
      gameState->isInit = true;
    }   
 
#if DEBUG
  if(input->controller.buttonArrowLeft.isDown)
    {
      ball.pos.x++;
      ball.velocity = v2(-200.0f,0.0f);
    }
  if(input->controller.buttonArrowRight.isDown)
    {
      ball.pos.x--;
      ball.velocity = v2(200.0f,0.0f);
    }
  if(input->controller.buttonArrowUp.isDown)
    {
      ball.pos.y--;
      ball.velocity = v2(0.0f,-200.0f);
    }
  if(input->controller.buttonArrowDown.isDown)
    {
      ball.pos.y++;
      ball.velocity = v2(0.0f,200.0f);
    }
  
#endif  
  v2 ddPlayer = v2(0.0f,0.0f);
  if(input->controller.buttonRight.isDown)
    {
      ddPlayer.x = 1;
    }
  if(input->controller.buttonLeft.isDown)
    {
      ddPlayer.x = -1;
    }

  ddPlayer = ddPlayer * 126.0f;
  player.pos = player.pos + (ddPlayer * input->dtForFrame);  
  
  // NOTE(shvayko): collision with player paddle
  
  if(checkCollision(ball.pos.x, ball.pos.y,
		    ball.pos.x + gameState->ballWidth, ball.pos.y + gameState->ballHeight,
		    player.pos.x, player.pos.y,
		    player.pos.x+player.size.x, player.pos.y + player.size.y))
    {
      // TODO(shvayko): may be use here vector math?
      f32 centerPlayer = player.pos.x + (player.size.x / 2.0f);
      f32 distance = ball.pos.x - centerPlayer;
      f32 per = distance / (player.size.x / 2.0f);
      f32 strength = 16.0f;
      v2 oldVelocity = ball.velocity;

      ball.velocity.x = 20.0f * per * strength; 
      ball.velocity.y *= -1.0f; 
    }

  // NOTE(shvayko): Ball movement

  if((ball.pos.x + gameState->ballWidth)  > framebuffer->width)
    {
      ball.velocity.x *= -1;
    }
  if(ball.pos.x <= 0)
    {
      ball.velocity.x *= -1;
    }
  if(ball.pos.y <= 0)
    {
      ball.velocity.y *= -1;
    }
  if(ball.pos.y > framebuffer->height)
    {
      ball.velocity.y *= -1;
    }
  ball.pos = ball.pos + (ball.velocity * input->dtForFrame);
  
  
  // NOTE(shvayko): check collision for every block
  for(s32 brickIndex = 0; brickIndex < gameState->currentLevel.bricksCount; brickIndex++)
    {
      assert(nextBrick < MAX_LEVEL_HEIGHT * MAX_LEVEL_WIDTH);
      Brick *brick = gameState->currentLevel.bricks+brickIndex;
      
      if(brick->destroyed == true) continue;
      
      if(checkCollision(ball.pos.x, ball.pos.y,
			ball.pos.x + gameState->ballWidth, ball.pos.y + gameState->ballHeight,
		        brick->pos.x, brick->pos.y,
			brick->pos.x + (gameState->brickWidth-1.0f), brick->pos.y+(gameState->brickHeight-1.0f)))
	{
	  f32 ballPosCenterX = ball.pos.x + (gameState->ballWidth  / 2.0f);
	  f32 ballPosCenterY = ball.pos.y + (gameState->ballHeight / 2.0f);
	  
	  v2 ballPosCenter  = v2(ballPosCenterX, ballPosCenterY);

	  f32 brickPosCenterX = brick->pos.x + (gameState->brickWidth / 2.0f);
	  f32 brickPosCenterY = brick->pos.y + (gameState->brickHeight / 2.0f);
	  
	  v2 brickPosCenter = v2(brickPosCenterX, brickPosCenterY);
	  
	  v2 ballToBrick = ballPosCenter - brickPosCenter;
	  ballToBrick    = normalizeVector(ballToBrick);
	  v2 brickFacing = v2(0.0f, 1.0f);
	  
	  f64 value = dotProduct(ballToBrick, brickFacing);
	  // NOTE(shvayko): those numbers relative to the brick's center(bottom side is starting point of caclulatuin
	  f64 angle = acos(value);
	  if(1.19421 > angle) // NOTE(shvayko): bottom collision
	    {
	      ball.velocity.y *= -1.0f;
	    }
	  else if((1.19421 <= angle) && (angle <= 1.98769)) // NOTE(shvayko): left/right collision
	    {
	      ball.velocity.x *= -1.0f;
	    }
	  else if(angle > 1.98769) // NOTE(shvayko): up collision
	    {
	      ball.velocity.y *= -1.0f;	    
	    }
	  
	  brick->destroyed = true;
	  break;
	}
    }
  
  // NOTE(shvayko): rendering all active blocks
  for(s32 brickIndex = 0; brickIndex  < gameState->currentLevel.bricksCount; brickIndex++)
    {
      Brick *brick = gameState->currentLevel.bricks+brickIndex; 
      
      if(!brick->destroyed)
	{
	  drawRectangle(framebuffer, brick->pos.x, brick->pos.y,
			gameState->brickWidth-1.0f,gameState->brickHeight-1.0f,
			brick->color);
	}
    }

  // NOTE(shvayko): test load level
#if DEBUG
  if(input->controller.buttonArrowLeft.isDown)
    {
      loadLevel(gameState, firstMap);
    }
  
  if(input->controller.buttonArrowRight.isDown)
    {
      loadLevel(gameState, secondMap);
    }
#endif
  
  drawRectangle(framebuffer, ball.pos.x, ball.pos.y,gameState->ballWidth ,gameState->ballHeight, v3(255.0f,0.0f,0.0f));  
  drawRectangle(framebuffer, player.pos.x,player.pos.y, player.size.x,player.size.y, player.color);
}


void gameGetSoundSamples(Game_Memory *gameMemory, Game_sound_output *gameSoundBuffer)
{
  Game_State *gameState = (Game_State*)gameMemory->permanentStorage;
    
  local_persist u32 runningSampleIndex = 0;
  s32 hz = 255;
  s32 wavePeriod = gameSoundBuffer->samplesPerSec / hz;
  s16 *sampleDest = gameSoundBuffer->samples;

  for(DWORD sampleIndex = 0; sampleIndex < gameSoundBuffer->samplesToOutput; sampleIndex++)
    {      
      s16 sampleValue = gameState->testSound.samples[0][(gameState->testSampleIndex + sampleIndex)
							% gameState->testSound.sampleCount];      
      *sampleDest++ = sampleValue;
      *sampleDest++ = sampleValue;
    }
  gameState->testSampleIndex += gameSoundBuffer->samplesToOutput;

    
  //gameSoundOutput(gameSoundBuffer,gameMemory);
}
