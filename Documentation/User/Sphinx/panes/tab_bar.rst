Tab bar
-------

The **Tab bar** displays tabs which are currently available for use;
each tab is essentially a separate Files pane, viewing a specific
location.

.. image:: /_static/images/pane_tabs/tab_bar.png

The user may quickly switch from location to location simply by
selecting the appropriate tab.

Each tab has a (right-click) :ref:`context menu <panes/tab_bar:Context
menu>`.

Viewing tabs
~~~~~~~~~~~~

If more tabs are open than will fit on the tab bar (depending on current
sizing of **Explorer++**'s window and other panes), a set of scrolling
buttons are available, allowing you to scroll left or right to
view/select other tabs which are currently not visible.

.. tip::

  The Tab bar may be extended to the full window width using an
  :ref:`option <menus/tools/options/window:Extend tab bar across entire
  window>`.

.. note::

  Hovering the mouse over a tab displays it's true location, regardless
  of the tab's title. Tab's opened at virtual folders show their *CLSID*
  (class ID), Windows' internal representation of their location.
  Practically, these *CLSID*\ s are of no use, except to a programmer.

Opening new tabs
~~~~~~~~~~~~~~~~

A new tab may be opened using a variety of methods, as listed :ref:`here
<new_tab_tip>`. From the **Tab bar**, the easiest methods may be

- double-clicking empty space on the bar. This opens a file displaying
  the :ref:`default contents <menus/tools/options/general:Default new
  tab folder>`.
- using the tabs :ref:`context <panes/tab_bar:Context menu>` menu

Closing tabs
~~~~~~~~~~~~

.. image:: /_static/images/pane_tabs/tab_bar_closebtn.png

When hovering the mouse over the "X" on the right side of the **Tab
bar**, it becomes a "Close" button which will close the current tab.
Tabs may also be closed by double-clicking on a tab, subject to an
options :ref:`setting <menus/tools/options/tabs:Double-click to close a
tab>`. Other methods of closing a tab are:

- from the :doc:`File menu <../menus/file/close_tab>`
- from the tab's context menu. The tab need not be the current (ie.
  selected) tab.

.. tip::

  Be careful closing tabs - the **current** tab may not be visible! A
  tab closed in error must be re-opened and re-configured, as necessary.

Re-arranging tabs
~~~~~~~~~~~~~~~~~

Tabs may re re-arranged using *drag-and-drop*, that is

- click on a tab and hold the left mouse button down
- move the cursor to the new (desired) position on the Tab bar
- release the left mouse button

Moving files and folders
~~~~~~~~~~~~~~~~~~~~~~~~

Files and folders in a tab's file view (ie. the Files pane) may be moved
from one folder to another simply by *dragging* and *dropping* them onto
a tab. If you pause slightly before *dropping* the items onto the tab,
that tab will become active, but this is not necessary in order to
complete the move.

Context menu
~~~~~~~~~~~~

.. image:: /_static/images/pane_tabs/tab_bar_context.png

Duplicate Tab
+++++++++++++

This item opens/creates a :doc:`new tab <../menus/file/new_tab>` on the
tab bar, but duplicates the tab from which this item was called.
Placement of the tab is dependent on an :ref:`option
<menus/tools/options/tabs:Open new tabs next to the current one>`
setting.

.. note::

  Duplicating a tab does not duplicate it's location :ref:`history
  <menus/go/back:History>`.

Open Parent in New Tab
++++++++++++++++++++++

This item opens/creates a new tab on the tab bar, using the location of
the current folder's immediate parent. This is the folder that *houses*
the current folder.

.. note::

  The :doc:`Desktop <../menus/go/desktop>` has no parent; this operation
  will **not** open a new tab if the current location is Desktop.

Refresh All
+++++++++++

This item performs a :doc:`refresh <../menus/view/refresh>` on all tabs
currently open.

Rename Tab...
+++++++++++++

.. image:: /_static/images/pane_tabs/tab_rename.png

Ordinarily, a newly created tab assumes the name of the actual folder it
displays (minus the drive and path information). This item, however,
allows you to rename the tab to something more suitable to your needs.
For example, while working on a programming project, you may have a tab
open at "C:\\Project\\today\\revisions\\code", but other folders of the
name "code" may also exist for that project (eg. different versions,
dates). You can rename the folder to "Today's code" by selecting this
item, choosing "Use custom name:" and typing in "Today's code". Renamed
tabs are *persistent*, that is, they will remain renamed even during
other sessions.

.. note::

  On first opening the *Rename Tab* dialog, the "Use custom name:"
  setting is always selected as a convenience; it is assumed that you
  want to set a custom name.

Lock Tab
++++++++

.. image:: /_static/images/pane_tabs/tab_locked.png

This item locks the tab, preventing it from being closed. The tab's icon
is changed, and the menu item becomes checked, reminding the user that
the tab is locked. Locked tabs can still be :ref:`re-arranged
<panes/tab_bar:Re-arranging tabs>` or :ref:`renamed
<panes/tab_bar:Rename Tab...>`. Locked tabs must be manually unlocked to
restore default behavior. Tab locked status is preserved between
sessions.

Lock Tab and Address
++++++++++++++++++++

This item locks the tab, as above (ie. can't be closed), but also
prevents the address from being changed by double-clicking on a folder
or drive. A new tab is opened instead, displaying the desired new
address.

Close Other Tabs
++++++++++++++++

This item closes all tabs on the Tab bar **except** the tab whose
context menu you are using, even if that tab is not active. Of course,
when all other tabs are closed, the remaining tab becomes active.