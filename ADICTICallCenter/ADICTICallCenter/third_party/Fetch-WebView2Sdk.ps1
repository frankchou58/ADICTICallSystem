<#
.SYNOPSIS
下載/升級 third_party/WebView2/build/native/ 底下這個專案實際會用到的 WebView2 SDK 檔案
（include/、x64/、x86/ 三個資料夾）。

.說明
這個 vcxproj 沒有設定 NuGet restore，WebView2 SDK 是手動下載 nupkg 解壓進來、直接 commit 進版控的。
只保留這個 native C++ MFC 專案實際會用到的子集（build/native/include + x64 + x86），
nupkg 裡其餘的 arm64（專案沒宣告這個平台）、lib/lib_manual/runtimes/tools（給 .NET/WinRT/WPF
專案用的 managed 投影版本，這個專案用不到）都不下載，避免 repo 背不必要的重量。

.用法
一般情況下不需要執行這支腳本——build/native/{include,x64,x86} 已經直接 commit 在 repo 裡，
clone 下來就能建置。只有要升級 WebView2 SDK 版本時才需要：
    1. 更新 third_party/WebView2/Microsoft.Web.WebView2.nuspec 裡的版本號
    2. 執行：.\Fetch-WebView2Sdk.ps1 -Version 1.0.xxxx.xx -Force
    3. git diff 確認 build/native/ 底下的變動，commit 進去
#>
param(
    [string]$Version,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$destDir = Join-Path $PSScriptRoot "WebView2"
$nuspecPath = Join-Path $destDir "Microsoft.Web.WebView2.nuspec"
$nativeDestDir = Join-Path $destDir "build\native"
$includeMarker = Join-Path $nativeDestDir "include\WebView2.h"

if ((Test-Path $includeMarker) -and (-not $Force)) {
    Write-Host "WebView2 SDK 已經存在（$includeMarker），不需要重新下載。"
    Write-Host "如果是要升級版本，請加上 -Force。"
    exit 0
}

if (-not $Version) {
    if (-not (Test-Path $nuspecPath)) {
        throw "找不到 $nuspecPath，無法自動判斷版本號，請用 -Version 參數指定。"
    }
    [xml]$nuspec = Get-Content -LiteralPath $nuspecPath
    $Version = $nuspec.package.metadata.version
    if (-not $Version) {
        throw "無法從 $nuspecPath 解析出版本號，請用 -Version 參數指定。"
    }
}

Write-Host "準備下載 Microsoft.Web.WebView2 $Version ..."

$packageId = "microsoft.web.webview2"
$url = "https://api.nuget.org/v3-flatcontainer/$packageId/$Version/$packageId.$Version.nupkg"
$tmpDir = Join-Path $env:TEMP "webview2-sdk-fetch-$Version"
if (Test-Path $tmpDir) {
    Remove-Item -LiteralPath $tmpDir -Recurse -Force
}
New-Item -ItemType Directory -Path $tmpDir | Out-Null

$nupkgPath = Join-Path $tmpDir "$packageId.$Version.nupkg"
$zipPath = Join-Path $tmpDir "$packageId.$Version.zip"

Invoke-WebRequest -Uri $url -OutFile $nupkgPath
Copy-Item -LiteralPath $nupkgPath -Destination $zipPath

$extractDir = Join-Path $tmpDir "extracted"
Expand-Archive -LiteralPath $zipPath -DestinationPath $extractDir -Force

if (Test-Path $nativeDestDir) {
    Remove-Item -LiteralPath $nativeDestDir -Recurse -Force
}
New-Item -ItemType Directory -Path $nativeDestDir -Force | Out-Null

$subFoldersToCopy = @("include", "x64", "x86")
foreach ($sub in $subFoldersToCopy) {
    $src = Join-Path $extractDir "build\native\$sub"
    if (-not (Test-Path $src)) {
        throw "套件裡找不到 build\native\$sub，套件內容可能改了版面配置，需要人工檢查。"
    }
    Copy-Item -LiteralPath $src -Destination $nativeDestDir -Recurse -Force
    Write-Host "已複製 build/native/$sub/"
}

Remove-Item -LiteralPath $tmpDir -Recurse -Force

if (Test-Path $includeMarker) {
    Write-Host "完成，WebView2.h 已就位：$includeMarker"
    Write-Host "請檢查 git diff 並 commit build/native/ 底下的變動。"
}
else {
    throw "下載/解壓完成，但找不到 $includeMarker，請檢查套件內容是否有變動。"
}
