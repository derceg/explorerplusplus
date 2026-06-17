Copy Folder Path
----------------

:kbd:`Ctrl` + :kbd:`Shift` + :kbd:`P`

This function places the full (ie. *fully qualified*) path to the
current folder - whose contents are shown in the files pane - on the
clipboard, for pasting into another application. This may be useful for
pasting into text or word processing documents. This is equivalent to

- selecting the path shown in the :doc:`address bar
  <../../toolbars/address>`, then
- copying that to the clipboard, using the context menu, or
  **Ctrl**\ +\ **C**.

The folder path is copied to the clipboard as Unicode text (ie. *plain*
text) - the clipboard contents cannot be used to paste the **actual**
folder or its contents. Because this is a Unicode copy, foreign
characters, such as the *Beta* in the example, are represented
correctly. If Asian characters are included, your Windows system must
have support installed for "East Asian Languages" (*Control
Panel/Regional and Language Options*). In order to save true Unicode
information, pasting must be done into a text editor which is Unicode
compatible, like Windows Notepad.

Example
~~~~~~~

::

    H:\Projects\Explorer++\html\mnu_file\New ß Folder

.. note::

  Using this function on a *virtual* folder (eg. My Computer, not really
  a true directory) copies the path as a special *registry path* or GUID
  (globally unique identifier) - not the path as shown in the
  :doc:`Address Bar <../../toolbars/address>`.