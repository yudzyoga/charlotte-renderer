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
    Color Li(const Ray &ray, Sampler &rng) override {
        Color goal = Color(0.f);
        float weight = 1.f;

        Intersection its = m_scene->intersect(ray, rng);
        // for(){
        //     intersect
        //     if(!its){color += bg * weight; break}
        //     color += its.emission*weight
            
        //     if (depth > ... )break
        //     NEE * weight
        //     ray = sampleBDDF
            
            
            
        // }


        return Color(0.f);
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "PathTracerIntegrator"
            // "PathTracerIntegrator[\n"
            // "  sampler = %s,\n"
            // "  image = %s,\n"
            // "]",
            // indent(m_sampler),
            // indent(m_image)
        );
    }
};

}

// this informs lightwave to use our class NormalIntegrator whenever a <integrator type="normals" /> is found in a scene file
REGISTER_INTEGRATOR(PathTracerIntegrator, "pathtracer")
