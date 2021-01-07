#include <windows.h>
#include <stdio.h>
#include <Dsound.h>
#include "breakout.h"
bool gGameIsRunning;
#include "breakout.cpp"
#include "win32_platform.h"
#define WINDOW_CLASS_NAME "gameClassName"
#define WINDOW_NAME       "breakout"
#define WINDOW_WIDTH      1280
#define WINDOW_HEIGHT     980

global Backbuffer gBackbuffer;
global LPDIRECTSOUNDBUFFER gSecondaryBuffer;
global HWND gWindowHandle;
global bool globalPause;


void
freeFile(File_content *fileContent)
{
    if(fileContent->memory)
    {
        VirtualFree(fileContent->memory,0, MEM_RELEASE);
    }
}

File_content
readFile(char *filename)
{
    File_content result = {};
    HANDLE fileHandle = CreateFileA(filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
    
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        GetFileSizeEx(fileHandle, &fileSize);
        result.size = (u32)fileSize.QuadPart;
        result.memory = VirtualAlloc(0, (size_t)fileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        u32 bytesToRead = result.size;
        DWORD bytesReaden;
        if(ReadFile(fileHandle,result.memory,bytesToRead, &bytesReaden,0)
           && bytesToRead == bytesReaden)
        {
            // NOTE(shvayko): file read success
        }
        else
        {	  
            VirtualFree(result.memory,0, MEM_RELEASE);
        }
    }
    else
    {
        // TODO(shvayko): logging
    }
    return result;
}

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
        waveFormat.nSamplesPerSec  = 48000;
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
            bufferDesc.dwFlags        = DSBCAPS_GETCURRENTPOSITION2;
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

global s32 mouseX;
global s32 mouseY;

internal void
keyboardMessagesProccessing(Keyboard *input)  
{
    MSG msg;
    while(PeekMessageA(&msg,0,0,0,PM_REMOVE))
    {
        switch(msg.message)
        {
            
            case WM_LBUTTONUP:
            case WM_LBUTTONDOWN:
            {
                bool isDown = false;
                if(msg.wParam & MK_LBUTTON)
                {
                    isDown = true;
                }
                u32 mask = 0x0000FFFF;
                mouseX = msg.lParam & (mask); 
                mouseY = (msg.lParam >> 16) & (mask);
                
                input->leftMouseButton.isDown = isDown;
            }break;
            case WM_RBUTTONUP: 
            case WM_RBUTTONDOWN:
            {
                bool isDown = false;
                
                if(msg.wParam & MK_RBUTTON)
                {	
                    isDown = true;
                }	    
                input->rightMouseButton.isDown = isDown;
            }break;
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
                            input->buttonUp.changed = isDown != wasDown;
                        }break;
                        case 'A':
                        {
                            input->buttonLeft.isDown = isDown;
                            input->buttonLeft.changed = isDown != wasDown;
                        }break;
                        case 'S':
                        {
                            input->buttonDown.isDown  = isDown;
                            input->buttonDown.changed = isDown != wasDown;
                        }break;
                        case 'D':
                        {
                            input->buttonRight.isDown = isDown;
                            input->buttonRight.changed = isDown != wasDown;
                        }break;
                        case VK_LEFT:
                        {
                            input->buttonArrowLeft.isDown = isDown;
                            input->buttonArrowLeft.changed = isDown != wasDown;
                        }break;
                        case VK_RIGHT:
                        {
                            input->buttonArrowRight.isDown = isDown;
                            input->buttonArrowRight.changed = isDown != wasDown;
                        }break;
                        case VK_DOWN:
                        {
                            input->buttonArrowDown.isDown = isDown;
                            input->buttonArrowDown.changed = isDown != wasDown;
                        }break;
                        case VK_UP:
                        {
                            input->buttonArrowUp.isDown = isDown;
                            input->buttonArrowUp.changed = isDown != wasDown;
                        }break;
                        case VK_RETURN:
                        {
                            input->buttonEnter.isDown = isDown;
                            input->buttonEnter.changed = isDown != wasDown;
                        }break;
                        case VK_ESCAPE:
                        {
                            input->buttonEscape.isDown = isDown;
                            input->buttonEscape.changed = isDown != wasDown;
                        }break;
                        
#if DEBUG
                        case 'P':
                        {
                            globalPause = !globalPause;
                        }break;
#endif
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

internal void
clearSoundBuffer(Sound_output *soundOutput)
{
    VOID *region1;
    DWORD region1Size;
    
    VOID *region2;
    DWORD region2Size;
    
    gSecondaryBuffer->Lock(
                           0,
                           soundOutput->secondaryBufferSize,
                           &region1,&region1Size,
                           &region2,&region2Size,
                           0
                           );
    
    u8 *samplesDest = (u8*)region1;
    for(DWORD byteIndex = 0; byteIndex < region1Size; byteIndex++)
    {
        *samplesDest++ = 0;
    }
    
    
    samplesDest = (u8*)region2;
    for(DWORD byteIndex = 0; byteIndex < region2Size; byteIndex++)
    {    
        *samplesDest++ = 0;
    }
    gSecondaryBuffer->Unlock(region1, region1Size, region2,  region2Size);
}

internal void
fillSoundBuffer(Sound_output *soundOutput, Game_sound_output *sourceBuffer,DWORD byteToLock,DWORD bytesToWrite)
{
    
    
    VOID *region1;
    DWORD region1Size;
    
    VOID *region2;
    DWORD region2Size;
    
    gSecondaryBuffer->Lock(
                           byteToLock,
                           bytesToWrite,
                           &region1,&region1Size,
                           &region2,&region2Size,
                           0
                           );
    
    s16 *sampleSource  = sourceBuffer->samples;
    s16 *samplesDest = (s16*)region1;
    DWORD region1SampleCount = region1Size / 4;
    for(DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
    {
        *samplesDest++ = *sampleSource++;
        *samplesDest++ = *sampleSource++;
        soundOutput->runningSampleIndex++;
    }
    
    
    DWORD region2SampleCount = region2Size / 4;
    samplesDest = (s16*)region2;
    for(DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
    {    
        *samplesDest++ = *sampleSource++;
        *samplesDest++ = *sampleSource++;
        soundOutput->runningSampleIndex++;
    }
    gSecondaryBuffer->Unlock(region1, region1Size, region2,  region2Size);
}

internal void
drawDebugVerticalLine(Backbuffer *backbuffer, s32 x, s32 top, s32 bottom, u32 color)
{
    if(top <= 0)
    {
        top = 0;      
    }
    if(bottom > backbuffer->height)
    {
        bottom = backbuffer->height;
    }
    
    if((x >= 0) && (x < backbuffer->width))
    {
        u8 *pixel = (u8*)backbuffer->memory + x * 4 + top*backbuffer->stride;
        for(s32 y = top;  y < bottom; y++)
        {
            *(u32*)pixel = color;
            pixel += backbuffer->stride; 
        }
    }
    
}

internal inline void
drawSoundBufferMarker(Backbuffer *backbuffer, Sound_output *soundOutput, Debug_time_marker *markers,
                      f32 C, s32 padX, s32 top, s32 bottom, DWORD value, u32 color)
{
    s32 x = padX + (s32)(C * (f32)value);
    drawDebugVerticalLine(backbuffer, x, top, bottom, color);
}

internal void
drawDebugSyncAudioDisplay(Backbuffer *backbuffer, Sound_output *soundOutput, Debug_time_marker *markers,
                          s32 currentMarkerIndex,
                          s32 markerCount)
{
    s32 padX = 16;
    s32 padY = 16;
    
    s32 lineHeight = 64 ;
    
    f32 C = (f32)(backbuffer->width - 2*padX) / (f32)soundOutput->secondaryBufferSize;
    for(s32 markerIndex  = 0; markerIndex < markerCount; markerIndex++)
    {
        DWORD playColor  = 0xFFFFFFFF;
        DWORD writeColor = 0xFFFF0000;
        
        s32 top = padY;
        s32 bottom = padY + lineHeight;
        Debug_time_marker *thisMarker = &markers[markerIndex];
        if(markerIndex == currentMarkerIndex)
        {
            s32 firstTop = top;
            top += lineHeight+padY;
            bottom += lineHeight+padY;
            
            drawSoundBufferMarker(backbuffer, soundOutput, markers,
                                  C,  padX,  top,  bottom, thisMarker->outputPlayCursor, playColor);
            drawSoundBufferMarker(backbuffer, soundOutput, markers,
                                  C,  padX,  top,  bottom, thisMarker->outputWriteCursor, writeColor);	  
            
            top += lineHeight+padY;
            bottom += lineHeight+padY;
            
            drawSoundBufferMarker(backbuffer, soundOutput, markers,
                                  C,  padX,  top,  bottom, thisMarker->outputLocation, playColor);
            drawSoundBufferMarker(backbuffer, soundOutput, markers,
                                  C,  padX,  top,  bottom, thisMarker->outputByteCount + thisMarker->outputLocation, writeColor);
            
            top += lineHeight+padY;
            bottom += lineHeight+padY;
            
            drawSoundBufferMarker(backbuffer, soundOutput, markers,
                                  C,  padX,  firstTop,  bottom, thisMarker->expectedFlipPlayCursor, 0xFFFFFF00);
        }
        
        drawSoundBufferMarker(backbuffer, soundOutput, markers,
                              C,  padX,  top,  bottom, thisMarker->flipPlayCursor, playColor);
        drawSoundBufferMarker(backbuffer, soundOutput, markers,
                              C,  padX,  top,  bottom, thisMarker->flipWriteCursor, writeColor);
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
            gameMemory.permanentStorageSize = MEGABYTES(256);
            gameMemory.permanentStorage = VirtualAlloc(0, gameMemory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
            gameMemory.transientStorageSize = MEGABYTES(64);
            gameMemory.transientStorage = VirtualAlloc(0, gameMemory.transientStorageSize, MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
            assert(gameMemory.transientStorage);
            assert(gameMemory.permanentStorage);
            
            s32 debugTimeMarkerIndex = 0;
            Debug_time_marker debugTimeMarkers[15] = {0};
            bool soundIsValid = false;
            gGameIsRunning = true;
            gWindowHandle = window;
            
            s32 gameUpdateHZ = 60;
            f32 targetSecondsPerFrame = 1.0f/(f32)gameUpdateHZ;
            
            Sound_output soundOutput;	  
            soundOutput.samplesPerSecond = 48000;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond  * (sizeof(s16)*2);	  
            soundOutput.hz = 256;
            soundOutput.runningSampleIndex = 0;
            soundOutput.wavePeriod  = soundOutput.samplesPerSecond / soundOutput.hz;
            soundOutput.latencyCount = 4*(soundOutput.samplesPerSecond / gameUpdateHZ);
            
            DWORD audioLatencyBytes   = 0;
            f32 audioLatencySeconds  = 0;	 
            
            soundOutput.safetyBytes = (soundOutput.samplesPerSecond * 4 / gameUpdateHZ) * 2;
            s16 *soundBuffer = (s16*)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            bool soundOsPlaying = false;
            initAudio(window, soundOutput.secondaryBufferSize);
            clearSoundBuffer(&soundOutput);
            gSecondaryBuffer->Play(0, 0,DSBPLAY_LOOPING);
            
#if 0
            while(gGameIsRunning)
            {
                DWORD playCursor;
                DWORD writeCursor;
                gSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);
                
                char buffer[256];
                _snprintf_s(buffer, sizeof(buffer),"PC:%u WC:%u\n", playCursor, writeCursor);
                OutputDebugStringA(buffer);
            }
#endif
            
            bool soundIsPlaying = false;
            
            HDC deviceContext = GetDC(window);	  
            LARGE_INTEGER lastTime = getClockValue();
            LARGE_INTEGER flipWallClock = getClockValue();	  
            u64 startCycleCount = __rdtsc();
            
            Input input[2] = {};
            Input *newInput = &input[0];
            Input *oldInput = &input[1];	  
            while(gGameIsRunning)	    
            {
                POINT mousePos;
                GetCursorPos(&mousePos);
                ScreenToClient(window, &mousePos);
                newInput->mouseX = mousePos.x;
                newInput->mouseY = mousePos.y;
                
                
                //newInput->mouseX = mouseX;
                //newInput->mouseY = mouseY;	      
                
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
                
                if(!globalPause)
                {
                    Game_Framebuffer gameFramebuffer;
                    gameFramebuffer.width  = gBackbuffer.width;
                    gameFramebuffer.height = gBackbuffer.height;
                    gameFramebuffer.stride = gBackbuffer.stride;
                    gameFramebuffer.memory = gBackbuffer.memory;
                    
                    gameUpdateAndRender(&gameFramebuffer, newInput, &gameMemory);
                    
                    Debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
                    
                    LARGE_INTEGER audioWallClock = getClockValue();		 
                    f32 fromBeginToAudioSeconds = getDifferenceTime(audioWallClock,flipWallClock)
                        / (f32)performanceFreq;
                    
                    DWORD playCursor  = 0;
                    DWORD writeCursor = 0;
                    if(SUCCEEDED(gSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                    {
                        if(!soundIsValid)
                        {
                            soundOutput.runningSampleIndex = writeCursor / 4;
                            soundIsValid = true;			      
                        }		
                        
                        DWORD byteToLock = ((soundOutput.runningSampleIndex*4) % soundOutput.secondaryBufferSize);
                        
                        DWORD expectedSoundBytesPerFrame = (soundOutput.samplesPerSecond  * 4) / gameUpdateHZ;
                        f32   secondsLeftUntilFlip = targetSecondsPerFrame - fromBeginToAudioSeconds;
                        DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip/targetSecondsPerFrame)*
                                                               (f32)expectedSoundBytesPerFrame);
                        
                        DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
                        DWORD safeWriteCursor = writeCursor;
                        if(safeWriteCursor < playCursor)
                        {
                            safeWriteCursor += soundOutput.secondaryBufferSize;
                        }
                        assert(safeWriteCursor >= playCursor);
                        safeWriteCursor += soundOutput.safetyBytes;
                        
                        bool audioCardIsLowLatency = (safeWriteCursor < expectedFrameBoundaryByte);
                        DWORD targetCursor = 0;
                        if(audioCardIsLowLatency)
                        {
                            targetCursor = (expectedFrameBoundaryByte + expectedSoundBytesPerFrame);
                        }
                        else
                        {
                            targetCursor = (writeCursor + expectedSoundBytesPerFrame + soundOutput.safetyBytes);
                        }
                        targetCursor = targetCursor % soundOutput.secondaryBufferSize;
                        DWORD bytesToWrite = 0;	      
                        if(byteToLock > targetCursor)
                        {
                            bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }
                        
                        
                        Game_sound_output gameSoundOutput = {};
                        gameSoundOutput.samplesPerSec = soundOutput.samplesPerSecond;
                        gameSoundOutput.samplesToOutput = bytesToWrite / 4;
                        gameSoundOutput.samples = soundBuffer;
                        gameGetSoundSamples(&gameMemory, &gameSoundOutput);
#if 1	      	      	      
#if DEBUG
                        marker->expectedFlipPlayCursor  = expectedFrameBoundaryByte;
                        marker->outputPlayCursor  = playCursor;
                        marker->outputWriteCursor = writeCursor;
                        marker->outputLocation    = byteToLock;
                        marker->outputByteCount   = bytesToWrite;
                        
                        DWORD unwrappedWriteCursor = writeCursor;
                        if(unwrappedWriteCursor < playCursor)
                        {
                            unwrappedWriteCursor += soundOutput.secondaryBufferSize;
                        }
                        audioLatencyBytes = writeCursor - playCursor;
                        audioLatencySeconds  = ((f32)audioLatencyBytes / 4.0f) / (f32)soundOutput.samplesPerSecond;
                        
                        char buffer[256];
                        _snprintf_s(buffer, sizeof(buffer),"BTL:%u TC:%u: BTW:%u  PC:%u WC:%u DELTA:%u (%fs)\n",
                                    byteToLock, targetCursor, bytesToWrite, playCursor,
                                    writeCursor, audioLatencyBytes,audioLatencySeconds);
                        OutputDebugStringA(buffer);
                        fillSoundBuffer(&soundOutput, &gameSoundOutput, byteToLock, bytesToWrite);
                        
                    }
                    else
                    {
                        soundIsValid = false;
                    }
#endif		  		 
#endif	      	      	      	      	      
                    LARGE_INTEGER currentTime = getClockValue();
                    f32 delta = getDifferenceTime(currentTime,lastTime) / (f32)performanceFreq;	      
                    f32 secondsElapsedSoFar = delta;
                    
                    if(secondsElapsedSoFar < targetSecondsPerFrame)
                    {		  
                        DWORD sleepMS = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedSoFar));		      /*
                                                                      if(sleepMS > 0)
                                                                      {
                                                                      Sleep(sleepMS);
                                                                      }		  		      */
                        currentTime = getClockValue();
                        secondsElapsedSoFar =  getDifferenceTime(currentTime, lastTime) / (f32)performanceFreq;
                        while(secondsElapsedSoFar < targetSecondsPerFrame)
                        {
                            currentTime = getClockValue();
                            secondsElapsedSoFar =  getDifferenceTime(currentTime, lastTime) / (f32)performanceFreq;
                        }
                    }
                    else
                    {
                        
                    }
                    currentTime = getClockValue();
                    delta = getDifferenceTime(currentTime,lastTime) / (f32)performanceFreq;	      	      
                    Window_dim dim = getWindowDim(window);
                    u64 endCycleCount = __rdtsc();
                    u64 cycleCountDiff = endCycleCount - startCycleCount;
#if 0
#if DEBUG
                    drawDebugSyncAudioDisplay(&gBackbuffer,&soundOutput, debugTimeMarkers, debugTimeMarkerIndex - 1,
                                              arrayCount(debugTimeMarkers));
#endif
#endif
                    stretchBitsToScreen(deviceContext, &gBackbuffer,dim.width, dim.height);
                    
                    flipWallClock = getClockValue();
                    
#if DEBUG     	      
                    if(SUCCEEDED(gSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                    {		  
                        marker->flipPlayCursor  = playCursor;
                        marker->flipWriteCursor = writeCursor;		    }
                    
#endif	      
                    s32 ms    = (s32)(delta * 1000.f);
                    s32 FPS   = (s32)(1.f/delta);
                    lastTime = currentTime;
                    startCycleCount = endCycleCount;
#if 1
                    char textBuffer[255];
                    wsprintfA(textBuffer, "ms: %d; fps: %d cycles:%d\n", ms, FPS, cycleCountDiff);
                    OutputDebugStringA(textBuffer);
#endif
                    debugTimeMarkerIndex++;
                    if(debugTimeMarkerIndex == arrayCount(debugTimeMarkers))
                    {
                        debugTimeMarkerIndex = 0;
                    }
                    Input *temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;
                }
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

