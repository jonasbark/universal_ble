#pragma once

#include <windows.h>
#include <combaseapi.h>

#include <coroutine>

#include <winrt/base.h>

namespace universal_ble {

/// True on a single-threaded apartment, e.g. Flutter's platform (UI) thread.
inline bool IsOnSingleThreadedApartment() noexcept {
  APTTYPE type{};
  APTTYPEQUALIFIER qualifier{};
  if (FAILED(CoGetApartmentType(&type, &qualifier))) {
    return false;
  }
  return type == APTTYPE_STA || type == APTTYPE_MAINSTA;
}

/// Awaitable that moves the rest of a coroutine off a single-threaded
/// apartment onto the thread pool (MTA).
///
/// C++/WinRT resumes `co_await` on the apartment the coroutine was on, so a
/// fire_and_forget started from a Flutter method handler otherwise runs every
/// step between its awaits on the UI thread. Each Bluetooth call there is a
/// cross-process call into the Bluetooth service (svchost); when that service
/// stalls (e.g. while a peripheral reboots), the window stops responding.
/// Off an STA this is a no-op.
struct resume_off_ui_thread {
  bool await_ready() const noexcept { return !IsOnSingleThreadedApartment(); }

  void await_suspend(std::coroutine_handle<> handle) const {
    if (!TrySubmitThreadpoolCallback(
            [](PTP_CALLBACK_INSTANCE, void *context) {
              std::coroutine_handle<>::from_address(context)();
            },
            handle.address(), nullptr)) {
      winrt::throw_last_error();
    }
  }

  void await_resume() const noexcept {}
};

}  // namespace universal_ble
