#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        // NOT_IMPLEMENTED

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
        if(Frame::cosTheta(wi) <= 0.f) return BsdfEval::invalid();

        return BsdfEval{
            .value = color * InvPi * Frame::cosTheta(wi),
            };
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`

        Vector wi = squareToCosineHemisphere(rng.next2D()).normalized();

        if(Frame::cosTheta(wi) <= 0.f) 
            return BsdfSample::invalid();
        return BsdfSample{
            .wi     = wi,
            .weight = color,
        };
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
        float cos_theta_o = abs(Frame::cosTheta(wo));
        if(cos_theta_o == 0.f) return BsdfEval::invalid();

        Vector wh = (wo + wi) / (wo + wi).length();
        float D = lightwave::microfacet::evaluateGGX(alpha, wh);

        float G_wi = lightwave::microfacet::smithG1(alpha, wh, wi);
        float G_wo = lightwave::microfacet::smithG1(alpha, wh, wo);
        
        // float cos_theta_i = abs(Frame::cosTheta(wi));
        
        // if(cos_theta_o == 0.f) return {.value=Color(0.f)};
        // assert(cos_theta_o != 0.f);
        
        // Color value = ((color*D*G_wi*G_wo)/(4.f*cos_theta_o*cos_theta_i))*Frame::cosTheta(wi);
        // Color value = (color*D*G_wi*G_wo)/(4.f*cos_theta_o);
        // value.r() = std::max(value.r(), 0.f);
        // value.g() = std::max(value.g(), 0.f);
        // value.b() = std::max(value.b(), 0.f);
        // return {.value=((color*D*G_wi*G_wo)/(4.f*cos_theta_o*cos_theta_i))*Frame::cosTheta(wi)};
        // return {.value=value};
        return {.value=(color*D*G_wi*G_wo)/(4.f*cos_theta_o)};
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you

        // sample microfacet normal
        Vector normal = lightwave::microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        
        // compute wi, can easily be computed by reflecting the ray around the sampled normal
        Vector wi = lightwave::reflect(wo, normal);

        // microfacet normal pdf
        float pdf = lightwave::microfacet::pdfGGXVNDF(alpha, normal, wo);
        pdf *= lightwave::microfacet::detReflection(normal, wo);
        if (!(pdf>0)) return BsdfSample::invalid();

        // compute jacobian term
        float G_wi = lightwave::microfacet::smithG1(alpha, normal, wi);
        
        Color weight = color * G_wi;
        return BsdfSample{
            .wi=wi,
            .weight=weight
        };
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;
    bool isorm;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = !isorm? m_specular->scalar(uv) : m_specular->scalar_g(uv);
        const auto metallic = !isorm? m_metallic->scalar(uv) : m_metallic->scalar_b(uv);


        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        isorm = properties.get<bool>("isorm", false);
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        const auto combination = combine(uv, wo);
        auto comb_diff = combination.diffuse.evaluate(wo, wi);
        auto comb_mett = combination.metallic.evaluate(wo, wi);
        // assert(std::isnan(comb_diff.value.r()) == false);
        // assert(std::isnan(comb_mett.value.r()) == false);
        // assert(comb_diff.value.r() >= 0.f);
        // assert(comb_mett.value.r() >= 0.f);
        return {
            .value = comb_diff.value + comb_mett.value
        };
        // hint: evaluate `combination.diffuse` and `combination.metallic` and
        // combine their results
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto combination = combine(uv, wo);
        // hint: sample either `combination.diffuse` (probability
        // `combination.diffuseSelectionProb`) or `combination.metallic`

        bool sampleDiffuse = rng.next() < combination.diffuseSelectionProb;

        BsdfSample sample = sampleDiffuse ? 
                                    combination.diffuse.sample(wo, rng) : 
                                    combination.metallic.sample(wo, rng);
        float probSample = sampleDiffuse ? combination.diffuseSelectionProb : (1.f-combination.diffuseSelectionProb);
        
        return {
            .wi=sample.wi,
            .weight=sample.weight/probSample
        };
    }
    
    Color albedo(const Point2 &uv) const override {
        // return m_baseColor->evaluate(uv);
        if(m_baseColor != nullptr){
            return m_baseColor->evaluate(uv);
        } else {
            return Color::black();
        }
    }


    std::string toString() const override {
        return tfm::format("Principled[\n"
                           "  baseColor = %s,\n"
                           "  roughness = %s,\n"
                           "  metallic  = %s,\n"
                           "  specular  = %s,\n"
                           "]",
                           indent(m_baseColor), indent(m_roughness),
                           indent(m_metallic), indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")
