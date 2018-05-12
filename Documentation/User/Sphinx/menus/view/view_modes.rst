View Modes
----------

The Files pane may present files and folders using a variety of display
methods. Each has its own merit, depending on the content being
displayed, and may display different amounts of file information.

**Views** may be also accessible from a button on the :doc:`Main toolbar
<../../toolbars/main>`.

.. _item_name_note:

.. note::

  In most views, the name may appear to be the primary display item.
  Actually, it the leftmost information column (as shown in
  :ref:`details <menus/view/view_modes:Details>` view) which is
  displayed! In :ref:`icons <menus/view/view_modes:Icons>` view, for
  example, the file size could be displayed instead. See :doc:`Select
  Columns... <select_columns>` for more information.

Thumbnails
~~~~~~~~~~

.. image:: /_static/images/mnu_view/view_thnails.png

This view displays thumbnail images for each file. Using **Thumbnails**
is primarily useful when displaying images; other files may simply
display a *generic* image assigned by Windows.

**Explorer++** presents a view of each folder, as well, showing a few
thumbnails of images (if any) contained therein.

Currently, the thumbnail size is fixed (internal to **Explorer++**) at
120x120 pixels. **Explorer++** does **not** cache thumbnails as Windows
Explorer does (optionally creates "Thumbs.db" file).

Tiles
~~~~~

.. image:: /_static/images/mnu_view/view_tiles.png

**Tiles** view displays the files and folders, using the Windows
*generic* images for file types. In addition, the file type and size is
displayed.

Files and folders are displayed in a manner similar to **Thumbnails**
- in as many columns as the width of the Files pane will allow.

Icons
~~~~~

.. image:: /_static/images/mnu_view/view_icons.png

**Icons** view uses embedded (resource) icons from files
and, as with **Thumbnails** and **Tiles**, displays the files and
folders in columns, as space allows.

**Icons** view is perhaps better suited to displaying executable
files, and icon files (of course). Icons are displayed in 32x32 bit
size.

.. note::

  Windows Vista / 7 presents 4 different sized icons for :doc:`View
  <index>` choices.

Small Icons
~~~~~~~~~~~

.. image:: /_static/images/mnu_view/view_smicons.png

The **Small Icons** view is similar to **Icons** view, but
the icons are displayed in 16x16 bit size allowing more files/folders
to be displayed in the same space.

.. admonition:: Bug

  The **Small Icons** view currently has some acknowledged deficiencies
  in layout behavior.

List
~~~~

.. image:: /_static/images/mnu_view/view_list.png

**List** view displays files using *generic* icons and a single piece of
information, usually the file name (see :ref:`above <item_name_note>`).
Items are displayed in columns to fit the available space.

Details
~~~~~~~

.. image:: /_static/images/mnu_view/view_details.png

The **Details** view shows textual information and generic icons, in a
single vertical list. It can display multiple columns of information, as
:doc:`set <select_columns>` by the user. This display method can yield
the most information, but usually takes up the most pane width. Columns
not displayed due to lack of pane width may be scrolled into view as
needed.

Items displayed in this view may be re-arranged using drag-and-drop.
This has no effect on their actual order in the folder, but may aid the
user in selections, editing, etc.