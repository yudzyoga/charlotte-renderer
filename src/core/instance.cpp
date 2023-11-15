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
    
    // re-transform the normal
    surf.frame.normal = m_transform->inverse(surf.frame.normal).normalized();

    if (m_flipNormal) {
        surf.frame.bitangent = -surf.frame.bitangent;
        surf.frame.normal = -surf.frame.normal;
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

    // Transform the ray from world to local
    localRay = m_transform->inverse(worldRay).normalized(); // to local

    // This part acquired by few iterations, the idea was
    // the distance (its.t) will be the same regardless the local or global positioning
    // because it basically the distance between the ray origin to its intersection point
    // in here we assume that the intersection still in global coordinates, thus, 
    // getting the world position
    its.position = worldRay(its.t);
    // then transform it back to the local coordinate
    its.position = m_transform->inverse(its.position);
    // and acquire updated distance in the local coordinate
    its.t = (its.position - localRay.origin).length(); 

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        // hint: how does its.t need to change?
        // if intersected, it means that all the initial intersected points (from sphere.cpp)
        // will be scaled down to some scale. without re-calculating the ray intersection, 
        // all the points can be re-mapped to its inverse. 
        // the distance (its.t) can also acquired from this calculation
        its.position = m_transform->apply(its.position); // change to world
        its.t = (its.position - worldRay.origin).length(); // world distance

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
