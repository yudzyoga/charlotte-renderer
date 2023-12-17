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
        
        if (wo.z()==0.f)  return BsdfSample::invalid();

        float  ior        = m_ior->scalar(uv);// ior = in/ext
        float  eta        = ior;
        Vector normal     = Vector(0.f, 0.f, 1.f);

        if(wo.z()<0.f) { //int->ext     
               normal     = - normal;
               eta        = 1.f/ior;
        }
        //make sure normal and wo are in the same hemisphere
        assert(Frame::sameHemisphere(normal, wo));

        //
        Vector reflect_wi = reflect(wo, normal).normalized();
        // float  Fr         = fresnelDielectric(reflect_wi.z(),eta);
        

        //another way to calculate Fr, feel free to switch between two
        Vector refract_wi = refract(wo, normal, eta).normalized();
        float  cosThetaI  = abs(reflect_wi.z());
        float  cosThetaT  = abs(refract_wi.z());
        // does not matter using ior or 1/ior, the position of costhetaT and coshtetaI is mutable since we apply sqr later
        float Rs = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
        float Rp = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
        float Fr = (Rs * Rs + Rp * Rp) / 2;
        //end of another way to calculate Fr
        

        if (rng.next() < Fr) // reflect
                          return BsdfSample{
                                .wi       = reflect_wi,
                                .weight   = m_reflectance->evaluate(uv) 
                          };
            
        else // refract
                          return BsdfSample{
                                .wi       = refract(wo, normal, eta).normalized(),
                                .weight   = m_transmittance->evaluate(uv) * sqr(1/eta)  
                          };        
        
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
