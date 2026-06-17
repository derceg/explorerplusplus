-- Adds a custom menu item to the "Tools" menu (by default) or the "Plugins" submenu.
menu.create("Hello from Lua", function ()
    print("Lua: Hello world!")
    -- You can use standard Lua prints if a console is attached,
    -- or other API calls to interact with Explorer++.
end)

menu.create("Open C:\\ in new tab", function ()
    tabs.create({ location = "C:\\", selected = true })
end)
