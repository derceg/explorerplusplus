Bookmarks Toolbar
-----------------

For general information about **Explorer++**'s toolbars, ie. moving,
resizing, customizing, etc., see the toolbars :doc:`Overview <index>`.

.. image:: /_static/images/toolbars/bookmarks_toolbar.png

The **Bookmarks** toolbar displays bookmarks (previously created) which
have been optionally designated to show on the toolbar by checking the
:ref:`Show on Bookmarks Toolbar <menus/bookmarks/bookmark_tab:Show on
Bookmarks Toolbar>` box. Clicking on the button for a bookmark opens
that bookmark (ie. location) in the current tab. Hovering over a
bookmark button displays the name and location of that bookmark as a
*tooltip*.

Bookmark :ref:`folders <menus/bookmarks/bookmark_tab:New Folder...>` may
also be shown on the **Bookmarks** toolbar; clicking on the button for a
folder opens that folder's bookmarks in a menu.

Context Menu (button)
~~~~~~~~~~~~~~~~~~~~~

Each button on the **Bookmarks** toolbar presents the following context
(ie. right-click) menu:

.. image:: /_static/images/toolbars/bookmark_btn_context.png

New Bookmark...
~~~~~~~~~~~~~~~

This function is *similar* to the :doc:`Bookmark This Tab...
<../menus/bookmarks/bookmark_tab>` item on the Bookmarks menu, but does
not operate on the current tab's location. Therefore, the dialog opens
with

- no bookmark name - the user must supply this, and
- no location - the location may be entered manually (if known) or
  copied/pasted from somewhere else (perhaps from the :doc:`Address Bar
  <address>` on another tab). Any bookmark created with no location will
  open in the :ref:`default new tab folder
  <menus/tools/options/general:Default new tab folder>`.

.. admonition:: Bug

  Bookmarks created by this routine do not show on the :doc:`Bookmarks
  menu <../menus/bookmarks/index>` until **Explorer++** is *re-started*.

.. tip::

  Using the middle-mouse button on a bookmark shown on the **Bookmarks**
  toolbar opens that bookmark in a new tab.