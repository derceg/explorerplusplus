Status Bar
----------

.. image:: /_static/images/mnu_view/status_bar.png

The status bar presents some quick information and may be toggled on/off
from the **View** menu.

Items/text
~~~~~~~~~~

This area shows either

- the number of *items* (files and/or folders) and displays one of the
  following:

   - if no selection exists - total **item count** (eg. *24 items*), a
     count of items in the folders pane (current folder)
   - if a selection exists - total **selection count**, the number of
     items selected, or

- the *helper text* of a menu item when the mouse is hovered over that
  item.

  Example: hovering over the File/\ :doc:`New Tab <../file/new_tab>`
  menu item displays "Creates a new tab".

Size
~~~~

This area shows the actual item (files and/or folders) size and displays
one of the following:

- if no selection exists - total **file size**, the combined size of all
  files in the folder. Note that folder items are not included in this
  size, even if the :ref:`Show folder sizes
  <menus/tools/options/files_and_folders:Show folder sizes>` option is
  enabled.
- if a selection exists - total **selected file size**, the combined
  size of all selected files (not folders)
- if the current tab is a *virtual* folder (ie. special Windows folder)
  - the words **Virtual Folder**. Selecting items has no effect.

If a real size is shown (not a virtual folder), then the size is
affected by the :ref:`Show all file sizes in:...
<menus/tools/options/files_and_folders:Show all file sizes in:>`
setting.

Free space
~~~~~~~~~~

This area shows free disk space for the volume which houses the
currently active folder (files pane). Free space is shown in bytes
(rounded to MB, GB, etc., regardless of any options settings) and in
percentage of the entire volume.