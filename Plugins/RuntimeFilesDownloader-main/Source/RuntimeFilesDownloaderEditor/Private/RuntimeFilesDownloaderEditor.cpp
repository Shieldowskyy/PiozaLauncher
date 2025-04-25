// Georgy Treshchev 2024.

#include "RuntimeFilesDownloaderEditor.h"

DEFINE_LOG_CATEGORY(LogRuntimeFilesDownloaderEditor);
#define LOCTEXT_NAMESPACE "FRuntimeFilesDownloaderEditorModule"

void FRuntimeFilesDownloaderEditorModule::StartupModule()
{
	// Android-specific code has been removed
}

void FRuntimeFilesDownloaderEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRuntimeFilesDownloaderEditorModule, RuntimeFilesDownloaderEditor)
