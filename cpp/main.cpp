#include "modulator.hpp"

#include <vector>

int main(int argc, const char **argv)
{
    QAMModulator modulator;
    QAMdeModulator demodulator;
    AWGNChannel channel;
    std::vector<std::complex<float>> symbols;
    std::vector<std::string> modscsv = {"qpsk", "qam16", "qam64"};
    std::vector<Modulation> mods = {Modulation::QPSK, Modulation::QAM16, Modulation::QAM64};
    size_t tests = 100;
    float min_db = -45.0f;
    float max_db = -1.0f;

    std::vector<float> BERs;
    std::vector<float> stddevs;
    std::vector<uint8_t> bits;
    std::vector<uint8_t> rx_bits;
    size_t L = 5000;

    modulator.set_modulation(Modulation::QAM16);
    demodulator.set_modulation(Modulation::QAM16);
    generate_bits(L, bits);
    modulator.modulate(bits, symbols);

    for (size_t k = 0; k < modscsv.size(); ++k)
    {
        modulator.set_modulation(mods[k]);
        demodulator.set_modulation(mods[k]);
        generate_bits(L, bits);
        modulator.modulate(bits, symbols);

        for (size_t i = 0; i < tests; ++i)
        {
            auto copy = symbols;
            auto db = (min_db - max_db) * (i + 1) / tests + max_db;
            channel.set_noise(db);
            channel.add_noise(copy);
            demodulator.demodulate(copy, rx_bits);
            auto ber = get_ber(bits, rx_bits);
            BERs.push_back(ber);
            stddevs.push_back(channel.get_sigma());
        }
        save_metrics(BERs, stddevs, "../../files/" + modscsv[k] + ".csv");
        BERs.clear();
        stddevs.clear();
    }
    return 0;
}
