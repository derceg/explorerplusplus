Sort By
-------

.. image:: /_static/images/nav/mnu-view-sort_by.png

This menu selects a sort order for display of files and folders in the
Files pane.

Selecting a **Sort By** order cancels a :doc:`Group By <group_by>`
order. The **Sort By** menu is built by **Explorer++** based on the
columns currently enabled in :ref:`Details
<menus/view/view_modes:Details>` view; your menu may differ from the one
shown here. Selecting an item in this menu has the same effect as
clicking on the header for that column in the Files pane.

Sort order
~~~~~~~~~~

Current sorting target
++++++++++++++++++++++

The current sorting target must be an enabled column, which would be
shown in :ref:`Details <menus/view/view_modes:Details>` view. This may
be any column, but some are more useful that others. Sorting by
**Name**, for example, places items in alphabetical order, while sorting
using the **Owner** (as per Windows security and permissions settings)
column may yield results which are essentially unusable, since likely
all files have the same owner.

Following is a list of just a few common sorting targets available in
the current version of **Explorer++**. See :doc:`Select Columns...
<select_columns>` for a more complete list.

+-------------------+-------------------------------------------------+
| **Name**          | This is the file name, including extension      |
+-------------------+-------------------------------------------------+
| **Type**          | Windows registered file type (eg. JPEG image)   |
+-------------------+-------------------------------------------------+
| **Size**          | File size (actual, in bytes)                    |
+-------------------+-------------------------------------------------+
| **Date Modified** | Last date file was modified/changed             |
+-------------------+-------------------------------------------------+
| **Attributes**    | eg. read-only, hidden, system                   |
+-------------------+-------------------------------------------------+
| **Extension**     | Just the file extension (eg. *txt*)             |
+-------------------+-------------------------------------------------+

Note that the list of available sorting targets (ie. **Sort By** menu)
may change depending the active folder. Some special folders, such as
**My Computer**, for example, have a different list of sorting targets
(or display columns) than others.

Sort direction
++++++++++++++

Items for display may be sorted in

- **ascending** order, that is, alphabetically - *a* before *b*, smaller
  before larger, etc., or
- **descending** order, that is, alphabetically - *b* before *a*, larger
  before smaller, etc.,

depending on your preference. For example, with **Date Modified** as the
target, it may be more desirable to use **descending** order, thereby
placing the most recent items at the top of the list. If you are trying
to locate files with zero length, then using an **ascending** sort with
**Size** as the target places zero length files at the top of the list.

Since the sorting :ref:`target <menus/view/sort_by:Current sorting
target>` is always an enabled column (as would be shown in :ref:`Details
<menus/view/view_modes:Details>` view), the column heading reflects both
the target and direction, as shown in the vertical arrow.

.. figure:: /_static/images/mnu_view/ascend_name.png

  Name/Ascending

.. figure:: /_static/images/mnu_view/descend_name.png

  Name/Descending

More...
+++++++

This menu item opens the :doc:`Select Columns... <select_columns>`
dialog allowing you to enable additional columns, or disable existing
ones. Once the columns are managed, the Sort By menu must be re-opened
to set a different sorting :ref:`target <menus/view/sort_by:Current
sorting target>`.