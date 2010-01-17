#ifndef IDROPSOURCE_INCLUDED
#define IDROPSOURCE_INCLUDED

#include <shlobj.h>
#include "../ShellBrowser/iShellView.h"
#include "../Helper/FileOperations.h"

HRESULT CreateDropSource(IDropSource **ppDropSource,DragTypes_t DragType);

#endif