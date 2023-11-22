#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 * 
 * In local coordinates (before applying m_transform), the camera looks in positive z direction [0,0,1].
 * Pixels on the left side of the image ( @code normalized.x < 0 @endcode ) are directed in negative x
 * direction ( @code ray.direction.x < 0 ), and pixels at the bottom of the image ( @code normalized.y < 0 @endcode )
 * are directed in negative y direction ( @code ray.direction.y < 0 ).
 */
class Perspective : public Camera {
public:
    Perspective(const Properties &properties)
    : Camera(properties) {        
        // NOT_IMPLEMENTED

        // define aspect ration and fov in x and y
        float fov = Deg2Rad * properties.get<float>("fov");
        if (properties.get<std::string>("fovAxis") == "x") {
            focal_length = (0.5f * m_resolution.x()) / (tan(fov * 0.5f));
        }
        else {
            focal_length = (0.5f * m_resolution.y()) / (tan(fov * 0.5f));
        }
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        // NOT_IMPLEMENTED

        // hints:
        // * use m_transform to transform the local camera coordinate system into the world coordinate system
        
        /*
        Step 1:
            transform normalized coordinates to the local camera coordinate system
            from normalized transform into pixel position
            to camera coordinates, given known focal point and principan point in 0, 0
            then transform to the world coordinate system in terms of vector
        */  
        float pt_screen_x, pt_screen_y;
        pt_screen_x = (0.5f * normalized.x() * m_resolution.x());
        pt_screen_y = (0.5f * normalized.y() * m_resolution.y());

        Vector ray_direction_local = Vector(pt_screen_x, pt_screen_y, focal_length).normalized();
        Ray localRay = Ray(Vector(0.f, 0.f, 0.f), ray_direction_local);

        /* 
        Step 2:
            transform ray to the world coordinate system
        */ 
        Ray globalRay = m_transform -> apply(localRay).normalized();

        return lightwave::CameraSample{.ray = globalRay, 
                                        .weight = Color(1.0f)};
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform)
        );
    }

private:
    // float res_x, res_y;
    // float fov_x, fov_y, focal_length_x, focal_length_y, aspect_ratio;
    float fov, focal_length;
};

}

REGISTER_CAMERA(Perspective, "perspective")
