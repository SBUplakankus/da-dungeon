// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/FrontendUISubsystem.h"

#include "FrontendDebugHelper.h"
#include "Engine/AssetManager.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/Widget_ActivatableBase.h"

UFrontendUISubsystem* UFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		
		return UGameInstance::GetSubsystem<UFrontendUISubsystem>(World->GetGameInstance());
	}
	
	return nullptr;
}

bool UFrontendUISubsystem::ShouldCreateSubsystem(UObject* Subsystem) const
{
	if (!CastChecked<UGameInstance>(Subsystem)->IsDedicatedServerInstance())
	{
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(), FoundClasses);
		
		return FoundClasses.IsEmpty();
	}
	
	return false;
}

void UFrontendUISubsystem::RegisterCreatedPrimaryLayoutWidget(UWidget_PrimaryLayout* InCreateWidget)
{
	check(InCreateWidget);
	CreatedPrimaryLayout = InCreateWidget;
	
	Debug::Print("Primary layout widget stored");
}

void UFrontendUISubsystem::PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag,
	TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass, TFunction<void(EAsyncPushWidgetState, UWidget_ActivatableBase*)> AsyncPushStateCallback) const
{
	if (InSoftWidgetClass.IsNull())
	{
		return;
	}
	
	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(InSoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([InSoftWidgetClass, this, InWidgetStackTag, AsyncPushStateCallback]()
		{
			UClass* LoadedWidgetClass = InSoftWidgetClass.Get();
			check(LoadedWidgetClass && CreatedPrimaryLayout);
			
			UCommonActivatableWidgetContainerBase* FoundWidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
			checkf(FoundWidgetStack, TEXT("No widget stack found for tag %s. Did you forget RegisterWidgetStack?"), *InWidgetStackTag.ToString());
			if (!FoundWidgetStack) return;
			
			UWidget_ActivatableBase* CreatedWidget = FoundWidgetStack->AddWidget<UWidget_ActivatableBase>(
				LoadedWidgetClass, 
				[AsyncPushStateCallback](UWidget_ActivatableBase& CreatedWidgetInstance)
				{
					AsyncPushStateCallback(EAsyncPushWidgetState::OnCreatedBeforePush, &CreatedWidgetInstance);
				}
			);
			
			AsyncPushStateCallback(EAsyncPushWidgetState::OnCreatedAfterPush, CreatedWidget);
		}));
}
