#include "VirtualCursor/ExtendedAnalogCursor.h"
#include "VirtualCursor/CursorSettings.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateUser.h"
#include "GameMapsSettings.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/SViewport.h"


bool IsWidgetInteractable(const TSharedPtr<SWidget> Widget)
{
	return Widget.IsValid() && Widget->IsInteractable();
}


FExtendedAnalogCursor::FExtendedAnalogCursor(ULocalPlayer* InLocalPlayer, UWorld* InWorld, float _Radius)
	: bDebugging(false)
	, bAnalogDebug(false)
	, Velocity(FVector2D::ZeroVector)
	, CurrentPosition(FLT_MAX, FLT_MAX)
	, LastCursorDirection(FVector2D::ZeroVector)
	, HoveredWidgetName(NAME_None)
	, bIsUsingAnalogCursor(false)
	, Radius(FMath::Max<float>(_Radius, 16.0f))
	, PlayerContext(InLocalPlayer, InWorld)
{
	ensure(PlayerContext.IsValid());

	const UCursorSettings* settings = GetMutableDefault<UCursorSettings>();
	bClampToViewport = settings->GetDefaultClampToViewport();
}


FExtendedAnalogCursor::FExtendedAnalogCursor(class APlayerController* PlayerController, float _Radius)
	: bDebugging(false)
	, bAnalogDebug(false)
	, Velocity(FVector2D::ZeroVector)
	, CurrentPosition(FLT_MAX, FLT_MAX)
	, LastCursorDirection(FVector2D::ZeroVector)
	, HoveredWidgetName(NAME_None)
	, bIsUsingAnalogCursor(false)
	, Radius(FMath::Max<float>(_Radius, 16.0f))
	, PlayerContext(PlayerController)
{
	ensure(PlayerContext.IsValid());

	const UCursorSettings* settings = GetMutableDefault<UCursorSettings>();
	bClampToViewport = settings->GetDefaultClampToViewport();
}


int32 FExtendedAnalogCursor::GetOwnerUserIndex() const
{
	if (ULocalPlayer* LocalPlayer = PlayerContext.GetLocalPlayer())
	{
		return LocalPlayer->GetControllerId();
	}
	return 0;
}


bool FExtendedAnalogCursor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	// If we assigned the first gamepad to player 2, we need to modify the
	// user index of the event.
	// This bool is not cached because some games may allow for this to 
	// change in real-time through game settings.
	UGameMapsSettings* gameSettings = GetMutableDefault<UGameMapsSettings>();
	const bool bSkipGamepadPlayer1 = gameSettings->GetSkipAssigningGamepadToPlayer1();
	FKeyEvent newInKeyEvent = InKeyEvent;
	if (bSkipGamepadPlayer1)
		newInKeyEvent = FKeyEvent(
			InKeyEvent.GetKey(),
			InKeyEvent.GetModifierKeys(),
			InKeyEvent.GetUserIndex() + 1,
			InKeyEvent.IsRepeat(),
			InKeyEvent.GetCharacter(),
			InKeyEvent.GetKeyCode()
		);

	// So we only read from the correct player index (to handle for local coop)
	if (!IsRelevantInput(newInKeyEvent))
	{
		// If the index of whoever pressed a key is not this cursor's index then its another local player(so they dont control the inputs)
		return false;
	}

	const FKey& PressedKey = newInKeyEvent.GetKey();

	if (newInKeyEvent.IsRepeat())
	{
		if (bDebugging)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "KEY: " + PressedKey.ToString() + " Held");
		}
	}
	else
	{
		PressedKeys.Add(PressedKey);
		if (bDebugging)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "KEY: " + PressedKey.ToString() + " Pressed");
		}
	}

	return FAnalogCursor::HandleKeyDownEvent(SlateApp, newInKeyEvent);
}


bool FExtendedAnalogCursor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	// If we assigned the first gamepad to player 2, we need to modify the
	// user index of the event.
	// This bool is not cached because some games may allow for this to 
	// change in real-time through game settings.
	UGameMapsSettings* gameSettings = GetMutableDefault<UGameMapsSettings>();
	const bool bSkipGamepadPlayer1 = gameSettings->GetSkipAssigningGamepadToPlayer1();
	FKeyEvent newInKeyEvent = InKeyEvent;
	if (bSkipGamepadPlayer1)
		newInKeyEvent = FKeyEvent(
			InKeyEvent.GetKey(),
			InKeyEvent.GetModifierKeys(),
			InKeyEvent.GetUserIndex() + 1,
			InKeyEvent.IsRepeat(),
			InKeyEvent.GetCharacter(),
			InKeyEvent.GetKeyCode()
		);

	// So we only read from the correct player index(to handle for local coop)
	if (!IsRelevantInput(newInKeyEvent))
	{
		// If the index of whoever pressed a key is not this cursor's index then its another local player(so they dont control the inputs)
		return false;
	}

	const FKey& ReleasedKey = newInKeyEvent.GetKey();

	PressedKeys.Remove(ReleasedKey);
	if (bDebugging)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, "KEY: " + ReleasedKey.ToString() + " Released");
	}

	return FAnalogCursor::HandleKeyUpEvent(SlateApp, newInKeyEvent);
}


bool FExtendedAnalogCursor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	// If we assigned the first gamepad to player 2, we need to modify the
	// user index of the event.
	// This bool is not cached because some games may allow for this to 
	// change in real-time through game settings.
	UGameMapsSettings* gameSettings = GetMutableDefault<UGameMapsSettings>();
	const bool bSkipGamepadPlayer1 = gameSettings->GetSkipAssigningGamepadToPlayer1();
	FAnalogInputEvent newInAnalogInputEvent = InAnalogInputEvent;
	if (bSkipGamepadPlayer1)
		newInAnalogInputEvent = FAnalogInputEvent(
			InAnalogInputEvent.GetKey(),
			InAnalogInputEvent.GetModifierKeys(),
			InAnalogInputEvent.GetUserIndex() + 1,
			InAnalogInputEvent.IsRepeat(),
			InAnalogInputEvent.GetCharacter(),
			InAnalogInputEvent.GetKeyCode(),
			InAnalogInputEvent.GetAnalogValue()
		);

	// So we only read from the correct player index(to handle for local coop)
	if (!IsRelevantInput(newInAnalogInputEvent))
	{
		// If the index of whoever pressed a key is not this cursor's index then its another local player(so they dont control the inputs)
		return false;
	}

	// Prevent Slate from swallowing events that aren't relevant to our virtual cursor
	if (!IsCursorStickInput(newInAnalogInputEvent))
	{
		return false;
	}

	if (bAnalogDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "ANALOG: " + newInAnalogInputEvent.GetKey().ToString());
	}
	return FAnalogCursor::HandleAnalogInputEvent(SlateApp, newInAnalogInputEvent);
}


bool FExtendedAnalogCursor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	// If we assigned the first gamepad to player 2, we need to modify the
	// user index of the event only if it came from a gamepad.
	// This bool is not cached because some games may allow for this to 
	// change in real-time through game settings.
	UGameMapsSettings* gameSettings = GetMutableDefault<UGameMapsSettings>();
	const bool bSkipGamepadPlayer1 = gameSettings->GetSkipAssigningGamepadToPlayer1();
	FPointerEvent newMouseEvent = MouseEvent;
	if (bSkipGamepadPlayer1 && MouseEvent.GetPressedButtons().Num() <= 0 && !MouseEvent.IsTouchEvent())
		// GetPressedButtons is empty if this event came from simulating a mouse press through a gamepad.
		// This is true in UE4.25, but may not be true in future versions.
		newMouseEvent = FPointerEvent(
			newMouseEvent.GetUserIndex() + 1,
			newMouseEvent.GetPointerIndex(),
			newMouseEvent.GetScreenSpacePosition(),
			newMouseEvent.GetLastScreenSpacePosition(),
			newMouseEvent.GetPressedButtons(),
			newMouseEvent.GetEffectingButton(),
			newMouseEvent.GetWheelDelta(),
			newMouseEvent.GetModifierKeys()
		);

	// So we only read from the correct player index(to handle for local coop)
	if (!IsRelevantInput(newMouseEvent))
	{
		// If the index of whoever pressed a key is not this cursor's index then its another local player(so they dont control the inputs)
		return false;
	}

	const FKey& PressedKey = newMouseEvent.GetEffectingButton();
	if (PressedKeys.Contains(PressedKey))
	{
		if (bDebugging)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "MOUSE: " + PressedKey.ToString() + " Held");
		}
	}
	else
	{
		PressedKeys.Add(PressedKey);
		if (bDebugging)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "MOUSE: " + PressedKey.ToString() + " Pressed");
		}
	}
	return false;
}


bool FExtendedAnalogCursor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	// If we assigned the first gamepad to player 2, we need to modify the
	// user index of the event only if it came from a gamepad.
	// This bool is not cached because some games may allow for this to 
	// change in real-time through game settings.
	UGameMapsSettings* gameSettings = GetMutableDefault<UGameMapsSettings>();
	const bool bSkipGamepadPlayer1 = gameSettings->GetSkipAssigningGamepadToPlayer1();
	FPointerEvent newMouseEvent = MouseEvent;
	if (bSkipGamepadPlayer1 && MouseEvent.GetPressedButtons().Num() <= 0 && !MouseEvent.IsTouchEvent())
		// GetPressedButtons is empty if this event came from simulating a mouse press through a gamepad.
		// This is true in UE4.25, but may not be true in future versions.
		newMouseEvent = FPointerEvent(
			newMouseEvent.GetUserIndex() + 1,
			newMouseEvent.GetPointerIndex(),
			newMouseEvent.GetScreenSpacePosition(),
			newMouseEvent.GetLastScreenSpacePosition(),
			newMouseEvent.GetPressedButtons(),
			newMouseEvent.GetEffectingButton(),
			newMouseEvent.GetWheelDelta(),
			newMouseEvent.GetModifierKeys()
		);

	// So we only read from the correct player index(to handle for local coop)
	if (!IsRelevantInput(newMouseEvent))
	{
		// If the index of whoever pressed a key is not this cursor's index then its another local player(so they dont control the inputs)
		return false;
	}

	const FKey& ReleasedKey = newMouseEvent.GetEffectingButton();
	if (bDebugging)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, "MOUSE: " + ReleasedKey.ToString() + " Released");
	}
	PressedKeys.Remove(ReleasedKey);
	return false;
}


void FExtendedAnalogCursor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	TSharedPtr<FSlateUser> slateUser = SlateApp.GetUser(GetOwnerUserIndex());
	if (PlayerContext.IsValid() && PlayerContext.GetPlayerController() && slateUser.IsValid())
	{
		const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PlayerContext.GetPlayerController());
		const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(FMath::RoundToInt(ViewportSize.X), FMath::RoundToInt(ViewportSize.Y)));

		const UCursorSettings* Settings = GetDefault<UCursorSettings>();

		// Set the current position if we haven't already
		static const float MouseMoveSizeBuffer = 2.0f;
		const FVector2D CurrentPositionTruc = FVector2D(FMath::TruncToFloat(CurrentPosition.X), FMath::TruncToFloat(CurrentPosition.Y));
		if (CurrentPositionTruc != slateUser->GetCursorPosition())
		{
			CurrentPosition = slateUser->GetCursorPosition();
			Velocity = FVector2D::ZeroVector;
			LastCursorDirection = FVector2D::ZeroVector;
			bIsUsingAnalogCursor = false;
			FSlateApplication::Get().SetCursorRadius(0.0f);
		}

		// Cache the old position
		const FVector2D OldPosition = CurrentPosition;

		// Figure out if we should clamp the speed or not
		const float MaxSpeedNoHover = Settings->GetMaxAnalogCursorSpeed() * DPIScale;
		const float MaxSpeedHover = Settings->GetMaxAnalogCursorSpeedWhenHovered() * DPIScale;
		const float DragCoNoHover = Settings->GetAnalogCursorDragCoefficient() * DPIScale;
		const float DragCoHovered = Settings->GetAnalogCursorDragCoefficientWhenHovered() * DPIScale;
		const float MinCursorSpeed = Settings->GetMinAnalogCursorSpeed() * DPIScale;

		HoveredWidgetName = NAME_None;
		float DragCo = DragCoNoHover;

		// Part of base class now
		MaxSpeed = MaxSpeedNoHover;

		// See if we are hovered over a widget or not
		FWidgetPath WidgetPath = SlateApp.LocateWindowUnderMouse(OldPosition, SlateApp.GetInteractiveTopLevelWindows());
		if (WidgetPath.IsValid())
		{
			for (int32 i = WidgetPath.Widgets.Num() - 1; i >= 0; --i)
			{
				// Grab the widget
				FArrangedWidget& ArrangedWidget = WidgetPath.Widgets[i];
				TSharedRef<SWidget> Widget = ArrangedWidget.Widget;

				// See if it is acceptable or not
				if (IsWidgetInteractable(Widget))
				{
					HoveredWidgetName = Widget->GetType();
					DragCo = DragCoHovered;
					MaxSpeed = MaxSpeedHover;
					break;
				}
			}
		}

		// Grab the cursor acceleration
		const FVector2D AccelFromAnalogStick = GetAnalogCursorAccelerationValue(GetAnalogValues(AnalogStick), DPIScale);

		FVector2D NewAccelerationThisFrame = FVector2D::ZeroVector;
		if (!Settings->GetAnalogCursorNoAcceleration())
		{
			// Calculate a new velocity. RK4.
			if (!AccelFromAnalogStick.IsZero() || !Velocity.IsZero())
			{
				const FVector2D A1 = (AccelFromAnalogStick - (DragCo * Velocity)) * DeltaTime;
				const FVector2D A2 = (AccelFromAnalogStick - (DragCo * (Velocity + (A1 * 0.5f)))) * DeltaTime;
				const FVector2D A3 = (AccelFromAnalogStick - (DragCo * (Velocity + (A2 * 0.5f)))) * DeltaTime;
				const FVector2D A4 = (AccelFromAnalogStick - (DragCo * (Velocity + A3))) * DeltaTime;
				NewAccelerationThisFrame = (A1 + (2.0f * A2) + (2.0f * A3) + A4) / 6.0f;
				Velocity += NewAccelerationThisFrame;
			}
		}
		else
		{
			// Else, use what is coming straight from the analog stick
			Velocity = AccelFromAnalogStick;
		}

		// If we are smaller than out min speed, zero it out
		const float VelSizeSq = Velocity.SizeSquared();
		if (VelSizeSq < (MinCursorSpeed * MinCursorSpeed))
		{
			Velocity = FVector2D::ZeroVector;
		}
		else if (VelSizeSq > (MaxSpeed * MaxSpeed))
		{
			//also cap us if we are larger than our max speed
			Velocity = Velocity.GetSafeNormal() * MaxSpeed;
		}

		//bool bClamped = false;
		//FBox2D viewportClamp = FBox2D();

		//// Check to see if we should clamp the cursor's position.
		//if (bClampToViewport && IsValid(GEngine) && IsValid(GEngine->GameViewport))
		//{
		//	TSharedPtr<IGameLayerManager> gameLayerManager = GEngine->GameViewport->GetGameLayerManager();
		//	if (gameLayerManager.IsValid())
		//	{
		//		// Get the player's viewport geometry to check that our cursor is within the bounds.
		//		const FGeometry viewportGeometry = gameLayerManager->GetPlayerWidgetHostGeometry(PlayerContext.GetLocalPlayer());
		//		const FVector2D playerViewportSize = viewportGeometry.GetLocalSize().RoundToVector();
		//		FVector2D localPosition = USlateBlueprintLibrary::AbsoluteToLocal(viewportGeometry, CurrentPosition + (Velocity * DeltaTime));

		//		// Check if we need to clamp. Reset the velocity if we do.
		//		if (localPosition.X + Radius > playerViewportSize.X || localPosition.X - Radius < 0.0f) 
		//		{
		//			bClamped = true;
		//			Velocity.X = 0.0f;
		//		}
		//		if (localPosition.Y + Radius > playerViewportSize.Y || localPosition.Y - Radius < 0.0f)
		//		{
		//			bClamped = true;
		//			Velocity.Y = 0.0f;
		//		}

		//		if (bClamped)
		//		{
		//			// Only perform this calculation if we actually need to clamp the position.
		//			viewportClamp.Min = viewportGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.0f, 0.0f));
		//			viewportClamp.Max = viewportGeometry.GetAbsolutePositionAtCoordinates(FVector2D(1.0f, 1.0f));
		//		}

		//		UE_LOG(LogTemp, Warning, TEXT("%d | Loc: %s | Max: %s | Min: %s"), GetOwnerUserIndex(), *localPosition.ToString(), *viewportClamp.Max.ToString(), *viewportClamp.Min.ToString());

		//		// Why don't we just set the CurrentPosition here? We could, but because we only
		//		// clamp one of the velocity axes, we still need to perform the velocity calculations below.
		//		// This just seemed easier, to me. - Yoneech
		//	}
		//}
		const FVector2D nextPosition = CurrentPosition + (Velocity * DeltaTime);
		FVector2D clampedPosition;
		GetAbsoluteClampedPosition(nextPosition, clampedPosition);


		//store off the last cursor direction
		if (!Velocity.IsZero())
		{
			LastCursorDirection = Velocity.GetSafeNormal();
		}

		// Update the new position
		//CurrentPosition += (Velocity * DeltaTime);
		CurrentPosition = bClampToViewport ? clampedPosition : nextPosition;

		// Update the cursor position
		UpdateCursorPosition(SlateApp, slateUser.ToSharedRef(), CurrentPosition);

		// If we get here, and we are moving the stick, then hooray
		if (!AccelFromAnalogStick.IsZero())
		{
			bIsUsingAnalogCursor = true;
			FSlateApplication::Get().SetCursorRadius(Settings->GetAnalogCursorRadius() * DPIScale);
		}
	}
}


void FExtendedAnalogCursor::SetClampToViewport(bool bNewClampToViewport)
{
	bClampToViewport = bNewClampToViewport;

	if (!bClampToViewport || !bIsUsingAnalogCursor || !IsValid(GEngine) || !IsValid(GEngine->GameViewport))
		return;

	FVector2D clampedPosition;
	if (!GetAbsoluteClampedPosition(CurrentPosition, clampedPosition))
		return;

	CurrentPosition = clampedPosition;
	Velocity.Set(0.0f, 0.0f);
	LastCursorDirection.Set(0.0f, 0.0f);

	UpdateCursorPosition(FSlateApplication::Get(), PlayerContext.GetLocalPlayer()->GetSlateUser().ToSharedRef(), CurrentPosition);
}


bool FExtendedAnalogCursor::GetAbsoluteClampedPosition(const FVector2D& inPosition, FVector2D& outPosition)
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport))
		return false;

	TSharedPtr<IGameLayerManager> gameLayerManager = GEngine->GameViewport->GetGameLayerManager();
	if (!gameLayerManager.IsValid())
		return false;

	bool bClamped = false;
	const FGeometry viewportGeometry = gameLayerManager->GetPlayerWidgetHostGeometry(PlayerContext.GetLocalPlayer());
	const FVector2D playerViewportSize = viewportGeometry.GetLocalSize().RoundToVector();
	FVector2D localPosition = USlateBlueprintLibrary::AbsoluteToLocal(viewportGeometry, inPosition);

	if (localPosition.X + Radius > playerViewportSize.X)
	{
		bClamped = true;
		localPosition.X = playerViewportSize.X - Radius;
	}
	if (localPosition.X - Radius < 0.0f)
	{
		bClamped = true;
		localPosition.X = Radius;
	}
	if (localPosition.Y + Radius > playerViewportSize.Y)
	{
		bClamped = true;
		localPosition.Y = playerViewportSize.Y - Radius;
	}
	if (localPosition.Y - Radius < 0.0f)
	{
		bClamped = true;
		localPosition.Y = Radius;
	}

	outPosition = USlateBlueprintLibrary::LocalToAbsolute(viewportGeometry, localPosition);
	return bClamped;
}


FVector2D FExtendedAnalogCursor::GetAnalogCursorAccelerationValue(const FVector2D& InAnalogValues, const float DPIScale) const
{
	const UCursorSettings* Settings = GetDefault<UCursorSettings>();

	FVector2D RetValue = FVector2D::ZeroVector;
	if (const FRichCurve* AccelerationCurve = Settings->GetAnalogCursorAccelerationCurve())
	{
		const float DeadZoneSize = Settings->GetAnalogCursorDeadZone();
		const float AnalogValSize = InAnalogValues.Size();
		if (AnalogValSize > DeadZoneSize)
		{
			RetValue = AccelerationCurve->Eval(AnalogValSize) * InAnalogValues.GetSafeNormal() * DPIScale;
			RetValue *= Settings->GetAnalogCursorAccelerationMultiplier() * DPIScale;
		}
	}
	return RetValue;
}


bool FExtendedAnalogCursor::IsCursorStickInput(const FAnalogInputEvent& AnalogInputEvent) const
{
	return
	  (AnalogStick == EAnalogStick::Left &&
	  	(AnalogInputEvent.GetKey() == EKeys::Gamepad_LeftX || AnalogInputEvent.GetKey() == EKeys::Gamepad_LeftY)
	  ) ||
	  (AnalogStick == EAnalogStick::Right &&
	  	(AnalogInputEvent.GetKey() == EKeys::Gamepad_RightX || AnalogInputEvent.GetKey() == EKeys::Gamepad_RightY)
	  );
}


