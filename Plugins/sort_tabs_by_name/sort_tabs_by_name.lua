menu.create("Sort tabs\tAlt+Shift+S", function ()
    sortTabs()
  end
)

commands.onCommand.addListener(function (command)
    if command == "sort_tabs" then
      sortTabs()
    end
  end
)

-- Retrieves all current tabs in Explorer++ and then sorts them by name.
function sortTabs()
  tabList = tabs.getAll()

  local tabsTable = {}

  -- Build a table containing the list of tabs to allow them to be easily
  -- sorted. There may ultimately be an easier way of doing this.
  for key = 1, #tabList do
    tab = tabList[key]
    
    table.insert(tabsTable, { id = tab.id, name = tab.name })
  end

  table.sort(tabsTable, function (tab1, tab2)
      return tab1.name < tab2.name
      end
  )

  for key = 1, #tabsTable do
    tab = tabsTable[key]
    
    -- Move the tab into its sorted position.
    tabs.move(tab.id, key - 1)
  end
end