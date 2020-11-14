call emcc t_shapes.cpp -o t_shapes_fast.html -O3 -s MINIMAL_RUNTIME=2 -s ENVIRONMENT=web -s SINGLE_FILE=0 --shell-file shell.html --closure 1 -s TEXTDECODER=2 -DFAST_VERSION --profiling-funcs -g
call emcc t_shapes.cpp -o t_shapes_slow.html -O3 -s MINIMAL_RUNTIME=2 -s ENVIRONMENT=web -s SINGLE_FILE=0 --shell-file shell.html --closure 1 -s TEXTDECODER=2 --profiling-funcs -g
