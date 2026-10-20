#include "browser/BrowserHandler.h"
#include "ui/MainWindow.h"

#include <sstream>
#include <string>
#include <QApplication>
#include <QMetaObject>

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

BrowserHandler* g_instance = nullptr;

}  // namespace

BrowserHandler::BrowserHandler() {
  DCHECK(!g_instance);
  g_instance = this;
}

BrowserHandler::~BrowserHandler() {
  g_instance = nullptr;
}

// static
BrowserHandler* BrowserHandler::GetInstance() {
  return g_instance;
}

void BrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK_EQ(CEF_RUNTIME_STYLE_ALLOY, browser->GetHost()->GetRuntimeStyle());

  if (tab_controller_) tab_controller_->BrowserCreated(browser);
}

bool BrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (!tab_controller_) return false;
  if (tab_controller_->Count() == 1) is_closing_ = true;
  return tab_controller_->BrowserReadyToClose(browser);
}

void BrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (!tab_controller_) {
    return;
  }
  tab_controller_->BrowserClosed(browser);
  if (tab_controller_->IsEmpty()) {
    // All browsers are closed. Bypass the Cocoa terminate: override, which
    // handles requests to close browsers but does not exit Qt's event loop.
    QMetaObject::invokeMethod(QApplication::instance(),[] { QApplication::exit(0); },
        Qt::QueuedConnection);
  }
}

void BrowserHandler::ShowMainWindow() {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::ShowMainWindow, this));
    return;
  }

  CefRefPtr<CefBrowser> browser = tab_controller_ ? tab_controller_->GetActiveBrowser() : nullptr;
  if (browser) {
    PlatformShowWindow(browser);
  }
}

void BrowserHandler::SetMainWindow(MainWindow* main_window) {
  tab_controller_ = main_window ? &main_window->GetTabController() : nullptr;
}

void BrowserHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::CloseAllBrowsers, this, force_close));
    return;
  }

  if (tab_controller_) tab_controller_->RequestCloseAll(force_close);
}

BrowserHandler::BrowserList BrowserHandler::GetBrowserList() const {
  BrowserList browsers;
  if (!tab_controller_) return browsers;
  browsers.reserve(tab_controller_->Count());
  for (const auto& tab : tab_controller_->GetTabs()) {
    browsers.push_back(tab.browser);
  }
  return browsers;
}

void BrowserHandler::ResizeBrowser(CefRefPtr<CefBrowser> browser, const CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  if (!browser) {
    return;
  }

  PlatformResizeView(browser, rect);
}

void BrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) {
  CEF_REQUIRE_UI_THREAD();

  if (!frame || !frame->IsMain() || !tab_controller_) {
    return;
  }

  tab_controller_->AddressChanged(browser->GetIdentifier(), QString::fromStdString(url.ToString()));
}

void BrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
  CEF_REQUIRE_UI_THREAD();

  if (tab_controller_) {
    tab_controller_->LoadingStateChanged(browser->GetIdentifier(), isLoading, canGoBack, canGoForward);
  }
}

void BrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  if (tab_controller_) {
    tab_controller_->TitleChanged(browser->GetIdentifier(), QString::fromStdString(title.ToString()));
  }
}
