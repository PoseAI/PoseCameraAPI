// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;


namespace PoseAI
{
    [System.Serializable]
    public class PoseAIRigUnity : PoseAIRigBase
    {
        public PoseAIRigUnity() : base(new Quaternion(0.0f, 0.7071f, 0.7071f, 0.0f)){}
    
        [System.Serializable]
        public class BodyWrapper : BaseBodyWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> Hips;
                public List<float> Right_UpperLeg;
                public List<float> Right_LowerLeg;
                public List<float> Right_Foot;
                public List<float> Right_Toes;
                public List<float> Left_UpperLeg;
                public List<float> Left_LowerLeg;
                public List<float> Left_Foot;
                public List<float> Left_Toes;
                public List<float> Spine;
                public List<float> Chest;
                public List<float> UpperChest;
                public List<float> Neck;
                public List<float> Head;
                public List<float> Left_Shoulder;
                public List<float> Left_UpperArm;
                public List<float> Left_LowerArm;
                public List<float> Right_Shoulder;
                public List<float> Right_UpperArm;
                public List<float> Right_LowerArm;
                
            }
            public RotationsWrapper Rotations = new RotationsWrapper();
        }
        
        [System.Serializable]
        public class LeftHandWrapper : BaseHandWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> Left_Hand;
                public List<float> Left_LowerArm_Twist;
                public List<float> Left_IndexProximal;
                public List<float> Left_IndexIntermediate;
                public List<float> Left_IndexDistal;
                public List<float> Left_MiddleProximal;
                public List<float> Left_MiddleIntermediate;
                public List<float> Left_MiddleDistal;
                public List<float> Left_RingProximal;
                public List<float> Left_RingIntermediate;
                public List<float> Left_RingDistal;
                public List<float> Left_PinkyProximal;
                public List<float> Left_PinkyIntermediate;
                public List<float> Left_PinkyDistal;
                public List<float> Left_ThumbProximal;
                public List<float> Left_ThumbIntermediate;
                public List<float> Left_ThumbDistal;

            }
            public RotationsWrapper Rotations = new RotationsWrapper();
        }

        [System.Serializable]
        public class RightHandWrapper : BaseHandWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> Right_Hand;
                public List<float> Right_LowerArm_Twist;
                public List<float> Right_IndexProximal;
                public List<float> Right_IndexIntermediate;
                public List<float> Right_IndexDistal;
                public List<float> Right_MiddleProximal;
                public List<float> Right_MiddleIntermediate;
                public List<float> Right_MiddleDistal;
                public List<float> Right_RingProximal;
                public List<float> Right_RingIntermediate;
                public List<float> Right_RingDistal;
                public List<float> Right_PinkyProximal;
                public List<float> Right_PinkyIntermediate;
                public List<float> Right_PinkyDistal;
                public List<float> Right_ThumbProximal;
                public List<float> Right_ThumbIntermediate;
                public List<float> Right_ThumbDistal;

            }
            public RotationsWrapper Rotations = new RotationsWrapper();
        }
        
        public BodyWrapper Body = new BodyWrapper();
        public LeftHandWrapper LeftHand = new LeftHandWrapper();
        public RightHandWrapper RightHand = new RightHandWrapper();
        
        protected internal override void BuildListsFromIntrospection(){
            Body.Rotations.AddFields(rotationData, rotationNames);
            LeftHand.Rotations.AddFields(rotationData, rotationNames);
            RightHand.Rotations.AddFields(rotationData, rotationNames);
            numOfBodyJoints = Body.Rotations.NumOfJoints();
            numOfLeftHandJoints = LeftHand.Rotations.NumOfJoints();
            numOfRightHandJoints = RightHand.Rotations.NumOfJoints();
        }
        
        public override BaseBodyWrapper GetBody()
        {
            return Body;
        }

        public override BaseHandWrapper GetLeftHand()
        {
            return LeftHand;
        }

        public override BaseHandWrapper GetRightHand()
        {
            return RightHand;
        }
       
    }            
}
