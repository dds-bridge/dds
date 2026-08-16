# End-to-end check for the .NET dd_table_for_deal CLI.
# Requires DDS_LIBRARY_PATH pointing at the Bazel-built shared library.
$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $Root

if (-not $env:DDS_LIBRARY_PATH) {
    Write-Error "DDS_LIBRARY_PATH is not set (build //jni:dds_shared and export the lib path)"
}
if (-not (Test-Path -LiteralPath $env:DDS_LIBRARY_PATH)) {
    Write-Error "DDS_LIBRARY_PATH does not exist: $($env:DDS_LIBRARY_PATH)"
}

$ExampleDeal = "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
$Project = "dotnet/DdTableForDeal/"

function Assert-Contains([string]$Haystack, [string]$Needle) {
    if (-not $Haystack.Contains($Needle)) {
        Write-Host "Missing expected output: $Needle" -ForegroundColor Red
        Write-Host "----- stdout -----"
        Write-Host $Haystack
        exit 1
    }
}

function Assert-NotContains([string]$Haystack, [string]$Needle) {
    if ($Haystack.Contains($Needle)) {
        Write-Host "Unexpected output: $Needle" -ForegroundColor Red
        Write-Host "----- stdout -----"
        Write-Host $Haystack
        exit 1
    }
}

function Invoke-CliCheck([string]$Label, [string]$Arg) {
    Write-Host "==> $Label"
    # Do not use Start-Process -ArgumentList with a spaced PBN string: PowerShell
    # joins the array into one command line without quoting, so the deal is split
    # into multiple argv entries ("Only one deal argument is allowed").
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = "dotnet"
    $psi.WorkingDirectory = "$Root"
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    [void]$psi.ArgumentList.Add("run")
    [void]$psi.ArgumentList.Add("--project")
    [void]$psi.ArgumentList.Add($Project)
    [void]$psi.ArgumentList.Add("--")
    [void]$psi.ArgumentList.Add($Arg)

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $outTask = $proc.StandardOutput.ReadToEndAsync()
    $errTask = $proc.StandardError.ReadToEndAsync()
    $proc.WaitForExit()
    $out = $outTask.GetAwaiter().GetResult()
    $err = $errTask.GetAwaiter().GetResult()
    if ($null -eq $out) { $out = "" }
    if ($null -eq $err) { $err = "" }

    if ($proc.ExitCode -ne 0) {
        Write-Host "dotnet run failed (exit $($proc.ExitCode))" -ForegroundColor Red
        Write-Host "----- stderr -----"
        Write-Host $err
        Write-Host "----- stdout -----"
        Write-Host $out
        exit 1
    }
    if ($err.Contains("DDS error:") -or $err.Contains("Failed to load native DDS library")) {
        Write-Host "Solver error on stderr:" -ForegroundColor Red
        Write-Host $err
        exit 1
    }

    Assert-Contains $out "dd_table_for_deal:"
    Assert-Contains $out "North"
    Assert-Contains $out "   NT     4     4     8     8"
    Assert-Contains $out "    S     3     3    10    10"
    Assert-Contains $out "    H     9     9     4     4"
    Assert-Contains $out "    D     8     8     4     4"
    Assert-Contains $out "    C     3     3     9     9"
    Assert-Contains $out "Par: NS 5Hx -2 -300"
    Assert-NotContains $out "NS score:"

    $northPos = $out.IndexOf("North")
    $parPos = $out.IndexOf("Par:")
    if ($northPos -lt 0 -or $parPos -lt 0 -or $northPos -ge $parPos) {
        Write-Host "Expected 'North' before 'Par:' in output" -ForegroundColor Red
        exit 1
    }
}

Invoke-CliCheck "inline PBN deal" $ExampleDeal
Invoke-CliCheck "hands/example.pbn" "hands/example.pbn"

Write-Host "DdTableForDeal e2e OK"
