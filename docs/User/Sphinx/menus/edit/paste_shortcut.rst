Paste Shortcut
--------------

:kbd:`Ctrl` + :kbd:`Shift` + :kbd:`V`

This menu item pastes Windows *shortcuts* to the files and folders
currently on the clipboard, in the current folder as shown in the files
pane. The files and folders would have been placed on the clipboard by
:doc:`Copy <copy>` or :doc:`Cut <cut>`. If **Cut** was used, then
pasting shortcuts does **not** delete the original file, as with
:doc:`Paste <paste>`.

About Shortcuts
~~~~~~~~~~~~~~~

Shortcuts are small files (typically < 1000 bytes) - always with the
extension **.LNK** - which contain, among other things, the path to a
file or folder. A shortcut file behaves, for the most part, the same as
the file it references. That is,

- double-clicking a shortcut (or .LNK file) opens the target file
- using a context (or right-click) menu to Edit a shortcut, opens the
  target file for editing
- etc.

Creating shortcuts using this function always creates a .LNK file with
the *fully qualified* path (eg. C:\\folder\\file.ext) used, rather than
a *relative* path (eg. ..\\..\\folder2\\file2.ext), and may, therefore,
be moved anywhere on the local computer and will still function
correctly.

Since shortcuts are true files and only link to the target through a
(text) path, deletion or modification of a shortcut has no effect on the
target file.

A good use for shortcuts is to place in one location (ie. one folder) a
collection of shortcuts to files which are related in content or
purpose, but which are scattered over the whole computer.

Shortcuts may be edited by using the :doc:`Properties
<../file/properties>` function. Typically, the user may change

- the shortcut's name. When created, it has the same name as the target
  file, but this is not necessary.
- the application used to open the target file
- the shortcut's (not the target file's) :doc:`attributes
  <../file/set_file_attributes>`
- the target file, working folder (if any) and hotkey (if any)
- the shortcut's icon (default is the icon of the target file)
- security settings (ownership, permissions, etc.), if implemented (NTFS
  drives only)

Shortcuts may be created (by other means, such as <right-click> *Create
Shortcut*) to Windows special folders, such as *Control Panel*, which
are not true physical folders. The target for these types of shortcuts
is usually not editable. Also, installation of some applications may
create shortcuts to those applications which are not editable.