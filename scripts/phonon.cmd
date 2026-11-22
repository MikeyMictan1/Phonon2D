@echo off
rem Runs phonon_cli.ps1 without needing to change PowerShell's execution policy. See phonon_cli.ps1 for commands.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0phonon_cli.ps1" %*
