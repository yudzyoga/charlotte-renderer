#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // the weigth would be canceled out to 1 if using cosineHemisphere
        Vector wi= squareToCosineHemisphere(rng.next2D()).normalized();
        float weight = InvPi/cosineHemispherePdf(wi)*Frame::cosTheta(wi);//always equal to one, will doublecheck with the tutor see if we can ignore
        return BsdfSample{
            .wi=wi,
            .weight = m_albedo->evaluate(uv)*weight
        };
    }

    std::string toString() const override {
        return tfm::format("Diffuse[\n"
                           "  albedo = %s\n"
                           "]",
                           indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")
