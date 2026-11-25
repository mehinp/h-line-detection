#include <rtl-sdr.h>
#include "spectrometer.h"
#include <iostream>
#include <cassert>
#include <complex>
#include <fstream>
constexpr int SAMPLE_RATE = 2'400'000;
constexpr int GAIN_MODE = 1;
constexpr int BIAS_TEE = 1;
constexpr int BUF_MAX = 1 << 18;
constexpr int FFT_SIZE = 2048;

struct Settings {
    int center_freq = 1'420'000'000;
    int gain = 280;
    double seconds = 10.0;
    std::string title = "test";
};

Settings parse(int argc, char* argv[]) {
    Settings s;
    for (int i = 1; i < argc; i++) {
        std::string which = argv[i];
        i += 1;
        std::string value = argv[i];
        if (which == "--freq") {
            s.center_freq = stoi(value);
        } else if (which == "--gain") {
            s.gain = stoi(value);
        } else if (which == "--seconds") {
            s.seconds = stod(value);
        } else if (which == "--title") {
            s.title = value;
        } else {
            std::cout << "Unknown setting: " << which << '\n';
            std::exit(1);
        }
    }
    return s;
}

void check(int res, const std::string& str, rtlsdr_dev_t* device) {
    if (res != 0) {
        std::cout << "\nFAILED " << str << '\n';
        if (device != nullptr) {
            rtlsdr_close(device);
        }
        std::exit(1);
    }
}

int main(int argc, char* argv[]) {
    Settings s = parse(argc, argv);
    rtlsdr_dev_t* device = nullptr;
    check(rtlsdr_open(&device, 0), "open", device);
    check(rtlsdr_set_sample_rate(device, SAMPLE_RATE), "set_sample_rate", device);
    check(rtlsdr_set_center_freq(device, s.center_freq), "set_center_freq", device); 
    check(rtlsdr_set_tuner_gain_mode(device, GAIN_MODE), "set_tuner_gain_mode", device);
    check(rtlsdr_set_tuner_gain(device, s.gain), "set_tuner_gain", device);
    check(rtlsdr_set_bias_tee(device, BIAS_TEE), "set_bias_tee_on", device);
    check(rtlsdr_reset_buffer(device), "reset_buffer", device);

    std::cout << "freq = " << s.center_freq << "\ngain = " << s.gain << "\nseconds = " << s.seconds << "\nlabel = " << s.title << "\n\n";
    Spectrometer spec(FFT_SIZE);
    std::vector<uint8_t> buffer(BUF_MAX);

    int n_read = 0;
    int total_reads = SAMPLE_RATE * s.seconds / (BUF_MAX / 2);
    for (int iter = 0; iter < total_reads; iter++) {
        check(rtlsdr_read_sync(device, buffer.data(), BUF_MAX, &n_read), "read_input", device);
        assert(n_read == BUF_MAX);

        std::vector<std::complex<float>> iq(BUF_MAX / 2);
        for (int i = 0; i < BUF_MAX - 1; i += 2) {
            iq[i / 2] = std::complex<float>((buffer[i] - 127.5) / 127.5, (buffer[i + 1] - 127.5) / 127.5);
        }
        spec.add(iq);
    }

    std::vector<double> results = spec.average();
    std::string path = "data/" + s.title + "_spectrum.csv";
    std::ofstream file(path);
    file << "frequency(Hz),power\n";
    for (int i = 0; i < FFT_SIZE; i++) {
        double power = results[i];
        double frequency = s.center_freq + (i - FFT_SIZE / 2.0) * ((double) SAMPLE_RATE / FFT_SIZE);
        file << std::fixed << std::setprecision(3) << frequency << ',' << std::setprecision(8) << power << '\n';
    }
    std::cout << "Processed " << 1LL * total_reads * (BUF_MAX / 2) << " I/Q pairs" << '\n';
    std::cout << "Results in " + path << '\n';
    rtlsdr_close(device);
}