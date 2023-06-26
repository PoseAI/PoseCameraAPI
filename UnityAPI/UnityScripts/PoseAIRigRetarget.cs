// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System;
using System.Collections.Generic;
using System.Reflection;


namespace PoseAI
{
    /*
     * This class holds remappings between rigs allowing retargetting for the PoseAI animation controller to a rig with arbitrary joint rotation configuration.   The class also
     * acts as a utility component to calculate the required retargetting between two rigs, and you can cut and paste the result into the definition below.  See our documentation for a walkthrough.
     */


    [RequireComponent(typeof(Animator))]
    public class PoseAIRigRetarget : MonoBehaviour
    {

        [Tooltip("Character animator from a standard rig.")]
        public Animator OtherAnimator;

        /* dictionary where developer stores joint remappings between rigs.  Use our utility described above to generate remappings. Here we demonstrate the remapping which allows the Unity setting in PoseAI Source to animate a standard Mixamo rig*/
        public static Dictionary<String, List<Quaternion>> Remappings = new Dictionary<String, List<Quaternion>> {
            {"Unity_to_Mixamo", new List<Quaternion> {new Quaternion(-0.001313163f,0.003519776f,0.001896139f,0.9999911f),new Quaternion(-0.03058119f,0.02195603f,0.999233f,-0.01078969f),new Quaternion(0.006050894f,-0.04297421f,0.9987512f,-0.02475512f),new Quaternion(-0.03191573f,0.455055f,0.8898568f,-0.007873511f),new Quaternion(0.0190341f,3.207369E-05f,0.999809f,-0.004487503f),new Quaternion(-0.0116777f,0.9980259f,-0.03762786f,0.04890846f),new Quaternion(-0.02769065f,0.9976774f,0.05977682f,-0.01732024f),new Quaternion(-0.00800541f,0.8915043f,-0.4518645f,0.03122204f),new Quaternion(-0.005037684f,0.9998137f,-4.02392E-05f,-0.01864369f),new Quaternion(-0.0523979f,0.003514861f,0.002123816f,0.9986179f),new Quaternion(-0.05237108f,0.003684888f,0.002468993f,0.9986181f),new Quaternion(-0.0523428f,0.003730215f,0.002645796f,0.998619f),new Quaternion(-0.06188715f,0.005846756f,0.001564328f,0.9980652f),new Quaternion(-0.001090677f,0.005567047f,0.002427951f,0.9999811f),new Quaternion(0.494374f,-0.4292746f,0.4316889f,0.6204538f),new Quaternion(-0.001135274f,-0.7353337f,-0.03168532f,0.6769634f),new Quaternion(-0.001315836f,-0.5905049f,-0.004879781f,0.8070188f),new Quaternion(-0.610872f,-0.4401043f,-0.4461678f,0.4838167f),new Quaternion(-0.7462658f,0.02401248f,-0.6651214f,-0.01118335f),new Quaternion(-0.8200869f,0.001748288f,-0.571965f,-0.0176588f),new Quaternion(-9.905143E-05f,-0.4663447f,-0.01526002f,0.8844718f),new Quaternion(0f,0f,0f,1f),new Quaternion(-0.01935682f,0.2739335f,-0.0417899f,0.960646f),new Quaternion(-0.03425756f,0.235438f,-0.00310079f,0.9712811f),new Quaternion(-0.06960159f,0.255824f,-0.007471299f,0.9641864f),new Quaternion(-0.01386514f,0.2004483f,-0.04546332f,0.9785514f),new Quaternion(-0.0256219f,0.1685862f,0.008595063f,0.9853171f),new Quaternion(-0.05482729f,0.1904371f,0.01253534f,0.9800877f),new Quaternion(0.002182333f,0.1251515f,-0.0551515f,0.9906018f),new Quaternion(-0.01264111f,0.09459478f,-0.002788985f,0.9954323f),new Quaternion(-0.04733385f,0.1190308f,0.01173992f,0.9916927f),new Quaternion(0.01066293f,0.05440548f,-0.05295889f,0.9970571f),new Quaternion(-0.01226323f,0.02860736f,-0.001516336f,0.9995151f),new Quaternion(-0.04626933f,0.04963236f,0.01244027f,0.9976185f),new Quaternion(-0.09742624f,-0.8154725f,-0.05910675f,-0.5674686f),new Quaternion(-0.004401662f,-0.8281091f,0.05486592f,-0.5578593f),new Quaternion(0.0661611f,-0.8386174f,0.07426596f,-0.5355641f),new Quaternion(-0.8888749f,0.01770193f,-0.4575714f,-0.01475984f),new Quaternion(0f,0f,0f,1f),new Quaternion(-0.9547191f,0.03582855f,0.2942503f,-0.02540832f),new Quaternion(-0.9675199f,0.01458453f,0.2482254f,-0.0455863f),new Quaternion(-0.9597897f,0.01755816f,0.2683034f,-0.08068816f),new Quaternion(-0.9739594f,0.0413457f,0.2219637f,-0.02066063f),new Quaternion(-0.9832075f,0.004061457f,0.1787396f,-0.03660329f),new Quaternion(-0.9779422f,-0.000444205f,0.1981785f,-0.06599379f),new Quaternion(-0.9884639f,0.05028569f,0.1428068f,-0.004229109f),new Quaternion(-0.9942132f,0.01508357f,0.1041002f,-0.02185236f),new Quaternion(-0.9908893f,0.001868558f,0.1219598f,-0.05711672f),new Quaternion(-0.9957547f,0.04906259f,0.07780629f,0.003593687f),new Quaternion(-0.9987341f,0.01390179f,0.04341658f,-0.02129835f),new Quaternion(-0.9966506f,0.002476632f,0.05972444f,-0.05582171f),new Quaternion(0.5606988f,0.06244955f,-0.8216972f,-0.08082215f),new Quaternion(0.5474771f,-0.02610496f,-0.8358694f,-0.03017553f),new Quaternion(0.5230319f,-0.0675695f,-0.8462015f,0.07626485f),}},
        };

        private int runOnce = 5;

        /* Utility which generates the remapping entry between two rigs after the first frame*/
        private void LateUpdate(){
            
            if (runOnce==0)
            {
                Animator animator = GetComponent<Animator>();
                List<Quaternion> rotations = new List<Quaternion>();
                String mapName = OtherAnimator.name + "_to_" + animator.name;

                String pasteText = " {\"" + mapName + "\", new List<Quaternion> {";
                foreach (var bone in (new PoseAIRigUnity()).GetBones())
                {
                    Quaternion relativeRotation;
                    if (bone == HumanBodyBones.LastBone)
                    {
                        relativeRotation = Quaternion.identity;
                    }
                    else if (animator.GetBoneTransform(bone) != null && OtherAnimator.GetBoneTransform(bone) != null)
                    {
                        var myRotation = animator.GetBoneTransform(bone).rotation;
                        var otherRotation = OtherAnimator.GetBoneTransform(bone).rotation;
                        relativeRotation = Quaternion.Inverse(otherRotation) * myRotation;
                    } else
                    {
                        Debug.LogWarning("Missing " + bone.ToString());
                        relativeRotation = rotations[rotations.Count-1];
                    }

                    rotations.Add(relativeRotation);
                    pasteText += "new Quaternion(" + relativeRotation.x + "f," + relativeRotation.y + "f," + relativeRotation.z + "f," + relativeRotation.w + "f),";
                }
                pasteText += "}},";
                print(pasteText);
                
            }
            runOnce -= 1;
        }
               
  

      
    }
}
