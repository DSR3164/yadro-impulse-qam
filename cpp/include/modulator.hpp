#pragma once

#include <cstdint>
#include <vector>
#include <ccomplex>
#include <random>
#include <fstream>

enum class Modulation
{
    QPSK,
    QAM16,
    QAM64
};

class QAMModulator
{
public:
    QAMModulator() {};
    void modulate(std::vector<uint8_t> &bits, std::vector<std::complex<float>> &symbols);
    void set_modulation(Modulation mod);
    uint8_t get_bits_from_vector(const uint8_t *data, size_t &offset, int n);

private:
    Modulation mod{Modulation::QPSK};
    uint32_t accumulator = 0;
    int bits_count = 0;
};

class QAMdeModulator
{
public:
    QAMdeModulator() {};
    void demodulate(const std::vector<std::complex<float>> &symbols, std::vector<uint8_t> &bits);
    void set_modulation(Modulation mod);
    void put_bits_to_vector(uint8_t bits, int n, std::vector<uint8_t> &out);

private:
    Modulation mod{Modulation::QPSK};
    uint32_t accumulator = 0;
    int bits_count = 0;
};

class AWGNChannel
{
public:
    AWGNChannel(float db = -105.0f)
    {
        noise_power = std::pow(10.0f, db / 10.0f);
        sigma = std::sqrt(noise_power / 2.0f);
        noise.param(std::normal_distribution<float>::param_type(0.0f, sigma));
        noise.reset();
    };

    void add_noise(std::vector<std::complex<float>> &data);
    void set_noise(float db);
    float get_sigma() { return sigma; };

private:
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::normal_distribution<float> noise{0.0f, 1.0f};
    float sigma;
    float noise_power;
};

void save_metrics(std::vector<float> ber, std::vector<float> stddev, const std::string &filename);
void generate_bits(size_t L, std::vector<uint8_t> &bits);
uint8_t get_bit_from_block(uint8_t &block, uint8_t position);
float get_ber(const std::vector<uint8_t> &tx, const std::vector<uint8_t> &rx);