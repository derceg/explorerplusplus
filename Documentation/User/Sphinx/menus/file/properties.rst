Properties
----------

:kbd:`Alt` + :kbd:`Enter`

.. image:: /_static/images/mnu_file/properties.png

This function opens the Windows property sheets for the file or group of
selected files. Property sheets may vary, depending on your
configuration, and whether additional utilities are installed, but in
general should show information such as:

- **Type of file:** as recognized by Windows, eg. *Text document*
  (.txt), *PNG image* (.png), etc.
- **Opens with:** an application associated with the file, if any
- **Location:** folder containing the file
- **Size:** and **Size on disk:** actual file size (of contents, if
  folder is selected) and disk space used
- **Contains:** number of files and folders contained in a folder (if a
  single folder is selected only)
- :doc:`File attributes <set_file_attributes>` currently applied
- **Security** settings and permissions (NTFS only - may not be shown,
  depending on system settings)
- File **Summary:** comments, if any assigned to the file by the user or
  an application (NTFS only)
- **Customize:** allows you to change certain properties (eg. image,
  icon) of a folder or group of folders

Utilities such as `Hashtab <http://beeblebrox.org/>`_ (which displays
*hashes* for the file, such as CRC32 and MD5) may introduce additional
property sheets.

If multiple files are selected, many of the same properties are shown,
including **Size:** and **Size on disk:**, which now refer to the
collective size of all the files selected.

The **Properties** function is also available as part of the context (ie.
right-click) menu in the folder or files panes.

.. note::

  The Properties dialog can also be activated by
  **Alt**\ +\ *doubleclick* while hovering over a file or folder.