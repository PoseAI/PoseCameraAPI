// Copyright 2023 Pose AI Ltd. All rights reserved

using UnityEngine;


namespace PoseAI
{
    /*
     * Add gameobject sockets to skeleton pieces and attach this component.
     * Set the bone and then PoseAICharacterAnimator can detect and use for things like extra penetration tests. 
     */
    
    public class PoseAIAnimationSocket : MonoBehaviour
    {
        [Tooltip("The BONE this socket is locally offset from")]
        public HumanBodyBones bone;

    }
}
