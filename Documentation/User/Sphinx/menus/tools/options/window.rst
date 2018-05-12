Window
------

.. image:: /_static/images/nav/options-window.png

Allow multiple instances
~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-----+
| System Default: | Yes |
+-----------------+-----+

By default, **Explorer++** allows multiple instances of itself, that is,
you may open more a one copy of **Explorer++**. Each new copy can have
its own set of tabs and settings, etc., subject to information below.
If this option is disabled (box unchecked), ie. *do not allow multiple
instances*, then attempting to open a second copy of **Explorer++** (eg.
by double-clicking on a shortcut) will open a :doc:`new tab
<../../file/new_tab>` instead.

.. note::

  While multiple instances of **Explorer++** are open, each copy
  maintains tab and settings information in its own memory space.
  However, on closing, each copy will write complete settings
  information to the Windows registry (or :ref:`config.xml
  <menus/tools/options/general:Run in portable mode>` file). Closing the
  next copy will **also** do this, overwriting the settings from the
  first. This means that the **last** closed instance of **Explorer++**
  ultimately determines the settings (and tab information) saved for the
  next session. This behavior is by design.

.. tip::

  If you have multiple copies of **Explorer++** open, the one with your
  **preferred tabs** (and settings) should be **closed last**. If
  multiple instances are being used along with :ref:`portable
  <menus/tools/options/general:Run in portable mode>` mode, you can make
  a backup copy of your config.xml file.

Tab bar settings
~~~~~~~~~~~~~~~~

Always show the tab bar
+++++++++++++++++++++++

+-----------------+-----+
| System Default: | Yes |
+-----------------+-----+

Disabling this option causes the :doc:`Tab bar <../../../panes/tab_bar>`
to disappear (ie. not displayed) when only 1 tab is open. If 2 or more
tabs are open, this setting has no effect.

Show the tab bar at the bottom
++++++++++++++++++++++++++++++

+-----------------+----+
| System Default: | No |
+-----------------+----+

The :doc:`Tab bar <../../../panes/tab_bar>` is normally displayed at the
top of the Files pane; enabling this option (ie. checking the box)
displays tabs at the bottom, just above the :doc:`Status Bar
<../../view/status_bar>` (if enabled).

Extend tab bar across entire window
+++++++++++++++++++++++++++++++++++

+-----------------+----+
| System Default: | No |
+-----------------+----+

With this option disabled, the :doc:`Tab bar <../../../panes/tab_bar>`
extends only the width of the Files pane. Enabling this option displays
the Tab bar the full width of the window; more tabs can be displayed in
this mode.

Title bar settings
~~~~~~~~~~~~~~~~~~

These settings modify **Explorer++**'s title bar, which normally only
displays the current folder, followed by "Explorer++". A sample title
bar follows, showing only the current folder ("AtomExplorer").

.. image:: /_static/images/mnu_tools/titlebar_normal.png

Note that these options may be enabled in any combination. The more
options enabled, the longer the window title.

Show full path in title bar
+++++++++++++++++++++++++++

+-----------------+----+
| System Default: | No |
+-----------------+----+

Enabling this setting displays the full path to the current folder. The
following image demonstrates this.

.. image:: /_static/images/mnu_tools/titlebar_path.png

Show username in title bar
++++++++++++++++++++++++++

+-----------------+----+
| System Default: | No |
+-----------------+----+

Enabling this setting displays the current user's name (actually the
*owner* of the **Explorer++** process), preceded by the domain name. The
domain name refers to the location of the user's account, which could be
another computer on a network. In most cases, individual computers will
just show "DESKTOP" signifying this is a local account/user.

.. image:: /_static/images/mnu_tools/titlebar_username.png

Show privilege level in title bar
+++++++++++++++++++++++++++++++++

+-----------------+----+
| System Default: | No |
+-----------------+----+

Enabling this setting displays the user's *privilege level*, as
determined by the type of his account. This is usually one of

- **Administrators** - high privilege, unrestricted access to all
  computer functions
- **Power Users** - extensive rights, but may not be allowed certain
  administrative tasks (eg. take ownership of files)
- **Users** - fewer rights. This prevents accidental changes to the
  system.
- **Guests** - fewer rights than Users

.. image:: /_static/images/mnu_tools/titlebar_privilege.png

Full row selection in details view
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+----+
| System Default: | No |
+-----------------+----+

Normal selection of an item in details view just highlights the
left-most column (usually Name). In order to select that item, the
left-most column must be clicked; clicking on another column has no
effect.

.. image:: /_static/images/mnu_tools/select_normal.png

However, when this option is enabled, the full row is highlighted;
selection may be done on any column.

.. image:: /_static/images/mnu_tools/select_fullrow.png

Disable treeview delay
~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-----+
| System Default: | Yes |
+-----------------+-----+

**Explorer++** can, if desired, introduce a 500 millisecond delay
between selecting a folder in the Folders pane (ie. treeview control)
and opening its contents in the Files pane. This delay is entirely
*cosmetic*; it may prove useful for users who navigate the folder tree
using the keyboard. With the delay, using the up/down arrow keys would
not open the folders contents for each folder as the selection was moved
up or down.

By default, this option is enabled, disabling the treeview delay;
clearing the checkbox enables the delay.