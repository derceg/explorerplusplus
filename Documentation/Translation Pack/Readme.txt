Notes on translating the attached rc file:

1. The only sections that need to be altered within the rc file are:
- the dialog section
- the menu section, and
- the string table section

2. Within the menu section:
- Any text after a '\t' character represents the menus' keyboard shortcut, and does not need to be changed.
- The character after an ampersand (&) denote a menus' accelerator key. These should be set for each menu item.

3. Within the string table section:
- Any string constant that starts with the prefix IDM_ is the helper string for the menu with the same constant. This helper string is what is displayed in the status bar when a menu is selected.