#pragma once

#include <windows.h>

/* used to set min align to 1 byte */
#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

/* RT_ICON resource info */
typedef PACK(struct _GRPICONDIRENTRY
{
	BYTE  bWidth;
	BYTE  bHeight;
	BYTE  bColorCount;
	BYTE  bReserved;
	WORD  wPlanes;
	WORD  wBitCount;
	DWORD dwBytesInRes;
	WORD  nId;
}) GRPICONDIRENTRY, *PGRPICONDIRENTRY;

/* RT_GROUP_ICON resource info */
typedef PACK(struct _GRPICONHEADER
{
	WORD Reserved;
	WORD ResourceType;
	WORD ImageCount;

	/* use PGRPICONHEADER + sizeof(GRPICONHEADER) */
	//GRPICONDIRENTRY idEntries[];
}) GRPICONHEADER, *PGRPICONHEADER;

/* RT_ICON resource info */
typedef PACK(struct _RSRCICON
{
	DWORD dwSize;
	DWORD dwWidth;
	DWORD dwHeight;
	WORD wPlanes;
	WORD wBitCount;
	DWORD dwCompression;
	DWORD dwSizeImage;
	DWORD dwXpelsPerMeter;
	DWORD dwYpelsPerMeter;
	DWORD dwClrUsed;
	DWORD dwClrImportant;
	BYTE ImageData[2304];
	BYTE BitMask[96];
}) RSRCICON, *PRSRCICON;