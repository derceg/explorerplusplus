Back
----

:kbd:`Alt` + :kbd:`←`

**Explorer++** maintains a list of previous locations, or history, for
each tab. The **Back** (and :doc:`Forward <forward>`) items on the
**Go** menu can walk through this history, much like hyperlinks in HTML
browsers (eg. Mozilla Firefox).

**Back** returns the currently active tab to an older location, closest
to the current location in the history. If this item is disabled, the
Files pane (active tab) is displaying the oldest location in the
history, the location when the tab was first created, or when
**Explorer++** was first opened.

History
~~~~~~~

A tab's history can be thought of as a collection (or list) of locations
in the order they were visited, with the oldest at one end, and newest
at the other. Locations are added to history each time a location is
visited by

- clicking on a drive in the :doc:`Drives toolbar
  <../../toolbars/drives>`,
- clicking on a folder (or other item) in the Folders pane,
- double-clicking (or single click, with :ref:`Single click option
  <menus/tools/options/files_and_folders:Single-click to open an item>`
  enabled) on a folder in the Files pane,
- clicking on a Bookmark, either from the :doc:`Bookmarks menu
  <../bookmarks/index>` or the :doc:`Bookmarks toolbar
  <../../toolbars/bookmarks>`,
- clicking on an item from the :doc:`Go menu <index>`, **except** Back
  or Forward,
- clicking on the Up button on the :doc:`Main toolbar
  <../../toolbars/main>` (same function as on the Go menu), or
- entering an address in the :doc:`Address bar <../../toolbars/address>`
  which changes the location.

Executing **Back** or **Forward** simply walks the Files pane through
the current history for a tab; the functions do not add to, or subtract
from, the location history. It is possible that history locations may
be duplicated, if they were visited by one of the above methods more
than once. As **Back** or **Forward** is used (either from the **Go**
menu or the :doc:`Main toolbar <../../toolbars/main>`), items in history
are *re-distributed* between the *Back history* and *Forward history*;
Explorer++ maintains histories in both directions.