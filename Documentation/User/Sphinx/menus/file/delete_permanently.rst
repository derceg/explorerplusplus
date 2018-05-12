Delete Permanently
------------------

:kbd:`Shift` + :kbd:`Delete`

This menu item :doc:`deletes <delete>` selected files and folders, but
does **not** send them to the Windows Recycle Bin. This is not a
*secure* delete; see :doc:`Destroy File(s)...
<../actions/destroy_files>` for secure deletion of files.

.. note::

  Files and folders deleted by Windows are not really removed
  (immediately) from the file system. Instead, the *File Allocation
  Table* (FAT16 and FAT32 file systems) or *Master File Table* (NTFS) is
  marked indicating that the file has been deleted, and its entry is
  cleared. The disk space occupied by the file is declared free for use,
  but the actual file data is not immediately removed. It can, however,
  be over-written; once that occurs, the file is usually unrecoverable,
  except possibly in part, by recovery utilities such as `Recuva
  <http://www.piriform.com/recuva>`_, a freeware product by Piriform.