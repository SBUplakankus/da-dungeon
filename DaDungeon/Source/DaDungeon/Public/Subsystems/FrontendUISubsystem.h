// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "FrontendUISubsystem.generated.h"

class UWidget_PrimaryLayout;

enum class EAsyncPushWidgetState : uint8
{
	OnCreatedBeforePush,
	OnCreatedAfterPush
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonDescriptionTextUpdatedDelegate, 
	UFrontendCommonButtonBase*, BroadcastingButton, 
	FText, DescriptionText);

/**
 * 
 */
UCLASS()
class DADUNGEON_API UFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UFrontendUISubsystem* Get(const UObject* WorldContextObject);
	
	virtual bool ShouldCreateSubsystem(UObject* Subsystem) const override;
	
	UFUNCTION(BlueprintCallable)
	void RegisterCreatedPrimaryLayoutWidget(UWidget_PrimaryLayout* InCreateWidget);
	
	UPROPERTY(BlueprintAssignable)
	FOnButtonDescriptionTextUpdatedDelegate OnButtonDescriptionTextUpdated;
	
	void PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass, 
		TFunction<void(EAsyncPushWidgetState, UWidget_ActivatableBase*)> AsyncPushStateCallback) const;
	
	void PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMessage,
		TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback) const;
	
private:
	UPROPERTY(Transient)
	UWidget_PrimaryLayout* CreatedPrimaryLayout;
	
};
