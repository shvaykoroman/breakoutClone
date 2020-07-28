#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global direct_sound_create *directSoundCreateProc;

struct Window_dim
{
  s32 width;
  s32 height;
};

struct Backbuffer
{
  BITMAPINFO bitmapInfo;
  
  s32 width;
  s32 height;
  
  s32 stride;  
  
  void *memory;
};


enum Win32_Error_Messages
  {
   WIN32_FATAL_ERROR,
  };


struct Debug_time_marker
{
  DWORD outputPlayCursor;
  DWORD outputWriteCursor;
  DWORD outputLocation;
  DWORD outputByteCount;
  
  DWORD flipPlayCursor;
  DWORD flipWriteCursor;   
};

struct Sound_output
{
  s32 samplesPerSecond;
  DWORD secondaryBufferSize;
  DWORD safetyBytes;
  s32 hz;
  u32 runningSampleIndex;
  s32 wavePeriod;
  s32 latencyCount;
}; 
