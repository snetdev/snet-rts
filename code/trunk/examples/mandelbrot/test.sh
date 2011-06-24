
pushd inputs
time ../mandelbrot_$1  -m 5 -dloc < $2 > /dev/null
popd
