# Generates non-copyrighted UWP package placeholder branding.
# Run before building XboxOneProbe.vcxproj (Windows PowerShell or pwsh on Windows).
param([string]$OutputDir = "$PSScriptRoot/../XboxOneProbe/Assets")
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
New-Item -Force -ItemType Directory -Path $OutputDir | Out-Null
function MakeLogo([string]$File, [int]$Width, [int]$Height) {
  $bitmap = [System.Drawing.Bitmap]::new($Width, $Height)
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.Clear([System.Drawing.Color]::FromArgb(9, 31, 47))
  $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $inset = [Math]::Max(3, [int]($Height / 12))
  $brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(41, 182, 125))
  $graphics.FillRectangle($brush, $inset, $inset, $Width - (2 * $inset), $Height - (2 * $inset))
  $font = [System.Drawing.Font]::new("Arial", [Math]::Max(12,[int]($Height / 4)), [System.Drawing.FontStyle]::Bold)
  $white = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
  $text = "PS3"
  $size = $graphics.MeasureString($text, $font)
  $graphics.DrawString($text, $font, $white,
    ($Width - $size.Width)/2, ($Height - $size.Height)/2)
  $path = Join-Path $OutputDir $File
  $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  $white.Dispose(); $font.Dispose(); $brush.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
}
MakeLogo "Logo44.png" 44 44
MakeLogo "Logo150.png" 150 150
MakeLogo "StoreLogo.png" 50 50
MakeLogo "WideLogo.png" 310 150
MakeLogo "Splash.png" 620 300
Write-Host "Generated probe icons in $OutputDir"
