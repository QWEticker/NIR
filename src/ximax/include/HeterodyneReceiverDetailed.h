#pragma once

class HeterodyneReceiverDetailed {
public:
    HeterodyneReceiverDetailed(double eta, double v_el, int adc_bits = 14, double adc_range = 1.0, double saturation_threshold = 1e6)
    : eta_(eta), v_el_(v_el), adc_bits_(adc_bits), adc_range_(adc_range), saturation_threshold_(saturation_threshold) {}

    double computeVdet() const {
        if (eta_ <= 0.0) return 1e9;
        double v_adc = 0.0;
        if (adc_bits_ > 0) {
            double qstep = (2.0 * adc_range_) / std::pow(2.0, adc_bits_);
            v_adc = (qstep * qstep) / 12.0;
        }
        return (v_el_ + v_adc) / eta_;
    }

private:
    double eta_;
    double v_el_;
    int adc_bits_;
    double adc_range_;
    double saturation_threshold_;
};
