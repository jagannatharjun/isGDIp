#include <Windows.h>
#include <gdiplus.h>
#include <windowsx.h>
using namespace Gdiplus;
#include <objidl.h>
#pragma comment(lib, "Gdiplus.lib")

#include <functional>
#include <gupta/cleanup.hpp>
#include <gupta/format_io.hpp>
#include <limits>
#include <memory>
#include <vector>

void msg(const char *str) { MessageBoxA(nullptr, str, "Message", 0); }
void msg(const wchar_t *str) { MessageBoxW(nullptr, str, L"Message", 0); }
template <typename C> void msg(const std::basic_string<C> &s) { msg(s.c_str()); }

std::wstring toWCS(const std::string &str) {
  std::wstring res(str.size(), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), &res[0], res.size());
  return res;
}

class UniqueClass {
public:
  UniqueClass() = default;
  UniqueClass(const UniqueClass &) = delete;
  UniqueClass(const UniqueClass &&) = delete;
  UniqueClass &operator=(const UniqueClass &) = delete;
  UniqueClass &operator=(const UniqueClass &&) = delete;
};

#define DLL_EXPORT(RETURN_TYPE) extern "C" __declspec(dllexport) RETURN_TYPE __stdcall

class _GdiManager {
public:
  _GdiManager() { GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr); }
  //~_GdiManager() { GdiplusShutdown(gdiplusToken); }
  void shutdown() { GdiplusShutdown(gdiplusToken); }

private:
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
} GdiManager;

class DrawableObject {
public:
  // using notifier = int (*__stdcall)(int32_t);
  typedef int(__stdcall *notifier)(int32_t);
  DrawableObject(ARGB Color, int x, int y, int width, int height)
      : m_X{x}, m_Y{y}, m_Width{width}, m_Height{height}, m_Color{Color}, m_IsVisible(true) {}
  virtual void draw(Gdiplus::Graphics &Graphics) = 0;
  virtual ~DrawableObject() = default;

  virtual int X() const { return m_X; }

  virtual void setX(int X) { m_X = X; }

  virtual int Y() const { return m_Y; }

  virtual void setY(int Y) { m_Y = Y; }

  virtual int Width() const { return m_Width; }

  void setWidth(int Width) { m_Width = Width; }

  virtual int Height() const { return m_Height; }

  virtual void setHeight(int Height) { m_Height = Height; }

  virtual ARGB Color() const { return m_Color; }

  virtual void setColor(ARGB Color) { m_Color = Color; }

  virtual void setTransparency(uint8_t v) {
    m_Color = (m_Color & 0xff000000) | ((DWORD(v) & 0xff) << 24);
  }

  notifier OnMouseClick() const { return m_OnMouseClick; }

  void setOnMouseClick(notifier OnMouseClick) { m_OnMouseClick = OnMouseClick; }

  virtual void setIsVisible(bool v) { m_IsVisible = v; }
  virtual bool IsVisible() { return m_IsVisible; }

private:
  int m_X, m_Y, m_Width, m_Height;
  ARGB m_Color;
  notifier m_OnMouseClick = nullptr;
  bool m_IsVisible;
};

namespace DrawableObjects {
class Rectangle : public DrawableObject {
public:
  Rectangle(ARGB Color, int X, int Y, int Width, int Height)
      : DrawableObject{Color, X, Y, Width, Height}, m_Brush{Color} {}
  void draw(Gdiplus::Graphics &graphics) override {
    m_Brush.SetColor(Color());
    graphics.FillRectangle(&m_Brush, X(), Y(), Width(), Height());
  }

private:
  Gdiplus::SolidBrush m_Brush;
};

class RoundObject : public DrawableObject {
public:
  RoundObject(ARGB Color, int X, int Y, int Width, int Height, float StartAngle, float SweepAngle)
      : DrawableObject{Color, X, Y, Width, Height}, m_StartAngle{StartAngle}, m_SweepAngle{
                                                                                  SweepAngle} {}

  virtual void setStartAngle(float n) { m_StartAngle = n; }
  virtual void setSweepAngle(float n) { m_SweepAngle = n; }

  virtual float StartAngle() const { return m_StartAngle; }
  virtual float SweepAngle() const { return m_SweepAngle; }

private:
  float m_StartAngle, m_SweepAngle;
};

class Outline {
public:
  Outline(int PenWidth) : m_PenWidth{PenWidth} {}
  void setPenWidth(int a) { m_PenWidth = a; }
  int PenWidth() { return m_PenWidth; }

private:
  int m_PenWidth;
};

class Circle : public RoundObject {
public:
  Circle(ARGB Color, int X, int Y, int Width, int Height, float StartAngle, float SweepAngle)
      : RoundObject{Color, X, Y, Width, Height, StartAngle, SweepAngle}, m_Brush{Color} {}

  void draw(Gdiplus::Graphics &graphics) override {
    m_Brush.SetColor(Color());
    graphics.FillPie(&m_Brush, X(), Y(), Width(), Height(), StartAngle(), SweepAngle());
  }

private:
  Gdiplus::SolidBrush m_Brush;
};

class Arc : public RoundObject, public Outline {
public:
  Arc(ARGB Color, int X, int Y, int Width, int Height, float StartAngle, float SweepAngle,
      int PenWidth)
      : RoundObject{Color, X, Y, Width, Height, StartAngle, SweepAngle}, Outline{PenWidth},
        m_Pen{Color} {}

  void draw(Gdiplus::Graphics &graphics) override {
    m_Pen.SetColor(Color());
    m_Pen.SetWidth(PenWidth());
    graphics.DrawArc(&m_Pen, X(), Y(), Width(), Height(), StartAngle(), SweepAngle());
  }

private:
  Gdiplus::Pen m_Pen;
};

class Image : public DrawableObject {
public:
  Image(const char *FileName, int X, int Y, int Width, int Height)
      : DrawableObject{0xff000000, X, Y, Width, Height}, m_Image{std::make_unique<Gdiplus::Image>(
                                                             toWCS(FileName).c_str())} {}

  void setTransparency(uint8_t transparency) override {
    float alpha = (float)(transparency) / 255.0f;
    Gdiplus::ColorMatrix matrix = {
        {{1, 0, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, alpha, 0}, {0, 0, 0, 0, 1}}

    };
    attrib.SetColorMatrix(&matrix);
  }

  void draw(Gdiplus::Graphics &graphics) override {
    graphics.DrawImage(m_Image.get(), Gdiplus::Rect(X(), Y(), Width(), Height()), 0, 0,
                       m_Image->GetWidth(), m_Image->GetHeight(), Gdiplus::UnitPixel, &attrib);
  }

  void setImageFile(const char *FileName) {
    m_Image = std::make_unique<Gdiplus::Image>(toWCS(FileName).c_str());
  }

private:
  std::unique_ptr<Gdiplus::Image> m_Image;
  Gdiplus::ImageAttributes attrib;
};

} // namespace DrawableObjects

LRESULT MasterWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Painter : UniqueClass {
public:
  Painter(HWND hWnd)
      : m_WindowHandle{hWnd}, m_DeviceContext{GetDC(hWnd)}, m_Graphics{m_DeviceContext} {
    m_OriginalWindowProc = (WNDPROC)GetWindowLongW(m_WindowHandle, GWL_WNDPROC);
    SetWindowLongW(m_WindowHandle, GWL_WNDPROC, (LONG)MasterWindowProc);
  }

  ~Painter() {
    SetWindowLongW(m_WindowHandle, GWL_WNDPROC, (LONG)m_OriginalWindowProc);
    ReleaseDC(m_WindowHandle, m_DeviceContext);
  }

  LRESULT CallOriginalWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return CallWindowProcW(m_OriginalWindowProc, hwnd, uMsg, wParam, lParam);
  }

  void Paint(LPPAINTSTRUCT) {

    PAINTSTRUCT ps;
    auto dc = BeginPaint(m_WindowHandle, &ps);
    if (!dc)
      msg("BeginPaint Failed");
    SCOPE_EXIT { EndPaint(m_WindowHandle, &ps); };
    Graphics gdiGraphics(m_WindowHandle);
    gdiGraphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
    for (auto &o : m_Objects)
      if (o->IsVisible()) {
        o->draw(gdiGraphics);
      }
    return;
  }

  std::vector<std::unique_ptr<DrawableObject>> &Objects() { return m_Objects; }

  auto WindowHandle() const { return m_WindowHandle; }

  auto &graphics() { return m_Graphics; }

private:
  HWND m_WindowHandle;
  HDC m_DeviceContext;
  Gdiplus::Graphics m_Graphics;
  WNDPROC m_OriginalWindowProc;
  std::vector<std::unique_ptr<DrawableObject>> m_Objects;
};

std::vector<std::unique_ptr<Painter>> WindowPainters;
using SizeT = uint16_t;

uint16_t FindPainterIndex(HWND hwnd) {
  uint16_t index;
  for (index = 0; index < WindowPainters.size(); ++index) {
    if (WindowPainters[index]->WindowHandle() == hwnd) {
      return index;
    }
  }
  return index;
}

uint16_t FindPainterIndexOrConstruct(HWND hwnd) {
  auto index = FindPainterIndex(hwnd);
  return index == WindowPainters.size() ? WindowPainters.push_back(std::make_unique<Painter>(hwnd)),
         index : index;
}

auto &FindPainter(HWND hwnd) {
  auto index = FindPainterIndex(hwnd);
  if (index == WindowPainters.size())
    throw std::invalid_argument{"Invalid HWND"};
  return WindowPainters[index];
}

std::pair<std::unique_ptr<Painter> &, SizeT> FindPainterAndIndex(HWND hwnd) {
  auto index = FindPainterIndex(hwnd);
  return {WindowPainters[index], index};
}

std::pair<std::unique_ptr<Painter> &, SizeT> FindPainterOrConstruct(HWND hwnd) {
  SizeT index = FindPainterIndexOrConstruct(hwnd);
  return {WindowPainters[index], index};
}

union ObjectIdentifier {
  int32_t Identifier;
  struct {
    SizeT PainterIndex, ObjectIndex;
  };
};

int32_t GetObjIdentifier(SizeT PainterIndex, SizeT ObjectIndex) {
  ObjectIdentifier i;
  i.PainterIndex = PainterIndex;
  i.ObjectIndex = ObjectIndex;
  return i.Identifier;
}

std::pair<SizeT, SizeT> GetIndices(int32_t Id) {
  ObjectIdentifier i{Id};
  return {i.PainterIndex, i.ObjectIndex};
}

LRESULT MasterWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  //  if (uMsg == WM_UPDATEUISTATE)
  //    return TRUE;
  //  if (uMsg == WM_ERASEBKGND)
  //    return TRUE;
  //  auto [p, i] = FindPainterAndIndex(hwnd);
  //  if (uMsg == WM_PAINT) {
  //    auto r = p->CallOriginalWndProc(hwnd, uMsg, wParam, lParam);
  //    p->Paint(nullptr);
  //    ValidateRect(hwnd, NULL);
  //    return r;
  //  } else if (uMsg == WM_DESTROY)
  //    PostQuitMessage(0);
  //  else if (uMsg == WM_LBUTTONUP) {
  //    auto xPos = GET_X_LPARAM(lParam);
  //    auto yPos = GET_Y_LPARAM(lParam);
  //    for (auto b = p->Objects().rbegin(); b != p->Objects().rend(); b++) {
  //      auto o = b->get();
  //      if (xPos > o->X() && xPos < o->X() + o->Width() && yPos > o->Y() &&
  //          yPos < o->Y() + o->Height()) {
  //        if (o->OnMouseClick())
  //          o->OnMouseClick()(GetObjIdentifier(
  //              i, p->Objects().size() - 1 - (b - p->Objects().rbegin())));
  //        break;
  //      }
  //    }
  //  }
  auto [p, i] = FindPainterAndIndex(hwnd);
  long r;
  switch (uMsg) {
  case WM_PAINT:
    r = p->CallOriginalWndProc(hwnd, uMsg, wParam, lParam);
    p->Paint(nullptr);
    return r;
  default:
    return p->CallOriginalWndProc(hwnd, uMsg, wParam, lParam);
  }
}

DLL_EXPORT(void) UpdateDrawing(HWND hwnd) { InvalidateRgn(hwnd, nullptr, true); }

DLL_EXPORT(int)
DrawRectangle(HWND hwnd, ARGB LineColor, int startX, int startY, int width, int height) {
  auto [my_painter, PainterIndex] = FindPainterOrConstruct(hwnd);
  my_painter->Objects().push_back(
      std::make_unique<DrawableObjects::Rectangle>(LineColor, startX, startY, width, height));
  return GetObjIdentifier(PainterIndex, my_painter->Objects().size() - 1);
}

DLL_EXPORT(int)
DrawImage(HWND hwnd, const char *FileName, int startX, int startY, int width, int height) {
  auto [my_painter, PainterIndex] = FindPainterOrConstruct(hwnd);
  my_painter->Objects().push_back(
      std::make_unique<DrawableObjects::Image>(FileName, startX, startY, width, height));
  return GetObjIdentifier(PainterIndex, my_painter->Objects().size() - 1);
}

DLL_EXPORT(int)
DrawCircle(HWND hwnd, ARGB LineColor, int startX, int startY, int width, int height,
           float StartAngle, float SweepAngle) {
  auto [my_painter, PainterIndex] = FindPainterOrConstruct(hwnd);
  my_painter->Objects().push_back(std::make_unique<DrawableObjects::Circle>(
      LineColor, startX, startY, width, height, StartAngle, SweepAngle));
  return GetObjIdentifier(PainterIndex, my_painter->Objects().size() - 1);
}

DLL_EXPORT(int)
DrawArc(HWND hwnd, ARGB LineColor, int startX, int startY, int width, int height, float StartAngle,
        float SweepAngle, int PenWidth) {
  auto [my_painter, PainterIndex] = FindPainterOrConstruct(hwnd);
  my_painter->Objects().push_back(std::make_unique<DrawableObjects::Arc>(
      LineColor, startX, startY, width, height, StartAngle, SweepAngle, PenWidth));
  return GetObjIdentifier(PainterIndex, my_painter->Objects().size() - 1);
}

RECT GetRectangle(DrawableObject *obj) {
  return {obj->X(), obj->Y(), obj->X() + obj->Width(), obj->Y() + obj->Height()};
}

using property_value = uint64_t;

auto &GetObj(int32_t id) {
  auto [PainterIndex, ObjIndex] = GetIndices(id);
  return WindowPainters[PainterIndex]->Objects()[ObjIndex];
}

void ChangeProperty(int32_t id, property_value newValue,
                    void (*f)(DrawableObject *, property_value NewValue)) {
  auto [PainterIndex, ObjIndex] = GetIndices(id);
  auto &obj = WindowPainters[PainterIndex]->Objects()[ObjIndex];
  f(obj.get(), newValue);
}

void ChangeProperty(int32_t id, double newValue, void (*f)(DrawableObject *, double NewValue)) {
  auto [PainterIndex, ObjIndex] = GetIndices(id);
  auto &obj = WindowPainters[PainterIndex]->Objects()[ObjIndex];
  f(obj.get(), newValue);
}

DLL_EXPORT(void)
SetImageFile(int32_t id, const char *FileName) {
  auto &o = GetObj(id);
  if (auto image = dynamic_cast<DrawableObjects::Image *>(o.get())) {
    image->setImageFile(FileName);
  }
}

DLL_EXPORT(void)
SetOnMouseClick(int32_t id, int NewEvent) {
  ChangeProperty(id, NewEvent, [](DrawableObject *p, property_value n) {
    p->setOnMouseClick((int(__stdcall *)(int32_t))n);
  });
}

DLL_EXPORT(void)
SetWidth(int32_t id, int NewWidth) {
  ChangeProperty(id, NewWidth, [](DrawableObject *p, property_value n) { p->setWidth(n); });
}

DLL_EXPORT(void)
SetHeight(int32_t id, int NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, property_value n) { p->setHeight(n); });
}

DLL_EXPORT(void)
SetX(int32_t id, int NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, property_value n) { p->setX(n); });
}

DLL_EXPORT(void)
SetY(int32_t id, int NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, property_value n) { p->setY(n); });
}

DLL_EXPORT(void)
SetColor(int32_t id, DWORD NewValue) {
  ChangeProperty(id, NewValue,
                 [](DrawableObject *p, property_value NewValue) { p->setColor(NewValue); });
}

DLL_EXPORT(void)
SetTransparency(int32_t id, uint8_t NewValue) {
  ChangeProperty(id, NewValue,
                 [](DrawableObject *p, property_value NewValue) { p->setTransparency(NewValue); });
}

DLL_EXPORT(void)
SetStartAngle(int32_t id, float NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, double n) {
    auto o = dynamic_cast<DrawableObjects::Circle *>(p);
    if (o) {
      o->setStartAngle(n);
    }
  });
}

DLL_EXPORT(void)
SetSweepAngle(int32_t id, float NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, double n) {
    auto o = dynamic_cast<DrawableObjects::RoundObject *>(p);
    if (o) {
      o->setSweepAngle(n);
    }
  });
}

DLL_EXPORT(void)
SetPenWidth(int32_t id, property_value NewValue) {
  ChangeProperty(id, NewValue, [](DrawableObject *p, property_value n) {
    auto o = dynamic_cast<DrawableObjects::Outline *>(p);
    if (o) {
      o->setPenWidth(n);
    }
  });
}

DLL_EXPORT(void)
SetIsVisible(int32_t id, int32_t v) {}

DLL_EXPORT(int)
GetIsVisible(int32_t id) { return GetObj(id)->IsVisible(); }

DLL_EXPORT(int)
GetWidth(int32_t id) { return GetObj(id)->Width(); }

DLL_EXPORT(int) GetHeight(int32_t id) { return GetObj(id)->Height(); }

DLL_EXPORT(void) gdishutdown() {
  WindowPainters.clear();
  GdiManager.shutdown();
}
