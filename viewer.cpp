#include "window.hpp"
#include "listplug.h"

#define _USING_V110_SDK71_
#include <atlcomcli.h>
#include <memory>
#include <wincodec.h>

HINSTANCE ghInstance;

bool supported_file(const wchar_t* loading_file)
{
  if(!loading_file)
    return false;

  std::wstring file = loading_file;
  size_t extp = file.rfind('.');
  if(extp == file.npos)
    return false;

  const wchar_t* ext = &file[extp + 1];
  if(!(wcsicmp(ext, L"htm") == 0 || wcsicmp(ext, L"html") == 0))
    return false;

  return true;
}

BOOL CALLBACK DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
  ghInstance = module;
  return true;
}

void CALLBACK ListGetDetectString(char* DetectString, int maxlen)
{
  std::strncpy(DetectString, R"(EXT="HTM" | EXT="HTML")", maxlen);
}

HWND CALLBACK ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
  return nullptr;
}

HWND CALLBACK ListLoadW(HWND ParentWin, wchar_t* FileToLoad, int ShowFlags)
{
  if(!supported_file(FileToLoad))
    return false;

  // create viewer window and load file
  auto wnd = std::make_unique<window>(ParentWin);
  if(!wnd->load_file(FileToLoad))
    return nullptr;

  // show window & go
  wnd->show();
  HWND re = wnd->get_hwnd();
  if(re)
    wnd.release();
  return re;
}

int CALLBACK ListLoadNextW(HWND ParentWin, HWND ListWin, const wchar_t* FileToLoad, int ShowFlags)
{
  if(!supported_file(FileToLoad))
    return LISTPLUGIN_ERROR;

  auto w = window::ptr(ListWin);
  if(!w)
    return LISTPLUGIN_ERROR;
  
  if(!w->load_file(FileToLoad))
    return LISTPLUGIN_ERROR;

  w->show();
  return LISTPLUGIN_OK;
}

void CALLBACK ListCloseWindow(HWND ListWin)
{
  if(auto w = window::ptr(ListWin))
    delete w;
}

int CALLBACK ListSendCommand(HWND ListWin, int Command, int Parameter)
{
  auto w = window::ptr(ListWin);
  if(!w)
    return LISTPLUGIN_ERROR;

  bool ok = false;
  switch(Command)
  {
  case lc_selectall:
    ok = w->select_all();
    break;

  case lc_copy:
    ok = w->copy_selection();
    break;

  case lc_setpercent:
    ok = w->scroll(Parameter);
    break;
  }

  return ok ? LISTPLUGIN_OK : LISTPLUGIN_ERROR;
}

HBITMAP CALLBACK ListGetPreviewBitmapW(const wchar_t* FileToLoad, int width, int height, const char* contentbuf, int contentbuflen)
{
  if(!supported_file(FileToLoad))
    return nullptr;

  CComPtr<ID2D1Factory> factory;
  SciterD2DFactory(&factory);
  if(!factory)
    return nullptr;

  auto wnd = std::make_unique<window>(nullptr);
  if(wnd->load_file(FileToLoad)) {

    // resize window to render document
    SIZE maxSize = { width, height }, minSize = {1024, 768};
    bool stretch = minSize.cx > maxSize.cx || minSize.cy > maxSize.cy;
    if(stretch)
      wnd->set_size(minSize);
    else
      wnd->set_size(maxSize);
    UpdateWindow(wnd->get_hwnd());

    SIZE wndSize = wnd->get_size();

    CComPtr<ID2D1DCRenderTarget> rt;
    auto rtprops = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
    HRESULT hr = factory->CreateDCRenderTarget(&rtprops , &rt);
    if(!rt)
      return nullptr;

    HDC screen = ::GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP hbmp = CreateCompatibleBitmap(screen, wndSize.cx, wndSize.cy);
    auto oldObj = SelectObject(dc, hbmp);

    RECT bindRect = { 0, 0, wndSize.cx, wndSize.cy };
    hr = rt->BindDC(dc, &bindRect);

    // render document to the render target
    if(SciterRenderD2D(wnd->get_hwnd(), rt)) {

      if(stretch) {
        // stretch bitmap
        SIZE stretch = {};
        stretch.cy = MulDiv(maxSize.cx, wndSize.cy, wndSize.cx);
        if(stretch.cy <= maxSize.cy) {
          stretch.cx = maxSize.cx;
        } else {
          stretch.cx = MulDiv(maxSize.cy, wndSize.cx, wndSize.cy);
          stretch.cy = maxSize.cy;
        }
        HDC sdc = CreateCompatibleDC(screen);
        HBITMAP thumb = CreateCompatibleBitmap(screen, stretch.cx, stretch.cy);
        auto oldObj2 = SelectObject(sdc, thumb);
        SetStretchBltMode(sdc, HALFTONE);
        SetBrushOrgEx(sdc, 0, 0, nullptr);

        StretchBlt(sdc, 0, 0, stretch.cx, stretch.cy, dc, 0, 0, wndSize.cx, wndSize.cy, SRCCOPY);
        SelectObject(sdc, oldObj2);
        DeleteDC(sdc);

        // need to free old big image
        SelectObject(dc, oldObj);
        DeleteObject(hbmp);
        hbmp = thumb;
      } else {
        // restore original DC
        SelectObject(dc, oldObj);
      }
      DeleteDC(dc);
      ReleaseDC(nullptr, screen);
      return hbmp;
    }
  }

  return nullptr;
}
