IconLoader::IconLoader() {
  OSVERSIONINFOEX osVinfo;

  memset(&osVinfo,0,sizeof(OSVERSIONINFOEX));
  osVinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  GetVersionEx((OSVERSIONINFO *)&osVinfo);

  uxThemeDll = 0;
  isVistaUp = (osVinfo.dwMajorVersion >= 6);

  if(isVistaUp) {
    uxThemeDll = LoadLibraryW(L"UxTheme.dll");
    if (uxThemeDll) {
      GetBufferedPaintBits = (GETBUFFEREDPAINTBITS)GetProcAddress(uxThemeDll, "GetBufferedPaintBits");
      BeginBufferedPaint = (BEGINBUFFEREDPAINT)GetProcAddress(uxThemeDll, "BeginBufferedPaint");
      EndBufferedPaint = (ENDBUFFEREDPAINT)GetProcAddress(uxThemeDll, "EndBufferedPaint");
    }
  }
}

IconLoader::~IconLoader() {
  std::map<string, HBITMAP>::iterator it;
  for (it = icons.begin(); it != icons.end(); ++it) {
    DeleteObject(it->second);
  }

  if(uxThemeDll)
    FreeLibrary(uxThemeDll);
}

HBITMAP IconLoader::LoadIcon(string icon) {
  if(!isVistaUp || icon.trim(" "," ")=="") return 0;

  std::map<string, HBITMAP>::iterator it = icons.find(icon);
  if(it != icons.end()) return it->second;

  string_vector iconparts;
  iconparts = icon.split(",");
  int index = 0;
  if(iconparts.size()>1)
    index = iconparts[1].trim(" "," ").integer();

  HICON hIcon;
  if(!ExtractIconExW((const wchar_t*)utf16_t(iconparts[0]),index,NULL,&hIcon,1))
    return 0;

  HBITMAP hBmp = IconToBitmapPARGB32(hIcon);

  DestroyIcon(hIcon);

  if(hBmp) {
    icons.insert(std::make_pair(icon, hBmp));
  }

  return hBmp;
}

void IconLoader::InitBitmapInfo(BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp)
{
  ZeroMemory(pbmi, cbInfo);
  pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmi->bmiHeader.biPlanes = 1;
  pbmi->bmiHeader.biCompression = BI_RGB;

  pbmi->bmiHeader.biWidth = cx;
  pbmi->bmiHeader.biHeight = cy;
  pbmi->bmiHeader.biBitCount = bpp;
}

HRESULT IconLoader::Create32BitHBITMAP(HDC hdc, const SIZE *psize, void **ppvBits, HBITMAP* phBmp)
{
  *phBmp = NULL;

  BITMAPINFO bmi;
  InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

  HDC hdcUsed = hdc ? hdc : GetDC(NULL);
  if (hdcUsed)
  {
    *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
    if (hdc != hdcUsed)
    {
      ReleaseDC(NULL, hdcUsed);
    }
  }
  return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT IconLoader::ConvertToPARGB32(HDC hdc, ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow)
{
  BITMAPINFO bmi;
  InitBitmapInfo(&bmi, sizeof(bmi), sizImage.cx, sizImage.cy, 32);

  HRESULT hr = E_OUTOFMEMORY;
  HANDLE hHeap = GetProcessHeap();
  void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
  if (pvBits)
  {
    hr = E_UNEXPECTED;
    if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
    {
      ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
      ARGB *pargbMask = static_cast<ARGB *>(pvBits);

      for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
      {
        for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
        {
          if (*pargbMask++)
          {
            // transparent pixel
            *pargb++ = 0;
          }
          else
          {
            // opaque pixel
            *pargb++ |= 0xFF000000;
          }
        }

        pargb += cxDelta;
      }

      hr = S_OK;
    }

    HeapFree(hHeap, 0, pvBits);
  }

  return hr;
}

bool IconLoader::HasAlpha(ARGB *pargb, SIZE& sizImage, int cxRow)
{
  ULONG cxDelta = cxRow - sizImage.cx;
  for (ULONG y = sizImage.cy; y; --y)
  {
    for (ULONG x = sizImage.cx; x; --x)
    {
      if (*pargb++ & 0xFF000000)
      {
        return true;
      }
    }

    pargb += cxDelta;
  }

  return false;
}

HRESULT IconLoader::ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon)
{
  RGBQUAD *prgbQuad;
  int cxRow;
  HRESULT hr = GetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
  if (SUCCEEDED(hr))
  {
    ARGB *pargb = reinterpret_cast<ARGB *>(prgbQuad);
    if (!HasAlpha(pargb, sizIcon, cxRow))
    {
      ICONINFO info;
      if (GetIconInfo(hicon, &info))
      {
        if (info.hbmMask)
        {
          hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
        }

        DeleteObject(info.hbmColor);
        DeleteObject(info.hbmMask);
      }
    }
  }

  return hr;
}

HBITMAP IconLoader::IconToBitmapPARGB32(HICON hicon)
{
  HRESULT hr = E_OUTOFMEMORY;
  HBITMAP hbmp = NULL;

  SIZE sizIcon;
  sizIcon.cx = GetSystemMetrics(SM_CXSMICON);
  sizIcon.cy = GetSystemMetrics(SM_CYSMICON);

  RECT rcIcon;
  SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);

  HDC hdcDest = CreateCompatibleDC(NULL);
  if (hdcDest)
  {
    hr = Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hbmp);
    if (SUCCEEDED(hr))
    {
      hr = E_FAIL;

      HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hbmp);
      if (hbmpOld)
      {
        BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        BP_PAINTPARAMS paintParams = {0};
        paintParams.cbSize = sizeof(paintParams);
        paintParams.dwFlags = 0x0001;
        paintParams.pBlendFunction = &bfAlpha;

        HDC hdcBuffer;
        HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
        if (hPaintBuffer)
        {
          if (DrawIconEx(hdcBuffer, 0, 0, hicon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL))
          {
            // If icon did not have an alpha channel, we need to convert buffer to PARGB.
            hr = ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hicon, sizIcon);
          }

          // This will write the buffer contents to the destination bitmap.
          EndBufferedPaint(hPaintBuffer, TRUE);
        }

        SelectObject(hdcDest, hbmpOld);
      }
    }

    DeleteDC(hdcDest);
  }

  if (FAILED(hr))
  {
    DeleteObject(hbmp);
    hbmp = NULL;
  }

  return hbmp;
}
