// Arcball camera by Eugene Hsu
// Based on 6.839 sample code for rotation code.
// Extended to handle translation (MIDDLE) and scale (RIGHT)

// -*-c++-*-
#ifndef CAMERA_H
#define CAMERA_H

class Camera
{
public:

    Camera();
       
    void SetDimensions(int w, int h);
    void SetViewport(int x, int y, int w, int h);
    void SetPerspective(float fovy);

    // Call from whatever UI toolkit
    void MouseClick(int button, int x, int y);
    void MouseDrag(int x, int y);
    void MouseRelease(int x, int y);

    // Apply viewport, perspective, and modeling
    Matrix4f GetProjection() const;
    Matrix4f GetWorldToView() const;

    // Set for relevant vars
    void SetCenter(const Vector3f& center);
    void SetRotation(const Matrix4f& rotation);
    void SetDistance(const float distance);

    // Get for relevant vars
    Vector3f GetCenter() const { return mCurrentCenter; }
    Matrix4f GetRotation() const { return mCurrentRot; }
    float GetDistance() const { return mCurrentDistance; }
    
private:

    // States 
    int     mDimensions[2] = { 0, 0 };
    int     mStartClick[2] = { 0, 0 };
    int     mButtonState = -1;

    // For rotation
    Matrix4f mStartRot = Matrix4f::Identity();
    Matrix4f mCurrentRot = Matrix4f::Identity();

    // For translation
    float   mPerspective[2] = { 0.0f, 0.0f };
    int     mViewport[4] = { 0, 0, 0, 0 };
    Vector3f mStartCenter = Vector3f{ 0.0f, 0.0f, 0.0f };
    Vector3f mCurrentCenter = Vector3f{ 0.0f, 0.0f, 0.0f };

    // For zoom
    float   mStartDistance = 0.5f;
    float   mCurrentDistance = 0.5f;

    void ArcBallRotation(int x, int y);
    void PlaneTranslation(int x, int y);
    void DistanceZoom(int x, int y);
};

#endif
