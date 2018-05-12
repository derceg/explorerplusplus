Save Directory Listing...
-------------------------

This menu item performs a simple directory (folder) listing and saves it
as a text file. A *Save As* Windows dialog allows the user to save the
file in any location, under any name.

The listing only contains file/folder names, sorted in alphabetical
order; folders are identified with "(Folder)" appended. Note that the
file is saved in Unicode format (codepage = UTF-16 LE), with a BOM (byte
order mark = FFFE hex) header. Windows Notepad can read this file format
and can save the file is ANSI format, if desired.

Example
~~~~~~~

::

  Directory
  ---------
  H:\Projects\Explorer++\html\mnu_file

  Date
  ----
  2011-10-30, 7:39:58 AM

  Statistics
  ----------
  Number of folders: 0
  Number of files: 15
  Total size (not including subfolders): 42.7 KB

  Folders
  -------

  Files
  -----
  clone_win.htm
  close_tab.htm
  copy_col_text.htm
  copy_file_paths.htm
  copy_folder_path.htm
  copy_Ufile_paths.htm
  delete.htm
  delete_perm.htm
  exit.htm
  new_tab.htm
  properties.htm
  rename.htm
  save_dir_list.htm
  set_file_attr.htm
  show_cmd_prompt.htm

.. tip::

  More detailed lists can be obtained by opening a :doc:`Command Prompt
  <show_command_prompt>` window and using the Dir command.

  Example: ``dir /ogn > "Directory Listing.txt"``

  Type: dir /? for help on the **Dir** command.