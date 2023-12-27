#include <lightwave.hpp>
#include "pcg32.h"

namespace lightwave {

class Halton : public Sampler {
    int m_primeSeed;
    bool useScreenOffset = true;
    Point2 m_offsetPixel;
    pcg32 m_pcg;
    int m_pcgSeed = 1337;

public:
    Halton(const Properties &properties) : Sampler(properties) {
        m_pcgSeed = properties.get<int>("seed", 1337);
    }

    void seed(int sampleIndex) override {
        m_primeSeed = sampleIndex;
        m_pcg.seed(m_pcgSeed, sampleIndex);
        resetPrime();
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        m_primeSeed = sampleIndex;
        resetPrime();

        // seed it, seed it. i said SEED IT!!!
        uint64_t hash = (uint64_t(pixel.x()) << 32) ^ pixel.y();
        m_pcg.seed(m_pcgSeed, hash);
        m_offsetPixel = {m_pcg.nextFloat(), m_pcg.nextFloat()};        
    }

    float next() override {
        return RadicalInverse(nextPrime(), m_primeSeed);
    }

    Point2 next2D() override {
        if(useScreenOffset){
            float posX = next() + m_offsetPixel.x();
            float posY = next() + m_offsetPixel.y();            
            return {std::fmod(posX, 1.0f), std::fmod(posY, 1.0f)};
        }
        else {
            return {next(), next()};
        }
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
    float nextPrime() {
        // if exceed the counts
        if (m_primeCount + 1 > m_maxPrimeCount) {
            resetPrime();
        }
        return nextPrimeValue(m_lastPrimeNum);
    }

    void resetPrime(){
        m_lastPrimeNum = 1;
        m_primeCount = 0;
    }

    inline float RadicalInverse(int base, uint64_t a) const {
        float invBase = (float)1 / (float)base, invBaseM = 1;
        uint64_t reversedDigits = 0;
        while (a) {
            // Extract least significant digit from _a_ and update _reversedDigits_
            uint64_t next = int(a / base);
            uint64_t digit = int(a - (next * base));
            reversedDigits = reversedDigits * base + digit;
            invBaseM *= invBase;
            a = next;
        }
        return std::max(std::min(reversedDigits * invBaseM, OneMinusEpsilon), 0.000001f);
    }

    int nextPrimeValue(int lastPrime){
        int newPrime = lastPrime;
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
        m_primeCount += 1;
        m_lastPrimeNum = newPrime;
        return newPrime;
    }

    int m_primeCount = 0;
    int m_maxPrimeCount = 1000;
    int m_lastPrimeNum = 2;
    static constexpr float OneMinusEpsilon = 0x1.fffffep-1;

};

}

REGISTER_SAMPLER(Halton, "halton")
