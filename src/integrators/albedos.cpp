#include <lightwave.hpp>

namespace lightwave {

class AlbedoIntegrator : public SamplingIntegrator {
    bool m_remap;

public:
    AlbedoIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap", true);
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // take the direction of the ray
        Color albedo = Color::black();

        // check whether the ray has an intersection with objects
        if (m_scene->intersect(ray,INFINITY,rng)) {
            Intersection its = m_scene->intersect(ray, rng);
            if(its.instance->bsdf() != nullptr){
                albedo = its.instance->bsdf()->albedo(its.uv);
            }
        } 
        return albedo;
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "AlbedoIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image)
        );
    }
};

}

// this informs lightwave to use our class AlbedosIntegrator whenever a <integrator type="albedo" /> is found in a scene file
REGISTER_INTEGRATOR(AlbedoIntegrator, "albedo")
