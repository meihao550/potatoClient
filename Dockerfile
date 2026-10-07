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
