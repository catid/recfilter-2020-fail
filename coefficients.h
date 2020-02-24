#ifndef _COEFFICIENTS_H_
#define _COEFFICIENTS_H_

Halide::Buffer<float> matrix_B(
        Halide::Buffer<float> feedfwd_coeff,
        Halide::Buffer<float> feedback_coeff,
        int scan_id,
        int tile_width,
        bool clamp_border);

Halide::Buffer<float> matrix_R(
        Halide::Buffer<float> feedback_coeff,
        int scan_id,
        int tile_width);

Halide::Buffer<float> matrix_transpose(Halide::Buffer<float> A);
Halide::Buffer<float> matrix_mult(Halide::Buffer<float> A, Halide::Buffer<float> B);
Halide::Buffer<float> matrix_antidiagonal(int size);


#endif // _COEFFICIENTS_H_
