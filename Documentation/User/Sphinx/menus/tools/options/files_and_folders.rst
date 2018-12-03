Files and Folders
-----------------

.. image:: /_static/images/nav/options-files_and_folders.png

Insert new items in their sorted positions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-----+
| System Default: | Yes |
+-----------------+-----+

**Explorer++** sorts items in the current Files pane (ie. tab) when a
:doc:`Refresh <../../view/refresh>` is done, or when refreshing is
done automatically by changing the column sorting, etc. When new items
are added to the tab by drag-and-drop, or by using the **New** item of
the context (right-click) menu, newly added items are (by default)
inserted in the correct location - folders at the top (or bottom,
depending on the column header status), files in position by alphabetic
order. Disabling this option, however, causes all new items to be
placed at the bottom the of current list. This may be of benefit when
editing folders, etc. and new files/folders can be determined at a
glance.

Of course, automatic refresh (and sorting) is still done when column
sorting is changed and at startup, etc.

Single-click to open an item
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+----+
| System Default: | No |
+-----------------+----+

Windows' default mode of operation (in most situations) is

- single-click on an item to select it (give it *focus*)
- double-click to activate it or execute it

The **X Window System** graphical user interface, popular with
Unix/Linux-type operating systems, uses a different approach

- hover over an item to select it (give it *focus*)
- click once to activate it or execute it

Enabling this option causes **Explorer++** to behave like an X Window
application - in the Files pane only. Some users may find this more
comfortable (less mouse clicks) or faster.

Hover time (ms)
+++++++++++++++

Hover time is the time taken for the item to be selected when using single click to open an item. Hovering over a item in single click to open mode is equivilent to clicking on it in double click to open mode. 

.. note::

  This behavior is available in Windows as a *system-wide* setting. The
  **General** page of the *Folder Options* Control Panel applet provides
  this functionality. **Explorer++**, as it is not a native Windows
  component, does not obey this setting.

Show folder sizes
~~~~~~~~~~~~~~~~~

+-----------------+----+
| System Default: | No |
+-----------------+----+

Checking this box forces **Explorer++** to calculate and display folder
sizes in the Files pane. The feature might be useful if, for example,
you were *house-cleaning* your drive and wanted to merge smaller folders
with larger ones; folder sizes would be listed is the **Size** column,
if :doc:`enabled <../../view/select_columns>`.

.. caution::

  Although it sounds obvious, when this option is enabled,
  **Explorer++** calculates folder sizes - **always**. If you are
  *walking* through your folder tree to get to a particular location,
  large folders in the files pane will be calculated, even if they are
  only momentarily displayed. Windows is a multi-tasking, multi-threaded
  operating system; calculations continue even though the folder is no
  longer displayed. This may **greatly** impact your system's
  performance - use this option with care.

Disable folder sizes on removable and network drives
++++++++++++++++++++++++++++++++++++++++++++++++++++

When folder size calculation and display is enabled, you may check this
box to turn off the feature for removable and network drives, as a
time-saving convenience. For network drives, this is a safety feature,
as the system may not know ahead of time just how large the network
drive is; time will be saved. In addition, network and removable drives
are transitory - it may not be useful to know the sizes since tomorrow,
the drives may not be connected.

Show all file sizes in:
~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-----------------------------------+
| System Default: | Flexible, Bytes if option enabled |
+-----------------+-----------------------------------+

.. image:: /_static/images/mnu_tools/filesize_flexible.png

When this box is cleared, drive and file sizes (and :ref:`folder sizes
<menus/tools/options/files_and_folders:Show folder sizes>`, if enabled)
are displayed in a *flexible* format, that is, in bytes, kilobytes (KB -
10\ :sup:`3` bytes), megabytes (MB - 10\ :sup:`6` bytes), etc.,
depending on file size.

.. image:: /_static/images/mnu_tools/filesize_menu.png

Checking the box allows the user  to choose from a variety of units,
from bytes up to terabytes (10\ :sup:`12` bytes) and even petabytes (10\
:sup:`15` bytes)! File sizes will be displayed in the chosen units, even
if files are small. This might prove useful in viewing the sizes of
larger drives, etc. or media files (DVD files could be in the gigabyte
(10\ :sup:`9` bytes) range.

Show user friendly dates
~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-----+
| System Default: | Yes |
+-----------------+-----+

Setting this option presents the

- Date Created,
- Date Modified and
- Date Accessed

:doc:`columns <../../view/select_columns>` in the Files pane with *user
friendly* dates, such as "Today, *time*" and 'Yesterday, *time*" instead
of the usual format. The usual format is determined by your system
settings for Date and Time display and can be modified in the **Regional
and Language Options** Control Panel applet.

You may open the Control Panel applet here: to customize your settings
(Short Date format, Time).

.. admonition:: Incomplete

  To date, only "Today" and "Yesterday" have been implemented as user
  friendly dates.
