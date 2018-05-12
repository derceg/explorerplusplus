Paste
-----

:kbd:`Ctrl` + :kbd:`V`

This menu item takes data from the clipboard and pastes it into the
files pane, or the folders pane if it has the focus. If files or folders
were :doc:`cut <cut>` in order to place them on the clipboard, then they
are deleted at this time. **Explorer++** handles various types of data,
and the pasting method, differently, according to the following table:

    +-----------------------+-----------------------+-----------------------+
    | **Clipboard data**    | **Paste method**      | **Result**            |
    +-----------------------+-----------------------+-----------------------+
    | files/folders         | paste into blank area | files/folders pasted  |
    |                       | of files pane         | normally to location  |
    |                       |                       | of the files pane     |
    +-----------------------+-----------------------+-----------------------+
    | files/folders         | paste into folders    | files/folders pasted  |
    |                       | pane (using menu      | into the folder       |
    |                       | item, or toolbar      | having the focus in   |
    |                       | button)               | the folders pane.     |
    |                       |                       | New files/folders     |
    |                       |                       | will, of course,      |
    |                       |                       | appear in the files   |
    |                       |                       | pane (contents of     |
    |                       |                       | focused folder).      |
    +-----------------------+-----------------------+-----------------------+
    | text                  | paste into dialog, or | text inserted, or     |
    |                       | rename file operation | overwrites text       |
    |                       |                       | selection             |
    +-----------------------+-----------------------+-----------------------+
    | text                  | paste into blank area | text pasted as new    |
    |                       | of files pane         | text file, automatic  |
    |                       |                       | naming:               |
    |                       |                       | eg. Clipboard Text    |
    |                       |                       | (10-20-2010, 6.44.01  |
    |                       |                       | PM).txt               |
    +-----------------------+-----------------------+-----------------------+
    | text                  | paste into folders    | text pasted as new    |
    |                       | pane (using menu      | text file (as above)  |
    |                       | item, or toolbar      | into the folder       |
    |                       | button)               | having the focus in   |
    |                       |                       | the folders pane.     |
    |                       |                       | New file will, of     |
    |                       |                       | course, appear in the |
    |                       |                       | files pane (contents  |
    |                       |                       | of focused folder).   |
    +-----------------------+-----------------------+-----------------------+
    | bitmap                | image data, copied    | new image created,    |
    | (image)               | from a paint program, | automatic naming:     |
    |                       | for example is pasted | eg. Clipboard Image   |
    |                       | into blank area of    | (10-20-2010, 6.53.48  |
    |                       | files pane            | PM).bmp               |
    |                       |                       | New image always      |
    |                       |                       | created as Windows    |
    |                       |                       | bitmap, regardless of |
    |                       |                       | original image type.  |
    +-----------------------+-----------------------+-----------------------+

See :doc:`Copy <copy>` for a discussion of behavior when file names are
duplicated.