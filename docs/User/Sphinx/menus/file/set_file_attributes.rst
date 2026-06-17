Set File Attributes...
----------------------

.. image:: /_static/images/mnu_file/chg_attributes.png

This dialog allows the user to change the file attributes, including
system time stamps, of selected files and folders.

Change date manually
~~~~~~~~~~~~~~~~~~~~

You may select each of the three fields (month/day/year, shown here),
then type a new value. For example, selecting the day field and typing
23 changes the day to the 23rd of the month shown.

.. note::

  The individual fields may not be the same as shown here - they are
  determined by the setting for Short date format, found in the *Control
  Panel* / :bolditalic:`Regional and Language Options` applet.

Calendar tool
~~~~~~~~~~~~~

.. image:: /_static/images/mnu_file/calendar.png

The calendar tool allows you to quickly select a date. You may

- click any **day** of the month to select the day
- cycle through **months** by clicking the left/right buttons
- click the **month** (eg. August) for a *drop-down* list of months and
  select one
- click the **year** (eg. 2010) for a *spinner* control to adjust the
  year
- click the **Today:** line at the bottom to select *Today* (ie. today's
  date)

Change time manually
~~~~~~~~~~~~~~~~~~~~

As with the manual date change, you should select a field, then type a
new value. Type "A" or "P" to change to AM or PM.

Attributes
~~~~~~~~~~

The checkboxes may be checked (set attribute) or cleared (clear
attribute) to affect the following attributes:

Archive
+++++++

The archive attribute is used to determine whether a file or folder has
been changed. Whenever a file (not folder) is created or saved, the
archive attribute is set. Most backup programs may be configured to
clear the archive attribute as files are backed up.

Read-only
+++++++++

This attribute marks a file or folder to prevent overwriting or
deletion. Obeying the status of the **Read-only** attribute is the duty
of individual programs using those files; some may ignore any read-only
status. In general, the Windows system will warn you before doing
deletion or overwriting of **Read-only** files.

Hidden
++++++

Assigning the hidden attribute flags a file or folder to prevent it from
:doc:`being shown <../view/show_hidden_files>` during normal file
listings, providing **Explorer++** (or Windows Explorer) is not set to
view hidden files. Command line operations (eg. :command:`Dir`) also obey
the setting of the hidden attribute.

System
++++++

Windows tries to protect system files (necessary for system operation)
in a number of ways, one of which is to assign the system attribute.
When this attribute is applied, some operations will respect it and
alter their function, ie. refuse deletion. In most cases, this behavior
can be over-ridden. Windows can also set the **Hidden** attribute on a
system file, making it :ref:`superhidden <superhidden_tip>`.

Indexed
+++++++

Starting with Windows 2000, the indexing service creates indices of
files for speed up of searches. With this attribute set, the file is
marked for inclusion in the index.

.. note::

  The **Date Accessed** (or **Accessed date** in the dialog) attribute
  may be considered to be unreliable as:

  - it is not present on FAT16 file systems,
  - has no time component (ie. resolution of 24 hours) on FAT32 file
    systems,
  - may not be transferred correctly from system to system, and
  - may not be enabled on systems running Windows XP. See :ref:`here
    <date_accessed_tip>` for a tip on enabling this feature.

  The **Indexed** attribute is not supported by FAT16 and FAT32 file
  systems.