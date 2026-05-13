Add-Type -AssemblyName System.Drawing

$pngPath = Join-Path $PSScriptRoot "resources\icons\app.png"
$icoPath = Join-Path $PSScriptRoot "resources\icons\app.ico"

$sizes = @(256, 128, 64, 48, 32, 16)
$pngStreams = @()

foreach ($size in $sizes) {
    $src = [System.Drawing.Image]::FromFile($pngPath)
    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawImage($src, 0, 0, $size, $size)
    $g.Dispose()
    $src.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $pngStreams += $ms
}

$icoStream = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter($icoStream)

$count = $sizes.Count
$bw.Write([uint16]0)
$bw.Write([uint16]1)
$bw.Write([uint16]$count)

$headerSize = 6 + $count * 16
$offset = $headerSize

foreach ($i in 0..($count - 1)) {
    $size = $sizes[$i]
    $pngBytes = $pngStreams[$i].ToArray()
    $byteCount = $pngBytes.Length

    $displaySize = if ($size -ge 256) { 0 } else { $size }
    $bw.Write([byte]$displaySize)
    $bw.Write([byte]$displaySize)
    $bw.Write([byte]0)
    $bw.Write([byte]0)
    $bw.Write([uint16]1)
    $bw.Write([uint16]32)
    $bw.Write([uint32]$byteCount)
    $bw.Write([uint32]$offset)
    $offset += $byteCount
}

foreach ($ms in $pngStreams) {
    $bw.Write($ms.ToArray())
    $ms.Dispose()
}

$bw.Close()
$icoStream.Close()

Write-Host " [OK] Icon saved to: $icoPath"
