#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // ior = in/ext
        // if (wo.z()==0.f) return BsdfSample::invalid();
        float ior = m_ior->scalar(uv);
        float eta = wo.z() < 0 ? ior : 1.f/ior;
        float Fr = fresnelDielectric(wo.z(), eta);
        
        assert(Fr>=0.f && Fr<=1.f);

        if (rng.next() < Fr) {
            // reflect
            return BsdfSample{
                .wi = reflect(wo, Vector(0, 0, 1)).normalized(),
                .weight = m_reflectance->evaluate(uv) / Fr
            };
        } else {
            // refract
            float cosThetaI = wo.z();
            float cosThetaT = sqrt(1 - sqr(eta) * (1 - sqr(cosThetaI)));
            return BsdfSample{
                .wi = Vector(-eta * wo.x(), -eta * wo.y(), cosThetaT).normalized(),
                .weight = m_transmittance->evaluate(uv) * sqr(eta) /(1.f-Fr) 
            };
        }
        // return BsdfSample{
        //     .wi = reflect(wo, Vector(0, 0, 1)).normalized(),
        //     .weight = m_reflectance->evaluate(uv) ,
        // };
        
    }

    std::string toString() const override {
        return tfm::format("Dielectric[\n"
                           "  ior           = %s,\n"
                           "  reflectance   = %s,\n"
                           "  transmittance = %s\n"
                           "]",
                           indent(m_ior), indent(m_reflectance),
                           indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")
