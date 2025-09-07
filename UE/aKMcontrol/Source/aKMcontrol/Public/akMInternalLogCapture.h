#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDevice.h"

/**
 * Lightweight UE log capture for specific categories, thread-safe with ring buffer.
 */
class AKMCONTROL_API FAkMInternalLogCapture : public FOutputDevice
{
public:
	FAkMInternalLogCapture();
	explicit FAkMInternalLogCapture(const TSet<FName>& InCategories);

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

	// Thread-safe snapshot
	void GetSnapshot(TArray<FString>& OutLines) const;
	void Clear();
	void SetMaxLines(int32 InMaxLines);
	void SetCategories(const TSet<FName>& InCategories);

private:
	mutable FCriticalSection Mutex;
	TArray<FString> Lines;
	int32 MaxLines = 1000;
	TSet<FName> Categories;
};


