#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,Sampler &rng) const override {
        // sample the incident dir(remind in ray tracing we are beginning from the 
        // camera, we need the incident ray dir to find light source)
        // one thing have to remind that wo and frame.normal can be in different 
        // hemisphere, remember the inside and outside test in Ex1, the color of 
        // them are different bcs of the different normal(though they are the same surface)
        Vector wi= squareToCosineHemisphere(uv).normalized();
        float theta_i = Frame::cosTheta(wi);
        // float theta_o = Frame::cosTheta(wo);
        Color albedo = m_albedo->evaluate(uv);


        // this will somehow be equal to one, which means no dependency of the incident ray dir, not sure if this is correct
        float weight = InvPi*theta_i/cosineHemispherePdf(wi);
        
        // not sure if this is necessary, to make the outgoing and incident ray in the same hemisphere
        // following https://pbr-book.org/4ed/Reflection_Models/Diffuse_Reflection#
        if(wo[2]<0) {
            wi[2]*=-1;
        }
        // may have to doublecheck with tutor about when to return invalid
        assert(theta_i>=0);
        if (theta_i==0) 
            return BsdfSample{
                .wi = Vector(0),
                .weight = Color(0),
        };
        
        return BsdfSample{
            .wi = wi,
            .weight = albedo*weight        
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
