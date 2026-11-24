#include "spectrometer.h"
#include <cassert>
constexpr int SIGN = FFTW_FORWARD;
constexpr unsigned FLAG = FFTW_MEASURE;

Spectrometer::Spectrometer(int n) {
    this->n = n;
    in = fftwf_alloc_complex(n);
    out = fftwf_alloc_complex(n);
    plan = fftwf_plan_dft_1d(n, in, out, SIGN, FLAG);
    total.resize(n);
}


void Spectrometer::add(const std::vector<std::complex<float>>& samples) {
    int m = samples.size();
    assert(m % n == 0);
    int chunks = m / n;

    for (int i = 0; i < chunks; i++) {
        for (int j = 0; j < n; j++) {
            in[j][0] = samples[i * n + j].real();
            in[j][1] = samples[i * n + j].imag();
        }
        fftwf_execute(plan);
        for (int j = 0; j < n; j++) {
            double power = out[j][0] * out[j][0] + out[j][1] * out[j][1];
            total[j] += power;
        }
    }
}

std::vector<double> Spectrometer::average() const {
    std::vector<double> res(n);
    
};