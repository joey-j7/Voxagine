cd "I:\Y2018C-Y2-PR-TDVS\Game\Engine\Assets\Shaders"

for %%f in (*.sb) do (
    del /f "%%~nf.sb"
)

for %%f in (*.sdb) do (
    del /f "%%~nf.sdb"
)