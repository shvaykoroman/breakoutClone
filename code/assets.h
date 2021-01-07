/* date = December 29th 2020 11:18 pm */

#ifndef ASSETS_H
#define ASSETS_H


#pragma pack(push,1)

struct Bitmap_info
{  
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;  
    u32 bitmapOffset;
    
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bitsPerPixel;
    
    u32 compression;
    u32 sizeOfBitmap;
    s32 horzResolution;
    s32 vertResolution;
    u32 colorUsed;
    u32 colorsImportant;
    
    
    u32 redMask;       /* Mask identifying bits of red component */
    u32 greenMask;     /* Mask identifying bits of green component */
    u32 blueMask;      /* Mask identifying bits of blue component */
    u32 alphaMask;     /* Mask identifying bits of alpha component */
};

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


#endif //ASSETS_H
