#include <lightwave.hpp>

namespace lightwave {

class PathTracerIntegrator : public SamplingIntegrator {
    int m_depth;
    bool nee;

public:
    PathTracerIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        m_depth = properties.get<int>("depth", 2);
        nee     = properties.get<bool>("nee", true);
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    
    Color Li(const Ray &ray, Sampler &rng) override {
        assert(m_depth >= 2);
        if(!m_scene->hasLights()) nee = false;
        /*
        incremental update:
        prev_le <- prev_le + prev_weight * new_le
        prev_weight <- prev_weight * new_weight
        */
        
        Color prev_weight = Color(1.f);
        Color prev_le     = Color(0.f);
        Color new_li      = Color(0.f);
        Ray   bounceRay   = ray; //with depth ==0

        while(bounceRay.depth < m_depth){
            // update ray and its depth
            // every event check light(nne)            
            Intersection its = m_scene->intersect(bounceRay, rng);
            if(!its) {
                // only break when escape the scene
                new_li = m_scene->evaluateBackground(bounceRay.direction).value;
                break;
            } 
            else{
                Color new_le      = Color(0.f);
                Color new_weight  = Color(1.f);

                if(nee && bounceRay.depth < m_depth-1){
                    // nne is recognized as a new bounce, for last bounce, nne is not considered
                    LightSample light_sample = m_scene->sampleLight(rng);

                    if(!light_sample.light->canBeIntersected()) { 
                        DirectLightSample direct_light_sample = light_sample.light->sampleDirect(its.position, rng);
                        bool              light_its           = m_scene->intersect( Ray(its.position, direct_light_sample.wi).normalized(),direct_light_sample.distance,rng);

                        if(!light_its)
                        {
                            BsdfEval bsdf_eval                = its.evaluateBsdf(direct_light_sample.wi);
                                     new_le                  += direct_light_sample.weight * bsdf_eval.value / light_sample.probability;
                        }
                    }
                }

                if(its.instance->emission()) new_le += its.evaluateEmission(); //continue tracing
                
                //sample next ray direction
                auto sample_result = its.sampleBsdf(rng);
                new_weight         = sample_result.weight;

                //incremental update
                prev_le           += (prev_weight * new_le);
                prev_weight       *= new_weight;   

                if(bounceRay.depth >= m_depth-1) break; //no need to update bounceRay, shortcut

                //update bounceRay for next iteration, depth +1
                bounceRay = Ray(its.position, sample_result.wi, bounceRay.depth+1).normalized();             
            }
        }
        //initial values: prev_le=0, prev_weight=1, new_li=0
        return prev_le + prev_weight * new_li;
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "PathTracerIntegrator[\n"
            "  depth = %s,\n"
            "]",
            indent(m_depth)
        );
    }
};

}

// this informs lightwave to use our class NormalIntegrator whenever a <integrator type="normals" /> is found in a scene file
REGISTER_INTEGRATOR(PathTracerIntegrator, "pathtracer")
