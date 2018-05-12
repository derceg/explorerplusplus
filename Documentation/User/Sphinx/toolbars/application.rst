Application Toolbar
-------------------

For general information about **Explorer++**'s toolbars, ie. moving,
resizing, customizing, etc., see the toolbars
:doc:`Overview <index>`.

.. image:: /_static/images/toolbars/app_toolbar.png

The **Application** toolbar shows a collection of your favorite
applications; each button shows the application name and icon, if any.
Buttons on the **Application** toolbar are similar to shortcuts to the
applications themselves, providing *instant* access to your favorite
tools. Unlike true Windows shortcuts, application toolbar buttons may
have **no** parameters.

Context Menu (button)
~~~~~~~~~~~~~~~~~~~~~

.. image:: /_static/images/toolbars/app_btn_context.png

Open
~~~~

Clicking this menu item opens the application (corresponding to the
button, of course). This function is the same as clicking the button.

.. tip::

  Files from the Files pane may be dragged-and-dro pped onto an
  application button to open them in that application. Selecting
  multiple files and using drag-and-drop will open all selected files,
  providing the application supports this behavior.

  **Example:** multiple text files may be dragged-and-dropped onto the
  application button for a multi-tab text editor to open all files.

New Application Button...
~~~~~~~~~~~~~~~~~~~~~~~~~

.. image:: /_static/images/toolbars/new_app_btn.png

Name
++++

The text entered here is the name for the application that could be
shown on the toolbar along with the icon (application specific or
Windows default).

Command
+++++++

This field is the URL to the executable for the application. You may
enter the URL manually, if it is known, or :ref:`browse
<toolbars/application:Open (find executable)>` for the executable using
the **Open** dialog. If the target file is not an executable (eg. a text
or *.txt* file), clicking the application's button on the Application
toolbar will open the file in any registered application.

Open (find executable)
++++++++++++++++++++++

This button opens a *standard* Windows dialog (Open) which allows the
user to browse for the application executable file. Note that the
default file type is EXE, but selecting All Files will browse for other
types of files (.com, .txt, etc.). The *Open as read-only* checkbox
serves no purpose.

Show name on application toolbar
++++++++++++++++++++++++++++++++

This box (checked by default) enables/disables the display of the
:ref:`Name: <toolbars/application:Name>` field for the target
application. Clear this box if the icon is self-explanatory, ie. the
nature of the application is clear.

.. note::

  This item (*New Application Button...*) also appears on the
  **Application** toolbar context menu (ie. right-click over blank space
  on toolbar).

Delete
~~~~~~

This menu item deletes the application button from the toolbar, with a
confirmation dialog. It cannot be recovered.

Properties
~~~~~~~~~~

This function opens the properties for the application *shortcut* and
button. A dialog (*Edit Application Button*) is opened which is
identical in function to the :ref:`New Application Button
<toolbars/application:New Application Button...>`, with all pertinent
fields (for the button) already filled in. Fields may be edited from
this dialog.

.. tip::

  **Re-ordering buttons on the toolbar** (advanced - be careful!)

  Application buttons are normally placed on the Application toolbar in
  the order they are created. Through judicious use of the Regedit
  application, they may be re-ordered.

  #. open Regedit and navigate to
     ``HKEY_CURRENT_USER\Software\Explorer++\ApplicationToolbar``
  #. select the **ApplicationToolbar** key in the left panel, and
     **Export** its contents to a suitable location using the context
     menu
  #. edit the file to renumber the sub-keys (application buttons). They
     must always be numbered 0, 1, 2, etc. The first button is 0, the
     second is 1, and so on. The actual order they appear in (inside the
     file) is not important.
  #. **make sure Explorer++ is closed**
  #. Double-click on the edited file to re-enter the data into the
     registry.
  #. That's it!

  A similar technique applies to using **Explorer++** as a portable
  application - edit the **config.xml** file to re-order the application
  button lines. A number is not used; the order of the lines (under
  <ApplicationToolbar>) determines the button order.