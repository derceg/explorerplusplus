Change Display Window Colors...
-------------------------------

This dialog allow you to customize the colors and font of the Display
Window. See below for a brief :ref:`description
<menus/view/change_display_window_colors:Description of color gradient>`
of the color gradient used as a background.

.. image:: /_static/images/mnu_view/dispwin_mod.png

Description of color gradient
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Display Window background is a diagonal color gradient in 2
directions. Think of the **Center color** as the *starting* point of the
gradient - a single point in the top-left corner of the **Display
Window** ; the **Surrounding color** is the *final* color, located in
the bottom-right corner. The gradient is *drawn diagonally* from
top-left to bottom-right, but also in a pseudo-complementary (ie. at
90°) direction. The end result is a pleasing diagonal gradient with
colors on the left generally close to the **Center color** and colors to
the right generally close to the **Surrounding color**. The algorithm
behind drawing the gradient is not useful; the colors should just be
adjusted visually for the best combination.

Center color
~~~~~~~~~~~~

The controls for the **Center color** (ie. top-left corner) allow RGB
(Red/Green/Blue) adjustment, either by moving the slider or by entering
an integer (0..255) in the edit box to the right of each slider. For a
little help with RGB colors, see `this site <https://www.rapidtables.com/web/color/RGB_Color.html>`_.

Surrounding color
~~~~~~~~~~~~~~~~~

The controls for the **Surrounding color** (ie. bottom-right corner)
allow RGB (Red/Green/Blue) adjustment, in a manner similar to the
:ref:`Center color <menus/view/change_display_window_colors:Center
color>`.

Restore Defaults
~~~~~~~~~~~~~~~~

This button restores **Explorer++**'s built-in defaults for the Display
Window, that is

- Center color = RGB (0,0,0)
- Surrounding color = RGB (0,94,138)
- Font = "Segoe UI"

   - height = 10 (approximate - device dependent)
   - weight = 500 (medium)
   - no italic
   - no underline
   - no strikeout
   - default character set
   - high quality output

   Note that the "Segoe UI" font is an included font in Windows Vista
   and 7; most Windows XP users will not have access to that font and
   their system will substitute an available font, probably "Courier". A
   similar font might be

   - Lucida Sans (Bold),

   - Microsoft Sans Serif (Bold),

   - Tahoma, or

   - Trebuchet MS

   but nearly any font may be used.

Choose font...
~~~~~~~~~~~~~~

This button opens a standard Windows dialog allowing the user to choose
a suitable font for text in the Display Window. Headings and information
text both use this font. See comments in :ref:`Restore Defaults
<menus/view/change_display_window_colors:Restore Defaults>` regarding
the font used.

Display Window preview
~~~~~~~~~~~~~~~~~~~~~~

This area previews the color gradient and text using the selected
:ref:`font <menus/view/change_display_window_colors:Choose font...>`.
Changes made to the :ref:`Center color
<menus/view/change_display_window_colors:Center color>` and
:ref:`Surrounding color
<menus/view/change_display_window_colors:Surrounding color>` are
previewed, as the changes occur, so that the user may choose the color
gradient visually.