Bookmark This Tab...
--------------------

:kbd:`Ctrl` + :kbd:`D`

.. image:: /_static/images/mnu_bookmarks/bookmark_new_expanded.png

Name:
~~~~~

The **Name** field allows entry of the final bookmark name, as it will
be displayed on the :doc:`Bookmarks menu <index>`. You should choose a
name that conveys (to you) the usage of this location.

Create In:
~~~~~~~~~~

.. image:: /_static/images/mnu_bookmarks/bookmark_createin.png

Bookmarks may be built in a hierarchy, similar to the tree shown in the
Folders pane, through use of the :ref:`New Folder...
<menus/bookmarks/bookmark_tab:New Folder...>` button. Bookmarks,
therefore, may be created on any level (ie. in any subfolder) which
exists. Clicking this field (or the *drop-down* button) opens a list
which shows all current levels; the user may select which folder or
level to create the new bookmark in, as in the image.

New Folder...
~~~~~~~~~~~~~

This dialog creates a subfolder in **Bookmarks** (the *root* level for
user bookmarks), or within any other bookmark subfolder which currently
exists. Note that folders may **contain** bookmarks, but do not have a
bookmark associated with them.

.. image:: /_static/images/mnu_bookmarks/bookmark_newfldr.png

.. admonition:: Bug

  Creation of a new folder (ie. one level below Bookmarks) is fine, but
  creation of second folder within the first (ie. two levels down)
  causes the folder names to be displayed incorrectly on the
  **Bookmarks** menu. All subfolders past the first level are shown with
  the same name - the name of the lowest level. **Workaround:** The
  :doc:`Organize Bookmarks <organize_bookmarks>` dialog displays the
  bookmark hierarchy and folder names correctly.

  During creation of a new folder on the same level, **Explorer++** does
  not check for duplicate names.

  See also - fatal error (bug), when :ref:`deleting <empty_folder_bug>`
  empty bookmark folders.

Location:
~~~~~~~~~

Normally, this field shows the URL (ie. location) of the currently
active folder in the Files pane, but you may edit it as desired. Make
sure that the location you edit actually exists.

.. note::

  Any bookmark with no location (ie. empty string) will open in the
  :ref:`default new tab folder <menus/tools/options/general:Default new
  tab folder>`.

Description:
~~~~~~~~~~~~

This field is normally not shown, but appears as part of the bookmark's
:ref:`Properties <menus/bookmarks/organize_bookmarks:Properties>` dialog
in the :doc:`Organize Bookmarks <organize_bookmarks>` tool. Any user
comments about this bookmark may be entered here, such as, perhaps:

- the use of this folder (eg. "Temporary storage for untested
  software", "Killer apps!")
- a date ("Wednesday's accounting updates")
- backup information ("These ZIP files have been archived")
- etc.

Show on Bookmarks Toolbar
~~~~~~~~~~~~~~~~~~~~~~~~~

Checking this box (or menu item) forces the bookmark created to be shown
on the :doc:`Bookmarks toolbar <../../toolbars/bookmarks>`. If this box
is not checked at the time of bookmark creation, it may be checked later
as part of the :ref:`Properties
<menus/bookmarks/organize_bookmarks:Properties>` dialog in the
:doc:`Organize Bookmarks <organize_bookmarks>` tool. Folders housing
bookmarks may also be designated to show on the Bookmarks toolbar.