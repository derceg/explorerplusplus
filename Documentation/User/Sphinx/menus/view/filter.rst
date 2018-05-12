Filter
------

Filtering allows certain files to be masked from view in the Files pane;
**only** the files you specify (and folders) will be visible. This helps
find files quickly, particularly in folders which contain a large number
of files, such as the *Windows* folder. Filtering can be applied on a
tab-by-tab basis.

Set Filter...
~~~~~~~~~~~~~

:kbd:`Ctrl` + :kbd:`Shift` + :kbd:`F`

.. image:: /_static/images/mnu_view/filter_set.png

The filter specified is in the form of a wildcard pattern, in a manner
similar to **Wildcard Select**. See :ref:`here
<menus/selection/wildcard_select:About wildcard patterns>` for a
description of wildcard patterns. When the pattern is set (by clicking
OK), then the filter is :ref:`applied <menus/view/filter:Apply Filter>`
automatically to the current tab and only those files which are a match
for the wildcard pattern are displayed.

The **Set Filter...** dialog *drop-down* shows previous wildcard
patterns used in filtering.

Apply Filter
~~~~~~~~~~~~

This menu item is a toggle which enables/disables filtering; filtering
(when enabled) always uses the last set wildcard pattern, as shown on
the **Set Filter...** (Filter Results) dialog.

When filtering is enabled, the following image is present in the center
of the Files pane:

.. image:: /_static/images/mnu_view/filtering_applied.gif

If filtering is disabled, the files which are now visible may be placed
at the top of the file list; a :doc:`Refresh <refresh>` may need to be
done in order to sort the files.