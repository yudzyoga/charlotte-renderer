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

        // hints:
        // * precompute any expensive operations here (most importantly trigonometric functions)
        // * use m_resolution to find the aspect ratio of the image

        // get the resolution in x and y, and axisName to map the FOV in X and Y
        res_x = (float)m_resolution.x();
        res_y = (float)m_resolution.y();
        
        // define aspect ration and fov in x and y
        if (properties.get<std::string>("fovAxis") == "x") {
            aspect_ratio = res_x / res_y;
            fov_x = Deg2Rad * properties.get<float>("fov");
            fov_y = 2 * atan(tan(fov_x / 2) / aspect_ratio);
        }
        else {
            aspect_ratio = res_y / res_x;
            fov_y = Deg2Rad * properties.get<float>("fov");
            fov_x = 2 * atan(tan(fov_y / 2) / aspect_ratio);
        }

        // calculate focal length in x and y
        focal_length_x = (0.5f * res_x) / (tan(fov_x * 0.5f));
        focal_length_y = (0.5f * res_y) / (tan(fov_y * 0.5f));
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
        pt_screen_x = ((0.5f * (normalized.x() + 1.0f)) * res_x);
        pt_screen_y = ((0.5f * (normalized.y() + 1.0f)) * res_y);

        // define inverse matrix for the internal camera matrix configuration
        Matrix3x3 m_internal_inv = Matrix3x3::identity();
        m_internal_inv.setRow(0, lightwave::Vector(1/focal_length_x, 0, -0.5f*res_x/focal_length_x));
        m_internal_inv.setRow(1, lightwave::Vector(0, 1/focal_length_y, -0.5f*res_y/focal_length_y));   
        
        // Matrix4x4 smth = lightwave::invert(m_internal_inv);
        lightwave::Vector ray_direction_local = m_internal_inv * lightwave::Vector(pt_screen_x, pt_screen_y, 1);
        float unit_vec_denom = ray_direction_local.length();
        Ray localRay = Ray(Vector(0.f, 0.f, 0.f), ray_direction_local / unit_vec_denom);

        /* 
        Step 2:
            transform ray to the world coordinate system
        */ 
        Ray globalRay = m_transform -> apply(localRay);

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
    float res_x, res_y;
    float fov_x, fov_y, focal_length_x, focal_length_y, aspect_ratio;
};

}

REGISTER_CAMERA(Perspective, "perspective")
