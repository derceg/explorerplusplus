Wildcard Select...
------------------

:kbd:`Ctrl` + :kbd:`Shift` + :kbd:`S`

.. image:: /_static/images/mnu_selection/wildcard_select.png

This menu item allows the user to select files in the files pane (ie.
current folder) using a wildcard pattern. A dialog is opened which
allows the user to enter a new wildcard pattern, or use the *drop-down*
button and choose a previously used pattern. Clicking OK selects files
and folders based on that pattern. Repeated use of **Wildcard
Select...** is cumulative; additional files may be selected, adding to
the current selection.

Unlike :doc:`Select All Of Same Type <select_all_of_same_type>`, the
selection process is based on file name and extension.

About wildcard patterns
~~~~~~~~~~~~~~~~~~~~~~~

A wildcard pattern is a file specification which contains symbols
representing unknown characters, as well as *ordinary* alphanumeric
characters.

The two wildcards available here are:

- **\* (asterisk)**

  This character substitutes for zero or more characters. Some examples
  are:

  **\*file.txt** matches "somefile.txt", "my_file.txt", but not
  "my_files.txt" (intervening "s" prevents match).

  **btn\*.\*** matches "btn_23.png", "btnumbers.txt" and "btn-img.jpg",
  but not "mybtn.jpg" (leading "my" prevents match) or "btn12" ("."
  specified must exist)

- **? (question mark)**

  This character substitutes for any single character, which must exist.
  Some examples are:

  **final?.doc** matches "final1.doc", but not "final_chapter.doc" (?
  matches only 1 character, "_chapter" is not 1 character)

  **file.tx?** matches "file.txt", but not "file.tx" (last wildcard
  character must exist)