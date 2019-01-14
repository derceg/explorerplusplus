#pragma once

#include <list>
#include <stack>

class CFileActionHandler
{
public:

	struct RenamedItem_t
	{
		std::wstring	strOldFilename;
		std::wstring	strNewFilename;
	};

	typedef std::list<RenamedItem_t> RenamedItems_t;
	typedef std::list<std::wstring> DeletedItems_t;

	CFileActionHandler();
	~CFileActionHandler();

	BOOL	RenameFiles(const RenamedItems_t &itemList);
	BOOL	DeleteFiles(HWND hwnd, const DeletedItems_t &FullFilenameList, BOOL bPermanent, BOOL bSilent);

	void	Undo();
	BOOL	CanUndo() const;

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

		RenamedItems_t renamedItems;
		DeletedItems_t deletedItems;
	};

	void	UndoRenameOperation(const RenamedItems_t &renamedItemList);
	void	UndoDeleteOperation(const DeletedItems_t &deletedItemList);

	std::stack<UndoItem_t>	m_stackFileActions;
};