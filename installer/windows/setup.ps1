<#
  Watch Studio setup for Windows - installs, changes and removes the app.

  Double-click Setup.cmd (next to the "files" folder) rather than running this
  directly. It opens a step-by-step installer like any other Windows program:
  Welcome -> Personalise -> Options -> Ready -> Finished.

  What it puts on the computer (about 0.1 MB, no administrator rights needed):
    %LOCALAPPDATA%\Programs\Watch Studio\   the app (one web page + icons), this
                                             script, config.js, install.json
    Start menu shortcut, and if chosen: desktop shortcut, keyboard shortcut,
    "open when I sign in" shortcut
    An entry in Settings > Apps so it can be removed like any program

  The app window is drawn by Microsoft Edge (part of Windows) or Chrome, run in
  "app mode": its own window and taskbar icon, no address bar or tabs. Nothing
  is downloaded and no browser is installed.

  Removing: Settings > Apps > Installed apps > Watch Studio > Uninstall, or run
  Setup.cmd again and press Remove. Only the files listed above are deleted.

  For testing and scripting:
    setup.ps1 -Silent [-InstallDir <folder>] [-ForName ..] [-Message ..] [-From ..]
              [-Accent green|blue|purple|orange|red|teal] [-Theme auto|light|dark]
              [-NoStartMenu] [-Desktop] [-Hotkey W] [-AtSignIn] [-Engine edge|chrome]
              [-NoRegister] [-Launch]
    setup.ps1 -Uninstall [-Quiet]

  Keep this file ASCII: Windows PowerShell 5.1 reads scripts without a BOM as
  the local code page.
#>
param(
  [switch]$Silent, [switch]$Uninstall, [switch]$Quiet,
  [string]$InstallDir = "", [string]$ForName = "", [string]$Message = "", [string]$From = "",
  [string]$Accent = "green", [string]$Theme = "auto",
  [switch]$NoStartMenu, [switch]$Desktop, [string]$Hotkey = "", [switch]$AtSignIn,
  [string]$Engine = "", [switch]$NoRegister, [switch]$Launch
)

$ErrorActionPreference = "Stop"
$AppName    = "Watch Studio"
$AppVersion = "1.1"
$RegKey     = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\WatchStudio"
$Marker     = "install.json"
$AppFiles   = @("index.html", "icon.svg", "icon.png", "manifest.webmanifest", "sw.js")
$OwnFiles   = @("setup.ps1", "watch.ico", "config.js", $Marker)
$Here       = $PSScriptRoot
$DefaultDir = Join-Path $env:LOCALAPPDATA "Programs\$AppName"
# Run from the unzipped download, the app is in files\app. Run from an installed copy
# (Settings > Apps > Modify), it is right here.
$Src = if (Test-Path -LiteralPath (Join-Path $Here "app\index.html")) { Join-Path $Here "app" } else { $Here }
$IsInstalledCopy = ($Src -eq $Here)

$StartMenuLnk = Join-Path ([Environment]::GetFolderPath("Programs")) "$AppName.lnk"
$DesktopLnk   = Join-Path ([Environment]::GetFolderPath("Desktop")) "$AppName.lnk"
$StartupLnk   = Join-Path ([Environment]::GetFolderPath("Startup")) "$AppName.lnk"

# ------------------------------------------------------------------ helpers
function Find-Engines {
  # Browsers that can show the app and talk to USB / Bluetooth. Edge first: every Windows 10/11 has it.
  $found = [ordered]@{}
  $edge = @("${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe", "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe",
            "$env:LOCALAPPDATA\Microsoft\Edge\Application\msedge.exe")
  $chrome = @("$env:ProgramFiles\Google\Chrome\Application\chrome.exe", "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
              "$env:LOCALAPPDATA\Google\Chrome\Application\chrome.exe")
  foreach ($p in $edge)   { if ($p -and (Test-Path -LiteralPath $p)) { $found["edge"] = $p; break } }
  foreach ($p in $chrome) { if ($p -and (Test-Path -LiteralPath $p)) { $found["chrome"] = $p; break } }
  return $found
}

function Read-Installed {
  # The previous install's choices, so changing the install starts from them.
  try {
    $loc = (Get-ItemProperty -Path $RegKey -ErrorAction Stop).InstallLocation
    $file = Join-Path $loc $Marker
    if (Test-Path -LiteralPath $file) {
      $o = Get-Content -LiteralPath $file -Raw -Encoding UTF8 | ConvertFrom-Json
      $o | Add-Member -NotePropertyName dir -NotePropertyValue $loc -Force
      return $o
    }
  } catch {}
  # Not registered (e.g. a -NoRegister test): look in the folder this script runs from.
  $file = Join-Path $Here $Marker
  if (Test-Path -LiteralPath $file) {
    $o = Get-Content -LiteralPath $file -Raw -Encoding UTF8 | ConvertFrom-Json
    $o | Add-Member -NotePropertyName dir -NotePropertyValue $Here -Force
    return $o
  }
  return $null
}

function Resolve-InstallDir([string]$dir) {
  # Always a folder of our own, so removing it can never take anything else with it.
  if (-not $dir) { return $DefaultDir }
  $dir = [IO.Path]::GetFullPath($dir.Trim().TrimEnd("\"))
  if ((Split-Path -Leaf $dir) -ne $AppName) { $dir = Join-Path $dir $AppName }
  return $dir
}

function Write-Utf8([string]$path, [string]$text) {
  [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding $false))
}

function New-Shortcut([string]$lnk, [string]$exe, [string]$url, [string]$dir, [string]$key) {
  $shell = New-Object -ComObject WScript.Shell
  $s = $shell.CreateShortcut($lnk)
  $s.TargetPath = $exe
  $s.Arguments = "--app=`"$url`" --window-size=1000,880"
  $s.WorkingDirectory = $dir
  $s.IconLocation = (Join-Path $dir "watch.ico") + ",0"
  $s.Description = "Set up, personalise and extend your watch"
  if ($key) { $s.Hotkey = $key }
  $s.Save()
}

function Install-App($o) {
  # $o: dir, forName, message, from, accent, theme, startMenu, desktop, hotkey, atSignIn, engine, register
  $dir = $o.dir
  $src = $Src
  if (-not (Test-Path -LiteralPath (Join-Path $src "index.html"))) { throw "The app files are missing next to setup.ps1 (expected $src). Unzip the whole download first." }
  if ((Test-Path -LiteralPath $dir) -and -not (Test-Path -LiteralPath (Join-Path $dir $Marker))) {
    if (@(Get-ChildItem -LiteralPath $dir -Force).Count -gt 0) { throw "$dir already exists and is not a Watch Studio folder. Choose another folder." }
  }
  $engines = Find-Engines
  $exe = $engines[$o.engine]
  if (-not $exe) { $o.engine = @($engines.Keys)[0]; $exe = $engines[$o.engine] }
  if (-not $exe) { throw "Microsoft Edge or Google Chrome is needed to show the app, and neither was found." }

  New-Item -ItemType Directory -Force -Path $dir | Out-Null
  # Changing an installed copy in place: its files are already there.
  $same = ([IO.Path]::GetFullPath($Here).TrimEnd("\") -eq [IO.Path]::GetFullPath($dir).TrimEnd("\"))
  if (-not $same) {
    foreach ($f in $AppFiles) {
      $p = Join-Path $src $f
      if (Test-Path -LiteralPath $p) { Copy-Item -LiteralPath $p -Destination $dir -Force }
    }
    Copy-Item -LiteralPath (Join-Path $Here "setup.ps1") -Destination $dir -Force
    Copy-Item -LiteralPath (Join-Path $Here "watch.ico") -Destination $dir -Force
  }

  $cfg = [ordered]@{ forName = $o.forName; message = $o.message; from = $o.from; accent = $o.accent; theme = $o.theme }
  Write-Utf8 (Join-Path $dir "config.js") ("// Written by Watch Studio setup. Run setup again to change it.`r`nwindow.STUDIO_CONFIG = " + ($cfg | ConvertTo-Json) + ";`r`n")
  $save = [ordered]@{ version = $AppVersion; forName = $o.forName; message = $o.message; from = $o.from; accent = $o.accent; theme = $o.theme
                      startMenu = [bool]$o.startMenu; desktop = [bool]$o.desktop; hotkey = $o.hotkey; atSignIn = [bool]$o.atSignIn; engine = $o.engine }
  Write-Utf8 (Join-Path $dir $Marker) ($save | ConvertTo-Json)
  Get-ChildItem -LiteralPath $dir -File | Unblock-File -ErrorAction SilentlyContinue

  $url = ([Uri](Join-Path $dir "index.html")).AbsoluteUri
  $key = ""
  if ($o.hotkey) { $key = "CTRL+ALT+" + $o.hotkey.ToUpper() }
  # The keyboard shortcut lives on the Start menu shortcut (Windows only honours it there or on the desktop).
  foreach ($pair in @(@($StartMenuLnk, $o.startMenu, $key), @($DesktopLnk, $o.desktop, ""), @($StartupLnk, $o.atSignIn, ""))) {
    if ($pair[1]) { New-Shortcut $pair[0] $exe $url $dir $pair[2] }
    elseif (Test-Path -LiteralPath $pair[0]) { Remove-Item -LiteralPath $pair[0] -Force }
  }
  if ($key -and -not $o.startMenu -and $o.desktop) { New-Shortcut $DesktopLnk $exe $url $dir $key }

  if ($o.register) {
    $kb = [int][Math]::Ceiling(((Get-ChildItem -LiteralPath $dir -File | Measure-Object Length -Sum).Sum) / 1024)
    $ps = "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe"
    New-Item -Path $RegKey -Force | Out-Null
    $vals = @{ DisplayName = $AppName; DisplayVersion = $AppVersion; Publisher = "Watch V1"; InstallLocation = $dir
               DisplayIcon = (Join-Path $dir "watch.ico"); Comments = "Companion app for the Watch V1 wrist watch"
               UninstallString = "`"$ps`" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$dir\setup.ps1`" -Uninstall"
               QuietUninstallString = "`"$ps`" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$dir\setup.ps1`" -Uninstall -Quiet"
               ModifyPath = "`"$ps`" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$dir\setup.ps1`"" }
    foreach ($k in $vals.Keys) { Set-ItemProperty -Path $RegKey -Name $k -Value $vals[$k] }
    Set-ItemProperty -Path $RegKey -Name EstimatedSize -Value $kb -Type DWord
    Set-ItemProperty -Path $RegKey -Name NoRepair -Value 1 -Type DWord
  }
  return @{ exe = $exe; url = $url }
}

function Uninstall-App([bool]$quiet) {
  $prev = Read-Installed
  $dir = if ($prev) { $prev.dir } else { $Here }
  if (-not $quiet) {
    Add-Type -AssemblyName System.Windows.Forms
    $a = [Windows.Forms.MessageBox]::Show("Remove Watch Studio from this computer?`n`nThe watch keeps all its settings and apps.", "Remove Watch Studio", "YesNo", "Question")
    if ($a -ne "Yes") { return $false }
  }
  foreach ($l in @($StartMenuLnk, $DesktopLnk, $StartupLnk)) {
    # Only our own shortcuts: they point at our folder.
    if (Test-Path -LiteralPath $l) {
      $t = (New-Object -ComObject WScript.Shell).CreateShortcut($l)
      if ($t.WorkingDirectory -eq $dir) { Remove-Item -LiteralPath $l -Force }
    }
  }
  if (Test-Path $RegKey) { Remove-Item -Path $RegKey -Recurse -Force }
  if (Test-Path -LiteralPath (Join-Path $dir $Marker)) {
    foreach ($f in $AppFiles + $OwnFiles) {
      $p = Join-Path $dir $f
      if ($f -ne "setup.ps1" -and (Test-Path -LiteralPath $p)) { Remove-Item -LiteralPath $p -Force }
    }
    # This script is still running from the folder: let a hidden helper finish once it has exited.
    # rmdir without /s only removes the folder if it is now empty.
    $cmd = "ping -n 3 127.0.0.1 >nul & del /f /q `"$dir\setup.ps1`" & rmdir `"$dir`""
    Start-Process -FilePath "$env:SystemRoot\System32\cmd.exe" -ArgumentList "/c", $cmd -WindowStyle Hidden
  }
  if (-not $quiet) { [Windows.Forms.MessageBox]::Show("Watch Studio has been removed.", "Remove Watch Studio") | Out-Null }
  return $true
}

# ---------------------------------------------------------- command line
if ($Uninstall) { Uninstall-App ([bool]$Quiet) | Out-Null; exit 0 }

if ($Silent) {
  $engines = Find-Engines
  $o = @{ dir = (Resolve-InstallDir $InstallDir); forName = $ForName; message = $Message; from = $From; accent = $Accent; theme = $Theme
          startMenu = -not $NoStartMenu; desktop = [bool]$Desktop; hotkey = $Hotkey; atSignIn = [bool]$AtSignIn
          engine = $(if ($Engine) { $Engine } else { @($engines.Keys)[0] }); register = -not $NoRegister }
  $r = Install-App $o
  Write-Output "Installed to $($o.dir)"
  if ($Launch) { Start-Process -FilePath $r.exe -ArgumentList "--app=`"$($r.url)`"", "--window-size=1000,880" }
  exit 0
}

# ------------------------------------------------------------- the wizard
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
try { Add-Type -Namespace W -Name Dpi -MemberDefinition '[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();'; [W.Dpi]::SetProcessDPIAware() | Out-Null } catch {}
[Windows.Forms.Application]::EnableVisualStyles()

$prev = Read-Installed
$engines = Find-Engines
$font = New-Object Drawing.Font("Segoe UI", 9)
$bold = New-Object Drawing.Font("Segoe UI", 10, [Drawing.FontStyle]::Bold)

$form = New-Object Windows.Forms.Form
$form.Text = "$AppName Setup"
$form.ClientSize = New-Object Drawing.Size(560, 420)
$form.FormBorderStyle = "FixedDialog"; $form.MaximizeBox = $false; $form.MinimizeBox = $false
$form.StartPosition = "CenterScreen"; $form.Font = $font; $form.AutoScaleMode = "Dpi"
$ico = Join-Path $Here "watch.ico"
# Loaded from memory so the files stay unlocked: Remove deletes them while this window is open.
function Read-Stream([string]$path) { New-Object IO.MemoryStream(,[IO.File]::ReadAllBytes($path)) }
if (Test-Path -LiteralPath $ico) { $form.Icon = New-Object Drawing.Icon((Read-Stream $ico)) }

# Banner, like every classic installer.
$banner = New-Object Windows.Forms.Panel
$banner.SetBounds(0, 0, 560, 62); $banner.BackColor = [Drawing.Color]::White
$form.Controls.Add($banner)
$hTitle = New-Object Windows.Forms.Label; $hTitle.Font = $bold; $hTitle.SetBounds(16, 10, 460, 22); $banner.Controls.Add($hTitle)
$hSub = New-Object Windows.Forms.Label; $hSub.SetBounds(30, 34, 450, 20); $banner.Controls.Add($hSub)
$png = Join-Path $Src "icon.png"
if (Test-Path -LiteralPath $png) {
  $pic = New-Object Windows.Forms.PictureBox; $pic.SetBounds(500, 7, 48, 48); $pic.SizeMode = "Zoom"
  $pic.Image = [Drawing.Image]::FromStream((Read-Stream $png)); $banner.Controls.Add($pic)
}
$line1 = New-Object Windows.Forms.Label; $line1.BorderStyle = "Fixed3D"; $line1.SetBounds(0, 62, 560, 2); $form.Controls.Add($line1)
$line2 = New-Object Windows.Forms.Label; $line2.BorderStyle = "Fixed3D"; $line2.SetBounds(0, 368, 560, 2); $form.Controls.Add($line2)

$btnBack = New-Object Windows.Forms.Button; $btnBack.Text = "< Back"; $btnBack.SetBounds(282, 382, 84, 27); $form.Controls.Add($btnBack)
$btnNext = New-Object Windows.Forms.Button; $btnNext.Text = "Next >"; $btnNext.SetBounds(368, 382, 84, 27); $form.Controls.Add($btnNext)
$btnCancel = New-Object Windows.Forms.Button; $btnCancel.Text = "Cancel"; $btnCancel.SetBounds(462, 382, 84, 27); $form.Controls.Add($btnCancel)
$btnRemove = New-Object Windows.Forms.Button; $btnRemove.Text = "Remove"; $btnRemove.SetBounds(14, 382, 84, 27); $btnRemove.Visible = $false; $form.Controls.Add($btnRemove)
$form.AcceptButton = $btnNext

function New-Page { $p = New-Object Windows.Forms.Panel; $p.SetBounds(0, 66, 560, 300); $p.Visible = $false; $form.Controls.Add($p); return $p }
function Add-Label($page, [string]$text, [int]$x, [int]$y, [int]$w, [int]$h) {
  $l = New-Object Windows.Forms.Label; $l.Text = $text; $l.SetBounds($x, $y, $w, $h); $page.Controls.Add($l); return $l
}
function Add-Box($page, [int]$x, [int]$y, [int]$w, [string]$value) {
  $t = New-Object Windows.Forms.TextBox; $t.SetBounds($x, $y, $w, 23); $t.Text = $value; $page.Controls.Add($t); return $t
}
function Add-Check($page, [string]$text, [int]$x, [int]$y, [bool]$on) {
  $c = New-Object Windows.Forms.CheckBox; $c.Text = $text; $c.SetBounds($x, $y, 420, 22); $c.Checked = $on; $page.Controls.Add($c); return $c
}
function Add-Combo($page, [int]$x, [int]$y, [int]$w, $items, [int]$index) {
  $c = New-Object Windows.Forms.ComboBox; $c.DropDownStyle = "DropDownList"; $c.SetBounds($x, $y, $w, 23)
  foreach ($i in $items) { [void]$c.Items.Add($i) }
  $c.SelectedIndex = [Math]::Max(0, [Math]::Min($index, $items.Count - 1)); $page.Controls.Add($c); return $c
}
function Pick($value, $default) { if ($null -ne $value -and "$value" -ne "") { return $value } return $default }

# Page 0: welcome
$p0 = New-Page
$welcome = if ($prev) {
  "Watch Studio is already installed in:`r`n$($prev.dir)`r`n`r`nPress Next to change its personal touches or options, or Remove to take it off this computer."
} else {
  "This will install Watch Studio, the companion app for the watch: set it up, choose its screens, make your own apps for it, record its sensors and back it up.`r`n`r`nIt opens in its own window from the Start menu or a keyboard shortcut. It takes about 0.1 MB, needs no administrator rights, and can be removed from Settings > Apps at any time.`r`n`r`nPress Next to continue."
}
Add-Label $p0 $welcome 30 18 500 200 | Out-Null
$engineNote = if ($engines.Count) { "" } else { "Microsoft Edge or Google Chrome is needed and neither was found. Install one, then run setup again." }
$warn = Add-Label $p0 $engineNote 30 230 500 40; $warn.ForeColor = [Drawing.Color]::Firebrick

# Page 1: personal touches
$p1 = New-Page
Add-Label $p1 "These appear in the app. Leave any of them empty." 30 10 500 20 | Out-Null
Add-Label $p1 "Who is it for?" 30 40 150 20 | Out-Null
$tName = Add-Box $p1 190 38 330 (Pick $prev.forName $ForName)
Add-Label $p1 "A message for them:" 30 72 150 20 | Out-Null
$tMsg = New-Object Windows.Forms.TextBox; $tMsg.Multiline = $true; $tMsg.ScrollBars = "Vertical"; $tMsg.AcceptsReturn = $true
$tMsg.SetBounds(190, 70, 330, 96); $tMsg.MaxLength = 600; $tMsg.Text = (Pick $prev.message $Message); $p1.Controls.Add($tMsg)
Add-Label $p1 "From:" 30 178 150 20 | Out-Null
$tFrom = Add-Box $p1 190 176 330 (Pick $prev.from $From)
$accents = @("green", "blue", "purple", "orange", "red", "teal")
Add-Label $p1 "Colour:" 30 212 150 20 | Out-Null
$cAccent = Add-Combo $p1 190 210 150 ($accents | ForEach-Object { (Get-Culture).TextInfo.ToTitleCase($_) }) ([Array]::IndexOf($accents, (Pick $prev.accent $Accent)))
$themes = @("auto", "light", "dark")
Add-Label $p1 "Appearance:" 30 246 150 20 | Out-Null
$cTheme = Add-Combo $p1 190 244 150 @("Follow Windows", "Light", "Dark") ([Array]::IndexOf($themes, (Pick $prev.theme $Theme)))

# Page 2: where and how it opens
$p2 = New-Page
Add-Label $p2 "Install in:" 30 14 120 20 | Out-Null
$tDir = Add-Box $p2 30 36 400 (Pick $prev.dir $DefaultDir)
$btnBrowse = New-Object Windows.Forms.Button; $btnBrowse.Text = "Browse..."; $btnBrowse.SetBounds(440, 35, 84, 25); $p2.Controls.Add($btnBrowse)
$btnBrowse.Add_Click({
  $d = New-Object Windows.Forms.FolderBrowserDialog; $d.Description = "A folder named Watch Studio will be made inside the folder you choose."
  if ($d.ShowDialog() -eq "OK") { $tDir.Text = Resolve-InstallDir $d.SelectedPath }
})
# To move an installed copy: Remove, then install again from the download.
if ($IsInstalledCopy) { $tDir.ReadOnly = $true; $btnBrowse.Enabled = $false }
$cStart = Add-Check $p2 "Start menu shortcut (type 'watch' in Start to find it)" 30 74 $(if ($prev) { [bool]$prev.startMenu } else { -not $NoStartMenu })
$cDesk = Add-Check $p2 "Desktop shortcut" 30 100 $(if ($prev) { [bool]$prev.desktop } else { [bool]$Desktop })
$letters = @("W", "S", "J", "K", "Q", "Z")
$cKey = Add-Check $p2 "Keyboard shortcut: Ctrl + Alt +" 30 126 $(if ($prev) { [bool]$prev.hotkey } else { $true })
$cKey.Width = 250
$cLetter = Add-Combo $p2 285 125 50 $letters ([Array]::IndexOf($letters, (Pick $prev.hotkey "W")))
$cSign = Add-Check $p2 "Open Watch Studio when I sign in to Windows" 30 152 $(if ($prev) { [bool]$prev.atSignIn } else { [bool]$AtSignIn })
Add-Label $p2 "Show the app with:" 30 192 150 20 | Out-Null
$engineKeys = @($engines.Keys)
$engineNames = $engineKeys | ForEach-Object { if ($_ -eq "edge") { "Microsoft Edge (built into Windows)" } else { "Google Chrome" } }
if (-not $engineNames) { $engineNames = @("(none found)") }
$cEngine = Add-Combo $p2 190 190 300 $engineNames ([Math]::Max(0, [Array]::IndexOf($engineKeys, (Pick $prev.engine "edge"))))
Add-Label $p2 "Neither browser is changed: the app just borrows its engine, which is why it is so small." 30 222 500 36 | Out-Null

# Page 3: summary
$p3 = New-Page
Add-Label $p3 "Setup is ready. Press Install to continue, or Back to change something." 30 12 500 20 | Out-Null
$tSum = New-Object Windows.Forms.TextBox; $tSum.Multiline = $true; $tSum.ReadOnly = $true; $tSum.ScrollBars = "Vertical"
$tSum.SetBounds(30, 40, 494, 240); $p3.Controls.Add($tSum)

# Page 4: finished
$p4 = New-Page
$doneText = Add-Label $p4 "" 30 18 500 150
$cOpen = Add-Check $p4 "Open Watch Studio now" 30 180 $true

$pages = @($p0, $p1, $p2, $p3, $p4)
$titles = @(@("Welcome to Watch Studio Setup", "The companion app for your watch"),
            @("Personal touches", "Make it yours, or theirs"),
            @("Where and how it opens", "Choose the folder and shortcuts"),
            @("Ready to install", "Check your choices"),
            @("Finished", "Watch Studio is installed"))
$script:page = 0
$script:result = $null

function Get-Choices {
  @{ dir = (Resolve-InstallDir $tDir.Text); forName = $tName.Text.Trim(); message = $tMsg.Text.Trim(); from = $tFrom.Text.Trim()
     accent = $accents[$cAccent.SelectedIndex]; theme = $themes[$cTheme.SelectedIndex]
     startMenu = $cStart.Checked; desktop = $cDesk.Checked; hotkey = $(if ($cKey.Checked) { $letters[$cLetter.SelectedIndex] } else { "" })
     atSignIn = $cSign.Checked; engine = $(if ($engineKeys.Count) { $engineKeys[$cEngine.SelectedIndex] } else { "" }); register = $true }
}

function Show-Page([int]$n) {
  $script:page = $n
  for ($i = 0; $i -lt $pages.Count; $i++) { $pages[$i].Visible = ($i -eq $n) }
  $hTitle.Text = $titles[$n][0]; $hSub.Text = $titles[$n][1]
  $btnBack.Enabled = ($n -gt 0 -and $n -lt 4)
  $btnNext.Enabled = ($n -ne 0 -or $engines.Count -gt 0)
  $btnNext.Text = @("Next >", "Next >", "Next >", "Install", "Finish")[$n]
  $btnCancel.Enabled = ($n -lt 4)
  $btnRemove.Visible = ($n -eq 0 -and $null -ne $prev)
  if ($n -eq 3) {
    $c = Get-Choices
    $how = @()
    if ($c.startMenu) { $how += "Start menu" }
    if ($c.desktop) { $how += "desktop" }
    if ($c.hotkey) { $how += "Ctrl+Alt+$($c.hotkey)" }
    if ($c.atSignIn) { $how += "opens at sign-in" }
    $lines = @("Folder:", "    $($c.dir)", "", "Opens from:", "    $(if ($how) { $how -join ', ' } else { 'the folder only (no shortcuts)' })", "",
               "Shown with:", "    $($cEngine.Text)", "", "For:  $(Pick $c.forName '-')", "From:  $(Pick $c.from '-')",
               "Colour:  $($cAccent.Text),  appearance: $($cTheme.Text)")
    if ($c.message) { $lines += @("", "Message:", "    " + ($c.message -replace "`r?`n", "`r`n    ")) }
    $lines += @("", "Space needed: about 0.1 MB")
    $tSum.Text = $lines -join "`r`n"
  }
}

$btnBack.Add_Click({ Show-Page ($script:page - 1) })
$btnNext.Add_Click({
  if ($script:page -lt 3) { Show-Page ($script:page + 1); return }
  if ($script:page -eq 3) {
    $form.UseWaitCursor = $true
    try {
      $script:result = Install-App (Get-Choices)
      $c = Get-Choices
      $how = if ($c.hotkey -and ($c.startMenu -or $c.desktop)) { "`r`n`r`nOpen it any time with Ctrl + Alt + $($c.hotkey)." } else { "" }
      $doneText.Text = "Watch Studio is installed.`r`n`r`nTo open it, type 'watch' in the Start menu$(if ($c.desktop) { ' or double-click the desktop icon' }).$how`r`n`r`nTo pin it to the taskbar: open it, then right-click its taskbar icon and choose 'Pin to taskbar'.`r`n`r`nTo remove it: Settings > Apps > Installed apps > Watch Studio > Uninstall."
      Show-Page 4
    } catch {
      [Windows.Forms.MessageBox]::Show("Setup could not finish:`n`n$($_.Exception.Message)", "$AppName Setup", "OK", "Error") | Out-Null
    } finally { $form.UseWaitCursor = $false }
    return
  }
  if ($cOpen.Checked -and $script:result) {
    Start-Process -FilePath $script:result.exe -ArgumentList "--app=`"$($script:result.url)`"", "--window-size=1000,880"
  }
  $form.Tag = "done"; $form.Close()
})
$btnCancel.Add_Click({ $form.Close() })
$btnRemove.Add_Click({
  $form.Hide()
  if (-not (Uninstall-App $false)) { $form.Show() } else { $form.Tag = "done"; $form.Close() }
})
$form.Add_FormClosing({
  param($s, $e)
  if ($form.Tag -ne "done" -and $script:page -lt 4) {
    $a = [Windows.Forms.MessageBox]::Show("Exit Watch Studio Setup? Nothing has been changed.", "$AppName Setup", "YesNo", "Question")
    if ($a -ne "Yes") { $e.Cancel = $true }
  }
})

Show-Page 0
if ($env:WATCHSTUDIO_SNAPSHOT) {
  # Developer aid: saves a picture of each page to that folder instead of running the installer.
  $form.Opacity = 0; $form.Show()
  for ($i = 0; $i -lt 4; $i++) {
    Show-Page $i; [Windows.Forms.Application]::DoEvents()
    $bmp = New-Object Drawing.Bitmap($form.Width, $form.Height)
    $form.DrawToBitmap($bmp, (New-Object Drawing.Rectangle(0, 0, $form.Width, $form.Height)))
    $bmp.Save((Join-Path $env:WATCHSTUDIO_SNAPSHOT "page$i.png")); $bmp.Dispose()
  }
  $form.Tag = "done"; $form.Close(); exit 0
}
[void]$form.ShowDialog()
