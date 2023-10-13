setlocal
set RESOURCE_DIR=%%1
set COMPILER_PATH=%%2

for /R %%f in (%RESOURCE_DIR%*.vert) do (
	%COMPILER_PATH% %%f -o !%%f:.vert=_vert!.spv
)

for /R %%f in (%RESOURCE_DIR%*.frag) do (
	%COMPILER_PATH% %%f -o !%%f:.frag=_frag!.spv
)

endlocal
