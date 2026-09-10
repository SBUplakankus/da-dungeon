// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncActions/AsyncAction_PushSoftWidget.h"

#include "Subsystems/FrontendUISubsystem.h"
#include "Widgets/Widget_ActivatableBase.h"

UAsyncAction_PushSoftWidget* UAsyncAction_PushSoftWidget::PushSoftWidget(const UObject* WorldContextObject,
                                                                         APlayerController* OwningPlayerController, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,
                                                                         UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InWidgetStackTag, bool bFocusOnNewlyPushedWidget)
{
	checkf(!InSoftWidgetClass.IsNull(), TEXT("Push Soft Widget to Stack was passed a null class"));
	
	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			auto Node = NewObject<UAsyncAction_PushSoftWidget>();
			Node->CachedOwningWorld = World;
			Node->CachedOwningPC = OwningPlayerController;
			Node->CachedSoftWidgetClass = InSoftWidgetClass;
			Node->CachedWidgetStackTag = InWidgetStackTag;
			Node->bCachedFocusOnNewlyPushedWidget = bFocusOnNewlyPushedWidget;
			
			Node->RegisterWithGameInstance(World);
			
			return Node;
		}
	}
	
	return nullptr;
}

void UAsyncAction_PushSoftWidget::Activate()
{
	if (CachedSoftWidgetClass.IsNull())
	{
		SetReadyToDestroy();
		return;
	}

	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(CachedOwningWorld.Get());
	if (!FrontendUISubsystem) return;
	
	FrontendUISubsystem->PushSoftWidgetToStackAsync(CachedWidgetStackTag, CachedSoftWidgetClass, 
		[this](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			switch (InPushState) 
			{
			case EAsyncPushWidgetState::OnCreatedBeforePush:
				
				PushedWidget->SetOwningPlayer(CachedOwningPC.Get());
				
				OnWidgetCreatedBeforePush.Broadcast(PushedWidget);
				
				break;
			case EAsyncPushWidgetState::OnCreatedAfterPush:
				
				OnWidgetCreatedAfterPush.Broadcast(PushedWidget);
				
				if (bCachedFocusOnNewlyPushedWidget)
				{
					if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
					{
						WidgetToFocus->SetFocus();
					}
				}
				
				SetReadyToDestroy();
				
				break;
			}
		}
		);
}
