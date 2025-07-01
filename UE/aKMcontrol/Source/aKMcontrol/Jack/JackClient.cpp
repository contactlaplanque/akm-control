// Fill out your copyright notice in the Description page of Project Settings.

#include "JackClient.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"

FJackClient::FJackClient()
	: JackClient(nullptr)
	, ConnectionStatus(EJackConnectionStatus::Disconnected)
	, bServerWasStarted(false)
{
}

FJackClient::~FJackClient()
{
	UnregisterAllPorts();
	Disconnect();
}

bool FJackClient::Connect(const FString& ClientName)
{
	if (IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("JackClient: Already connected"));
		return true;
	}

	ConnectionStatus = EJackConnectionStatus::Connecting;

	// Convert FString to C string
	FTCHARToUTF8 Convert(*ClientName);
	const char* ClientNameStr = Convert.Get();

	// Open connection to Jack server
	jack_status_t Status;
	JackClient = jack_client_open(ClientNameStr, JackNullOption, &Status);

	if (JackClient == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to connect to Jack server. Status: 0x%x"), Status);
		ConnectionStatus = EJackConnectionStatus::Failed;
		return false;
	}

	// Check connection status
	if (Status & JackServerStarted)
	{
		bServerWasStarted = true;
		UE_LOG(LogTemp, Log, TEXT("JackClient: Jack server started by this client"));
	}
	else
	{
		bServerWasStarted = false;
		UE_LOG(LogTemp, Log, TEXT("JackClient: Connected to existing Jack server"));
	}

	if (Status & JackNameNotUnique)
	{
		FString ActualName = FString(jack_get_client_name(JackClient));
		UE_LOG(LogTemp, Log, TEXT("JackClient: Unique name assigned: %s"), *ActualName);
	}

	// Set up callbacks
	jack_on_shutdown(JackClient, ShutdownCallback, this);
	jack_set_xrun_callback(JackClient, XRunCallback, this);

	ConnectionStatus = EJackConnectionStatus::Connected;
	UE_LOG(LogTemp, Log, TEXT("JackClient: Successfully connected to Jack server"));
	UE_LOG(LogTemp, Log, TEXT("JackClient: Sample rate: %d, Buffer size: %d"), GetSampleRate(), GetBufferSize());

	return true;
}

void FJackClient::Disconnect()
{
	if (JackClient != nullptr)
	{
		Deactivate();
		jack_client_close(JackClient);
		JackClient = nullptr;
		ConnectionStatus = EJackConnectionStatus::Disconnected;
		UE_LOG(LogTemp, Log, TEXT("JackClient: Disconnected from Jack server"));
	}
}

bool FJackClient::IsConnected() const
{
	return JackClient != nullptr && ConnectionStatus == EJackConnectionStatus::Connected;
}

FString FJackClient::GetClientName() const
{
	if (JackClient != nullptr)
	{
		return FString(jack_get_client_name(JackClient));
	}
	return TEXT("");
}

uint32 FJackClient::GetSampleRate() const
{
	if (JackClient != nullptr)
	{
		return jack_get_sample_rate(JackClient);
	}
	return 0;
}

uint32 FJackClient::GetBufferSize() const
{
	if (JackClient != nullptr)
	{
		return jack_get_buffer_size(JackClient);
	}
	return 0;
}

float FJackClient::GetCPUUsage() const
{
	if (JackClient != nullptr)
	{
		return jack_cpu_load(JackClient);
	}
	return 0.0f;
}

jack_port_t* FJackClient::RegisterPort(const FString& PortName, const FString& PortType, uint32 Flags, uint32 BufferSize)
{
	if (!IsConnected())
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Cannot register port - not connected"));
		return nullptr;
	}

	FTCHARToUTF8 PortNameConvert(*PortName);
	FTCHARToUTF8 PortTypeConvert(*PortType);

	jack_port_t* Port = jack_port_register(JackClient, PortNameConvert.Get(), PortTypeConvert.Get(), Flags, BufferSize);
	
	if (Port == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to register port: %s"), *PortName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("JackClient: Successfully registered port: %s"), *PortName);
	}

	return Port;
}

bool FJackClient::UnregisterPort(jack_port_t* Port)
{
	if (!IsConnected() || Port == nullptr)
	{
		return false;
	}

	int Result = jack_port_unregister(JackClient, Port);
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to unregister port"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("JackClient: Successfully unregistered port"));
	return true;
}

TArray<FString> FJackClient::GetAvailablePorts(const FString& NamePattern, const FString& TypePattern, uint32 Flags)
{
	TArray<FString> Ports;

	if (!IsConnected())
	{
		return Ports;
	}

	FTCHARToUTF8 NamePatternConvert(*NamePattern);
	FTCHARToUTF8 TypePatternConvert(*TypePattern);

	const char** JackPorts = jack_get_ports(JackClient, 
		NamePattern.IsEmpty() ? nullptr : NamePatternConvert.Get(),
		TypePattern.IsEmpty() ? nullptr : TypePatternConvert.Get(),
		Flags);

	if (JackPorts != nullptr)
	{
		for (int i = 0; JackPorts[i] != nullptr; i++)
		{
			Ports.Add(FString(JackPorts[i]));
		}
		jack_free(JackPorts);
	}

	return Ports;
}

bool FJackClient::ConnectPorts(const FString& SourcePort, const FString& DestinationPort)
{
	if (!IsConnected())
	{
		return false;
	}

	FTCHARToUTF8 SourceConvert(*SourcePort);
	FTCHARToUTF8 DestConvert(*DestinationPort);

	int Result = jack_connect(JackClient, SourceConvert.Get(), DestConvert.Get());
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to connect ports %s -> %s"), *SourcePort, *DestinationPort);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("JackClient: Connected ports %s -> %s"), *SourcePort, *DestinationPort);
	return true;
}

bool FJackClient::DisconnectPorts(const FString& SourcePort, const FString& DestinationPort)
{
	if (!IsConnected())
	{
		return false;
	}

	FTCHARToUTF8 SourceConvert(*SourcePort);
	FTCHARToUTF8 DestConvert(*DestinationPort);

	int Result = jack_disconnect(JackClient, SourceConvert.Get(), DestConvert.Get());
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to disconnect ports %s -> %s"), *SourcePort, *DestinationPort);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("JackClient: Disconnected ports %s -> %s"), *SourcePort, *DestinationPort);
	return true;
}

bool FJackClient::SetProcessCallback(JackProcessCallback ProcessCallback, void* Arg)
{
	if (!IsConnected())
	{
		return false;
	}

	int Result = jack_set_process_callback(JackClient, ProcessCallback, Arg);
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to set process callback"));
		return false;
	}

	return true;
}

bool FJackClient::SetShutdownCallback(JackShutdownCallback ShutdownCallback, void* Arg)
{
	if (!IsConnected())
	{
		return false;
	}

	jack_on_shutdown(JackClient, ShutdownCallback, Arg);
	return true;
}

bool FJackClient::SetXRunCallback(JackXRunCallback XRunCallback, void* Arg)
{
	if (!IsConnected())
	{
		return false;
	}

	int Result = jack_set_xrun_callback(JackClient, XRunCallback, Arg);
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to set xrun callback"));
		return false;
	}

	return true;
}

bool FJackClient::Activate()
{
	if (!IsConnected())
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Cannot activate - not connected"));
		return false;
	}

	int Result = jack_activate(JackClient);
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to activate client"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("JackClient: Client activated"));
	return true;
}

bool FJackClient::Deactivate()
{
	if (!IsConnected())
	{
		return false;
	}

	int Result = jack_deactivate(JackClient);
	if (Result != 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to deactivate client"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("JackClient: Client deactivated"));
	return true;
}

FJackPerformanceMetrics FJackClient::GetPerformanceMetrics() const
{
	FJackPerformanceMetrics Metrics = PerformanceMetrics;
	
	// Update real-time values
	if (IsConnected())
	{
		Metrics.CPUUsage = GetCPUUsage();
		// Note: XRuns and other metrics are updated in callbacks
	}
	
	return Metrics;
}

// Static callback implementations
int FJackClient::ProcessCallback(jack_nframes_t Nframes, void* Arg)
{
	FJackClient* Client = static_cast<FJackClient*>(Arg);
	if (Client)
	{
		Client->UpdatePerformanceMetrics();
	}
	return 0;
}

void FJackClient::ShutdownCallback(void* Arg)
{
	FJackClient* Client = static_cast<FJackClient*>(Arg);
	if (Client)
	{
		Client->ConnectionStatus = EJackConnectionStatus::Failed;
		UE_LOG(LogTemp, Warning, TEXT("JackClient: Jack server shutdown detected"));
	}
}

int FJackClient::XRunCallback(void* Arg)
{
	FJackClient* Client = static_cast<FJackClient*>(Arg);
	if (Client)
	{
		Client->PerformanceMetrics.XRuns++;
		UE_LOG(LogTemp, Warning, TEXT("JackClient: XRun detected! Total XRuns: %d"), Client->PerformanceMetrics.XRuns);
	}
	return 0;
}

void FJackClient::UpdatePerformanceMetrics()
{
	// Update performance metrics in real-time callback
	// This is called from the Jack audio thread, so keep it minimal
	PerformanceMetrics.AudioCallbackTime = FPlatformTime::Seconds() * 1000.0f; // Convert to milliseconds
}

bool FJackClient::WasServerStarted() const
{
	return bServerWasStarted;
}

FString FJackClient::GetServerInfo() const
{
	if (!IsConnected())
	{
		return TEXT("Not connected to Jack server");
	}

	FString Info = FString::Printf(TEXT("Jack Server Info:\n"));
	Info += FString::Printf(TEXT("  Server Started by this client: %s\n"), bServerWasStarted ? TEXT("Yes") : TEXT("No"));
	Info += FString::Printf(TEXT("  Sample Rate: %d Hz\n"), GetSampleRate());
	Info += FString::Printf(TEXT("  Buffer Size: %d frames\n"), GetBufferSize());
	Info += FString::Printf(TEXT("  CPU Usage: %.2f%%\n"), GetCPUUsage());
	
	// Calculate latency
	float LatencyMs = (float)GetBufferSize() * 1000.0f / (float)GetSampleRate();
	Info += FString::Printf(TEXT("  Latency: %.2f ms\n"), LatencyMs);
	
	return Info;
}

bool FJackClient::RegisterAudioPorts(int32 NumInputs, int32 NumOutputs, const FString& BaseName)
{
	if (!IsConnected())
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Cannot register ports - not connected"));
		return false;
	}

	// Clear existing ports
	UnregisterAllPorts();

	bool bSuccess = true;

	// Register input ports
	for (int32 i = 0; i < NumInputs; i++)
	{
		FString PortName = FString::Printf(TEXT("%s_in_%d"), *BaseName, i + 1);
		jack_port_t* Port = RegisterPort(PortName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
		
		if (Port != nullptr)
		{
			InputPorts.Add(Port);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to register input port %d"), i + 1);
			bSuccess = false;
		}
	}

	// Register output ports
	for (int32 i = 0; i < NumOutputs; i++)
	{
		FString PortName = FString::Printf(TEXT("%s_out_%d"), *BaseName, i + 1);
		jack_port_t* Port = RegisterPort(PortName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
		
		if (Port != nullptr)
		{
			OutputPorts.Add(Port);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to register output port %d"), i + 1);
			bSuccess = false;
		}
	}

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("JackClient: Successfully registered %d input and %d output ports"), NumInputs, NumOutputs);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("JackClient: Failed to register some ports"));
	}

	return bSuccess;
}

void FJackClient::UnregisterAllPorts()
{
	// Unregister input ports
	for (jack_port_t* Port : InputPorts)
	{
		if (Port != nullptr)
		{
			UnregisterPort(Port);
		}
	}
	InputPorts.Empty();

	// Unregister output ports
	for (jack_port_t* Port : OutputPorts)
	{
		if (Port != nullptr)
		{
			UnregisterPort(Port);
		}
	}
	OutputPorts.Empty();

	UE_LOG(LogTemp, Log, TEXT("JackClient: Unregistered all ports"));
}

TArray<jack_port_t*> FJackClient::GetInputPorts() const
{
	return InputPorts;
}

TArray<jack_port_t*> FJackClient::GetOutputPorts() const
{
	return OutputPorts;
} 