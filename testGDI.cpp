
#include "dlllibrary.hpp"
//#include "gdi.cpp"
#include <ObjIdl.h>
#include <Windows.h>
#include <cassert>
#include <future>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

void msg(const char *str) { MessageBoxA(nullptr, str, "Message", 0); }
void msg(const wchar_t *str) { MessageBoxW(nullptr, str, L"Message", 0); }

int __stdcall a(int id) {
  msg(__FUNCTION__);
  return 0;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow) {
  HWND hWnd;
  MSG msg;
  WNDCLASS wndClass;

  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = WndProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = hInstance;
  wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wndClass.lpszMenuName = NULL;
  wndClass.lpszClassName = TEXT("GettingStarted");

  RegisterClass(&wndClass);

  hWnd = CreateWindow(TEXT("GettingStarted"),  // window class name
                      TEXT("Getting Started"), // window caption
                      WS_OVERLAPPEDWINDOW,     // window style
                      CW_USEDEFAULT,           // initial x position
                      CW_USEDEFAULT,           // initial y position
                      CW_USEDEFAULT,           // initial x size
                      CW_USEDEFAULT,           // initial y size
                      NULL,                    // parent window handle
                      NULL,                    // window menu handle
                      hInstance,               // program instance handle
                      NULL);                   // creation parameters

  ShowWindow(hWnd, iCmdShow);
  UpdateWindow(hWnd);

  auto dll = LoadLibraryW(L"isGDI.dll");
  assert(dll);
  auto DrawRectangle = (int(__stdcall *)(
      HWND, DWORD, int, int, int, int))GetProcAddress(dll, "DrawRectangle");
  assert(DrawRectangle);
  //  auto rect = DrawRectangle(hWnd, 0xffff0000, 0, 0, 100, 100);

  auto DrawCircle = (int(__stdcall *)(HWND, DWORD, int, int, int, int, float,
                                      float))GetProcAddress(dll, "DrawCircle");
  assert(DrawCircle);

  auto DrawArc = (int(__stdcall *)(HWND, DWORD, int, int, int, int, float,
                                   float, int))GetProcAddress(dll, "DrawArc");
  assert(DrawArc);

  auto DrawImage = (int(__stdcall *)(HWND, const char *, int, int, int,
                                     int))GetProcAddress(dll, "DrawImage");
  assert(DrawImage);
  auto rect =
      DrawImage(hWnd, R"(E:\Inno\MyProjects\Ascent\Resources\Graphics\a.png)",
                0, 0, 100, 100);

  auto SetOnMouseClick =
      (void(__stdcall *)(int32_t, int))GetProcAddress(dll, "SetOnMouseClick");
  assert(SetOnMouseClick);
  SetOnMouseClick(rect, (int)&a);

  auto UpdateDrawing =
      (void(__stdcall *)(HWND))GetProcAddress(dll, "UpdateDrawing");
  assert(UpdateDrawing);
  UpdateDrawing(hWnd);

  auto SetWidth =
      (void(__stdcall *)(int32_t id, int width))GetProcAddress(dll, "SetWidth");
  assert(SetWidth);

  auto SetHeight = (void(__stdcall *)(int32_t id, int width))GetProcAddress(
      dll, "SetHeight");
  assert(SetHeight);

  auto SetX =
      (void(__stdcall *)(int32_t id, int width))GetProcAddress(dll, "SetX");
  assert(SetX);

  auto SetY =
      (void(__stdcall *)(int32_t id, int width))GetProcAddress(dll, "SetY");
  assert(SetY);

  auto SetColor =
      (void(__stdcall *)(int32_t id, DWORD))GetProcAddress(dll, "SetColor");
  assert(SetColor);

  auto SetTransparency = (void(__stdcall *)(int32_t id, uint8_t))GetProcAddress(
      dll, "SetTransparency");
  assert(SetTransparency);

  auto SetStartAngle = (void(__stdcall *)(int32_t id, float))GetProcAddress(
      dll, "SetStartAngle");
  assert(SetStartAngle);

  auto SetSweepAngle = (void(__stdcall *)(int32_t id, float))GetProcAddress(
      dll, "SetSweepAngle");
  assert(SetSweepAngle);

  bool EndDrawing = false;
  std::future<void> f = std::async(std::launch::async, [&]() {
    while (!EndDrawing) {
      SetWidth(rect, rand() % 1000);
      SetHeight(rect, rand() % 1000);
      SetX(rect, rand() % 100);
      SetY(rect, rand() % 100);
      SetColor(rect,
               Color(rand() % 255, rand() % 255, rand() % 255, rand() % 255)
                   .GetValue());
      SetTransparency(rect, rand() % 255);
      // SetStartAngle(rect, rand() % 360);
      // SetSweepAngle(rect, rand() % 360);
      UpdateDrawing(hWnd);
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1000ms);
    }
  });

  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  EndDrawing = true;
  f.wait();
  auto Shutdown = (void(__stdcall *)())GetProcAddress(dll, "gdishutdown");
  assert(Shutdown);
  Shutdown();
  FreeLibrary(dll);

  return msg.wParam;
} // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
  case WM_PAINT:
    Graphics g(hWnd);
    Pen p(Color(255, 255, 255, 255));
    p.SetWidth(15);
    g.DrawLine(&p, 0, 0, 100, 100);
    return 0;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
} // WndProc
