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
        Color weight = Color(1.f);
        Color color ;
        // step1: check whether the primary ray has an intersection with objects
        if (m_scene->intersect(ray,INFINITY,rng)) {
            // step2: if yes, generate a secondary ray and check whether it has an intersection with objects
            Intersection its = m_scene->intersect(ray, rng);
            auto sample_result = its.sampleBsdf(rng);
            Ray secondaryRay = Ray(its.position,sample_result.wi).normalized();

            // step4: generate secondary ray to check whether it hits the light source
            if(m_scene->intersect(secondaryRay,INFINITY,rng)){
            // hidden by other objects， somehow I don't know why not return black(shadow), but then most of the model will be black them
            // step3: update the remain energy (basically the color)

                return sample_result.weight;
            }
            else {
            // not hidden by other objects, hit the background, return the darkened color
                weight = sample_result.weight;

                color = m_scene->evaluateBackground(secondaryRay.direction).value;

                // std::cout<<color<<std::endl;
                return weight*m_scene->evaluateBackground(ray.direction).value;
            }
        }
        // get lightened by envlight 
        // else return Color(1.f);
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
