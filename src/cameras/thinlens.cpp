#include <lightwave.hpp>
// #include "thinlens.hpp"

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 * 
 * In local coordinates (before applying m_transform), the camera looks in positive z direction [0,0,1].
 * Pixels on the left side of the image ( @code normalized.x < 0 @endcode ) are directed in negative x
 * direction ( @code ray.direction.x < 0 ), and pixels at the bottom of the image ( @code normalized.y < 0 @endcode )
 * are directed in negative y direction ( @code ray.direction.y < 0 ).
 */
class Thinlens : public Camera {
public:
    Thinlens(const Properties &properties)
    : Camera(properties) {        
        // NOT_IMPLEMENTED

        // define aspect ration and fov in x and y
        float fov = Deg2Rad * properties.get<float>("fov");
        bool is_x = properties.get<std::string>("fovAxis") == "x";
        focal_length = (is_x * m_resolution.x() + (1-is_x) * m_resolution.y()) * 0.5f / (tan(fov * 0.5f));
        lens_radius = properties.get<float>("lensRadius", 0.05f);
        focal_distance = properties.get<float>("focalDistance", 7.f);
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
        // float pt_screen_x, pt_screen_y;
        // pt_screen_x = (0.5f * normalized.x() * m_resolution.x());
        // pt_screen_y = (0.5f * normalized.y() * m_resolution.y());
        
        /*
        float focal_distance = 1.f;
        float lens_radius = 0.f;

        Point2 pointInSquare = {0.5f + 0.5f * normalized.x(), 0.5f + 0.5f * normalized.y()};
        Point2 disk_pt = squareToUniformDiskConcentric(pointInSquare);
        Point2 p_lens = {disk_pt.x() * lens_radius, disk_pt.y() * lens_radius};


        // float pt_screen_x = (0.5f * normalized.x() * m_resolution.x());
        // float pt_screen_y = (0.5f * normalized.y() * m_resolution.y());
        Vector ray_direction_local = Vector(normalized.x() * m_resolution.x() * 0.5f, normalized.y() * m_resolution.y() * 0.5f, focal_length).normalized();
        Ray localRay = Ray(Vector(0.f, 0.f, 0.f), ray_direction_local);

        float ft = focal_distance / localRay.direction.z();
        Point p_focus = localRay(ft);

        // Vector ray_direction_local = Vector(pt_screen_x, pt_screen_y, 500.f).normalized();
        localRay.origin = Point(p_lens.x(), p_lens.y(), 0.f);
        localRay.direction = (p_focus - localRay.origin).normalized();
        
        // {
        //     .origin = Point(p_lens.x() * lens_radius, p_lens.y() * lens_radius, 0.f),
        //     .direction = Vector(p_lens.x(), p_lens.y(), 1.f).normalized()
        // };

        //  
        Step 2:
            transform ray to the world coordinate system
        // 

        // sample point on lens
        // p_lens *= lens_radius;

        // compute point on plane of focus 

        // update ray
        // localRay.origin = Point(p_lens.x() * lens_radius, p_lens.y() * lens_radius, 0.f);
        // localRay.direction = (p_focus - localRay.origin).normalized();
        
        */


        // METHOD2
        // float lens_radius = 0.01f;
        // float focal_distance = 7.f;
        float pt_screen_x, pt_screen_y;
        
        pt_screen_x = (0.5f * normalized.x() * m_resolution.x()); // [-0.5res, 0.5res]
        pt_screen_y = (0.5f * normalized.y() * m_resolution.y()); // [-0.5res, 0.5res]

        Vector ray_direction_local = Vector(pt_screen_x, pt_screen_y, focal_length).normalized();
        Ray localRay = Ray(Point(0.f), ray_direction_local).normalized();

        // if lens radius exist, then it's thinlens
        if (lens_radius > 0.f) {
            // generate another random points
            Point2 pointInSquare = rng.next2D();
            Point2 disk_pt_norm = squareToUniformDiskConcentric(pointInSquare);
            Point p_lens = {(2.f * disk_pt_norm.x() - 1.0f) * lens_radius, (2.f * disk_pt_norm.y() - 1.0f) * lens_radius, 0.f};

            // Compute point on plane of focus
            float ft = focal_distance / localRay.direction.z();
            Point p_focus = localRay.direction * ft;

            // remap the local ray
            localRay.origin = p_lens;
            localRay.direction = (p_focus - p_lens).normalized();
            localRay = localRay.normalized();
        }

        // to global
        Ray globalRay = m_transform -> apply(localRay).normalized();

        return lightwave::CameraSample{.ray = globalRay, 
                                        .weight = Color(1.0f)};
    }

    std::string toString() const override {
        return tfm::format(
            "Thinlens[\n"
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
    float focal_length;
    float lens_radius, focal_distance;
    // float lens_radius = 1.f;
};

}

REGISTER_CAMERA(Thinlens, "thinlens")