#include "PetrichorApp.h"

#include <string>

#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>

#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "browser/BrowserHandler.h"
#include "ui/MainWindow.h"

PetrichorApp::PetrichorApp() {
  message_pump_timer_ = new QTimer(QCoreApplication::instance());
  message_pump_timer_->setSingleShot(true);

  QObject::connect(message_pump_timer_, &QTimer::timeout,
                   message_pump_timer_, [this] { DoMessageLoopWork(); });
}

void PetrichorApp::SetMainWindow(MainWindow* main_window) {
  main_window_ = main_window;
}

void PetrichorApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  // use Alloy runtime style so CEF gives a bare window we can
  // embed into Qt, rather than Chrome style which renders its own UI shell.
  CefRefPtr<BrowserHandler> handler(new BrowserHandler());

  handler->SetMainWindow(main_window_);

  // Default URL — will be driven by the Qt address bar later.
  std::string url = "https://www.google.com";

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  const std::string cmd_url = command_line->GetSwitchValue("url");
  if (!cmd_url.empty()) {
    url = cmd_url;
  }

  // CEF is ready: create the first browser through the same path as new tabs.
  main_window_->CreateTab(QString::fromStdString(url));
}

CefRefPtr<CefClient> PetrichorApp::GetDefaultClient() {
  return BrowserHandler::GetInstance();
}

void PetrichorApp::OnScheduleMessagePumpWork(int64_t delay_ms) {
  // CEF may call this method from any thread. Queue the scheduling operation
  // onto the Qt thread that owns message_pump_timer_.
  QMetaObject::invokeMethod(
      message_pump_timer_,
      [this, delay_ms] {
        // CEF's sample external pump never waits longer than roughly one
        // frame before checking for work again. This fallback matters because
        // CEF may not issue another scheduling callback immediately after a
        // pump iteration.
        constexpr int64_t kMaxPumpDelayMs = 1000 / 30;

        int64_t requested_delay_ms = delay_ms;
        if (requested_delay_ms <= 0) {
          requested_delay_ms = 0;
        } else if (requested_delay_ms > kMaxPumpDelayMs) {
          requested_delay_ms = kMaxPumpDelayMs;
        }

        message_pump_timer_->start(static_cast<int>(requested_delay_ms));
      },
      Qt::QueuedConnection);
}

void PetrichorApp::DoMessageLoopWork() {
  CefDoMessageLoopWork();

  // QTimer is single-shot. Keep a fallback wakeup scheduled after every
  // iteration; an explicit CEF scheduling request can replace it with a
  // sooner timer through OnScheduleMessagePumpWork().
  if (!message_pump_timer_->isActive()) {
    constexpr int kMaxPumpDelayMs = 1000 / 30;
    message_pump_timer_->start(kMaxPumpDelayMs);
  }
}
