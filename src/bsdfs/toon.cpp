#include <lightwave.hpp>

namespace lightwave {

class Toon : public Bsdf {
    ref<Texture> m_albedo;

public:
    Toon(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    float ramp(float v0, float v1, float v, float clampLow=0.051f, float clampHigh=1.f) const {
        if (v < v0) {
            return clampLow;
        } else if (v >= v1) {
            return clampHigh;
        } else {
            float r = (v - v0) / (v1 - v0);
            return clampLow + r * (clampHigh - clampLow);
        }
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // compute how strongly the current hit reflects the light source
        if(Frame::cosTheta(wi) <= 0.f) return BsdfEval::invalid();
        Color out = m_albedo->evaluate(uv) * InvPi * Frame::cosTheta(wi);

        out.r() = ramp(0.1f, 0.12f, out.r());
        out.g() = ramp(0.1f, 0.2f, out.g());
        out.b() = ramp(0.05f, 0.3f, out.b());

        return BsdfEval{
            .value = out
            };
          
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        
        Vector wi= squareToCosineHemisphere(rng.next2D()).normalized();
        
        if(Frame::cosTheta(wi) <= 0.f) 
            return BsdfSample::invalid();
        return BsdfSample{
            .wi     = wi,
            .weight = m_albedo->evaluate(uv),
        };
    }
    
    Color albedo(const Point2 &uv) const override {
        return Color::black();
    }

    std::string toString() const override {
        return tfm::format("Toon[\n"
                           "  albedo = %s\n"
                           "]",
                           indent(m_albedo));
    }
};

} // namespace lightwave

// REGISTER_BSDF(Toon, "toon")
