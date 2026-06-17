# Explorer++ Command Line Interface

Explorer++ supports a variety of command-line arguments to customize its behavior and open specific directories.

## Standard Arguments

| Argument | Long Flag | Description |
| :--- | :--- | :--- |
| `-h` | `--help` | Display help message and exit. |
| `-c` | `--clear-settings` | Clear existing registry settings. |
| `-r` | `--remove-default` | Remove Explorer++ as the default file manager. |
| `-d` | `--set-default` | Set Explorer++ as the default file manager. (Values: `filesystem`, `all`) |
| `-l` | `--log` | Enable logging. |
| `-f` | `--features` | Enable experimental features. |
| `-n` | `--notify` | Select directory change notification mode. (Values: `shell`, `filesystem`) |
| `-L` | `--lang` | Select UI language (e.g., `FR`, `RU`). |
| `-s` | `--select` | Open the parent folder and select the specified file/folder. |

## Windows Explorer Compatibility Arguments

Explorer++ also supports standard Windows Explorer switches for better integration:

| Argument | Description |
| :--- | :--- |
| `/n` | Force open a new window, even if "Allow multiple instances" is disabled. |
| `/e` | Open in "Explorer" view (default behavior). |
| `/select,<path>` | Opens a window with the specified file or folder selected. |
| `/select <path>` | Same as above (space instead of comma). |

## Relative Path Support

You can launch Explorer++ using relative paths, just like Windows Explorer:

- `explorer++ .` - Opens the current directory.
- `explorer++ ..` - Opens the parent directory.
- `explorer++ .\my_folder` - Opens a subfolder in the current directory.

All relative paths are resolved to absolute paths based on the current working directory of the caller.
