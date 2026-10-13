#include "browser/BrowserHandler.h"
#include "ui/MainWindow.h"

#include <sstream>
#include <string>

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

  QWidget* host_page = pending_host_page;
  pending_host_page = nullptr;

  if (main_window_ && host_page) {
    main_window_->RegisterBrowser(browser, host_page);
  }
}

bool BrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (main_window_->GetTabManager().Count() == 1) {
    is_closing_ = true;
  }
  return false;
}

void BrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  if (main_window_) {
    main_window_->GetTabManager().RemoveBrowser(browser);
    main_window_->SetBrowser(main_window_->GetTabManager().GetActiveBrowser());
  }

  if (main_window_->GetTabManager().IsEmpty()) {
    // Last window closed — shut down the message loop.
    // TODO: replace with Qt shutdown once event loops are integrated.
    CefQuitMessageLoop();
  }
}

void BrowserHandler::ShowMainWindow() {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::ShowMainWindow, this));
    return;
  }

  CefRefPtr<CefBrowser> browser = main_window_->GetTabManager().GetActiveBrowser();
  if (browser) {
    PlatformShowWindow(browser);
  }
}

void BrowserHandler::SetMainWindow(MainWindow* main_window) {
  main_window_ = main_window;
}

void BrowserHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::CloseAllBrowsers, this, force_close));
    return;
  }

  for (const auto& tab : main_window_->GetTabManager().GetTabs()) {
    tab.browser->GetHost()->CloseBrowser(force_close);
  }
}

BrowserHandler::BrowserList BrowserHandler::GetBrowserList() const {
  BrowserList browsers;
  browsers.reserve(main_window_->GetTabManager().Count());
  for (const auto& tab : main_window_->GetTabManager().GetTabs()) {
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

  if (!frame || !frame->IsMain() || !main_window_) {
    return;
  }

  main_window_->SetUrl(QString::fromStdString(url.ToString()));
}

void BrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
  CEF_REQUIRE_UI_THREAD();

  if (main_window_) {
    main_window_->SetNavigationState(isLoading, canGoBack, canGoForward);
  }
}

bool BrowserHandler::CreateBrowserForHost(const CefWindowInfo& window_info, const CefRefPtr<CefClient>& client, const CefString& url, const CefBrowserSettings& browser_settings, QWidget* host_page) {
  CEF_REQUIRE_UI_THREAD();
  // A single saved page supports one creation request at a time.
  if (pending_host_page || !host_page) {
    return false;
  }
  pending_host_page = host_page;
  if (!CefBrowserHost::CreateBrowser(window_info, client, url, browser_settings, nullptr, nullptr)) {
    pending_host_page = nullptr;
    return false;
  }
  return true;
}

void BrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  if (main_window_) {
    main_window_->SetWindowTitle(QString::fromStdString(title.ToString()));
  }
}
