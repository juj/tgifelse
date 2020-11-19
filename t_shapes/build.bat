@set COMMON=-s MINIMAL_RUNTIME=2 -s SUPPORT_ERRNO=0 -s ENVIRONMENT=web --shell-file shell.html -s TEXTDECODER=2 -s MALLOC=emmalloc --js-library library_t_shapes.js -s TOTAL_MEMORY=256KB -s TOTAL_STACK=4KB

@set OPTS=-O3 --closure 1

call emcc %COMMON% %OPTS% t_shapes.cpp -o t_shapes_sc.html
call emcc %COMMON% %OPTS% t_shapes_mt.cpp -o t_shapes_mt.html -s USE_WASM_WORKERS=2
call emcc %COMMON% %OPTS% t_shapes_simd.cpp -o t_shapes_sd.html -msimd128
call emcc %COMMON% %OPTS% t_shapes_simd_mt.cpp -o t_shapes_sd_mt.html -msimd128 -s USE_WASM_WORKERS=2
