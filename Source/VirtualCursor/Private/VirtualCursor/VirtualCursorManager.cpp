#include "VirtualCursor/VirtualCursorManager.h"
#include "VirtualCursor/ExtendedAnalogCursor.h"
#include "VirtualCursor/CursorSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "GameMapsSettings.h"
#include "Slate/SGameLayerManager.h"
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

			// Center the cursor in the player's respective viewport.
			// This may not perfectly center if the viewport geometry's size is not properly divisible.
			FVector2D cursorStartingPosition = FVector2D::ZeroVector;
			if (IsValid(GEngine) && IsValid(GEngine->GameViewport))
			{
				TSharedPtr<IGameLayerManager> gameLayerManager = GEngine->GameViewport->GetGameLayerManager();
				if (gameLayerManager.IsValid())
				{
					// Get the player's viewport geometry to determine where the cursor should be placed.
					const FGeometry playerViewportGeometry = gameLayerManager->GetPlayerWidgetHostGeometry(GetLocalPlayer());
					cursorStartingPosition = playerViewportGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
				}
			}

			if (GetLocalPlayer()->GetSlateUser().IsValid())
			{
				GetLocalPlayer()->GetSlateUser()->SetCursorPosition(cursorStartingPosition);
			}
			else
			{
				// BUG! Why does this happen? Shouldn't the slate users be ready on application start?
				UE_LOG(LogTemp, Warning, TEXT("UVirtualCursorManager::EnableAnalogCursor -- SlateUser for player %d is not valid."), GetLocalPlayer()->GetControllerId());
			}

			// Fake a button click so that the window has capture and focus. 
			// Otherwise the cursor will not appear until the user presses a button.
			FSlateApplication::Get().ProcessMouseButtonDownEvent(nullptr,FPointerEvent(GetLocalPlayer()->GetControllerId(), 0, cursorStartingPosition, cursorStartingPosition, 0.0f, true));
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


void UVirtualCursorManager::SetClampCursorToViewport(bool bNewClamp)
{
	if (Cursor.IsValid())
		Cursor->SetClampToViewport(bNewClamp);
}


void UVirtualCursorManager::ToggleClampCursorToViewport()
{
	SetClampCursorToViewport(!CheckClampCursorToViewport());
}


bool UVirtualCursorManager::CheckClampCursorToViewport() const 
{
	if (Cursor.IsValid())
		return Cursor->CheckClampToViewport();

	return false;
}