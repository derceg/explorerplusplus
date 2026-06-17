Address Bar
-----------

:kbd:`Alt` + :kbd:`D`

:kbd:`Ctrl` + :kbd:`L`

.. image:: /_static/images/toolbars/address_bar.png

For general information about **Explorer++**'s toolbars, ie. moving,
resizing, customizing, etc., see the toolbars :doc:`Overview <index>`.

The **Address Bar** serves 3 general purposes:

#. it **displays the icon** associated with the current folder (tab),
#. it **displays the address** of the current folder (tab); this address
   can be copied to the clipboard (if desired), and
#. it allows the user to **enter an address** and, using the
   :ref:`Go <toolbars/address:Go button>` button, **jump to that
   address**.

Folder icon
~~~~~~~~~~~

This area displays any folder icon associated with a particular folder;
the default is usually as shown in the image. Most special folders on
the :doc:`Go menu <../menus/go/index>` have distinct icons associated
with them and will display those icons here.

.. tip::

  Dragging and dropping the **Address Bar**'s icon to a folder (in the
  Folders or Files pane) - or to another tab - creates a Windows
  shortcut (ie. a *LNK* file) in that folder. The shortcut points to the
  original location.

Address/text field
~~~~~~~~~~~~~~~~~~

Address display
+++++++++++++++

If a location for the active tab is selected, by any :ref:`valid means
<menus/go/back:History>`, the address/text field of the **Address Bar**
displays the current address. If the address is too long to fit in the
space allotted (ie. the **Explorer++** window has been resized, and the
**Address Bar** is now too small to display the full address, or the
address is *deeply nested* in subfolders), the text in the address/text
field can be scrolled by placing the cursor in the field, then moving
the curser (left/right arrow, Ctrl+left/right arrow, etc.).

Text in the address/text field may be selected and manipulated (eg.
copied to the clipboard), by using Windows hotkeys (eg. Ctrl+C for
copy), or through the use of a small context (ie. right-click) menu. The
:doc:`Copy Folder Path <../menus/file/copy_folder_path>` operation
copies the entire address to the clipboard.

Address entry
+++++++++++++

You may enter an address in the address/text field and jump to that
address by pressing Enter or clicking the :ref:`Go button
<toolbars/address:Go button>`. The address must one of either

- a fully qualified path, ie. <drive letter>:\\<folder>\\<folder>\\...
  etc. An example might be:

  C:\\WINDOWS\\system32\\drivers

  Note that fully qualified paths should not end in the backslash (\\)
  character.

  .. tip::

    Every time you enter backslash (\\) character, **Explorer++** opens
    a special (resizable) window and displays a list of the contents
    (files and folders both) of the path as typed so far (if valid).
    You may select a folder from this list; **Explorer++** will jump to
    that folder. Selecting a file will attempt to open it in any
    *registered* application, the same as if you tried to open the file
    using a double-click (or single-click, with an :ref:`option
    <menus/tools/options/files_and_folders:Single-click to open an
    item>` set). A :doc:`refresh <../menus/view/refresh>` will restore
    the true address.

- the name of a subfolder in the current location. The folder must
  already exist. This is the only time **Explorer++** will accept less
  than a fully qualified path, except for special locations (following).

- the name of a special location. **Explorer++** recognizes the
  following locations in the **Address Bar** as special:

  +-----------------------+-----------------------+
  | **Special location**  | **Target**            |
  +-----------------------+-----------------------+
  | Desktop               | <*drive*>:\\Documents |
  |                       | and                   |
  |                       | Settings\\<*user*>\\D |
  |                       | esktop                |
  +-----------------------+-----------------------+
  | Documents             | <*drive*>:\\Documents |
  |                       | and                   |
  |                       | Settings\\<*user*>\\\ |
  |                       | My Documents          |
  +-----------------------+-----------------------+
  | Pictures              | <*drive*>:\\Documents |
  |                       | and                   |
  |                       | Settings\\<*user*>\\\ |
  |                       | My Documents\\My Pic\ |
  |                       | tures                 |
  +-----------------------+-----------------------+
  | Music                 | <*drive*>:\\Documents |
  |                       | and                   |
  |                       | Settings\\<*user*>\\\ |
  |                       | My Documents\\My Mus\ |
  |                       | ic                    |
  +-----------------------+-----------------------+
  | Videos                | <*drive*>:\\Documents |
  |                       | and                   |
  |                       | Settings\\<*user*>\\\ |
  |                       | My Documents\\My Vid\ |
  |                       | eos                   |
  +-----------------------+-----------------------+

  Entering one of the above locations (case-insensitive) in the
  address/text field and clicking the **Go** button (or pressing Enter)
  jumps to the target location. **Explorer++** also recognizes several
  virtual folders, as shown on the :doc:`Go menu <../menus/go/index>`,
  namely

  - *My Computer*
  - *Recycle Bin*
  - *Control Panel*
  - *Printers and Faxes* (opens in Windows Explorer)
  - *My Network Places*

  although, it is likely easier just to use the Go menu for these
  locations.

Adress drop-down
^^^^^^^^^^^^^^^^

The *drop-down* list for the **Address Bar** shows all subfolders
(folders only - no files) in the current location. You may click on any
folder in the list to put its name in the address/text field; pressing
Enter or clicking the **Go** button will take you there.

Go button
~~~~~~~~~

The **Go** button, when clicked, set the address for the current tab to
the address shown in the address/text field of the **Address Bar**.
Pressing Enter is the same as clicking the **Go** button when the
**Address Bar** has the focus. The **Go** button has a context (ie.
right-click) menu identical to the :doc:`Toolbars menu
<../menus/view/toolbars>`.

.. note::

  The Tab, Shift+Tab, F6 and Shift+F6 keys cycle the focus in
  **Explorer++** between the **Address Bar**, Folders pane and the Files
  pane.