#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {

public:
    DirectIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        
    }

    /**
     * @brief 
     * 
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // step1: check whether the primary ray has an intersection with objects
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            // step2: if yes, generate a secondary ray and check whether it has an intersection with objects
            if(its.instance->emission()) return its.evaluateEmission();//return black if it has no emission property
            
            auto sample_result = its.sampleBsdf(rng);
            Ray secondaryRay = Ray(its.position,sample_result.wi).normalized();
            Intersection secondIts = m_scene->intersect(secondaryRay,rng);
            Color weight = sample_result.weight;

            if(secondIts) {
                if (secondIts.instance->emission()) return weight*secondIts.evaluateEmission();
                else return Color(0.f);
            }   
            else // hitted once, then escape the scene
                return weight*m_scene->evaluateBackground(secondaryRay.direction).value;
        }
        // escape the scene
        else return m_scene->evaluateBackground(ray.direction).value; 
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "DirectIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image)
        );
    }
};

}

// this informs lightwave to use our class NormalIntegrator whenever a <integrator type="normals" /> is found in a scene file
REGISTER_INTEGRATOR(DirectIntegrator, "direct")
