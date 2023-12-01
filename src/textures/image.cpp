#include <lightwave.hpp>

namespace lightwave {

class ImageTexture : public Texture {
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;

public:
    ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        m_border =
            properties.getEnum<BorderMode>("border", BorderMode::Repeat,
                                           {
                                               { "clamp", BorderMode::Clamp },
                                               { "repeat", BorderMode::Repeat },
                                           });

        m_filter = properties.getEnum<FilterMode>(
            "filter", FilterMode::Bilinear,
            {
                { "nearest", FilterMode::Nearest },
                { "bilinear", FilterMode::Bilinear },
            });
    }

    Point2 remapPixel(Point2 &uv, bool isClamp=true) const {
        // assume clamp as default
        // make sure in range
        Point2 uvOut = uv;
        if (uv.x() < 0.f || uv.x() > 1.f || 
            uv.y() < 0.f || uv.y() > 1.f){

            // if clamp
            if (isClamp) {
                uvOut = Point2{
                    clamp(uv.x(), 0.f, 1.f),
                    clamp(uv.y(), 0.f, 1.f)
                };
            } else {
                uvOut = Point2(uv.x() - floor(uv.x()), 
                               uv.y() - floor(uv.y()));
            }
        } 
        return uvOut;
    }

    Color interpolateColor(Point2 &uv, bool isBilinearFiltering) const {
        Point2i res = m_image->resolution();
        
        if (!isBilinearFiltering) {
            return Color(m_image->get(Point2i{
                        (int)floor(res.x() * uv.x()),
                        (int)floor(res.y() * uv.y())
                    }));
        } else {
            // not yet structured for easy access
            // -0.5f because of the origin in the top left
            // Point2 p {-0.5f + (float)(res.x()) * uv.x(), -0.5f + (float)(res.y()) * uv.y()};
            Point2 p {-0.5f + (float)(res.x() * uv.x()), -0.5f + (float)(res.y() * uv.y())};

            // check edge case !!!
            Point2i floorPt {(int)floor(p.x()), (int)floor(p.y())};
            Point2i ceilPt {(int)ceil(p.x()), (int)ceil(p.y())};
            if (floorPt.x() < 0) {floorPt.x() = floorPt.x() + res.x();}
            if (floorPt.y() < 0) {floorPt.y() = floorPt.y() + res.y();}
            if (ceilPt.x() > (res.x()-1)) {ceilPt.x() = ceilPt.x() - res.x();}
            if (ceilPt.y() > (res.y()-1)) {ceilPt.y() = ceilPt.y() - res.y();}

            // assign points
            Point2i p00 {floorPt.x(), floorPt.y()};
            Point2i p10 {ceilPt.x(), floorPt.y()};
            Point2i p01 {floorPt.x(), ceilPt.y()};
            Point2i p11 {ceilPt.x(), ceilPt.y()};

            Color c00 = Color(m_image->get(p00));
            Color c10 = Color(m_image->get(p10));
            Color c01 = Color(m_image->get(p01));
            Color c11 = Color(m_image->get(p11));

            Color f_y1 = c00 * (p10.x() - p.x()) + c10 * (p.x() - p00.x());
            Color f_y2 = c01 * (p11.x() - p.x()) + c11 * (p.x() - p01.x());
            Color f_final = f_y1 * (ceilPt.y() - p.y()) + f_y2 * (p.y() - floorPt.y());
            
            return f_final;
        }
    }

    Color evaluate(const Point2 &uv) const override {
        Point2 centerPt{0.5f};
        const float imgRatioX = 0.5f;
        const float imgRatioY = 0.5f;

        // get min and max points   
        Point2 minUV {centerPt.x() - 0.5f * imgRatioX, 
                      centerPt.y() - 0.5f * imgRatioY};
        Point2 maxUV {centerPt.x() + 0.5f * imgRatioX, 
                      centerPt.y() + 0.5f * imgRatioY};
        
        float xPosRatio = ((uv.x() - minUV.x()) / imgRatioX);
        float yPosRatio = ((uv.y() - minUV.y()) / imgRatioY);
        Point2 uvRatio = Point2{xPosRatio, yPosRatio};

        Point2 uvRemapped = remapPixel(uvRatio, m_border == BorderMode::Clamp);
        Color pixelColor = interpolateColor(uvRemapped, m_filter == FilterMode::Bilinear);    

        return pixelColor;
    }

    std::string toString() const override {
        return tfm::format("ImageTexture[\n"
                           "  image = %s,\n"
                           "  exposure = %f,\n"
                           "]",
                           indent(m_image), m_exposure);
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")
