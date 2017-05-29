/******************************************************************************
File:   win32_projecth.cpp
Author: GyuHyeon Lee
Email:  email: evulstudio\@gmail.com
Date:   24/05/2017
Info:   This contains windows platform layer.

Notice: (C) Copyright 2017 by GyuHyeon, Lee. All Rights Reserved. $
******************************************************************************/
#pragma warning (disable : 4459)

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct win32_window_dimension
{
    int width;
    int height;  
};

global_variable bool gRunning;
global_variable win32_offscreen_buffer gBackBuffer;

win32_window_dimension Win32GetWindowDimension(HWND window)
{
    win32_window_dimension result;

    /*
        structure RECT
        left, top, right, bottom
    */
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;

    return result;
}
    
void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;

    int bitmapMemorySize = buffer->width * buffer->height * bytesPerPixel;
    buffer->memory = VirtualAlloc(buffer->memory, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixel;
}

void Win32DisplayBuffer(HDC deviceContext, 
                        int windowWidth, int windowHeight,
                        win32_offscreen_buffer *buffer)
{
    StretchDIBits(deviceContext, 
                    0, 0, windowWidth, windowHeight,
                    0, 0, buffer->width, buffer->height,
                    buffer->memory,
                    &buffer->info,
                    DIB_RGB_COLORS, SRCCOPY);
}

void RenderWeirdScreen(int xOffset, int yOffset,
                        win32_offscreen_buffer *buffer)
{
    //Because each byte is 8 bytes, we have to cast it.
    //Also because we want to increae it by
    //row += buffer->pitch
    uint8 *row = (uint8 *)buffer->memory;
    for(int y = 0;
        y < buffer->height;
        ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = 0;
            x < buffer->width;
            ++x)
        {
            uint8 blue = (uint8)(x + xOffset);
            uint8 green = (uint8)(y + yOffset);

            *pixel++ = ((green << 8) | blue);
        }
        row += buffer->pitch;
    }
}

LRESULT CALLBACK MainWindowProc(HWND hWnd,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam)
{
    LRESULT result = 0;
    
    switch(uMsg)
    {
        //Whenever the user presses the close button.
        case WM_CLOSE:
        {
            gRunning = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        //whenever the window is destroyed.
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");            
        } break;

        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");  
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");  
        } break;

        case WM_PAINT:
        {
            /*****
            typedef struct tagPAINTSTRUCT {
                HDC  hdc;
                BOOL fErase; //whether the background should be deleted or not
                RECT rcPaint; //upper left and lower right of the rectangle in which the painting is requested.
                BOOL fRestore; //reserved.
                BOOL fIncUpdate; //reserved.
                BYTE rgbReserved[32]; //reserved.
                } PAINTSTRUCT, *PPAINTSTRUCT;
            *****/
            PAINTSTRUCT paintStruct;
            HDC deviceContext = BeginPaint(hWnd, &paintStruct);

            win32_window_dimension dimension = Win32GetWindowDimension(hWnd);
            Win32DisplayBuffer(deviceContext, dimension.width, dimension.height, &gBackBuffer);
            EndPaint(hWnd, &paintStruct);
        }
        default:
        {
            result = DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    return result;
}//LRESULT CALLBACK MainWindowProc

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE HPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
    /*Make window class*/
    WNDCLASSEXA windowClass;

    Win32ResizeDIBSection(&gBackBuffer, 1280, 720);

    windowClass.cbSize = sizeof(windowClass); //size of this structure
    windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC; //what will be the style of the created window?
    windowClass.lpfnWndProc = MainWindowProc; //Function Pointer to the Windows Procedure function
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "ProjectHWindowClass"; //Need for the register & createwindow function below!
    //Not needed or initially initialized to zero
    windowClass.cbClsExtra = 0; //
    windowClass.hIcon = 0; //Window Icon
    windowClass.hIconSm = 0; //Window Icon Small    
    windowClass.hCursor = 0; //Cursor Icon
    windowClass.hbrBackground = 0; //BackGround
    windowClass.lpszMenuName = 0; //Menu Name

    /*Register window class*/
    if(RegisterClassExA(&windowClass))
    {
        /****************
        HWND WINAPI CreateWindowEx(
            DWORD     dwExStyle, //extended window style
            LPCTSTR   lpClassName, //name of the window class
            LPCTSTR   lpWindowName, //name of the window itself
            DWORD     dwStyle, //window style
            int       x, //x position
            int       y, //y position
            int       nWidth, //width of the window
            int       nHeight, //height of the window
            HWND      hWndParent, //pointer to the parent window
            HMENU     hMenu, //handle to a menu
            HINSTANCE hInstance, 
            LPVOID    lpParam);
        ****************/
        HWND hWnd = 
            CreateWindowExA(
                0,
                windowClass.lpszClassName,
                "ProjectH",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
        if(hWnd)
        {
            int xOffset = 0;
            int yOffset = 0;
            HDC deviceContext = GetDC(hWnd);
            gRunning = true;
            //infinite message loop
            while(gRunning)
            {
                MSG msg;
                while(PeekMessageA(&msg, hWnd, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                RenderWeirdScreen(xOffset, yOffset, &gBackBuffer);
                win32_window_dimension dimension = Win32GetWindowDimension(hWnd);
                Win32DisplayBuffer(deviceContext, dimension.width, dimension.height, &gBackBuffer);
            }
        }
        else
        {
            OutputDebugStringA("creating window failed.");
            //creating window failed.
        }//if(windowHandle)
    }
    else
    {
        OutputDebugStringA("Registering Windows Class Failed.");        
        //Registering Windows Class Failed.
    }//RegisterClassExA()
    return 0;
}//int CALLBACK WinMain