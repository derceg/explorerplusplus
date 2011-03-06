#ifndef FILEACTIONHANDLER_INCLUDED
#define FILEACTIONHANDLER_INCLUDED

#include <stack>

class CFileActionHandler
{
public:

	struct RenamedItem_t
	{
		std::wstring	strOldFilename;
		std::wstring	strNewFilename;
	};

	CFileActionHandler();
	~CFileActionHandler();

	BOOL	RenameFiles(const std::list<RenamedItem_t> &ItemList);
	BOOL	DeleteFiles(HWND hwnd,const std::list<std::wstring> &FullFilenameList,BOOL bPermanent);

	void	Undo();
	BOOL	CanUndo();

private:

	enum UndoType_t
	{
		FILE_ACTION_RENAMED,
		FILE_ACTION_COPIED,
		FILE_ACTION_MOVED,
		FILE_ACTION_DELETED
	};

	struct UndoItem_t
	{
		UndoType_t	Type;

		/* Pointer to a structure containing
		more detailed information about this
		operation. */
		void		*pInfo;
	};

	void	UndoRenameOperation(const std::list<RenamedItem_t> &RenamedItemList);
	void	UndoDeleteOperation(const std::list<std::wstring> &DeletedItemList);

	std::stack<UndoItem_t>	stackFileActions;
};

#endif