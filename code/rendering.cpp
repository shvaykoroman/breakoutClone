

Loaded_bitmap
createEmptyBitmap(Memory_arena *arena,s32 width, s32 height)
{
    Loaded_bitmap result = {};
    
    result.width = width;
    result.height = height;
    
    result.stride = width * 4;
    s32 totalBitmapSize = width * height * 4;
    result.memory = _pushSize(arena,totalBitmapSize);
    zeroSize(totalBitmapSize,result.memory);
    
    return result;
}

internal void
drawSprite(Game_Framebuffer *framebuffer,Loaded_bitmap bitmap, v2 pos)
{
    
    s32 positionX    = roundFromFloatToInt(pos.x);
    s32 positionY    = roundFromFloatToInt(pos.y);
    s32 maxPositionX = roundFromFloatToInt((f32)bitmap.width  + pos.x);
    s32 maxPositionY = roundFromFloatToInt((f32)bitmap.height + pos.y);
    
    if( positionX <= 0)
    {
        positionX  = 0;
    }
    if(positionY <= 0)
    {
        positionY = 0;
    }
    if(maxPositionX >= framebuffer->width)
    {
        maxPositionX = framebuffer->width;
    }
    if(maxPositionY >= framebuffer->height)
    {
        maxPositionY = framebuffer->height;
    }  
    
    u8 *destRow = (u8*)framebuffer->memory + (positionX * 4) + (positionY * framebuffer->stride);
    u32 *sourceRow =  (u32*)bitmap.memory;
    for(s32 y = positionY; y < maxPositionY; ++y)
    {
        u32 *dest   = (u32*)destRow;
        u32 *source = sourceRow;
        for(s32 x = positionX; x < maxPositionX; ++x)
        {
            *dest++ = *source++;
        }
        destRow -= framebuffer->stride;
        sourceRow += bitmap.width;
    }  
}

// NOTE(shvayko): font proccessing with stb library
// TODO(shvayko): procces font through win32
internal void
drawText(Game_Framebuffer *framebuffer,Transient_state *transState, char *text, v2 pos,f32 size)
{
    Temp_memory textMemory = beginTempMemory(&transState->transArena);
    f32 scaleX = 25.0f;
    local_persist bool isInit = false;
    local_persist stbtt_fontinfo font;
    local_persist File_content loadedFont;
    s32 width, height ,xOffset, yOffset;      
    if(!isInit)
    {
        loadedFont = readFile("C:/windows/fonts/consola.ttf");
        stbtt_InitFont(&font, (u8*)loadedFont.memory, stbtt_GetFontOffsetForIndex((u8*)loadedFont.memory,0));
        
        isInit = true;
    }
    
    for(char *at = text; *at; at++)
    {
        u8 *monoBitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, size),
                                                  *at,&width,&height,&xOffset,&yOffset);
        Loaded_bitmap result = createEmptyBitmap(textMemory.arena, width,height);
        
        f32 lineHeight = 64;
        f32 scale = stbtt_ScaleForPixelHeight(&font, lineHeight);
        
        
        s32 ascent = 0;
        s32 descent = 0;
        s32 lineGap = 0;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
        
        ascent  = roundFromFloatToInt(scale * ascent);
        descent = roundFromFloatToInt(scale * descent);
        
        //STBTT_DEF void stbtt_GetCodepointHMetrics(&font, , int *advanceWidth, int *leftSideBearing);
        
        
        u8 *source = monoBitmap;
        u8 *destRow = (u8*)result.memory + (height - 1)*result.stride;
        
        for(s32 y = 0; y < height; y++)
        {
            u32 *dest = (u32*)destRow;
            for(s32 x = 0; x < width; x++)
            {
                u8 alpha = *source++;
                *dest++ = ((alpha << 24) |
                           (alpha << 16) | 
                           (alpha << 8) |
                           (alpha << 0));
            }
            destRow -= result.stride;	  
        }
        
        pos.x += scaleX;
        drawSprite(framebuffer,result,pos);
        stbtt_FreeBitmap(monoBitmap, 0);      
    }
    
    endTempMemory(textMemory);
    checkArena(&transState->transArena);
}

internal void
drawTextWithNum(Transient_state *transState, Game_Framebuffer *framebuffer, v2 pos,
                char *srcText, s64 value, f32 size)
{
    char dstText[100];
    snprintf(dstText, sizeof(dstText),"%s:%lld", srcText,value);
    drawText(framebuffer,transState, dstText, pos, size);
}


void
clearBackbuffer(Game_Framebuffer *framebuffer)
{
    memset(framebuffer->memory,0,framebuffer->width * framebuffer->height * 4);
}

internal void 
drawRectangle(Game_Framebuffer *framebuffer,v2 minP,v2 maxP, v3 color)
{
    BEGIN_TIME_BLOCK(drawRectangle);
    s32 xMin = (s32)minP.x;
    s32 yMin = (s32)minP.y;
    s32 xMax = xMin + (s32)maxP.x;
    s32 yMax = yMin + (s32)maxP.y;
    
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
    
    u32 colorSrc = (((u32)(color.r*255.0f) << 16) |
                    ((u32)(color.g*255.0f) << 8) |
                    ((u32)(color.b*255.0f) << 0));
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
    END_TIME_BLOCK(drawRectangle);
}


internal void
drawCircle(Game_Framebuffer *framebuffer, v2 center, f32 radius, v3 color)
{
	BEGIN_TIME_BLOCK(drawCircle);

	s32 xMin = roundFromFloatToInt(center.x - radius);
	s32 yMin = roundFromFloatToInt(center.y - radius);

	s32 xMax = roundFromFloatToInt(center.x + radius);
	s32 yMax = roundFromFloatToInt(center.y + radius);

	u32 colorSrc = (((u32)(color.r*255.0f) << 16) |
		((u32)(color.g*255.0f) << 8) |
		((u32)(color.b*255.0f) << 0));
	u8 *row = (u8*)framebuffer->memory + (xMin * 4) + (yMin * framebuffer->stride);

	f32 sqRadius = sq(radius);

	for (s32 y = yMin; y < yMax; y++)
	{
		u32 *pixel = (u32*)row;
		for (s32 x = xMin; x < xMax; x++)
		{
			f32 distX = (center.x - x + 0.5f);
			f32 distY = (center.y - y + 0.5f);
			f32 dist = sq(distX) + sq(distY);

			if (dist <= sqRadius)
			{
				pixel[x - xMin] = colorSrc;
			}
		}
		row += framebuffer->stride;
	}
	END_TIME_BLOCK(drawCircle);
}
