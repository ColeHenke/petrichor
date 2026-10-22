#ifndef BROWSER_APP_H_
#define BROWSER_APP_H_

#include "include/cef_app.h"

class MainWindow;
class QTimer;

// BrowserApp handles application-level CEF callbacks for the browser process.
// This is the first CEF object created and is responsible for initializing
// the browser and creating the first window once CEF is ready.
class PetrichorApp : public CefApp, public CefBrowserProcessHandler {
 public:
  PetrichorApp();

  // main() needs to hand us the MainWindow instance before
  // CefInitialize() runs, so OnContextInitialized() can request the first tab.
  void SetMainWindow(MainWindow* main_window);

  // CefApp methods:
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  // CefBrowserProcessHandler methods:

  // Called on the browser process UI thread immediately after CEF has
  // initialized. This is the earliest point at which we can create browsers.
  void OnContextInitialized() override;

  void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override;

  // Returns the default client for Chrome-style popup windows.
  CefRefPtr<CefClient> GetDefaultClient() override;

  // only called when CefSettings.external_message_pump is true. CEF
  // is telling us "call CefDoMessageLoopWork() again in delay_ms
  // milliseconds" instead of pumping its own loop. Schedule that with
  // QTimer::singleShot(). Careful: delay_ms can be negative — check CEF's
  // doc comment on this method (in cef_browser_process_handler.h) for what
  // that means and handle it before handing the value to Qt.
  void OnScheduleMessagePumpWork(int64_t delay_ms) override;

 private:
  void DoMessageLoopWork();

  MainWindow* main_window_ = nullptr;
  QTimer* message_pump_timer_ = nullptr;

  IMPLEMENT_REFCOUNTING(PetrichorApp);
};

#endif  // BROWSER_APP_H_
