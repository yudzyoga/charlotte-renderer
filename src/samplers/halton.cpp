#include <lightwave.hpp>

namespace lightwave {

class Halton : public Sampler {
    int m_seed;
    bool useScreenResolution = true;
    // Point2i m_screenResolution = {512, 512};
    Point2 m_seedPixel;
public:
    Halton(const Properties &properties) : Sampler(properties) {
        m_seed = properties.get<int>("seed", 1337);
    }

    void seed(int sampleIndex) override {
        m_seed = sampleIndex;
        resetPrime();
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        m_seed = sampleIndex;
        m_seedPixel = {(float)pixel.x() / (float)m_screenResolution.x(), (float)pixel.y() / (float)m_screenResolution.y()};
        resetPrime();
    }

    float next() override {
        return RadicalInverse(nextPrime(), m_seed);
    }

    Point2 next2D() override {
        if(useScreenResolution)
            return {std::fmod(next() + m_seedPixel.x(), 1.f), std::fmod(next() + m_seedPixel.y(), 1.f)};
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
        // m_init = true;
        m_lastPrimeNum = 1;
        m_primeCount = 0;
    }

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
        return std::min(reversedDigits * invBaseM, OneMinusEpsilon);    
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
