// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/FrontendUISubsystem.h"

#include "FrontendDebugHelper.h"

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
