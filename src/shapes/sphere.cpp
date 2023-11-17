#include <lightwave.hpp>

namespace lightwave {

class Sphere : public Shape {

public:
    Point center = Point(0);
    float radius = 1.0f;

    Sphere(const Properties &properties) {
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        

        // begin sphere calculation || O + t*D - C ||^2 = r^2
        Vector ray_origin_to_sphere_center = center - ray.origin; 
        float A = ray.direction.dot(ray.direction);
        float B = ray.direction.dot(ray_origin_to_sphere_center);
        float C = ray_origin_to_sphere_center.dot(ray_origin_to_sphere_center) - (radius * radius);

        // find D of linear equation
        float D = B*B-A*C;

        // if D<0, there are no interaction
        if (D<0){
            return false;
        }
        
        // find distance1 and distance2
        D = sqrt(D);
        float t1 = (B - D) / A;
        float t2 = (B + D) / A;

        // if t1>=0
        // t1 obviously less than t2 due to the substraction
        auto distance = its.t;
        if (t1 >= 0){
            distance = t1;
        } else if (t2 >= 0){
            distance = t2;
        }

        if (distance < Epsilon || distance > its.t){
            return false;
        }

        // calculate normal, and store variables
        its.pdf = 0.f;
        its.t = distance;
        its.position = ray(distance);
        its.frame.normal = (its.position - center).normalized();
        its.frame = Frame(its.frame.normal);
        its.uv.x() = (its.position.x() + 1.0) / 2;
        its.uv.y() = (its.position.y() + 1.0) / 2;
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