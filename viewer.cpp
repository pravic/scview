#include "window.hpp"
#include "listplug.h"

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
  window* wnd = new window(ParentWin);
  if(!wnd->load_file(FileToLoad)) {
    delete wnd;
    return nullptr;
  }

  // show window & go
  wnd->show();
  return wnd->get_hwnd();
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

HBITMAP CALLBACK ListGetPreviewBitmapW(const wchar_t* FileToLoad, int width, int height, char* contentbuf, int contentbuflen)
{
  if(!supported_file(FileToLoad))
    return nullptr;

  return nullptr;
}
