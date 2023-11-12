#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {

    // inline void populate(SurfaceEvent &surf, const Point &position) const {
    //     surf.position = position;
        
    //     // map the position from [-1,-1,0]..[+1,+1,0] to [0,0]..[1,1] by discarding the z component and rescaling
    //     surf.uv.x() = (position.x() + 1) / 2;
    //     surf.uv.y() = (position.y() + 1) / 2;

    //     // the tangent always points in positive x direction
    //     surf.frame.tangent = Vector(1, 0, 0);
    //     // the bitagent always points in positive y direction
    //     surf.frame.bitangent = Vector(0, 1, 0);
    //     // and accordingly, the normal always points in the positive z direction
    //     surf.frame.normal = Vector(0, 0, 1);

    //     // since we sample the area uniformly, the pdf is given by 1/surfaceArea
    //     surf.pdf = 1.0f / 4;
    // }

public:
    Point center = Point(0);
    float radius = 1.0f;

    Sphere(const Properties &properties) {
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        float distance = its.t;
        Vector normal = ray.direction;

        // begin sphere calculation || O + t*D - C ||^2 = r^2
        Vector ray_origin_to_sphere_center = center - ray.origin; 
        float A = ray.direction.dot(ray.direction);
        float B = ray.direction.dot(ray_origin_to_sphere_center);
        float C = ray_origin_to_sphere_center.dot(ray_origin_to_sphere_center) - (radius * radius);

        // find D of linear equation
        float D = B*B-A*C;

        // if D<0, there are no interaction
        if (D<0){
            its.wo = normal;
            return false;
        }
        
        // find distance1 and distance2
        D = sqrt(D);
        float t1 = (B - D) / A;
        float t2 = (B + D) / A;

        // if t1>=0
        // t1 obviously less than t2 due to the substraction
        if (t1 >= 0){
            distance = t1;
        } else if (t2 >= 0){
            distance = t2;
        }

        if (distance < Epsilon || distance > its.t)
            return false;

        // calculate normal, and store variables
        normal = (ray(distance) - center).normalized();
        its.t = distance;
        its.wo = normal;
        its.pdf = 0.f;
        // populate(its, ray(distance));
        return true;
    }
    Bounds getBoundingBox() const override {
        return Bounds(center - Vector(radius), center + Vector(radius));
    }
    Point getCentroid() const override {
        return center;
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