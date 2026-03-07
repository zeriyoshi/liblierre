# liblierre Emscripten support

add_compile_options(
  "-pthread"
)

add_link_options(
  "-sWASM=1"
  "-sINITIAL_MEMORY=134217728"
  "-sSTACK_SIZE=5242880"
  "-pthread"
  "-sPTHREAD_POOL_SIZE=8"
  "-sNODERAWFS=1"
)
