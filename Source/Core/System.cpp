#include "PCH.h"
#include "System.h"
#include "Debug.h"

namespace System
{
    ////////////////////////////////////////////////////////////////////////////////
    // TODO: wakeup timer doesn't seem to work
    bool sleep(float wakeTimeSeconds)
    {
        // Make sure we can actually go to sleep
        SYSTEM_POWER_CAPABILITIES sysPowCab = { 0 };
        if (CallNtPowerInformation(SystemPowerCapabilities, NULL, 0, &sysPowCab, sizeof(SYSTEM_POWER_CAPABILITIES)) != STATUS_SUCCESS)
        {
            Debug::log_debug() << "Unable to retrieve system power management information." << Debug::end;
            return false;
        }
        if (!sysPowCab.SystemS1 && !sysPowCab.SystemS2 && !sysPowCab.SystemS3)
        {
            Debug::log_debug() << "Sleep mode is disabled." << Debug::end;
            return false;
        }

        // Get active power scheme
        GUID* pPwrGUID;
        DWORD ret = PowerGetActiveScheme(NULL, &pPwrGUID);
        if (ret != ERROR_SUCCESS)
        {
            Debug::log_debug() << "Error retrieving current power scheme." << Debug::end;
            return false;
        }

        // Enable wake by timers
        GUID subGUID = GUID_SLEEP_SUBGROUP;
        GUID BriGUID = GUID_ALLOW_RTC_WAKE;
        ret = PowerWriteACValueIndex(NULL, pPwrGUID, &subGUID, &BriGUID, 1);
        if (ret != ERROR_SUCCESS)
        {
            Debug::log_debug() << "Error registering sleep mode." << Debug::end;
            return false;
        }
        
        // Register the wake timer
        if (wakeTimeSeconds >= 0)
        {
            LARGE_INTEGER WaitTime;
            WaitTime.HighPart = 0;
            WaitTime.QuadPart = LONGLONG(wakeTimeSeconds * -10000000LL);

            HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
            if (SetWaitableTimer(hTimer, &WaitTime, 0, NULL, NULL, TRUE) == 0)
            {
                Debug::log_debug() << "Unable to register wake timer." << Debug::end;
                return false;
            }
            if (GetLastError() == ERROR_NOT_SUPPORTED)
            {
                Debug::log_debug() << "Unable to register wake timer (wake by timer not supported)." << Debug::end;
                return false;
            }
        }

        // Leave a log message
        Debug::log_debug() << "Entering sleep mode" << Debug::end;
        if (wakeTimeSeconds >= 0)
        {
            Debug::log_debug() << "Waking in " << wakeTimeSeconds << " seconds" << Debug::end;
        }

        // Put the computer to sleep
        if (SetSuspendState(FALSE, FALSE, FALSE) == 0)
        {
            Debug::log_debug() << "Unable to put computer to sleep." << Debug::end;
            return false;
        }

        // Successfully entered sleep mode
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    void copyToClipboard(std::string const& data)
    {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size());
        memcpy(GlobalLock(hMem), data.data(), data.size());
        GlobalUnlock(hMem);
        OpenClipboard(0);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
    }
}