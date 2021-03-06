/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_bmp.cpp (BMP loading for LICE)
  See lice.h for license and other information
*/

#include "lice.h"

static LICE_IBitmap *hbmToBit(HBITMAP hbm, LICE_IBitmap *bmp)
{
  BITMAP bm;
  GetObject(hbm, sizeof(BITMAP), (LPSTR)&bm);

  LICE_SysBitmap sysbitmap(bm.bmWidth,bm.bmHeight);
  
#ifdef _WIN32
  HDC hdc=CreateCompatibleDC(NULL);
  HGDIOBJ oldBM=SelectObject(hdc,hbm);

  BitBlt(sysbitmap.getDC(),0,0,bm.bmWidth,bm.bmHeight,hdc,0,0,SRCCOPY);
  GdiFlush();

  if (!bmp) bmp=new LICE_MemBitmap(bm.bmWidth,bm.bmHeight);
  LICE_Copy(bmp,&sysbitmap);

  SelectObject(hdc,oldBM);
  DeleteDC(hdc);
  #else
  LICE_Clear(&sysbitmap,0);
  RECT r={0,0,bm.bmWidth,bm.bmHeight};
  DrawImageInRect(sysbitmap.getDC(),hbm,&r);
  if (!bmp) bmp=new LICE_MemBitmap(bm.bmWidth,bm.bmHeight);
  LICE_Copy(bmp,&sysbitmap);
  #endif

  return bmp;
}


LICE_IBitmap *LICE_LoadBMP(const char *filename, LICE_IBitmap *bmp) // returns a bitmap (bmp if nonzero) on success
{
  HBITMAP bm=NULL;
#ifdef _WIN32
  if (GetVersion()<0x80000000)
  {
    WCHAR wf[2048];
    if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename,-1,wf,2048))
      bm = (HBITMAP) LoadImageW(NULL,wf,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE);
  }

  if (!bm) bm=(HBITMAP) LoadImage(NULL,filename,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE);
#else
  bm=(HBITMAP) LoadNamedImage(filename,false);
#endif
  if (!bm) return 0;

  LICE_IBitmap *ret=hbmToBit(bm,bmp);

  DeleteObject(bm);
  return ret;
}

#ifdef _WIN32
LICE_IBitmap *LICE_LoadBMPFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp) // returns a bitmap (bmp if nonzero) on success
{
  HBITMAP bm=(HBITMAP) LoadImage(hInst,MAKEINTRESOURCE(resid),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
  if (!bm) return 0;

  LICE_IBitmap *ret=hbmToBit(bm,bmp);

  DeleteObject(bm);
  return ret;
}
#endif



class LICE_BMPLoader
{
public:
  _LICE_ImageLoader_rec rec;
  LICE_BMPLoader() 
  {
    rec.loadfunc = loadfunc;
    rec.get_extlist = get_extlist;
    rec._next = LICE_ImageLoader_list;
    LICE_ImageLoader_list = &rec;
  }

  static LICE_IBitmap *loadfunc(const char *filename, bool checkFileName, LICE_IBitmap *bmpbase)
  {
    if (checkFileName)
    {
      const char *p=filename;
      while (*p)p++;
      while (p>filename && *p != '\\' && *p != '/' && *p != '.') p--;
      if (stricmp(p,".bmp")) return 0;
    }
    return LICE_LoadBMP(filename,bmpbase);
  }
  static const char *get_extlist()
  {
    return "BMP files (*.BMP)\0*.BMP\0";
  }

};

static LICE_BMPLoader LICE_bmpldr;