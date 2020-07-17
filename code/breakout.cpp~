#include <immintrin.h>

#define DEBUG 1

struct Brick
{
  v3 color;
  v2 pos;   

  bool destroyed;
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
  
    
  u8 *row = (u8*)framebuffer->memory + (xMin * 4) + (yMin * framebuffer->stride);
  
  for(s32 y = yMin; y < yMax; y++)
    {
      u8 *pixel = row;  
      for(s32 x = xMin; x < xMax; x++)
	{
	  *pixel++ = (u8)color.b;
	  *pixel++ = (u8)color.g;
	  *pixel++ = (u8)color.r;
	  *pixel++;
	}
      row += framebuffer->stride;
    }  
}

struct Level
{
  u32 width;
  u32 height;  
  
  u8 *map;  
};

// TODO(shvayko)DELETE THIS FROM GLOBAL SCOPE! 
Brick bricks[128];
s32 nextBrick = 0;
global Player player;
global Ball   ball;
global f32 brickWidth  = 64.0f;
global f32 brickHeight = 21.0f;
global f32 ballWidth   = 10.0f;
global f32 ballHeight  = 10.0f;

#define levelMapHeight   10
#define levelMapWidth    20

void
gameUpdateAndRender(Game_Framebuffer *framebuffer, Input *input, Game_Memory *gameMemory)
{
  clearBackbuffer(framebuffer);

  
  u8 levelMap1[levelMapHeight][levelMapWidth] =
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

  Level level = {levelMapWidth, levelMapHeight, (u8*)levelMap1};
  
  u8 levelMap2[levelMapHeight][levelMapWidth] =
    {
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     {0,0,0,0,0,0,2,1,2,1,2,1,2,1,2,0,0,0,0,0},
     {0,0,0,0,0,0,1,2,1,2,1,2,1,2,1,0,0,0,0,0},
     {0,0,0,0,0,0,2,1,2,1,2,1,2,1,2,0,0,0,0,0},
     {0,0,0,0,0,0,1,2,1,2,1,2,1,2,1,0,0,0,0,0},
     {0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
     {0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,0,0,0,0,0},
     {0,0,0,0,0,0,2,1,2,1,2,1,2,1,2,0,0,0,0,0},
     {0,0,0,0,0,0,1,2,2,1,1,2,2,1,1,0,0,0,0,0},
    };

  Level level1 = {levelMapWidth, levelMapHeight, (u8*)levelMap2};
  
  Game_Memory *gameState = (Game_Memory*)gameMemory->permanentStorage; 
  if(!gameState->isInit)
    {            
      for(u32 y = 0; y < level1.height; y++)
	{
	  for(u32 x = 0; x < level1.width; x++)
	    {
	      if(level1.map[y*level1.width + x] != 0)
		{
		  Brick *brick = bricks + nextBrick++;
		  v3 brickColor = v3(255.0f,0.0f,255.0f);
		  if(level1.map[y*level.width + x] == 1)
		    {
		      brickColor = v3(255.0f,255.0f,0.0f);
		    }
		  brick->color = brickColor;
		  brick->pos   = v2(x * brickWidth, y* brickHeight);
		  brick->destroyed = false;
		}
	    }
	}
      
      player.pos.x = framebuffer->width / 2.0f;
      player.pos.y = framebuffer->height - 20.0f;
      player.color = v3(255.0f,255.0f,255.0f);
      player.size  = v2(100.0f, 20.0f);
      ball.pos = v2(480.0f,350.0f);//v2(player.pos.x+50.0f,player.pos.y-15.0f);
      ball.velocity = v2(0.0f, -200.0f);
      gameState->isInit = true;
    }   

  // TODO(shvaykko): only for debuggin purposes
#if DEBUG
  if(input->controller.buttonArrowLeft.isDown)
    {
      ball.pos = v2(10.0f,100.0f);
      ball.velocity = v2(200.0f,0.0f);
    }
  if(input->controller.buttonArrowRight.isDown)
    {
      ball.pos = v2((f32)framebuffer->width - 10.0f,104.0f);
      ball.velocity = v2(-200.0f,0.0f);
    }
  if(input->controller.buttonArrowUp.isDown)
    {
      ball.pos = v2(framebuffer->width / 2.0f, framebuffer->height/2.0f);
      
      ball.velocity = v2(0.0f,-200.0f);
    }
  // TODO(shvayko):doesn;t work!
  if(input->controller.buttonArrowDown.isDown)
    {
      ball.pos = v2(framebuffer->width / 2.0f, 10.0f);
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
  
  if(checkCollision(ball.pos.x, ball.pos.y, ball.pos.x + ballWidth, ball.pos.y + ballHeight,
		    player.pos.x, player.pos.y,player.pos.x+player.size.x, player.pos.y + player.size.y))
    {
      f32 centerPlayer = player.pos.x + (player.size.x / 2.0f);
      f32 distance = ball.pos.x - centerPlayer;
      f32 per = distance / (player.size.x / 2.0f);
      f32 strength = 16.0f;
      //f32 angle = dotProduct(ball.velocity, v2(player.pos.x,player.pos.y));
      v2 oldVelocity = ball.velocity;

      ball.velocity.x = 20.0f * per * strength; 
      ball.velocity.y *= -1.0f; 
    }

  // NOTE(shvayko): Ball movement

  if((ball.pos.x + ballWidth)  > framebuffer->width)
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
  for(s32 brickIndex = 0; brickIndex < 103; brickIndex++)
    {
      Brick *brick = bricks+brickIndex;
      
      if(brick->destroyed == true) continue;
      
      if(checkCollision(ball.pos.x, ball.pos.y, ball.pos.x + ballWidth, ball.pos.y + ballHeight,
		        brick->pos.x, brick->pos.y, brick->pos.x + brickWidth-1.0f, brick->pos.y+brickHeight-1.0f))
	{
	  //NOTE(shvayko):expects the values in the range of -1 to 1 and values are returned in the range 0 to PI (3.1415927)
	  	 	  
	  v2 ballPosCenter  = v2(ball.pos.x + ballWidth / 2.0f, ball.pos.y + ballHeight / 2.0f);
	  v2 brickPosCenter = v2(brick->pos.x + brickWidth / 2.0f, brick->pos.y + brickHeight / 2.0f);
	  
	  v2 ballToBrick = ballPosCenter - brickPosCenter;
	  ballToBrick    = normalizeVector(ballToBrick);
	  v2 brickFacing = v2(0.0f, -1.0f);

	  f64 value = dotProduct(ballToBrick, brickFacing);
	  
	  f64 angle = acos(value);

	  if(angle > 1.5708)
	    {
	      ball.velocity.y *= -1.0f;
	    }
	  else if(angle > 1.0f && angle < 1.1f)
	    {
	      ball.velocity.y *= -1.0f;
	    }
	  else
	    {
	      ball.velocity.x *= -1.0f;
	    }
	  
	  brick->destroyed = true;
	  // TODO(shvayko): can I to do smth better that this?
	  break; // NOTE(shvayko): I should break here cause i need to detect only 1 collison in 1 frame
	}
    }
  
  for(s32 brickIndex = 0; brickIndex < 103; brickIndex++)
    {
      Brick *brick = bricks+brickIndex; 
      
      if(!brick->destroyed)
	{
	  drawRectangle(framebuffer, brick->pos.x, brick->pos.y, brickWidth-1.0f,brickHeight-1.0f, brick->color);
	}
    }
  drawRectangle(framebuffer, ball.pos.x, ball.pos.y,ballWidth ,ballHeight, v3(255.0f,0.0f,0.0f));  
  drawRectangle(framebuffer, player.pos.x,player.pos.y, player.size.x,player.size.y, player.color);
}
