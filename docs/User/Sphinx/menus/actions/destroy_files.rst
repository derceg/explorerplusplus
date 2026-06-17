Destroy File(s)...
------------------

.. image:: /_static/images/mnu_actions/destroy.png

The **Destroy File(s)...** action securely deletes selected files by
overwriting their file space, preventing file recovery. The dialog
allows

- removal of files marked for deletion, and
- selection of either a 1-pass or 3-pass overwrite.

.. caution::

  The **Destroy File(s)** action is not the same as the :doc:`Delete
  Permanently <../file/delete_permanently>` routine!

Why use secure deletion?
~~~~~~~~~~~~~~~~~~~~~~~~

Today's computer stores **much** more than just games and documents;
many users access secure websites using password protection to

- buy and sell stocks and securities,
- manipulate their bank accounts and move money electronically
- pay bills
- make purchases using credit card information.

The computer of such a user likely contains records of such transactions
which, in the wrong hands, could lead to identity theft, or worse!
Although encryption is often used as a security measure, it has already
been shown that RSA public key encryption - once considered unbeatable
(even using sophisticated computers) in less than thousands of years -
may be hacked under certain conditions in as little as several days. In
short, we must always think about

- what is possibly on our computer - passwords, sensitive data, etc.,
- and how can we protect it.

Most of us know that :doc:`deleted <../file/delete_permanently>` files
can be recovered fairly easily by tools such as `Recuva
<http://www.piriform.com/recuva>`_, a freeware product. But today
computer forensic tools can be used to read residual magnetism on hard
drives, *possibly* re-constructing portions of data that *could* be used
unlawfully. Secure deletion of files - in a way which renders them
unrecoverable - is one simple way we can safeguard some of our personal
data.

.. note::

  USB flash drives, although not magnetic, may still be recovered in a
  manner similar to hard drives. Secure deletion can be useful for flash
  drives as well.

What does Destroy File(s) actually do?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Destroy File(s)** - secure deletion - routine performs the
following tasks on each file:

#. locates file data
#. determines the actual size of the file on disk, i.e. how many
   clusters are allocated to that file
#. opens the file, blocking any sharing mode, thus stopping the file
   from being opened (by another network user, for example) while it is
   being overwritten
#. extends the file out to the end of its last sector, eliminating file
   slack
#. starts at the beginning of the file, and writes in the first-pass
   data, 0x00 over the length of the whole file. This overwriting
   effectively eliminates the former contents of the file.
#. if **3-pass overwrite** has been chosen, then

   #. starts at the beginning of the file, and writes in the second-pass
      data - 0xFF - over the length of the whole file.
   #. then, writes in a *random* set of data, again byte by byte, as the
      third pass. This random data is obtained via the operating
      system's cryptography functions, and as such, is extremely
      difficult to duplicate.

#. flushes (ie. empties) all memory buffers connected to the file
#. closes the file and *frees* the Windows file handle used for the
   above procedures
#. deletes the file **without** sending it to the Recycle Bin.

Speed
~~~~~

**Destroy File(s)** writes data either once or three times, one byte at
a time, and is, therefore, somewhat *disk intensive*. What this means
is that the routine is not as fast as normal file writing. Calculation
of an estimated write time would be difficult, since it would be
dependent on a number of *uncertain* aspects, namely

- the type (number of cores, etc.) and speed of processor
- the type of drive (IDE, EIDE, SCSI, SATA, USB, flash, etc)
- fragmentation of the file - fragmented files contain clusters that
  may be physically farther apart
- amount of system memory - this affects the speed of system operations
  which indirectly could affect write times
- etc.

Attempting to use **Destroy File(s)** on a large file may create a
**very** long wait time, so the following estimates are included here
for reference and comparison:

.. _destroy_files_hardware:

**Hardware:** Pentium III processor, 1.2 Ghz, 768 Mb ram

**Operating System:** Windows XP SP2

**Drives:** 1: EIDE, 160 Gb; 2: USB, 1.0 Tb

.. _destroy_files_times:

.. list-table::
  :header-rows: 1

  * -
    - **Files**
    - **1**
    - **2**
    - **3**
  * -
    - **Sizes**
    - 1 Mb

      (1,048,576)
    - 10 Mb

      (10,485,760)
    - 100 Mb

      (104,857,600)
  * - **1-pass**
    - Drive 1

      Drive 2
    - ± 2.1 sec

      ± 1.8 sec
    - ± 19.6 sec

      ± 16.0 sec
    - ± 3.2 min

      ± 2.6 min
  * - **3-pass**
    - Drive 1

      Drive 2
    - ± 9.9 sec

      ± 8.5 sec
    - ± 1.6 min

      ± 1.4 min
    - ± 16.0 min

      ± 14.0 min

The times for the USB drive are shorter, I believe, owing to the larger
drive capacity (shorter seek time). To put these times into
perspective, for the listed hardware, one could estimate the secure
deletion times as

- 2 seconds/megabyte for 1-pass, and
- 10 seconds/megabyte for 3-pass.

This means that deleting a DVD image file (± 4 Gb), for example, using
3-pass secure deletion, could take as long as 11.5 hours! Sleep on it!

Early termination
~~~~~~~~~~~~~~~~~

On the :ref:`hardware <destroy_files_hardware>` listed above, the
Windows Task Manager reports that **Explorer++** is using between 95%
and 99% of the CPU time during a **Destroy File(s)** operation, despite
the fact that the process is operating at *Normal* priority. The effect
of this is that

- the appearance of **Explorer++** (not the **Destroy File(s)** dialog)
  is that of a program that has "stopped responding to the system", and
- attempting to switch to another application (via the taskbar) and do
  something else is a waste of time.

If **Destroy File(s)** is launched in error on a large file (see
:ref:`times <destroy_files_times>` above), it can be prematurely
terminated using the Task Manager by

#. opening the Task Manager using **Ctrl+Alt+Delete**
#. selecting the process **Explorer++.exe**
#. clicking *End Process*
#. agreeing to the warning dialog

The effect of this is to forcefully close **Explorer++** and, of course,
the **Destroy File(s)** dialog (a child window), freeing up your system
resources. However, take note that

- settings for **Explorer++** (ie. open tabs, views, etc.) may not have
  been saved, and
- the file(s) targeted for secure deletion were, in all likelihood,
  damaged (partially overwritten), but not fully deleted.

Disclaimer
~~~~~~~~~~

Although the **Destroy File(s)** action is deemed to be secure,
**Explorer++** is released under the GNU General Public `License
<https://www.gnu.org/licenses/gpl-3.0.en.html>`_ and no warranty as to
suitability is offered. Section 15 of the GNU license states that the
program is provided "as-is" without warranty of any kind.

In other words, if you use the **Destroy File(s)** routine, and you
**still** get busted for importing ivory when the police seize your
computer and find your files, you're on your own!