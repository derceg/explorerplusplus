Toolbars
========

**Explorer++** has 5 internal toolbars, any of which may be enabled
(made visible) by checking one of the
:doc:`menu <../menus/view/toolbars>` items. They are

.. list-table::
  :header-rows: 1

  * - **Toolbar**
    - **ID\***
    - **Customizable**
    - **Comments**
  * - :doc:`Address Bar <address>`
    - 1
    - no
    - set or displays the address of the currently active tab (folder)
  * - :doc:`Main <main>`
    - 0
    - :doc:`yes <customize>`
    - provides 19 (in the current version) common tools or actions which
      may be quickly accessed by clicking a button's icon
  * - :doc:`Bookmarks <bookmarks>`
    - 2
    - limited
    - displays selected bookmarks as buttons/icons along with their name
  * - :doc:`Drives <drives>`
    - 3
    - no
    - shows and accesses all available volumes (ie. drives)
  * - :doc:`Application <application>`
    - 4
    - yes
    - houses *shortcuts* to selected applications

\* IDs are maintained internal to **Explorer++**, but may be useful when
editing the Windows registry, or the "config.xml" (ie. :ref:`portable
<menus/tools/options/general:Run in portable mode>` settings) file.

Moving toolbars
~~~~~~~~~~~~~~~

When :doc:`unlocked <lock>`, he toolbars may be relocated (ie. *docked*)
and *resized*, and placed on separate *rows* or combined on a *row* to
save space.

.. image:: /_static/images/toolbars/toolbar_grip.png

Using a toolbar's *grip* (found at the left side of the toolbar), it may
be dragged-and-dropped to a new row, or moved along that row effectively
resizing the toolbar to its left. Moving a toolbar along a row is only
possible when there is more than one toolbar per row.

Dragging a toolbar **up** will place it on a row with another toolbar,
while dragging a toolbar **down** (to the bottom-most row) places it on
its own row.

Toolbar positions are preserved by **Explorer++** for the next session.

.. admonition:: Bug

  If the **Address Bar** has a second toolbar on the same line to the
  right of it, it may be partially obscured, even after restarting
  **Explorer++**.

*Chevrons*
~~~~~~~~~~

|chevron| |arrow_right| |chevron_list|

If there is insufficient room to display all toolbar items, a *chevron*
symbol (») is used; clicking on the *chevron* opens the rest of the
toolbar, as a menu.

.. |chevron| image:: /_static/images/toolbars/chevron.png
.. |arrow_right| image:: /_static/images/common/arrow_rt.gif
.. |chevron_list| image:: /_static/images/toolbars/chevron_list.png

.. toctree::
   :titlesonly:
   :caption: Contents

   address
   main
   bookmarks
   drives
   application
   lock
   customize