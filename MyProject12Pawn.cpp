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



	Pc_CurrentForwardSpeed = InitialTrueAirspeed;//here m/s
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
	TorqueAoA = 0;
	DeltaAoA = 0;
	/*Aerodynamic force*/
	CurrentDrag = 0.0f;
	CurrentLift = 0.0f;

	MachSpeed = Pc_CurrentForwardSpeed / LocalSonicSpeed;

	/*Mechanical force*/
	CurrentMass = Flanker_Empty_Weight + Flanker_Fuel_Weight;
	CurrentWeight = (Flanker_Empty_Weight + Flanker_Fuel_Weight)*9.8f;
	CurrentThrust = FMath::Pow(CurrentThrottle,3)* 2 * Flanker_numEngine*Flanker_EngineThrust_Dry_Max;

	/*Motion*/
	Motion_CurrentASL = AMyProject12Pawn::GetActorLocation().Z*0.01;
	CurrentHeight = AMyProject12Pawn::GetActorLocation().Z*0.01;

	CurrentHDG = AMyProject12Pawn::GetActorRotation().Yaw;
	
	/*Angular motion*/
	Apr = 0;
	Apd = 0;
	Apc = 0;
	Ap = 0;
	ZeroAlpha = 0;

	/*Current Stick Input*/
	CurrentStickPitch = 0.0f;
	CurrentStickRoll = 0.0f;
	CurrentStickYaw = 0.0f;

	/*Pilot Input*/
	InputThrust = 0.0f;
	InputPitch = 0.0f;
	InputRoll = 0.0f;
	InputYaw = 0.0f;
	

}

void AMyProject12Pawn::Tick(float DeltaSeconds)
{
	/*
	static float framecount = 0;
	if (framecount == 0)
	{
		BalanceCalculation();
		framecount = 1;
		CurrentAoA = StartAlpha;
	}
	*/
	//attitude calculation
	//static float atx = 0;
	//static float aty = 0;
	//static float atz = 0;
	//atx = FMath::RadiansToDegrees(AMyProject12Pawn::GetActorRotation().Pitch);
	//atx = AMyProject12Pawn::GetActorRotation().Pitch;
	CurrentTheta = AMyProject12Pawn::GetActorRotation().Pitch;//GetRotation Gets values in Degrees
	//UE_LOG(LogTemp, Log, TEXT("Current Pitch %s"), *(FString::SanitizeFloat(atx)));
	CurrentHDG = AMyProject12Pawn::GetActorRotation().Yaw;

	EngineSim();
	LiftCalculation();
	DragCalculation();
	WeightCalculation();
	ThrustCalculation();
	MomentumSim();
	SonicCorrection();//re-calculate local speed of sound
	//End of Movements of Center of Gravity


	//const FVector LocalMove = FVector(100*Pc_CurrentForwardSpeed * DeltaSeconds, 0.f, 0.f);
	const FVector LocalMove = FVector(100 * Sc_X_Velocity * DeltaSeconds, 100 * Sc_Y_Velocity * DeltaSeconds, 0.f);
	//const FVector LocalMove = FVector(0.0f, 0.0f, 20.0f);
	//aircraft local axis<->local move (x,z,y) (front, right, up)@positive value
	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	//FLCS control
	FLCS();

	//Aero and Torque sim
	AeroAnalysisPitch();//alpha correction

	AeroAnalysisRoll();
	AeroAnalysisYaw();
	//---end of aero, start torque sim
	//pitch cal
	//roll cal
	PitchTorCalculation();
	YawTorCalculation();
	RollTorCalculation();

	// Calculate change in rotation this frame
	FRotator DeltaRotation(0, 0, 0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// Rotate plane
	AddActorLocalRotation(DeltaRotation);

	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);//what's super?

	/*
	UE_LOG(LogTemp, Log, TEXT("Current True Airspeed is %s m/s"), *(FString::SanitizeFloat(Pc_CurrentForwardSpeed)));
	UE_LOG(LogTemp, Log, TEXT("Current Throttle %s"), *(FString::SanitizeFloat(CurrentThrottle)));
	UE_LOG(LogTemp, Log, TEXT("Current RPM %s"), *(FString::SanitizeFloat(CurrentRPM)));
	UE_LOG(LogTemp, Log, TEXT("Current Thrust %s N"), *(FString::SanitizeFloat(CurrentThrust)));
	*/

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
	//PlayerInputComponent->BindAxis("MoveUp", this, &AMyProject12Pawn::MoveUpInput);
	//PlayerInputComponent->BindAxis("MoveRight", this, &AMyProject12Pawn::MoveRightInput);
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
	InputThrust = Val;
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
	//static float CurrentAcc = 0.0f;//acceleration, unit: m/s2
	
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
/*
void AMyProject12Pawn::MoveUpInput(float val)
{
	// target pitch speed is based in input
	float TargetPitchSpeed = (val * TurnSpeed * -1.f);

	// when steering, we decrease pitch slightly
	TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2f);

	// smoothly interpolate to target pitch speed
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
*/


void AMyProject12Pawn::StickPitchInput(float Val) //并非实时调用，只有有输入的时候才调用。所以回杆需要FLCS 
{						// Not used in every Tick. Executed when function call. FLCS recenter the tick
	//does not automatically return to neutral,0.5 seconds from 0 to 1
	//不回中，0.5秒到底---Up 1.0, Down-1.0
	bPitchHasInput = !FMath::IsNearlyEqual(Val, 0.0f);
	if (bPitchHasInput == false)
	{
		CurrentStickPitch = CurrentStickPitch;
	}
	else
	{
		CurrentStickPitch=FMath::Clamp(CurrentStickPitch+ 0.8f*Val*(GetWorld()->GetDeltaSeconds()), -1.0f, 1.0f);
	}
}

void AMyProject12Pawn::StickRollInput(float Val) 
{
	//automatically return to neutral,0.5 seconds from 0 to 1
	//回中，0.5秒到底
	bRollHasInput=!FMath::IsNearlyEqual(Val, 0.0f);
	if (bRollHasInput == false)
	{
		CurrentStickRoll = CurrentStickRoll;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Paddle pressed"));
		CurrentStickRoll = FMath::Clamp(CurrentStickRoll + 1.0f*Val*(GetWorld()->GetDeltaSeconds()), -1.0f, 1.0f);//Paddle drives the rudder,0.3 to full
	}
}


void AMyProject12Pawn::StickYawInput(float Val) 
{	//val:0 or 1 in keyboard mode
	//automatically return to neutral,0.5 seconds from 0 to 1
	//回中，0.5秒到底
	bYawHasInput = !FMath::IsNearlyEqual(Val, 0.0f);

	// If input is not held down, yaw to neutral
	if (bYawHasInput == false)//paddle not pressed down
	{

		CurrentStickYaw = CurrentStickYaw;

	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Paddle pressed"));
		CurrentStickYaw = FMath::Clamp(CurrentStickYaw + 3.0f*Val*(GetWorld()->GetDeltaSeconds()), -1.0f, 1.0f);//Paddle drives the rudder
	}
}

/*FLCS:Automatically correct sticks and throttle*/
void AMyProject12Pawn::FLCS()
{
	if (bYawHasInput==false) //ruddle-yaw control
	{
		CurrentStickYaw = CurrentStickYaw - CurrentStickYaw*1.7f*(GetWorld()->GetDeltaSeconds());
	}
	if (bRollHasInput == false) //must return roll
	{
		CurrentStickRoll = CurrentStickRoll - CurrentStickRoll*1.7f*(GetWorld()->GetDeltaSeconds());
	}
	//pitch stay in position
	//throttle stay in position
}

void AMyProject12Pawn::PitchTorCalculation()
{
	
	CurrentPitchSpeed = FMath::Clamp(CurrentPitchSpeed,-25.0f,55.0f);
}

void AMyProject12Pawn::RollTorCalculation()
{
	CurrentRollSpeed = CurrentStickRoll*Max_Roll_Rate;
}

void AMyProject12Pawn::YawTorCalculation()
{
	CurrentYawSpeed = CurrentStickYaw*Max_Yaw_Rate;
}

void AMyProject12Pawn::AeroAnalysisPitch()//....Pitch use accelerating calculation
{

	//get acc->get pitchspeed
	//alpha0=0

	Apr = -4.0f*(IAS/433.0f) * (CurrentAoA-SteadyCL)/120;
	//CurrentAoA = CurrentAoA + Apr;
	//y = -0.0002x2 + 0.2541x - 0.9448  Aerodamping Torque
	Apd = -1.0f*(-0.0002*CurrentPitchSpeed*CurrentPitchSpeed + 0.2514*CurrentPitchSpeed - 0.9448);

	//control surface's torque caused angular acceleration=IAS/433*30
	//when pull stick, current stick pitch decrease to -1
	Apc = -1.0f*(IAS / 433) *CurrentStickPitch*60;
	CurrentPitchSpeed = Apr + Apd + Apc;
	//CurrentAoA = FMath::Clamp(CurrentAoA + (Apc + Apr+Apd)*GetWorld()->GetDeltaSeconds(),-8.0f,40.0f);
	//DeltaAoA = Apr + Apd + Apc;
	//CurrentAoA = CurrentAoA+DeltaAoA;
}

void AMyProject12Pawn::AeroAnalysisRoll()
{
	if (CurrentAoA < 0)
	{
		/*y = -0.0005x2 + 0.48x + 0.0386
		y = -0.0055x + 109.09
		*/
		if (IAS < 500)
		{
			Max_Roll_Rate = -0.0005*IAS*IAS + 0.48*IAS + 0.0386;
		}
		else
		{
			Max_Roll_Rate = -0.0055*IAS + 109.09;
		}

	}
	else if (CurrentAoA < 20)
	{
		/*
		y = 4E-06x3 - 0.004x2 + 1.4335x + 2.0789
		y = -0.0055x + 182.73
		*/
		if (IAS < 500)
		{
			Max_Roll_Rate = (4e-6)*FMath::Pow(IAS, 3) - 0.004*IAS*IAS + 1.4335*IAS + 2.0789;
		}
		else
		{
			Max_Roll_Rate = -0.0055*IAS + 182.73;
		}
	}
	else if (CurrentAoA < 30)
	{
		/*
		y = 2E-06x3 - 0.0024x2 + 0.9831x + 0.6167
		y = -0.0055x + 145.91
		*/
		if (IAS < 500)
		{
			Max_Roll_Rate = (2e-6)*FMath::Pow(IAS, 3) - 0.0024*IAS*IAS + 0.9831*IAS + 0.6176;
		}
		else
		{
			Max_Roll_Rate = -0.0055*IAS + 145.91;
		}


	}
	else if (CurrentAoA < 40)
	{
		/*
		y = 4E-07x3 - 0.0008x2 + 0.5326x - 0.8444
		y = -0.0055x + 109.09
		*/
		if (IAS < 500)
		{
			Max_Roll_Rate = (4e-7)*FMath::Pow(IAS, 3) - 0.0008*IAS*IAS + 0.5326*IAS - 0.8444;
		}
		else
		{
			Max_Roll_Rate = -0.005*IAS + 109.09;
		}

	}
	else
	{
		/*
		y = -4E-07x3 + 0.0003x2 - 0.0225x + 0.8115
		y = -0.0027x + 21.813
		*/
		if (IAS < 500)
		{
			Max_Roll_Rate = (-4e-7)*FMath::Pow(IAS, 3) + 0.0003*IAS*IAS - 0.0225*IAS + 0.8115;
		}
		else
		{
			Max_Roll_Rate = -0.0027*IAS + 21.813;
		}
	}
}

void AMyProject12Pawn::AeroAnalysisYaw() //Decides how fast can the plane yaw
{
	if (CurrentAoA < 10)
	{
		//y = 3E-07x3 - 0.0003x2 + 0.1143x - 4.7008
		Max_Yaw_Rate = (3e-7)*FMath::Pow(IAS,3)-0.0003*FMath::Pow(IAS,2)+0.1143*IAS -4.7008;
	}
	else if (CurrentAoA < 20)
	{
		//y = -3E-05x2 + 0.0327x - 1.2812
		Max_Yaw_Rate = - (3e-5)*FMath::Pow(IAS, 2) + 0.0327*IAS - 1.2812f;
	}
	else if (CurrentAoA < 40)
	{
		//y = 7E-06x2 + 0.0051x - 0.1529
		Max_Yaw_Rate = (7e-6)*FMath::Pow(IAS, 2) + 0.0051*IAS - 0.1529f;
	}
	else
	{
		//y = 1E-05x2 - 0.0022x + 0.1406
		Max_Yaw_Rate = (1e-5)*FMath::Pow(IAS, 2) - 0.0022*IAS + 0.1406f;
	}
}




void AMyProject12Pawn::LiftCalculation()
{
	static float cl = 0.5f;
	if (MachSpeed < 0.2)
	{
		cl = (-4.336e-5)*FGenericPlatformMath::Pow(CurrentAoA, 3) + 0.001116*FGenericPlatformMath::Pow(CurrentAoA, 2) + 0.08618*CurrentAoA - 0.03852f;//0.07f for zero lift alpha correction
	}
	else
	{
		//Ma<0.6
		//y = -0.0001x2 + 0.0642x + 0.0062
		cl = (-0.0001f)*CurrentAoA*CurrentAoA + 0.0642*CurrentAoA + 0.0062f;

	}
	
	CurrentHeight = AMyProject12Pawn::GetActorLocation().Z*0.01;
	density = (2.676e-9)*FGenericPlatformMath::Pow(CurrentHeight, 2) + (-0.0001071)*CurrentHeight + 1.283;//matlab cf poly
	CurrentLift = 0.5*density*IAS*IAS*Flanker_WingArea*cl;//Unit in Newton
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
	//     f(x) =  a0 + a1*cos(x*w) + b1*sin(x*w) + a2*cos(2 * x*w) + b2*sin(2 * x*w)
	/*
	a0 =      0.8749  (0.8307, 0.9191)
	a1 =      -1.124  (-1.181, -1.067)
	b1 =     -0.1479  (-0.2022, -0.09373)
	a2 =      0.2703  (0.2503, 0.2903)
	b2 =     0.09096  (0.05577, 0.1261)
	w =     0.09369  (0.0897, 0.09768)
	AoA>30
	y = -0.0094x2 + 0.7045x - 11.144;

	*/
	if (CurrentAoA<30)
	{ 
	cd = 0.8749 - 1.124*FMath::Cos(FMath::DegreesToRadians(CurrentAoA)*0.09369) - 0.1479*FMath::Sin(FMath::DegreesToRadians(CurrentAoA)*0.09369)
		+ 0.2703*FMath::Cos(2 * FMath::DegreesToRadians(CurrentAoA)*0.09369) + 0.09096*FMath::Sin(2 * FMath::DegreesToRadians(CurrentAoA)*0.09369);
	}
	else
	{
		cd = -0.0094*CurrentAoA*CurrentAoA + 0.7045*CurrentAoA - 11.144;
	}
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
	CurrentDrag = density*IAS*IAS*Flanker_WingArea*cd;
}



void AMyProject12Pawn::MomentumSim()
{
	//Acceleration in self axial and vertical direction
	Pc_Acc_A_X = (CurrentThrust*FMath::Cos(FMath::DegreesToRadians(CurrentAoA)) - CurrentDrag- CurrentWeight*FMath::Sin(FMath::DegreesToRadians(CurrentTheta))) / CurrentMass;
	Pc_Acc_A_Y = (CurrentThrust*FMath::Sin(FMath::DegreesToRadians(CurrentAoA)) + CurrentLift - CurrentWeight*FMath::Cos(FMath::DegreesToRadians(CurrentTheta)))/CurrentMass;


    //float NewForwardSpeed = Pc_CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc); don't know what New
    //FWspeed is
	Pc_CurrentForwardSpeed = Pc_CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * Pc_Acc_A_X);//speed synthesis
	Pc_CurrentLocalVS = Pc_CurrentLocalVS + (GetWorld()->GetDeltaSeconds() * Pc_Acc_A_Y);

	//CG's velocity converted to self cordinate velocity
	Sc_X_Velocity = Pc_CurrentForwardSpeed*FMath::Cos(FMath::DegreesToRadians(CurrentAoA));
	Sc_Y_Velocity = Pc_CurrentForwardSpeed*FMath::Sin(FMath::DegreesToRadians(CurrentAoA));

	//TAS->IAS
	/*Linear model Poly2:
	f(x) = p1*x^2 + p2*x + p3
	Coefficients (with 95% confidence bounds):
	p1 =   2.481e-09  (2.384e-09, 2.578e-09)
	p2 =  -9.083e-05  (-9.205e-05, -8.961e-05)
	p3 =      0.9948  (0.9916, 0.998)*/
	Aero_PR = (2.481e-9)*CurrentHeight*CurrentHeight - (9.083e-5)*CurrentHeight + 0.9948;
	IAS = Pc_CurrentForwardSpeed*FMath::Sqrt(Aero_PR);

}

void AMyProject12Pawn::SonicCorrection() //used to correct speed of Sound
{
	//y = -0.0041x + 340.87  SpdM1 vs ASL

	Motion_CurrentASL= AMyProject12Pawn::GetActorLocation().Z*0.01;
	LocalSonicSpeed = -0.0041*Motion_CurrentASL + 340.87f;
	MachSpeed = Pc_CurrentForwardSpeed/LocalSonicSpeed;
}


void AMyProject12Pawn::BalanceCalculation()
{
	Aero_PR = (2.481e-9)*CurrentHeight*CurrentHeight - (9.083e-5)*CurrentHeight + 0.9948;
	IAS = Pc_CurrentForwardSpeed*FMath::Sqrt(Aero_PR);
	//CurrentWeight = (Flanker_Empty_Weight + Flanker_Fuel_Weight)*9.8;//unit: Newton
	//level flight CL and AoA 
	SteadyCL = 2 * CurrentWeight / (IAS*IAS*Flanker_WingArea*density);
	if (MachSpeed < 0.2)
	{
		//y = 11.637x - 0.0627
		StartAlpha = 11.637*SteadyCL - 0.0267f;
	}
	else
	{
		//y = 16.144x - 0.0285
		StartAlpha = 16.144*SteadyCL - 0.0285f;
	}
	
	//
}
