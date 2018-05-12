Customize Colors...
-------------------

This tool allows **Explorer++** to display files in different colors,
according to *rules* which modify the display of files by filename
and/or attributes. Like the :doc:`Filter <../view/filter>`
routine, coloring certain files or folders aids in quickly finding what
you want.

.. image:: /_static/images/mnu_tools/customize_colors.png

Rules list
~~~~~~~~~~

This list shows all color rules, by their description (ie. *name*).
Their application to files in the Files pane` is by their order of
appearance in this list; the first rule (top) is applied first, the
bottom of the list is applied last. Individual rules (ie. one at a time)
may be selected for re-ordering, editing or deletion.

New...
~~~~~~

This opens the **New Color Rule** dialog for creation of a new rule. The
dialog is virtually identical in form and function to the :ref:`Edit
Color Rule <menus/tools/customize_colors:Edit...>` dialog, buts adds to
the rule list rather than editing an existing rule.

Edit...
~~~~~~~

This function edits the currently selected rule.

.. image:: /_static/images/mnu_tools/customize_colors_edit.png

Description
+++++++++++

Enter a description in this field; this is the name that will identify
the rule in the :ref:`Rules list <menus/tools/customize_colors:Rules
list>`.

Color
+++++

The color assigned to files which match this rule is shown in a small
*swatch*.

Change color...
+++++++++++++++

This button opens a standard Windows Color dialog which allows you to
set a new color. You may

- choose a color from some common colors, or
- define one using

   - RGB (Red/Green/Blue) notation
   - Hue, Saturation and Luminescence values, or
   - visual selection

Filename pattern
++++++++++++++++

Rules may specify a filename pattern in addition to, or instead of,
:ref:`attributes <menus/tools/customize_colors:Attributes>`. If a file
name matches this pattern, then that portion of the rule is satisfied.
Filename patterns are entered using :ref:`wildcard patterns
<menus/selection/wildcard_select:About wildcard patterns>`, in a manner
similar to the old DOS wildcards. For example, the filename pattern

\*_*.\*

would match any file containing an underscore character. The most common
usage would likely be to match certain files by file extension, as in a
pattern of

\*.gif

which would match any GIF image files.

If attributes are also checked, then both the filename pattern AND the
attributes must match in order for the rule to succeed and the file be
colored.

Attributes
++++++++++

File **attributes** may be specified in addition to, or instead of, a
:ref:`Filename pattern <menus/tools/customize_colors:Filename pattern>`.
Attributes may be the *common* :ref:`attributes
<menus/file/set_file_attributes:Attributes>` (as can be changed from the
File menu), or may also be NTFS specific, namely

- **Compressed** - file is compressed using native Windows compression,
  or

- **Encrypted** - file is encrypted using native Windows encryption.

Note that checking any attribute box in this dialog does not affect the
attributes for any file, it merely reads the attributes as part of this
rule.

Move Up/Down
~~~~~~~~~~~~

The **Move Up** and **Move Down** buttons move a selected rule up or
down in the list. Priority of application of rules to the Files pane is
determined by their order in the list.

Delete
~~~~~~

Clicking the **Delete** button deletes a rule (currently selected) from
the list; a confirmation dialog is issued prior to deletion. There is
no way to recover a deleted rule.