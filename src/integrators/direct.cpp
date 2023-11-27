#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {

public:
    DirectIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        
    }

    /// @brief One bouce integrator
    Color Li(const Ray &ray, Sampler &rng) override {

        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            
            //if the hitted obj emits light, assign its color to the pixel
            if(its.instance->emission()) 
                return its.evaluateEmission();
            
            auto sample_result = its.sampleBsdf(rng);
            Ray secondaryRay = Ray(its.position,sample_result.wi).normalized();
            Intersection secondIts = m_scene->intersect(secondaryRay,rng);
            Color weight = sample_result.weight;

            if(secondIts) //hit twice
                return weight*secondIts.evaluateEmission();//evaluateEmission would return BLACK if no m_emission
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
