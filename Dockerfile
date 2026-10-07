# escape=`

# Builds client.dll inside a Windows container (Docker must be in Windows-container mode).
# The container only compiles the DLL - injecting it into the game has to happen on the host.
#
#   docker build -t potatoclient-build .
#   docker create --name potatoclient-out potatoclient-build
#   docker cp potatoclient-out:C:\out .\out
#   docker rm potatoclient-out
#
# The base image must match the host's Windows build when using process isolation.
# ltsc2022 fits Windows 11 / Server 2022; on Windows 10 pass
# --build-arg WINDOWS_VERSION=ltsc2019 or build with --isolation=hyperv.

ARG WINDOWS_VERSION=ltsc2022

FROM mcr.microsoft.com/windows/servercore:${WINDOWS_VERSION} AS build
SHELL ["cmd", "/S", "/C"]

# Visual Studio 2019 Build Tools: MSVC x64, the bundled CMake and a Windows 10 SDK
# (the same toolchain the README uses for local builds). Exit code 3010 = "reboot required", which is fine here.
ADD https://aka.ms/vs/16/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe
RUN (start /w C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools" `
        --add Microsoft.VisualStudio.Workload.VCTools `
        --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        --add Microsoft.VisualStudio.Component.VC.CMake.Project `
        --add Microsoft.VisualStudio.Component.Windows10SDK.19041 `
     || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
 && del /q C:\TEMP\vs_buildtools.exe
