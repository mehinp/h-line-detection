#pragma once
#include <complex>
#include <vector>
#include <fftw3.h>

class Spectrometer {
public:
    explicit Spectrometer(int n);
    void add(const std::vector<std::complex<float>>& samples);
    std::vector<double> average() const;

private:
    std::size_t n;
    fftwf_complex* in;
    fftwf_complex* out;
    std::vector<double> total;
    fftwf_plan plan;
};