// GetAndSetRHI.cpp

#include "GetAndSetRHI.h"
#include "HardwareInfo.h"
#include "RHI.h"

#if PLATFORM_LINUX
#include <cstdlib>
#include <atomic>
#include <thread>
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  WHY THE OLD CODE RETURNED 60 ON WAYLAND
//
//  The old regex grepped for '*' to find the active mode. On Wayland (Nobara /
//  KDE Plasma), xrandr talks to XWayland — and XWayland often doesn't mark
//  the current mode with '*' because the compositor owns the display, not Xorg.
//  So the grep matched nothing, the pipe returned "", atoi("") == 0, fallback 60.
//
//  FIX — two-strategy approach:
//
//  Strategy 1 (primary): xrandr --verbose
//    Verbose mode always prints hardware timing for every mode, including a
//    "v: ... clock NNN.NNHz" line. The active mode has "*current" on it.
//    We parse the clock Hz from the vertical timing line right after *current.
//    This works correctly under XWayland because the dotclock comes from
//    the DRM kernel layer, not from what X thinks it set.
//
//  Strategy 2 (fallback): /sys/class/drm sysfs
//    The kernel writes the active mode resolution to
//    /sys/class/drm/card0-DP-X/modes (first line = active).
//    We use that resolution to do a targeted xrandr grep, which is more
//    reliable than grepping the entire output for a bare '*'.
//
//  Both run async — game thread never blocks.
// ─────────────────────────────────────────────────────────────────────────────

#if PLATFORM_LINUX
namespace
{
	std::atomic<int32> GCachedRefreshRate{ 0 };

	// Strategy 1: parse actual hardware vrefresh from xrandr --verbose
	// Looks for the line after "*current" that contains "clock NNN.NNHz"
	int32 TryXrandrVerbose()
	{
		FILE* Pipe = popen(
			"xrandr --verbose 2>/dev/null | "
			"awk '/*current/{found=1} "
			"     found && /v:/{match($0,/clock +([0-9.]+)Hz/,a); "
			"                   if(a[1]+0>0){print a[1]+0; exit}} "
			"     /^[A-Z]/{found=0}'",
			"r"
		);
		if (!Pipe) return 0;

		char Buffer[32] = {};
		int32 Rate = 0;
		if (fgets(Buffer, sizeof(Buffer), Pipe) != nullptr)
		{
			double Val = atof(Buffer);
			if (Val > 0.0)
				Rate = static_cast<int32>(Val + 0.5); // round 164.99 -> 165
		}
		pclose(Pipe);
		return Rate;
		}

		// Strategy 2: /sys/class/drm connector + targeted xrandr grep
		// Gets the active resolution from the kernel, then finds its rate in xrandr.
		int32 TryDrmSysfs()
		{
			FILE* Pipe = popen(
				"for f in /sys/class/drm/card*-*/status; do "
				"  [ \"$(cat \"$f\" 2>/dev/null)\" = \"connected\" ] || continue; "
				"  res=$(head -1 \"$(dirname $f)/modes\" 2>/dev/null); "
				"  [ -n \"$res\" ] || continue; "
				"  rate=$(xrandr 2>/dev/null | grep -A30 \"$res\" | "
				"         grep -oE '[0-9]+\\.[0-9]+\\*' | head -1 | "
				"         grep -oE '^[0-9]+'); "
				"  [ -n \"$rate\" ] && echo \"$rate\" && exit 0; "
				"done",
				"r"
			);
			if (!Pipe) return 0;

			char Buffer[16] = {};
			int32 Rate = 0;
			if (fgets(Buffer, sizeof(Buffer), Pipe) != nullptr)
			{
				int32 Parsed = FCString::Atoi(UTF8_TO_TCHAR(Buffer));
				if (Parsed > 0) Rate = Parsed;
			}
			pclose(Pipe);
			return Rate;
		}

		void FetchRefreshRateAsync()
		{
			int32 Rate = TryXrandrVerbose();

			if (Rate <= 0)
				Rate = TryDrmSysfs();

			if (Rate <= 0)
				Rate = 60;

			GCachedRefreshRate.store(Rate, std::memory_order_relaxed);
		}

		} // anonymous namespace
		#endif // PLATFORM_LINUX

		// ─────────────────────────────────────────────────────────────────────────────
		//  Public API
		// ─────────────────────────────────────────────────────────────────────────────

		FString UGetAndSetRHI::GetCurrentRhiName()
		{
			return FHardwareInfo::GetHardwareInfo(NAME_RHI);
		}

		int32 UGetAndSetRHI::GetCurrentMonitorRefreshRate()
		{
			#if PLATFORM_LINUX

			int32 Cached = GCachedRefreshRate.load(std::memory_order_relaxed);

			if (Cached > 0)
				return Cached;

			if (Cached == 0)
			{
				int32 Expected = 0;
				if (GCachedRefreshRate.compare_exchange_strong(
					Expected, -1, std::memory_order_relaxed, std::memory_order_relaxed))
				{
					std::thread(FetchRefreshRateAsync).detach();
				}
			}

			// Thread is in flight — return safe default, next call gets the real value
			return 60;

			#else // Windows / other platforms

			FScreenResolutionArray Resolutions;
			if (RHIGetAvailableResolutions(Resolutions, false))
			{
				int32 MaxRefreshRate = 0;
				for (const FScreenResolutionRHI& Res : Resolutions)
				{
					if (static_cast<int32>(Res.RefreshRate) > MaxRefreshRate)
						MaxRefreshRate = static_cast<int32>(Res.RefreshRate);
				}
				if (MaxRefreshRate > 0)
					return MaxRefreshRate;
			}
			return 60;

			#endif
		}