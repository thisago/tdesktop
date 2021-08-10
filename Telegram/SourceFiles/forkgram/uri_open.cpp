/*
Author: 23rd.
*/

#include "forkgram/uri_open.h"

#include "base/platform/base_platform_info.h"

#ifdef Q_OS_WIN
// For Windows 10.
#pragma warning(push)
// class has virtual functions, but destructor is not virtual
#pragma warning(disable:4265)
#include <winrt/base.h>
#pragma warning(pop)

#include <winrt/Windows.System.h>
// For Windows 7.
#include <windows.h>
#include <shellapi.h>

#else // !Q_OS_WIN
#include <QDesktopServices>
#endif

// Since Qt 5.15 removes the second colon from "customUri://https://gnu.org"
// for some reason, we have to ignore its implementation
// and open links directly through the Win API.

namespace Forkgram {

void OpenUrlWithCustomUri(const QString &url) {
#ifdef Q_OS_WIN
	if (Platform::IsWindows10OrGreater()) {
		winrt::Windows::System::Launcher::LaunchUriAsync(
			winrt::Windows::Foundation::Uri(
				winrt::to_hstring(url.toStdString())),
			winrt::Windows::System::LauncherOptions());
	} else {
		ShellExecute(0, 0, (LPCWSTR)url.utf16(), 0, 0 , SW_SHOW );
	}
#else // !Q_OS_WIN
	QDesktopServices::openUrl(url);
#endif // !Q_OS_WIN
}

} // namespace Forkgram
