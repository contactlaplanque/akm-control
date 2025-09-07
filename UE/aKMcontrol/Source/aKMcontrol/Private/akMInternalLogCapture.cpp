#include "akMInternalLogCapture.h"
#include "Misc/ScopeLock.h"

FAkMInternalLogCapture::FAkMInternalLogCapture()
{
	Categories = { FName(TEXT("LogJackAudioLink")), FName(TEXT("LogSpatServer")) };
}

FAkMInternalLogCapture::FAkMInternalLogCapture(const TSet<FName>& InCategories)
	: Categories(InCategories)
{
}

void FAkMInternalLogCapture::Serialize(const TCHAR* V, ELogVerbosity::Type /*Verbosity*/, const FName& Category)
{
	if (!Categories.Contains(Category))
	{
		return;
	}
	FString Line = FString::Printf(TEXT("[%s] %s"), *Category.ToString(), V);
	FScopeLock Lock(&Mutex);
	Lines.Add(MoveTemp(Line));
	if (Lines.Num() > MaxLines)
	{
		const int32 NumToRemove = Lines.Num() - MaxLines;
		Lines.RemoveAt(0, NumToRemove, EAllowShrinking::No);
	}
}

void FAkMInternalLogCapture::GetSnapshot(TArray<FString>& OutLines) const
{
	FScopeLock Lock(&Mutex);
	OutLines = Lines;
}

void FAkMInternalLogCapture::Clear()
{
	FScopeLock Lock(&Mutex);
	Lines.Reset();
}

void FAkMInternalLogCapture::SetMaxLines(int32 InMaxLines)
{
	FScopeLock Lock(&Mutex);
	MaxLines = FMath::Max(1, InMaxLines);
	if (Lines.Num() > MaxLines)
	{
		const int32 NumToRemove = Lines.Num() - MaxLines;
		Lines.RemoveAt(0, NumToRemove, EAllowShrinking::No);
	}
}

void FAkMInternalLogCapture::SetCategories(const TSet<FName>& InCategories)
{
	FScopeLock Lock(&Mutex);
	Categories = InCategories;
}


