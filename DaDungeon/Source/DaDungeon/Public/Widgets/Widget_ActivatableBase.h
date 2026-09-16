// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Controllers/FrontendPlayerController.h"
#include "Widget_ActivatableBase.generated.h"

/**
 * 
 */
UCLASS()
class DADUNGEON_API UWidget_ActivatableBase : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	UFUNCTION(BlueprintPure)
	AFrontendPlayerController* GetOwningFrontendPlayerController();
	
private:
	TWeakObjectPtr<AFrontendPlayerController> CachedOwningFrontendPC;
};
