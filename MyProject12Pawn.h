// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved. Aero dyanmics and FLCS added by FANZY
#pragma once
#include "GameFramework/Pawn.h"
#include "MyProject12Pawn.generated.h"

UCLASS(config=Game)
class AMyProject12Pawn : public APawn
{
	GENERATED_BODY()

	/** StaticMesh component that will be the visuals for our flying pawn */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* PlaneMesh;

	/** Spring arm that will offset the camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;
public:
	AMyProject12Pawn();

	// Begin AActor overrides
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	// End AActor overrides

protected:

	// Begin APawn overrides
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override; // Allows binding actions/axes to functions
	// End APawn overrides

	/** Bound to the thrust axis */
	void ThrustInput(float Val);

	/*Simulate thge Engine status*/
	void EngineSim();
	
	/** Bound to the vertical axis */
	//void MoveUpInput(float Val);

	/** Bound to the horizontal axis */
	//void MoveRightInput(float Val);

	/*detect pitch input 杆舵输入-俯仰*/
	void StickPitchInput(float Val);

	/*detect roll input  杆舵输入-滚转*/
	void StickRollInput(float Val);

	/*detect pitch input 杆舵输入-偏航*/
	void StickYawInput(float Val);

	/*Force Calculation*/
	/*Lift realistic calculation*/
	void LiftCalculation();

	void WeightCalculation();

	void ThrustCalculation();

	void DragCalculation();

	void BalanceCalculation();

	/*Momentum and rotation sim*/
	void MomentumSim();

	/*Flight Control System*/
	void FLCS();

	/*Torque and Rotation Calculation*/
	void PitchTorCalculation(); //俯仰力矩

	void RollTorCalculation();

	void YawTorCalculation();

	/*Aerodynamic Calulation—Decide Maximum Rorate Rate*/
	void AeroAnalysisPitch();

	void AeroAnalysisRoll();

	void AeroAnalysisYaw();

	void SonicCorrection();

/*private*/
public:

	/** How quickly forward speed changes */
	UPROPERTY(Category=Plane, EditAnywhere, BlueprintReadWrite)
	float Acceleration;

	/** How quickly pawn can steer */
	UPROPERTY(Category=Plane, EditAnywhere, BlueprintReadWrite)
	float TurnSpeed;


	//relative wing area
	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_WingArea;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_numEngine;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_EngineThrust_Dry_Max;//maximum thrust for single engin

	/** Max forward speed */
	UPROPERTY(Category = Plane, EditAnywhere)
	float MaxSpeed;

	/** Min forward speed */
	UPROPERTY(Category= Plane, EditAnywhere)
	float MinSpeed;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Max_AoA;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Min_AoA;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Max_G;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Min_G;

	/*dynamics*/
	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float CurrentDrag;//Q

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float CurrentLift;//Y

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float CurrentWeight;//G

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float CurrentMass;

	/*dynamic acceleration*/
	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float Pc_Acc_A_X;//fpm direction

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float Pc_Acc_A_Y;//lift direction

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float Pc_Acc_A_Z;//side force direction, unused for now
	/*dynamic acceleration*/

	/*Position*/
	float CurrentHeight;

	/* Current forward speed, path cordinate system */
	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float Pc_CurrentForwardSpeed;//True airspeed, against self longitude axis

	/*Current Local Vertical, against relative airflow,path cordinate system*/
	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float Pc_CurrentLocalVS;

	/*The x-y-z speed vecotr converted to local,or self cordinate system*/
	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float Sc_X_Velocity; //True airspeed

	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float Sc_Y_Velocity;

	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float Sc_Z_Velocity;

	UPROPERTY(Category = Movement, EditAnywhere, BlueprintReadWrite)
	float CurrentHDG;

	/*Dynamic pressure conversion*/
	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	float IAS;//indicated air speed. IAS=TAS*sqrt(P/P0);	
	float Aero_PR;//pressure rate. Pressure at local height against pressure at sea level
	/*Linear model Poly2:
     f(x) = p1*x^2 + p2*x + p3
	Coefficients (with 95% confidence bounds):
       p1 =   2.481e-09  (2.384e-09, 2.578e-09)
       p2 =  -9.083e-05  (-9.205e-05, -8.961e-05)
       p3 =      0.9948  (0.9916, 0.998)*/

	UPROPERTY(Category = Dynamic, EditAnywhere, BlueprintReadWrite)
	/*Aerodata*/
	float MachSpeed;// aircraft true airspeed vs local sonic


	/** Current yaw speed  偏航速度 */
	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	float CurrentYawSpeed;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	/** Current pitch speed 俯仰速度*/
	float CurrentPitchSpeed;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	/** Current roll speed 滚转速度*/
	float CurrentRollSpeed;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	/** Maximum Pitch Speed 最大俯仰速度*/
	float Max_Pitch_Rate;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	/** Maximum Roll Speed 最大俯仰速度*/
	float Max_Roll_Rate;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	/** Maximum Yaw Speed 最大俯仰速度*/
	float Max_Yaw_Rate;

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	float Apr;//angular acceleration caused by alpha

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	float Apd;//angular acceleration caused by aero-damping

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	float Apc;//angular acceleration caused by control surface

	UPROPERTY(Category = AngularMotion, EditAnywhere, BlueprintReadWrite)
	float Ap;//composited angular acceleration




	UPROPERTY(Category = Engine, EditAnywhere, BlueprintReadWrite)
	/*****Engine:Current thrust*******/
	float CurrentThrust;

	UPROPERTY(Category = FLCSHumanInput, EditAnywhere, BlueprintReadWrite)
	/**Engine: Current throttle**/
	float CurrentThrottle;

	/**Engine: Current throttle**/
	UPROPERTY(Category = Engine, EditAnywhere, BlueprintReadWrite)
	float CurrentRPM;

	UPROPERTY(Category = FLCSHumanInput, EditAnywhere, BlueprintReadWrite)
	/*飞控Stick Input--杆舵当前位置*/
	float CurrentStickPitch;

	UPROPERTY(Category = FLCSHumanInput, EditAnywhere, BlueprintReadWrite)
	/*飞控Stick Input--杆舵当前位置*/
	float CurrentStickRoll;

	UPROPERTY(Category = FLCSHumanInput, EditAnywhere, BlueprintReadWrite)
	/*飞控Stick Input--杆舵当前位置*/
	float CurrentStickYaw;

	bool bPitchHasInput;
	
	bool bRollHasInput;//judge whether there is input on stick and paddle

	bool bYawHasInput;

	UPROPERTY(Category = GAMEIO, EditAnywhere, BlueprintReadWrite)
	/*IO--俯仰输入*/
	float InputPitch;

	UPROPERTY(Category = GAMEIO, EditAnywhere, BlueprintReadWrite)
	/*IO--滚转输入*/
	float InputRoll;

	UPROPERTY(Category = GAMEIO, EditAnywhere, BlueprintReadWrite)
	/*IO--偏航输入*/
	float InputYaw;

	UPROPERTY(Category = GAMEIO, EditAnywhere, BlueprintReadWrite)
	/*IO--油门输入*/
	float InputThrust;

	/*机电 Mechinary and Eletricity System --Left Elevator position 左升降舵位置*/

	/*机电 Mechinary and Eletricity System --Left Elevator position 右升降舵位置*/

	/*机电 Mechinary and Eletricity System --Rudder position 方向舵位置*/

	/*机电 Mechinary and Eletricity System --Airbrake position 减速板位置*/

	/**姿态000Atitude Current Aagle of attack**/
	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	float CurrentAoA;//α

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/**姿态000Atitude Current Pitch Angle**/
	float CurrentTheta;//θ

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/**姿态000Atitude Current Roll Angle**/
	float CurrentGamma;//γ

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/**Zero-lift alpha**/
	float ZeroAlpha;//A0

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/**Zero-lift alpha**/
	float StartAlpha;//As..Same as steady alpha

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/*delta AoA that cause rotation*/
	float TorqueAoA;//

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/*delta AoA persecond*/
	float DeltaAoA;//

	UPROPERTY(Category = Atitude, EditAnywhere, BlueprintReadWrite)
	/**level flight CL**/
	float SteadyCL;//As..Same as steady alpha

	UPROPERTY(Category = Atmosphere, EditAnywhere, BlueprintReadWrite)
	/*Aerodata*/
	float density;

	UPROPERTY(Category = Atmosphere, EditAnywhere, BlueprintReadWrite)
	/*Aerodata*/
	float LocalSonicSpeed;


public:
	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return PlaneMesh; }
	/** Returns SpringArm subobject **/
	FORCEINLINE class USpringArmComponent* GetSpringArm() const { return SpringArm; }
	/** Returns Camera subobject **/
	FORCEINLINE class UCameraComponent* GetCamera() const { return Camera; }


	UPROPERTY(Category = Plane, EditAnywhere)
		float Flanker_Empty_Weight = 18500.0f;//kg

	UPROPERTY(Category = Plane, EditAnywhere)
		float Flanker_Fuel_Weight = 9400.0f;

	UPROPERTY(Category = Atitude, EditAnywhere)
		float Initial_AngleofAttack = 0.0f;//intial AoA value for unique drag and lift, in degrees**need convertion

	UPROPERTY(Category = Plane, EditAnywhere)
		float Motion_CurrentASL=200.0f;
	/*Motion*/

	UPROPERTY(Category = Plane, EditAnywhere)
		float InitialTrueAirspeed = 100.0f;
	


};
