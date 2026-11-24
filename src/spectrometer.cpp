#include "spectrometer.h"
#include <cassert>
#include <stdexcept>
#include <cmath>
#include <numbers>
#include <algorithm>
constexpr int SIGN = FFTW_FORWARD;
constexpr unsigned FLAG = FFTW_MEASURE;

Spectrometer::Spectrometer(int n) {
    this->n = n;
    in = fftwf_alloc_complex(n);
    out = fftwf_alloc_complex(n);
    plan = fftwf_plan_dft_1d(n, in, out, SIGN, FLAG);
    total_power.resize(n);
    window.reserve(n);
    total_chunks = 0;
    for (int i = 0; i < n; i++) {
        window.push_back(0.5 - 0.5 * cos(2 * std::numbers::pi_v<float> * i / (n - 1)));
    }
}

Spectrometer::~Spectrometer() {
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);
}


void Spectrometer::add(const std::vector<std::complex<float>>& samples) {
    int m = samples.size();
    assert(m % n == 0);
    int chunks = m / n;

    for (int i = 0; i < chunks; i++) {
        for (int j = 0; j < n; j++) {
            in[j][0] = samples[i * n + j].real() * window[j];
            in[j][1] = samples[i * n + j].imag() * window[j];
        }
        fftwf_execute(plan);
        for (int j = 0; j < n; j++) {
            double power = out[j][0] * out[j][0] + out[j][1] * out[j][1];
            total_power[j] += power;
        }
        total_chunks += 1;
    }
}

std::vector<double> Spectrometer::average() const {
    if (total_chunks == 0) {
        throw std::runtime_error("Total chunks is 0.");
    }
    std::vector<double> res(n);
    for (int i = 0; i < n; i++) {
        res[i] = total_power[i] / total_chunks;
    }
    std::rotate(res.begin(), res.begin() + n / 2, res.end());
    return res;
};