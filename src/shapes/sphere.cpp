#include <lightwave.hpp>

namespace lightwave {

class Sphere : public Shape {
    /**
     * @brief Constructs a surface event for a given position, used by @ref intersect to populate the @ref Intersection
     * and by @ref sampleArea to populate the @ref AreaSample .
     * @param surf The surface event to populate with texture coordinates, shading frame and area pdf
     * @param position The hitpoint (i.e., point in [-1,-1,0] to [+1,+1,0]), found via intersection or area sampling
     */
    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;

        // use the sphere to uv equation through two implicit trigonometry parameter
        // theta = atan2(position.x(), position.z())
        // phi = acos(position.y())
        surf.uv.x() = atan2(position.z(), position.x()) / (2 * Pi);     // z, x
        surf.uv.y() = asin(position.y()) / Pi;                          // asin

        // since we sample the area uniformly, the pdf is given by 1/surfaceArea
        // surf.pdf = 1.0f / 4;
    }

public:
    Point center = Point(0);
    float radius = 1.0f;

    Sphere(const Properties &properties) {
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        static constexpr float Epsilon = 1e-4f;
        // Begin sphere calculation. 
        // the sphere equation are given by || O + t*D - C ||^2 = r^2
        // O -> origin of the ray
        // t -> ray distance
        // C -> center of the sphere
        // r -> radius of the sphere
        Vector ray_origin_to_sphere_center = center - ray.origin; 
        // if (abs(ray_origin_to_sphere_center.length()-1)<Epsilon) return false;

        float A = ray.direction.dot(ray.direction);
        float B = ray.direction.dot(ray_origin_to_sphere_center);
        float C = ray_origin_to_sphere_center.dot(ray_origin_to_sphere_center) - (radius * radius);

        // find D of linear equation
        float D = B*B-A*C;

        // if D<0, there are no interaction
        // if D=0, there are only one intersection
        // if D>0, there are two intersections
        if (D<0){
            return false;
        }
        
        // find distance1 and distance2
        D = sqrt(D);
        float t1 = (B - D) / A;
        float t2 = (B + D) / A;

        // If t1>=0
        // t1 obviously less than t2 due to the substraction
        // if not, take t2 if its bigger than 0
        float distance ;
        if (t1 > Epsilon){
            distance = t1;
        } else if (t2 > Epsilon){
            distance = t2;
        }
        else return false;

        // Skip if less than predefined Epsilon, and measure whether
        // the distance is smaller than the previous ray distance (its.t)
        // it means to take closer intersection of the ray to the sphere.
        // !!! mind that in the secondary intersection checking, pre and current distance would be inf
        if (distance < Epsilon || distance > its.t || distance==Infinity){
            return false;
        }

        // Calculate normal, and store variables
        its.pdf = 0.f;
        its.t = distance;
        its.position = ray(distance);

        its.frame.normal = (its.position - center).normalized();
        its.frame = Frame(its.frame.normal);
        populate(its, its.position);

        return true;
    }
    Bounds getBoundingBox() const override {
        return Bounds(center - Vector(radius), center + Vector(radius));
    }
    Point getCentroid() const override {
        return Point(0);
    }
    AreaSample sampleArea(Sampler &rng) const override {
        NOT_IMPLEMENTED
    }
    std::string toString() const override {
        return "Sphere[]";
    }
};

}

REGISTER_SHAPE(Sphere, "sphere")