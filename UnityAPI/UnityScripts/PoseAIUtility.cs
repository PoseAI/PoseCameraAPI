// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;

namespace PoseAI
{
    /// <summary>
    /// Component to load 
    /// </summary>
    public class PoseAIRuntimeLoader
    {
        // the path must be relative to a 'Resources' folder in the project structure.  If necessary move files into a Resources folder
        public static string DefaultAnimationControllerPath = "StarterAssets/ThirdPersonController/Character/Animations/StarterAssetsThirdPerson";

        // specify which type of rig you use
        public static PoseAI_Rigs rigType = PoseAI_Rigs.Unity;

        // specify the rig remapping here, if any, to make sure it is set for each new avatar
        public static string Remapping = "";

        
        /*
         * pass in 
         * 1. gameCharacter to be initialized with PoseAIComponents
         * 2. a configured avatar from a template character 
         * 3. either a new port for a new source, or an exsiting PoseAISource (i.e. if player is already connected and we are animating a new character or scene).
         * 4. optionally pass in a different animation controller if you want to use different animation controllers in different parts of the game
         */
        public static bool RuntimeLoad(GameObject gameCharacter, Avatar avatar, int port = 0, PoseAISourceNetwork poseAISourceIn = null, RuntimeAnimatorController animationController = null)
        {
            Debug.Assert(poseAISourceIn != null || port != 0);

            /* 
             * adds an animator component if it doesn't exist.  Then configures key required properties:  
            * 1. runtimeAnimatorController - this should be a controller with IK pass enabled and the state machine logic referred to in the PoseAICharacterController
            * 2. avatar - this should be a configured humanoid avatar with the correct mappings for the Mecanim system.  Difficult to create at Runtime, easier to pass in from a template character in the project.
            */
            Animator animator = gameCharacter.GetComponent<Animator>();
            if (animator == null)
                animator = gameCharacter.AddComponent<Animator>() as Animator;
            if (animationController == null)
            {
                animator.runtimeAnimatorController = Resources.Load(DefaultAnimationControllerPath) as RuntimeAnimatorController;
                if (animator.runtimeAnimatorController == null)
                {
                    Debug.LogWarning("Failed to load animator at " + DefaultAnimationControllerPath);
                    return false;
                }
            }
            else
            {
                animator.runtimeAnimatorController = animationController;
            }
            animator.avatar = avatar;

            /* 
             * adds an default character controller and configures the capsule.  may need to be tweaked to suit game and rig 
            */
            CharacterController characterController = gameCharacter.GetComponent<CharacterController>();
            if (characterController == null)
                characterController = gameCharacter.AddComponent<CharacterController>() as CharacterController;
            characterController.radius = 0.28f;
            characterController.height = 1.8f;
            characterController.center = new Vector3(0.0f, 0.93f, 0.0f);

            /* 
             * if an established Source is not passed to loader, this sets up a new poseAI source to listen on the specified port. 
            */
            PoseAISource poseAISource;
            if (poseAISourceIn != null)
            {
                PoseAISourceShared poseAISourceShared = gameCharacter.AddComponent<PoseAISourceShared>() as PoseAISourceShared;
                poseAISourceShared.Source = poseAISourceIn;
                poseAISource = poseAISourceShared;
            }
            else
            {
                PoseAISourceNetwork poseAISourceDirect = gameCharacter.GetComponent<PoseAISourceNetwork>();
                if (poseAISourceDirect == null)
                    poseAISourceDirect = gameCharacter.AddComponent<PoseAISourceNetwork>() as PoseAISourceNetwork;
                poseAISourceDirect.Mode = PoseAI_Modes.Room;
                poseAISourceDirect.RigType = rigType;
                poseAISourceDirect.Port = port;
                poseAISourceDirect.MirrorCamera = false;
                poseAISource = poseAISourceDirect;
            }

            /* 
             * if an established Source is not passed to loader, this sets up a new poseAI source to listen on the specified port. 
            */
            PoseAICharacterAnimator poseAICharacterAnimator = gameCharacter.GetComponent<PoseAICharacterAnimator>();
            if (poseAICharacterAnimator == null)
                poseAICharacterAnimator = gameCharacter.AddComponent<PoseAICharacterAnimator>() as PoseAICharacterAnimator;
            poseAICharacterAnimator.SetSource(poseAISource);
            poseAICharacterAnimator.SetRemapping(Remapping);
            poseAICharacterAnimator.OverridePelvisName = "";

            PoseAICharacterController poseAICharacterController = gameCharacter.GetComponent<PoseAICharacterController>();
            if (poseAICharacterController == null)
                poseAICharacterController = gameCharacter.AddComponent<PoseAICharacterController>() as PoseAICharacterController;
            poseAICharacterController.SetSource(poseAISource);
            poseAICharacterController.CinemachineCameraTarget = GameObject.Find("PlayerFollowCamera");

            if (animator == null)
            {
                Debug.LogWarning("Failed to load animator");
                return false;
            }
            if (animator.avatar == null)
            {
                Debug.LogWarning("Failed to load animator's avatar");
                return false;
            }
            if (characterController == null)
            {
                Debug.LogWarning("Failed to load character controller");
                return false;
            }
            if (poseAISource == null)
            {
                Debug.LogWarning("Failed to load source");
                return false;
            }
            if (poseAICharacterAnimator == null)
            {
                Debug.LogWarning("Failed to load poseaicharacteranimator");
                return false;
            }
            if (poseAICharacterController == null)
            {
                Debug.LogWarning("Failed to load poseaicharactercontroller");
                return false;
            }
            return true;
        }


        public void EnableOrDisablePoseAI(GameObject gameCharacter, bool enabled)
        {
            PoseAISourceNetwork poseAISource = gameCharacter.GetComponent<PoseAISourceNetwork>();
            if (poseAISource != null)
                poseAISource.enabled = enabled;

            PoseAICharacterAnimator poseAICharacterAnimator = gameCharacter.GetComponent<PoseAICharacterAnimator>();
            if (poseAICharacterAnimator != null)
                poseAICharacterAnimator.enabled = enabled;

            PoseAICharacterController poseAICharacterController = gameCharacter.GetComponent<PoseAICharacterController>();
            if (poseAICharacterController != null)
                poseAICharacterController.enabled = enabled;

        }


    }



    public static class PoseAI_Decoder
    {
        static readonly uint[] reverse_map = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
        static readonly float[] firstByte = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9384465070835368f, 0.9697117733268197f, 0.9384465070835368f, 0.9384465070835368f, 0.9697117733268197f, 0.6257938446507083f, 0.6570591108939912f, 0.688324377137274f, 0.7195896433805569f, 0.7508549096238397f, 0.7821201758671226f, 0.8133854421104054f, 0.8446507083536883f, 0.8759159745969711f, 0.907181240840254f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -0.9687347337567171f, -0.9374694675134343f, -0.9062042012701514f, -0.8749389350268686f, -0.8436736687835857f, -0.8124084025403029f, -0.78114313629702f, -0.7498778700537372f, -0.7186126038104543f, -0.6873473375671715f, -0.6560820713238886f, -0.6248168050806058f, -0.5935515388373229f, -0.5622862725940401f, -0.5310210063507572f, -0.49975574010747437f, -0.4684904738641915f, -0.43722520762090866f, -0.4059599413776258f, -0.37469467513434296f, -0.3434294088910601f, -0.31216414264777725f, -0.2808988764044944f, -0.24963361016121155f, -0.2183683439179287f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9697117733268197f, 0.0f, -0.18710307767464585f, -0.155837811431363f, -0.12457254518808014f, -0.09330727894479729f, -0.06204201270151444f, -0.030776746458231585f, 0.0004885197850512668f, 0.03175378602833412f, 0.06301905227161697f, 0.09428431851489982f, 0.12554958475818268f, 0.15681485100146553f, 0.18808011724474838f, 0.21934538348803123f, 0.2506106497313141f, 0.28187591597459694f, 0.3131411822178798f, 0.34440644846116264f, 0.3756717147044455f, 0.40693698094772834f, 0.4382022471910112f, 0.46946751343429405f, 0.5007327796775769f, 0.5319980459208598f, 0.5632633121641426f, 0.5945285784074255f };
        static readonly float[] secondByte = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.030288226673180263f, 0.030776746458231558f, 0.030288226673180263f, 0.030288226673180263f, 0.030776746458231558f, 0.025403028822667317f, 0.025891548607718612f, 0.026380068392769906f, 0.0268685881778212f, 0.027357107962872496f, 0.02784562774792379f, 0.028334147532975085f, 0.02882266731802638f, 0.029311187103077674f, 0.02979970688812897f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0004885197850512946f, 0.0009770395701025891f, 0.0014655593551538837f, 0.0019540791402051783f, 0.002442598925256473f, 0.0029311187103077674f, 0.003419638495359062f, 0.0039081582804103565f, 0.004396678065461651f, 0.004885197850512946f, 0.00537371763556424f, 0.005862237420615535f, 0.006350757205666829f, 0.006839276990718124f, 0.0073277967757694185f, 0.007816316560820713f, 0.008304836345872008f, 0.008793356130923302f, 0.009281875915974597f, 0.009770395701025891f, 0.010258915486077186f, 0.01074743527112848f, 0.011235955056179775f, 0.01172447484123107f, 0.012212994626282364f, 0.0f, 0.0f, 0.0f, 0.0f, 0.030776746458231558f, 0.0f, 0.012701514411333659f, 0.013190034196384953f, 0.013678553981436248f, 0.014167073766487542f, 0.014655593551538837f, 0.015144113336590131f, 0.015632633121641426f, 0.01612115290669272f, 0.016609672691744015f, 0.01709819247679531f, 0.017586712261846604f, 0.0180752320468979f, 0.018563751831949193f, 0.019052271617000488f, 0.019540791402051783f, 0.020029311187103077f, 0.02051783097215437f, 0.021006350757205666f, 0.02149487054225696f, 0.021983390327308255f, 0.02247191011235955f, 0.022960429897410845f, 0.02344894968246214f, 0.023937469467513434f, 0.024425989252564728f, 0.024914509037616023f };
        static int CharToInt(char c)
        {
            return (int)(c);
        }
        
        public static uint UintB64ToUint(char a, char b)
        {
            return reverse_map[CharToInt(a)] * 64 + reverse_map[CharToInt(b)];
        }
        public static uint UintB64ToUint(char a, char b, char c)
        {
            return reverse_map[CharToInt(a)] * 4096 + reverse_map[CharToInt(b)] * 64 + reverse_map[CharToInt(c)];
        }

        public static float FixedB64pairToFloat(char a, char b)
        {
         
            return firstByte[CharToInt(a)] + secondByte[CharToInt(b)];
        }

        public static void FStringFixed12ToFloatInPlace(ref string data, ref List<float> flatArray)
        {
            for (int i = 0; i < data.Length - 1; i += 2)
                flatArray[i/2] = FixedB64pairToFloat(data[i], data[i + 1]);
        }
        public static void FStringFixed12ToFloat(ref string data, ref List<float> flatArray, bool clearFirst=false)
        {
            if (clearFirst)
                flatArray.Clear();
            flatArray.Capacity = flatArray.Count + data.Length / 2;
            for (int i = 0; i < data.Length - 1; i += 2)
                flatArray.Add(FixedB64pairToFloat(data[i], data[i + 1]));
        }
        public static void FStringFixed12ToFloat(string data, ref List<float> flatArray, float scale=1.0f, bool clearFirst = false)
        {
            if (clearFirst)
                flatArray.Clear();
            flatArray.Capacity = flatArray.Count + data.Length / 2;
            for (int i = 0; i < data.Length - 1; i += 2)
                flatArray.Add(FixedB64pairToFloat(data[i], data[i + 1]) * scale);
        }
        public static void FStringFixed12ToFloat(ref string data, ref List<float?> flatArray)
        {
            flatArray.Capacity = flatArray.Count + data.Length / 2;
            for (int i = 0; i < data.Length - 1; i += 2)
            {
                if(data[i]=='=' && data[i+1]=='=')
                    flatArray.Add(null);
                else
                    flatArray.Add(FixedB64pairToFloat(data[i], data[i + 1]));
            }
                
        }

        public static void FlatArrayToQuats(ref List<float> flatArray, ref List<Quaternion> quatArray)
        {
            quatArray.Capacity = quatArray.Count + flatArray.Count / 4;
            for (int i = 0; i < flatArray.Count - 3; i += 4)
            {
                //reordered Quats for Unity coordinate system.
                var quat = new Quaternion(flatArray[i + 1], -flatArray[i], flatArray[i + 3], -flatArray[i + 2]);
                quat.Normalize();
                quatArray.Add(quat);
            }
                
        }

        public static Vector3 FStringFixed12ToVector3(ref string data)
        {
            if (data.Length < 6)
                return Vector3.zero;
            else
                return new Vector3(FixedB64pairToFloat(data[0], data[1]), FixedB64pairToFloat(data[2], data[3]), FixedB64pairToFloat(data[4], data[5]));
            
        }
    }

    public class UITouch
    {
        public int index = 0;
        public float x = 0.0f;
        public float y = 0.0f;
        public TouchState state = TouchState.Ended;

        public override string ToString()
        {
            return "Touch_" + index + ":[" + x.ToString("0.000")  + "," + y.ToString("0.000") + "] " + state;
        }


        public static void DecodeMultiTouchString(ref string data, ref Queue<UITouch> touchQueue)
        {
            for (int i = 0; i < data.Length - 4; i += 5)
            {
                UITouch touch = new UITouch() { index = data[i] - '0'};
                if (data[i + 1] == '=' && data[i + 2] == '=')
                    touch.state = TouchState.Ended;
                else if (data[i + 1] == '=' && data[i + 2] == '0')
                    touch.state = TouchState.Cancelled;
                else
                {
                    touch.x = PoseAI_Decoder.FixedB64pairToFloat(data[i + 1], data[i + 2]);
                    touch.y = PoseAI_Decoder.FixedB64pairToFloat(data[i + 3], data[i + 4]);
                    touch.state = TouchState.Touching;
                }
                touchQueue.Enqueue(touch);
            }
                
        }

        public static void DecodeMultiTouchStateString(ref string data, ref List<UITouch> touchArray)
        {            
            for (int i = 0; i < data.Length - 3; i += 4)
            {
                int idx = i / 4;
                touchArray[idx].index = idx;
                if (data[i] == '=' && data[i + 1] == '=')
                    touchArray[idx].state = TouchState.Ended;
                else if (data[i + 1] == '=' && data[i + 2] == '0')
                    touchArray[idx].state = TouchState.Cancelled;
                else
                {
                    touchArray[idx].x = PoseAI_Decoder.FixedB64pairToFloat(data[i], data[i + 1]);
                    touchArray[idx].y = PoseAI_Decoder.FixedB64pairToFloat(data[i + 2], data[i + 3]);
                    if (touchArray[idx].state == TouchState.Touching || touchArray[idx].state == TouchState.Begun)
                         touchArray[idx].state = TouchState.Touching;
                    else
                        touchArray[idx].state = TouchState.Begun;
                }
            }

        }

        public static List<UITouch> MakeList(int number) {
            var retValue = new List<UITouch>(number);
            for (int i = 0; i < number; ++i)
                retValue.Add(new UITouch() { index = i }) ;
            return retValue;
        }
    }

    
}

