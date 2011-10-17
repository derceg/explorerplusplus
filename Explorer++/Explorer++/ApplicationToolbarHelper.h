#ifndef APPLICATIONTOOLBARHELPER_INCLUDED
#define APPLICATIONTOOLBARHELPER_INCLUDED

struct ApplicationButton_t
{
	/* External. */
	TCHAR	szName[512];
	TCHAR	szCommand[512];
	BOOL	bShowNameOnToolbar;

	/* Internal. */
	ApplicationButton_t *pNext;
	ApplicationButton_t	*pPrevious;
	int		iImage;
};

#endif