#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {


public:
    AreaLight(const Properties &properties) {
        m_instance = properties.getChild<Instance>();
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        AreaSample sampleArea = m_instance->sampleArea(rng);
        Point lightPosition = sampleArea.position;
        Vector wi = lightPosition - origin;   //direction
        // Bounds box = m_instance->getBoundingBox();
        // Point boxLength = box.max() - box.min();
        // float area = abs(boxLength.x() * boxLength.y() * boxLength.z());

        Color intensity = m_instance->emission()->evaluate(sampleArea.uv, wi.normalized()).value;
        return DirectLightSample{
            .wi = wi.normalized(), //y
            .weight = intensity * Inv4Pi / wi.lengthSquared(),
            .distance = wi.length(), //y
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("AreaLight[\n"
                           "]");
    }

private:
    ref<Instance> m_instance;
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")
