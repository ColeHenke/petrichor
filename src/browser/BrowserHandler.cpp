#include "browser/BrowserHandler.h"
#include "ui/MainWindow.h"

#include <sstream>
#include <string>

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {

BrowserHandler* g_instance = nullptr;

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
  return "data:" + mime_type + ";base64," +
         CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
             .ToString();
}

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

void BrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                   const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  // Always Alloy style — use platform API to set the window title.
  // TODO: signal Qt MainWindow to update the tab title.
  PlatformTitleChange(browser, title);
}

void BrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  DCHECK_EQ(CEF_RUNTIME_STYLE_ALLOY, browser->GetHost()->GetRuntimeStyle());
  browser_list_.push_back(browser);

  if (main_window_ != nullptr) {
    main_window_->SetBrowser(browser);
  }
}

bool BrowserHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (browser_list_.size() == 1) {
    is_closing_ = true;
  }
  return false;
}

void BrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  for (auto it = browser_list_.begin(); it != browser_list_.end(); ++it) {
    if ((*it)->IsSame(browser)) {
      browser_list_.erase(it);
      break;
    }
  }

  if (browser_list_.empty()) {
    // Last window closed — shut down the message loop.
    // TODO: replace with Qt shutdown once event loops are integrated.
    CefQuitMessageLoop();
  }
}

void BrowserHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 ErrorCode errorCode,
                                 const CefString& errorText,
                                 const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  if (errorCode == ERR_ABORTED) {
    return;
  }

  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
     << std::string(failedUrl) << " with error " << std::string(errorText)
     << " (" << errorCode << ").</h2></body></html>";

  frame->LoadURL(GetDataURI(ss.str(), "text/html"));

  fprintf(stderr, "Load error: %s (%d)\n",
        failedUrl.ToString().c_str(), errorCode);
}

void BrowserHandler::ShowMainWindow() {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::ShowMainWindow, this));
    return;
  }

  if (!browser_list_.empty()) {
    PlatformShowWindow(browser_list_.front());
  }
}

void BrowserHandler::SetMainWindow(MainWindow* main_window) {
  main_window_ = main_window;
}

void BrowserHandler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::BindOnce(&BrowserHandler::CloseAllBrowsers,
                                       this, force_close));
    return;
  }

  for (const auto& browser : browser_list_) {
    browser->GetHost()->CloseBrowser(force_close);
  }
}

void BrowserHandler::ResizeBrowser(CefRefPtr<CefBrowser> browser, const CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  if (!browser) {
    return;
  }

  PlatformResizeView(browser, rect);
}

void BrowserHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) {
  CEF_REQUIRE_UI_THREAD();

  fprintf(stderr, "Load finished: %s status=%d\n",
          frame->GetURL().ToString().c_str(), httpStatusCode);
}

void BrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) {
  CEF_REQUIRE_UI_THREAD();

  if (!frame || !frame->IsMain() || !main_window_) {
    return;
  }

  main_window_->SetUrl(QString::fromStdString(url.ToString()));
}
