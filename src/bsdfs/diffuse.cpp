#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // compute how strongly the current hit reflects the light source
        // if(!Frame::sameHemisphere(wi,wo)) return BsdfEval{
        //     .value = Color(0.6f,0.3f,0.3f)

        //     // .value = Color(0.f)
        // }; 
        if(Frame::cosTheta(wi) <= 0.f) return BsdfEval::invalid();
       
        return BsdfEval{
            .value = m_albedo->evaluate(uv) * InvPi * Frame::cosTheta(wi)
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

    std::string toString() const override {
        return tfm::format("Diffuse[\n"
                           "  albedo = %s\n"
                           "]",
                           indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")
