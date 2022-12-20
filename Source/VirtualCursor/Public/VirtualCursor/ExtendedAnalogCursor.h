#pragma once

#include "Framework/Application/AnalogCursor.h"


class VIRTUALCURSOR_API FExtendedAnalogCursor : public FAnalogCursor
{
public:

	typedef FAnalogCursor Super;

	FExtendedAnalogCursor(ULocalPlayer* InLocalPlayer, UWorld* InWorld, float _Radius);
	FExtendedAnalogCursor(class APlayerController* PlayerController, float _Radius);

	virtual ~FExtendedAnalogCursor() {}

	virtual int32 GetOwnerUserIndex() const override;

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;

	/** 
	* Sets whether the cursor is clamped to the viewport. 
	* This will also immediately clamp the cursor's position if
	* it was outside the viewport.
	* 
	* This will also reset the cursor's position even if the user 
	* has last used a mouse, which is not necessarily an intended 
	* effect but one that may be a necessary evil. - Yoneech
	*/
	void SetClampToViewport(bool bNewClampToViewport);

	FORCEINLINE FName GetHoveredWidgetName() const
	{
		return HoveredWidgetName;
	}

	FORCEINLINE bool IsHovered() const
	{
		return HoveredWidgetName != NAME_None;
	}

	FORCEINLINE FVector2D GetCurrentPosition() const
	{
		return CurrentPosition;
	}

	FORCEINLINE FVector2D GetVelocity() const
	{
		return Velocity;
	}

	FORCEINLINE bool GetIsUsingAnalogCursor() const
	{
		return bIsUsingAnalogCursor;
	}

	FORCEINLINE FVector2D GetLastCursorDirection() const
	{
		return LastCursorDirection;
	}

	FORCEINLINE float GetRadius() const
	{
		return Radius;
	}

	FORCEINLINE void SetStick(const EAnalogStick CursorMovementStick)
	{
		AnalogStick = CursorMovementStick;
	}

	FORCEINLINE bool CheckClampToViewport() const
	{
		return bClampToViewport;
	}

	uint8 bDebugging : 1;

	uint8 bAnalogDebug : 1;

protected:

	bool GetAbsoluteClampedPosition(const FVector2D& inPosition, FVector2D& outPosition);

private:

	/** Takes in values from the analog stick, returns a vector that represents acceleration */
	FVector2D GetAnalogCursorAccelerationValue(const FVector2D& InAnalogValues, float DPIScale) const;

	/** Test whether the input is for the correct stick */
	bool IsCursorStickInput(const FAnalogInputEvent& AnalogInputEvent) const;

	/** Current velocity of the cursor */
	FVector2D Velocity;

	/** Current position of the cursor. Stored outside of ICursor's position to avoid float->int32 truncation */
	FVector2D CurrentPosition;

	/** Unit vector derived from Velocity */
	FVector2D LastCursorDirection;

	/** The name of the hovered widget */
	FName HoveredWidgetName;

	/** Is this thing even active right now? */
	bool bIsUsingAnalogCursor;

	/** True if the cursor should clamp to the player's viewport. */
	bool bClampToViewport;

	/** The radius of the analog cursor */
	float Radius;

	FLocalPlayerContext PlayerContext;

	TSet<FKey> PressedKeys;

	EAnalogStick AnalogStick = EAnalogStick::Left;
};
