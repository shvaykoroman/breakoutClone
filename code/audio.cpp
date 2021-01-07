internal Playing_sound*
playSound(Game_State *gameState, Loaded_sound sound)
{
    if(!gameState->firstFreePlayingSound)
    {
        gameState->firstFreePlayingSound  = (Playing_sound*)pushStruct(&gameState->levelArena, Playing_sound);
        gameState->firstFreePlayingSound->next = 0;
    }
    Playing_sound *playingSound = gameState->firstFreePlayingSound;
    gameState->firstFreePlayingSound = playingSound->next;
    
    playingSound->volume[0] = 1.0f;
    playingSound->volume[1] = 1.0f;
    playingSound->loadedSound = sound;
    
    playingSound->samplesPlayed = 0;  
    playingSound->next = gameState->firstPlayingSound;
    
    gameState->firstPlayingSound = playingSound;
    
    return playingSound;
}

internal Playing_sound*
deleteSound(Game_State *gameState, Loaded_sound sound)
{
}
