#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
    Color m_color0, m_color1;
    Vector2 m_scale;

public:
    CheckerboardTexture(const Properties &properties) {
        m_color0 = properties.get<Color>("color0");
        m_color1 = properties.get<Color>("color1");
        m_scale = properties.get<Vector2>("scale");
    }

    Color evaluate(const Point2 &uv) const override { 
        /* ----- slightly detailed explanation just for explanation -----
            int tileIdxX, tileIdxY;
            tileIdxX = floor(uv.x() * m_scale.x());
            tileIdxY = floor(uv.y() * m_scale.y());

            if (fmod(tileIdxX + tileIdxY, 2) == 0){
                return m_color0;
            } else {
                return m_color1;
            }            
        */

        if (fmod(floor(uv.x() * m_scale.x()) + 
                 floor(uv.y() * m_scale.y()), 2) == 0){
            return m_color0;
        } else {
            return m_color1;
        }       
    }

    std::string toString() const override {
        return tfm::format("CheckerboardTexture[\n"
                           "  color0 = %s\n"
                           "  color1 = %s\n"
                           "  scale = %s\n"
                           "]",
                           indent(m_color0), 
                           indent(m_color1),
                           indent(m_scale));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")
