internal Loaded_bitmap
loadBitmap(char *filename)
{
    Loaded_bitmap result = {};
    File_content fileContent = readFile(filename);
    if(fileContent.memory)
    {
        Bitmap_info *content = (Bitmap_info*)fileContent.memory;
        result.width  = content->width;
        result.height = content->height;
        result.memory = (u32*)((u8*)fileContent.memory + content->bitmapOffset);
        
        //NOTE(shvayko): reversing byte order for bmp format!      
        u32 *source = (u32*)result.memory;
        for(s32 y = 0; y < result.height;y++)
        {
            for(s32 x = 0; x < result.width; x++)
            {
                *source = (*source >> 0) | (*source << 24);
                source++;
            }	  
        }
        
    }
    return result;
}


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
