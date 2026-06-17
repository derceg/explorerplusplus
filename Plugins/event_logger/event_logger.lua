-- Listen for new tabs being created
tabs.onCreated.addListener(function (tab)
    -- 'tab' is a Tab object as defined in ApiBinding.cpp
    -- It has fields: id, location, name, locked, addressLocked, folderSettings
    print("Lua: Tab created! ID: " .. tab.id .. ", Location: " .. tab.location)
end)

-- Listen for tabs being removed
tabs.onRemoved.addListener(function (tabId)
    print("Lua: Tab removed! ID: " .. tabId)
end)

-- Listen for tabs being updated (e.g. navigation)
tabs.onUpdated.addListener(function (tab)
    print("Lua: Tab updated! ID: " .. tab.id .. ", New Location: " .. tab.location)
end)
