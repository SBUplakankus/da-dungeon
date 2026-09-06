// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "FrontendUISubsystem.generated.h"

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
	
private:
	UPROPERTY(Transient)
	UWidget_PrimaryLayout* CreatedPrimaryLayout;
	
};
