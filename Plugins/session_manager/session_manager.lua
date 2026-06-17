local function openWorkSession()
    -- Create several tabs at once
    tabs.create({ location = "C:\\Windows", selected = false })
    tabs.create({ location = "C:\\Users", selected = false })
    tabs.create({ location = "C:\\Program Files", selected = true })
end

menu.create("Open Work Session", openWorkSession)
