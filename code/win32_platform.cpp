#include <windows.h>
#include <Dsound.h>
#include "breakout.h"
#include "breakout.cpp"
#include "win32_platform.h"

#define WINDOW_CLASS_NAME "gameClassName"
#define WINDOW_NAME       "breakout"
#define WINDOW_WIDTH      1280
#define WINDOW_HEIGHT     980


global bool gGameIsRunning;
global Backbuffer gBackbuffer;
global LPDIRECTSOUNDBUFFER gSecondaryBuffer;
global HWND gWindowHandle;

internal void 
win32ErrorMessage(HWND window, char *message, Win32_Error_Messages type)
{
  u32 boxTypes = MB_OK;
  if(type == WIN32_FATAL_ERROR)
    {
      MessageBox(window, message, "WIN32_FATAL_ERROR", boxTypes | MB_ICONSTOP);
      ExitProcess(0);
    }
}

inline
LARGE_INTEGER getClockValue()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}

inline
f32 getDifferenceTime(LARGE_INTEGER endTime,LARGE_INTEGER beginTime)
{
    f32 result;
    
    result = (f32)(endTime.QuadPart - beginTime.QuadPart);
    
    return result;
}

internal Window_dim
getWindowDim(HWND window)
{
  Window_dim result = {};
  RECT rect;
  GetClientRect(window,&rect);

  result.width  = rect.right - rect.left;
  result.height = rect.bottom - rect.top;

  return result;
}

internal
void initAudio(HWND window, s32 secondaryBufferSize)
{
  HMODULE directSoundLib = LoadLibraryA("dsound.dll");

  if(directSoundLib)
    {
      directSoundCreateProc = (direct_sound_create*)GetProcAddress(directSoundLib, "DirectSoundCreate");

      LPDIRECTSOUNDBUFFER primaryBuffer;

      LPDIRECTSOUND directSound;

      WAVEFORMATEX waveFormat    = {};
      waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
      waveFormat.nChannels       = 2;
      waveFormat.nSamplesPerSec  = 44100;
      waveFormat.wBitsPerSample  = 16;
      waveFormat.nBlockAlign     = (WORD)(waveFormat.nChannels * waveFormat.wBitsPerSample / 8);
      waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	  
      
      if(SUCCEEDED(directSoundCreateProc(0,&directSound,0)))
	{	  	  	  
	  if(SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
	    {
	      DSBUFFERDESC bufferDesc   = {};
	      bufferDesc.dwSize         = sizeof(bufferDesc);
	      bufferDesc.dwFlags        = DSBCAPS_PRIMARYBUFFER;
	      bufferDesc.lpwfxFormat    = 0; 
	  
	      if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc,&primaryBuffer,0)))
		{	      
		  if(SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
		    {
		      OutputDebugString("primary buffer has been created\n");
		    }
		  else
		    {
		      win32ErrorMessage(gWindowHandle,
					"unable set format", WIN32_FATAL_ERROR);
		    }
		}
	      else
		{
		  win32ErrorMessage(gWindowHandle,
				    "primary buffer has not been created", WIN32_FATAL_ERROR);
		}
	    }
	  else
	    {
	      win32ErrorMessage(gWindowHandle,
				"unable set cooperative level", WIN32_FATAL_ERROR);
	    }
	  
	  DSBUFFERDESC bufferDesc   = {};
	  bufferDesc.dwSize         = sizeof(bufferDesc);
	  bufferDesc.dwFlags        = 0;
	  bufferDesc.dwBufferBytes  = secondaryBufferSize;
	  bufferDesc.lpwfxFormat    = &waveFormat; 
	  
      
	  if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc,&gSecondaryBuffer,0)))
	    {	  
	      OutputDebugString("secondary buffer has been created\n");
	    }
	  else
	    {
	      win32ErrorMessage(gWindowHandle, "secondary buffer has not been created\n", WIN32_FATAL_ERROR);
	    }
	}
      else
	{
	  win32ErrorMessage(gWindowHandle, "direct sound interface has not been set", WIN32_FATAL_ERROR);
	}
    }
  else
    {
      win32ErrorMessage(gWindowHandle, "direct sound dll haven't found", WIN32_FATAL_ERROR);
    }
}

internal void
createBackbuffer(Backbuffer *backbuffer, s32 width, s32 height)
{
  
  if(backbuffer->memory)
    {
      VirtualFree(backbuffer->memory,0,MEM_RELEASE);
    }
  
  backbuffer->width  = width;
  backbuffer->height = height;

  backbuffer->bitmapInfo.bmiHeader = {};
  
  backbuffer->bitmapInfo.bmiHeader.biSize     = sizeof(backbuffer->bitmapInfo.bmiHeader);
  backbuffer->bitmapInfo.bmiHeader.biWidth    = backbuffer->width;
  backbuffer->bitmapInfo.bmiHeader.biHeight   = -backbuffer->height;
  backbuffer->bitmapInfo.bmiHeader.biPlanes   = 1;
  backbuffer->bitmapInfo.bmiHeader.biBitCount = 32;
  backbuffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;

  s32 backbufferSize = backbuffer->width * backbuffer->height * 4;
  backbuffer->stride = backbuffer->width * 4;  
  
  backbuffer->memory = VirtualAlloc(0, (size_t)backbufferSize,MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);  
  
}

internal void
stretchBitsToScreen(HDC deviceContext, Backbuffer *backbuffer, s32 windowWidth, s32 windowHeight)
{

  
  StretchDIBits(
		deviceContext,
		0,0, windowWidth, windowHeight,
		0,0, backbuffer->width, backbuffer->height,
		backbuffer->memory,
		&backbuffer->bitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY
		);
}

#if 0
internal void
createOpenglContext(HDC deviceContext,HWND window)
{
  PIXELFORMATDESCRIPTOR pixelFormat = {};

  pixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pixelFormat.nVersion = 1;
  pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pixelFormat.iPixelType = PFD_TYPE_RGBA;
  pixelFormat.cColorBits = 24;
  pixelFormat.cStencilBits = 8;
  pixelFormat.iLayerType = PFD_MAIN_PLANE;
  
  int pixelFormatIndex = ChoosePixelFormat(deviceContext, &pixelFormat);
  if(SetPixelFormat(deviceContext, pixelFormatIndex, &pixelFormat))
    {
      HGLRC renderingContext = wglCreateContext(deviceContext);
      if(renderingContext)
	{
	  bool result = wglMakeCurrent(deviceContext, renderingContext);
	}
      else
	{
	  
	}
    }
  else
    {
      
    }
   
}
#endif

internal void
keyboardMessagesProccessing(Keyboard *input)  
{
  MSG msg;
  while(PeekMessageA(&msg,0,0,0,PM_REMOVE))
    {
      switch(msg.message)
	{
	  
	case WM_SYSKEYDOWN:
	case WM_KEYUP:	  
	case WM_KEYDOWN:
	  {
	    bool isDown  =  (msg.lParam & (1 << 31)) == 0;
	    bool wasDown =  (msg.lParam & (1 << 30)) != 0;;
	    if(isDown != wasDown)
	      {
		switch(msg.wParam)
		  {
		  case 'W':
		    {
		      input->buttonUp.isDown = isDown;
		    }break;
		  case 'A':
		    {
		      input->buttonLeft.isDown = isDown;
		    }break;
		  case 'S':
		    {
		      input->buttonDown.isDown  = isDown;
		    }break;
		  case 'D':
		    {
		      input->buttonRight.isDown = isDown;
		    }break;
		  case VK_LEFT:
		    {
		      input->buttonArrowLeft.isDown = isDown;
		    }break;
		  case VK_RIGHT:
		    {
		      input->buttonArrowRight.isDown = isDown;
		    }break;
		  case VK_DOWN:
		    {
		      input->buttonArrowDown.isDown = isDown;
		    }break;
		  case VK_UP:
		    {
		      input->buttonArrowUp.isDown = isDown;
		    }break;
		  default:
		    {
		  
		    }break;
		  }
	      }
	  }break; 
	default:
	  {
	    TranslateMessage(&msg); 
	    DispatchMessage(&msg);
	  }break;
	}
    }
}


LRESULT CALLBACK windowProc(
			    HWND   window,
			    UINT   msg,
			    WPARAM wParam,
			    LPARAM lParam
			    )
{
  LRESULT result = 0;
  
  switch(msg)
    {
    case WM_ACTIVATE:
      {

      }break;
    case WM_CREATE:
      {
      }break;
    case WM_PAINT:
      {
	PAINTSTRUCT ps;
	HDC deviceContext = BeginPaint(window, &ps);
	Window_dim dim = getWindowDim(window);
	stretchBitsToScreen(deviceContext, &gBackbuffer, dim.width, dim.height);
	EndPaint(window, &ps);
      }break;
    case WM_DESTROY:
      {
	PostQuitMessage(0);
	gGameIsRunning = false;
      }break;
    case WM_QUIT:
      {
      }break;
    default:
      {
	return DefWindowProc(window,msg,wParam,lParam);
      }break;
    }
  return result;
}

int WinMain(HINSTANCE hInstance,
	    HINSTANCE hPrevInstance,
	    LPSTR     lpCmdLine,
	    int       nShowCmd)
{
  
  LARGE_INTEGER performanceFreqRes;
  QueryPerformanceFrequency(&performanceFreqRes);
  s64 performanceFreq = performanceFreqRes.QuadPart;
  
  createBackbuffer(&gBackbuffer, WINDOW_WIDTH, WINDOW_HEIGHT);    
  WNDCLASSA windowClass = {};    
  windowClass.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc   = &windowProc;
  windowClass.hInstance     = hInstance;
  windowClass.hCursor       = LoadCursorA(0, IDC_ARROW);
  windowClass.lpszClassName = WINDOW_CLASS_NAME;

  if(RegisterClassA(&windowClass))
    {
      HWND window = CreateWindowExA(
				    0,
				    WINDOW_CLASS_NAME,
				    WINDOW_NAME,
				    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    WINDOW_WIDTH, WINDOW_HEIGHT,
				    0,
				    0,
				    hInstance,
				    0				  
				    );      
      if(window)
	{

	  Game_Memory gameMemory = {};
	  gameMemory.permanentStorageSize = MEGABYTES(64);
	  gameMemory.permanentStorage = VirtualAlloc(0, gameMemory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	  if(!gameMemory.permanentStorage)
	    {
	      assert(0)
	    }
					
	  
	  gWindowHandle = window;
	  s32 samplesPerSecond = 44100;
	  s32 secondaryBufferSize = samplesPerSecond  * sizeof(s16)*2;
	  initAudio(window, secondaryBufferSize);
	  s32 hz = 256;
	  u32 runningSampleIndex = 0;
	  s32 squareWavePeriod  = samplesPerSecond / hz;
	  s32 halfSquareWavePeriod = squareWavePeriod / 2;
	  gGameIsRunning = true;
	  HDC deviceContext = GetDC(window);	  
	  LARGE_INTEGER lastTime = getClockValue();
	  f32 targetSecondsPerFrame = 0.01666666f;
	  Input input[2] = {};
	  Input *newInput = &input[0];
	  Input *oldInput = &input[1];	  
	  while(gGameIsRunning)	    
	    {
	      newInput->dtForFrame = targetSecondsPerFrame;
	      
	      Keyboard *oldKeyboardInput = &oldInput->controller;
	      Keyboard *newKeyboardInput = &newInput->controller;
	      Keyboard zeroController = {};
	      *newKeyboardInput = zeroController;

	      for(s32 buttonIndex = 0; buttonIndex < arrayCount(newKeyboardInput->buttons); buttonIndex++)
		{
		  newKeyboardInput->buttons[buttonIndex].isDown = oldKeyboardInput->buttons[buttonIndex].isDown;
		}
	      
	      keyboardMessagesProccessing(newKeyboardInput);
#if 0	      	      
	      DWORD playCursor;
	      DWORD writeCursor;
	      
	      if(SUCCEEDED(gSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
		{
		  DWORD byteToLock = runningSampleIndex*4 % secondaryBufferSize;
		  DWORD bytesToWrite = 0;
		  if(byteToLock > playCursor)
		    {
		      bytesToWrite = (secondaryBufferSize - byteToLock);
		      bytesToWrite += playCursor;
		    }
		  else
		    {
		      bytesToWrite = playCursor - byteToLock;
		    }
	      
		  VOID *region1;
		  DWORD region1Size;
	      
		  VOID *region2;
		  DWORD region2Size;
		  
		  gSecondaryBuffer->Lock(
					 byteToLock,
					 bytesToWrite,
					 &region1,&region1Size,
					 &region2,&region2Size,
					 DSBLOCK_FROMWRITECURSOR
					 );

		  s16 *sampleOut = (s16*)region1;
		  DWORD region1SampleCount = region1Size / 4;
		  for(DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
		    {
		      s16 sampleValue = ((runningSampleIndex++ / squareWavePeriod)%2) ? 8000 : -8000;
		      *sampleOut++ = sampleValue;
		      *sampleOut++ = sampleValue;
		    }
		  
		  sampleOut = (s16*)region2;
		  DWORD region2SampleCount = region2Size / 4;
		  for(DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
		    {
		      s16 sampleValue = ((runningSampleIndex++ / squareWavePeriod) %2) ? 8000 : -8000;
		      *sampleOut++ = sampleValue;
		      *sampleOut++ = sampleValue;
		    }
		}

	      gSecondaryBuffer->Play(0, 0,DSBPLAY_LOOPING);
#endif
	      	      	      	      
	      // NOTE(shvayko): gameUpdateAndRender
	      //
	      //	      	      	      	      	      
	      Game_Framebuffer gameFramebuffer;
	      gameFramebuffer.width  = gBackbuffer.width;
	      gameFramebuffer.height = gBackbuffer.height;
	      gameFramebuffer.stride = gBackbuffer.stride;
	      gameFramebuffer.memory = gBackbuffer.memory;
	      
	      gameUpdateAndRender(&gameFramebuffer, newInput, &gameMemory);
	      
	      LARGE_INTEGER currentTime = getClockValue();
	      f32 delta = getDifferenceTime(currentTime,lastTime) / (f32)performanceFreq;	      
	      f32 secondsElapsedSoFar = delta;
	      
	      if(secondsElapsedSoFar < targetSecondsPerFrame)
		{		  
		  DWORD sleepMS = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedSoFar));
		  if(sleepMS > 0)
		    {
		      Sleep(sleepMS);
		    }		  
		  while(secondsElapsedSoFar < targetSecondsPerFrame)
		    {
		      currentTime = getClockValue();
		      secondsElapsedSoFar =  getDifferenceTime(currentTime, lastTime) / (f32)performanceFreq;
		    }
		}
	      else
		{
		  
		}
	      	      	      	      
	      Window_dim dim = getWindowDim(window);
	      stretchBitsToScreen(deviceContext, &gBackbuffer,dim.width, dim.height);	      	     
	      
	      currentTime = getClockValue();
	      delta = getDifferenceTime(currentTime,lastTime) / (f32)performanceFreq;	      	      
	      s32 ms    = (s32)(delta * 1000.f);
	      s32 FPS   = (s32)(1.f/delta);
	      lastTime = currentTime;
#if 1
	      char textBuffer[255];
	      wsprintfA(textBuffer, "ms: %d; fps: %d\n", ms, FPS);
	      OutputDebugStringA(textBuffer);
#endif

	      Input *temp = newInput;
	      newInput = oldInput;
	      oldInput = temp;
	      
	    }
	}
      else
	{
	  win32ErrorMessage(gWindowHandle, "window handle invalid", WIN32_FATAL_ERROR);
	}
    }
  else
    {
      win32ErrorMessage(gWindowHandle, "Window Class not registred", WIN32_FATAL_ERROR);
    }
  
  return 0;  
}

