// Copyright 2021 Pose AI Ltd. All rights reserved

using System;
using UnityEngine;

namespace PoseAI
{
    /*
     * A character component which can be used to supply a shared source to the 
     * PoseAICharacterController and PoseAIAnimator.  Create a direct source elsewhere
     * and then drag into this component.
     */
    public abstract class PoseAISource : MonoBehaviour
    {
        virtual public PoseAIRigBase GetRig()
        {
            return null;
        }
    }
    public class PoseAISourceShared : PoseAISource
    {
        [Tooltip("Specify a direct source created elsewhere.")]
        public PoseAISource Source;
        public override PoseAIRigBase GetRig()
        {
            return Source.GetRig();
        }
    }
    
}
