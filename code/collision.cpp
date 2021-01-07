struct Segment
{
    v2 P0;
    v2 P1;
};

bool
checkCollisionAABB(v2 obj1Min, v2 obj1Max,
                   v2 obj2Min, v2 obj2Max)
{
    bool result = false;
    
    bool intersectionX = obj1Max.x >= obj2Min.x && obj1Min.x <= obj2Max.x;
    bool intersectionY = obj1Max.y >= obj2Min.y && obj1Min.y <= obj2Max.y;
    
    result = intersectionX && intersectionY;
    
    return result;
}

bool
ballBrickIntersection(Game_State *gameState,
                      Brick *brick, Ball *currentBall, v2 *newBallP)
{
    bool result = false;
    f32 Ydp = newBallP->y - currentBall->pos.y;
    if(Ydp != 0)
    {
        f32 collisionP = 0;
        if(currentBall->velocity.y > 0)
        {
            collisionP = brick->pos.y - gameState->ballSize.y;
        }
        else
        {
            collisionP = brick->pos.y + gameState->brickSize.y;
        }
        f32 ty = (collisionP - currentBall->pos.y) / Ydp;
        
        if(((ty >= 0.0f) && (ty <= 1.0f)))
        {
            f32 tx = (1.0f-ty)*currentBall->pos.x + ty*newBallP->x;
            
            if((tx + gameState->ballSize.x > brick->pos.x) &&
               (tx < gameState->brickSize.x-1.0f + brick->pos.x))
            {
                newBallP->y = currentBall->pos.y+ty*(newBallP->y - currentBall->pos.y);
                currentBall->velocity.y *= -1.0f;
                return true;
            }
        }
    }
    
    f32 Xdp = newBallP->x - currentBall->pos.x;
    if((Xdp != 0))
    {
        f32 collisionP = 0;
        if(currentBall->velocity.x > 0)
        {
            collisionP = brick->pos.x - gameState->ballSize.x;
        }
        else
        {
            collisionP = brick->pos.x + gameState->brickSize.x;
        }
        f32 tx = (collisionP - currentBall->pos.x) / Xdp;
        
        if(((tx >= 0.0f) && (tx <= 1.0f)))
        {
            f32 ty = (1.0f-tx)*currentBall->pos.y + tx*newBallP->y;
            if((ty + gameState->ballSize.y > brick->pos.y) &&
               (ty < gameState->brickSize.y + brick->pos.y))
            {
                newBallP->x = (1.0f-tx)*currentBall->pos.x + tx*newBallP->x;
                currentBall->velocity.x *= -1.0f;
                return true;
            }
        }
    }
    return result;
}


bool
ballBrickIntersectionCompressed(Game_State *gameState,
                                Brick *brick, Ball *currentBall, v2 *newBallP)
{
    bool result = false;
    for(u32 axisIndex = 0; axisIndex < 2; axisIndex++)
    {
        f32 dp = newBallP->e[axisIndex] - currentBall->pos.e[axisIndex];
        if(dp != 0)
        {
            f32 collisionP = 0;
            if(currentBall->velocity.e[axisIndex] > 0)
            {
                collisionP = brick->pos.e[axisIndex] - gameState->ballSize.e[axisIndex];
            }
            else
            {
                collisionP = brick->pos.e[axisIndex] + gameState->brickSize.e[axisIndex];
            }
            
            f32 t0 = (collisionP - currentBall->pos.e[axisIndex]) / dp;
            if((t0 >= 0.0f) && (t0 <= 1.0f))
            {
                f32 t1 = 0;
                bool col;
                if(axisIndex == 0)
                {
                    
                    t1 = (1.0f-t0)*currentBall->pos.y + t0*newBallP->y;
                    col = (t1 + gameState->ballSize.e[1] > brick->pos.e[1]) &&
                        (t1 < gameState->brickSize.e[1] + brick->pos.e[1]);
                }
                else
                {
                    
                    t1 = (1.0f-t0)*currentBall->pos.x + t0*newBallP->x;
                    col = (t1 + gameState->ballSize.e[0] > brick->pos.e[0]) &&
                        (t1 < gameState->brickSize.e[0] + brick->pos.e[0]);
                }
                
                if(col)
                {
                    newBallP->e[axisIndex] = currentBall->pos.e[axisIndex]+t0*(newBallP->e[axisIndex] - currentBall->pos.e[axisIndex]);
                    
                    currentBall->velocity.e[axisIndex] *= -1.0f;
                    return true;
                }
            }
        }
    }
    
    return result;
}


