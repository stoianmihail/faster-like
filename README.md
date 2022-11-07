Faster FFT-based Wildcard Pattern Matching
====

The original algorithm for wildcard pattern matching contained some redundancy. 

We fully exploit convolution output and provide a 2x faster algorithm.

## Build

```
mkdir -p build
cd build
cmake ..
```

## Reproduce `random-512`

Generate data:

```
python3 gen.py
```

Run the benchmark:

```
./run.sh random512.in
```

Use `plot.ipynb` to visualize the results.

## Reproduce `o_comment`

Extract the `o_comment` column, e.g., using `extract.ipynb`.

Run the benchmark:

```
./run.sh o_comment.in
```

Use `plot.ipynb` to visualize the results.