#include <lightwave.hpp>
#include "tools.hpp"

namespace lightwave {

class ToonIntegrator : public SamplingIntegrator {

public:
    Color color1 = {0.033, 0.217, 0.739};
    Color color2 = {0.033, 0.739, 0.217};
    Color color3 = {0.739, 0.217, 0.033};
    
    ToonIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        
    }

    /// @brief One bouce integrator
    Color Li(const Ray &ray, Sampler &rng) override {

        Color light = Color(0.f);
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            
            // next event estimation for light       
            if(m_scene->hasLights()) {
                LightSample light_sample = m_scene->sampleLight(rng);

                if(!light_sample.light->canBeIntersected()) { 
                    DirectLightSample direct_light_sample = light_sample.light->sampleDirect(its.position, rng);
                    bool light_its                        = m_scene->intersect( Ray(its.position, direct_light_sample.wi).normalized(),direct_light_sample.distance,rng);

                    if(!light_its)
                    {
                        BsdfEval bsdf_eval                = its.evaluateBsdf(direct_light_sample.wi);
                        light                            += direct_light_sample.weight * bsdf_eval.value / light_sample.probability;
                    }
                }
            }

            //if the hitted obj emits light, assign its color to the pixel
            if(its.instance->emission()) {
                light += its.evaluateEmission();
            } else {
                auto sample_result           = its.sampleBsdf(rng);
                Color       weight           = sample_result.weight;
                if (weight==Color(0)) {

                } else {
                    Ray secondary_Ray            = Ray(its.position,sample_result.wi).normalized();
                    Intersection second_its      = m_scene->intersect(secondary_Ray,rng);

                    if (second_its) {
                        light += weight * second_its.evaluateEmission();//evaluateEmission would return BLACK if no m_emission                
                    
                    } else {
                        light += weight * m_scene->evaluateBackground(secondary_Ray.direction).value;                
                    }   
                }       
            }
        }
        // escape the scene
        else {
            light = m_scene->evaluateBackground(ray.direction).value; 
        }

        // Color newLight = light.r() * color1 + light.g() * color2 + light.b() * color3;
        Color newLight = lightwave::colorFromIntensity(light);
        Color out = newLight;
        // if (newLight != Color::black())
        //     out = lightwave::darken(newLight, 0.98f, Color(0.1f));
        return out;
    }


    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "ToonIntegrator[\n"
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
REGISTER_INTEGRATOR(ToonIntegrator, "toon")
