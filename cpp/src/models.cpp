#include "modulator.hpp"

#include <cstdio>
#include <random>
#include <vector>
#include <complex>
#include <bit>
#include <algorithm>
#include <cstdint>

constexpr auto QPSK_3GPP_SCALE = 0.707106562f;
constexpr auto QAM16_3GPP_SCALE = 0.316227732f;
constexpr auto QAM64_3GPP_SCALE = 0.154303343f;

std::complex<float> get_symbol(Modulation mod, uint8_t bits)
{
    const auto &gb = get_bit_from_block;

    switch (mod)
    {
    case Modulation::QPSK:
        return {(gb(bits, 0) * -2.0f + 1.0f) * QPSK_3GPP_SCALE,
                (gb(bits, 1) * -2.0f + 1.0f) * QPSK_3GPP_SCALE};
    case Modulation::QAM16:
        return {((1 - 2 * gb(bits, 0)) * (2 - (1 - 2 * gb(bits, 2)))) * QAM16_3GPP_SCALE,
                ((1 - 2 * gb(bits, 1)) * (2 - (1 - 2 * gb(bits, 3)))) * QAM16_3GPP_SCALE};
    case Modulation::QAM64:
        return {((1 - 2 * gb(bits, 0)) * (4 - (1 - 2 * gb(bits, 2)) * (2 - (1 - 2 * gb(bits, 4))))) * QAM64_3GPP_SCALE,
                ((1 - 2 * gb(bits, 1)) * (4 - (1 - 2 * gb(bits, 3)) * (2 - (1 - 2 * gb(bits, 5))))) * QAM64_3GPP_SCALE};
    default:
        return {0.0f, 0.0f};
    }
    return {0.0f, 0.0f};
}

uint8_t get_bps(Modulation mod)
{
    switch (mod)
    {
    case Modulation::QPSK:
        return 2;
    case Modulation::QAM16:
        return 4;
    case Modulation::QAM64:
        return 6;
    default:
        return 2;
    }
}

uint8_t get_bit_from_block(uint8_t &block, uint8_t position)
{
    return (block >> position) & 1;
}

void generate_bits(size_t L, std::vector<uint8_t> &bits)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint8_t> dis;

    bits.resize(L);
    for (size_t k = 0; k < L; ++k)
        bits[k] = dis(gen);
}

uint8_t QAMModulator::get_bits_from_vector(const uint8_t *data, size_t &offset, int n)
{
    while (bits_count < n)
    {
        accumulator |= (static_cast<uint32_t>(data[offset++]) << bits_count);
        bits_count += 8;
    }

    uint8_t result = accumulator & ((1 << n) - 1);

    accumulator >>= n;
    bits_count -= n;

    return result;
}

void QAMModulator::modulate(std::vector<uint8_t> &bytes, std::vector<std::complex<float>> &symbols)
{
    size_t bps = get_bps(this->mod);
    if ((bytes.size() * 8) % bps != 0)
        bytes.push_back(0);
    uint8_t qam_bits = 0;
    size_t num_symbols = bytes.size() * 8 / bps;
    size_t bytes_processed = 0;

    symbols.clear();
    symbols.resize(num_symbols);

    for (size_t i = 0; i < num_symbols; ++i)
    {
        qam_bits = get_bits_from_vector(bytes.data(), bytes_processed, bps);
        symbols[i] = get_symbol(mod, qam_bits);
    }
}

void QAMModulator::set_modulation(Modulation mod)
{
    this->mod = mod;
}

void QAMdeModulator::set_modulation(Modulation mod)
{
    this->mod = mod;
}

void AWGNChannel::add_noise(std::vector<std::complex<float>> &data)
{
    for (auto &x : data)
        x += std::complex<float>(noise(gen), noise(gen));
};

void AWGNChannel::set_noise(float db)
{
    noise_power = std::pow(10.0f, db / 10.0f);
    sigma = std::sqrt(noise_power / 2.0f);
    noise.param(std::normal_distribution<float>::param_type(0.0f, sigma));
    noise.reset();
};

void demod_2bit(float x, float scale, uint8_t &b_sign, uint8_t &b_mag)
{
    b_sign = (x < 0.0f) ? 1u : 0u;
    b_mag = (std::fabs(x) > 2.0f * scale) ? 1u : 0u;
}

void demod_3bit(float x, float scale, uint8_t &b_sign, uint8_t &b_mag1, uint8_t &b_mag0)
{
    static const float levels[4] = {3.0f, 1.0f, 5.0f, 7.0f};
    static const uint8_t mag_bits[4][2] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1},
    };

    b_sign = (x < 0.0f) ? 1u : 0u;

    float ax = std::fabs(x) / scale;

    int best = 0;
    float best_dist = std::fabs(ax - levels[0]);
    for (int k = 1; k < 4; ++k)
    {
        float d = std::fabs(ax - levels[k]);
        if (d < best_dist)
        {
            best_dist = d;
            best = k;
        }
    }

    b_mag1 = mag_bits[best][0];
    b_mag0 = mag_bits[best][1];
}

uint8_t get_bits_from_symbol(Modulation mod, std::complex<float> sym)
{
    const float I = sym.real();
    const float Q = sym.imag();
    uint8_t bits = 0;

    switch (mod)
    {
    case Modulation::QPSK:
    {
        uint8_t b0 = (I < 0.0f) ? 1 : 0;
        uint8_t b1 = (Q < 0.0f) ? 1 : 0;
        bits = b0 | (b1 << 1);
        break;
    }
    case Modulation::QAM16:
    {
        uint8_t b0, b2, b1, b3;
        demod_2bit(I, QAM16_3GPP_SCALE, b0, b2);
        demod_2bit(Q, QAM16_3GPP_SCALE, b1, b3);
        bits = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3);
        break;
    }
    case Modulation::QAM64:
    {
        uint8_t b0, b2, b4, b1, b3, b5;
        demod_3bit(I, QAM64_3GPP_SCALE, b0, b2, b4);
        demod_3bit(Q, QAM64_3GPP_SCALE, b1, b3, b5);
        bits = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3) | (b4 << 4) | (b5 << 5);
        break;
    }
    default:
        bits = 0;
        break;
    }
    return bits;
}

void QAMdeModulator::put_bits_to_vector(uint8_t bits, int n, std::vector<uint8_t> &out)
{
    accumulator |= (static_cast<uint32_t>(bits & ((1u << n) - 1u)) << bits_count);
    bits_count += n;

    while (bits_count >= 8)
    {
        out.push_back(static_cast<uint8_t>(accumulator & 0xFFu));
        accumulator >>= 8;
        bits_count -= 8;
    }
}

void QAMdeModulator::demodulate(const std::vector<std::complex<float>> &symbols, std::vector<uint8_t> &bytes)
{
    const size_t bps = get_bps(this->mod);

    bytes.clear();
    bytes.reserve((symbols.size() * bps + 7) / 8);

    for (const auto &sym : symbols)
    {
        uint8_t bits = get_bits_from_symbol(mod, sym);
        put_bits_to_vector(bits, static_cast<int>(bps), bytes);
    }

    if (bits_count > 0)
        bytes.push_back(static_cast<uint8_t>(accumulator & 0xFFu));
}

float get_ber(const std::vector<uint8_t> &tx, const std::vector<uint8_t> &rx)
{
    size_t size = std::min(tx.size(), rx.size());
    size_t bit_errors = 0;

    for (size_t i = 0; i < size; ++i)
        bit_errors += std::popcount(static_cast<uint8_t>(tx[i] ^ rx[i]));
    size_t total_bits = size * 8;
    return static_cast<float>(bit_errors) / static_cast<float>(total_bits);
}

void save_metrics(std::vector<float> ber, std::vector<float> stddev, const std::string &filename)
{
    std::ofstream ofs(filename, std::ios::out);
    size_t size = std::min(ber.size(), stddev.size());
    if (!ofs.is_open())
        return;
    for (size_t i = 0; i < size; ++i)
        if (ber[i] > 0.0)
            ofs << stddev[i] << "," << ber[i] << "\n";
}