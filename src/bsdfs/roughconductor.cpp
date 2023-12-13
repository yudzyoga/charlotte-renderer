#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        Color R = m_reflectance->evaluate(uv);
        Vector wh = (wo + wi) / (wo + wi).length();
        float D = lightwave::microfacet::evaluateGGX(alpha, wh);

        float G_wi = lightwave::microfacet::smithG1(alpha, wh, wi);
        float G_wo = lightwave::microfacet::smithG1(alpha, wh, wo);
        
        float cos_theta_i = wi.y() / sqrt(pow(wi.x(), 2) + pow(wi.z(), 2));
        float cos_theta_o = wo.y() / sqrt(pow(wo.x(), 2) + pow(wo.z(), 2));
        
        return {.value=R*D*G_wi*G_wo/(4*cos_theta_o*cos_theta_i)};

        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // sample microfacet normal
        Vector normal = lightwave::microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        
        // compute wi, can easily be computed by reflecting the ray around the sampled normal
        Vector wi = lightwave::reflect(wo, normal);

        // microfacet normal pdf
        float pdf = lightwave::microfacet::pdfGGXVNDF(alpha, normal, wo);

        // compute jacobian term
        float probInput = pdf * lightwave::microfacet::detReflection(normal, wo);
        // float theta = atan2(sqrt(pow(wi.x(), 2) + pow(wi.z(), 2)), wi.y());
        // The weight of the sample, given by cos(theta) * B(wi, wo) / p(wi)
        // Color weight = Color(cos(theta) * Frame(normal).bitangent / probInput);

        float G_wi = lightwave::microfacet::smithG1(alpha, normal, wi);

        Color weight = m_reflectance->evaluate(uv) * G_wi;

        // return {
        //     .wi     = wi,
        //     .weight = Color(probInput),
        // };


        // float weight = InvPi/cosineHemispherePdf(wi)*Frame::cosTheta(wi);//always equal to one
        return BsdfSample{
            .wi=wi,
            .weight=weight,
            .pdf=pdf
        };
        // hints:
        // * do not forget to cancel out as many terms from your equations as possible!
        //   (the resulting sample weight is only a product of two factors)
    }

    std::string toString() const override {
        return tfm::format("RoughConductor[\n"
                           "  reflectance = %s,\n"
                           "  roughness = %s\n"
                           "]",
                           indent(m_reflectance), indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")
