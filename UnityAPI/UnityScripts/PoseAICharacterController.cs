// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;


/* 
 */

namespace PoseAI
{
	[RequireComponent(typeof(CharacterController))]
	[RequireComponent(typeof(PoseAISource))]
	public class PoseAICharacterController : MonoBehaviour
	{
		[Header("Player")]
		[Tooltip("Custom gravity to change dynamics if desired")]
		public float Gravity = -9.81f;

		//***********************  Turning controls  *********************/

		[Header("Turning controls")]
		[Tooltip("Scales character turning rate from the player leaning to side. Use 0.0 to disable")]
		[Range(0.0f, 8.0f)]
		public float LeanToTurnScale = 4.0f;
		[Tooltip("How much the player needs to lean before turning begins (degrees)")]
		[Range(0.0f, 10.0f)]
		public float LeanToTurnDeadzone = 5f;
		[Tooltip("Scales character turning rate from the player twisting to side. Use 0.0 to disable")]
		[Range(0.0f, 4.0f)]
		public float TwistToTurnScale = 2.0f;
		[Tooltip("How much the player needs to twist before turning begins (degrees)")]
		[Range(0.0f, 20.0f)]
		public float TwistToTurnDeadzone = 10.0f;

		[Space(10)]
		//***********************  Ground motion  *********************/
		[Header("Ground motion controls")]
		[Tooltip("Allow arm pump motion to also trigger walking/running.")]
		public bool CanArmPumpsTriggerWalking = true;
		[Tooltip("If true, vary speed linearily with speed of player input. If false, use walk/sprint speeds depending on input threshold.")]
		public bool UseAnalogGroundSpeed = false;
		[Tooltip("Sprint speed of the character in m/s")]
		public float SprintSpeed = 5.335f;
		[Tooltip("Walk speed of the character in m/s")]
		public float WalkSpeed = 2.62f;
		[Tooltip("Required speed to trigger walking")]
		public float WalkAtStepsPerSecond = 1.0f;
		[Tooltip("an OR required speed to trigger sprint")]
		public float SprintAtStepsPerSecond = 4.0f;
		[Tooltip("an OR Required speed to trigger sprint, in approx body height / second units")]
		public float SprintAtDistancePerSecond = 0.6f;

		[Tooltip("Scalar for analog input speed")]
		public float ForwardSpeedScale = 8.0f;
		[Tooltip("Scaling player lateral movement")]
		public float PlayerStrafeScale = 3.0f;
		[Tooltip("Upper limit for groundspeed")]
		public float MaxRunSpeed = 7.0f;
		[Tooltip("Acceleration and deceleration")]
		public float GroundSpeedChangeRate = 10.0f;
		
		[Space(10)]
		
		[Tooltip("The height the player can jump")]
		public float JumpHeight = 1.2f;
		[Tooltip("Time required to pass before being able to jump again. Set to 0f to instantly jump again")]
		public float JumpTimeout = 0.0f;
		[Tooltip("Time required to pass before entering the fall state. Useful for walking down stairs")]
		public float FallTimeout = 0.15f;

		[Space(10)]

		[Header("Player Grounded")]
		[Tooltip("If the character is grounded or not. Not part of the CharacterController built in grounded check")]
		public bool IsGrounded = true;
		[Tooltip("Useful for rough ground")]
		public float GroundedOffset = -0.14f;
		[Tooltip("The radius of the grounded check. Should match the radius of the CharacterController")]
		public float GroundedRadius = 0.28f;
		[Tooltip("What layers the character uses as ground")]
		public LayerMask GroundLayers = 1;

		[Space(10)]
		//***********************  Flight  *********************/
		[Header("Player Flight")]
		[Tooltip("Flapping arms up and down trigger flight")]
		public bool FlyWithArmFlaps = true;
		[Tooltip("Chest expansions trigger flight")]
		public bool FlyWithChestExpansions = false;
		[Tooltip("How quickly forward movement is slowed while airborn.  in m/s/s")]
		public float WindResistance = -5.0f;
		[Tooltip("How long glide is set after a fly up event")]
		public float FlyAutoGlideTime = 0.75f;
		[Tooltip("The height gained from one flap of the wings ")]
		public float FlapHeight = 1.5f;
		[Tooltip("The height gained from the rocket up action")]
		public float RocketUpHeight = 6.0f;
		[Tooltip("How fast the player falls when gliding")]
		public float GlidingFallVelocity = -0.4f;
		[Tooltip("Forward speed of glide, ignores wind resistance")]
		public float GlidingFwdVelocity = 5.5f;
		[Tooltip("Acceleration and deceleration into forward glide speed")]
		public float GlideSpeedChangeRate = 1.0f;
		public bool IsGliding = false;

		[Space(10)]
		//***********************  Camera  *********************/
		[Header("Cinemachine")]
		[Tooltip("The follow target set in the Cinemachine Virtual Camera that the camera will follow")]
		public GameObject CinemachineCameraTarget;
		[Tooltip("While airborn camera moves to this angle (degrees)")]
		public float AerialTargetPitch = 40.0f;
		[Tooltip("Additional degress to override the camera. Useful for fine tuning camera position when locked")]
		public float CameraAngleOverride = 0.0f;


		// cinemachine
		private float _cinemachineTargetYaw;
		private float _cinemachineTargetPitch = 0f;
		private float _cinemachineCurrentPitch = 0f;

		//PoseCam
		protected PoseAISource _source;
		protected PoseAIRigBase _rigObj;
		private StepCounter _footsteps = new StepCounter();
		private StepCounter _sidesteps = new StepCounter();
		private StepCounter _armpumps = new StepCounter();
		private float _currentLean = 0.0f;
		private float _currentChestYaw = 0.0f;
		private bool _lostVisibility = true;
		private float _playerLateralOffset = 0.0f;
		private float _charLateralOffset = 0.0f;

		// player velocity
		private float _animationBlend;
		private float _verticalVelocity = 0f;
		private float _targetForwardVelocity = 0f;
		private float _targetRightVelocity = 0f;
		private float _terminalVelocity = -53.0f;


		// timeout deltatime
		private float _glideTimeoutDelta = 0.5F;
		private float _jumpTimeoutDelta;
		private float _fallTimeoutDelta;

		// animation IDs
		private int _animIDSpeed;
		private int _animIDGrounded;
		private int _animIDJump;
		private int _animIDFreeFall;
		private int _animIDMotionSpeed;

		private Animator _animator;
		private CharacterController _controller;
		private GameObject _mainCamera;
		private bool _hasAnimator;

		private void Awake()
		{
			// get a reference to our main camera
			if (_mainCamera == null)
			{
				_mainCamera = GameObject.FindGameObjectWithTag("MainCamera");
			}
		}

		private void Start()
		{
			_hasAnimator = TryGetComponent(out _animator);
			_controller = GetComponent<CharacterController>();
			if (_controller==null)
				Debug.Log("Missing controller for PoseAI CharacterController in " + transform.root.gameObject.name.ToString());
			SetSource(GetComponent<PoseAISource>());
			if (_source==null)
				Debug.Log("Missing source for PoseAI CharacterController in " + transform.root.gameObject.name.ToString());
			AssignAnimationIDs();

			// reset our timeouts on start
			_jumpTimeoutDelta = JumpTimeout;
			_fallTimeoutDelta = FallTimeout;
		}

		public void SetSource(PoseAISource source)
        {
			_source = source;
			if (_source != null)
				_rigObj = source.GetRig();
        }

		private void Update()
		{
			_hasAnimator = TryGetComponent(out _animator);
			bool hasJumped = false;

			if (_source != null && _rigObj != null)
			{
				var pVisibility = _rigObj.visibility;
				var pEvents = _rigObj.GetEvents();
				var pScalars = _rigObj.GetBody().Scalars;
				var pVectors = _rigObj.GetBody().Vectors;

				// keeps events from being triggered when player first (re)connects or appears on camera
				if (_lostVisibility)
					pEvents.ClearEventTriggers();

				if (pVisibility.isTorso)
				{
					_lostVisibility = false;
					_currentLean = pVectors.HipLean[0];
					_currentChestYaw = pScalars.ChestYaw;
				} else
				{
					_lostVisibility = true;
					// fade lean and chest yaw back to neutral
					_currentLean *= (1.0f - Time.deltaTime);
					_currentChestYaw *= (1.0f - Time.deltaTime);
					
				}

				/* here we check if an action occurs and update our StepCounter objects */
				if (pEvents.Footstep.CheckTriggerAndUpdate())
				{
					_footsteps.RegisterStep(Mathf.Abs(pEvents.Footstep.Magnitude));
				}
				if (pEvents.ArmPump.CheckTriggerAndUpdate())
				{
					_armpumps.RegisterStep(Mathf.Abs(pEvents.ArmPump.Magnitude));

				}

				/* this triggers if the number of stable feet has changed.  If both feet are stable than we could use that to immediately halt motion for quicker responsiveness.
				 * depending on how the user jogs in place, this could generate too many false positives for your use case, in which case it could be dropped */
				if(pScalars.StableFootHasChangedAndUpdate() && pScalars.StableFoot == 2)
				{
					_footsteps.Halt(true);
					_sidesteps.Halt(true);
				}

				// we set a booleon and trigger the jump
				hasJumped = pEvents.Jump.CheckTriggerAndUpdate();

				/*
				 * if your animation has strafing (i.e. sidesteps), you could use the Sidestep events to launch that animation and scale horizontal movement appropriately
				 * for the sidestep counter we include both left and right legs as events, and use the sign of the magnitude field to tell us which direction
				 * this can then be used to send a strafe signal
				 */
				if (pEvents.SidestepL.CheckTriggerAndUpdate())
				{
					_sidesteps.RegisterStep(Mathf.Sign(pEvents.SidestepL.Magnitude));
				}
				if (pEvents.SidestepR.CheckTriggerAndUpdate())
				{
					_sidesteps.RegisterStep(Mathf.Sign(pEvents.SidestepR.Magnitude));
				}
				/*
				 * instead we will just use the players position in the camera frameto slide the figure laterally based on the configured PlayerStrafeScale.
				 */
				if (pVisibility.isTorso && pScalars.BodyHeight > 0.0f)
					_playerLateralOffset = pVectors.HipScreen[0] / pScalars.BodyHeight * PlayerStrafeScale;


				if (pEvents.ArmFlex.CheckTriggerAndUpdate())
				{
					if (FlyWithChestExpansions && pEvents.ArmFlex.Magnitude > 0f && pScalars.HandZoneL == 4 && pScalars.HandZoneR == 6)
					{
						FlyUp();
						FlyUp();
					}
				}
				if (pEvents.ArmGestureL.CheckTriggerAndUpdate())
                {
					switch (pEvents.ArmGestureL.GetGesture()){
						case PoseAI_Gestures.FlapLateral:
						case PoseAI_Gestures.ReverseOverheadClap:
						case PoseAI_Gestures.Flap:
							FlyUp();
							break;
						//only need to call this in one hand
						case PoseAI_Gestures.OverheadClap:
						case PoseAI_Gestures.OverheadClapSmall:
							RocketUp();
							break;
					}
                    
                        
                }
                if (pEvents.ArmGestureR.CheckTriggerAndUpdate())
                {
					switch (pEvents.ArmGestureR.GetGesture()){
						case PoseAI_Gestures.FlapLateral:
						case PoseAI_Gestures.ReverseOverheadClap:
						case PoseAI_Gestures.Flap:
							FlyUp();
							break;
					}
				}


				/*
                 * There are events not handled by the Starter Asset controller and animation state machine, like crouching, that could be handled in a more developed setup.
                 */
				if (pEvents.FeetSplit.CheckTriggerAndUpdate())
				{
					_armpumps.RegisterStep(Mathf.Abs(pEvents.ArmPump.Magnitude));

				}


				if (pScalars.CrouchHasChangedAndUpdate())
				{
					Debug.Log((pScalars.IsCrouching == 1) ? "Player is crouching!" : "Player stood up!");
				}
				if (pScalars.HandZoneLHasChangedAndUpdate())
				{
					//Debug.Log("Left hand moved to zone " + pScalars.HandZoneL);
				}
				if (pScalars.HandZoneRHasChangedAndUpdate())
				{
					//Debug.Log("Right hand moved to zone " + pScalars.HandZoneR);
				}

				
			}
			UpdateGliding();
			Jump(hasJumped);
			ApplyGravity();
			GroundedCheck();
			Move();
		}

		private void LateUpdate()
		{
			CameraRotation();
		}

		private void AssignAnimationIDs()
		{
			_animIDSpeed = Animator.StringToHash("Speed");
			_animIDGrounded = Animator.StringToHash("Grounded");
			_animIDJump = Animator.StringToHash("Jump");
			_animIDFreeFall = Animator.StringToHash("FreeFall");
			_animIDMotionSpeed = Animator.StringToHash("MotionSpeed");
		}

		private void GroundedCheck()
		{
			// set sphere position, with offset
			Vector3 spherePosition = new Vector3(transform.position.x, transform.position.y - GroundedOffset, transform.position.z);
			IsGrounded = Physics.CheckSphere(spherePosition, GroundedRadius, GroundLayers, QueryTriggerInteraction.Ignore);

			if(IsGrounded)
				_cinemachineTargetPitch = 0f;
			// update animator if using character
			if (_hasAnimator)
			{
				_animator.SetBool(_animIDGrounded, IsGrounded);
			}
		}

		private void CameraRotation()
		{
			_cinemachineCurrentPitch = Mathf.Lerp(_cinemachineCurrentPitch,_cinemachineTargetPitch, Time.deltaTime * 4.0f);
			// clamp our rotations so our values are limited 360 degrees
			_cinemachineTargetYaw = ClampAngle(_cinemachineTargetYaw, float.MinValue, float.MaxValue);
			_cinemachineCurrentPitch = ClampAngle(_cinemachineCurrentPitch, float.MinValue, float.MaxValue);

			// Cinemachine will follow this target
			float appliedYaw = ClampAngle(_cinemachineTargetYaw + +transform.eulerAngles.y, float.MinValue, float.MaxValue);
			CinemachineCameraTarget.transform.rotation = Quaternion.Euler(_cinemachineCurrentPitch + CameraAngleOverride, appliedYaw , 0.0f);
		}

		private void MoveLaterally()
        {
			float offset = Mathf.Lerp(_charLateralOffset, _playerLateralOffset, Time.deltaTime * GroundSpeedChangeRate); 
			_targetRightVelocity = (offset - _charLateralOffset) / Time.deltaTime;
			_charLateralOffset = offset;
		}

		private void MoveOnGround()
        {
			float stepsPerSecond = _footsteps.StepsPerSecond();
			float distancePerSecond = _footsteps.DistancePerSecond();
			if (CanArmPumpsTriggerWalking) stepsPerSecond = Mathf.Max(stepsPerSecond, _armpumps.StepsPerSecond());

			if (UseAnalogGroundSpeed)
			{
				_targetForwardVelocity = _footsteps.DistancePerSecond() * ForwardSpeedScale;
			}
			else
			{
				if (stepsPerSecond > SprintAtStepsPerSecond || distancePerSecond > SprintAtDistancePerSecond) _targetForwardVelocity = SprintSpeed;
				else if (stepsPerSecond > WalkAtStepsPerSecond) _targetForwardVelocity = WalkSpeed;
				else _targetForwardVelocity = 0.0f;
			}
			_targetForwardVelocity = Mathf.Min(_targetForwardVelocity, MaxRunSpeed);
			}

		private void MoveInAir()
        {
			if (IsGliding)
				_targetForwardVelocity = GlidingFwdVelocity;
			else
				ApplyWindResistance();
        }

		private void Move()
		{
			float currentForwardSpeed = Vector3.Dot(_controller.velocity, transform.forward);
			float speedChangeRate;
			MoveLaterally();
			if (IsGrounded)
			{
				MoveOnGround();
				speedChangeRate = GroundSpeedChangeRate;
			}
			else
			{
				MoveInAir();
				speedChangeRate = GlideSpeedChangeRate;
			}
			
			float speedOffset = 0.1f;
			// accelerate or deaccelerate to target speed
			if (currentForwardSpeed < _targetForwardVelocity - speedOffset || currentForwardSpeed > _targetForwardVelocity + speedOffset)
			{
				currentForwardSpeed = Mathf.Lerp(currentForwardSpeed, _targetForwardVelocity, Time.deltaTime * speedChangeRate);
			}
			else
			{
				currentForwardSpeed = _targetForwardVelocity;
			}
			_animationBlend = Mathf.Lerp(_animationBlend, _targetForwardVelocity, Time.deltaTime * GroundSpeedChangeRate);

			float targetYaw = transform.eulerAngles.y;
			targetYaw += deadzone(_currentLean, LeanToTurnDeadzone) * LeanToTurnScale * Time.deltaTime ;
			targetYaw += deadzone(_currentChestYaw, TwistToTurnDeadzone) * TwistToTurnScale * Time.deltaTime;
			targetYaw = ClampAngle(targetYaw, float.MinValue, float.MaxValue);

			transform.rotation = Quaternion.Euler(0.0f, targetYaw, 0.0f);
			// move the player
			_controller.Move((
				transform.forward * currentForwardSpeed +
				transform.right * _targetRightVelocity +
				transform.up * _verticalVelocity
				) * Time.deltaTime);

			// update animator if using character
			if (_hasAnimator)
			{
				_animator.SetFloat(_animIDSpeed, _animationBlend);
				_animator.SetFloat(_animIDMotionSpeed, 1.0f);
			}
		}

		private float deadzone(float value, float deadzone)
        {
			if (value > deadzone) return value - deadzone;
			else if (value < -deadzone) return value + deadzone;
			else return 0.0f;
        }

		private void Jump(bool hasJumped)
		{
			if (IsGrounded)
			{
				// reset the fall timeout timer
				_fallTimeoutDelta = FallTimeout;

				// update animator if using character
				if (_hasAnimator)
				{
					_animator.SetBool(_animIDJump, false);
					_animator.SetBool(_animIDFreeFall, false);
				}

				// Jump
				if (hasJumped && _jumpTimeoutDelta <= 0.0f)
				{
					// the square root of H * -2 * G = how much velocity needed to reach desired height
					_verticalVelocity = Mathf.Sqrt(JumpHeight * -2f * Gravity);

					// update animator if using character
					if (_hasAnimator)
					{
						_animator.SetBool(_animIDJump, true);
					}
				}

				// jump timeout
				if (_jumpTimeoutDelta >= 0.0f)
				{
					_jumpTimeoutDelta -= Time.deltaTime;
				}
			}
			else
			{
				// reset the jump timeout timer
				_jumpTimeoutDelta = JumpTimeout;

				// fall timeout
				if (_fallTimeoutDelta >= 0.0f)
				{
					_fallTimeoutDelta -= Time.deltaTime;
				}
				else
				{
					// update animator if using character
					if (_hasAnimator)
					{
						_animator.SetBool(_animIDFreeFall, true);
					}
				}
			}
		}


		private void ApplyGravity()
		{
			// gliding effectively applies a much lower terminal velocity
			float thresholdVelocity = IsGliding ? GlidingFallVelocity : _terminalVelocity;

			// apply gravity over time if under terminal (multiply by delta time twice to linearly speed up over time)
			if (_verticalVelocity > thresholdVelocity)
				_verticalVelocity += Gravity * Time.deltaTime;

			// if already falling faster than smoothly move to threshold
			if (IsGliding && _verticalVelocity < thresholdVelocity)
				_verticalVelocity = Mathf.Lerp(_verticalVelocity, thresholdVelocity, GlideSpeedChangeRate * Time.deltaTime);

			// stop our velocity dropping infinitely when grounded
			if (IsGrounded && _verticalVelocity < 0.0f)
				_verticalVelocity = -2f;

		}

		/* slows character's forward movement gradually when airborne */
		private void ApplyWindResistance()
		{
			if (!IsGrounded)
			{
				if (_targetForwardVelocity > 0f)
					_targetForwardVelocity = Mathf.Max(_targetForwardVelocity + WindResistance * Time.deltaTime, 0f);
				else
					_targetForwardVelocity = Mathf.Min(_targetForwardVelocity - WindResistance * Time.deltaTime, 0f);
			}
		}

		/* function which checks if arms are outstretched */
		private bool AreArmsExtended()
		{
			if (_source==null || _rigObj == null)
				return false;
			var pScalars = _rigObj.GetBody().Scalars;
			uint handzoneL = pScalars.HandZoneL;
			uint handzoneR =  pScalars.HandZoneR;
			bool IsLeftExtended = handzoneL == 7 || handzoneL == 4;
			bool IsRightExtended = handzoneR == 9 || handzoneR == 6;
			return IsLeftExtended && IsRightExtended;
		}


		/* upwards flight applies burst velocity, similar to jump, to coincide with a flap of the wings */
		private void FlyUp()
		{
			float verticalBoost = Mathf.Sqrt(FlapHeight * -2f * Gravity);

			// get rid of negative starting velocity from ground
			if (IsGrounded)
				_verticalVelocity = Mathf.Max(_verticalVelocity, 0.0f);

			// we only add half of boost as each arm triggers function.  Both together will get maximum lift
			if (_verticalVelocity < verticalBoost)
				_verticalVelocity = Mathf.Min(_verticalVelocity + 0.5f * verticalBoost, verticalBoost);

			_cinemachineTargetPitch = AerialTargetPitch;
			_glideTimeoutDelta = FlyAutoGlideTime;

		}
		private void RocketUp()
		{
			// Only allow rocketup if on ground
			if (IsGrounded)
			{
				_verticalVelocity = Mathf.Sqrt(RocketUpHeight * -2f * Gravity);
				_cinemachineTargetPitch = AerialTargetPitch;
				_glideTimeoutDelta = 0f;
			}

		}

		/* can glide if arms are extended or given some glide time from flight event */
		private void UpdateGliding()
        {	
			_glideTimeoutDelta -= Time.deltaTime;
			IsGliding = !IsGrounded && ((_glideTimeoutDelta > 0.0f) || AreArmsExtended());
        }

		private static float ClampAngle(float lfAngle, float lfMin, float lfMax)
		{
			if (lfAngle < -360f) lfAngle += 360f;
			if (lfAngle > 360f) lfAngle -= 360f;
			return Mathf.Clamp(lfAngle, lfMin, lfMax);
		}

		private void OnDrawGizmosSelected()
		{
			Color transparentGreen = new Color(0.0f, 1.0f, 0.0f, 0.35f);
			Color transparentRed = new Color(1.0f, 0.0f, 0.0f, 0.35f);

			if (IsGrounded) Gizmos.color = transparentGreen;
			else Gizmos.color = transparentRed;

			// when selected, draw a gizmo in the position of, and matching radius of, the grounded collider
			Gizmos.DrawSphere(new Vector3(transform.position.x, transform.position.y - GroundedOffset, transform.position.z), GroundedRadius);
		}
	}

	public class StepCounter
	{
		/** Average step distance in [body width, body height] units per second.  If most recent step was longer than <timout> ago, speed is faded to zero */
		public float DistancePerSecond() { return distance_per_second_ * CheckIfActiveAndFade(); }

		/** clears all steps.  Optionally fades speed over <timeout>*/
		public void Halt(bool fade)
		{
			num_ = 0;
			tail_ = -1;
			if (fade)
				last_time_ = Mathf.Min(last_time_, Time.time - timeout);
			else
			{
				last_time_ = Mathf.Min(last_time_, Time.time - timeout - fadeDurationOnTimeout);
				steps_per_second_ = 0.0f;
				distance_per_second_ = 0.0f;
			}
		}

		/** Average number of steps per second.  If most recent step was longer than <timout> ago, speed is faded to zero */
		public float StepsPerSecond() { return steps_per_second_ * CheckIfActiveAndFade(); }

		/** Time since last step in seconds*/
		public float TimeSinceLastStep() { return Time.time - last_time_; }

		/** most recent step distance*/
		public float LastDistance() { return (num_ > 0) ? heights_[tail_] : 0.0f; }

		/** time since last step when motion begins to slow*/
		public float timeout = 0.75f;

		/** after a next step timeout, the speed is faded to zero over this duration*/
		public float fadeDurationOnTimeout = 0.25f;

		/** total steps registered by the stepcounter*/
		public int totalSteps = 0;

		/** total distance registered by the stepcounter*/
		public float totalDistance = 0.0f;

		public void RegisterStep(float stepDistance)
		{
			totalSteps++;
			totalDistance += stepDistance;
			tail_ = (tail_ + 1) % num_to_track;
			last_time_ = Time.time;
			times_[tail_] = last_time_;
			heights_[tail_] = stepDistance;
			num_ = Mathf.Min(num_ + 1, num_to_track);

			float elapsed_time;
			if (num_ < 2)
				elapsed_time = timeout;
			else
			{
				int head_ = (tail_ + 1) % num_;
				elapsed_time = (times_[tail_] - times_[head_]);
			}
			steps_per_second_ = ((float)num_) / elapsed_time;
			distance_per_second_ = 0.0f;
			for (int h = 0; h < num_; ++h)
				distance_per_second_ += heights_[h];
			distance_per_second_ /= elapsed_time;
		}


		private float CheckIfActiveAndFade()
		{
			float time_since_last_step = TimeSinceLastStep();
			if (time_since_last_step > timeout)
			{
				num_ = 0;
				tail_ = -1;
				return (fadeDurationOnTimeout > 0.0f) ? Mathf.Max(0.0f, 1.0f - (time_since_last_step - timeout) / fadeDurationOnTimeout) : 0.0f;
			}
			else
				return 1.0f;
		}

		readonly private int num_to_track = 4;
		private int num_ = 0;
		private int tail_ = -1;
		private float steps_per_second_ = 0.0f;
		private float distance_per_second_ = 0.0f;
		private float last_time_ = 0.0f;
		private List<float> times_ = new List<float> { 0.0f, 0.0f, 0.0f, 0.0f };
		private List<float> heights_ = new List<float> { 0.0f, 0.0f, 0.0f, 0.0f };
	}

}