Show Hidden Files
-----------------

:kbd:`Ctrl` + :kbd:`H`

Clicking on this item toggles display of hidden files - on or off.
Hidden files are those which have the :ref:`hidden attribute
<menus/file/set_file_attributes:Hidden>` set; they are *usually* masked
from display if marked as hidden. This setting can be changed on a
tab-by-tab basis; new tabs are created with this setting enabled.

In the Files pane (only), hidden files and folders are displayed with
their accompanying icons *ghosted* (ie. *washed out*).

The *Control Panel/Folder Options/View* setting - **Show (Do not show)
hidden files and folders** - has no effect on display of hidden items
(see :ref:`here <menus/tools/options/general:Default new tab folder>`);
**Explorer++** controls hidden visibility for itself.

.. _superhidden_tip:

.. tip::

  System files and folders may be *superhidden* - that is, they have
  **both** the Hidden and System attribute set - and are not displayed,
  even if **Show Hidden Files** is checked. To allow *superhidden* files
  and folders to be affected by this function, you must uncheck the
  *"Hide protected operating system files"* setting in the :doc:`Files
  and Folders tab <../tools/options/files_and_folders>` of
  **Explorer++**'s options.

  Display of *superhidden* files is contingent on a Windows system
  setting being enabled; you must uncheck the *"Hide protected operating
  system files"* setting in the **Folder Options** applet (*View* tab).
  The **Folder Options** applet is found in the **Control Panel**. Note
  that **Explorer++** can control visiblility of *ordinary* hidden files
  (Hidden attribute, but **not** System attribute) **without** enabling
  the "*Show hidden files and folders*" option in the **Control Panel**.