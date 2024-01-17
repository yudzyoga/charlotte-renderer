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

    // basic idea: transformed tangent plane is still tangent(normal not normal), so just tranform (bi)tangent (two inparallel lines define a plane)
    // steps1: transform both (bi)tangents (form a new tangent plane after transforming)
    // step2:  compute normal based on new tangent plane
    // step3:  to build orthogonal basis, recompute bitangent based on tangent and normal
    surf.frame.bitangent = m_transform->apply(surf.frame.bitangent);
    surf.frame.tangent = m_transform->apply(surf.frame.tangent).normalized();
    if (m_flipNormal) {
        //clockwise
        surf.frame.normal = surf.frame.bitangent.cross(surf.frame.tangent).normalized();
    }  
    else {
        //anticlockwise
        surf.frame.normal = surf.frame.tangent.cross(surf.frame.bitangent).normalized();
    }
    surf.frame.bitangent = surf.frame.normal.cross(surf.frame.tangent).normalized();
   
    if (m_normal != nullptr) {
        // [RC] evaluate normals
        Color norm_color = m_normal->evaluate(surf.uv);
        Vector norm_data {2*norm_color.r()-1, 2*norm_color.g()-1, 2*norm_color.b()-1};
        Vector new_normal = norm_data.x() * surf.frame.tangent + 
                            norm_data.y() * surf.frame.bitangent + 
                            norm_data.z() * 5 * surf.frame.normal; 
        surf.frame.normal = new_normal.normalized();
        surf.frame = Frame(surf.frame.normal);
    }
}

bool Instance::intersect(const Ray &worldRay, Intersection &its, Sampler &rng) const {
    if (!m_transform) {
        // fast path, if no transform is needed
        Ray localRay = worldRay;
        if (m_shape->intersect(localRay, its, rng)) {
            its.instance = this;
            return true;
        } else {
            return false;
        }
    }

    // /*
    // hints:
    // * transform the ray (do not forget to normalize!)
    // * how does its.t need to change?

    // step1: Transform world ray into local ray
    // step2: Duplicate its and get local intersect position
    // step3: Set local intersect position to infinity so that 
    // we are not ignoring potential hitting candidate
    Ray localRay = m_transform->inverse(worldRay).normalized(); // to local
    Intersection localIts = its;
    Intersection worldIts = its;
    localIts.position = m_transform->inverse(worldIts.position); // to local
    localIts.t = INFINITY;
    
    // step4: check and update if candidate exists
    bool isIts = m_shape->intersect(localRay, localIts, rng);
    // step5: Transform the positon and t from local to world coord
    worldIts.position = m_transform->apply(localIts.position); 
    // assert(worldIts.position[0] != worldRay.origin[0] || worldIts.position[1] != worldRay.origin[1] || worldIts.position[2] != worldRay.origin[2]);
    worldIts.t = (worldIts.position - worldRay.origin).length(); 

    // if(worldIts.t<Epsilon) return false;

    // step6: compare candidate with our original its (then update or do nothing)
    if (isIts && worldIts.t<its.t) {
        // [RC] add a new check for uv, to check whether it is alpha or not.
        if (m_alpha != nullptr){
            Color alpha_mask = m_alpha->evaluate(localIts.uv);
            float a = alpha_mask.mean();
            if (rng.next() > a) {
                return false;
            } 
            // if (alpha_mask == Color(0)) return false;
        }
        // hint: how does its.t need to change?
        its = localIts;
        its.t = worldIts.t;
        // its.position = worldIts.position;

        its.instance = this;
        transformFrame(its); //transform the surfaceevent
        its.wo = (localRay.origin-its.position).normalized();
        // its.wo = (worldRay.origin-worldIts.position).normalized();

        return true;
    } else {
        return false;
    }
    // */

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
