cd "I:\Y2018C-Y2-PR-TDVS\Game\Engine\Assets\Shaders"

for %%f in (*.vs.hlsl) do (
    "%SCE_ORBIS_SDK_DIR%\host_tools\bin\orbis-wave-psslc.exe" -profile sce_vs_vs_orbis "%%~nf.hlsl" -cache -Od
)
pause