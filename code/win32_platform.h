#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global direct_sound_create *directSoundCreateProc;

struct File_content
{
  u32  size;
  void *memory;
};

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
