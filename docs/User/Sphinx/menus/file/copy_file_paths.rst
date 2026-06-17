Copy File Paths
---------------

This function places the full (ie. *fully qualified*) paths to all
currently selected files - as shown in the files pane - on the
clipboard, for pasting into another application. If multiple file paths
are selected, the paths are delimited (ie. separated) by
carriage-return/linefeed characters; pasting multiple files to a text
document pastes them as separate line.

File paths are copied as Unicode text - see :doc:`here
<copy_folder_path>` for more comments.

Folder paths, ie. selected folders displayed in the files pane, are
copied the same as files.

Example
~~~~~~~

.. image:: /_static/images/mnu_file/files_paths_example.png

::

    H:\Projects\Explorer++\html\mnu_file\New Folder
    H:\Projects\Explorer++\html\mnu_file\clone_win.htm
    H:\Projects\Explorer++\html\mnu_file\close_tab.htm
    H:\Projects\Explorer++\html\mnu_file\copy_col_text.htm

.. note::

  Network resources - files, folders and printers, etc. on a network -
  are copied using :doc:`UNC <copy_universal_file_paths>` rules, rather
  than fully qualified paths on the local computer.