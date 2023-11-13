#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf) const {
    // hints:
    // * transform the hitpoint and frame here
    // * if m_flipNormal is true, flip the direction of the bitangent (which in effect flips the normal)
    // * make sure that the frame is orthonormal (you are free to change the bitangent for this, but keep
    //   the direction of the transformed tangent the same)

    surf.position = m_transform->apply(surf.position);
    // surf.position = surf.frame.toWorld(surf.position);
    
    // auto hit_point = surf.position;
    // auto frame = surf.frame;
    
    if (m_flipNormal) {
        // surf.position;
        // surf.frame.bitangent = -surf.frame.bitangent;
        // surf.frame.normal = -surf.frame.normal;
    }
}

bool Instance::intersect(const Ray &worldRay, Intersection &its, Sampler &rng) const {
    if (!m_transform) {
        // fast path, if no transform is needed
        Ray localRay = worldRay;
        if (m_shape->intersect(localRay, its, rng)) {
            its.instance = this;
        }
        return false;
    }

    const float previousT = its.t;
    Ray localRay;

    // hints:
    // * transform the ray (do not forget to normalize!)
    // * how does its.t need to change?
    // map the ray from world to local ray, then normalize
    localRay = m_transform->inverse(worldRay).normalized();

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        // hint: how does its.t need to change?
        // if intersected, it means that all the initial intersected points (from sphere.cpp)
        // will be scaled down to some scale. without re-calculating the ray intersection, 
        // all the points can be re-mapped to its inverse thus produces the desired results.
        its.position = m_transform->inverse(its.position);
        Vector new_direction = Vector(its.position - m_shape->getCentroid()).normalized();

        // its.position = new_position;
        its.t = (its.position - localRay.origin).length();
        // its.wo = Vector(0); // this makes sphere_scaled correct hmmm ...
        // its.frame.normal = new_direction;

        // its.t = (new_position - localRay.origin).length();
        its.instance = this;
        transformFrame(its);
        return true;
    } else {
        its.t = previousT;
    }

    return false;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample);
    return sample;
}

}

REGISTER_CLASS(Instance, "instance", "default")
