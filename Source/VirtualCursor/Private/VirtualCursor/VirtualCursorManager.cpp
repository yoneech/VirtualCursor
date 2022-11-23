#include "VirtualCursor/VirtualCursorManager.h"
#include "VirtualCursor/ExtendedAnalogCursor.h"
#include "VirtualCursor/CursorSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "GameMapsSettings.h"

#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogVirtualCursorManager);


void UVirtualCursorManager::Initialize(FSubsystemCollectionBase& Collection)
{
}


void UVirtualCursorManager::Deinitialize()
{
	// When the local player is ending, cleanup the analog cursor and reset the shared ptr variable
	DisableAnalogCursor();
	Cursor.Reset();
}


void UVirtualCursorManager::EnableAnalogCursor(const bool bUseLeftStick)
{
	// Ensure that slate and the world is valid
	if (FSlateApplication::IsInitialized() && GetWorld())
	{
		const float CursorRadius = GetDefault<UCursorSettings>()->GetAnalogCursorRadius();

		// If the shared ptr isnt tied to a valid obj then create one and connect the two
		if (!IsCursorValid())
		{
			Cursor = MakeShareable(new FExtendedAnalogCursor(GetLocalPlayer(), GetWorld(), CursorRadius));
			
			const EAnalogStick Stick = bUseLeftStick ? EAnalogStick::Left : EAnalogStick::Right;
			Cursor->SetStick(Stick);
		}

		// Check that we're not re-adding it(which counts as a duplicate)
		if (!ContainsGamepadCursorInputProcessor())
		{
			FSlateApplication::Get().RegisterInputPreProcessor(Cursor);

			// Center the player's cursor in the center of their viewport based on the viewport UV.
			// NOTE: If the viewport size is not evenly divisible, the location of the cursor may not be
			// in the exact center.
			FVector2D viewportCenterUV = GetViewportUVCenterFromLocalPlayer(GetLocalPlayer());
			const FVector2D cursorStartingPosition = GetLocalPlayer()->ViewportClient->Viewport->ViewportToVirtualDesktopPixel(viewportCenterUV);

			GetLocalPlayer()->GetSlateUser()->SetCursorPosition(cursorStartingPosition);

			// Fake a button click so that the window has capture and focus. 
			// Otherwise the cursor will not appear until the user presses a button.
			FSlateApplication::Get().ProcessMouseButtonDownEvent(nullptr,FPointerEvent(GetLocalPlayer()->GetControllerId(),0, cursorStartingPosition, cursorStartingPosition,true));
		}
		FSlateApplication::Get().SetCursorRadius(CursorRadius);
	}
}


void UVirtualCursorManager::DisableAnalogCursor()
{
	if (FSlateApplication::IsInitialized())
	{
		// Dont try to remove it if we already removed it, you may say overkill I say ensuring safeguards
		if (ContainsGamepadCursorInputProcessor())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(Cursor);
		}
		FSlateApplication::Get().SetCursorRadius(0.0f);
	}
}


void UVirtualCursorManager::ToggleCursorDebug()
{
	if (IsCursorValid())
	{
		Cursor->bDebugging = !Cursor->bDebugging;
		const FString BoolResult = ((Cursor->bDebugging) ? "true" : "false");
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Emerald, "Cursor Debug: " + BoolResult);
	}
}


void UVirtualCursorManager::ToggleAnalogDebug()
{
	if (IsCursorValid())
	{
		Cursor->bAnalogDebug = !Cursor->bAnalogDebug;
		const FString BoolResult = ((Cursor->bAnalogDebug) ? "true" : "false");
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Emerald, "Analog Debug: " + BoolResult);
	}
}


bool UVirtualCursorManager::IsCursorDebugActive() const
{
	if (IsCursorValid())
	{
		return Cursor->bDebugging;
	}
	return false;
}


bool UVirtualCursorManager::IsAnalogDebugActive() const
{
	if (IsCursorValid())
	{
		return Cursor->bAnalogDebug;
	}
	return false;
}


bool UVirtualCursorManager::IsCursorOverInteractableWidget() const
{
	if (IsCursorValid())
	{
		return Cursor->IsHovered();
	}
	return false;
}


bool UVirtualCursorManager::IsCursorValid() const
{
	return Cursor.IsValid();
}


bool UVirtualCursorManager::ContainsGamepadCursorInputProcessor() const
{
	if (FSlateApplication::IsInitialized())
	{
		// Continue if we're using a valid cursor
		if (IsCursorValid())
		{
			// Returns true if index is greater than -1, false if less than/equal to -1
			const int32 FoundIndex = FSlateApplication::Get().FindInputPreProcessor(Cursor);
			return (FoundIndex > -1);
		}
	}
	return false;
}


FVector2D UVirtualCursorManager::GetViewportUVCenterFromLocalPlayer(ULocalPlayer* localPlayer) const
{
	FVector2D viewportCenterUV = FVector2D(0.5f, 0.5f);
	if (!IsValid(localPlayer) || !IsValid(GEngine) || !IsValid(GEngine->GameViewport))
		// Something has not been initialized.
		return viewportCenterUV;

	UGameMapsSettings* settings = GetMutableDefault<UGameMapsSettings>();
	if (!settings->bUseSplitscreen)
		// We're using a shared screen.
		return viewportCenterUV;

	const int32 playerIndex = localPlayer->GetControllerId();
	if (playerIndex >= GEngine->GameViewport->MaxSplitscreenPlayers || playerIndex < 0)
		// The index we've received is not valid.
		return viewportCenterUV;

	// I'm sure there's a more mathematical way of doing this, but why complicate things?
	ESplitScreenType::Type splitScreenType = GEngine->GameViewport->GetCurrentSplitscreenConfiguration();
	if (splitScreenType == ESplitScreenType::TwoPlayer_Vertical)
	{
		// Get the center UVs for a two player vertically split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.25f, 0.5f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.75f, 0.5f);
	}
	else if (splitScreenType == ESplitScreenType::TwoPlayer_Horizontal)
	{
		// Get the center UVs for a two player horizontally split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.5, 0.25f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.5f, 0.75f);
	}
	else if (splitScreenType == ESplitScreenType::ThreePlayer_Vertical)
	{
		// Get the center UVs for a three player vertically split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.165f, 0.5f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.495, 0.5f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.825, 0.5f);
	}
	else if (splitScreenType == ESplitScreenType::ThreePlayer_Horizontal)
	{
		// Get the center UVs for a three player horizontally split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.5f, 0.165f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.5f, 0.495);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.5f, 0.825);
	}
	else if (splitScreenType == ESplitScreenType::ThreePlayer_FavorTop)
	{
		// Get the center UVs for a three player split screen which favors the top.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.5f, 0.25f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.25, 0.75f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.75f, 0.75f);
	}
	else if (splitScreenType == ESplitScreenType::ThreePlayer_FavorBottom)
	{
		// Get the center UVs for a three player split screen which favors the bottom.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.25, 0.25f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.75f, 0.25f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.5f, 0.75f);
	}
	else if (splitScreenType == ESplitScreenType::FourPlayer_Grid)
	{
		// Get the center UVs for a four player grid split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.25f, 0.25f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.75f, 0.25f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.25f, 0.75f);
		else if (playerIndex == 3)
			viewportCenterUV = FVector2D(0.75f, 0.75f);
	}
	else if (splitScreenType == ESplitScreenType::FourPlayer_Vertical)
	{
		// Get the center UVs for a four player vertically split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.125f, 0.5f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.375f, 0.5f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.625f, 0.5f);
		else if (playerIndex == 3)
			viewportCenterUV = FVector2D(0.875f, 0.5f);
	}
	else if (splitScreenType == ESplitScreenType::FourPlayer_Horizontal)
	{
		// Get the center UVs for a four player horizontally split screen.
		if (playerIndex == 0)
			viewportCenterUV = FVector2D(0.5f, 0.125f);
		else if (playerIndex == 1)
			viewportCenterUV = FVector2D(0.5f, 0.375f);
		else if (playerIndex == 2)
			viewportCenterUV = FVector2D(0.5f, 0.625f);
		else if (playerIndex == 3)
			viewportCenterUV = FVector2D(0.5f, 0.875f);
	}

	return viewportCenterUV;
}