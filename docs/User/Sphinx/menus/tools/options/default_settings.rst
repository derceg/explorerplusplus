Default Settings
----------------

.. image:: /_static/images/nav/options-default_settings.png

Default Columns...
~~~~~~~~~~~~~~~~~~

+-----------------------+--------------------------+
| System Default:       | see |folder_types_link|, |
|                       | varies with type         |
+-----------------------+--------------------------+

.. |folder_types_link| replace:: :ref:`Folder types <menus/view/select_columns:Folder type>`

You may set the :doc:`columns <../../view/select_columns>` (and
:ref:`column order <menus/view/select_columns:Column Order>`) to be
shown on **new tabs**; default column widths are always reset to 150
pixels. Default column widths may be changed, however, by the :doc:`Save
Column Layout as Default <../../view/save_column_layout>` menu item.

About Default Columns and Column Widths
+++++++++++++++++++++++++++++++++++++++

.. note::

  **Explorer++** keeps 2 different sets of *default* columns for use,
  each with its own column order, visibility and column widths for the 7
  different :ref:`types of folders <menus/view/select_columns:Folder
  type>`.

  - one set of **system defaults**. This set is the set used to create
    :doc:`new tabs <../../file/new_tab>`. It can be altered by the
    **Default Columns...** setting (this setting - column order and
    visibility only) and the :doc:`Save Column Layout as Default
    <../../view/save_column_layout>` setting (order, visibility and
    width). Column widths are **only** changed when the current tab
    layout is saved as default. New tabs, when created, adopt this set
    of columns and widths. These settings are *persistent* , maintained
    in either the :ref:`config.xml file <menus/tools/options/general:Run
    in portable mode>` or the Windows registry.

  - one set for **each tab**. This set is used when the tab is switched
    to one of the 7 different types of folders, but is never used for
    another tab. Column display settings and width changes are only
    maintained for this tab. If the **Save Column Layout as Default**
    option is used, the current column settings and widths (this type of
    folder only) is saved to the system defaults. Once the tab is
    closed, these settings are lost.