Delete
------

:kbd:`Delete`

This function deletes - with a confirmation dialog - selected files and
folders. Deleted items are sent to the Windows Recycle Bin and may be
recovered from there, if necessary.

:ref:`Read-only <menus/file/set_file_attributes:Read-only>` files or
folders are deleted as usual, but are identified as read-only by the
dialog; if a read-only file is among the selected files (not read-only)
it will require an additional confirmation.

:ref:`System <menus/file/set_file_attributes:System>` files receive a
similar warning, but may still be deleted (probably not a good idea!).

.. note::

  Files deleted from removable drives (eg. *flash* or *jump* drives are
  not sent to the Recycle Bin but are :doc:`deleted permanently
  <delete_permanently>` by Windows. Not all USB drives are considered
  removable drives; some larger drives install as local drives. Viewing
  the :doc:`properties <properties>` for a drive in the Folders pane
  will identify it as Local (fixed) or Removable.