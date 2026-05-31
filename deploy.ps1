# deploy.ps1 — buduje MicroLua3DS, kopiuje na wirtualna karte SD Citry i uruchamia emulator
# Uzycie:
#   .\deploy.ps1              — kompilacja + kopia Lua + Citra
#   .\deploy.ps1 -NoCompile   — tylko kopia Lua + Citra (bez make)

param([switch]$NoCompile)

$ProjectDir = $PSScriptRoot
$Citra      = "Z:\Unity Nintendo 3ds\citra-windows-msvc-20240303-0ff3440_nightly\citra-windows-msvc-20240303-0ff3440\citra-qt.exe"
$SdRoot     = "$env:APPDATA\Citra\sdmc"
$SdTarget   = "$SdRoot\3ds\MicroLua3DS"
$Bash       = "D:\devkitPro\msys2\usr\bin\bash.exe"
$Build3dsx  = "$ProjectDir\MicroLua3DS.3dsx"
$LuaSrc     = "$ProjectDir\romfs\lua"
$LuaDst     = "$SdTarget"

# ── 1. Kompilacja (pominięta jeśli -NoCompile) ────────────────────────────────
if ($NoCompile) {
    Write-Host "`n[1/4] Pomijam kompilacje (-NoCompile)." -ForegroundColor Yellow
} else {
    Write-Host "`n[1/4] Kompilacja..." -ForegroundColor Cyan
    $makeCmd = "cd '/opt/devkitpro/examples/3ds/MicroLua3DS' && make 2>&1"
    & $Bash -l -c $makeCmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n[BLAD] Kompilacja nieudana." -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] Skompilowano." -ForegroundColor Green
}

# ── 2. Zamknij Citre jesli dziala ─────────────────────────────────────────────
Write-Host "`n[2/4] Zamykanie Citry (jesli otwarta)..." -ForegroundColor Cyan
Stop-Process -Name "citra-qt" -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

# ── 3. Kopiowanie plikow ───────────────────────────────────────────────────────
Write-Host "`n[3/4] Kopiowanie plikow na wirtualna karte SD..." -ForegroundColor Cyan

New-Item -ItemType Directory -Force $SdTarget | Out-Null

# .3dsx
Copy-Item $Build3dsx $SdTarget -Force
Write-Host "  -> MicroLua3DS.3dsx"

# Wszystkie pliki .lua z romfs/lua — kopiuj BEZ BOM (Lua 5.1 nie obsługuje BOM)
$enc     = New-Object System.Text.UTF8Encoding($false)
$luaFiles = Get-ChildItem $LuaSrc -Recurse -Filter "*.lua"
foreach ($f in $luaFiles) {
    $rel = $f.FullName.Substring($LuaSrc.Length + 1)
    $dst = Join-Path $LuaDst $rel
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    $content = [System.IO.File]::ReadAllText($f.FullName)
    [System.IO.File]::WriteAllText($dst, $content, $enc)
    Write-Host "  -> $rel"
}

Write-Host "[OK] Skopiowano $($luaFiles.Count + 1) plik(ow)." -ForegroundColor Green

# ── 4. Uruchom Citre ──────────────────────────────────────────────────────────
Write-Host "`n[4/4] Uruchamianie Citry..." -ForegroundColor Cyan
$dsx = "$SdTarget\MicroLua3DS.3dsx"
Start-Process $Citra -ArgumentList "`"$dsx`""
Write-Host "[OK] Citra uruchomiona." -ForegroundColor Green
Write-Host ""
