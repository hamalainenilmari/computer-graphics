#include "skeleton.h"

#include <cassert>
#include <fstream>
#include <stack>
#include <sstream>
#include <cctype>
#include <iostream>

using namespace std;


namespace {
    // Helper for linear interpolation of Euler angles between animation frames.
    // Needed because the angles are always given in [0,360[, in the BVH,
    // and wrapping causes incorrect interpolation "the long way around".
    // Essentially just a phase unwrap (https://se.mathworks.com/help/matlab/ref/unwrap.html)
    float lerpAngle(float a, float b, float w)
    {
        if (b - a > 180.0f)
            b = b - 360.0f;
        if (b - a < -180.0f)
            b = b + 360.0f;
        return (1.0f - w) * a + w * b;
    }
    Vector3f lerpEuler(const Vector3f& a, const Vector3f& b, float w)
    {
        return Vector3f{ lerpAngle(a(0), b(0), w),
                         lerpAngle(a(1), b(1), w),
                         lerpAngle(a(2), b(2), w) };
    }
}

void Skeleton::setJointRotation(unsigned index, const Vector3f& euler_angles)
{
    Joint& joint = joints_[index];

    // For convenient reading, we store the rotation angles in the joint
    // as-is, in addition to setting the to_parent matrix to match the rotation.
    // We store the angles in degress (grrrr...) because that's the way they're
    // given in the .bvh files, and having the values be the same makes
    // debugging easier.
    joint.euler_angles = euler_angles;
}

Matrix4f Skeleton::computeJointToParent(unsigned index) const
{
    const Joint& joint = joints_[index];

    Matrix4f to_parent = Matrix4f::Identity();
    to_parent.block(0, 3, 3, 1) = joint.position;     // set first three entries of last column

    // YOUR CODE HERE (R2)
    // Compute the "to_parent" matrix to match the joint's Euler
    // angles. Thanks to the line above, "to_parent" already
    // contains the correct transformation component in the
    // last column. Compute the rotation that corresponds to
    // the current Euler angles and replace the
    // upper 3x3 block of "to_parent" with the result.
    // You must account for the ordering of the rotations,
    // as determined by joint.euler_order -- see its comment
    // in the Joint struct in skeleton.h for details.
    // NOTE: You need to make sure you convert from degrees to
    // radians before using functions that (~always) expect them.
    // Hint: You can do Matrix3f(AngleAxis<float>(...))
    // three times in a row, once for each axis determined
    // by joint.euler_order, and multiply the results.

    return to_parent;
}

void Skeleton::incrJointRotation(unsigned index, const Vector3f& euler_angles)
{
    setJointRotation(index, getJointRotation(index) + euler_angles);
    updateToWorldTransforms();
}

void Skeleton::updateToWorldTransforms()
{
    // Here we initiate the hierarchical transformation from the root node (at index 0),
    // almost precisely as in the lecture slides on hierarchical modeling.
    // Instead of starting with an identity transformation, we use the object_to_world_
    // transformation that was constructed during loading to scale and center the
    // content appropriately for our viewer.
    updateToWorldTransforms(0, object_to_world_);
}

void Skeleton::updateToWorldTransforms(unsigned joint_index, const Matrix4f& parent_to_world)
{
    // YOUR CODE HERE (R1)
    // Update transforms for joint at joint_index and its children.

    Matrix4f jointToWorld = parent_to_world * computeJointToParent(joint_index);
    joints_[joint_index].joint_to_world = jointToWorld;
    
    for (int i = 0; i < joints_[joint_index].children.size(); ++i) {
        //cout << "i: " << i << "joint children: " << joint.children << endl;
        int child = joints_[joint_index].children[i];
        updateToWorldTransforms(child, jointToWorld);
    };
    
    
}

void Skeleton::computeToBindTransforms()
{
    assert(object_to_world_ == Matrix4f::Identity());
    updateToWorldTransforms();
    // YOUR CODE HERE (R4)
    // Given the current joint_to_world transforms for each bone,
    // compute the inverse bind pose transformations (as per the lecture slides),
    // and store the results in the member bind_to_joint of each joint.
    // When this is called, the object_to_world_ matrix is set to identity,
    // meaning that the object space and world space coincide,
    // which means that joint_to_world is, in fact, the mapping from
    // joint's spaces to the object space where the skin's bind pose vertex
    // coordinates are given.
}

void Skeleton::getToWorldTransforms(vector<Matrix4f>& dest)
{
    dest.clear();
    for (const auto& j : joints_)
        dest.push_back(j.joint_to_world);
}

const Matrix4f& Skeleton::getToWorldTransform(unsigned joint)
{
    return joints_[joint].joint_to_world;
}


void Skeleton::getSSDTransforms(vector<Matrix4f>& dest)
{
    // YOUR CODE HERE (R4)
    // Compute the relative transformations between the bind pose and current pose,
    // store the results in the "dest" vector. These are the transformations
    // passed into the actual skinning code. In the lecture slides' terms,
    // these are the T_i * inv(B_i) matrices. The identity that is pushed by the
    // starter code just makes sure the downstream rendering code work,
    // though obviously without animation.

    dest.clear();

    for (const auto& j : joints_)
        dest.push_back(Matrix4f::Identity());

}

void Skeleton::loadBVH(string skeleton_file, bool initial_pose_zeros)
{
    joints_.clear();
    joint_name_map_.clear();
    animation_frames_.clear();
    object_to_world_ = Matrix4f::Identity();
    animation_frame_time_ = 1.0f / 30.0f;
    animation_frame_ = 0.0f;


    ifstream in(skeleton_file);

    string s;
    string line;
    while (getline(in, line))
    {
        stringstream stream(line);
        stream >> s;
        
        if (s == "ROOT")
        {
            string jointName;
            stream >> jointName;
            loadJoint(in, -1, jointName);
        }
        else if (s == "MOTION")
        {
            loadAnim(in);
        }
    }

    // initially set to_parent matrices to identity
    for (auto j = 0u; j < joints_.size(); ++j)
        if (initial_pose_zeros)
            setJointRotation(j, Vector3f(0, 0, 0));
        else
            setJointRotation(j, animation_frames_[0].joint_angles[j]);

    // this needs to be done while skeleton is still in bind pose
    computeToBindTransforms();

    computeNormalizationWorldTransform();
}

void Skeleton::loadJoint(ifstream& in, int parent, string name)
{
    Joint j;
    j.name = name;
    j.parent = parent;

    bool pushed = false;

    int curIdx = -1;
    string s;
    string line;
    while (getline(in, line))
    {
        stringstream stream(line);
        stream >> s;
        if (s == "JOINT")
        {
            string jointName;
            stream >> jointName;
            loadJoint(in, curIdx, jointName);
        }
        else if (s == "End")
        {
            while (getline(in, line))
            {
                // Read End block so it doesn't get interpreted as a closing bracket or offset keyword
                stringstream end(line);
                end >> s;
                if (s == "}")
                    break;
            }
        }
        else if (s == "}")
        {
            return;
        }
        else if (s == "CHANNELS")
        {
            int channelCount;
            stream >> channelCount;
            if (channelCount == 6)
                stream >> s >> s >> s;

            Vector3i axis_order;
            for (int i = 0; i < 3; ++i)
            {
                stream >> s;
                if (s == "Xrotation")
                    axis_order[i] = 0;
                else if (s == "Yrotation")
                    axis_order[i] = 1;
                else if (s == "Zrotation")
                    axis_order[i] = 2;
            }
            joints_[curIdx].euler_order = axis_order;
            //axis_orderings_.push_back(axis_order);
        }
        else if (s == "OFFSET")
        {
            Vector3f pos;
            stream >> pos(0) >> pos(1) >> pos(2);
            j.position = pos;

            if (!pushed)
            {
                pushed = true;
                joints_.push_back(j);
                curIdx = int(joints_.size() - 1);

                if (parent != -1)
                    joints_[parent].children.push_back(curIdx);
            }
            joint_name_map_[name] = curIdx;
        }
    }
}

void Skeleton::loadAnim(ifstream& in)
{
    string word;
    in >> word;
    int frames;
    in >> frames;

    animation_frames_.resize(frames);

    int frameNum = 0;
    string line;

    do
    {
        getline(in, line);
        float spf;
        if (sscanf(line.c_str(), "Frame Time: %f", &spf) == 1)
        {
            animation_frame_time_ = spf;
            break;
        }
    } while (true);

    // Load animation angle and position data for each frame
    while (getline(in, line))
    {
        while (isspace(*line.rbegin()))
            line.pop_back();
        stringstream stream(line);
        stream >> animation_frames_[frameNum].position(0) >> animation_frames_[frameNum].position(1) >> animation_frames_[frameNum].position(2);
        while (stream.good())
        {
            float a1, a2, a3;
            stream >> a1 >> a2 >> a3;
            animation_frames_[frameNum].joint_angles.push_back(Vector3f(a1, a2, a3));
        }
        frameNum++;
    }
}


// Make sure the skeleton is of sensible size.
void Skeleton::computeNormalizationWorldTransform()
{
    updateToWorldTransforms();

    // compute bounding box of joint world positions
    const float fltmax = numeric_limits<float>::max();
    Array3f bbmin(fltmax, fltmax, fltmax); // Array is Eigen, too
    Array3f bbmax(-bbmin);
    for (auto& j : joints_)
    {
        auto wposarray = j.joint_to_world.block(0, 3, 3, 1).array();
        bbmin = bbmin.min(wposarray);
        bbmax = bbmax.max(wposarray);
    }

    float longest_axis = (bbmax - bbmin).maxCoeff();

    // Scale down by the appropriate power of ten, so that, e.g.,
    // longest axis on the order of 180 units will result in scale = 1/100, etc.
    float scale = float(pow(10.0, -floor(log10(longest_axis))));

    // scale and translate so that the minimum y coordinate is 0,
    // and the center of the bounding box is at x=0 and z=0
    object_to_world_ = Matrix4f{
        { scale,  0.0f,  0.0f, -0.5f * scale * (bbmin(0) + bbmax(0)) },
        {  0.0f, scale,  0.0f,                     -scale * bbmin(1) },
        {  0.0f,  0.0f, scale, -0.5f * scale * (bbmin(2) + bbmax(2)) },
        {  0.0f,  0.0f,  0.0f,                                  1.0f }
    };

    // This time using the just-set object_to_world_
    updateToWorldTransforms();
}

string Skeleton::getJointName(unsigned index) const
{
    return joints_[index].name;
}

const Vector3f& Skeleton::getJointRotation(unsigned index) const
{
    return joints_[index].euler_angles;
}

int Skeleton::getJointParent(unsigned index) const
{
    return joints_[index].parent;
}

int Skeleton::getJointIndex(string name)
{
    return joint_name_map_[name];
}

void Skeleton::setAnimationFrame(float AnimationFrame)
{
    assert(AnimationFrame >= 0.0f && AnimationFrame <= animation_frames_.size() - 1);
    animation_frame_ = AnimationFrame;
    setAnimationState();
}

void Skeleton::setAnimationState()
{
    // No actual animation exists.
    if (!animation_frames_.size()) {
        updateToWorldTransforms();
        return;
    }

    // Get the current position in the animation..
    int frame1 = int(floor(animation_frame_)) % animation_frames_.size();
    int frame2 = int(ceil(animation_frame_)) % animation_frames_.size();
    float w = animation_frame_ - frame1;
    assert(w >= 0 && w < 1.0f);
    const auto& frameData1 = animation_frames_[frame1];
    const auto& frameData2 = animation_frames_[frame2];
    // .. and set all joint rotations accordingly.
    for (int j = 0; j < frameData1.joint_angles.size(); ++j)
        setJointRotation(j, lerpEuler(frameData1.joint_angles[j], frameData2.joint_angles[j], w));

    // Also translate the root to the position given in the animation description.
    joints_[0].position = (1.0f - w) * frameData1.position + w * frameData2.position;
    updateToWorldTransforms(0, object_to_world_);
}
