// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonTextBlock.h"
#include "Components/DynamicEntryBox.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_ConfirmScreen.generated.h"

USTRUCT(BlueprintType)
struct FConfirmScreenButtonInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConfirmScreenButtonType ConfirmScreenButtonType = EConfirmScreenButtonType::Unknown;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ButtonTextToDisplay;
	
	
};

UCLASS()
class DADUNGEON_API UConfirmScreenInfoObject : public UObject
{
	GENERATED_BODY()
	
public:
	static UConfirmScreenInfoObject* CreateOKScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	static UConfirmScreenInfoObject* CreateYesNoScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	static UConfirmScreenInfoObject* CreateOKCancelScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	
	UPROPERTY(Transient)
	FText ScreenTitle;
	
	UPROPERTY(Transient)
	FText ScreenMessage;
	
	UPROPERTY(Transient)
	TArray<FConfirmScreenButtonInfo> AvailableScreenButtons;
};
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class DADUNGEON_API UWidget_ConfirmScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
	
public:
	void InitConfirmScreen(UConfirmScreenInfoObject* InScreenInfoObject, TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallback);
	
private:
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_Title;
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_Message;
	
	UPROPERTY(meta = (BindWidget))
	UDynamicEntryBox* DynamicEntryBox_Buttons;
};
