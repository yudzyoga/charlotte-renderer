#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight : public Light {


public:
    DirectionalLight(const Properties &properties) {
        m_direction = properties.get<Vector>("direction");
        m_intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        
        
        return DirectLightSample{
            .wi = m_direction.normalized(),
            .weight = m_intensity,
            .distance = INFINITY
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("PointLight[\n"
                           "]");
    }

private:
    Vector m_direction;
    Color m_intensity;

};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")
