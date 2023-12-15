#include <lightwave.hpp>

namespace lightwave {

class PathTracerIntegrator : public SamplingIntegrator {
    int m_depth;

public:
    PathTracerIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        m_depth = properties.get<int>("depth");
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    

    Color RecurseRay(const Ray &ray, Sampler &rng){
        Color light = Color(0.f);
        // if (ray.depth >= m_depth) return m_scene->evaluateBackground(ray.direction).value; 
        if (ray.depth >= m_depth) return Color(0.f);  //0.00215

        bool isIntersected = m_scene->intersect(ray, INFINITY, rng);
        if (!isIntersected) {
            return m_scene->evaluateBackground(ray.direction).value; 
        } else {
            Intersection its = m_scene->intersect(ray, rng);
            if(m_scene->hasLights()) {
                LightSample light_sample = m_scene->sampleLight(rng);

                if(!light_sample.light->canBeIntersected()) { 
                    DirectLightSample direct_light_sample = light_sample.light->sampleDirect(its.position, rng);
                    bool light_its                        = m_scene->intersect( Ray(its.position, direct_light_sample.wi).normalized(),direct_light_sample.distance,rng);

                    if(!light_its)
                    {
                        BsdfEval bsdf_eval = its.evaluateBsdf(direct_light_sample.wi);
                        light += direct_light_sample.weight * bsdf_eval.value / light_sample.probability;
                    }
                }
            }

            //if the hitted obj emits light, assign its color to the pixel
            if(its.instance->emission()) return light + its.evaluateEmission();

            auto sample_result = its.sampleBsdf(rng);
            Color weight = sample_result.weight;
            if(weight==Color(0)) return Color(0.f);

            Ray nextRay = Ray(its.position, sample_result.wi, ray.depth+1).normalized();
            
            // do repeat the ray recursively
            Color nextColor = RecurseRay(nextRay, rng);
            return light + weight * nextColor;
        }
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Color outLight = RecurseRay(ray, rng);
        return outLight;
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
