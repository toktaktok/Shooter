// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomUtils.h"

FVector2D CustomUtils::GetViewportSize()
{
	FVector2D ViewportSize{};

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	return ViewportSize;
}
