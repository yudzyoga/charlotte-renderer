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
        Point V0;


        // a shorthand for m_triangles[i]
        Vector3i triangle;

        D = ray.direction;
        
        triangle = m_triangles[primitiveIndex];
        
        V0 = m_vertices[triangle[0]].position;          
        edge1 = m_vertices[triangle[1]].position - V0;
        edge2 = m_vertices[triangle[2]].position - V0;  
        pvec = D.cross(edge2);
        det = pvec.dot(edge1);

        // if determinant is too small() almost parallel with the triangle, ignore this hit
        // allow double-side intersection
        if (det<Epsilon && det>-Epsilon) return false;
        inv_det = 1.0 / det;

        T = ray.origin - V0;              
        u = pvec.dot(T)*inv_det;

        if (u < 0.0 || u > 1.0) return false; 

        qvec = T.cross(edge1);
        v = qvec.dot(D)*inv_det;
        
        if (v < 0.0 || v+u > 1.0) return false; 

        
        t = qvec.dot(edge2)*inv_det ;

        if (t >= Epsilon && t <its.t ) {
            //update its.t/uv/frame/position/pdf
            its.t = t;
            
            Point position = m_vertices[triangle[0]].interpolate(Vector2(u,v), m_vertices[triangle[0]], m_vertices[triangle[1]], m_vertices[triangle[2]]).position;
            its.uv.x() = (position.x() + 1.0) / 2;
            its.uv.y() = (position.y() + 1.0) / 2;
            its.position = position;

            Vector normal;
            normal = edge1.cross(edge2);

            //smooth normal 
            if(m_smoothNormals){
                normal = m_vertices[triangle[0]].interpolate(Vector2(u,v), m_vertices[triangle[0]], m_vertices[triangle[1]], m_vertices[triangle[2]]).normal;
            }

            its.frame.normal = normal.normalized();
            its.frame = Frame(its.frame.normal);

            return true;
            
        }
        else return false;

    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector3i triangle;
        Bounds bbox;

        triangle = m_triangles[primitiveIndex];

        bbox.extend(m_vertices[triangle[0]].position);
        bbox.extend(m_vertices[triangle[1]].position);
        bbox.extend(m_vertices[triangle[2]].position);
        return bbox;
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector3i triangle;

        // choose an arbituary point in the hitted triangle 
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
