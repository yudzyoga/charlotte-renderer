#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {


public:
    PointLight(const Properties &properties) {
        m_position = properties.get<Point>("position");
        m_power = properties.get<Color>("power");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        
        // Color I = m_power*Inv4Pi;   //intensity
        Vector wi = m_position- origin;   //direction

        return DirectLightSample{
            .wi = wi.normalized(),
            .weight = m_power * Inv4Pi / wi.lengthSquared(),
            .distance = wi.length(),
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("PointLight[\n"
                           "]");
    }

private:
    Point m_position;
    Color m_power;

};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")
