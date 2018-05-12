Search...
---------

:kbd:`Ctrl` + :kbd:`F`

:kbd:`F3`

.. image:: /_static/images/mnu_tools/search.png

The Search dialog provides a quick and convenient way to locate files or
folders, based on a search *pattern* (partial name, etc.). The results
area provides standard *Shell* context menus for each item found,
allowing you to view, edit, copy, delete, etc. the files or folders.

.. note::

  The Search routine can **only** find files/folders by name or
  attributes; no searching for text **within** a file can be done.

Filename
~~~~~~~~

This field contains either

- the partial file name (or search *pattern*) to search for. Although
  :ref:`wildcard *patterns* <menus/selection/wildcard_select:About
  wildcard patterns>` may be used, it is not strictly necessary to do
  so; the search implemented here will match a **partial** filename
  (similar to the built-in Windows search tool) without using wildcard
  characters. A *drop-down* list is available with quick access to
  previous search *patterns*. If using a partial file name (or wildcard)
  *pattern* here, make sure the :ref:`Use Regular Expressions
  <menus/tools/search:Use Regular Expressions>` checkbox is not checked
  to prevent the pattern from being interpreted as a regular expression.

..

    or

- a regular expression to match. Using a regular expression requires the
  :ref:`Use Regular Expressions <menus/tools/search:Use Regular
  Expressions>` checkbox be checked.

.. tip::

  The **Filename:** field is also used for entering Regular Expressions,
  which can be quite long. Currently, this field does not scroll, so
  your long expression may not be possible to enter. Simply resize the
  whole window (larger) for more room in the edit box.

Directory
~~~~~~~~~

This field should contain the folder in which to perform the search, or
the parent folder, if the :ref:`Search Subfolders
<menus/tools/search:Search Subfolders>` box is checked. The button
beside the field opens a Windows *Browse For Folder* dialog (see
:ref:`here <menus/tools/options/general:Default new tab folder>` for a
picture of this Windows dialog), allowing the user to navigate to and
set the **Search Directory**. A *drop-down* list is available with quick
access to previous search directories.

The **Search Directory** defaults to the folder/directory of the
currently active tab.

Attributes
~~~~~~~~~~

Checking any :ref:`attributes
<menus/file/set_file_attributes:Attributes>` provides a filtering
mechanism; files searched must satisfy the Filename *pattern* AND the
attributes if they are to be included in the results.

Case Insensitive
~~~~~~~~~~~~~~~~

The state of this checkbox determines whether text case (i.e. *UPPER* or
*lower* case) will be considered during the search.

- **checked** - searches are case insensitive, that is, *UPPER* and
  *lower* case are considered the same. Searching for "gif", for
  example, will also find "GIF" and "Gif".
- **not checked** - searches are case sensitive and the target and
  results case must match exactly, that is, "gif" ≠ "GIF" ≠ "Gif".

Search Subfolders
~~~~~~~~~~~~~~~~~

Normally, the search would be done only in the scope of the folder shown
as the :ref:`Directory <menus/tools/search:Directory>`. However, if this
box is checked, all subfolders (and subfolders of subfolders, etc.) will
be included in the search. Results can be from either the search
Directory, or any subfolder.

Use Regular Expressions
~~~~~~~~~~~~~~~~~~~~~~~

If this box is checked, then the text entered in the :ref:`Filename
<menus/tools/search:Filename>` edit box is assumed to be a regular
expression and the search is done using pattern matching instead of
simple file name comparisons. Regular expressions, while appearing
complicated to the user of text and wildcard expressions (e.g.
"\*.jpg"), can be a more flexible and powerful approach to searching for
files and folders. Having this tool available - as well as
*conventional* (i.e. non regular expression) text searching - provides
superior capabilities and choice to the user. For help on regular
expressions, see `this
<https://msdn.microsoft.com/en-us/library/bb982727.aspx>`_ MSDN page.
Regular Expression searches obey the settings of the **Attributes** and
**Search type** checkboxes.

Results pane
~~~~~~~~~~~~

When the search is completed, this list shows the name and path (ie.
location) of all files found which matched the criteria for (partial)
name and attributes. The **Name** and **Path** columns are resizable,
and *sortable* (ascending or descending, by clicking on the column
header) for convenience when numerous results are visible.

.. tip::

  Each file in the Results pane has a context (ie. right-click) menu.
  Two items on this menu are of particular interest:

  - **Open** - this opens the file in any application registered to that
    file type or presents a dialog if the type is not registered. This
    may also be accomplished by double-clicking on the file.
  - **Open file location** - this opens a new tab at the folder which
    houses the file. The target file is already selected in the new tab.
    If multiple files were selected originally, only a single tab is
    opened and a single file selected.

  This context menu is constructed from Windows *shell* associations;
  items present will vary with the file type.

.. note::

  The Microsoft .NET framework, when present on a system, creates a
  subfolder in the Windows folder called "*assembly*". The nature of the
  .NET installation may interfere with **Explorer++** (Windows
  Explorer's exploration is *modified*) successfully browsing this
  folder. If this folder is included in your search (usually by virtue
  of using the :ref:`Search Subfolders <menus/tools/search:Search
  Subfolders>` option), some results unusual results (eg. incorrect file
  names, locations) may be expected, but these erroneous results are
  usually obvious.

Status:
~~~~~~~

This area shows

- the folder being searched (quickly!), as the search progresses,
- "Cancelled." - if the user stopped the search prematurely (by clicking
  the Stop button or by pressing **Enter** or **spacebar**),
  or
- the final count of folders and files found, when the search completes.