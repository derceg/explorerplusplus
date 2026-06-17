# Explorer++ Lua Plugin System

Explorer++ features a Lua-based plugin system that allows you to extend the application's functionality.

## Plugin Structure

Each plugin should reside in its own folder within the `Plugins/` directory and contain:
- `plugin.json`: Metadata about the plugin.
- `<filename>.lua`: The Lua script containing the logic.

### plugin.json Example

```json
{
  "name": "My Plugin",
  "description": "A description of my plugin",
  "file": "main.lua",
  "version": "1.0",
  "author": "Your Name"
}
```

## API Reference

### `tabs` API

- `tabs.getAll()`: Returns a list of all open tabs.
- `tabs.get(id)`: Returns a specific tab by ID.
- `tabs.create({ location, selected })`: Creates a new tab.
- `tabs.move(id, newIndex)`: Moves a tab.
- `tabs.close(id)`: Closes a tab.

#### Events
- `tabs.onCreated.addListener(callback)`
- `tabs.onRemoved.addListener(callback)`
- `tabs.onUpdated.addListener(callback)`

### `menu` API

- `menu.create(text, callback)`: Adds a menu item to the Plugins menu.

### `commands` API

#### Events
- `commands.onCommand.addListener(callback)`: Listens for custom commands (e.g., from keyboard shortcuts).

## Examples

Check the `Plugins/` directory for several examples:
1. `sort_tabs_by_name`: Demonstrates tab manipulation.
2. `custom_menu_item`: Demonstrates adding menu items.
3. `event_logger`: Demonstrates event listeners.
4. `session_manager`: Demonstrates batch operations.
