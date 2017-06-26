// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved. Aero dyanmics and dummy FLCS added by FANZY
/*
Prefix:
Sc_:Self cordinate frame. X point to gun sight direction.Y point to airplane's right 
Pc_:Path_cordinate frame. X point to velocity
*/


#include "MyProject12.h"
#include "MyProject12Pawn.h"

AMyProject12Pawn::AMyProject12Pawn()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
	RootComponent = PlaneMesh;

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 160.0f; // The camera follows at this distance behind the character	
	SpringArm->SocketOffset = FVector(0.f,0.f,60.f);
	SpringArm->bEnableCameraLag = false;
	SpringArm->CameraLagSpeed = 15.f;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller

	// Set handling parameters
	Acceleration = 500.f;
	TurnSpeed = 50.f;
	MaxSpeed = 4000.f;
	MinSpeed = 0.0f;
	Pc_CurrentForwardSpeed = 120.f;//here m/s
	Pc_CurrentLocalVS = 0.0f;//local vertical speed, m/s, need to convert to cm/s

	Sc_X_Velocity = Pc_CurrentForwardSpeed*FMath::Cos(FMath::DegreesToRadians(CurrentAoA));
	Sc_Y_Velocity = Pc_CurrentForwardSpeed*FMath::Sin(FMath::DegreesToRadians(CurrentAoA));
	Sc_Z_Velocity = 0;

	//aircraft data
	Flanker_WingArea = 62.0f;//square meters
	Flanker_numEngine = 2.0f;
	Flanker_EngineThrust_Dry_Max = 121520.0f;// N dry
	CurrentRPM = 0.7;//70% RMP, should providing 34.3% thrust
	CurrentThrottle = 0.5;// 0~1 is the range of the thruttle     R=0.3T+0.7

	//Aircraft atitude
	CurrentAoA = Initial_AngleofAttack;

	/*Aerodynamic force*/
	CurrentDrag = 0.0f;
	CurrentLift = 0.0f;

	/*Mechanical force*/
	CurrentMass = Flanker_Empty_Weight + Flanker_Fuel_Weight;
	CurrentWeight = (Flanker_Empty_Weight + Flanker_Fuel_Weight)*9.8f;
	CurrentThrust = FMath::Pow(CurrentThrottle,3)* 2 * Flanker_numEngine*Flanker_EngineThrust_Dry_Max;

	/*Motion*/
	Motion_CurrentASL = AMyProject12Pawn::GetActorLocation().Z*0.01;
}

void AMyProject12Pawn::Tick(float DeltaSeconds)
{
	EngineSim();
	const FVector LocalMove = FVector(100*Pc_CurrentForwardSpeed * DeltaSeconds, 0.f, 0.f);
	//const FVector LocalMove = FVector(0.0f, 0.0f, 20.0f);
	//aircraft local axis<->local move (x,z,y) (front, right, up)@positive value
	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	// Calculate change in rotation this frame
	FRotator DeltaRotation(0,0,0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// Rotate plane
	AddActorLocalRotation(DeltaRotation);

	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);//what's super?

	UE_LOG(LogTemp, Log, TEXT("Current True Airspeed is %s m/s"), *(FString::SanitizeFloat(Pc_CurrentForwardSpeed)));
	UE_LOG(LogTemp, Log, TEXT("Current Throttle %s"), *(FString::SanitizeFloat(CurrentThrottle)));
	UE_LOG(LogTemp, Log, TEXT("Current RPM %s" ), *(FString::SanitizeFloat(CurrentRPM)));
	UE_LOG(LogTemp, Log, TEXT("Current Thrust %s N"), *(FString::SanitizeFloat(CurrentThrust)));
}

void AMyProject12Pawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation(RootComponent);
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));
}


void AMyProject12Pawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	// Bind our control axis' to callback functions
	PlayerInputComponent->BindAxis("Thrust", this, &AMyProject12Pawn::ThrustInput);
	PlayerInputComponent->BindAxis("MoveUp", this, &AMyProject12Pawn::MoveUpInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProject12Pawn::MoveRightInput);
	PlayerInputComponent->BindAxis("PitchInput", this, &AMyProject12Pawn::StickPitchInput);
	PlayerInputComponent->BindAxis("RollInput", this, &AMyProject12Pawn::StickRollInput);
	PlayerInputComponent->BindAxis("YawInput", this, &AMyProject12Pawn::StickYawInput);

}

void AMyProject12Pawn::ThrustInput(float Val)
{
	/*
	// Is there no input?
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	// If input is not held down, reduce speed
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.5f * Acceleration);
	// Calculate new speed
	float NewForwardSpeed = Pc_CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	// Clamp between MinSpeed and MaxSpeed
	Pc_CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
	*/
	// Is there no input?
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);

	// If input is not held down, reduce speed
	if (bHasInput == false)//Engine has no input
	{
		CurrentThrottle = CurrentThrottle;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Move throttle"));
		CurrentThrottle = FMath::Clamp(CurrentThrottle + Val*(GetWorld()->GetDeltaSeconds()),0.0f,1.0f);//CurrentThrottle Then Drive Engine RPM
	}
}

void AMyProject12Pawn::EngineSim()
{
	//1.throttle increase with delay
	//2.RPM increase with delay
	//3.thrust-RPM sync (Round per minute, in percentage)
	//reaction time: 1 seconds from 0.7-1
	//RPM=0.3Thrust+0.7
	float RPMChangingSpeed = 0.25f;//RPM increases by 30%/second at most
	float RPMChangeDirection = 0.0f;
	//static float density = 1.29f;//density of the air
	
	//static float CurrentDrag = 0.0f;//force, unit:newton
	static float CurrentAcc = 0.0f;//acceleration, unit: m/s2
	
	if ((CurrentThrottle - CurrentRPM < 0))
	{
		RPMChangeDirection = -1.0f;
	}
	else
	{
		RPMChangeDirection = 1.0f;//increase RPM
	}
	//change RPM
	CurrentRPM = FMath::Clamp(float(CurrentRPM + 0.25*(GetWorld()->GetDeltaSeconds())*RPMChangeDirection),0.7f,1.0f);



}

void AMyProject12Pawn::MoveUpInput(float Val)
{
	// Target pitch speed is based in input
	float TargetPitchSpeed = (Val * TurnSpeed * -1.f);

	// When steering, we decrease pitch slightly
	TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2f);

	// Smoothly interpolate to target pitch speed
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}

void AMyProject12Pawn::MoveRightInput(float Val)
{
	// Target yaw speed is based on input
	float TargetYawSpeed = (Val * TurnSpeed);

	// Smoothly interpolate to target yaw speed
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, GetWorld()->GetDeltaSeconds(), 2.f);

	// Is there any left/right input?
	const bool bIsTurning = FMath::Abs(Val) > 0.2f;

	// If turning, yaw value is used to influence roll
	// If not turning, roll to reverse current roll value
	float TargetRollSpeed = bIsTurning ? (CurrentYawSpeed * 0.5f) : (GetActorRotation().Roll * -2.f);

	// Smoothly interpolate roll speed
	CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}

void AMyProject12Pawn::StickPitchInput(float Val)
{

}

void AMyProject12Pawn::StickRollInput(float Val)
{

}

void AMyProject12Pawn::StickYawInput(float Val)
{
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);

	// If input is not held down, yaw to neutral
	if (bHasInput == false)//paddle not pressed down
	{
		CurrentStickYaw = CurrentStickYaw;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Move throttle"));
		CurrentThrottle = FMath::Clamp(CurrentThrottle + Val*(GetWorld()->GetDeltaSeconds()), 0.0f, 1.0f);//CurrentThrottle Then Drive Engine RPM
	}
}

void AMyProject12Pawn::LiftCalculation()
{
	static float cl = 0.5f;
	cl = (-4.336e-5)*FGenericPlatformMath::Pow(CurrentAoA, 3)+0.001116*FGenericPlatformMath::Pow(CurrentAoA, 2)+0.08618*CurrentAoA-0.03852f;
	Motion_CurrentASL = AMyProject12Pawn::GetActorLocation().Z*0.01;
	density = (2.676e-9)*FGenericPlatformMath::Pow(Motion_CurrentASL, 2) + (-0.0001071)*Motion_CurrentASL + 1.283;//matlab cf poly
	CurrentLift = 0.5*density*Pc_CurrentForwardSpeed*Pc_CurrentForwardSpeed*Flanker_WingArea*cl;//Unit in Newton
}

void AMyProject12Pawn::WeightCalculation()
{
	CurrentWeight = (Flanker_Empty_Weight + Flanker_Fuel_Weight)*9.8;//unit: Newton
}

void AMyProject12Pawn::ThrustCalculation()
{
	//Change Thrust
	CurrentThrust = FGenericPlatformMath::Pow(CurrentRPM, 3) * 2 * Flanker_numEngine*Flanker_EngineThrust_Dry_Max;
}

void AMyProject12Pawn::DragCalculation()
{
	static float cd = 0.02f; //drag coefficient, temporary
	static float ASL = 0.0f;//player pawn absolute height
	//Drag Calculation
	//CD: 0.02 for temp
	//-8<=AoA<30
	//AoA>=30
	FVector ActorLocation = AMyProject12Pawn::GetActorLocation();
	// Move it slightly  
	ASL = ActorLocation.Z*0.01;
	density = (2.676e-9)*FGenericPlatformMath::Pow(ASL, 2) + (-0.0001071)*ASL + 1.283;//matlab cf poly
		/*
		     f(x) = p1*x^2 + p2*x + p3
Coefficients (with 95% confidence bounds):
       p1 =   2.676e-09  (2.572e-09, 2.78e-09)
       p2 =  -0.0001071  (-0.0001086, -0.0001057)
       p3 =       1.283  (1.279, 1.287)*/
	CurrentDrag = 0.5*density*Pc_CurrentForwardSpeed*Pc_CurrentForwardSpeed*Flanker_WingArea*cd;
}

void AMyProject12Pawn::MomentumSim()
{
	//Acceleration in self axial and vertical direction
	Pc_Acc_A_X = (CurrentThrust*FMath::Cos(FMath::DegreesToRadians(CurrentAoA)) - CurrentDrag) / CurrentMass;
	Pc_Acc_A_Y = (CurrentThrust*FMath::Sin(FMath::DegreesToRadians(CurrentAoA)) + CurrentLift - CurrentWeight*FMath::Cos(FMath::DegreesToRadians(CurrentTheta)))/CurrentMass;


    //float NewForwardSpeed = Pc_CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc); don't know what New
    //FWspeed is
	Pc_CurrentForwardSpeed = Pc_CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * Pc_Acc_A_X);//speed synthesis
	Pc_CurrentLocalVS = Pc_CurrentLocalVS + (GetWorld()->GetDeltaSeconds() * Pc_Acc_A_Y);

	//CG's velocity converted to self cordinate velocity
	Sc_X_Velocity = Pc_CurrentForwardSpeed*FMath::Cos(FMath::DegreesToRadians(CurrentAoA));
	Sc_Y_Velocity = Pc_CurrentForwardSpeed*FMath::Sin(FMath::DegreesToRadians(CurrentAoA));

}
