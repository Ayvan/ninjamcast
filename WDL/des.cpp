/* Loosely based on:
 *
 * D3DES (V5.09) -
 *
 * A portable, public domain, version of the Data Encryption Standard.
 *
 * Written with Symantec's THINK (Lightspeed) C by Richard Outerbridge.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
 * (GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.
 */


#include "des.h"

static const unsigned char pc1[56] = {
  56, 48, 40, 32, 24, 16,  8,      0, 57, 49, 41, 33, 25, 17,
   9,  1, 58, 50, 42, 34, 26,     18, 10,  2, 59, 51, 43, 35,
  62, 54, 46, 38, 30, 22, 14,      6, 61, 53, 45, 37, 29, 21,
  13,  5, 60, 52, 44, 36, 28,     20, 12,  4, 27, 19, 11,  3 
};
static const unsigned char totrot[16] = { 1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

static const unsigned char pc2[48] = {
  13, 16, 10, 23,  0,  4,  2, 27, 14,  5, 20,  9,
  22, 18, 11,  3, 25,  7, 15,  6, 26, 19, 12,  1,
  40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47,
  43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31 };

static unsigned int SP1[64] = {
  0x01010400, 0x00000000, 0x00010000, 0x01010404,
  0x01010004, 0x00010404, 0x00000004, 0x00010000,
  0x00000400, 0x01010400, 0x01010404, 0x00000400,
  0x01000404, 0x01010004, 0x01000000, 0x00000004,
  0x00000404, 0x01000400, 0x01000400, 0x00010400,
  0x00010400, 0x01010000, 0x01010000, 0x01000404,
  0x00010004, 0x01000004, 0x01000004, 0x00010004,
  0x00000000, 0x00000404, 0x00010404, 0x01000000,
  0x00010000, 0x01010404, 0x00000004, 0x01010000,
  0x01010400, 0x01000000, 0x01000000, 0x00000400,
  0x01010004, 0x00010000, 0x00010400, 0x01000004,
  0x00000400, 0x00000004, 0x01000404, 0x00010404,
  0x01010404, 0x00010004, 0x01010000, 0x01000404,
  0x01000004, 0x00000404, 0x00010404, 0x01010400,
  0x00000404, 0x01000400, 0x01000400, 0x00000000,
  0x00010004, 0x00010400, 0x00000000, 0x01010004 
};

static unsigned int SP2[64] = {
  0x80108020, 0x80008000, 0x00008000, 0x00108020,
  0x00100000, 0x00000020, 0x80100020, 0x80008020,
  0x80000020, 0x80108020, 0x80108000, 0x80000000,
  0x80008000, 0x00100000, 0x00000020, 0x80100020,
  0x00108000, 0x00100020, 0x80008020, 0x00000000,
  0x80000000, 0x00008000, 0x00108020, 0x80100000,
  0x00100020, 0x80000020, 0x00000000, 0x00108000,
  0x00008020, 0x80108000, 0x80100000, 0x00008020,
  0x00000000, 0x00108020, 0x80100020, 0x00100000,
  0x80008020, 0x80100000, 0x80108000, 0x00008000,
  0x80100000, 0x80008000, 0x00000020, 0x80108020,
  0x00108020, 0x00000020, 0x00008000, 0x80000000,
  0x00008020, 0x80108000, 0x00100000, 0x80000020,
  0x00100020, 0x80008020, 0x80000020, 0x00100020,
  0x00108000, 0x00000000, 0x80008000, 0x00008020,
  0x80000000, 0x80100020, 0x80108020, 0x00108000 
};

static unsigned int SP3[64] = {
  0x00000208, 0x08020200, 0x00000000, 0x08020008,
  0x08000200, 0x00000000, 0x00020208, 0x08000200,
  0x00020008, 0x08000008, 0x08000008, 0x00020000,
  0x08020208, 0x00020008, 0x08020000, 0x00000208,
  0x08000000, 0x00000008, 0x08020200, 0x00000200,
  0x00020200, 0x08020000, 0x08020008, 0x00020208,
  0x08000208, 0x00020200, 0x00020000, 0x08000208,
  0x00000008, 0x08020208, 0x00000200, 0x08000000,
  0x08020200, 0x08000000, 0x00020008, 0x00000208,
  0x00020000, 0x08020200, 0x08000200, 0x00000000,
  0x00000200, 0x00020008, 0x08020208, 0x08000200,
  0x08000008, 0x00000200, 0x00000000, 0x08020008,
  0x08000208, 0x00020000, 0x08000000, 0x08020208,
  0x00000008, 0x00020208, 0x00020200, 0x08000008,
  0x08020000, 0x08000208, 0x00000208, 0x08020000,
  0x00020208, 0x00000008, 0x08020008, 0x00020200 
};

static unsigned int SP4[64] = {
  0x00802001, 0x00002081, 0x00002081, 0x00000080,
  0x00802080, 0x00800081, 0x00800001, 0x00002001,
  0x00000000, 0x00802000, 0x00802000, 0x00802081,
  0x00000081, 0x00000000, 0x00800080, 0x00800001,
  0x00000001, 0x00002000, 0x00800000, 0x00802001,
  0x00000080, 0x00800000, 0x00002001, 0x00002080,
  0x00800081, 0x00000001, 0x00002080, 0x00800080,
  0x00002000, 0x00802080, 0x00802081, 0x00000081,
  0x00800080, 0x00800001, 0x00802000, 0x00802081,
  0x00000081, 0x00000000, 0x00000000, 0x00802000,
  0x00002080, 0x00800080, 0x00800081, 0x00000001,
  0x00802001, 0x00002081, 0x00002081, 0x00000080,
  0x00802081, 0x00000081, 0x00000001, 0x00002000,
  0x00800001, 0x00002001, 0x00802080, 0x00800081,
  0x00002001, 0x00002080, 0x00800000, 0x00802001,
  0x00000080, 0x00800000, 0x00002000, 0x00802080 
};

static unsigned int SP5[64] = {
  0x00000100, 0x02080100, 0x02080000, 0x42000100,
  0x00080000, 0x00000100, 0x40000000, 0x02080000,
  0x40080100, 0x00080000, 0x02000100, 0x40080100,
  0x42000100, 0x42080000, 0x00080100, 0x40000000,
  0x02000000, 0x40080000, 0x40080000, 0x00000000,
  0x40000100, 0x42080100, 0x42080100, 0x02000100,
  0x42080000, 0x40000100, 0x00000000, 0x42000000,
  0x02080100, 0x02000000, 0x42000000, 0x00080100,
  0x00080000, 0x42000100, 0x00000100, 0x02000000,
  0x40000000, 0x02080000, 0x42000100, 0x40080100,
  0x02000100, 0x40000000, 0x42080000, 0x02080100,
  0x40080100, 0x00000100, 0x02000000, 0x42080000,
  0x42080100, 0x00080100, 0x42000000, 0x42080100,
  0x02080000, 0x00000000, 0x40080000, 0x42000000,
  0x00080100, 0x02000100, 0x40000100, 0x00080000,
  0x00000000, 0x40080000, 0x02080100, 0x40000100 
};

static unsigned int SP6[64] = {
  0x20000010, 0x20400000, 0x00004000, 0x20404010,
  0x20400000, 0x00000010, 0x20404010, 0x00400000,
  0x20004000, 0x00404010, 0x00400000, 0x20000010,
  0x00400010, 0x20004000, 0x20000000, 0x00004010,
  0x00000000, 0x00400010, 0x20004010, 0x00004000,
  0x00404000, 0x20004010, 0x00000010, 0x20400010,
  0x20400010, 0x00000000, 0x00404010, 0x20404000,
  0x00004010, 0x00404000, 0x20404000, 0x20000000,
  0x20004000, 0x00000010, 0x20400010, 0x00404000,
  0x20404010, 0x00400000, 0x00004010, 0x20000010,
  0x00400000, 0x20004000, 0x20000000, 0x00004010,
  0x20000010, 0x20404010, 0x00404000, 0x20400000,
  0x00404010, 0x20404000, 0x00000000, 0x20400010,
  0x00000010, 0x00004000, 0x20400000, 0x00404010,
  0x00004000, 0x00400010, 0x20004010, 0x00000000,
  0x20404000, 0x20000000, 0x00400010, 0x20004010
};

static unsigned int SP7[64] = {
  0x00200000, 0x04200002, 0x04000802, 0x00000000,
  0x00000800, 0x04000802, 0x00200802, 0x04200800,
  0x04200802, 0x00200000, 0x00000000, 0x04000002,
  0x00000002, 0x04000000, 0x04200002, 0x00000802,
  0x04000800, 0x00200802, 0x00200002, 0x04000800,
  0x04000002, 0x04200000, 0x04200800, 0x00200002,
  0x04200000, 0x00000800, 0x00000802, 0x04200802,
  0x00200800, 0x00000002, 0x04000000, 0x00200800,
  0x04000000, 0x00200800, 0x00200000, 0x04000802,
  0x04000802, 0x04200002, 0x04200002, 0x00000002,
  0x00200002, 0x04000000, 0x04000800, 0x00200000,
  0x04200800, 0x00000802, 0x00200802, 0x04200800,
  0x00000802, 0x04000002, 0x04200802, 0x04200000,
  0x00200800, 0x00000000, 0x00000002, 0x04200802,
  0x00000000, 0x00200802, 0x04200000, 0x00000800,
  0x04000002, 0x04000800, 0x00000800, 0x00200002 
};

static unsigned int SP8[64] = {
  0x10001040, 0x00001000, 0x00040000, 0x10041040,
  0x10000000, 0x10001040, 0x00000040, 0x10000000,
  0x00040040, 0x10040000, 0x10041040, 0x00041000,
  0x10041000, 0x00041040, 0x00001000, 0x00000040,
  0x10040000, 0x10000040, 0x10001000, 0x00001040,
  0x00041000, 0x00040040, 0x10040040, 0x10041000,
  0x00001040, 0x00000000, 0x00000000, 0x10040040,
  0x10000040, 0x10001000, 0x00041040, 0x00040000,
  0x00041040, 0x00040000, 0x10041000, 0x00001000,
  0x00000040, 0x10040040, 0x00001000, 0x00041040,
  0x10001000, 0x00000040, 0x10000040, 0x10040000,
  0x10040040, 0x10000000, 0x00040000, 0x10001040,
  0x00000000, 0x10041040, 0x00040040, 0x10000040,
  0x10040000, 0x10001000, 0x10001040, 0x00000000,
  0x10041040, 0x00041000, 0x00041000, 0x00001040,
  0x00001040, 0x00040040, 0x10000000, 0x10041000
};

WDL_DES::WDL_DES()
{
}
WDL_DES::~WDL_DES()
{
}

void WDL_DES::SetKey(const unsigned char *key8, bool isEncrypt)
{
  int i;
  unsigned char pc1m[56], pcr[56];
  for ( i = 0; i < 56; i++ ) 
  {
    int l = pc1[i];
    int m = l & 07;
    pc1m[i] = (key8[l >> 3] & (1<<m)) ? 1 : 0;
  }

  for (i = 0; i < 16; i++) 
  {
    int m = (isEncrypt?i:15-i) << 1;
    int n = m + 1;
    m_keydata[m] = m_keydata[n] = 0;
    int j;
    for (j = 0; j < 28; j++) 
    {
      int l = j + totrot[i];
      if( l < 28 ) pcr[j] = pc1m[l];
      else pcr[j] = pc1m[l - 28];
    }
    for (j = 28; j < 56; j++) 
    {
      int l = j + totrot[i];
      if( l < 56 ) pcr[j] = pc1m[l];
      else pcr[j] = pc1m[l - 28];
    }
    for (j = 0; j < 24; j++) 
    {
      int v= 1<<(23-j);
      if( pcr[pc2[j]] ) m_keydata[m] |= v;
      if( pcr[pc2[j+24]] ) m_keydata[n] |= v;
    }
  }

  for( i = 0; i < 32; i+=2) 
  {
    unsigned int a = ((m_keydata[i] & 0x00fc0000) << 6) | 
             ((m_keydata[i] & 0x00000fc0) << 10) |
             ((m_keydata[i+1] & 0x00fc0000) >> 10) |
             ((m_keydata[i+1] & 0x00000fc0) >> 6);

    unsigned int b= ((m_keydata[i] & 0x0003f000) << 12) |
               ((m_keydata[i] & 0x0000003f) << 16) |
               ((m_keydata[i+1] & 0x0003f000) >> 4) |
               (m_keydata[i+1] & 0x0000003f);
    m_keydata[i]=a;
    m_keydata[i+1]=b;
  }


}

void WDL_DES::Process8(unsigned char *buf8)
{
  unsigned int leftt=(buf8[0]<<24)|(buf8[1]<<16)|(buf8[2]<<8)|(buf8[3]);
  unsigned int right=(buf8[4]<<24)|(buf8[5]<<16)|(buf8[6]<<8)|(buf8[7]);

  unsigned int *keys = m_keydata;
  unsigned int fval, work;
  int round;

  work = ((leftt >> 4) ^ right) & 0x0f0f0f0f;
  right ^= work;
  leftt ^= (work << 4);
  work = ((leftt >> 16) ^ right) & 0x0000ffff;
  right ^= work;
  leftt ^= (work << 16);
  work = ((right >> 2) ^ leftt) & 0x33333333;
  leftt ^= work;
  right ^= (work << 2);
  work = ((right >> 8) ^ leftt) & 0x00ff00ff;
  leftt ^= work;
  right ^= (work << 8);
  right = ((right << 1) | ((right >> 31) & 1)) & 0xffffffff;
  work = (leftt ^ right) & 0xaaaaaaaa;
  leftt ^= work;
  right ^= work;
  leftt = ((leftt << 1) | ((leftt >> 31) & 1)) & 0xffffffff;

  for (round = 0; round < 8; round++) 
  {
    work  = (right << 28) | (right >> 4);
    work ^= *keys++;
    fval  = SP7[ work & 0x3f];
    fval |= SP5[(work >>  8) & 0x3f];
    fval |= SP3[(work >> 16) & 0x3f];
    fval |= SP1[(work >> 24) & 0x3f];
    work  = right ^ *keys++;
    fval |= SP8[ work & 0x3f];
    fval |= SP6[(work >>  8) & 0x3f];
    fval |= SP4[(work >> 16) & 0x3f];
    fval |= SP2[(work >> 24) & 0x3f];
    leftt ^= fval;
    work  = (leftt << 28) | (leftt >> 4);
    work ^= *keys++;
    fval  = SP7[ work & 0x3f];
    fval |= SP5[(work >>  8) & 0x3f];
    fval |= SP3[(work >> 16) & 0x3f];
    fval |= SP1[(work >> 24) & 0x3f];
    work  = leftt ^ *keys++;
    fval |= SP8[ work & 0x3f];
    fval |= SP6[(work >>  8) & 0x3f];
    fval |= SP4[(work >> 16) & 0x3f];
    fval |= SP2[(work >> 24) & 0x3f];
    right ^= fval;
  }

  right = (right << 31) | (right >> 1);
  work = (leftt ^ right) & 0xaaaaaaaa;
  leftt ^= work;
  right ^= work;
  leftt = (leftt << 31) | (leftt >> 1);
  work = ((leftt >> 8) ^ right) & 0x00ff00ff;
  right ^= work;
  leftt ^= (work << 8);
  work = ((leftt >> 2) ^ right) & 0x33333333;
  right ^= work;
  leftt ^= (work << 2);
  work = ((right >> 16) ^ leftt) & 0x0000ffff;
  leftt ^= work;
  right ^= (work << 16);
  work = ((right >> 4) ^ leftt) & 0x0f0f0f0f;
  leftt ^= work;
  right ^= (work << 4);

  buf8[0] = (right>>24)&0xff;
  buf8[1] = (right>>16)&0xff;
  buf8[2] = (right>>8)&0xff;
  buf8[3] = (right)&0xff;
  buf8[4] = (leftt>>24)&0xff;
  buf8[5] = (leftt>>16)&0xff;
  buf8[6] = (leftt>>8)&0xff;
  buf8[7] = (leftt)&0xff;
}
