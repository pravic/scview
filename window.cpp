#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <sciter3/sciter-x-dom.hpp>

#include "window.hpp"

using namespace sciter;

extern HINSTANCE ghInstance;

ATOM window::get_class()
{
  static ATOM cls = 0;
  if(cls)
    return cls;

  WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW };
  wcex.lpszClassName = _T("TcSciterViewer");
  wcex.hInstance = ghInstance;
  wcex.lpfnWndProc = SciterProc;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

  return cls = RegisterClassEx(&wcex);
}

window::window(HWND parent)
  :wnd()
{
  RECT rc;
  GetClientRect(parent, &rc);

  wnd = CreateWindowEx(0, MAKEINTATOM(get_class()), NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP, 0, 0, rc.right, rc.bottom, parent, nullptr, ghInstance, nullptr);
  if(!wnd)
    return;

  SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  setup_callback();
  attach_dom_event_handler(wnd, this);

  __super::load_file(L"res:default.htm");

}

window::~window()
{
  if(wnd) {
    DestroyWindow(wnd);
    wnd = nullptr;
  }
}

HINSTANCE window::get_resource_instance()
{
  return ghInstance;
}

HWND window::get_hwnd()
{
  return this ? wnd : nullptr;
}

window* window::ptr(HWND wnd)
{
  window* self = reinterpret_cast<window*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
  return self && self->get_hwnd() == wnd ? self : nullptr;
}



void window::show()
{
  ShowWindow(wnd, SW_SHOW);
}

bool window::select_all()
{
  return false;
}

bool window::copy_selection()
{
  return false;
}

bool window::scroll(int percent)
{
  return false;
}

json::value window::launch_debug()
{
  dom::element root = get_root();
  dom::element frame = root.find_first("frame#content");
  assert(frame.is_valid());
  sciter::inspect(frame);
  return true;
}

bool window::load_file(const wchar_t* uri)
{
  dom::element root = get_root();
  json::value re = root.call_function("loadFileToView", json::value(uri));
  return re.get(false) || true;
}

void window::ensure_min_size()
{
  if(!wnd)
    return;
  set_size(get_size(true));
}

SIZE window::get_size(bool minsize) const
{
  SIZE size = {};
  if(!wnd)
    return size;
  if(minsize) {
    size.cx = SciterGetMinWidth(wnd);
    size.cy = SciterGetMinHeight(wnd, size.cx);
  } else {
    RECT rc = {};
    GetClientRect(wnd, &rc);
    size.cx = rc.right;
    size.cy = rc.bottom;
  }
  return size;
}

void window::set_size(const SIZE& size)
{
  if(wnd) {
    RECT wc, rc;
    GetWindowRect(wnd, &wc);
    GetClientRect(wnd, &rc);
    OffsetRect(&wc, -wc.left, -wc.top);
    rc.right = (wc.right - rc.right) + size.cx;
    rc.bottom = (wc.bottom - rc.bottom) + size.cy;
    SetWindowPos(wnd, nullptr, 0, 0, rc.right, rc.bottom, SWP_NOZORDER | SWP_NOMOVE);
  }
}
