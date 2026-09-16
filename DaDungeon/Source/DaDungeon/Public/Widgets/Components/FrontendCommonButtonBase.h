// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "FrontendCommonButtonBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class DADUNGEON_API UFrontendCommonButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetButtonText(FText InText);
	
private:
	virtual void NativePreConstruct() override;
	virtual void NativeOnCurrentTextStyleChanged() override;
	
	//**** Bound Widgets ****//
	UPROPERTY(meta = (BindWidgetOptional))
	UCommonTextBlock* CommonTextBlock_ButtonText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	FText ButtonDisplayText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	bool bUserUpperCaseForButtonText = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	FText ButtonDescriptionText;
};
