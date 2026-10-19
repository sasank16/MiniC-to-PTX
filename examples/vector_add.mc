//
// MiniC-to-PTX Example: Vector Computation
// Computes an element-wise vector scale and accumulator loop
//
__global__ int vector_compute(int count, int scale) {
    int total = 0;
    int idx = 0;

    while (idx < count) {
        int val = idx * scale;
        total = total + val;
        idx = idx + 1;
    }

    return total;
}
