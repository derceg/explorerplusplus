Select Columns...
-----------------

.. image:: /_static/images/mnu_view/sel_col.png

**Explorer++** allows you to set data column visibility for folder
listings on a **tab-by-tab** basis. Columns available also depends on
the current tab's :ref:`folder type <menus/view/select_columns:Folder
type>`. The settings here apply only to the current tab; future (new)
tabs adopt the settings from the :ref:`Default Columns...
<menus/tools/options/default_settings:Default Columns...>` setting.

For more information about column settings, see :ref:`here
<menus/tools/options/default_settings:About Default Columns and Column
Widths>`.

Columns available
~~~~~~~~~~~~~~~~~

This area shows the list of columns available for the :ref:`type
<menus/view/select_columns:Folder type>` of folder currently being
viewed; checking the checkbox beside the name enables the column in the
Files pane. This list of available columns may vary with the type of
folder currently being viewed (ie. the active folder for a tab). In
addition, while a particular column may be checked, not every file in
the folder may possess the type of data placed in that column. For
example, image files may show data in the **Width** and **Height**
columns, but text files will not. In addition, the columns is different
for virtual folders, etc, as outline in the lists following.

Column Order
~~~~~~~~~~~~

Columns are displayed (left to right in the Files pane) in the order
shown, from the top down. The order may be changed using the **Move
Up** and **Move Down** buttons.

Folder type
~~~~~~~~~~~

.. list-table::
  :header-rows: 1

  * - **Folder type/name**
    - **Comments**
  * - :ref:`Control Panel <menus/view/select_columns:Control Panel>`
    - Windows *virtual* folder: a collection of *applets* for modifying
      your system, etc.
  * - :ref:`General <menus/view/select_columns:General>`
    - This is a normal (ie. real) folder on your system. This type of
      folder comprises 99% of what you will view and presents,
      therefore, the most common list of columns.
  * - :ref:`My Computer <menus/view/select_columns:My Computer>`
    - Windows *virtual* folder: shows volumes (ie. drive letters) and
      some other virtual or system folders, such as *My Documents*,
      *Control Panel*, etc.
  * - :ref:`Network Connections <menus/view/select_columns:Network
      Connections>`
    - Windows *virtual* folder: This is actually the *Control Panel*
      applet which manages connections.
  * - :ref:`My Network Places <menus/view/select_columns:My Network
      Places>`
    - Windows *virtual* folder: This is actually the *Control Panel*
      applet which manages connections.
  * - :ref:`Printers and Faxes <menus/view/select_columns:Printers and
      Faxes>`
    - Windows *virtual* folder: list of printers, faxes and printer
      drivers connected to your system.
  * - :ref:`Recycle Bin <menus/view/select_columns:Recycle Bin>`
    - Windows *virtual* folder: collection of erased files which are
      still recoverable.

Following is a list of all columns available for the different types of
folders which can be viewed. Default columns (with a checkmark in the
**Def** column) and default column order, assigned by the system (ie.
order, left to right) ,are also shown. The user may change the default
widths (always 150 pixels) through the :doc:`Save Column Layout as
Default <save_column_layout>` menu item.

.. admonition:: Incomplete

  Columns marked as "**n/a**" (not available) may not be implemented as
  of this version of **Explorer++**, ie. they may be present but contain
  no data.

Control Panel
~~~~~~~~~~~~~

+------------+----------+---------------------------------------+
| Def        | Column   | Description                           |
+------------+----------+---------------------------------------+
| ✓ 1        | Name     | *applet* name                         |
+------------+----------+---------------------------------------+
| ✓ 2        | Comments | brief description of applet's purpose |
+------------+----------+---------------------------------------+

General
~~~~~~~

Applies to *real* files or folders. Not all tags apply to all file
types.

+-----------------------+-----------------------+-----------------------+
| Def                   | Column                | Description or        |
|                       |                       | Comment               |
+-----------------------+-----------------------+-----------------------+
| ✓ 1                   | Name                  | file or folder name   |
+-----------------------+-----------------------+-----------------------+
| ✓ 2                   | Type                  | Windows associated    |
|                       |                       | descriptive file type |
+-----------------------+-----------------------+-----------------------+
| ✓ 3                   | Size                  | file size or folder   |
|                       |                       | contents size (if     |
|                       |                       | enabled)              |
+-----------------------+-----------------------+-----------------------+
| ✓ 4                   | Date Modified         | date of last          |
|                       |                       | file/folder           |
|                       |                       | modification. Some    |
|                       |                       | applications may      |
|                       |                       | modify the file       |
|                       |                       | without changing this |
|                       |                       | attribute.            |
+-----------------------+-----------------------+-----------------------+
|                       | Attributes            | common file           |
|                       |                       | |attributes_link|     |
+-----------------------+-----------------------+-----------------------+
|                       | Size On Disk          | amount of disk space  |
|                       |                       | used by file          |
+-----------------------+-----------------------+-----------------------+
|                       | 8.3 Name              | file name in older    |
|                       |                       | (ie. DOS) 8 character |
|                       |                       | name, 3 character     |
|                       |                       | extension format      |
+-----------------------+-----------------------+-----------------------+
|                       | Owner                 | Windows security -    |
|                       |                       | owner of file or      |
|                       |                       | folder                |
+-----------------------+-----------------------+-----------------------+
|                       | Product Name          | executable - from     |
|                       |                       | Version               |
|                       |                       | |property_sheet_link| |
+-----------------------+-----------------------+-----------------------+
|                       | Company               | executable - as above |
+-----------------------+-----------------------+-----------------------+
|                       | Description           | executable - as above |
+-----------------------+-----------------------+-----------------------+
|                       | File Version          | executable - as above |
+-----------------------+-----------------------+-----------------------+
|                       | Product Version       | executable - as above |
+-----------------------+-----------------------+-----------------------+
|                       | Shortcut to           | target of shortcut    |
|                       |                       | (.LNK or .PIF) file   |
+-----------------------+-----------------------+-----------------------+
|                       | Hard links            | number of hard links  |
|                       |                       | associated with this  |
|                       |                       | file - see            |
|                       |                       | |hard_links_link|     |
|                       |                       | for more              |
+-----------------------+-----------------------+-----------------------+
|                       | Extension             | file extension (minus |
|                       |                       | *dot*)                |
+-----------------------+-----------------------+-----------------------+
|                       | Date Created          | date of file or       |
|                       |                       | folder creation       |
+-----------------------+-----------------------+-----------------------+
|                       | Date Accessed         | date of last access   |
|                       |                       | (read or write) of    |
|                       |                       | file or folder. On    |
|                       |                       | Windows XP this may   |
|                       |                       | only reflect the      |
|                       |                       | nearest hour. Also    |
|                       |                       | see                   |
|                       |                       | |date_accessed_link|  |
|                       |                       | below.                |
+-----------------------+-----------------------+-----------------------+
|                       | Title                 | user supplied - title |
|                       |                       | of file - entered in  |
|                       |                       | Summary property      |
|                       |                       | sheet                 |
+-----------------------+-----------------------+-----------------------+
|                       | Subject               | user supplied -       |
|                       |                       | subject of document - |
|                       |                       | entered in Summary    |
|                       |                       | property sheet        |
+-----------------------+-----------------------+-----------------------+
|                       | Author                | user supplied -       |
|                       |                       | author of document -  |
|                       |                       | entered in Summary    |
|                       |                       | property sheet        |
+-----------------------+-----------------------+-----------------------+
|                       | Keywords              | user supplied -       |
|                       |                       | associated key words  |
|                       |                       | - entered in Summary  |
|                       |                       | property sheet        |
+-----------------------+-----------------------+-----------------------+
|                       | Comment               | user supplied -       |
|                       |                       | comment - entered in  |
|                       |                       | Summary property      |
|                       |                       | sheet                 |
+-----------------------+-----------------------+-----------------------+
|                       | Camera Model          | EXIF data, recorded   |
|                       |                       | by camera in an image |
|                       |                       | file                  |
+-----------------------+-----------------------+-----------------------+
|                       | Date Taken            | EXIF data, recorded   |
|                       |                       | by camera in an image |
|                       |                       | file                  |
+-----------------------+-----------------------+-----------------------+
|                       | Width                 | width of image (bits) |
+-----------------------+-----------------------+-----------------------+
|                       | Height                | height of image       |
|                       |                       | (bits)                |
+-----------------------+-----------------------+-----------------------+
|                       |                       | **Note:** The         |
|                       |                       | following media tags  |
|                       |                       | **may** be associated |
|                       |                       | with audio, video or  |
|                       |                       | ? - tagging of media  |
|                       |                       | files allows many     |
|                       |                       | options!              |
+-----------------------+-----------------------+-----------------------+
|                       | Bit rate              | media (audio/video)   |
|                       |                       | file - bit rate       |
|                       |                       | associated with data  |
+-----------------------+-----------------------+-----------------------+
|                       | Copyright             | media (audio/video)   |
|                       |                       | file - internal tag   |
|                       |                       | identifying file as   |
|                       |                       | copyrighted           |
+-----------------------+-----------------------+-----------------------+
|                       | Duration              | media (audio/video)   |
|                       |                       | file - play time      |
+-----------------------+-----------------------+-----------------------+
|                       | Protected             | media (audio/video)   |
|                       |                       | file - identifies     |
|                       |                       | file as protected     |
|                       |                       | media                 |
+-----------------------+-----------------------+-----------------------+
|                       | Rating                | media (usually video) |
|                       |                       | - internal tag for    |
|                       |                       | rating                |
+-----------------------+-----------------------+-----------------------+
|                       | Album artist          | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Album                 | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Beats-per-minute      | media (audio) -       |
|                       |                       | internal tag          |
+-----------------------+-----------------------+-----------------------+
|                       | Composer              | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Conductor             | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Director              | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Genre                 | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Language              | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Broadcast date        | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Channel               | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Station name          | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Mood                  | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Parental rating       | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Parental rating       | media (video) -       |
|                       | reason                | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Period                | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Producer              | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Publisher             | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Writer                | media (video) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+
|                       | Year                  | media (audio) -       |
|                       |                       | internal tag (text)   |
+-----------------------+-----------------------+-----------------------+

.. |attributes_link| replace:: :ref:`attributes <menus/file/set_file_attributes:Attributes>`
.. |property_sheet_link| replace:: :doc:`property sheet <../file/properties>`
.. |hard_links_link| replace:: :ref:`here <menus/edit/paste_hard_link:About Hard Links>`
.. |date_accessed_link| replace:: :ref:`tip <date_accessed_tip>`

.. _date_accessed_tip:

.. tip::

  The *Date Accessed* timestamp may not be reliable in Windows XP. It is
  subject to a registry setting - *NtfsDisableLastAccessUpdate* - which
  does not exist, by default. To enable automatic updating of *Date
  Accessed,* open a command prompt window and execute the following
  command:

  ``fsutil behavior set DisableLastAccess 0``

  Executing this command creates the required registry entry and enables
  automatic updating of *Date Accessed*. Updating can be disabled by
  setting the value to 1.

My Computer
~~~~~~~~~~~

+-----------------------+-----------------------+-----------------------+
| Def                   | Column                | Description           |
+-----------------------+-----------------------+-----------------------+
| ✓ 1                   | Name                  | drive name (and       |
|                       |                       | letter) or special    |
|                       |                       | folder name           |
+-----------------------+-----------------------+-----------------------+
| ✓ 2                   | Type                  | Local Disk, CD Drive, |
|                       |                       | Removable Disk,       |
|                       |                       | Folder, etc.          |
+-----------------------+-----------------------+-----------------------+
| ✓ 3                   | Total Size            | size available on     |
|                       |                       | drive                 |
+-----------------------+-----------------------+-----------------------+
| ✓ 4                   | Free Space            | unused drive space    |
|                       |                       | available (bytes, GB, |
|                       |                       | etc.)                 |
+-----------------------+-----------------------+-----------------------+
|                       | Comments              | provided by Windows - |
|                       |                       | folder function,      |
|                       |                       | drive details, etc.   |
+-----------------------+-----------------------+-----------------------+
|                       | File System           | FAT (ie. FAT16),      |
|                       |                       | FAT32, NTFS, Unknown  |
+-----------------------+-----------------------+-----------------------+

Network Connections
~~~~~~~~~~~~~~~~~~~

+-------------+--------+-----------------------------------------------+
| Def         | Column | Description                                   |
+-------------+--------+-----------------------------------------------+
| ✓ 1         | Name   | name of connection, eg. Local Area Connection |
+-------------+--------+-----------------------------------------------+
| ✓ 2         | Type   | n/a (future feature?)                         |
+-------------+--------+-----------------------------------------------+
| ✓ 3         | Status | n/a (future feature?)                         |
+-------------+--------+-----------------------------------------------+
| ✓ 4         | Owner  | n/a (future feature?)                         |
+-------------+--------+-----------------------------------------------+

My Network Places
~~~~~~~~~~~~~~~~~

+-------------+----------+-------------------------------+
| Def         | Column   | Description                   |
+-------------+----------+-------------------------------+
| ✓ 1         | Name     | name of tool or applet        |
+-------------+----------+-------------------------------+
| ✓ 2         | Comments | description of tool or applet |
+-------------+----------+-------------------------------+

Printers and Faxes
~~~~~~~~~~~~~~~~~~

+-----------------------+-----------------------+-----------------------+
| Def                   | Column                | Description           |
+-----------------------+-----------------------+-----------------------+
| ✓ 1                   | Name                  | device or driver name |
|                       |                       | (descriptive name,    |
|                       |                       | not file name)        |
+-----------------------+-----------------------+-----------------------+
| ✓ 2                   | Documents             | number of documents   |
|                       |                       | in queue (ready for   |
|                       |                       | printing)             |
+-----------------------+-----------------------+-----------------------+
| ✓ 3                   | Status                | Ready, Printing,      |
|                       |                       | error status, etc. -  |
|                       |                       | driver dependent      |
+-----------------------+-----------------------+-----------------------+
| ✓ 4                   | Comments              | user assigned, or     |
|                       |                       | place there by        |
|                       |                       | installation          |
+-----------------------+-----------------------+-----------------------+
| ✓ 5                   | Location              | user assigned?        |
|                       |                       | network?              |
+-----------------------+-----------------------+-----------------------+
| ✓ 6                   | Model                 | brand name, model     |
|                       |                       | name                  |
+-----------------------+-----------------------+-----------------------+

Recycle Bin
~~~~~~~~~~~

+-----------------------+-----------------------+-----------------------+
| Def                   | Column                | Description           |
+-----------------------+-----------------------+-----------------------+
| ✓ 1                   | Name                  | file or folder name   |
+-----------------------+-----------------------+-----------------------+
| ✓ 2                   | Original Location     | original file or      |
|                       |                       | folder location       |
+-----------------------+-----------------------+-----------------------+
| ✓ 3                   | Date Deleted          | date the file or      |
|                       |                       | folder was deleted    |
+-----------------------+-----------------------+-----------------------+
| ✓ 4                   | Size                  | file size or folder   |
|                       |                       | contents size (if     |
|                       |                       | enabled)              |
+-----------------------+-----------------------+-----------------------+
| ✓ 5                   | Type                  | Windows associated    |
|                       |                       | descriptive file type |
+-----------------------+-----------------------+-----------------------+
| ✓ 6                   | Date Modified         | date of last          |
|                       |                       | file/folder           |
|                       |                       | modification          |
+-----------------------+-----------------------+-----------------------+