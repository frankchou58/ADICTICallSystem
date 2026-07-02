<#
.SYNOPSIS
補齊 third_party/WebView2/ 底下沒有進版控的 WebView2 SDK 檔案（build/、lib/、lib_manual/、runtimes/、tools/）。

.說明
這個 vcxproj 沒有設定 NuGet restore，WebView2 SDK 是手動下載 nupkg 解壓進來的。
其中中繼資料檔（.nuspec/.idl/.tlb/LICENSE 等）有進版控，但實際的標頭檔/函式庫/執行期 DLL
因為是 Microsoft 授權的二進位檔，被 .gitignore 排除，每個人 clone 下來後要自己執行這支腳本補齊一次。

.用法
在 PowerShell 執行：
    .\Fetch-WebView2Sdk.ps1
版本預設抓 third_party/WebView2/Microsoft.Web.WebView2.nuspec 記載的版本，
需要換版本就加參數：
    .\Fetch-WebView2Sdk.ps1 -Version 1.0.xxxx.xx
#>
param(
    [string]$Version
)

$ErrorActionPreference = "Stop"

$destDir = Join-Path $PSScriptRoot "WebView2"
$nuspecPath = Join-Path $destDir "Microsoft.Web.WebView2.nuspec"
$includeMarker = Join-Path $destDir "build\native\include\WebView2.h"

if (Test-Path $includeMarker) {
    Write-Host "WebView2 SDK 已經存在（$includeMarker），不需要重新下載。"
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

if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
}

$foldersToCopy = @("build", "lib", "lib_manual", "runtimes", "tools")
foreach ($folder in $foldersToCopy) {
    $src = Join-Path $extractDir $folder
    if (Test-Path $src) {
        Copy-Item -LiteralPath $src -Destination $destDir -Recurse -Force
        Write-Host "已複製 $folder/"
    }
}

Remove-Item -LiteralPath $tmpDir -Recurse -Force

if (Test-Path $includeMarker) {
    Write-Host "完成，WebView2.h 已就位：$includeMarker"
}
else {
    throw "下載/解壓完成，但找不到 $includeMarker，請檢查套件內容是否有變動。"
}
