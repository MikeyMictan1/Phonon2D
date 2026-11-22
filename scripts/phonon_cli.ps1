# Phonon2D project helper.
#
#   phonon build             dev build: unoptimised + debug info, writes compile_commands.json
#   phonon build -Fast       optimised debug build (still loads in the editor). Use this for benchmarks.
#   phonon build -Release    template_release build (only used by exported release games)
#   phonon build <args>      anything else is passed straight to SCons (e.g. phonon build -c to clean)
#   phonon help              show this
#
# phonon.cmd runs this script (named differently so PowerShell picks the .cmd), so it works even where PowerShell blocks .ps1 files.

param(
	[Parameter(Position = 0)]
	[string]$Command = "help",
	[switch]$Fast,
	[switch]$Release,
	[Parameter(ValueFromRemainingArguments = $true)]
	[string[]]$Extra
)

$root = Split-Path $PSScriptRoot -Parent

function Show-Help {
	Get-Content $PSCommandPath | Where-Object { $_ -match '^#' } | ForEach-Object { $_ -replace '^# ?', '' }
}

function Invoke-Build {
	$scons = Join-Path $root ".venv\Scripts\scons.exe"
	if (-not (Test-Path $scons)) {
		Write-Host "SCons not found at $scons" -ForegroundColor Red
		Write-Host "Create the venv first:  py -m venv .venv  then  .venv\Scripts\pip install scons"
		exit 1
	}

	$sconsArgs = @("platform=windows", "compiledb=yes", "-j$env:NUMBER_OF_PROCESSORS")
	if ($Release) {
		$sconsArgs += "target=template_release"
	} elseif ($Fast) {
		$sconsArgs += "target=template_debug"
	} else {
		$sconsArgs += @("target=template_debug", "dev_build=yes")
	}
	if ($Extra) {
		$sconsArgs += $Extra
	}

	Write-Host "scons $($sconsArgs -join ' ')" -ForegroundColor Cyan
	Push-Location $root
	try {
		& $scons @sconsArgs
		exit $LASTEXITCODE
	} finally {
		Pop-Location
	}
}

switch ($Command) {
	"build" { Invoke-Build }
	"help" { Show-Help }
	default {
		Write-Host "Unknown command '$Command'." -ForegroundColor Red
		Show-Help
		exit 1
	}
}
