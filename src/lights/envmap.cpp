#include <lightwave.hpp>

namespace lightwave {

class EnvironmentMap final : public BackgroundLight {
    /// @brief The texture to use as background
    ref<Texture> m_texture;

    /// @brief An optional transform from local-to-world space
    ref<Transform> m_transform;

public:
    EnvironmentMap(const Properties &properties) {
        m_texture   = properties.getChild<Texture>();
        m_transform = properties.getOptionalChild<Transform>();
    }

    BackgroundLightEval evaluate(const Vector &direction) const override {
        // Vector2 warped = ;
        // hints:
        // * if (m_transform) { transform direction vector from world to local
        // coordinates }
        // * find the corresponding pixel coordinate for the given local
        if (!m_transform) {
            return{
                .value = m_texture->evaluate(Vector2(0, 0))
            };
        }
        else {
            // world to local
            Vector localDir = m_transform->inverse(direction).normalized();

            // 3D dir to spherical coord(theta, phi)
            float phi = atan2(localDir.z(), localDir.x()); //already take care of x=0, returns the arccosine of z/x in the range -pi to pi radians
            // assert(localDir.y() >= -1.f); //acos will be indefite, for debug
            // assert(localDir.y() <= 1.f);   //acos will be indefite, for debug
            float theta = acos(localDir.y()); //returns the arccosine of x in the range 0 to pi radians
            
            // spherical coord (theta, phi) to 2D coord
            return {
            .value = m_texture->evaluate(Vector2(0.5f-Inv2Pi * phi,  InvPi * theta))
            };
        }

    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Vector direction = squareToUniformSphere(rng.next2D());
        auto E           = evaluate(direction);

        // implement better importance sampling here, if you ever need it
        // (useful for environment maps with bright tiny light sources, like the
        // sun for example)

        return {
            .wi     = direction,
            .weight = E.value / Inv4Pi,
            .distance = Infinity,
        };
    }

    std::string toString() const override {
        return tfm::format("EnvironmentMap[\n"
                           "  texture = %s,\n"
                           "  transform = %s\n"
                           "]",
                           indent(m_texture), indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_LIGHT(EnvironmentMap, "envmap")
