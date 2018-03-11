#pragma once

#include "UniqueHandle.h"

struct ImageListTraits
{
	typedef HIMAGELIST pointer;

	static HIMAGELIST invalid()
	{
		return nullptr;
	}

	static void close(HIMAGELIST value)
	{
		ImageList_Destroy(value);
	}
};

struct BitmapTraits
{
	typedef HBITMAP pointer;

	static HBITMAP invalid()
	{
		return nullptr;
	}

	static void close(HBITMAP value)
	{
		DeleteObject(value);
	}
};

struct IconTraits
{
	typedef HICON pointer;

	static HICON invalid()
	{
		return nullptr;
	}

	static void close(HICON value)
	{
		DestroyIcon(value);
	}
};

typedef unique_handle<ImageListTraits> HImageListPtr;
typedef unique_handle<BitmapTraits> HBitmapPtr;
typedef unique_handle<IconTraits> HIconPtr;