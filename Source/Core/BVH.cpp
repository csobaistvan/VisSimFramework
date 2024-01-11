#include "PCH.h"
#include "BVH.h"
#include "LibraryExtensions/GlmEx.h"

namespace BVH
{
    ////////////////////////////////////////////////////////////////////////////////
    bool isZero(float val, float epsilon = 1e-5f)
    {
        return glm::abs(val) <= epsilon;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Constructors
    Ray::Ray() :
        m_origin(),
        m_direction(0.0f, 0.0f, 1.0f)
    {}

    Ray::Ray(glm::vec3 origin, glm::vec3 direction) :
        m_origin(origin),
        m_direction(direction)
    {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Accessors
    Ray Ray::moveTo(glm::vec3 destination)
    {
        return Ray(destination, m_direction);
    }

    Ray Ray::faceTo(glm::vec3 destination)
    {
        return Ray(m_origin, glm::normalize(destination - m_origin));
    }

    Ray Ray::orient(glm::vec3 orientation)
    {
        return Ray(m_origin, orientation);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Intersections
    bool Ray::isPointOnRay(glm::vec3 point) const
    {
        return isZero(glm::dot(m_direction, point - m_origin));
    }

    /** Computes the closest point on the ray to the parameter point. */
    glm::vec3 Ray::getClosestPoint(glm::vec3 point) const
    {
        return m_origin + m_direction * glm::dot(m_direction, point - m_origin);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Constructors
    Plane::Plane() :
        m_normal(0.0f, 1.0f, 0.0f),
        m_distance(0.0f)
    {}

    Plane::Plane(glm::vec3 normal, float d) :
        m_normal(normal),
        m_distance(d)
    {}

    Plane::Plane(glm::vec3 normal, glm::vec3 memberPoint) :
        m_normal(normal),
        m_distance(-glm::dot(normal, memberPoint))
    {}

    Plane::Plane(glm::vec3 a, glm::vec3 b, glm::vec3 c) :
        Plane(glm::normalize(glm::cross(b - a, c - a)), a)
    {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Intersections
    float Plane::distanceTo(glm::vec3 point) const
    {
        return glm::abs(glm::dot(point, m_normal) + m_distance);
    }

    float Plane::distanceToSigned(glm::vec3 point) const
    {
        return glm::dot(point, m_normal) + m_distance;
    }

    bool Plane::isOnSurface(glm::vec3 point) const
    {
        return isZero(glm::dot(point, m_normal) + m_distance);
    }

    bool Plane::intersects(const Ray& ray) const
    {
        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the angle between the ray and the plane
        float angle = glm::dot(m_normal, ray.m_direction);

        ////////////////////////////////////////////////////////////////////////////////
        //  Make sure they're not parallel
        if (isZero(angle))
            return false;

        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the ray parameter for the intersection
        float param = -(glm::dot(m_normal, ray.m_origin) + m_distance) / angle;

        ////////////////////////////////////////////////////////////////////////////////
        //  There's collision if it's positive
        return param >= 0.0f;
    }

    bool Plane::intersects(const Plane& plane) const
    {
        return glm::length2(glm::cross(m_normal, plane.m_normal)) > 1e-5f;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Constructors
    AABB::AABB() :
        m_min(),
        m_max()
    {}

    AABB::AABB(glm::vec3 point) :
        m_min(point),
        m_max(point)
    {}

    AABB::AABB(glm::vec3 min, glm::vec3 max) :
        m_min(min),
        m_max(max)
    {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Accessors
    AABB AABB::extend(glm::vec3 point) const
    {
        return AABB(glm::min(m_min, point), glm::max(m_max, point));
    }

    AABB AABB::extend(const AABB& other) const
    {
        return extend(other.m_min).extend(other.m_max);
    }

    AABB AABB::scale(float scale) const
    {
        return AABB(m_min * scale, m_max * scale);
    }

    AABB AABB::transform(glm::mat4 const& model) const
    {
        static const glm::vec3 s_MASKS[] =
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(1.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
        };
        
        glm::vec3 extent = m_max - m_min;
        AABB result = AABB(glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX));
        for (size_t i = 0; i < 8; ++i)
        {
            const glm::vec3 vertex = m_min + s_MASKS[i] * extent;
            result = result.extend(glm::vec3(model * glm::vec4(vertex, 1.0f)));
        }
        return result;
    }

    AABB AABB::moveTo(glm::vec3 destination) const
    {
        glm::vec3 offset = destination - getCenter();
        return AABB(m_min + offset, m_max + offset);
    }

    glm::vec3 AABB::getCenter() const
    {
        return (m_min + m_max) * 0.5f;
    }

    glm::vec3 AABB::getSize() const
    {
        return m_max - m_min;
    }

    glm::vec3 AABB::getHalfSize() const
    {
        return (m_max - m_min) * 0.5f;
    }

    float AABB::getVolume() const
    {
        return (m_max.x - m_min.x) * (m_max.y - m_min.y) * (m_max.z - m_min.z);
    }

    glm::vec3 AABB::getVertex(size_t id) const
    {
        return glm::vec3(
            m_min.x + (id % 2) * (m_max.x - m_min.x),
            m_min.y + (id / 4) * (m_max.y - m_min.y),
            m_min.z + ((id / 2) % 2) * (m_max.z - m_min.z)
        );
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Intersections
    float AABB::distanceTo(glm::vec3 point) const
    {
        return (point - closestPointTo(point)).length();
    }

    glm::vec3 AABB::closestPointTo(glm::vec3 point) const
    {
        return glm::clamp(point, m_min, m_max);
    }

    bool AABB::isInside(glm::vec3 point) const
    {
        return (point.x >= m_min.x && point.x <= m_max.x &&
            point.y >= m_min.y && point.y <= m_max.y &&
            point.z >= m_min.z && point.z <= m_max.z);
    }

    bool AABB::isFullyInside(glm::vec3 point) const
    {
        return (point.x > m_min.x && point.x < m_max.x&&
            point.y > m_min.y && point.y < m_max.y&&
            point.z > m_min.z && point.z < m_max.z);
    }

    bool AABB::isOnSurface(glm::vec3 point) const
    {
        return ((point.x == m_min.x || point.x == m_max.x) &&
            (point.y == m_min.y || point.y == m_max.y) &&
            (point.z == m_min.z || point.z == m_max.z));
    }

    bool AABB::isOutside(glm::vec3 point) const
    {
        return (point.x < m_min.x || point.x > m_max.x &&
            point.y < m_min.y || point.y > m_max.y &&
            point.z < m_min.z || point.z > m_max.z);
    }

    bool AABB::intersects(const Ray& ray) const
    {
        ////////////////////////////////////////////////////////////////////////////////
        //  Test against the planes
        glm::vec3 tNear = (m_min - ray.m_origin) / ray.m_direction;
        glm::vec3 tFar = (m_max - ray.m_origin) / ray.m_direction;

        ////////////////////////////////////////////////////////////////////////////////
        //  Get the params for each axis
        glm::vec3 tMin(glm::min(tNear, tFar));
        glm::vec3 tMax(glm::max(tNear, tFar));

        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the enter and leave parameters
        float tEnter = glm::min(glm::min(tMin.x, tMin.y), tMin.z);
        float tLeave = glm::max(glm::max(tMax.x, tMax.y), tMax.z);

        ////////////////////////////////////////////////////////////////////////////////
        //  Interpret the results
        return (tEnter < tLeave || tLeave >= 0.0f);
    }

    bool AABB::intersects(const Plane& plane) const
    {
        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the radius of possible intersection
        float radius = glm::abs(glm::dot(plane.m_normal, getHalfSize()));

        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the distance from the AABB center to the plane
        float distToCenter = plane.distanceTo(getCenter());

        ////////////////////////////////////////////////////////////////////////////////
        //  Interpret the results
        return distToCenter <= radius;
    }

    bool AABB::isInside(const AABB& other) const
    {
        return (other.m_min.x >= m_min.x && other.m_min.y >= m_min.y && other.m_min.z >= m_min.z &&
            other.m_max.x <= m_max.x && other.m_max.y <= m_max.y && other.m_max.z <= m_max.z);
    }

    bool AABB::isFullyInside(const AABB& other) const
    {
        return (other.m_min.x > m_min.x && other.m_min.y > m_min.y && other.m_min.z > m_min.z &&
            other.m_max.x < m_max.x&& other.m_max.y < m_max.y&& other.m_max.z < m_max.z);
    }

    bool AABB::intersects(const AABB& other) const
    {
        return (
            (other.m_min.x < m_min.x && other.m_max.x > m_min.x) ||
            (other.m_min.x < m_max.x && other.m_max.x > m_max.x) ||
            (other.m_min.y < m_min.y && other.m_max.y > m_min.y) ||
            (other.m_min.y < m_max.y && other.m_max.y > m_max.y) ||
            (other.m_min.z < m_min.z && other.m_max.z > m_min.z) ||
            (other.m_min.z < m_max.z && other.m_max.z > m_max.z));
    }

    bool AABB::isOutside(const AABB& other) const
    {
        return (other.m_min.x > m_max.x || other.m_min.y > m_max.y || other.m_min.z > m_max.z ||
            other.m_max.x < m_min.x || other.m_max.y < m_min.y || other.m_max.z < m_min.z);
    }

    bool AABB::isInside(const Sphere& sphere) const
    {
        return isInside(sphere.m_center - glm::vec3(sphere.m_radius)) &&
            isInside(sphere.m_center + glm::vec3(sphere.m_radius));
    }

    bool AABB::intersects(const Sphere& sphere) const
    {
        return distanceTo(sphere.m_center) <= sphere.m_radius;
    }

    bool AABB::isOutside(const Sphere& sphere) const
    {
        return distanceTo(sphere.m_center) > sphere.m_radius;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Constructors
    Sphere::Sphere() :
        m_center(),
        m_radius(1.0f)
    {}

    Sphere::Sphere(glm::vec3 center, float radius) :
        m_center(center),
        m_radius(radius)
    {}

    Sphere::Sphere(glm::vec3 center, glm::vec3 surfacePoint) :
        m_center(center),
        m_radius((center - surfacePoint).length())
    {}

    ////////////////////////////////////////////////////////////////////////////////
    //  Accessors
    Sphere Sphere::scale(float scale) const
    {
        return Sphere(m_center, m_radius * scale);
    }

    Sphere Sphere::moveTo(glm::vec3 destination) const
    {
        return Sphere(destination, m_radius);
    }

    float Sphere::getVolume() const
    {
        return 1.33f * glm::pi<float>() * m_radius * m_radius * m_radius;
    }

    float Sphere::distanceTo(glm::vec3 point) const
    {
        return glm::max(distanceTo(point), 0.0f);
    }

    float Sphere::distanceToSigned(glm::vec3 point) const
    {
        return (point - m_center).length() - m_radius;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Intersection tests
    bool Sphere::isInside(glm::vec3 point) const
    {
        return (point - m_center).length() <= m_radius;
    }

    bool Sphere::isFullyInside(glm::vec3 point) const
    {
        return (point - m_center).length() < m_radius;
    }

    bool Sphere::isOutside(glm::vec3 point) const
    {
        return (point - m_center).length() > m_radius;
    }

    bool Sphere::intersects(const Ray& ray) const
    {
        ////////////////////////////////////////////////////////////////////////////////
        //  Solve || ray.origin + t * ray.dir - sphere.center|| = sphere.radius
        glm::vec3 a = ray.m_origin - m_center;
        float C = glm::dot(a, a) - m_radius * m_radius;
        float B = 2.0f * glm::dot(a, ray.m_direction);

        ////////////////////////////////////////////////////////////////////////////////
        //  Compute the determinant for the above equation
        float det = B * B - 4.0f * C;

        ////////////////////////////////////////////////////////////////////////////////
        //  There is intersection, if the determinant is not negative
        return det > 0.0f;
    }

    bool Sphere::intersects(const Plane& plane) const
    {
        return plane.distanceTo(m_center) <= m_radius;
    }

    bool Sphere::isOutside(const Sphere& other) const
    {
        return (m_center - other.m_center).length() > (m_radius + other.m_radius);
    }

    bool Sphere::isInside(const Sphere& other) const
    {
        return (m_center - other.m_center).length() <= m_radius - other.m_radius;
    }

    bool Sphere::isFullyInside(const Sphere& other) const
    {
        return (m_center - other.m_center).length() < m_radius - other.m_radius;
    }

    bool Sphere::intersects(const Sphere& other) const
    {
        return (m_center - other.m_center).length() <= (m_radius + other.m_radius);
    }

    bool Sphere::isOutside(const AABB& AABB) const
    {
        return isOutside(AABB.m_min) && isOutside(AABB.m_max);
    }

    bool Sphere::isInside(const AABB& AABB) const
    {
        return isInside(AABB.m_min) && isInside(AABB.m_max);
    }

    bool Sphere::intersects(const AABB& AABB) const
    {
        return (isInside(AABB.m_min) && isOutside(AABB.m_max)) ||
            (isOutside(AABB.m_min) && isInside(AABB.m_max));
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Constructors
    Frustum::Frustum()
    {
        m_clipPlanes[0] = { glm::vec3(-1.0f,  0.0f,  0.0f), -glm::vec3(1.0f, 0.0f, 0.0f) };
        m_clipPlanes[1] = { glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec3(1.0f, 0.0f, 0.0f) };
        m_clipPlanes[2] = { glm::vec3( 0.0f,  1.0f,  0.0f), -glm::vec3(0.0f, 1.0f, 0.0f) };
        m_clipPlanes[3] = { glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec3(0.0f, 1.0f, 0.0f) };
        m_clipPlanes[4] = { glm::vec3( 0.0f,  0.0f, -1.0f), -glm::vec3(0.0f, 0.0f, 1.0f) };
        m_clipPlanes[5] = { glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec3(0.0f, 0.0f, 1.0f) };
    }

    Frustum::Frustum(glm::vec3 nearMin, glm::vec3 nearMax, glm::vec3 farMin, glm::vec3 farMax)
    {
        AABB near(nearMin, nearMax);
        AABB far(farMin, farMax);

        /* Layout of the two AABB's:
        *      4-------5
        *     /|      /|
        *    / |     / |
        *   4-------5  |
        *   |  0----|--1 far
        *   | /     | /
        *   |/      |/
        *   0-------1    near
        */

        /** Left, right, bottom, top, near, far. */
        m_clipPlanes[0] = Plane(far.getVertex(0), near.getVertex(0), near.getVertex(4));
        m_clipPlanes[1] = Plane(near.getVertex(1), far.getVertex(1), far.getVertex(5));
        m_clipPlanes[2] = Plane(far.getVertex(0), far.getVertex(1), near.getVertex(1));
        m_clipPlanes[3] = Plane(near.getVertex(4), near.getVertex(5), far.getVertex(5));
        m_clipPlanes[4] = Plane(near.getVertex(0), near.getVertex(1), near.getVertex(5));
        m_clipPlanes[5] = Plane(far.getVertex(1), far.getVertex(0), far.getVertex(4));
    }

    Frustum::Frustum(glm::mat4 const& viewProj)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Extract the clip planes from the transformation matrix
        // As explained by: http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf
        glm::vec4 rowX = glm::row(viewProj, 0);
        glm::vec4 rowY = glm::row(viewProj, 1);
        glm::vec4 rowZ = glm::row(viewProj, 2);
        glm::vec4 rowW = glm::row(viewProj, 3);

        glm::vec4 left  = -(rowW + rowX);
        glm::vec4 right = -(rowW - rowX);
        glm::vec4 bot   = -(rowW + rowY);
        glm::vec4 top   = -(rowW - rowY);
        glm::vec4 near  = -(rowW + rowZ);
        glm::vec4 far   = -(rowW - rowZ);

        left /= glm::length(glm::vec3(left));
        right /= glm::length(glm::vec3(right));
        bot /= glm::length(glm::vec3(bot));
        top /= glm::length(glm::vec3(top));
        near /= glm::length(glm::vec3(near));
        far /= glm::length(glm::vec3(far));

        /** Left, right, bottom, top, near, far. */
        m_clipPlanes[0] = Plane(glm::vec3(left),   left.w  );
        m_clipPlanes[1] = Plane(glm::vec3(right),  right.w );
        m_clipPlanes[2] = Plane(glm::vec3(bot),    bot.w   );
        m_clipPlanes[3] = Plane(glm::vec3(top),    top.w   );
        m_clipPlanes[4] = Plane(glm::vec3(near),   near.w  );
        m_clipPlanes[5] = Plane(glm::vec3(far),    far.w   );
    }

    ////////////////////////////////////////////////////////////////////////////////
    //  Intersections
    bool Frustum::isOnSurface(glm::vec3 point) const
    {
        return 
            m_clipPlanes[0].isOnSurface(point) ||
            m_clipPlanes[1].isOnSurface(point) ||
            m_clipPlanes[2].isOnSurface(point) ||
            m_clipPlanes[3].isOnSurface(point) ||
            m_clipPlanes[4].isOnSurface(point) ||
            m_clipPlanes[5].isOnSurface(point);
    }

    bool Frustum::isInside(glm::vec3 point) const
    {
        return 
            m_clipPlanes[0].distanceToSigned(point) < 0.0f &&
            m_clipPlanes[1].distanceToSigned(point) < 0.0f &&
            m_clipPlanes[2].distanceToSigned(point) < 0.0f &&
            m_clipPlanes[3].distanceToSigned(point) < 0.0f &&
            m_clipPlanes[4].distanceToSigned(point) < 0.0f &&
            m_clipPlanes[5].distanceToSigned(point) < 0.0f;
    }

    bool Frustum::isOutside(glm::vec3 point) const
    {
        return 
            m_clipPlanes[0].distanceToSigned(point) > 0.0f ||
            m_clipPlanes[1].distanceToSigned(point) > 0.0f ||
            m_clipPlanes[2].distanceToSigned(point) > 0.0f ||
            m_clipPlanes[3].distanceToSigned(point) > 0.0f ||
            m_clipPlanes[4].distanceToSigned(point) > 0.0f ||
            m_clipPlanes[5].distanceToSigned(point) > 0.0f;
    }

    bool Frustum::intersects(const Ray& ray) const
    {
        return m_clipPlanes[0].intersects(ray) ||
            m_clipPlanes[1].intersects(ray) ||
            m_clipPlanes[2].intersects(ray) ||
            m_clipPlanes[3].intersects(ray) ||
            m_clipPlanes[4].intersects(ray) ||
            m_clipPlanes[5].intersects(ray);
    }

    bool Frustum::intersects(const Plane& plane) const
    {
        return m_clipPlanes[0].intersects(plane) ||
            m_clipPlanes[1].intersects(plane) ||
            m_clipPlanes[2].intersects(plane) ||
            m_clipPlanes[3].intersects(plane) ||
            m_clipPlanes[4].intersects(plane) ||
            m_clipPlanes[5].intersects(plane);
    }

    Intersection Frustum::intersection(AABB const& box) const
    {
        Intersection result = Inside;
        glm::vec3 vmin, vmax;

        for (int i = 0; i < 6; ++i)
        {
            for (int a = 0; a < 3; ++a)
            {
                if (m_clipPlanes[i].m_normal[a] > 0)
                {
                    vmin[a] = box.m_min[a];
                    vmax[a] = box.m_max[a];
                }
                else
                {
                    vmin[a] = box.m_max[a];
                    vmax[a] = box.m_min[a];
                }
            }

            if (m_clipPlanes[i].distanceToSigned(vmin) > 0)
                return Outside;
            if (m_clipPlanes[i].distanceToSigned(vmax) >= 0)
                result = Intersects;
        }
        return result;
    }

    bool Frustum::intersects(const AABB& AABB) const
    {
        return intersection(AABB) == Intersects;
    }

    bool Frustum::isInside(const AABB& AABB) const
    {
        return intersection(AABB) == Inside;
    }

    bool Frustum::isOutside(const AABB& AABB) const
    {
        return intersection(AABB) == Outside;
    }

    bool Frustum::intersects(const Sphere& sphere) const
    {
        return m_clipPlanes[0].distanceTo(sphere.m_center) <= sphere.m_radius ||
            m_clipPlanes[1].distanceTo(sphere.m_center) <= sphere.m_radius ||
            m_clipPlanes[2].distanceTo(sphere.m_center) <= sphere.m_radius ||
            m_clipPlanes[3].distanceTo(sphere.m_center) <= sphere.m_radius ||
            m_clipPlanes[4].distanceTo(sphere.m_center) <= sphere.m_radius ||
            m_clipPlanes[5].distanceTo(sphere.m_center) <= sphere.m_radius;
    }

    bool Frustum::isInside(const Sphere& sphere) const
    {
        return m_clipPlanes[0].distanceToSigned(sphere.m_center) <= -sphere.m_radius ||
            m_clipPlanes[1].distanceToSigned(sphere.m_center) <= -sphere.m_radius ||
            m_clipPlanes[2].distanceToSigned(sphere.m_center) <= -sphere.m_radius ||
            m_clipPlanes[3].distanceToSigned(sphere.m_center) <= -sphere.m_radius ||
            m_clipPlanes[4].distanceToSigned(sphere.m_center) <= -sphere.m_radius ||
            m_clipPlanes[5].distanceToSigned(sphere.m_center) <= -sphere.m_radius;
    }

    bool Frustum::isOutside(const Sphere& sphere) const
    {
        return m_clipPlanes[0].distanceToSigned(sphere.m_center) > sphere.m_radius ||
            m_clipPlanes[1].distanceToSigned(sphere.m_center) > sphere.m_radius ||
            m_clipPlanes[2].distanceToSigned(sphere.m_center) > sphere.m_radius ||
            m_clipPlanes[3].distanceToSigned(sphere.m_center) > sphere.m_radius ||
            m_clipPlanes[4].distanceToSigned(sphere.m_center) > sphere.m_radius ||
            m_clipPlanes[5].distanceToSigned(sphere.m_center) > sphere.m_radius;
    }
}

namespace std
{
    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Ray const& ray)
    {
        return stream << "Ray{ origin: " << ray.m_origin << ", direction: " << ray.m_direction << " }";
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Plane const& plane)
    {
        return stream << "Plane{ normal: " << plane.m_normal << ", distance: " << plane.m_distance << " }";
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::AABB const& aabb)
    {
        return stream << "AABB{ min: " << aabb.m_min << ", max: " << aabb.m_max << " }";
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Sphere const& sphere)
    {
        return stream << "Sphere{ min: " << sphere.m_center << ", radius: " << sphere.m_radius << " }";
    }

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Frustum const& frustum)
    {
        return stream << "Frustum{ "
            << "left: " << frustum.m_clipPlanes[0] << ", "
            << "right: " << frustum.m_clipPlanes[1] << ", "
            << "bottom: " << frustum.m_clipPlanes[2] << ", "
            << "top: " << frustum.m_clipPlanes[3] << ", "
            << "near: " << frustum.m_clipPlanes[4] << ", "
            << "far: " << frustum.m_clipPlanes[5] << " }";
    }
}