#include <rtl-sdr.h>
#include "spectrometer.h"
#include <iostream>
#include <cassert>
#include <complex>
#include <fstream>
constexpr int SAMPLE_RATE = 2'400'000;
constexpr int CENTER_FREQ = 98'000'000;
constexpr int GAIN_MODE = 1;
constexpr int GAIN = 280;
constexpr int BUF_MAX = 1 << 18;
constexpr int FFT_SIZE = 2048;

void check(int res, const std::string& str, rtlsdr_dev_t* device) {
    if (res != 0) {
        std::cout << "\nFAILED " << str << '\n';
        if (device != nullptr) {
            rtlsdr_close(device);
        }
        std::exit(1);
    }
}

int main() {
    rtlsdr_dev_t* device = nullptr;
    check(rtlsdr_open(&device, 0), "open", device);
    check(rtlsdr_set_sample_rate(device, SAMPLE_RATE), "set_sample_rate", device);
    check(rtlsdr_set_center_freq(device, CENTER_FREQ), "set_center_freq", device); 
    check(rtlsdr_set_tuner_gain_mode(device, GAIN_MODE), "set_tuner_gain_mode", device);
    check(rtlsdr_set_tuner_gain(device, GAIN), "set_tuner_gain", device);
    check(rtlsdr_reset_buffer(device), "reset_buffer", device);

    Spectrometer spec(FFT_SIZE);
    std::vector<uint8_t> buffer(BUF_MAX);
    int n_read = 0;
    int total_reads = 157; // equates to about 10,000 chunks (we get 64 chunks from each buffer read, 64 * 157 ~= 10,000)

    while (total_reads--) {
        check(rtlsdr_read_sync(device, buffer.data(), BUF_MAX, &n_read), "read_input", device);
        assert(n_read == BUF_MAX);

        std::vector<std::complex<float>> iq(BUF_MAX / 2);
        for (int i = 0; i < BUF_MAX - 1; i += 2) {
            iq[i / 2] = std::complex<float>((buffer[i] - 127.5) / 127.5, (buffer[i + 1] - 127.5) / 127.5);
        }
        spec.add(iq);
    }

    std::vector<double> results = spec.average();
    std::ofstream file("../data/spectrum.csv");
    file << "frequency,power\n";
    for (int i = 0; i < FFT_SIZE; i++) {
        double power = results[i];
        double frequency = CENTER_FREQ + (i - FFT_SIZE / 2.0) * ((double) SAMPLE_RATE / FFT_SIZE);
        file << std::fixed << std::setprecision(3) << frequency << ',' << power << '\n';
    }
    rtlsdr_close(device);
}