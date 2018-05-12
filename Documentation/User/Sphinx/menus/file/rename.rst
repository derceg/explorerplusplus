Rename
------

:kbd:`F2`

This routine is really two routines in one, depending on whether a
single file (or folder) is selected or :ref:`multiple <menus/file/rename:Multiple
items - Mass Rename>` items are selected.

Single item
~~~~~~~~~~~

.. image:: /_static./images/mnu_file/rename_single_item.png

**Explorer++** allows you to edit the name in an *in-place* edit box, keeping
the extension the same. The extension may be edited, but must be deleted then
re-typed or selected then over-written.

.. tip::

  Pressing F2 during a single item rename cycles between highlighting

    - the file name
    - the file extension the full name, including the extension

Multiple items - Mass Rename
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: /_static/images/mnu_file/rename_mass.png

The **Mass Rename** dialog allows you to build a new name for all files
selected using alphanumeric characters, combined with special character
combinations or *words*. The special *words* are visible in a *reminder*
box by clicking the arrow button to the right of the **Target pattern:**
edit box. Note that special *words* are case-sensitive and any
characters that do not form a special *word* are placed in the new
filename *as-is*.

+-----------------------------------+-----------------------------------+
| **/F**                            | resolves to the entire filename,  |
|                                   | as originally named. Same as      |
|                                   | **/B/E**                          |
+-----------------------------------+-----------------------------------+
| **/B**                            | resolves to the filename minus    |
|                                   | extension, example: for           |
|                                   | "about_help.htm", **/B** resolves |
|                                   | to "about_help"                   |
+-----------------------------------+-----------------------------------+
| **/E**                            | resolves to the file extension,   |
|                                   | including " **.** ", example: for |
|                                   | "title.htm", **/E** resolves to   |
|                                   | ".htm"                            |
+-----------------------------------+-----------------------------------+
| **/N**                            | resolves to an integer, beginning |
|                                   | with 0 for the first file         |
+-----------------------------------+-----------------------------------+

Typing the new file name pattern into the edit box shows the new files
previewed to the right of the existing names. Clicking OK completes the
rename operation.

.. note::

  :doc:`Undo <../edit/undo>` works on the entire list as a set; all files
  renamed by a Mass Rename as restored to their original names.