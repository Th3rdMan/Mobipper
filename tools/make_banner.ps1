# Flipper-style 128x64 1-bit banner, upscaled x10 to 1280x640.
param([string]$Out = "$PSScriptRoot\..\docs\social_preview.png", [string]$Name = 'Mobipper')
Add-Type -AssemblyName System.Drawing

# NB: PowerShell variable names are case-insensitive and dynamically
# scoped, so screen size lives in $ScrW/$ScrH and helpers avoid $w/$h.
$ScrW = 128; $ScrH = 64
$px = New-Object 'bool[,]' $ScrW, $ScrH

function Plot([int]$x, [int]$y) { if ($x -ge 0 -and $x -lt $script:ScrW -and $y -ge 0 -and $y -lt $script:ScrH) { $script:px[$x, $y] = $true } }
function Rect([int]$x, [int]$y, [int]$rw, [int]$rh) { for ($i = $x; $i -lt $x + $rw; $i++) { for ($j = $y; $j -lt $y + $rh; $j++) { Plot $i $j } } }
function HLine([int]$x, [int]$y, [int]$len) { Rect $x $y $len 1 }
function VLine([int]$x, [int]$y, [int]$len) { Rect $x $y 1 $len }
function Grid([string[]]$rows, [int]$ox, [int]$oy, [int]$s = 1) {
    for ($y = 0; $y -lt $rows.Count; $y++) { for ($x = 0; $x -lt $rows[$y].Length; $x++) {
        if ($rows[$y][$x] -eq '#') { Rect ($ox + $x * $s) ($oy + $y * $s) $s $s } } }
}
# Crisp 1-bit text: render without antialiasing, copy dark pixels so that
# the glyphs' ink box starts at (ox, oy). Returns the ink box size.
function Text([string]$t, [string]$font, [float]$size, [int]$ox, [int]$oy, [Drawing.FontStyle]$style = 'Regular', [switch]$Measure) {
    $b = New-Object Drawing.Bitmap 300, 64
    $g = [Drawing.Graphics]::FromImage($b); $g.Clear([Drawing.Color]::White)
    $g.TextRenderingHint = 'SingleBitPerPixelGridFit'
    $f = New-Object Drawing.Font $font, $size, $style, ([Drawing.GraphicsUnit]::Pixel)
    $g.DrawString($t, $f, [Drawing.Brushes]::Black, 0, 0)
    $ink = New-Object System.Collections.Generic.List[int[]]
    $minx = 999; $maxx = -1; $miny = 999; $maxy = -1
    for ($x = 0; $x -lt 300; $x++) { for ($y = 0; $y -lt 64; $y++) {
        if ($b.GetPixel($x, $y).R -lt 128) {
            $ink.Add(@($x, $y))
            if ($x -lt $minx) { $minx = $x }; if ($x -gt $maxx) { $maxx = $x }
            if ($y -lt $miny) { $miny = $y }; if ($y -gt $maxy) { $maxy = $y } } } }
    $g.Dispose(); $b.Dispose()
    if (-not $Measure) { foreach ($p in $ink) { Plot ($ox + $p[0] - $minx) ($oy + $p[1] - $miny) } }
    return @{ Width = $maxx - $minx + 1; Height = $maxy - $miny + 1 }
}

# --- Flipper UI frame -------------------------------------------------------
# status bar like the Flipper desktop: battery at right, small icons at left
HLine 0 9 128
Grid @('#####.', '#...##', '#...##', '#####.') 3 2          # SD card-ish
Grid @('.#.#.', '#.#.#', '.#.#.') 11 3                       # NFC waves dots
Grid @('##############', '#............##', '#.##########.##', '#.##########.##', '#............##', '##############') 110 2  # battery
# rounded outer frame for the content area
HLine 2 12 124; HLine 2 62 124; VLine 1 13 49; VLine 126 13 49

# --- Big card icon (the app icon x3) ---------------------------------------
$icon = @('..........', '.########.', '#........#', '#.##..#..#', '##..#..#.#', '##..#..#.#', '#.##..#..#', '#........#', '.########.', '..........')
Grid $icon 6 18 3

# --- Name -------------------------------------------------------------------
$null = Text $Name 'Arial' 17 44 17 ([Drawing.FontStyle]::Bold)

# --- Modified mobib logo: m (o) b i b ))) -----------------------------------
# Letters are aligned on the x-height of the "m"; the "o" becomes the card
# ring and the word ends with the contactless waves, like the app icon.
$lx = 46; $base = 45                                  # bottom row of the x-height
$m = Text 'm' 'Arial' 13 0 0 ([Drawing.FontStyle]::Bold) -Measure
$xh = $m.Height
$null = Text 'm' 'Arial' 13 $lx ($base - $xh + 1) ([Drawing.FontStyle]::Bold)
$rx = $lx + $m.Width + 2
$ring = @('..####..', '.######.', '##....##', '##....##', '##....##', '##....##', '##....##', '.######.', '..####..')
Grid $ring $rx ($base - $ring.Count + 1)
$bib = Text 'bib' 'Arial' 13 0 0 ([Drawing.FontStyle]::Bold) -Measure
$null = Text 'bib' 'Arial' 13 ($rx + 10) ($base - $bib.Height + 1) ([Drawing.FontStyle]::Bold)
$wave = @('##...##..', '.##...##.', '..##...##', '..##...##', '..##...##', '..##...##', '..##...##', '.##...##.', '##...##..')
Grid $wave ($rx + 10 + $bib.Width + 3) ($base - $wave.Count + 1)

# --- Tagline: first wording that fits at a legible size ---------------------
# (below 9 px the 1-bit Windows fonts turn to mush)
$sz = 9
foreach ($tag in 'Lecteur MOBIB pour Flipper Zero', 'Lecteur MOBIB - Flipper Zero', 'Lecteur de cartes MOBIB') {
    $tm = Text $tag 'Tahoma' $sz 0 0 -Measure; if ($tm.Width -le 118) { break }
}
$null = Text $tag 'Tahoma' $sz ([int]((128 - $tm.Width) / 2)) (59 - $tm.Height + 1)
"tagline: '$tag' ($($tm.Width) px)"

# --- Render x10 on the Flipper orange LCD -----------------------------------
$S = 10
$img = New-Object Drawing.Bitmap ($ScrW * $S), ($ScrH * $S)
$g = [Drawing.Graphics]::FromImage($img)
$g.Clear([Drawing.Color]::FromArgb(255, 140, 41))
$on = New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(20, 12, 6))
for ($x = 0; $x -lt $ScrW; $x++) { for ($y = 0; $y -lt $ScrH; $y++) { if ($px[$x, $y]) { $g.FillRectangle($on, $x * $S, $y * $S, $S - 1, $S - 1) } } }
# faint LCD grid between pixels
$grid = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(25, 0, 0, 0)), 1
for ($x = 0; $x -le $ScrW; $x++) { $g.DrawLine($grid, $x * $S - 1, 0, $x * $S - 1, $ScrH * $S) }
for ($y = 0; $y -le $ScrH; $y++) { $g.DrawLine($grid, 0, $y * $S - 1, $ScrW * $S, $y * $S - 1) }
$img.Save($Out, [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $img.Dispose()
"saved $Out ($((Get-Item $Out).Length) bytes)"
