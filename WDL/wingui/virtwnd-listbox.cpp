/*
    WDL - virtwnd-listbox.cpp
    Copyright (C) 2006 and later Cockos Incorporated

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
      

    Implementation for virtual window listboxes.

*/

#include "virtwnd-controls.h"
#include "../lice/lice.h"

WDL_VirtualListBox::WDL_VirtualListBox()
{
  m_scrollbuttonsize=14;
  m_cap_startitem=-1;
  m_cap_state=0;
  m_margin_l=m_margin_r=0;
  m_GetItemInfo=0;
  m_CustomDraw=0;
  m_GetItemInfo_ctx=0;
  m_viewoffs=0;
  m_align=-1;
  m_rh=14;
  m_maxcolwidth=m_mincolwidth=0;
  m_font=0;
  m_clickmsg=0;
  m_dropmsg=0;
  m_dragbeginmsg=0;
}

WDL_VirtualListBox::~WDL_VirtualListBox()
{
}

void WDL_VirtualListBox::CalcLayout(int num_items, int *nrows, int *ncols, int *leftrightbuttonsize, int *updownbuttonsize, int *startpos,
                                    int *usedw)
{
  *usedw = m_position.right - m_position.left;
  *leftrightbuttonsize=*updownbuttonsize=0;
  if (m_rh<7) m_rh=7;
  
  *ncols=1;

  *nrows= (m_position.bottom - m_position.top) / m_rh;
  if (*nrows<1) *nrows=1;

  if (m_mincolwidth>0)
  {
    *ncols = (num_items+*nrows-1) / *nrows; // round up
    if (*ncols<1) *ncols=1;
    if ((m_position.right-m_position.left) < m_mincolwidth* *ncols)
      *ncols =  (m_position.right-m_position.left)/m_mincolwidth;
  }

  if (m_maxcolwidth>0)
  {
    int oc = *ncols;
    if (m_mincolwidth<=0 || (m_position.right-m_position.left) <= m_maxcolwidth * *ncols)
      *ncols = (m_position.right-m_position.left) / m_maxcolwidth; // round down
  }
  
  if (*ncols < 1) *ncols=1;
  *startpos=0;
  
  if (num_items > *nrows * *ncols)
  {
    *startpos=m_viewoffs;

    if (*ncols > 1 && m_mincolwidth>0) // reduce columns to meet size requirements
    {
      if ((m_position.right-m_position.left - m_scrollbuttonsize*2) < m_mincolwidth* *ncols)
      {
        *ncols =  (m_position.right-m_position.left - m_scrollbuttonsize*2)/m_mincolwidth;
        if (*ncols < 1) *ncols=1;
      }
    }

    if (*ncols > 1) *leftrightbuttonsize = m_scrollbuttonsize;
    else *updownbuttonsize = m_scrollbuttonsize;
    
    *nrows = (m_position.bottom - m_position.top - *updownbuttonsize) / m_rh;
    if (*nrows<1) *nrows=1;

    if (m_maxcolwidth>0 && *ncols == 1 && *nrows ==1)
    {
      *leftrightbuttonsize = m_scrollbuttonsize;
      *updownbuttonsize=0;

      *nrows = (m_position.bottom - m_position.top - *updownbuttonsize) / m_rh;
      if (*nrows<1) *nrows=1;
    }
    
    int tot=*nrows * *ncols - (*nrows-1);
    if (*startpos > num_items-tot) *startpos=num_items-tot;
    if (*startpos<0)*startpos=0;
    
    if (*ncols>1)
      *startpos -= *startpos % *nrows;
  }

  if (m_maxcolwidth > 0)
  {
    int maxw=*ncols*m_maxcolwidth + 2* *leftrightbuttonsize;
    if (maxw < *usedw) *usedw=maxw;
  }

}

static void DrawBkImage(LICE_IBitmap *drawbm, WDL_VirtualWnd_BGCfg *bkbm, int drawx, int drawy, int draww, int drawh,
                RECT *cliprect, int drawsrcx, int drawsrcw, int bkbmstate)
{
  int hh=bkbm->bgimage->getHeight()/3;

  if (bkbm->bgimage_lt[0]||bkbm->bgimage_lt[1]||bkbm->bgimage_rb[0] || bkbm->bgimage_rb[1])
  {
    WDL_VirtualWnd_BGCfg tmp = *bkbm;
    if ((tmp.bgimage_noalphaflags&0xffff)!=0xffff) tmp.bgimage_noalphaflags=0;  // force alpha channel if any alpha

    LICE_SubBitmap bm(tmp.bgimage,drawsrcx,bkbmstate*hh,drawsrcw+1,hh+2);
    tmp.bgimage = &bm;

    WDL_VirtualWnd_ScaledBlitBG(drawbm,&tmp,
                                  drawx,drawy,draww,drawh,
                                  cliprect->left,cliprect->top,cliprect->right,cliprect->bottom,
                                  1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
  }
  else
  {
    LICE_ScaledBlit(drawbm,bkbm->bgimage,
      drawx,drawy,draww,drawh,
      drawsrcx,bkbmstate*hh,
      drawsrcw,hh,1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
  }
}


void WDL_VirtualListBox::OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{

  RECT r=m_position;
  r.left+=origin_x;
  r.right+=origin_x;
  r.top+=origin_y;
  r.bottom+=origin_y;

  WDL_VirtualWnd_BGCfg *mainbk=0;
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,&mainbk) : 0;
  LICE_pixel bgc=WDL_STYLE_GetSysColor(COLOR_BTNFACE);
  bgc=LICE_RGBA_FROMNATIVE(bgc,255);

  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos,usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);
  if (r.right > r.left + usedw) r.right=r.left+usedw;

  if (mainbk && mainbk->bgimage)
  {
    if (mainbk->bgimage->getWidth()>1 && mainbk->bgimage->getHeight()>1)
    {
      WDL_VirtualWnd_ScaledBlitBG(drawbm,mainbk,
                                    r.left,r.top,r.right-r.left,r.bottom-r.top,
                                    cliprect->left,cliprect->top,cliprect->right,cliprect->bottom,
                                    1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
    }
  }
  else
  {
    LICE_FillRect(drawbm,r.left,r.top,r.right-r.left,r.bottom-r.top,bgc,1.0f,LICE_BLIT_MODE_COPY);
  }

  LICE_pixel pencol = WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
  LICE_pixel pencol2 = WDL_STYLE_GetSysColor(COLOR_3DHILIGHT);
  pencol=LICE_RGBA_FROMNATIVE(pencol,255);
  pencol2=LICE_RGBA_FROMNATIVE(pencol2,255);

  LICE_pixel tcol=WDL_STYLE_GetSysColor(COLOR_BTNTEXT);
  if (m_font) m_font->SetBkMode(TRANSPARENT);

  
  int endpos=r.bottom - updownbuttonsize;
  int itempos=startpos;
  
  int colpos;
  int y=0;
  for (colpos = 0; colpos < num_cols; colpos ++)
  {
    int col_x = r.left + leftrightbuttonsize + ((r.right-r.left-leftrightbuttonsize*2)*colpos) / num_cols;
    int col_w = r.left + leftrightbuttonsize + ((r.right-r.left-leftrightbuttonsize*2)*(colpos+1)) / num_cols - col_x;
    for (y = r.top + m_rh; y <= endpos; y += m_rh)
    {
      int ly=y-m_rh;
      WDL_VirtualWnd_BGCfg *bkbm=0;
      if (m_GetItemInfo && ly >= r.top)
      {
        char buf[64];
        buf[0]=0;
        int color=tcol;

        if (m_GetItemInfo(this,itempos++,buf,sizeof(buf),&color,&bkbm))
        {
          color=LICE_RGBA_FROMNATIVE(color,0);
          RECT thisr;
          thisr.left = col_x;
          thisr.right = col_x + col_w;
          thisr.top = ly+1;
          thisr.bottom = y-1;
          int rev=0;
          int bkbmstate=0;
          if (m_cap_state==1 && m_cap_startitem==itempos-1)
          {
            if (bkbm) bkbmstate=1;
            else color = ((color>>1)&0x7f7f7f7f)+LICE_RGBA(0x7f,0x7f,0x7f,0);
          }
          if (m_cap_state>=0x1000 && m_cap_startitem==itempos-1)
          {
            if (bkbm) bkbmstate=2;
            else
            {
              rev=1;
              LICE_FillRect(drawbm,thisr.left,thisr.top,thisr.right-thisr.left,thisr.bottom-thisr.top, color,1.0f,LICE_BLIT_MODE_COPY);
            }
          }
          if (bkbm && bkbm->bgimage) //draw image!
          {
            
            DrawBkImage(drawbm,bkbm,
                thisr.left,thisr.top-1,thisr.right-thisr.left,thisr.bottom-thisr.top+2,
                cliprect,
                0,bkbm->bgimage->getWidth(),bkbmstate);

          }
          if (m_CustomDraw)
            m_CustomDraw(this,itempos-1,&thisr,drawbm);

          if (buf[0])
          {
            thisr.left+=m_margin_l;
            thisr.right-=m_margin_r;
            if (m_font)
            {
              m_font->SetTextColor(rev?bgc:color);
              m_font->DrawText(drawbm,buf,-1,&thisr,DT_VCENTER|(m_align<0?DT_LEFT:m_align>0?DT_RIGHT:DT_CENTER)|DT_NOPREFIX);
            }
          }
        }
      }

      if (!bkbm)
      {
        LICE_Line(drawbm,col_x,y,col_x+col_w,y,pencol2,1.0f,LICE_BLIT_MODE_COPY,false);
      }
    }
  }
  if (updownbuttonsize||leftrightbuttonsize)
  {
    WDL_VirtualWnd_BGCfg *bkbm=0;
    int a=m_GetItemInfo ? m_GetItemInfo(this,
      leftrightbuttonsize ? WDL_VWND_LISTBOX_ARROWINDEX_LR : WDL_VWND_LISTBOX_ARROWINDEX,NULL,0,NULL,&bkbm) : 0;

    if (bkbm && bkbm->bgimage)
    {
      if (leftrightbuttonsize)
      {
        int bkbmstate=startpos>0 ? 2 : 1;
        DrawBkImage(drawbm,bkbm,
            r.left,r.top,m_scrollbuttonsize,(r.bottom-r.top),
            cliprect,
            0,bkbm->bgimage->getWidth()/2,bkbmstate);


        bkbmstate=itempos<num_items ? 2 : 1;
        DrawBkImage(drawbm,bkbm,
            r.right-m_scrollbuttonsize,r.top,m_scrollbuttonsize,(r.bottom-r.top),
            cliprect,
            bkbm->bgimage->getWidth()/2,bkbm->bgimage->getWidth() - bkbm->bgimage->getWidth()/2,bkbmstate);
      }
      else
      {
        int bkbmstate=startpos>0 ? 2 : 1;
        DrawBkImage(drawbm,bkbm,
            r.left,y-m_rh,(r.right-r.left)/2,m_scrollbuttonsize,
            cliprect,
            0,bkbm->bgimage->getWidth()/2,bkbmstate);
  
        bkbmstate=itempos<num_items ? 2 : 1;
        DrawBkImage(drawbm,bkbm,
            (r.left+r.right)/2,y-m_rh,(r.right-r.left) - (r.right-r.left)/2,m_scrollbuttonsize,
            cliprect,
            bkbm->bgimage->getWidth()/2,bkbm->bgimage->getWidth() - bkbm->bgimage->getWidth()/2,bkbmstate);
      }
    }

    if (!a||!bkbm||!bkbm->bgimage)
    {
      bool butaa = true;
      if (updownbuttonsize)
      {
        int cx=(r.left+r.right)/2;
        int bs=5;
        int bsh=8;
        LICE_Line(drawbm,cx,y-m_scrollbuttonsize+2,cx,y-1,pencol2,1.0f,0,false);
        LICE_Line(drawbm,r.left,y,r.right,y,pencol2,1.0f,0,false);
      
        y-=m_scrollbuttonsize/2+bsh/2;

        if (itempos<num_items)
        {
          cx=(r.left+r.right)*3/4;

          LICE_Line(drawbm,cx-bs+1,y+2,cx,y+bsh-2,pencol2,1.0f,0,butaa);
          LICE_Line(drawbm,cx,y+bsh-2,cx+bs-1,y+2,pencol2,1.0f,0,butaa);
          LICE_Line(drawbm,cx+bs-1,y+2,cx-bs+1,y+2,pencol2,1.0f,0,butaa);

          LICE_Line(drawbm,cx-bs-1,y+1,cx,y+bsh-1,pencol,1.0f,0,butaa);
          LICE_Line(drawbm,cx,y+bsh-1,cx+bs+1,y+1,pencol,1.0f,0,butaa);
          LICE_Line(drawbm,cx+bs+1,y+1,cx-bs-1,y+1,pencol,1.0f,0,butaa);
        }
        if (startpos>0)
        {
          y-=2;
          cx=(r.left+r.right)/4;
          LICE_Line(drawbm,cx-bs+1,y+bsh,cx,y+3+1,pencol2,1.0f,0,butaa);
          LICE_Line(drawbm,cx,y+3+1,cx+bs-1,y+bsh,pencol2,1.0f,0,butaa);
          LICE_Line(drawbm,cx+bs-1,y+bsh,cx-bs+1,y+bsh,pencol2,1.0f,0,butaa);

          LICE_Line(drawbm,cx-bs-1,y+bsh+1,cx,y+3,pencol,1.0f,0,butaa);
          LICE_Line(drawbm,cx,y+3,cx+bs+1,y+bsh+1,pencol,1.0f,0,butaa);
          LICE_Line(drawbm,cx+bs+1,y+bsh+1, cx-bs-1,y+bsh+1, pencol,1.0f,0,butaa);
        }
      }
      else  // sideways buttons
      {
        #define LICE_LINEROT(bm,x1,y1,x2,y2,pc,al,mode,aa) LICE_Line(bm,y1,x1,y2,x2,pc,al,mode,aa)
        int bs=5;
        int bsh=8;
        int cx = (r.bottom + r.top)/2;
        if (itempos < num_items)
        {
          int y = r.right - leftrightbuttonsize/2 - bsh/2;
          LICE_LINEROT(drawbm,cx-bs+1,y+2,cx,y+bsh-2,pencol2,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx,y+bsh-2,cx+bs-1,y+2,pencol2,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx+bs-1,y+2,cx-bs+1,y+2,pencol2,1.0f,0,butaa);

          LICE_LINEROT(drawbm,cx-bs-1,y+1,cx,y+bsh-1,pencol,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx,y+bsh-1,cx+bs+1,y+1,pencol,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx+bs+1,y+1,cx-bs-1,y+1,pencol,1.0f,0,butaa);
        }
        if (startpos>0)
        {
          int y = r.left + leftrightbuttonsize/2-bsh/2 - 2;
          LICE_LINEROT(drawbm,cx-bs+1,y+bsh,cx,y+3+1,pencol2,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx,y+3+1,cx+bs-1,y+bsh,pencol2,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx+bs-1,y+bsh,cx-bs+1,y+bsh,pencol2,1.0f,0,butaa);

          LICE_LINEROT(drawbm,cx-bs-1,y+bsh+1,cx,y+3,pencol,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx,y+3,cx+bs+1,y+bsh+1,pencol,1.0f,0,butaa);
          LICE_LINEROT(drawbm,cx+bs+1,y+bsh+1, cx-bs-1,y+bsh+1, pencol,1.0f,0,butaa);
        }
        #undef LICE_LINEROT
      }
    }
  }



  if (!mainbk)
  {
    LICE_Line(drawbm,r.left,r.bottom-1,r.left,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.left,r.top,r.right-1,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.top,r.right-1,r.bottom-1,pencol2,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.bottom-1,r.left,r.bottom-1,pencol2,1.0f,0,false);
  }


}

bool WDL_VirtualListBox::HandleScrollClicks(int xpos, int ypos, int leftrightbuttonsize, int updownbuttonsize, int nrows, int num_cols, int num_items, int usedw)
{
  if (leftrightbuttonsize && (xpos<m_scrollbuttonsize || xpos >= usedw-m_scrollbuttonsize))
  {
    if (xpos<m_scrollbuttonsize)
    {
      if (m_viewoffs>0)
      {
        m_viewoffs-=nrows;
        if (m_viewoffs<0)m_viewoffs=0;
        RequestRedraw(NULL);
      }
    }
    else
    {
     if (m_viewoffs+nrows*num_cols < num_items)
     {
        m_viewoffs+=nrows;
        RequestRedraw(NULL);
      }
    }
    m_cap_state=0;
    m_cap_startitem=-1;
    return true;
  }
  if (updownbuttonsize && ypos >= nrows*m_rh)
  {
    if (ypos < (nrows)*m_rh + m_scrollbuttonsize)
    {
      if (xpos < usedw/2)
      {
        if (m_viewoffs>0)
        {
          m_viewoffs--;
          RequestRedraw(NULL);
        }
      }
      else
      {
        if (m_viewoffs+nrows*num_cols < num_items)
        {
          m_viewoffs++;
          RequestRedraw(NULL);
        }
      }
    }
    m_cap_state=0;
    m_cap_startitem=-1;
    return true;
  }
  return false;
}

int WDL_VirtualListBox::OnMouseDown(int xpos, int ypos)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;

  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos,usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);

  if (xpos >= usedw) return 0; 

  if (HandleScrollClicks(xpos,ypos,leftrightbuttonsize,updownbuttonsize,nrows,num_cols,num_items,usedw)) return 1;
  


  m_cap_state=0x1000;
  int usewid=(usedw-leftrightbuttonsize*2);
  int col = num_cols > 0 && usewid>0 ? ((xpos-leftrightbuttonsize)*num_cols)/usewid : 0;
  m_cap_startitem=startpos + (ypos)/m_rh + col*nrows;
  RequestRedraw(NULL);

  return 1;
}


bool WDL_VirtualListBox::OnMouseDblClick(int xpos, int ypos)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;

  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos,usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);

  if (xpos >= usedw) return false; 
  
  if (HandleScrollClicks(xpos,ypos,leftrightbuttonsize,updownbuttonsize,nrows,num_cols,num_items,usedw)) return true;
  
  return false;
}

bool WDL_VirtualListBox::OnMouseWheel(int xpos, int ypos, int amt)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos,usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);

  if (xpos >= usedw) return false; 

  if (num_items > nrows*num_cols)
  {
    if (amt>0 && m_viewoffs>0) 
    {
      m_viewoffs-=(num_cols>1?nrows:1);
      if (m_viewoffs<0)m_viewoffs=0;
      RequestRedraw(NULL);
    }
    else if (amt<0)
    {
      if (m_viewoffs+nrows*num_cols < num_items)
      {
        m_viewoffs+=(num_cols>1?nrows:1);
        RequestRedraw(NULL);
      }
    }
  }
  return true;
}

void WDL_VirtualListBox::OnMouseMove(int xpos, int ypos)
{
  if (m_cap_state>=0x1000)
  {
    m_cap_state++;
    if (m_cap_state==0x1008)
    {
      if (m_dragbeginmsg)
      {
        SendCommand(m_dragbeginmsg,(INT_PTR)this,m_cap_startitem,this);
      }
    }
  }
  else if (m_cap_state==0)
  {
    int a=IndexFromPt(xpos,ypos);
    if (a>=0)
    {
      m_cap_startitem=a;
      m_cap_state=1;
      RequestRedraw(NULL);
    }
  }
  else if (m_cap_state==1)
  {
    int a=IndexFromPt(xpos,ypos);
    if (a>=0 && a != m_cap_startitem)
    {
      m_cap_startitem=a;
      m_cap_state=1;
      RequestRedraw(NULL);
    }
    else if (a<0)
    {
      m_cap_state=0;
      RequestRedraw(NULL);
    }
  }
}

void WDL_VirtualListBox::OnMouseUp(int xpos, int ypos)
{
  int hit=IndexFromPt(xpos,ypos);
  if (m_cap_state>=0x1000 && m_cap_state<0x1008 && hit==m_cap_startitem) 
  {
    if (m_clickmsg)
    {
      SendCommand(m_clickmsg,(INT_PTR)this,hit,this);
    }
  }
  else if (m_cap_state>=0x1008)
  {
    // send a message saying drag & drop occurred
    if (m_dropmsg)
      SendCommand(m_dropmsg,(INT_PTR)this,m_cap_startitem,this);
  }

  m_cap_state=0;
  RequestRedraw(NULL);
}

bool WDL_VirtualListBox::GetItemRect(int item, RECT *r)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos, usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);
  item -= startpos;
  
  if (r)
  {
    int col = item / nrows;
    int row = item % nrows;
    r->top = row * m_rh;
    r->bottom = (row+1)*m_rh;
    r->left = leftrightbuttonsize + (col * (usedw - leftrightbuttonsize*2)) / num_cols;
    r->right = leftrightbuttonsize + ((col+1) * (usedw - leftrightbuttonsize*2)) / num_cols;
  }
  return item >= startpos && item < startpos + nrows*num_cols;;
}

int WDL_VirtualListBox::IndexFromPt(int x, int y)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int nrows,num_cols,updownbuttonsize,leftrightbuttonsize,startpos,usedw;
  CalcLayout(num_items,&nrows,&num_cols,&leftrightbuttonsize,&updownbuttonsize,&startpos,&usedw);

  if (x>=usedw) return -2;

  if (y < 0 || y >= nrows*m_rh ||x<leftrightbuttonsize || x >= (usedw-leftrightbuttonsize)) return -1;

  int usewid=(usedw-leftrightbuttonsize*2);
  int col = num_cols > 0 && usewid>0 ? ((x-leftrightbuttonsize)*num_cols)/usewid : 0;
  
  return startpos + (y)/m_rh + col * nrows;

}
