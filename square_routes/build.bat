emcc square_routes.cpp -o square_routes.html -O3 -s MINIMAL_RUNTIME=2 -s ENVIRONMENT=web -s SINGLE_FILE=1 --shell-file shell.html --closure 1 -s TEXTDECODER=2 -s ALLOW_MEMORY_GROWTH=1 -mnontrapping-fptoint
