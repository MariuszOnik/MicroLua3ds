-- MicroLua3DS boot.lua — file browser

local clrBg      = Color.new(0, 0, 10)
local clrBar     = Color.new(0, 6, 18)
local clrText    = Color.new(26, 26, 26)
local clrDim     = Color.new(13, 13, 13)
local clrSel     = Color.new(0, 6, 24)   -- ciemnoniebieski pasek
local clrSelTxt  = Color.new(31, 31, 31) -- bialy zaznaczony
local clrDir     = Color.new(8, 18, 31)  -- jasnoniebieski katalog
local clrLua     = Color.new(8, 26, 8)   -- jasnozielony plik lua
local clrErr     = Color.new(31, 6, 0)
local clrOk      = Color.new(0, 24, 0)
local clrHint    = Color.new(20, 20, 4)

-- C2D_AtBaseline: Y to dolna krawedz tekstu (linia bazowa).
-- Tekst przy skali 0.5 ma ~13px wysokosci — rozciaga sie w GORE od Y.
-- Dlatego kazdy tekst rysujemy przy DOLNEJ krawedzi wiersza.
local LINE_H   = 15   -- wysokosc wiersza w px
local LIST_Y   = 20   -- Y pierwszego wiersza listy (zaraz po pasku)
local VISIBLE  = 14   -- wierszy widocznych (floor((240-20)/15))
local TXT_OFF  = 12   -- offset baseline od gory wiersza (LINE_H - 3)

-- ---- helpers ----------------------------------------------------------------

local function endsWith(s, suffix)
    return s:sub(-#suffix):lower() == suffix:lower()
end

local function joinPath(dir, name)
    if dir:sub(-1) == "/" then return dir .. name
    else return dir .. "/" .. name end
end

local function parentDir(path)
    -- "sdmc:/foo/bar" → "sdmc:/foo"
    -- "sdmc:/foo"     → "sdmc:/"
    -- "sdmc:/"        → "sdmc:/"
    local p = path:match("^(.*)/[^/]+$")
    if p == nil or p == "" or p:find(":$") then
        -- already at root like "sdmc:"
        return path:match("^([^/]+:/)") or path
    end
    return p
end

-- Listuje katalog; zwraca tablice {name, isDir}, dirs najpierw, sorted
local function listDir(path)
    local ok, entries = pcall(System.listDirectory, path)
    if not ok then return nil, tostring(entries) end

    local dirs, files = {}, {}
    for _, e in ipairs(entries) do
        if e.name ~= "." and e.name ~= ".." then
            if e.isDir then
                dirs[#dirs+1] = { name = e.name, isDir = true }
            elseif endsWith(e.name, ".lua") then
                files[#files+1] = { name = e.name, isDir = false }
            end
        end
    end
    table.sort(dirs,  function(a,b) return a.name:lower() < b.name:lower() end)
    table.sort(files, function(a,b) return a.name:lower() < b.name:lower() end)

    local result = {}
    for _, v in ipairs(dirs)  do result[#result+1] = v end
    for _, v in ipairs(files) do result[#result+1] = v end
    return result
end

-- ---- ekrany pomocnicze ------------------------------------------------------

local function drawStatusBar(path)
    screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, LIST_Y, clrBar)
    local display = path
    if #display > 46 then display = "..." .. display:sub(-43) end
    -- baseline przy dolnej krawedzi paska (LIST_Y - 3)
    screen.print(SCREEN_UP, 4, LIST_Y - 3, display, clrText)
end

local function drawHints(bottom)
    screen.drawFillRect(SCREEN_DOWN, 0, 0, 320, 240, clrBg)
    screen.drawFillRect(SCREEN_DOWN, 0, 0, 320, LIST_Y, clrBar)
    screen.print(SCREEN_DOWN, 4, LIST_Y - 3, "MicroLua3DS", clrText)
    local y = LIST_Y + TXT_OFF
    for _, line in ipairs(bottom) do
        screen.print(SCREEN_DOWN, 4, y, line, clrDim)
        y = y + 16
    end
end

local function showMessage(msg, color, hints)
    while Controls.isRunning() do
        Controls.read()
        screen.startDrawing2D()
            screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, clrBg)
            drawStatusBar("MicroLua3DS")
            screen.print(SCREEN_UP, 4, LIST_Y + TXT_OFF, msg, color or clrText)
            drawHints(hints or {"A / START = wyjscie"})
        screen.endDrawing()
        screen.waitForVBL()
        if Keys.newPress.A or Keys.newPress.Start then break end
    end
end

-- Ekran bledu skryptu — A wraca do browsera
local function showError(msg)
    local cont = true
    while Controls.isRunning() and cont do
        Controls.read()
        screen.startDrawing2D()
            screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, clrBg)
            drawStatusBar("Blad skryptu")
            screen.print(SCREEN_UP, 4, LIST_Y + TXT_OFF,      "Blad:", clrErr)
            screen.print(SCREEN_UP, 4, LIST_Y + TXT_OFF + 16, tostring(msg), clrErr)
            drawHints({"A = powrot do listy", "START = wyjscie"})
        screen.endDrawing()
        screen.waitForVBL()
        if Keys.newPress.A     then cont = false end
        if Keys.newPress.Start then return false end
    end
    return true  -- wroc do browsera
end

-- ---- glowna petla browsera --------------------------------------------------

local path    = "sdmc:/"
local cursor  = 1
local scroll  = 0   -- indeks pierwszego widocznego wiersza (0-based)
local entries = {}
local errMsg  = nil

local function reload()
    local list, err = listDir(path)
    if list then
        entries = list
        errMsg  = nil
    else
        entries = {}
        errMsg  = err
    end
    if cursor > #entries then cursor = math.max(1, #entries) end
    if cursor - 1 < scroll then scroll = cursor - 1 end
    if cursor - 1 >= scroll + VISIBLE then scroll = cursor - 1 - VISIBLE + 1 end
end

reload()

-- debouncery nawigacji
local navTimer = 0
local NAV_DELAY = 10  -- klatek

while Controls.isRunning() do
    Controls.read()

    -- Nawigacja z powtarzaniem po przytrzymaniu
    local moved = false
    if Keys.held.Up then
        if navTimer == 0 or navTimer > NAV_DELAY then
            cursor = math.max(1, cursor - 1)
            moved = true
        end
        navTimer = navTimer + 1
    elseif Keys.held.Down then
        if navTimer == 0 or navTimer > NAV_DELAY then
            cursor = math.min(#entries, cursor + 1)
            moved = true
        end
        navTimer = navTimer + 1
    else
        navTimer = 0
    end

    if moved then
        -- Scroll
        if cursor - 1 < scroll then
            scroll = cursor - 1
        elseif cursor - 1 >= scroll + VISIBLE then
            scroll = cursor - 1 - VISIBLE + 1
        end
    end

    -- Wejdz do katalogu / uruchom plik
    if Keys.newPress.A then
        local e = entries[cursor]
        if e then
            if e.isDir then
                path = joinPath(path, e.name)
                cursor = 1; scroll = 0
                reload()
            else
                -- Uruchom plik Lua w osobnym srodowisku (sandbox)
                local fullpath = joinPath(path, e.name)
                local scriptDir = fullpath:match("^(.*)/[^/]+$") or "sdmc:/"
                System.changeDirectory(scriptDir)

                local ok, errmsg = System.runScript(fullpath)
                if ok then
                    showMessage("Skrypt zakonczony.", clrOk,
                        {"A = powrot do listy", "START = wyjscie"})
                else
                    if not showError(errmsg) then break end
                end
            end
        end
    end

    -- Wyjdz do katalogu nadrzednego
    if Keys.newPress.B then
        local parent = parentDir(path)
        if parent ~= path then
            path = parent
            cursor = 1; scroll = 0
            reload()
        end
    end

    -- START = wyjscie
    if Keys.newPress.Start then break end

    -- ---- Rysowanie ----------------------------------------------------------
    screen.startDrawing2D()

    -- Gorny ekran — lista plikow
    screen.drawFillRect(SCREEN_UP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, clrBg)
    drawStatusBar(path)

    if errMsg then
        screen.print(SCREEN_UP, 4, LIST_Y + TXT_OFF, "Blad: " .. errMsg, clrErr)
    elseif #entries == 0 then
        screen.print(SCREEN_UP, 4, LIST_Y + TXT_OFF, "Brak katalogow i plikow .lua", clrDim)
    else
        for i = 1, VISIBLE do
            local idx  = scroll + i
            local e    = entries[idx]
            if not e then break end

            local rowY = LIST_Y + (i - 1) * LINE_H  -- gorna krawedz wiersza
            local txtY = rowY + TXT_OFF              -- baseline = dolna krawedz tekstu

            if idx == cursor then
                screen.drawFillRect(SCREEN_UP, 0, rowY, SCREEN_WIDTH - 5, rowY + LINE_H, clrSel)
            end

            local label, color
            if e.isDir then
                label = "[" .. e.name .. "]"
                color = (idx == cursor) and clrSelTxt or clrDir
            else
                label = e.name
                color = (idx == cursor) and clrSelTxt or clrLua
            end
            screen.print(SCREEN_UP, 6, txtY, label, color)
        end

        -- Pasek przewijania
        if #entries > VISIBLE then
            local trackH = VISIBLE * LINE_H
            local thumbH = math.max(6, math.floor(trackH * VISIBLE / #entries))
            local thumbY = LIST_Y + math.floor(scroll * (trackH - thumbH) / math.max(1, #entries - VISIBLE))
            screen.drawFillRect(SCREEN_UP, SCREEN_WIDTH - 5, LIST_Y,
                                SCREEN_WIDTH,     LIST_Y + trackH, clrBar)
            screen.drawFillRect(SCREEN_UP, SCREEN_WIDTH - 5, thumbY,
                                SCREEN_WIDTH,     thumbY + thumbH, clrText)
        end
    end

    -- Dolny ekran — instrukcje
    screen.drawFillRect(SCREEN_DOWN, 0, 0, 320, 240, clrBg)
    screen.drawFillRect(SCREEN_DOWN, 0, 0, 320, LIST_Y, clrBar)
    screen.print(SCREEN_DOWN, 4, LIST_Y - 3, "MicroLua3DS", clrText)
    screen.print(SCREEN_DOWN, 4, LIST_Y + TXT_OFF,      "Gora/Dol = przewijanie",    clrDim)
    screen.print(SCREEN_DOWN, 4, LIST_Y + TXT_OFF + 16, "A        = otworz/uruchom", clrDim)
    screen.print(SCREEN_DOWN, 4, LIST_Y + TXT_OFF + 32, "B        = katalog wyzej",  clrDim)
    screen.print(SCREEN_DOWN, 4, LIST_Y + TXT_OFF + 48, "START    = wyjscie",        clrDim)
    if #entries > 0 and entries[cursor] then
        local e = entries[cursor]
        screen.print(SCREEN_DOWN, 4, LIST_Y + TXT_OFF + 72,
            (e.isDir and "[dir] " or "       ") .. e.name, clrHint)
    end

    screen.endDrawing()
    screen.waitForVBL()
end
