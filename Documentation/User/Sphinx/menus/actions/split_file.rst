Split File...
-------------

.. image:: /_static/images/mnu_actions/split_file.png

This function provides a useful operation - it splits the **selected**
file into equal sized *sub-files*. While this may not seem that useful
at first glance, many email providers, for example, have a limitation on
the size of attachment that can be sent with a single message. If the
file is over that size, then the server will automatically reject that
message entirely; the file can be split, sent in parts, then
*re-assembled* when received.

The companion function - :doc:`Merge Files... <merge_files>` - restores
the original file by combining the *sub-files*.

Split size:
~~~~~~~~~~~

Enter the split size in the left box; set the units in the right box
using the *pulldown*. Units are determined by the units *drop-down*
list to the right and may be

- Bytes,
- KB kilobytes = 1024 (2\ :sup:`10`) bytes,
- MB megabytes = 1024 KB or 1,048,576 (2\ :sup:`20`) bytes, or
- GB gigabytes = 1024 MB or 1,073,741,824 (2\ :sup:`30`) bytes.

The final size of the individual *sub-files* is a combination of the
size and units selected.

Output Filename:
~~~~~~~~~~~~~~~~

The output filename is actually a pattern for consecutive
naming/numbering of the output files; the string "/N" (capital N only!)
represents the *variable* numeric part of the future file names (e.g. 1,
2, 3, etc.). The default is <original filename>.part/N, which in the
sample dialog above (original file = NB200_D.zip) would create
*sub-files* of

- NB200_D.zip.part.1
- NB200_D.zip.part.2
- NB200_D.zip.part.3
- etc.

The *sub-file* naming pattern **must** contain a variable (i.e. /N) and
the numeric variable portion of the new files always starts with 1.

.. tip::

  Be careful with leading zeroes in the output filename; they are
  treated as constants. For example, using NB200_D.00/N as the pattern
  yields files NB200_D.001, NB200_D.002 but would also yield
  NB200_D.0010 instead of the *anticipated* NB200_D.010! **Explorer++**'
  s :doc:`Merge Files... <merge_files>` routine can handle this naming,
  but other *split/join* applications may not behave properly.

Output Folder:
~~~~~~~~~~~~~~

The output folder (where the sub-files will be placed) defaults to the
same folder as the target file. The folder may be chosen by typing (the
target folder must exist) or by using the ... button to the right which
opens a standard *Browse for folder* dialog (see :ref:`here
<menus/tools/options/general:Default new tab folder>` for a picture of
this Windows dialog).

Split
~~~~~

Clicking the Split button commences the operation, creating *sub-files*
of the specified size. The last file may not be the same size; it is
simply the remaining portion of the original file. Files are named the
same as the target file, but with a number appended, that is, if
**message.txt** is split, *sub-files* will be **message.txt.1**,
**message.txt.2**, and so on.

.. note::

  If **Split File...** involves creating a large number of files (e.g. >
  500), there may be delays in response, depending on the speed of your
  computer. Be patient - **Explorer++** **will** finish the operations
  properly.