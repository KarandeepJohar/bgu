// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>

#include "bgu_3x4.h"
#include "fit_only_3x4.h"

#include "benchmark.h"
#include "Halide.h"
#include "halide_image_io.h"

using namespace Halide;
using namespace Halide::Internal;

// using namespace Halide::Tools;

int main(int argc, char **argv) {

    if (argc < 7) {
        printf("Usage: ./filter low_res_in.png low_res_out.png high_res_in.png high_res_out.png range_bins spatial_sigma\n");
        return 0;
    }

    Image<float> low_res_in = Tools::load_image(argv[1]);
    Image<float> low_res_out = Tools::load_image(argv[2]);
    Image<float> high_res_in = Tools::load_image(argv[3]);
    Image<float> high_res_out(high_res_in.width(), high_res_in.height(), high_res_in.channels());
    float r_sigma = 1.0f/atoi(argv[5]);
    float s_sigma = atoi(argv[6]);

    // Fit the curves and slice out the result.
    bgu_3x4(r_sigma, s_sigma, low_res_in, low_res_out, high_res_in, high_res_out);
    Tools::save_image(high_res_out, argv[4]);

    // You'd normally slice out the result using a shader. Check the
    // runtime of curve fitting alone.
    buffer_t coeffs = {0};
    int grid_w = low_res_in.width() / s_sigma;
    int grid_h = low_res_in.height() / s_sigma;
    int grid_z = round(1.0f/r_sigma);
    grid_w = ((grid_w+7)/8)*8;
    coeffs.host = (uint8_t *)malloc(sizeof(float) * 12 * grid_w * grid_h * grid_z);
    coeffs.extent[0] = grid_w;
    coeffs.extent[1] = grid_h;
    coeffs.extent[2] = grid_z;
    coeffs.extent[3] = 12;
    coeffs.stride[0] = 1;
    for (int i = 1; i < 4; i++) {
        coeffs.stride[i] = coeffs.extent[i-1] * coeffs.stride[i-1];
    }
    coeffs.elem_size = 4;

    double min_t = benchmark(10, 10, [&]() {
        fit_only_3x4(r_sigma, s_sigma, low_res_in, low_res_out, high_res_in, &coeffs);
    });
    printf("Time for fitting: %gms\n", min_t * 1e3);

    return 0;
}
