// A connect attempt starts on Flutter's platform thread (an STA). Every
// Bluetooth call it makes talks to the Bluetooth service in svchost, and a
// stalled call there must not freeze the window: once a coroutine has
// awaited resume_off_ui_thread, nothing after it may run on the STA.
#include "off_ui_thread.h"

#include <winrt/Windows.Foundation.h>

#include <atomic>
#include <chrono>
#include <iostream>

namespace {

#define CHECK(expression)                                        \
  do {                                                           \
    if (!(expression)) {                                         \
      std::cerr << "CHECK failed: " << #expression << std::endl; \
      return 1;                                                  \
    }                                                            \
  } while (false)

std::atomic<DWORD> continuation_thread = 0;
std::atomic<bool> continuation_on_sta = true;
std::atomic<bool> stalled_call_done = false;

// Shaped like ConnectAsync: hop first, then a call into a stalled service.
winrt::fire_and_forget ConnectShapedCoroutine(
    std::chrono::milliseconds stall) {
  co_await universal_ble::resume_off_ui_thread{};
  continuation_thread = GetCurrentThreadId();
  continuation_on_sta = universal_ble::IsOnSingleThreadedApartment();
  Sleep(static_cast<DWORD>(stall.count()));  // e.g. FromBluetoothAddressAsync
  stalled_call_done = true;
}

winrt::fire_and_forget AlreadyOffUiThread(DWORD &thread_after) {
  co_await universal_ble::resume_off_ui_thread{};
  thread_after = GetCurrentThreadId();
}

}  // namespace

int main() {
  // The UI thread: an STA with a message queue, as in a Flutter runner.
  CHECK(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)));
  CHECK(universal_ble::IsOnSingleThreadedApartment());
  const DWORD ui_thread = GetCurrentThreadId();

  const auto started = std::chrono::steady_clock::now();
  ConnectShapedCoroutine(std::chrono::milliseconds(1500));
  const auto returned_after = std::chrono::steady_clock::now() - started;

  // The plugin method returns at once; the UI thread is free to pump.
  CHECK(returned_after < std::chrono::milliseconds(200));
  CHECK(!stalled_call_done);

  // Wait for the stalled call while pumping, as the UI thread would.
  while (!stalled_call_done) {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
      DispatchMessageW(&message);
    }
    Sleep(5);
  }
  CHECK(continuation_thread != ui_thread);
  CHECK(!continuation_on_sta);

  // Off an STA there is nothing to leave: no extra thread hop.
  struct Threads {
    DWORD worker = 0;
    DWORD after = 0;
  } threads;
  HANDLE worker = CreateThread(
      nullptr, 0,
      [](LPVOID param) -> DWORD {
        auto *t = static_cast<Threads *>(param);
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        t->worker = GetCurrentThreadId();
        AlreadyOffUiThread(t->after);
        return 0;
      },
      &threads, 0, nullptr);
  CHECK(worker != nullptr);
  WaitForSingleObject(worker, 5000);
  CloseHandle(worker);
  CHECK(threads.worker != 0);
  CHECK(threads.after == threads.worker);

  CoUninitialize();
  std::cout << "off_ui_thread_test passed" << std::endl;
  return 0;
}
