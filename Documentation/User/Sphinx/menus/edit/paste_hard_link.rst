Paste Hard Link
---------------

This menu item pastes Windows *hard links* to the files currently on the
clipboard, into the current folder as shown in the files pane. The files
would have been placed on the clipboard by :doc:`Copy <copy>` or
:doc:`Cut <cut>`. If **Cut** was used, then pasting shortcuts does
**not** delete the original file, as with :doc:`Paste <paste>`. Note
that folders **cannot** have hard links.

About Hard Links
~~~~~~~~~~~~~~~~

Hard links are a feature of the NTFS file system (ie. not available on
FAT16 or FAT32 drives) which allows creation of *virtual files* which
point to a true file. Multiple *virtual* files may point to the same
file, with each behaving as though it were the true file. The hard link
*points* to the MFT (Master File Table) entry for the true file and
propagates changes to the entry (except for the file name) to all other
hard links *pointing* to the same file.

The following diagram may explain this concept better, but may not be
completely accurate in its representation of the MFT entry or file
structure.

.. image:: /_static/images/mnu_edit/hard_link_diagram.png

In the diagram, the original name points to (and accesses) an MFT entry
which contains all file attributes (timestamps, read-only flag, etc.)
and in turn points to the actual file data. When the hard link is
created, it points to the **same** MFT entry, but could have a different
name. The **Explorer++** function - *Paste Hard Link* - always creates
the hard link with the original file name; obviously, then it cannot be
created in the original folder or else a naming conflict will exist.
Since the original name and hard link name use the same MFT entry, etc.,
changes to either file result in changes to the true file data and
attributes; the newly created hard link *file* is essentially a *true
file*! To remove a hard link, deleting either file name has the same
effect; the remaining (single) link becomes the *true file*.

In Windows, the NTFS file system counts the original name as hard link
#1 and the additional hard link as #2; all *normal* files, then would
have a hard link count of 1. The file system tracks the hard link count
for each file at all times; when the count becomes 0, then the file data
area is considered free space and may be reclaimed. It is interesting to
note, therefore, that when a file is deleted (to the Recycle Bin), it
would still have a count of 1 since only the name (and MFT entry?) have
been removed. It is not until the Recycle Bin is emptied that the file
is completely deleted, assuming its hard link count was 1, ie. no
additional hard links to the file exist.

Hard links may **only** be created for files on the same volume (ie. the
same drive letter); attempting to paste a hard link to a file on a
different volume results in a true copy of the file.

**Explorer++** can view the hard link count by enabling *Hard links* as
a :doc:`column heading <../view/select_columns>` in the files pane.

If you use hard links frequently, you might consider a useful freeware
tool - `Link Shell Extension
<http://schinagl.priv.at/nt/hardlinkshellext/hardlinkshellext.html>`_ -
which can manage hard links. When installed, it (among other things)
creates an additional :doc:`property sheet <../file/properties>` which
allow enumeration (ie. location) of any additional hard links to a file,
something that Windows can't do!

.. note::

  Hard links may also be created by the Windows command line tool
  :command:`Fsutil` (Windows XP) or :command:`Mklink` (Windows Vista or
  later).