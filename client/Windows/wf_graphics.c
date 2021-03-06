/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Windows Graphical Objects
 *
 * Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/utils/memory.h>
#include <freerdp/codec/bitmap.h>

#include "wf_gdi.h"
#include "wf_graphics.h"

extern HINSTANCE g_hInstance; /* in wfreerdp.c */

HBITMAP wf_create_dib(wfInfo* wfi, int width, int height, int bpp, BYTE* data, BYTE** pdata)
{
	HDC hdc;
	int negHeight;
	HBITMAP bitmap;
	BITMAPINFO bmi;
	BYTE* cdata = NULL;

	/**
	 * See: http://msdn.microsoft.com/en-us/library/dd183376
	 * if biHeight is positive, the bitmap is bottom-up
	 * if biHeight is negative, the bitmap is top-down
	 * Since we get top-down bitmaps, let's keep it that way
	 */

	negHeight = (height < 0) ? height : height * (-1);

	hdc = GetDC(NULL);
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = negHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = wfi->dstBpp;
	bmi.bmiHeader.biCompression = BI_RGB;
	bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**) &cdata, NULL, 0);

	if (data != NULL)
		freerdp_image_convert(data, cdata, width, height, bpp, wfi->dstBpp, wfi->clrconv);

	if (pdata != NULL)
		*pdata = cdata;

	ReleaseDC(NULL, hdc);
	GdiFlush();

	return bitmap;
}

wfBitmap* wf_image_new(wfInfo* wfi, int width, int height, int bpp, BYTE* data)
{
	HDC hdc;
	wfBitmap* image;

	hdc = GetDC(NULL);
	image = (wfBitmap*) malloc(sizeof(wfBitmap));
	image->hdc = CreateCompatibleDC(hdc);

	image->bitmap = wf_create_dib(wfi, width, height, bpp, data, &(image->pdata));

	image->org_bitmap = (HBITMAP) SelectObject(image->hdc, image->bitmap);
	ReleaseDC(NULL, hdc);

	return image;
}

void wf_image_free(wfBitmap* image)
{
	if (image != 0)
	{
		SelectObject(image->hdc, image->org_bitmap);
		DeleteObject(image->bitmap);
		DeleteDC(image->hdc);
		free(image);
	}
}

/* Bitmap Class */

void wf_Bitmap_New(rdpContext* context, rdpBitmap* bitmap)
{
	HDC hdc;
	wfBitmap* wf_bitmap = (wfBitmap*) bitmap;
	wfInfo* wfi = ((wfContext*) context)->wfi;

	wf_bitmap = (wfBitmap*) bitmap;

	hdc = GetDC(NULL);
	wf_bitmap->hdc = CreateCompatibleDC(hdc);

	if (bitmap->data == NULL)
		wf_bitmap->bitmap = CreateCompatibleBitmap(hdc, bitmap->width, bitmap->height);
	else
		wf_bitmap->bitmap = wf_create_dib(wfi, bitmap->width, bitmap->height, bitmap->bpp, bitmap->data, NULL);

	wf_bitmap->org_bitmap = (HBITMAP) SelectObject(wf_bitmap->hdc, wf_bitmap->bitmap);
	ReleaseDC(NULL, hdc);
}

void wf_Bitmap_Free(rdpContext* context, rdpBitmap* bitmap)
{
	wfBitmap* wf_bitmap = (wfBitmap*) bitmap;

	if (wf_bitmap != 0)
	{
		SelectObject(wf_bitmap->hdc, wf_bitmap->org_bitmap);
		DeleteObject(wf_bitmap->bitmap);
		DeleteDC(wf_bitmap->hdc);
	}
}

void wf_Bitmap_Paint(rdpContext* context, rdpBitmap* bitmap)
{
	int width, height;
	wfBitmap* wf_bitmap = (wfBitmap*) bitmap;
	wfInfo* wfi = ((wfContext*) context)->wfi;

	width = bitmap->right - bitmap->left + 1;
	height = bitmap->bottom - bitmap->top + 1;

	BitBlt(wfi->primary->hdc, bitmap->left, bitmap->top,
		width, height, wf_bitmap->hdc, 0, 0, SRCCOPY);

	wf_invalidate_region(wfi, bitmap->left, bitmap->top, width, height);
}

void wf_Bitmap_Decompress(rdpContext* context, rdpBitmap* bitmap,
		BYTE* data, int width, int height, int bpp, int length, BOOL compressed, int codec_id)
{
	UINT16 size;

	size = width * height * (bpp / 8);

	if (bitmap->data == NULL)
		bitmap->data = (BYTE*) malloc(size);
	else
		bitmap->data = (BYTE*) realloc(bitmap->data, size);

	if (compressed)
	{
		BOOL status;

		status = bitmap_decompress(data, bitmap->data, width, height, length, bpp, bpp);

		if (status != TRUE)
		{
			printf("Bitmap Decompression Failed\n");
		}
	}
	else
	{
		freerdp_image_flip(data, bitmap->data, width, height, bpp);
	}

	bitmap->compressed = FALSE;
	bitmap->length = size;
	bitmap->bpp = bpp;
}

void wf_Bitmap_SetSurface(rdpContext* context, rdpBitmap* bitmap, BOOL primary)
{
	wfInfo* wfi = ((wfContext*) context)->wfi;

	if (primary)
		wfi->drawing = wfi->primary;
	else
		wfi->drawing = (wfBitmap*) bitmap;
}

/* Pointer Class */

void wf_Pointer_New(rdpContext* context, rdpPointer* pointer)
{
	HCURSOR hCur;
	unsigned char am[32 * 4];
	unsigned char xm[32 * 4];
	int i, j, ii;
	int width, height, bpp;

	width  = pointer->width;
	height = pointer->height;
	bpp    = pointer->xorBpp;

	if ((bpp != 1 && bpp != 8 && bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32) ||
			width > 32 || height > 32)
	{
		printf("wf_Pointer_New: Unsupported Cursor width = %u, height = %u, xorBpp = %u\n", width, height, bpp);
		return;
	}
	memset(am, 0, 32 * 4);
	memset(xm, 0, 32 * 4);
	for (i = 0; i < 32; i++)
	{
		ii = (bpp == 1) ? i : (height - 1) - i;
		for (j = 0; j < 32; j++)
		{
			if (freerdp_get_pixel(pointer->andMaskData, j, i, width, height, 1))
			{
				freerdp_set_pixel(am, j, ii, width, height, 1, 1);
			}
			if (freerdp_get_pixel(pointer->xorMaskData, j, i, width, height, bpp))
			{
				freerdp_set_pixel(xm, j, ii, width, height, 1, 1);
			}
		}
	}
	hCur = CreateCursor(g_hInstance, pointer->xPos, pointer->yPos, pointer->width, pointer->height, am, xm);
	((wfPointer*) pointer)->cursor = hCur;
}

void wf_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{
	HCURSOR hCur;

	hCur = ((wfPointer*) pointer)->cursor;
	if (hCur != 0)
		DestroyCursor(hCur);
}

void wf_Pointer_Set(rdpContext* context, rdpPointer* pointer)
{
	wfInfo* wfi;
	HCURSOR hCur;

	wfi = ((wfContext*) context)->wfi;
	hCur = ((wfPointer*) pointer)->cursor;
	if (hCur != NULL)
	{
		SetCursor(hCur);
		wfi->cursor = hCur;
	}
}

void wf_Pointer_SetNull(rdpContext* context)
{

}

void wf_Pointer_SetDefault(rdpContext* context)
{

}

/* Graphics Module */

void wf_register_graphics(rdpGraphics* graphics)
{
	rdpBitmap bitmap;
	rdpPointer pointer;

	memset(&bitmap, 0, sizeof(rdpBitmap));
	bitmap.size = sizeof(wfBitmap);
	bitmap.New = wf_Bitmap_New;
	bitmap.Free = wf_Bitmap_Free;
	bitmap.Paint = wf_Bitmap_Paint;
	bitmap.Decompress = wf_Bitmap_Decompress;
	bitmap.SetSurface = wf_Bitmap_SetSurface;

	memset(&pointer, 0, sizeof(rdpPointer));
	pointer.size = sizeof(wfPointer);
	pointer.New = wf_Pointer_New;
	pointer.Free = wf_Pointer_Free;
	pointer.Set = wf_Pointer_Set;
	pointer.SetNull = wf_Pointer_SetNull;
	pointer.SetDefault = wf_Pointer_SetDefault;

	graphics_register_bitmap(graphics, &bitmap);
	graphics_register_pointer(graphics, &pointer);
}
