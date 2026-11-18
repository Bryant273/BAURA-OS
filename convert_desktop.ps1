Add-Type -AssemblyName System.Drawing

function Convert-BmpToPng($src, $dst) {
    if (Test-Path $src) {
        $bmp = [System.Drawing.Bitmap]::FromFile($src)
        $bmp.Save($dst, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
        Write-Host "Converted $src -> $dst"
    }
}

Convert-BmpToPng "C:\Users\SDA\Downloads\splash_final.bmp" "C:\Users\SDA\.gemini\antigravity\brain\ee2114af-8f40-4f20-a039-3567a4c7272d\splash_final.png"
Convert-BmpToPng "C:\Users\SDA\Downloads\splash_strip.bmp" "C:\Users\SDA\.gemini\antigravity\brain\ee2114af-8f40-4f20-a039-3567a4c7272d\splash_strip.png"
Convert-BmpToPng "C:\Users\SDA\Downloads\splash_final.bmp" "C:\Users\SDA\Downloads\splash_final.png"
Convert-BmpToPng "C:\Users\SDA\Downloads\splash_strip.bmp" "C:\Users\SDA\Downloads\splash_strip.png"

