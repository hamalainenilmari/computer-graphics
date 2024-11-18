// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include "Im3d.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#include "object.h"

#include "hit.h"
#include "vec_utils.h"

#include <cassert>


void GroupObject::preview_render(const Matrix4f& objectToWorld) const
{
    for (auto& o : objects_)
        o->preview_render(objectToWorld);
}

void TransformObject::preview_render(const Matrix4f& objectToWorld) const
{
    object_->preview_render(objectToWorld*matrix_);
}

void PlaneObject::preview_render(const Matrix4f& objectToWorld) const
{
    Matrix4f matrix = Matrix4f::Identity();
    auto n = normal_.normalized(); Vector3f b, c;
    if (n.cross(Vector3f(1.0f, .0f, .0f)).norm() > .0001f)
        b = n.cross(Vector3f(1.0f, 0.0f, 0.0f));
    else
        b = n.cross(Vector3f(0.0f, 1.0f, 0.0f));
    c = b.cross(n);
    
    matrix.block(0, 0, 3, 1) = c;
    matrix.block(0, 1, 3, 1) = n;
    matrix.block(0, 2, 3, 1) = b;
    matrix.block(0, 3, 3, 1) = offset_*normal_;

    Vector3f color = material_->diffuse_color(Vector3f::Zero());
    Im3d::SetColor(color(0), color(1), color(2));
    Im3d::PushMatrix(*(Im3d::Mat4*)matrix.data());
    Im3d::DrawAlignedBoxFilled(Im3d::Vec3(-10, 0, -10), Im3d::Vec3(10, 0, 10));
    Im3d::PopMatrix();
}

void SphereObject::preview_render(const Matrix4f& objectToWorld) const
{
    Matrix4f matrix = Matrix4f::Identity();
    matrix.col(0) = Vector4f(radius_, .0f, .0f, .0f);
    matrix.col(1) = Vector4f(.0f, radius_, .0f, .0f);
    matrix.col(2) = Vector4f(.0f, .0f, radius_, .0f);
    matrix.block(0, 3, 3, 1) = center_;

    matrix = objectToWorld * matrix;

    Vector3f color = material_->diffuse_color(Vector3f::Zero());
    Im3d::SetColor(color(0), color(1), color(2));
    Im3d::PushMatrix(*(Im3d::Mat4*)matrix.data());
    Im3d::DrawSphereFilled(Im3d::Vec3(0, 0, 0), 1.0f, 100);
    Im3d::PopMatrix();
}

void BoxObject::preview_render(const Matrix4f& objectToWorld) const
{
    Im3d::PushMatrix(*(Im3d::Mat4*)objectToWorld.data());
    Im3d::DrawAlignedBoxFilled(Im3d::Vec3(min_(0), min_(1), min_(2)), Im3d::Vec3(max_(0), max_(1), max_(2)));
    Im3d::PopMatrix();
}

void TriangleObject::preview_render(const Matrix4f& objectToWorld) const
{
    Vector3f color = material_->diffuse_color(vertices_[0]);

    Im3d::PushMatrix(*(Im3d::Mat4*)objectToWorld.data());
    Im3d::SetColor(color(0), color(1), color(2));
    Im3d::BeginTriangles();
    Im3d::Vertex(vertices_[0][0], vertices_[0][1], vertices_[0][2]);
    Im3d::Vertex(vertices_[1][0], vertices_[1][1], vertices_[1][2]);
    Im3d::Vertex(vertices_[2][0], vertices_[2][1], vertices_[2][2]);
    Im3d::End();
    Im3d::PopMatrix();
}
