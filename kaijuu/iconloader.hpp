typedef HRESULT (WINAPI *GETBUFFEREDPAINTBITS) (HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer, int *pcxRow);
typedef HPAINTBUFFER (WINAPI *BEGINBUFFEREDPAINT) (HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (WINAPI *ENDBUFFEREDPAINT) (HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);

typedef DWORD ARGB;

struct IconLoader {
protected:
  HMODULE uxThemeDll;
  bool isVistaUp;
  std::map<string,HBITMAP> icons;

  GETBUFFEREDPAINTBITS GetBufferedPaintBits;
  BEGINBUFFEREDPAINT BeginBufferedPaint;
  ENDBUFFEREDPAINT EndBufferedPaint;

  HBITMAP IconToBitmapPARGB32(HICON hicon);
  HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, void **ppvBits, HBITMAP* phBmp);
  void InitBitmapInfo(BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp);
  HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon);
  HRESULT ConvertToPARGB32(HDC hdc, ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow);
  bool HasAlpha(ARGB *pargb, SIZE& sizImage, int cxRow);

public:
  IconLoader();
  ~IconLoader();

  HBITMAP LoadIcon(string icon);
};
