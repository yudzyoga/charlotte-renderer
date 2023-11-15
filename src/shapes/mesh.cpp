#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which share an index and vertex buffer.
 * Since individual triangles are rarely needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of the element corresponds to one
     * vertex index (into @c m_vertices ) of the triangle.
     * This list will always contain as many elements as there are triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be fewer than @code 3 * numTriangles @endcode
     * vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the geometric normal instead.
    bool m_smoothNormals;

protected:
    int numberOfPrimitives() const override {
        return int(m_triangles.size());
    }

    // a flag for primitiveIndex to indicate whether 
    // const int notHIT = -1;

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its, Sampler &rng) const override {
        // NOT_IMPLEMENTED

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be computed from the vertex positions)
        
        // notations refers to "Fast Minimum Storage Ray/Triangle Intersection":
        /**
         *   t              1             | T, Edge1, Edge2|
         * [ u ] = -------------------- [ |-D,  T   , Edge2|]
         *   v     (D × Edge2) · Edge1    |-D, Edge1,  T   |
         * 
        */
        //plz refer to the equation above
        Vector edge1, edge2, T, pvec, qvec, D;
        float det, t, u, v, inv_det;
        Point V1, V2, V0;

        // store the intersection information for the nearest hit
        float its_u, its_v;

        // a shorthand for m_triangles[i]
        Vector3i triangle;

        D = ray.direction;
        primitiveIndex = -1;

        // traverse all triangle, find the nearest intersection
        // store its t and relative intersection information (values of u,v)
        for( int i = 0 ; i < numberOfPrimitives() ; i++){
            triangle = m_triangles[i];

            V0 = m_vertices[triangle.x()].position;
            V1 = m_vertices[triangle.y()].position;
            V2 = m_vertices[triangle.z()].position;

            edge1 = V1 - V0;
            edge2 = V2 - V0;
            pvec = D.cross(edge2);
            det = pvec.dot(edge1);

            
            // if determinant is too small() almost parallel with the triangle, ignore this hit
            if (det < Epsilon && det > -Epsilon) continue; // allow double-side intersection
            
            
            T = ray.origin - V0;              
            u = pvec.dot(T);
            if (u < 0.0 || u > det) continue; 

            qvec = T.cross(edge1);
            v = qvec.dot(D);
            if (v < 0.0 || v+u > det) continue; 

            inv_det = 1.0 / det;
            t = qvec.dot(edge2)*inv_det ;
            if (t < Epsilon || t > its.t) continue; 

            primitiveIndex = i;
            its_u = u*inv_det;
            its_v = v*inv_det;
            its.t = t;
        }

        if(primitiveIndex==-1) return false;
        else{
            triangle = m_triangles[primitiveIndex];
            V0 = m_vertices[triangle.x()].position;          
            edge1 = m_vertices[triangle.y()].position - V0;
            edge2 = m_vertices[triangle.z()].position - V0;  

            Point position = ray(its.t);
            its.uv.x() = (position.x() + 1.0) / 2;
            its.uv.y() = (position.y() + 1.0) / 2;
            its.position = position;

            Vector normal;
            normal = edge1.cross(edge2);
            its.pdf = 1.0/abs(normal.length());            
            if(m_smoothNormals){
                normal = m_vertices[triangle.x()].interpolate(Vector2(its_u,its_v), m_vertices[triangle.x()], m_vertices[triangle.y()], m_vertices[triangle.z()]).normal;
            }
            // its.wo = normal.normalized();
            its.frame.normal = normal.normalized();
            // set tangent parallel with V0V1
            its.frame.tangent = (edge1 + (position- V0)).normalized();
            its.frame.bitangent = normal.cross(its.frame.tangent).normalized();
            return true;
        }

    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        //suppose this shape is hitted???? or we will have to check if intersect ==true
        Vector3i triangle;
        Point m_min, m_max;

        // suppose primitiveIndex is updated, this function is invocked only when known it's hitted
        // create bbox for the hitted triangle
        triangle = m_triangles[primitiveIndex];
        m_min = elementwiseMin(m_vertices[triangle.x()].position , m_vertices[triangle.y()].position);
        m_min = elementwiseMin(m_min,m_vertices[triangle.z()].position);
        m_max = elementwiseMax(m_vertices[triangle.x()].position , m_vertices[triangle.y()].position);
        m_max = elementwiseMax(m_max , m_vertices[triangle.z()].position);

        return Bounds(m_min,m_max);
    }

    Point getCentroid(int primitiveIndex) const override {
        //suppose this shape is hitted???? or we will have to check if intersect ==true
        Vector3i triangle;

        // suppose primitiveIndex is updated, this function is invocked only when known it's hitted
        // choose a arbituary point in the hitted triangle 
        triangle = m_triangles[primitiveIndex];
        return m_vertices[triangle.x()].interpolate(Vector2(0.25,0.5), m_vertices[triangle.x()], m_vertices[triangle.y()], m_vertices[triangle.z()]).position;
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath.string(), m_triangles, m_vertices);
        logger(EInfo, "loaded ply with %d triangles, %d vertices",
            m_triangles.size(),
            m_vertices.size()
        );
        buildAccelerationStructure();
    }

    AreaSample sampleArea(Sampler &rng) const override {
        // only implement this if you need triangle mesh area light sampling for your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string()
        );
    }
};

}

REGISTER_SHAPE(TriangleMesh, "mesh")
