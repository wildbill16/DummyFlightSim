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
	void MoveUpInput(float Val);

	/** Bound to the horizontal axis */
	void MoveRightInput(float Val);

	/*detect pitch input*/
	void StickPitchInput(float Val);

	/*detect roll input*/
	void StickRollInput(float Val);

	/*detect pitch input*/
	void StickYawInput(float Val);

	/*Force Calculation*/
	/*Lift realistic calculation*/
	void LiftCalculation();

	void WeightCalculation();

	void ThrustCalculation();

	void DragCalculation();

	/*Momentum and rotation sim*/
	void MomentumSim();


private:

	/** How quickly forward speed changes */
	UPROPERTY(Category=Plane, EditAnywhere)
	float Acceleration;

	/** How quickly pawn can steer */
	UPROPERTY(Category=Plane, EditAnywhere)
	float TurnSpeed;


	//relative wing area
	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_WingArea;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_numEngine;

	UPROPERTY(Category = Plane, EditAnywhere)
	float Flanker_EngineThrust_Dry_Max;//maximum thrust for single engin

	/** Max forward speed */
	UPROPERTY(Category = Pitch, EditAnywhere)
	float MaxSpeed;

	/** Min forward speed */
	UPROPERTY(Category=Yaw, EditAnywhere)
	float MinSpeed;

	/*dynamics*/
	float CurrentThrust;//P
	float CurrentDrag;//Q
	float CurrentLift;//Y
	float CurrentWeight;//G
	float CurrentMass;

	/*dynamic acceleration*/
	float Pc_Acc_A_X;//fpm direction
	float Pc_Acc_A_Y;//lift direction
	float Pc_Acc_A_Z;//side force direction, unused for now
	/*dynamic acceleration*/

	/** Current forward speed, path cordinate system */
	float Pc_CurrentForwardSpeed;//True airspeed, against self longitude axis

	/*Current Local Vertical, against relative airflow,path cordinate system*/
	float Pc_CurrentLocalVS;

	/*The x-y-z speed vecotr converted to local,or self cordinate system*/
	float Sc_X_Velocity;

	float Sc_Y_Velocity;

	float Sc_Z_Velocity;

	/** Current yaw speed */
	float CurrentYawSpeed;

	/** Current pitch speed */
	float CurrentPitchSpeed;

	/** Current roll speed */
	float CurrentRollSpeed;

	/*****Engine:Current thrust*******/
	float CurrentThrust;

	/**Engine: Current throttle**/
	float CurrentThrottle;

	/**Engine: Current throttle**/
	float CurrentRPM;

	/*Stick Input*/
	float CurrentStickPitch;

	/*Stick Input*/
	float CurrenStickRoll;

	/*Stick Input*/
	float CurrentStickYaw;

	/**Atitude Current Aagle of attack**/
	float CurrentAoA;//¦Á

	/**Atitude Current Pitch Angle**/
	float CurrentTheta;//¦È

	/**Atitude Current Roll Angle**/
	float CurrentGamma;//¦Ã

	/*Aerodata*/
	float density;



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

	UPROPERTY(Category = Plane, EditAnywhere)
		float Initial_AngleofAttack = 7.0f;//intial AoA value for unique drag and lift, in degrees**need convertion

	UPROPERTY(Category = Plane, EditAnywhere)
		float Motion_CurrentASL=200.0f;
	/*Motion*/
	


};
