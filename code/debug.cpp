internal void
handleDebugCycleCountersCounter(Transient_state *transState, Game_Framebuffer *framebuffer)
{
    
    f32 yOffset = 0.0f;
    char *nameTable[] = {
        "gameUpdateAndRender",
        "checkCollision",
        "drawRectangle"
    };
    
    for(s32 counterIndex = 0; counterIndex < arrayCount(debugGlobalGameMemory->debugCounter); counterIndex++)
    {
        char **counterName = nameTable+counterIndex;
        Debug_cycle_counters *cycleCounter = debugGlobalGameMemory->debugCounter + counterIndex;
        yOffset += 50.0f;
        drawTextWithNum(transState, framebuffer, v2(500.0f, 500.0f + yOffset), nameTable[counterIndex], cycleCounter->counter, 20);
        yOffset += 50.0f;
        drawTextWithNum(transState, framebuffer, v2(500.0f, 500.0f + yOffset), nameTable[counterIndex], cycleCounter->hitCount, 20);
        cycleCounter->counter = 0;
        cycleCounter->hitCount = 0;
    }  
}
