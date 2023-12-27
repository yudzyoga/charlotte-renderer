#include <lightwave.hpp>

#include <functional>
#include "halton.h"

namespace lightwave {

/**
 * @brief Generates random numbers uniformely distributed in [0,1), which are all stochastically independent from another.
 * This is the simplest form of random number generation, and will be sufficient for our introduction to Computer Graphics.
 * If you want to reduce the noise in your renders, a simple way is to implement more sophisticated random numbers (e.g.,
 * jittered sampling or blue noise sampling).
 * @see Internally, this sampler uses the PCG32 library to generate random numbers.
 */
class Halton : public Sampler {
    enum class RandomizeStrategy { None, PermuteDigits, FastOwen, Owen };
    // uint64_t m_seed;
    // pcg32 m_pcg;
    // halton m_halton;
    int m_primeCount = 0;
    int m_maxPrimeCount = 1000;
    int m_lastPrimeNum = 2;
    int m_seed;
public:
    Halton(const Properties &properties) : Sampler(properties) {
    }

    void seed(int sampleIndex) override {
        m_seed = sampleIndex;
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        m_seed = sampleIndex;
    }

    float nextPrime() {
        m_primeCount += 1;
        if (m_primeCount >= m_maxPrimeCount) {
            m_lastPrimeNum = 2;
            m_primeCount = 0;
        }
        return nextPrimeValue(m_lastPrimeNum);
    }

    float next() override {
        return RadicalInverse(nextPrime(), m_seed);
    }

    Point2 next2D() override {
        return {RadicalInverse(nextPrime(), m_seed), RadicalInverse(nextPrime(), m_seed)};
    }

    ref<Sampler> clone() const override {
        return std::make_shared<Halton>(*this);
    }

    std::string toString() const override {
        return tfm::format(
            "Halton[\n"
            "  count = %d\n"
            "]",
            m_samplesPerPixel
        );
    }

private:
    inline float RadicalInverse(int base, uint64_t a) const {
        float invBase = (float)1 / (float)base, invBaseM = 1;
        uint64_t reversedDigits = 0;
        while (a) {
            // Extract least significant digit from _a_ and update _reversedDigits_
            uint64_t next = a / base;
            uint64_t digit = a - next * base;
            reversedDigits = reversedDigits * base + digit;
            invBaseM *= invBase;
            a = next;
        }
        return std::min(reversedDigits * invBaseM, 0.99999999f);    
    }

    int nextPrimeValue(int currentPrime){
        int newPrime = currentPrime;
        bool isPrime = true;
        while(true){
            newPrime += 1;
            isPrime = true;
            for(int i=2; i<newPrime; i++){
                if ((newPrime % i) == 0){
                    isPrime = false;
                    break;
                }
            }
            if (isPrime) {
                break;
            }
        }
        return newPrime;
    }
};

}

REGISTER_SAMPLER(Halton, "halton")
