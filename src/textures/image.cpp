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

    Point2 remapPixel(const Point2 &uv, bool isClamp=true) const {
        Point2 edgeMargin {
            0.5f/m_image->resolution().x(),
            0.5f/m_image->resolution().y()
        };
        
        // assume clamp as default
        // make sure in range
        Point2 uvOut = uv;
        if (uv.x() < edgeMargin.x() || uv.x() > 1.f-edgeMargin.y() || 
            uv.y() < edgeMargin.y() || uv.y() > 1.f-edgeMargin.y()){

            // if clamp, make sure to take the edge pixel by pointing into the center position
            if (isClamp) {
                uvOut = Point2{
                    clamp(uv.x(), edgeMargin.x(), 1.f-edgeMargin.x()),
                    clamp(uv.y(), edgeMargin.y(), 1.f-edgeMargin.y())
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
        
        // nearest neighboor
        if (!isBilinearFiltering) {
            // reverse the y ratio
            return m_image->operator()(Point2{uv.x(), 1.f-uv.y()});
        } else {
            // bilinear method
            // not yet structured for easy access
            // -0.5f because of the origin in the top left
            Point2 uv_pixel {-0.5f + (float)(res.x() * uv.x()), -0.5f + (float)(res.y() * uv.y())};
            
            // Check for edge cases
            Point2i floorPt {(int)floor(uv_pixel.x()), (int)floor(uv_pixel.y())};
            Point2i ceilPt {(int)ceil(uv_pixel.x()), (int)ceil(uv_pixel.y())};
            if (floorPt.x() < 0) {floorPt.x() = floorPt.x() + res.x();}
            if (floorPt.y() < 0) {floorPt.y() = floorPt.y() + res.y();}
            if (ceilPt.x() > (res.x()-1)) {ceilPt.x() = ceilPt.x() - res.x();}
            if (ceilPt.y() > (res.y()-1)) {ceilPt.y() = ceilPt.y() - res.y();}

            // Find the colors of neighbooring pixels position, then extract colors for each position
            // also reverse the position(s) in y
            Color c00 = Color(m_image->get(Point2i{floorPt.x(), (res.y()-1)-floorPt.y()}));
            Color c10 = Color(m_image->get(Point2i{ceilPt.x(),  (res.y()-1)-floorPt.y()}));
            Color c01 = Color(m_image->get(Point2i{floorPt.x(), (res.y()-1)-ceilPt.y()}));
            Color c11 = Color(m_image->get(Point2i{ceilPt.x(),  (res.y()-1)-ceilPt.y()}));

            // interpolate the neighbooring pixels
            Color f_y1 = c00 * (1.f - (uv_pixel.x()-floor(uv_pixel.x()))) + c10 * ((uv_pixel.x()-floor(uv_pixel.x())) - 0.f);
            Color f_y2 = c01 * (1.f - (uv_pixel.x()-floor(uv_pixel.x()))) + c11 * ((uv_pixel.x()-floor(uv_pixel.x())) - 0.f);
            Color f_final = f_y1 * (1.f - (uv_pixel.y()-floor(uv_pixel.y()))) + f_y2 * ((uv_pixel.y()-floor(uv_pixel.y())) - 0.f);
            return f_final * m_exposure;
        }
    }

    Color evaluate(const Point2 &uv) const override {
        if(std::isnan(uv.x()) || std::isnan(uv.y())) {
            return Color(0);
        }
        // remap the uv
        Point2 uvRemapped = remapPixel(uv, m_border == BorderMode::Clamp);
        // get the pixel volor based on the interpolation
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
