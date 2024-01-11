#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Constants.h"

namespace BVH
{
    ////////////////////////////////////////////////////////////////////////////////
    /** Forward-declare the relevant classes. */
    struct Ray;
    struct Plane;
    struct AABB;
    struct Sphere;
    struct Frustum;

    // Intersection relation
    meta_enum(Intersection, int, Inside, Intersects, Outside);

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a ray in 3D space. */
    struct Ray
    {
        /** Starting point of the ray. */
        glm::vec3 m_origin;

        /** Direction of the ray. */
        glm::vec3 m_direction;

        /** Initializes an empty ray. */
        Ray();

        /** Initializes the ray using its origin and direction. */
        Ray(glm::vec3 origin, glm::vec3 direction);

        /** Moves the ray to the target point. */
        Ray moveTo(glm::vec3 destination);

        /** Makes the ray face the target point. */
        Ray faceTo(glm::vec3 destination);

        /** Changes the orientation of the ray. */
        Ray orient(glm::vec3 orientation);

        /** Tests whether the parameter point is on the ray. */
        bool isPointOnRay(glm::vec3 point) const;

        /** Computes the closest point on the ray to the parameter point. */
        glm::vec3 getClosestPoint(glm::vec3 point) const;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a plane in 3D space. */
    struct Plane
    {
        /** Normal vector of the plane. */
        glm::vec3 m_normal;

        /** Distance from the origin. */
        float m_distance;

        /** Constructs an empty plane, which is on the XY plane, and lookin up the Y axis. */
        Plane();

        /** Construcs a plane using the provided normal and D coefficient. */
        Plane(glm::vec3 normal, float d);

        /** Constructs a plane from the provided normal and plane member point. */
        Plane(glm::vec3 normal, glm::vec3 memberPoint);

        /** Constructs a plane from 3 points that all lie on the plane. */
        Plane(glm::vec3 a, glm::vec3 b, glm::vec3 c);

        /** Calculates the plane's distance to the parameter point. */
        float distanceTo(glm::vec3 point) const;

        /** Calculates the plane's distance to the parameter point. */
        float distanceToSigned(glm::vec3 point) const;

        /** Tests whether the parameter point is on the plane. */
        bool isOnSurface(glm::vec3 point) const;

        /** Tests whether the parameter ray intersects the plane. */
        bool intersects(Ray const& ray) const;

        /** Tests whether the parameter plane intersects the plane. */
        bool intersects(Plane const& plane) const;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents an axis-aligned bounding box in 3D space. */
	struct AABB
    {
        /** Lower-left corner of the AABB. */
        glm::vec3 m_min;

        /** Upper-right corner of the AABB. */
        glm::vec3 m_max;

        /** Constructs an empty AABB. */
        AABB();

        /** Initializes the AABB with the given location vector. */
        AABB(glm::vec3 point);

        /** Initializes the AABB from the parameter corners. */
        AABB(glm::vec3 min, glm::vec3 max);

        /** Extends the box such that it contains the target point. */
        AABB extend(glm::vec3 point) const;

        /** Extends the box by another one. */
        AABB extend(AABB const& other) const;

        /** Scales the matrix */
        AABB scale(float scale) const;

        /** Transforms the box by the parameter matrix. */
        AABB transform(const glm::mat4& model) const;

        /** Moves the box such that its center point will be the parameter point. */
        AABB moveTo(glm::vec3 destination) const;

        /** Returns the center point of the box. */
        glm::vec3 getCenter() const;

        /** Returns the size of the box along each axis. */
        glm::vec3 getSize() const;

        /** Returns half of the size of the box along each axis. */
        glm::vec3 getHalfSize() const;

        /** Calculates the volume of the box. */
        float getVolume() const;

        /** Calculates one of the 8 vertices of the AABB
        *      4-------5
        *     /|      /|
        *    / |     / |
        *   6-------7  |
        *   |  0----|--1
        *   | /     | /
        *   |/      |/
        *   2-------3
        */
        glm::vec3 getVertex(size_t id) const;

        /** Calculates the distance to the parameter point. */
        float distanceTo(glm::vec3 point) const;

        /** Calculates the closest point to the parameter point. */
        glm::vec3 closestPointTo(glm::vec3 point) const;

        /** Tests whether the parameter point is inside the box (or in its surface). */
        bool isInside(glm::vec3 point) const;

        /** Tests whether the parameter point is fully inside the box (i.e. not on its surface). */
        bool isFullyInside(glm::vec3 point) const;

        /** Tests whether the parameter point exactly on the surface of the box. */
        bool isOnSurface(glm::vec3 point) const;

        /** Tests whether the parameter point is outside the box. */
        bool isOutside(glm::vec3 point) const;

        /** Tests whether the parameter ray intersects the box. */
        bool intersects(Ray const& ray) const;

        /** Tests whether the parameter plane intersects the box. */
        bool intersects(Plane const& plane) const;

        /** Tests whether the parameter box intersects the box. */
        bool intersects(AABB const& other) const;

        /** Tests whether the parameter box inside the box. */
        bool isInside(AABB const& other) const;

        /** Tests whether the parameter box is fully inside the box (i.e. they're not identical). */
        bool isFullyInside(AABB const& other) const;

        /** Tests whether the parameter box is outside the box. */
        bool isOutside(AABB const& other) const;

        /** Tests whether the parameter sphere is inside the box. */
        bool isInside(Sphere const& sphere) const;

        /** Tests whether the parameter sphere intersects the box. */
        bool intersects(Sphere const& sphere) const;

        /** Tests whether the parameter sphere is outside the box. */
        bool isOutside(Sphere const& sphere) const;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a sphere in 3D space. */
    struct Sphere
    {
        /** Center of the sphere. */
        glm::vec3 m_center;

        /** Radius of the sphere. */
        float m_radius;

        /** Initializes an empty sphere. */
        Sphere();

        /** Initializes the sphere with its center and radius. */
        Sphere(glm::vec3 center, float radius);

        /** Initializes the sphere using its center and a surface location. */
        Sphere(glm::vec3 center, glm::vec3 surfacePoint);

        /** Moves the sphere such that its center point will be the parameter point. */
        Sphere scale(float scale) const;

        /** Moves the sphere such that its center point will be the parameter point. */
        Sphere moveTo(glm::vec3 destination) const;

        /** Calculates the volume of the sphere. */
        float getVolume() const;

        /** Calculates the distance to the parameter point. */
        float distanceTo(glm::vec3 point) const;

        /** Calculates the signed distance (i.e. negative if the point is inside) to the parameter point. */
        float distanceToSigned(glm::vec3 point) const;

        /** Tests whether the parameter point is inside the sphere. */
        bool isInside(glm::vec3 point) const;

        /** Tests whether the parameter point is fully inside the sphere (i.e. not on the surface) */
        bool isFullyInside(glm::vec3 point) const;

        /** Tests whether the parameter point is outside the sphere. */
        bool isOutside(glm::vec3 point) const;

        /** Tests whether the parameter ray intersects the sphere. */
        bool intersects(Ray const& ray) const;

        /** Tests whether the parameter plane intersects the sphere. */
        bool intersects(Plane const& ray) const;

        /** Tests whether the parameter sphere is outside the sphere. */
        bool isOutside(Sphere const& other) const;

        /** Tests whether the parameter sphere is inside the sphere. */
        bool isInside(Sphere const& other) const;

        /** Tests whether the parameter sphere is fully inside the sphere. */
        bool isFullyInside(Sphere const& other) const;

        /** Tests whether the parameter sphere intersects the sphere. */
        bool intersects(Sphere const& other) const;

        /** Tests whether the parameter box is outside the sphere. */
        bool isOutside(AABB const& box) const;

        /** Tests whether the parameter box is inside the sphere. */
        bool isInside(AABB const& box) const;

        /** Tests whether the parameter box intersects the sphere. */
        bool intersects(AABB const& box) const;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /** Represents a view frustum in 3D space. */
    struct Frustum
    {
        /** Stores the planes that make up the frustum in the following order: left, right, bottom, top, near, far. */
        Plane m_clipPlanes[6];

        /** Constructs an empty frustum. */
        Frustum();

        /** Constructs a frustum using its 4 corners */
        Frustum(glm::vec3 nearMin, glm::vec3 nearMax, glm::vec3 farMin, glm::vec3 farMax);

        /** Constructs a frustum by extracting the clip planes of the provided transformation matrix. */
        Frustum(const glm::mat4& viewProj);

        /** Tests whether the parameter point is on the surface of the frustum. */
        bool isOnSurface(glm::vec3 point) const;

        /** Tests whether the parameter point is inside the frustum. */
        bool isInside(glm::vec3 point) const;

        /** Tests whether the parameter point is outside the frustum. */
        bool isOutside(glm::vec3 point) const;

        /** Tests whether the parameter ray intersects the frustum. */
        bool intersects(Ray const& ray) const;

        /** Tests whether the parameter plane intersects the frustum. */
        bool intersects(Plane const& plane) const;

        /** Tests the intersection status of the BVH and the AABB. */
        Intersection intersection(AABB const& box) const;

        /** Tests whether the parameter box intersects the frustum. */
        bool intersects(AABB const& box) const;

        /** Tests whether the parameter box is inside the frustum. */
        bool isInside(AABB const& box) const;

        /** Tests whether the parameter box is outside the frustum. */
        bool isOutside(AABB const& box) const;

        /** Tests whether the parameter sphere intersects the frustum. */
        bool intersects(Sphere const& sphere) const;

        /** Tests whether the parameter sphere is inside the frustum. */
        bool isInside(Sphere const& sphere) const;

        /** Tests whether the parameter sphere is outside the frustum. */
        bool isOutside(Sphere const& sphere) const;
    };  
}

namespace std
{
    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Ray const& ray);

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Plane const& plane);

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::AABB const& aabb);

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Sphere const& sphere);

    ////////////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& stream, BVH::Frustum const& frustum);
}