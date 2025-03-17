using UnrealBuildTool;
using System.IO;

public class EasyFileDialog : ModuleRules
{
	public EasyFileDialog(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Dodaj publiczne ścieżki nagłówkowe
		PublicIncludePaths.AddRange(
			new string[] {
				"/usr/include/gtk-3.0/gtk",     // Ścieżka do GTK
				"/usr/include/gtk-3.0",          // Ścieżka do GTK ogólna
				"/usr/include/glib-2.0",         // Ścieżka do glib
				"/usr/lib/x86_64-linux-gnu/glib-2.0/include", // Dodatkowa ścieżka glib
				"/usr/lib64/glib-2.0/include",   // Ścieżka do glibconfig.h
				// ... inne wymagane ścieżki
			}
		);

		// Dodaj prywatne ścieżki nagłówkowe
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... inne prywatne ścieżki
			}
		);

		// Dodaj zależności dla publicznych modułów
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... inne publiczne zależności
			}
		);

		// Dodaj zależności dla prywatnych modułów
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... inne prywatne zależności
			}
		);

		// Dodaj dynamicznie ładowane moduły
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... moduły ładowane dynamicznie
			}
		);

		// Linkowanie do bibliotek GTK
		string GtkLibPath = "/usr/lib/x86_64-linux-gnu"; // Ścieżka do bibliotek GTK
		PublicAdditionalLibraries.Add(Path.Combine(GtkLibPath, "libgtk-3.so"));
		PublicAdditionalLibraries.Add(Path.Combine(GtkLibPath, "libgdk-3.so"));
		PublicAdditionalLibraries.Add(Path.Combine(GtkLibPath, "libglib-2.0.so")); // Linkowanie do biblioteki glib

		// Możesz również użyć pkg-config dla GTK i glib
		string GtkPkgConfig = "pkg-config --libs --cflags gtk+-3.0";
		PublicAdditionalLibraries.Add(GtkPkgConfig);
	}
}
