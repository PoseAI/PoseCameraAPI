// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System;
using System.Collections.Generic;
using System.Reflection;


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

        [Tooltip("Whether to only use upper body rotations")]
        public bool UseUpperBodyOnly = true;
        
        [Tooltip("Moves root for sideways movement in camera frame, if using full body animation")]
        public bool MoveRootSideways = false;

        [Tooltip("Approx height of rig from ankle to head.  Scales lateral/vertical movement.")]
        public float RigHeight = 1.6f;
        
        [Tooltip("Use only if rig has a prefix before all bone names (i.e. mixamorig1: if the root is at mixamorig1:Hips)")]
        public string JointNamePrefix = "";

        [Tooltip("Use to override the default rootname if the rig has a different root than standard")]
        public string OverrideRootName = "";

        [Tooltip("Use only if you have set up a custom joint-by-joint remapping array (for non-standard rig configurations)")]
        public string Remapping = "";

        private Animator _animator;
        private PoseAISource _source;
        private Transform _rootJoint;
        private Vector3 _smoothedRootTranslation = Vector3.zero; 
        private float idleAlpha = 1.0f;
        private List<String> _jointNames = new List<String>();
        private List<Quaternion> _remapping = null;

        private PoseAIRigBase _rigObj;

        public void SetSource(PoseAISource source)
        {
            _source = source;
            if (_source != null)
            {
                _rigObj = source.GetRig();
                string rootName = OverrideRootName != "" ? OverrideRootName : JointNamePrefix + _rigObj.GetRootName()  ;
                _rootJoint = FindJointTransformRecursive(transform, rootName, skipToChildren: true);
                if (_rootJoint == null)
                    Debug.Log("Did not find rootjoint in " + transform.name + ".  Try overriding with correct name in editor.");
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
                    Debug.Log("Could not find an existing remapping named " + Remapping);
                }
            }
        }

        private void Start(){
            _animator = GetComponent<Animator>();
            SetSource(GetComponent<PoseAISource>());
            if (_source == null)
                Debug.Log("Missing source for PoseAI CharacterAnimator in "  + transform.root.gameObject.name.ToString());
            SetRemapping(Remapping);
        }
               
        void OnAnimatorIK()
        {
            float useAlpha = AnimationAlpha * Mathf.Clamp(idleAlpha, 0.0f, 1.0f);
            if (useAlpha > 0.0f && CheckValidRig())
            {
                var bones = PoseAIRigBase.bones;
                Tuple<List<bool>, List<Quaternion>> rotationTuple = _rigObj.GetRotationsCameraSpace();
                List<bool> validity = rotationTuple.Item1;
                List<Quaternion> rotations = rotationTuple.Item2;
                int startAt;

                if (UseUpperBodyOnly)
                {
                     startAt = 9;
                } else
                {
                    if (_remapping != null)
                    {
                        SetBone(bones[0], _rigObj.GetBaseRotation() * rotations[0] * _remapping[0], useAlpha);
                    }
                    else
                    {
                        SetBone(bones[0], _rigObj.GetBaseRotation() * rotations[0], useAlpha);
                    }
                    startAt = 1;
                }
               
                for (int j = startAt; j < rotations.Count; ++j)
                {
                    var p = PoseAIRigBase.parentIndices[j];
                    var bone = bones[j];
                    if (bone != HumanBodyBones.LastBone && validity[j] && validity[p])
                    {
                        if (_remapping != null)
                        {
                            SetBone(bone, Quaternion.Inverse(rotations[p] * _remapping[p]) * rotations[j] * _remapping[j], useAlpha);
                        } else
                        {
                            SetBone(bone, Quaternion.Inverse(rotations[p]) * rotations[j], useAlpha);
                        }
                        
                    } 
                }
            } 
        }

        void LateUpdate()
        {
            if (AnimationAlpha > 0.0f & !UseUpperBodyOnly && CheckValidRig())
                AdjustRootLocation();
        }

        void Update()
        {
            if (CheckValidRig())
            {
                // smoothly transition to (over 1 second) and away (over 2 seconds) from pose cam animations if torso is not visible.  Clamp above 1 to provide a few milliseconds before fade begins  
                idleAlpha = Mathf.Clamp(idleAlpha + Time.deltaTime * ((_rigObj.visibility.isTorso) ? 1.0f : -0.5f ), 0.0f, 1.1f);
            }
        }

        private bool CheckValidRig()
        {
            return (_source != null && 
                    _animator != null &&
                    _animator.runtimeAnimatorController != null &&
                    _rigObj != null &&
                    _rootJoint != null &&
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
            var scaBody = _rigObj.GetBody().Scalars;
            var vecBody = _rigObj.GetBody().Vectors;
            var vis = _rigObj.visibility;

            var targetTranslation = new Vector3(0.0f, GetBonePenetration(), 0.0f);
            if (vis.isTorso && scaBody.BodyHeight > 0.0f)
                targetTranslation.x = vecBody.HipScreen[0] * RigHeight / scaBody.BodyHeight * AnimationAlpha * idleAlpha;

            // smooths translation
            float alpha = Mathf.Clamp(Time.deltaTime * 4.0f, 0.1f, 1.0f);
            _smoothedRootTranslation = Vector3.Lerp(_smoothedRootTranslation, targetTranslation, alpha);
            _rootJoint.localPosition += _smoothedRootTranslation;
        }

        /* calculates lowest Y relative to component space. */
        private float GetBonePenetration()
        {
            float minY = float.MaxValue;
            MethodInfo methodGetAxisLength = typeof(Avatar).GetMethod("GetAxisLength", BindingFlags.Instance | BindingFlags.NonPublic);
            if (methodGetAxisLength != null)
            {
                float ltoes = (float)methodGetAxisLength.Invoke(_animator.avatar, new object[] { HumanBodyBones.LeftToes });
                float rtoes = (float)methodGetAxisLength.Invoke(_animator.avatar, new object[] { HumanBodyBones.RightToes });
                var lt = _animator.GetBoneTransform(HumanBodyBones.LeftToes);
                var rt = _animator.GetBoneTransform(HumanBodyBones.RightToes);

                minY = Mathf.Min(
                    lt.position.y - (lt.rotation * new Vector3(0, ltoes, 0)).y,
                    rt.position.y - (rt.rotation * new Vector3(0, rtoes, 0)).y
                );
            }

            foreach (var bone in PoseAIRigBase.bones)
            {
                if (bone != HumanBodyBones.LastBone)
                {
                    minY = Mathf.Min(minY, _animator.GetBoneTransform(bone).position.y);
                }
            }
            float componentY =  _rootJoint.parent.position.y;
            return componentY - minY;
        }

        private static Transform FindJointTransformRecursive(Transform parent, string name, bool skipToChildren = false)
        {
            if (!skipToChildren && string.Equals(parent.name, name, StringComparison.InvariantCultureIgnoreCase))
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
