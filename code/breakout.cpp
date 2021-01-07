#define BUTTON_IS_DOWN(BUTTON) (input->controller.BUTTON.isDown)
#define BUTTON_PRESSED(BUTTON) (input->controller.BUTTON.isDown && input->controller.BUTTON.changed)

#include <immintrin.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "random.h"
#include "random.cpp"
#include "rendering.h"
#include "rendering.cpp"
#include "assets.h"
#include "assets.cpp"
#include "debug.cpp"
#include "collision.cpp"
#include "audio.cpp"

#define MAX_BALLS 3 // NOTE MAX AT SCREEN IN ONE TIME TOGETHER

s32 nextBrick = 0;
global Player player;
global Ball   balls[MAX_BALLS];

internal Ball
addNewBall(v2 pos, v2 velocity)
{
    // TODO(shvayko): free list
    Ball result = {};
    
    result.pos = pos;
    result.velocity = velocity;
    result.isActive = true;
    
    return result;
}

internal void
simulatePowerups(Game_State *gameState, Input *input, Powerup_type type0)
{
    
    for(s32 powerupIndex = 0; powerupIndex < powerup_count; powerupIndex++)
    {
        s32 flag = (1 << powerupIndex); 
        if((CHECK_FLAG(gameState->powerupsFlag, flag)))	
        {
            Powerup_type type = (Powerup_type)powerupIndex;
            switch(type)
            {
                case powerup_increasingPaddleSize:
                {
                    if(gameState->increasingPaddleSizeTime > 0)
                    {
                        gameState->increasingPaddleSizeTime -= input->dtForFrame;
                    }
                    else
                    {
                        player.size.x -= 20.0f;
                        CLEAR_FLAG(gameState->powerupsFlag, flag);		    
                        gameState->increasingPaddleSizeTime = 0;
                    }
                }break;
                case powerup_doublePoints:
                {
                    if(gameState->doublePointsTime > 0)
                    {
                        gameState->doublePointsTime -= input->dtForFrame;
                    }
                    else
                    {
                        CLEAR_FLAG(gameState->powerupsFlag, flag);
                        gameState->pointsAddition = 20;
                        gameState->doublePointsTime = 0;
                    }
                }break;
                case powerup_additinonalBalls:
                {
                    if(gameState->additionalBallsTime > 0)
                    {
                        gameState->additionalBallsTime -= input->dtForFrame;
                    }
                    else
                    {
                        CLEAR_FLAG(gameState->powerupsFlag, flag);
                        gameState->additionalBallsTime = 0;
                        balls[1].isActive = false;
                        balls[2].isActive = false;
                    }
                }break;
            }
        }
    }
}

internal Powerup
addPowerup(Game_State *gameState,v2 brickPosition)
{
    Powerup *result = (gameState->currentLevel.powerups  + gameState->nextPowerup++);
    
    result->startPos = brickPosition + v2(gameState->brickSize.x * 0.5f,gameState->brickSize.y);
    Powerup_type randomType = (Powerup_type)getRandomNumberInRange(0,3);
    result->taken = false;
    result->type = randomType;
    
    assert(gameState->nextPowerup <= gameState->currentLevel.bricksCount);
    return *result;
}

struct Menu_item
{  
    v2 pos;
    char *name;  
    f32 size;
};

internal void
menu(Game_State *gameState,Game_Framebuffer *framebuffer,Transient_state *transState, Input *input)
{
    //TODO(shvayko): play menu music
    playSound(gameState,gameState->menuMusic);
    f32 center = (framebuffer->width / 2.0f) - 50.0f;
    
    local_persist s32 choice = 0;
    
    Menu_item playButton = {v2(center, 440.0f),"Play",50.0f};
    drawText(framebuffer,transState, playButton.name, playButton.pos, playButton.size);
    
    Menu_item exitButton = {v2(center, 550.0f),"Exit",50.0f};
    drawText(framebuffer,transState, exitButton.name, exitButton.pos,exitButton.size);
    
    local_persist v2 arrowPos = v2(playButton.pos.x - 50.0f,playButton.pos.y);
    if(BUTTON_PRESSED(buttonUp))
    {
        if(choice > 0) choice--;
        arrowPos = v2(playButton.pos.x - 50.0f,playButton.pos.y);
    }
    if(BUTTON_PRESSED(buttonDown))
    {
        if(choice < 1) choice++;
        arrowPos = v2(playButton.pos.x - 50.0f,playButton.pos.y + 100.0f);
    }
    // TODO(shvayko): draw arrow!
    drawSprite(framebuffer, gameState->arrowBitmap, v2(arrowPos.x,arrowPos.y));
    
    if(input->controller.buttonEnter.isDown && input->controller.buttonEnter.changed)
    {
        switch(choice)
        {
            case 0:
            {
                gameState->currentGameState = gameState_gameplay;
            }break;
            case 1:
            {
                gGameIsRunning = false;
            }break;
            default:
            {
                invalidCodePath;
            }break;
        }
    }
    
    if(BUTTON_PRESSED(buttonEscape))
    {
        gameState->currentGameState = gameState_gameplay;
        //deleteSound(gameState,gameState->menuMusic);
    }  
}

#define MAX_LEVEL_HEIGHT   10
#define MAX_LEVEL_WIDTH    20
internal void
gameSoundOutput(Game_sound_output *soundOutput,Game_Memory *gameMemory)
{
    Game_State *gameState = (Game_State*)gameMemory->permanentStorage;
}

internal void
loadLevel(Game_State *gameState, u8 *levelMap)
{
    // TODO(shvayko): delete this constants for the framebuffer width/height!
    player.pos.x = 1280.0f * .5f;
    player.pos.y = 980.0f - 20.0f;
    
    f32 ballStartingPositionX = player.pos.x;
    f32 ballStartingPositionY = player.pos.y - 200.0f;
    v2 ballPos = v2(ballStartingPositionX,ballStartingPositionY);
    v2 up = v2(0.0f,   200.0f);            
    balls[0] = addNewBall(ballPos, up); // NOTE(shvayko): first ball
    
    gameState->currentLevel.score = 0;
    memset(gameState->currentLevel.powerups,0,sizeof(Powerup) * 200);
    gameState->nextPowerup = 0;
    nextBrick = 0;  
    gameState->currentLevel.map = levelMap;
    for(u32 y = 0; y < MAX_LEVEL_HEIGHT; y++)
    {
        for(u32 x = 0; x < MAX_LEVEL_WIDTH; x++)
        {
            if(gameState->currentLevel.map[y*MAX_LEVEL_WIDTH + x] != 0)
            {
                Brick *brick = gameState->currentLevel.bricks + nextBrick++;
                assert(nextBrick < MAX_LEVEL_HEIGHT * MAX_LEVEL_WIDTH);
                v3 brickColor = v3(1.0f,0.0f,1.0f);
                if(gameState->currentLevel.map[y*MAX_LEVEL_WIDTH + x] == 1)
                {
                    brickColor = v3(1.0f,1.0f,0.0f);
                }
                brick->color = brickColor;
                brick->pos   = v2(x * gameState->brickSize.x, y* gameState->brickSize.y);
                brick->destroyed = false;
            }
        }
    }
    gameState->currentLevel.bricksCount = nextBrick;      
} 

global f32 gScreenCenterX;
global f32 gScreenCenterY;
global v2  gScreenCenter;

internal bool
checkGameOver()
{
    bool result = false;
    
    result = ((!balls[0].isActive) && (!balls[1].isActive) && (!balls[2].isActive));
    
    return result;
}

#if DEBUG
Game_Memory *debugGlobalGameMemory;
#endif
void
gameUpdateAndRender(Game_Framebuffer *framebuffer, Input *input, Game_Memory *gameMemory)
{
    
#if DEBUG
    debugGlobalGameMemory = gameMemory;
#endif
    BEGIN_TIME_BLOCK(gameUpdateAndRender);
    
    
    
    u8 levelMap2[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH] =
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0},
        {0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0},
        {0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0},
        {0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0},
        {0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0},
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
        initArena(&gameState->levelArena, gameMemory->permanentStorageSize - sizeof(*gameState),
                  (u8*)gameMemory->permanentStorage + sizeof(*gameState));                 
        
        gameState->powerupsFlag = 0x00000000;      
        
        char *bloop = "bloop_00.wav";
        gameState->bloop = loadWAVEFile(bloop);
        char *testMusic = "music_test.wav";
        gameState->menuMusic = loadWAVEFile(testMusic);
        
        gameState->ballBitmap = loadBitmap("ball.bmp");
        gameState->increaseBitmap = loadBitmap("increase.bmp");
        gameState->doublePointsBitmap = loadBitmap("points.bmp");
        gameState->arrowBitmap = loadBitmap("arrow.bmp");
        
        gameState->brickSize = v2(64.0f,21.0f);
        gameState->ballSize = v2(10.0f,10.0f);
        gameState->powerupWidth  = 25.0f;
        gameState->powerupHeight = 25.0f;
        
        gameState->currentLevel.map = firstMap;
        gameState->pointsAddition = 20;
        
        loadLevel(gameState, firstMap);      
        
        gScreenCenter = v2((f32)framebuffer->width, (f32)framebuffer->height)*0.5f;
        
        player.color = v3(1.0f,1.0f,1.0f);
        player.size  = v2(100.0f, 20.0f);
        gameState->framebufferSize = v2((f32)framebuffer->width,(f32)framebuffer->height);
        
        gameState->currentGameState = gameState_menu;
        gameState->isInit = true;
    }
    
    
    drawRectangle(framebuffer,v2(0.0f,0.0f),gameState->framebufferSize, v3(0.3f,0.3f,0.3f));
    
    assert(sizeof(Transient_state) < gameMemory->transientStorageSize);
    Transient_state *transState = (Transient_state*)gameMemory->transientStorage;
    if(!transState->isInit)
    {
        initArena(&transState->transArena, gameMemory->transientStorageSize - sizeof(Transient_state),
                  (u8*)gameMemory->transientStorage + sizeof(Transient_state));
        transState->isInit = true;
    }
    switch(gameState->currentGameState)
    {
        case gameState_gameplay:
        {	
#if DEBUG
            
            if(BUTTON_IS_DOWN(buttonArrowLeft))
            {
                
                balls[0].pos.x++;
                balls[0].velocity = v2(-200.0f,0.0f);
                
            }
            if(BUTTON_IS_DOWN(buttonArrowRight))
            {
                balls[0].pos.x--;
                balls[0].velocity = v2(200.0f,0.0f);
            }
            if(BUTTON_IS_DOWN(buttonArrowUp))
            {
                balls[0].pos.y--;
                balls[0].velocity = v2(0.0f,-200.0f);
            }
            if(BUTTON_IS_DOWN(buttonArrowDown))
            {
                balls[0].pos.y++;
                balls[0].velocity = v2(0.0f,200.0f);
            }
            
#endif  
#if 0
            {
                v2 ddPlayer = v2(0.0f,0.0f);
                if(BUTTON_IS_DOWN(buttonRight))
                {
                    ddPlayer.x = 1;
                }
                if(BUTTON_IS_DOWN(buttonLeft))
                {
                    ddPlayer.x = -1;
                }
                
                ddPlayer = ddPlayer * 128.0f;
                player.pos = player.pos + (ddPlayer * input->dtForFrame);  
            }
#else
            player.pos.x = roundFromFloatToInt(input->mouseX);
#endif
            // NOTE(shvayko): check collisions 
            for(s32 ballIndex = 0; ballIndex < MAX_BALLS; ballIndex++)
            {
                Ball *currentBall = balls + ballIndex;
                if(!currentBall->isActive) continue;
                
                v2 oldBallP = currentBall->pos;
                
                if((currentBall->pos.x + gameState->ballSize.x)  > framebuffer->width)
                {
                    currentBall->velocity.x *= -1;
                }
                if(currentBall->pos.x <= 0)
                {
                    currentBall->velocity.x *= -1;
                }
                if(currentBall->pos.y <= 0)
                {
                    currentBall->velocity.y *= -1;
                }
                if(currentBall->pos.y > framebuffer->height)
                {
                    currentBall->velocity.y *= -1;
                    currentBall->isActive = false;
                }
                v2 ballDp = (currentBall->velocity * input->dtForFrame);	    
                v2 newBallP = currentBall->pos + ballDp;
                if(checkCollisionAABB(newBallP,
                                      newBallP + gameState->ballSize, 
                                      player.pos,
                                      player.pos+player.size))
                {
                    f32 centerPlayerX = player.pos.x + (player.size.x * 0.5f);
                    f32 distance = (currentBall->pos.x+gameState->ballSize.x*0.5f) - centerPlayerX;
                    f32 t = distance / (player.size.x * 0.5f);
                    f32 strength = 16.0f;
                    v2 oldVelocity = currentBall->velocity;
                    
                    currentBall->velocity.x = 20.0f * t * strength; 
                    currentBall->velocity.y *= -1.0f; 
                }
                
                for(u32 brickIndex = 0; 
                    brickIndex < gameState->currentLevel.bricksCount; 
                    brickIndex++)
                {
                    Brick *brick = gameState->currentLevel.bricks+brickIndex;
                    if(brick->destroyed) continue; 
                    
                    if(ballBrickIntersectionCompressed(gameState, brick, currentBall, &newBallP))
                    {
                        brick->destroyed = true;
                    }
                }
                currentBall->pos = newBallP;
            }
            
            // NOTE(shvayko): render all powerups
            for(u32 powerupIndex = 0; powerupIndex < gameState->nextPowerup; powerupIndex++)
            {
                Powerup *powerup = gameState->currentLevel.powerups + powerupIndex;
                if(powerup->taken) continue; 
                powerup->startPos.y += 128 * input->dtForFrame;
                
                Loaded_bitmap currentPowerupBitmap = {};
                switch(powerup->type)
                {
                    case powerup_increasingPaddleSize:
                    {
                        currentPowerupBitmap = gameState->increaseBitmap;
                    }break;
                    case powerup_doublePoints:
                    {
                        currentPowerupBitmap = gameState->doublePointsBitmap;
                    }break;
                    case powerup_additinonalBalls:
                    {
                        currentPowerupBitmap = gameState->ballBitmap;
                    }break;
                    default:
                    {
                        invalidCodePath;
                    }
                }
                
                drawSprite(framebuffer, currentPowerupBitmap, powerup->startPos);
                
                if(checkCollisionAABB(powerup->startPos,
                                      powerup->startPos + v2(gameState->powerupWidth,gameState->powerupHeight),
                                      player.pos,
                                      player.pos + player.size))
                {
                    // NOTE(shvayko): Activation powerup
                    // NOTE(shvayko): All time in seconds.
                    switch(powerup->type)
                    {
                        case powerup_increasingPaddleSize:
                        {
                            if(!(CHECK_FLAG(gameState->powerupsFlag, INCREASE_PLAYER_SIZE)))
                            {
                                gameState->powerupsFlag = SET_FLAG(gameState->powerupsFlag, INCREASE_PLAYER_SIZE);
                                player.size.x += 20.0f;
                            }
                            gameState->increasingPaddleSizeTime += 7.0f;  
                        }break;
                        case powerup_doublePoints:
                        {
                            if(!(CHECK_FLAG(gameState->powerupsFlag, DOUBLE_POINTS)))
                            {
                                gameState->powerupsFlag = SET_FLAG(gameState->powerupsFlag, DOUBLE_POINTS);
                                gameState->pointsAddition = 40;
                            }
                            gameState->doublePointsTime += 7.0f;
                        }break;
                        case powerup_additinonalBalls:
                        {
                            if(!(CHECK_FLAG(gameState->powerupsFlag, ADDITIONAL_BALLS)))
                            {
                                gameState->powerupsFlag = SET_FLAG(gameState->powerupsFlag, ADDITIONAL_BALLS);	    
                                v2 firstAdditionalBallPos = v2(player.pos.x + player.size.x / 2,player.pos.y - 30.0f);
                                // NOTE(shvayko): When I adding balls I have already set them to "active" state
                                balls[1] = addNewBall(firstAdditionalBallPos,
                                                      v2(-200.0f,200.0f));
                                v2 secondAdditionalBallPos = v2(player.pos.x + player.size.x / 2,player.pos.y - 30.0f);
                                balls[2] = addNewBall(secondAdditionalBallPos,
                                                      v2(200.0f, 200.0f));		 
                            }		
                            gameState->additionalBallsTime += 7.0f;
                        }break;
                        default :
                        {
                            invalidCodePath;
                        }
                    }
                    
                    powerup->taken = true;
                }      
            }
            
            // NOTE(shvayko): simulation powerups
            simulatePowerups(gameState,input, powerup_increasingPaddleSize);
            // NOTE(shvayo): debugging time remaining for powerups
#if DEBUG
            drawTextWithNum(transState,framebuffer, v2(20.0f,600.0f),
                            "increasingSize",(s32)gameState->increasingPaddleSizeTime,30.0f);
            drawTextWithNum(transState,framebuffer, v2(20.0f,650.0f),
                            "doublePoints",(s32)gameState->doublePointsTime,30.0f);
            drawTextWithNum(transState,framebuffer, v2(20.0f,700.0f),
                            "additionalBalls",(s32)gameState->additionalBallsTime,30.0f);
#endif
            // NOTE(shvayko): rendering all active bricks in game
            for(u32 brickIndex = 0; brickIndex  < gameState->currentLevel.bricksCount; brickIndex++)
            {
                Brick *brick = gameState->currentLevel.bricks+brickIndex; 
                
                if(!brick->destroyed)
                {
                    drawRectangle(framebuffer, brick->pos,
                                  gameState->brickSize - 1.0f,
                                  brick->color);
                    
                }
            }   
            
            // NOTE(shvayko): test load level
#if DEBUG
            
#endif	
            for(s32 ballIndex = 0; ballIndex < MAX_BALLS; ballIndex++)
            {
                if(!(balls[ballIndex].isActive)) continue;
                drawRectangle(framebuffer, balls[ballIndex].pos,
                              gameState->ballSize,
                              v3(1.0f,0.0f,0.0f));
            }
            if(checkGameOver())
            {	    
                gameState->currentGameState = gameState_gameIsOver;
            }
            drawTextWithNum(transState,framebuffer,v2(0.0f,30.0f), "Score",gameState->currentLevel.score,30.0f);
            drawRectangle(framebuffer, player.pos, player.size, player.color);
#if DEBUG
            drawTextWithNum(transState,framebuffer,v2(0.0f,130.0f), "X", input->mouseX,30.0f);
            
            drawTextWithNum(transState,framebuffer,v2(0.0f,230.0f), "Y", input->mouseY,30.0f);
            
            handleDebugCycleCountersCounter(transState,framebuffer);
#endif	
            if(input->controller.buttonEscape.isDown && input->controller.buttonEscape.changed)
            {
                gameState->currentGameState = (Game_states)((gameState->currentGameState + 1) % gameState_count);
            }
        }break;
        case gameState_menu:
        {
            menu(gameState,framebuffer,transState,input);
        }break;
        case gameState_gameIsOver:
        {
            drawText(framebuffer,transState, "Game over", gScreenCenter - 100.0f, 50.0f);
            v2 pos = v2(gScreenCenter.x - 500.0f, gScreenCenter.y + 100.0f);
            drawText(framebuffer,transState, "Press ENTER for starting that  level again", pos, 50.0f);
            if(BUTTON_PRESSED(buttonEnter))
            {
                gameState->currentGameState = gameState_gameplay;
                loadLevel(gameState, gameState->currentLevel.map);
            }
        } break;
        default:
        {
            invalidCodePath;
        }
    }
    END_TIME_BLOCK(gameUpdateAndRender);
}


void gameGetSoundSamples(Game_Memory *gameMemory, Game_sound_output *gameSoundBuffer)
{
    Game_State *gameState = (Game_State*)gameMemory->permanentStorage; 
    Transient_state *transState = (Transient_state *)gameMemory->transientStorage;
    // TODO(shvayko): create abilitty to kill the current sound
    Temp_memory mixerMemory = beginTempMemory(&transState->transArena);
    
    f32 *realChannel0 = (f32*)pushArray(&transState->transArena, f32, gameSoundBuffer->samplesToOutput);
    f32 *realChannel1 = (f32*)pushArray(&transState->transArena, f32, gameSoundBuffer->samplesToOutput);
    
    {
        f32 *dest0 = realChannel0;
        f32 *dest1 = realChannel1;                  
        
        for(DWORD sampleIndex = 0;
            sampleIndex < gameSoundBuffer->samplesToOutput;
            sampleIndex++)
        {
            *dest0++ = 0;
            *dest1++ = 0;
        }
    }  
    // NOTE(shvayko): sound mixer
    
    for(Playing_sound **playingSoundPtr = &gameState->firstPlayingSound;
        *playingSoundPtr;)
    {
        Playing_sound *playingSound = *playingSoundPtr;
        bool soundIsFinished = false;
        if(playingSound->loadedSound.sampleCount)
        {
            f32 *dest0 = realChannel0;
            f32 *dest1 = realChannel1;
            f32 volume0 = playingSound->volume[0];
            f32 volume1 = playingSound->volume[1];
            
            u32 samplesToMix = gameSoundBuffer->samplesToOutput;
            u32 samplesRemaining = playingSound->loadedSound.sampleCount - playingSound->samplesPlayed;
            
            if(samplesToMix > samplesRemaining)
            {
                samplesToMix = samplesRemaining;
            }
            
            for(DWORD sampleIndex = playingSound->samplesPlayed;
                sampleIndex < playingSound->samplesPlayed + samplesToMix;
                sampleIndex++)
            {
                f32 sampleValue = playingSound->loadedSound.samples[0][sampleIndex];
                *dest0++ = volume0 * sampleValue;
                *dest1++ = volume1 * sampleValue;
            }
            playingSound->samplesPlayed += samplesToMix;
            soundIsFinished = (playingSound->samplesPlayed == playingSound->loadedSound.sampleCount);
        }
        // NOTE(shvayko): sound gets killed	 
        if(soundIsFinished)
        {
            *playingSoundPtr = playingSound->next;
            playingSound->next = gameState->firstFreePlayingSound;
            gameState->firstFreePlayingSound = playingSound;
        }
        else
        {
            playingSoundPtr = &playingSound->next;
        }
    }
    
    {
        f32 *source0 = realChannel0;
        f32 *source1 = realChannel1;
        s16 *sampleDest = gameSoundBuffer->samples;  
        for(DWORD sampleIndex = 0; sampleIndex < gameSoundBuffer->samplesToOutput; sampleIndex++)
        {
            *sampleDest++ = (s16)(*source0++);
            *sampleDest++ = (s16)(*source1++);
        }
    }
    endTempMemory(mixerMemory);
    checkArena(&transState->transArena);
    //gameSoundOutput(gameSoundBuffer,gameMemory);
}
