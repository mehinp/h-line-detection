#include <rtl-sdr.h>
#include "spectrometer.h"
#include <iostream>
#include <cassert>
#include <complex>
#include <fstream>
#include <chrono>
#include <string>
#include <sstream>
constexpr int SAMPLE_RATE = 2'400'000;
constexpr int GAIN_MODE = 1;
constexpr int BIAS_TEE = 1;
constexpr int BUF_MAX = 1 << 18;
constexpr int FFT_SIZE = 2048;
constexpr double LATITUDE = 39.307;
constexpr double LONGITUDE = -76.811;

using Time = std::chrono::system_clock::time_point;

struct Settings {
    int center_freq = 1'420'000'000;
    int gain = 280;
    double seconds = 10.0;
    std::string title = "test";
};

struct Metadata {
    std::string title;
    std::string target;
    Time start_time;
    Time end_time;
    int chunks;
    int center_freq_hz;
    int sample_rate;
    int fft_size;
    int gain_tenths_db;
    double lat;
    double lng; 
    double alt_deg;
    double az_deg;
};

Settings parse(int argc, char* argv[], Metadata& meta) {
    Settings s;
    for (int i = 1; i < argc; i++) {
        std::string which = argv[i];
        i += 1;
        std::string value = argv[i];
        if (which == "--target") {
            meta.target = value;
        } else if (which == "--alt") {
            meta.alt_deg = stod(value);
        } else if (which == "--az") {
            meta.az_deg = stod(value);
        } else if (which == "--freq") {
            s.center_freq = stoi(value);
            meta.center_freq_hz = s.center_freq;
        } else if (which == "--gain") {
            s.gain = stoi(value);
            meta.gain_tenths_db = s.gain;
        } else if (which == "--seconds") {
            s.seconds = stod(value);
        } else if (which == "--title") {
            s.title = value;
            meta.title = s.title;
        } else {
            std::cout << "Unknown setting: " << which << '\n';
            std::exit(1);
        }
    }
    meta.lat = LATITUDE;
    meta.lng = LONGITUDE;
    meta.fft_size = FFT_SIZE;
    meta.sample_rate = SAMPLE_RATE;
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

std::string to_utc_string(Time t) {
    // time formatting thanks to claude
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::ostringstream out;
    out << std::put_time(std::gmtime(&tt), "%FT%TZ");
    return out.str();
}

void gen_file(Metadata& meta, std::vector<double>& results) {
    std::string path = "data/" + meta.title + to_utc_string(meta.end_time) + ".csv";
    std::ofstream file(path);
    file << "# Title: " << meta.title << '\n';
    file << "# Target: " << meta.target << '\n';
    file << "# Start time: " << to_utc_string(meta.start_time) << '\n';
    file << "# End time: " << to_utc_string(meta.end_time) << '\n';
    file << "# Chunks: " << meta.chunks << '\n';
    file << "# Center frequency (Hz): " << meta.center_freq_hz << '\n';
    file << "# Sample rate (per second): " << meta.sample_rate << '\n';
    file << "# FFT size: " << meta.fft_size << '\n';
    file << "# Gain (tenths of db): " << meta.gain_tenths_db << '\n';
    file << "# Source latitude: " << meta.lat << '\n';
    file << "# Source longitude: " << meta.lng << '\n';
    file << "# Altitude degree: " << meta.alt_deg << '\n';
    file << "# Azimuth degree: " << meta.az_deg << '\n';
    file << "frequency(Hz),power\n";
    for (int i = 0; i < FFT_SIZE; i++) {
        double power = results[i];
        double frequency = meta.center_freq_hz + (i - FFT_SIZE / 2.0) * ((double) SAMPLE_RATE / FFT_SIZE);
        file << std::fixed << std::setprecision(3) << frequency << ',' << std::setprecision(8) << power << '\n';
    }
} 


int main(int argc, char* argv[]) {
    Metadata meta;
    Settings s = parse(argc, argv, meta);
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
    meta.chunks = total_reads * (BUF_MAX / 2 / FFT_SIZE);
    meta.start_time = std::chrono::system_clock::now();
    double average_power = 0;
    for (int iter = 0; iter < total_reads; iter++) {
        check(rtlsdr_read_sync(device, buffer.data(), BUF_MAX, &n_read), "read_input", device);
        assert(n_read == BUF_MAX);

        std::vector<std::complex<float>> iq(BUF_MAX / 2);
        for (int i = 0; i < BUF_MAX - 1; i += 2) {
            iq[i / 2] = std::complex<float>((buffer[i] - 127.5) / 127.5, (buffer[i + 1] - 127.5) / 127.5);
            average_power += iq[i / 2].real() * iq[i / 2].real() + iq[i / 2].imag() * iq[i / 2].imag();
        }

        if ((iter + 1) % 18 == 0) {
            average_power /= (18 * BUF_MAX / 2);
            average_power = 10 * log10(average_power);
            std::cout << "t = " << (iter + 1) / 18 << "s" << " power = " << average_power << "dB" << '\n';
            average_power = 0;
        }
        spec.add(iq);
    }
    meta.end_time = std::chrono::system_clock::now();
    std::vector<double> results = spec.average();
    gen_file(meta, results);

    std::cout << "Processed " << 1LL * total_reads * (BUF_MAX / 2) << " I/Q pairs" << '\n';
    rtlsdr_close(device);
}