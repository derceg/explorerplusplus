Organize Bookmarks...
---------------------

:kbd:`Ctrl` + :kbd:`B`

.. image:: /_static/images/mnu_bookmarks/bookmarks_organize.png

Folders tree
~~~~~~~~~~~~

This pane shows the hierarchical *tree* structure of the bookmarks
collection. Folders can be expanded to show their contents (more folders
only). Selecting a particular folder displays the contents (including
bookmarks) in the **Bookmarks & Folders** pane to the right. The folder
**Bookmarks** is the parent (or root) folder of all subfolders or
bookmarks.

Bookmarks & Folders
~~~~~~~~~~~~~~~~~~~

This pane displays the contents of a selected folder in the **Folders
tree** to the left. In addition to the bookmark or folder's name, it's
location and any comment (Description) are displayed. The three columns
may be resized (by dragging the edges of the column headers), but not
dragged into a different order.

Double-clicking on a bookmark (|organize_bookmarks_icon|) transfers the
current tab to that location, ie. *goes* to the bookmark, while
double-clicking on a folder (|organize_bookmarks_folder_icon|) opens
that folder and displays its contents.

Context Menu
^^^^^^^^^^^^

.. image:: /_static/images/mnu_bookmarks/organize_bm_context.png

Each bookmark item in this pane has a context (ie. right-click) menu
which is quite similar to the context menu for items on the
:ref:`Bookmarks toolbar <toolbars/bookmarks:Context Menu (button)>`,
with the addition of a :ref:`Show on Bookmarks Toolbar
<menus/bookmarks/bookmark_tab:Show on Bookmarks Toolbar>` item.

.. admonition:: Incomplete

  The *New Bookmark...* and *New Folder...* items on this menu are not
  yet coded, ie. they are non-functional.

Move Up, Move Down
~~~~~~~~~~~~~~~~~~

These buttons move a selected bookmark (or folder) in the Bookmarks &
Folders pane up or down in the bookmarks list. Only a single bookmark
or folder may be selected at a time.

Delete
~~~~~~

This button deletes (with a confirmation dialog) a single bookmark or
folder selected in the Bookmarks & Folders pane. Deleting a folder
obviously deletes all the bookmarks and folders it contains.

.. _empty_folder_bug:

.. admonition:: Bug

  Deleting a folder does not update the Folders tree until the next
  opening of the **Organize Bookmarks** dialog.

  It is possible to create a new folder, then save the bookmark
  elsewhere or cancel saving the bookmark, thereby creating an empty
  folder. Attempting to subsequently delete an empty folder will cause a
  **fatal crash**.

  **Workaround:** Prior to deleting a folder, select it in the Folders
  tree to ensure that it is not empty. If necessary, add a dummy
  bookmark to the folder prior to deleting it.

Properties
~~~~~~~~~~

You edit the properties for any item selected in the Bookmarks & Folders
pane. If the item is a folder, then the **Location:** field is absent.

.. image:: /_static/images/mnu_bookmarks/bookmark_properties.png

.. |organize_bookmarks_icon| image:: /_static/images/icons/organize_bookmarks.png
.. |organize_bookmarks_folder_icon| image:: /_static/images/icons/new_tab.png