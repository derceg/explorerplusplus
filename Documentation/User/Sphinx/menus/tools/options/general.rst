General
-------

.. image:: /_static/images/nav/options-general.png

On Startup
~~~~~~~~~~

+-----------------+--------------------+
| System Default: | Load previous tabs |
+-----------------+--------------------+

This option specifies the action **Explorer++** should take on startup.
Two choices are available:

- **Load previous tabs** - This option forces **Explorer++** to save
  the current tabs and settings - either in the Windows registry, or in
  the :ref:`portable <menus/tools/options/general:Run in portable mode>`
  XML file - and restore them on startup. This ensures that your
  :doc:`Tab bar <../../../panes/tab_bar>` (and the settings for each
  tab) are "just the way you left them".

- **Load default folder** - This option tells **Explorer++** to just
  open the :ref:`default (new) tab <menus/tools/options/general:Default
  new tab folder>` at startup - **no** previous tabs will be loaded (but
  see Note and Tip below!).

The Info and Tip following are applicable to the current version (v1.35)
- future versions may vary from this behavior.

.. note::

  **Explorer++** saves any tabs and settings from memory variables when
  the application is closed; changing from the **previous tabs** setting
  to **default folder** in the Options dialog only changes the setting.
  At the next startup, however, the memory variables will contain only
  the tab information specified by the setting - the previous tab
  settings are **still** in the registry or XML file, at least until
  **Explorer++** is closed.

.. tip::

  If you have closed **Explorer++** and changed the setting to **Load
  default folder** (by accident?), you may still be able to restore your
  tabs for the next startup! One of the following changes (depending on
  whether you are using **Explorer++** in *portable mode* or not) must
  be made before the next session:

  Registry: The key

  ::

       HKEY_CURRENT_USER\Software\Explorer++\Settings\StartupMode

  must be set to 1 (restore tabs).

  XML file: The setting

  ::

       <Setting name="StartupMode">2</Setting>

  must be set to 1 (2=Load default folder). Settings in the XML file
  should be in alphabetical order; the StartupMode setting occurs around
  line 59. You can edit the file with a text editor like NotePad (not a
  word processor!).

  The usual *caveats* apply for Windows registry editing - don't attempt
  this if you are unsure.

Run in portable mode
~~~~~~~~~~~~~~~~~~~~

+-----------------+------------------------------------+
| System Default: | No (if no config file present)     |
+-----------------+------------------------------------+

.. _install_folder:

Enabling this option (ie. checking the box) tells **Explorer++** to save
and fetch all its settings to an XML file (a text file, structured
hierarchically to enable **Explorer++** to re-construct its
settings, etc. at program startup).

By default, the XML file is named config.xml and is located in the
installation folder, usually ...\\Program Files\\Explorer++ v1.2
(your version might be different). You can change where the config file
is found by setting the environment variable ``EXPLORERPP_CONFIG`` to
the full path to the file where you would like to store yours.  If the
variable points to an invalid location, the Windows registry will be
used as a fallback.

.. note::

  At startup, **Explorer++** always looks for a valid XML file in the
  correct location, and if it is found loads it and uses those settings.
  The actual setting value for portable mode **is not saved**, either in
  the Windows registry, or in the XML.

If **Explorer++** is currently using portable mode (ie. XML file), and
it is desired to switch back to using the Windows registry, the XML file
must be either

- deleted, or
- renamed (eg. config.xml.bak)

**and** the portable mode option must be disabled (ie. clear the
checkbox) prior to closing **Explorer++**, to avoid re-writing the XML
file. Note that **Explorer++** creates the XML file (if this setting is
enabled) immediately upon closing the options dialog.

Default new tab folder
~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-------------+
| System Default: | My Computer |
+-----------------+-------------+

This option allows the user to select a default folder to be used when a
:doc:`new tab <../../file/new_tab>` is opened. The option presents
a typical Windows *Browse For Folder* dialog, allowing the user to
choose a folder. Prior to using this option, the default folder is *My
Computer*.

.. image:: /_static/images/mnu_tools/browse_for_fldr.png

If a folder is set by this option, then subsequently is renamed or
deleted (ie. folder cannot be found), new tabs default back to *My
Computer*.

.. tip::

  The *Browse For Folder* dialog is a native Windows dialog and, as
  such, respects the settings from the **Control Panel/Folder Options**
  - **View** tab. Specifically, the

  - *Do not show hidden files and folders,* and the
  - *Hide protected operating system files (Recommended)* setting

  if enabled will prevent display of some folders by this dialog. This
  behavior exists even if **Explorer++** is set to :doc:`display hidden
  <../../view/show_hidden_files>` items, since **Explorer++** has no
  control over this dialog.

  It is recommended that both of the above settings be changed to
  display all files and folders when using **Explorer++**, and to use
  its own control of these attributes.

Language
~~~~~~~~

+-----------------+------------------------+
| System Default: | English (9) - internal |
+-----------------+------------------------+

.. image:: /_static/images/mnu_tools/languages.png

**Explorer++** is a native English language application, but
International users have contributed translation DLLs which, when placed
in the :ref:`installation folder <install_folder>`, present the menus,
etc. in a different language. The DLLs are not shipped with
**Explorer++**, but may be downloaded from the translation page of
**Explorer++**'s site at

https://www.explorerplusplus.com/translations

Once the DLLs are placed in the **Explorer++** installation folder, the
languages will appear as choices on the language drop-down (or drop-\
*up*!) control for this option.

.. note::

  **Japanese**, **Korean** and **Chinese** require support for East
  Asian languages to be installed. This installation is available from
  the Control Panel *Regional and Language Options* applet.

.. note::

  **Sinhala** requires installation of either the SinhalaTamil Kit
  (Windows XP) or possibly a Sinhala LIP (Language Interface Pack -
  Windows Vista/7, untested), available from `Microsoft Sri Lanka
  <http://www.microsoft.com/en/lk/>`_.

The translations are incomplete, but are an excellent start to the
*globalization* of **Explorer++**.

To check the status of a translation, update a translation or submit a
new translation, please see this project's page on `Crowdin
<https://crowdin.com/project/explorerplusplus>`_.