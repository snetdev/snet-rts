
pushd inputs
time ../mandelbrot_$1 -m 5 -dloc < $2 > $1-$2.out
popd
