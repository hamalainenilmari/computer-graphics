#pragma once

#include <string>
#include <vector>
#include <map>

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#define WEIGHTS_PER_VERTEX 8
//#define ANIM_JOINT_COUNT 100

struct AnimFrame
{
    Vector3f			position;
    vector<Vector3f>	joint_angles;
};

struct Joint
{
    // Joint rotation in Euler angles.
    // This changes when you animate the mesh.
    Vector3f euler_angles = Vector3f::Zero();
    
    // Origin of current bone in parent's coordinate system.
    // This always stays fixed -- EXCEPT for the root node's
    // position, which may change with animation when the
    // character moves around.
    Vector3f position = Vector3f::Zero();

    // Current transform from joint space to world space.
    // (This is the matrix T_i in the lecture slides' notation.)
    Matrix4f joint_to_world = Matrix4f::Identity();

    // Transform from bind pose object space to joint's local coordinates.
    // This is the matrix inv(B_i) in the lecture slides' notation.
    Matrix4f bind_to_joint = Matrix4f::Identity();

    // The joint's plaintext name.
    string name;

    // This vector contains the ordering in which Euler rotations
    // are to be constructed for this particular joint. All its
    // entries are either 0, 1, or 2. 0 corresponds to the x axis,
    // 1 corresponds to the y axis, and 2 corresponds to the z axis.
    // The rotation matrix for this joint is to be constructed as
    // R(euler_angles[0], 1st axis) * R(euler_angles[1], 2nd axis) * R(euler_angles[2], 3rd axis),
    // where R denotes a function that takes an angle and a single 3D axis
    // around which the rotation is to be performed. You can use
    // Eigen's AngleAxis<float> for this; just note that you need to
    // cast the result to a Matrix3f.
    // Very concretely, if euler_order = [2 1 0], the 1st axis is z,
    // 2nd axis is y, and 3rd axis is x; if it is [2 1 2], the 1st
    // axis is z, the 2nd is y, and 3rd is (again!) z. (Yes, ZYZ is a
    // convention sometimes used for Euler angles.)
    Vector3i euler_order = Vector3i::Zero();

    // Indices of the child joints.
    vector<int> children;

    // Index of parent joint (-1 for root).
    int parent = -1;
};

class Skeleton
{
public:
    void                    loadBVH(string skeleton_file, bool initial_pose_zeros);     // if initial_pose_zeros == true,
                                                                                        // use (0,0,0) Euler angles for bind pose.
                                                                                        // Otherwise initialize with first animation frame.

    int                     getJointIndex(string name);
    string					getJointName(unsigned index) const;
    const Vector3f&			getJointRotation(unsigned index) const;
    int						getJointParent(unsigned index) const;

    int						getNumAnimationFrames() const { return animation_frames_.size(); }
    float                   getAnimationFrameTime() const { return animation_frame_time_; }
    void					setAnimationFrame(float frame);  // interpolate fractional
    void					setJointRotation(unsigned index, const Vector3f& euler_angles);
    void					incrJointRotation(unsigned index, const Vector3f& euler_angles);
    Matrix4f                computeJointToParent(unsigned index) const;
    
    void					updateToWorldTransforms();
    void                    computeNormalizationWorldTransform();

    void					getToWorldTransforms(vector<Matrix4f>& dest);
    const Matrix4f&			getToWorldTransform(unsigned joint);
    void					getSSDTransforms(vector<Matrix4f>& dest);

    const Matrix4f&         getObjectToWorldTransform() const { return object_to_world_; }

    size_t					getNumJoints() { return joints_.size(); }

private:
    void					setAnimationState();
    void					loadJoint(ifstream& in, int parent, string name);
    void					loadAnim(ifstream& in);
    void					updateToWorldTransforms(unsigned joint_index, const Matrix4f& parent_to_world);

    void					computeToBindTransforms();

    vector<Joint>			joints_;
    map<string, int>		joint_name_map_;
    vector<AnimFrame>		animation_frames_;
    Matrix4f                object_to_world_        = Matrix4f::Identity();
    float                   animation_frame_time_   = 1.0f / 30.0f;
    float					animation_frame_        = 0;
};
