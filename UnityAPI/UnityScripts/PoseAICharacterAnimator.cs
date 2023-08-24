// Copyright 2021-2023 Pose AI Ltd. All rights reserved

using UnityEngine;
using System;
using System.Collections.Generic;


namespace PoseAI
{
    /*
     * Main animation component for Pose Camera.  Must be attached to a normal Animator component with both a valid Animator Controller (with IK Pass enabled on the active layer) and a Humanoid avatar.
     * Animation occurs on the IK pass and player input events are triggered on the Update stage.  If using full body Pose Camera animation, consider refining the root placement routine based on your rig specifics
     */

    [RequireComponent(typeof(Animator))]
    [RequireComponent(typeof(PoseAISource))]
    public class PoseAICharacterAnimator : MonoBehaviour
    {
        [Tooltip("Animation weighting for Pose Camera vs normal animation (0.0-1.0)")]
        public float AnimationAlpha = 1.0f;


        [Header("Facial rig")]
        [Tooltip("Facial mesh to apply blendshapes.")]
        public SkinnedMeshRenderer FaceMesh;

        [Tooltip("Eye mesh transform for applying gaze direction blendshapes.")]
        public Transform LeftEye;

        [Tooltip("Eye mesh transform for applying gaze direction blendshapes.")]
        public Transform RightEye;


        [Header("Motion control")]

        [Tooltip("Approx height of avatar.  Scales lateral/vertical movement.")]
        public float RigHeight = 1.8f;

        [Tooltip("Moves pelvis laterally/vertically based on player motion (if using full body animation)")]
        public Vector3 ScaleRootMotion = new(1.0f, 1.0f, 1.0f);

        [Tooltip("Whether to keep avatar in contact with ground")]
        public bool PinToFloor = true;


        [Header("Configuration")]
        [Tooltip("Whether to only use upper body rotations")]
        public bool UseUpperBodyOnly = true;

        [Tooltip("Use to override the default hip/pelvis if the rig has a different hip/pelvis than standard")]
        public string OverridePelvisName = "";

        [Tooltip("Use only if you have set up a custom joint-by-joint remapping array (for non-standard rig configurations)")]
        public string Remapping = "";

        [Tooltip("Use extra joints not included in humanoid avatar (like twist), if found on rig, in LateUpdate")]
        public bool UseExtraJoints = true;


        private Animator _animator;
        private PoseAISource _source;
        private Transform _pelvisJoint;
        private Vector3 _smoothedRootTranslation = Vector3.zero;
        private float idleAlpha = 1.0f;
        private List<Quaternion> _remapping = null;
        private List<PoseAIAnimationSocket> _penetrationSockets = new();
        private PoseAIRigBase _rigObj;
        private Vector3 _preIKPelvisLocalPosition;
        private Dictionary<int, Transform> _extraJoints = new();
        
        public void SetSource(PoseAISource source)
        {
            _source = source;
            if (_source != null)
            {
                _rigObj = source.GetRig();
                string rootName = OverridePelvisName != "" ? OverridePelvisName : _rigObj.GetRootName();
                _pelvisJoint = FindJointTransformRecursive(transform, rootName, skipToChildren: true, canBeEnding: false);
                if (_pelvisJoint == null)
                    Debug.LogWarning("Did not find rootjoint in " + transform.name + ".  Try overriding with correct name in editor.");
                else
                {
                    _penetrationSockets = new List<PoseAIAnimationSocket>(_pelvisJoint.GetComponentsInChildren<PoseAIAnimationSocket>());
                    
                    // find if any extra joints match with rig joints for supplementary animation.
                    foreach (KeyValuePair<int, string> entry in _rigObj.GetExtraBones())
                    {
                        _extraJoints.Add(entry.Key, FindJointTransformRecursive(_pelvisJoint, entry.Value, true));
                    }
                }
            }
        }

        public void SetRemapping(string remapping)
        {
            Remapping = remapping;
            if (Remapping != "")
            {
                if (PoseAIRigRetarget.Remappings.ContainsKey(Remapping))
                {
                    Debug.Log("Remapped " + _animator.name + " with " + Remapping);
                    _remapping = PoseAIRigRetarget.Remappings[Remapping];
                }
                else
                {
                    Debug.LogWarning("Could not find an existing remapping named " + Remapping);
                }
            }
            
        }


        private void Start()
        {
            _animator = GetComponent<Animator>();
            SetSource(GetComponent<PoseAISource>());
            if (_source == null)
                Debug.LogError("Missing source for PoseAI CharacterAnimator in " + transform.root.gameObject.name.ToString());
            SetRemapping(Remapping);
        }

        void OnAnimatorIK()
        {
            
            float useAlpha = AnimationAlpha * Mathf.Clamp(idleAlpha, 0.0f, 1.0f);
            if (useAlpha > 0.0f && CheckValidRig())
            {
                var bones = _rigObj.GetBones();
                _preIKPelvisLocalPosition = _animator.GetBoneTransform(bones[0]).transform.position - _animator.GetBoneTransform(bones[0]).transform.parent.transform.position;
                Tuple<List<bool>, List<Quaternion>> rotationTuple = _rigObj.GetRotationsCameraSpace();
                List<bool> validity = rotationTuple.Item1;
                List<Quaternion> rotations = rotationTuple.Item2;
                int startAt;

                if (UseUpperBodyOnly)
                {
                    startAt = 9;
                }
                else
                {
                    if (_remapping != null)
                    {
                        SetBone(bones[0], _rigObj.GetBaseRotation() * rotations[0] * _remapping[0], useAlpha);
                    }
                    else
                    {
                        SetBone(bones[0],_rigObj.GetBaseRotation() * rotations[0], useAlpha);
                    }
                    startAt = 1;
                }
                var parentIndices = _rigObj.GetParentIndices();
                for (int j = startAt; j < rotations.Count; ++j)
                {
                    var p = parentIndices[j];
                    var bone = bones[j];

                    if (bone != HumanBodyBones.LastBone && validity[j] && validity[p])
                    {
                        if (_remapping != null)
                        {
                            SetBone(bone, Quaternion.Inverse(rotations[p] * _remapping[p]) * rotations[j] * _remapping[j], useAlpha);
                        }
                        else
                        {
                            SetBone(bone, Quaternion.Inverse(rotations[p]) * rotations[j], useAlpha);
                        }
                    } 
                }
            }
           
        }

        void LateUpdate()
        {
            if (AnimationAlpha > 0.0f && !UseUpperBodyOnly && CheckValidRig())
                AdjustRootLocation();
            float useAlpha = AnimationAlpha * Mathf.Clamp(idleAlpha, 0.0f, 1.0f);
            if (UseExtraJoints && useAlpha > 0.0f && CheckValidRig())
                AnimateExtraJoints();
        }

        void AnimateExtraJoints()
        {
            Tuple<List<bool>, List<Quaternion>> rotationTuple = _rigObj.GetRotationsCameraSpace();
            List<bool> validity = rotationTuple.Item1;
            List<Quaternion> rotations = rotationTuple.Item2;
            var parentIndices = _rigObj.GetParentIndices();
            foreach (KeyValuePair<int, Transform> entry in _extraJoints)
            {

                var j = entry.Key;
                if (UseUpperBodyOnly && j < 9) // note 9 may not be the number of lower joints in a custom schedule so this may need to be altered
                    continue;
                var p = parentIndices[j];
                if (validity[j] && validity[p])
                {
                    var boneTransform = entry.Value;
                    if (_remapping != null)
                    {
                        boneTransform.localRotation = Quaternion.Slerp(boneTransform.localRotation, Quaternion.Inverse(rotations[p] * _remapping[p]) * rotations[j] * _remapping[j], AnimationAlpha);
                    }
                    else
                    {
                        boneTransform.localRotation = Quaternion.Slerp(boneTransform.localRotation, Quaternion.Inverse(rotations[p]) * rotations[j], AnimationAlpha);
                    }
                }
            }
        }

        void Update()
        {
            UpdateFaceMesh();   
        }

      
        private void UpdateFaceMesh()
        {
            if (FaceMesh != null && _rigObj != null)
            {
                List<float> blendshapes = PoseAIFaceBlendShapeReorder.ReorderBlendshapes(ref _rigObj.blendshapes);
                for (int f = 0; f < blendshapes.Count; f++)
                {
                    FaceMesh.SetBlendShapeWeight(f, blendshapes[f] * 100.0f); //scale by 100 as Unity uses 0-100 and we predict 0.0-1.0
                }
                float kScaleEye = 20.0f; // +- degrees for movement, multiplied by blenshape difference ranges from -1 to 1.

                float DownUpL = _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookDownLeft] -
                                _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookUpLeft];

                float DownUpR = _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookDownRight] -
                                 _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookUpRight];

                // averaging ensures eyes will move up/down together which is "normal" behavior. We still allow cross eyes (independent inout)
                float DownUp = 0.5f * (DownUpL + DownUpR); 

                if (LeftEye != null)
                {
                    float InOut = _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookInLeft] -
                                _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookOutLeft];
                    LeftEye.localRotation = Quaternion.Euler(DownUp * kScaleEye, InOut * kScaleEye, 0.0f);
                }
                if (RightEye != null)
                {
                    float InOut = _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookInRight] -
                                _rigObj.blendshapes[(int)PoseAIFaceBlendShapeReorder.PoseAIFaceBlendShapes.EyeLookOutRight];

                    RightEye.localRotation = Quaternion.Euler(DownUp * kScaleEye, -InOut * kScaleEye, 0.0f);
                }

            }
        }

        private bool CheckValidRig()
        {
            return (_source != null &&
                    _animator != null &&
                    _animator.runtimeAnimatorController != null &&
                    _rigObj != null &&
                    _pelvisJoint != null &&
                    _rigObj.HasRigInfo());
        }

        private void SetBone(HumanBodyBones bone, Quaternion target, float alpha)
        {
            if (alpha < 1.0f)
                target = Quaternion.Slerp(_animator.GetBoneTransform(bone).localRotation, target, alpha);
            _animator.SetBoneLocalRotation(bone, target);
        }


        /*
         * adjusts the root joint to ground the lowest joint and deal with lateral movement in the camera frame, but only knows about joint positions rather than mesh borders.
         * a more robust foot placing, as per IK rigs, would be better but will depend on rig setup and probably checking sockets on the bottom of the foot.
         * jumping should be handled via events.
        */
        private void AdjustRootLocation()
        {

            float[] hip = new float[3];
            _rigObj.GetBody().Vectors.Hip.CopyTo(hip);
            Vector3 targetTranslation = new(-hip[0] * ScaleRootMotion.x, hip[2] * ScaleRootMotion.y, hip[1] * ScaleRootMotion.z);
            targetTranslation = targetTranslation * RigHeight + _preIKPelvisLocalPosition;
            
            //makes sure nw part of the character goes below the parent of the pelvis (root or player object)
            float bone_penetration = GetBonePenetration();
            if (PinToFloor || bone_penetration > 0.0f)
                targetTranslation.y = bone_penetration;
            // could be used to additionally smooth translation, although our engine already does some
            float alpha = 1.0f; // Mathf.Clamp(Time.deltaTime * 10.0f, 0.1f, 1.0f);
            _smoothedRootTranslation =  Vector3.Lerp(_smoothedRootTranslation, targetTranslation, alpha);
            _pelvisJoint.localPosition += _smoothedRootTranslation;
           
        }

        /* calculates lowest Y relative to component space. */
        private float GetBonePenetration()
        {
            float minY = float.MaxValue;

            foreach (var bone in _rigObj.GetBones())
            {
                if (bone != HumanBodyBones.LastBone)
                {
                    var bt = _animator.GetBoneTransform(bone);
                    if (bt != null)
                    {
                        minY = Mathf.Min(minY, bt.position.y);
                    }
                }
            }
            // also checks any penetration sockets added to avatar for better foot placement
            foreach (var socket in _penetrationSockets)
            {
                var bt = _animator.GetBoneTransform(socket.bone);
                if (bt != null)
                {
                    minY = Mathf.Min(minY, bt.TransformPoint(socket.transform.localPosition).y);
                }
                    
            }

            float componentY = _pelvisJoint.parent.position.y;
            return componentY - minY;
        }

        private static Transform FindJointTransformRecursive(Transform parent, string name, bool skipToChildren = false, bool canBeEnding=true)
        {
            if (!skipToChildren && string.Equals(parent.name, name, StringComparison.InvariantCultureIgnoreCase))
            {
                return parent;
            }
            if (!skipToChildren && canBeEnding &&  parent.name.EndsWith(name, StringComparison.InvariantCultureIgnoreCase))
            {
                return parent;
            }

            foreach (Transform child in parent)
            {
                var result = FindJointTransformRecursive(child, name);
                if (result != null)
                    return result;
            }
            return null;
        }

    }
}
