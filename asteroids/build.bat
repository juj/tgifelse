@set COMMON=-s MINIMAL_RUNTIME=2 -s SUPPORT_ERRNO=0 -s ENVIRONMENT=web --shell-file shell.html -s TEXTDECODER=2 -s MALLOC=emmalloc --js-library library_asteroids.js -s TOTAL_MEMORY=256KB -s TOTAL_STACK=4KB -s ALLOW_MEMORY_GROWTH=1

@set OPTS=-O3 --closure 1 -s SINGLE_FILE=1

call em++ %COMMON% %OPTS% asteroids.cpp -o asteroids.html
