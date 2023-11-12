#include <lightwave.hpp>

namespace lightwave {

class NormalsIntegrator : public SamplingIntegrator {
    bool m_remap;

public:
    NormalsIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap", true);
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced by the camera model.
     * This will be run for each pixel of the image, potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        // take the direction of the ray
        Vector d = ray.direction;

        // check whether the ray has an intersection with objects
        if (m_scene->intersect(ray, rng) == false) {
            d = Vector(0.f);
        } else {
            // if yes, then proceed to use the saved "normal" from "sphere.cpp" code
            Intersection its = m_scene->intersect(ray, rng);
            d = its.wo;
        }

        if (m_remap) {
            // remap the direction from [-1,+1]^3 to [0,+1]^3 so that colors channels are never negative
            d = (d + Vector(1)) / 2;
        }
        return Color(d);
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "NormalsIntegrator[\n"
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
REGISTER_INTEGRATOR(NormalsIntegrator, "normals")
