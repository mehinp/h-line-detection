#include <rtl-sdr.h>
#include <iostream>
#include <cassert>
constexpr int SAMPLE_RATE = 2'400'000;
constexpr int CENTER_FREQ = 98'000'000;
constexpr int GAIN_MODE = 1;
constexpr int GAIN = 280;

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


    rtlsdr_close(device);
}