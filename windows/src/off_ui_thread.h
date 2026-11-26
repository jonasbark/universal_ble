#pragma once

#include <windows.h>
#include <combaseapi.h>

#include <coroutine>

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

/// TODO: not implemented yet.
struct resume_off_ui_thread {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  void await_resume() const noexcept {}
};

}  // namespace universal_ble
