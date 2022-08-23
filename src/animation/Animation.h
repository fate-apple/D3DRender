#pragma once
#include <unordered_map>

#include "core/math.h"

struct SkinningWeights
{
    uint8 skinIndices[4];
    uint8 skinWeights[4];
};

struct SkeletonJoint
{
    std::string name;
    mat4 bindTransform;	  // Position of joint relative to model space.
    mat4 invBindTransform; // Transforms from model space to joint space.
    uint32 parentID;
};

struct AnimationJoint
{
    bool isAnimated = false;

    uint32 firstPositionKeyframe;
    uint32 numPositionKeyframes;

    uint32 firstRotationKeyframe;
    uint32 numRotationKeyframes;

    uint32 firstScaleKeyframe;
    uint32 numScaleKeyframes;
};

struct AnimationClip
{
    std::string name;
    fs::path filename;

    std::vector<float> positionTimestamps;
    std::vector<float> rotationTimestamps;
    std::vector<float> scaleTimestamps;

    std::vector<vec3> positionKeyframes;
    std::vector<quat> rotationKeyframes;
    std::vector<vec3> scaleKeyframes;

    std::vector<AnimationJoint> joints;

    AnimationJoint rootMotionJoint;
	
    float lengthInSeconds;
    bool looping = true;
    bool bakeRootRotationIntoPose = false;
    bool bakeRootXZTranslationIntoPose = false;
    bool bakeRootYTranslationIntoPose = false;


    void Edit();
    trs GetFirstRootTransform() const;
    trs GetLastRootTransform() const;
};

struct AnimationSkelon
{
    std::vector<SkeletonJoint> joints;
    std::unordered_map<std::string, uint32> nameToJointID;

    std::vector<AnimationClip> clips;
    std::vector<fs::path> files;
    
};