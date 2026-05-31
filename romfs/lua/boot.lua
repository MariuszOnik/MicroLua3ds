-- MicroLua3DS boot.lua
-- Equivalent to MicroLua NDS boot: load user's main.lua from SD card.
-- Place your scripts on the SD card at: /3ds/MicroLua3DS/main.lua

local red   = Color.new(31, 0, 0)
local white = Color.new(31, 31, 31)
local black = Color.new(0, 0, 0)

local function showError(msg)
    while Controls.isRunning() do
        Controls.read()
        screen.startDrawing2D()
            screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, black)
            screen.print(SCREEN_UP, 4, 10, "MicroLua3DS - Error:", red)
            screen.print(SCREEN_UP, 4, 30, tostring(msg), white)
            screen.print(SCREEN_UP, 4, 220, "Press START to exit", white)
        screen.endDrawing()
        screen.waitForVBL()
        if Keys.newPress.Start then break end
    end
end

-- Try to load user script from SD card
local paths = {
    "sdmc:/3ds/MicroLua3DS/main.lua",
    "sdmc:/lua/main.lua",
    "sdmc:/lua/boot.lua",  -- legacy MicroLua NDS path
}

local loaded = false
local lastErr = ""
for _, path in ipairs(paths) do
    local f, err = loadfile(path)
    if f then
        local ok, runerr = pcall(f)
        if not ok then showError(runerr) end
        loaded = true
        break
    else
        lastErr = lastErr .. "\n" .. tostring(err)
    end
end

if not loaded then
    local clrBg   = Color.new(0, 0, 15)
    local clrBox  = Color.new(0, 15, 31)
    local clrText = Color.new(31, 31, 31)
    local clrErr  = Color.new(31, 10, 0)

    while Controls.isRunning() do
        Controls.read()

        screen.startDrawing2D()
            screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, clrBg)
            screen.drawFillRect(SCREEN_UP, 4, 4, SCREEN_WIDTH-4, 236, clrBox)
            screen.print(SCREEN_UP, 8, 10, "MicroLua3DS - no script found", clrText)
            screen.print(SCREEN_UP, 8, 30, "Put main.lua at:", clrText)
            screen.print(SCREEN_UP, 8, 46, "sdmc:/3ds/MicroLua3DS/main.lua", clrText)
            screen.print(SCREEN_UP, 8, 70, "Load errors:", clrErr)
            screen.print(SCREEN_UP, 8, 86, lastErr, clrErr)
            screen.print(SCREEN_UP, 8, 226, "START = exit", clrText)

            screen.drawFillRect(SCREEN_DOWN, 0, 0, 320, 240, clrBg)
        screen.endDrawing()
        screen.waitForVBL()

        if Keys.newPress.Start then break end
    end
end
