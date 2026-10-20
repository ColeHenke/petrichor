#include "browser/TabController.h"
#include <algorithm>
#include "browser/BrowserHandler.h"

TabController::TabController(QObject* parent) : QObject(parent) {}

bool TabController::CreateTab(const QString& url, QWidget* host_page) {
  auto* handler = BrowserHandler::GetInstance();
  // Preserve the existing single pending creation contract of the shared client.
  if (!handler || !host_page || pending_host_page_) return false;
  CefWindowInfo info;
  info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  const QRect bounds = host_page->rect();
  info.SetAsChild(reinterpret_cast<CefWindowHandle>(host_page->winId()),
                 {bounds.x(), bounds.y(), bounds.width(), bounds.height()});
  CefBrowserSettings settings;
  pending_host_page_ = host_page;
  if (!CefBrowserHost::CreateBrowser(info, handler, url.toStdString(),
                                    settings, nullptr, nullptr)) {
    pending_host_page_.clear();
    close_pending_browser_ = false;
    force_close_pending_browser_ = false;
    return false;
  }
  return true;
}

void TabController::BrowserCreated(CefRefPtr<CefBrowser> browser) {
  if (!browser || !pending_host_page_) return;
  const int id = browser->GetIdentifier();
  if (FindBrowser(id)) return;
  const auto page = pending_host_page_;
  pending_host_page_.clear();
  const bool close = close_pending_browser_;
  const bool force = force_close_pending_browser_;
  close_pending_browser_ = false;
  force_close_pending_browser_ = false;
  tabs_.push_back(Tab{id, browser, page});
  emit TabAdded(id);
  SetActiveBrowser(id);
  if (close) RequestCloseTab(id, force);
}

void TabController::RequestCloseTab(int browser_id, bool force_close) {
  const auto browser = FindBrowser(browser_id);
  // DoClose marks the tab closing only after unload handlers allow it.
  if (browser) browser->GetHost()->CloseBrowser(force_close);
}

void TabController::RequestCloseAll(bool force_close) {
  if (pending_host_page_) {
    close_pending_browser_ = true;
    force_close_pending_browser_ = force_close_pending_browser_ || force_close;
  }
  // CEF callbacks may mutate tabs_ synchronously during CloseBrowser.
  std::vector<CefRefPtr<CefBrowser>> browsers;
  for (const auto& tab : tabs_)
    if (!tab.closing) browsers.push_back(tab.browser);
  for (const auto& browser : browsers) browser->GetHost()->CloseBrowser(force_close);
}

bool TabController::BrowserReadyToClose(CefRefPtr<CefBrowser> browser) {
  for (auto& tab : tabs_) {
    if (tab.browser_id == browser->GetIdentifier() && tab.browser->IsSame(browser)) {
      if (!tab.closing) {
        tab.closing = true;
        // The window removes this tab's native host page, completing CEF's
        // close without sending a close notification to the shared MainWindow.
        if (tab.host_page) emit HostCloseRequested(tab.host_page);
      }
      return true;
    }
  }
  return false;
}

void TabController::BrowserClosed(CefRefPtr<CefBrowser> browser) {
  const int id = browser->GetIdentifier();
  auto it = std::find_if(tabs_.begin(), tabs_.end(), [&](const Tab& tab) {
    return tab.browser_id == id && tab.browser->IsSame(browser);
  });
  if (it == tabs_.end()) return;
  const auto page = it->host_page;
  const bool needs_teardown = !it->closing;
  tabs_.erase(it);
  if (active_browser_id_ == id) {
    active_browser_id_ = -1;
    for (const auto& tab : tabs_) {
      if (!tab.closing) { active_browser_id_ = tab.browser_id; break; }
    }
  }
  if (needs_teardown && page) emit HostCloseRequested(page);
  emit TabRemoved(id);
  emit ActiveBrowserChanged(active_browser_id_);
}

bool TabController::SetActiveBrowser(int browser_id) {
  if (!FindBrowser(browser_id)) return false;
  active_browser_id_ = browser_id;
  emit ActiveBrowserChanged(browser_id);
  return true;
}

CefRefPtr<CefBrowser> TabController::GetActiveBrowser() const {
  return FindBrowser(active_browser_id_);
}

CefRefPtr<CefBrowser> TabController::FindBrowser(int browser_id) const {
  for (const auto& tab : tabs_)
    if (tab.browser_id == browser_id && !tab.closing) return tab.browser;
  return nullptr;
}

CefRefPtr<CefBrowser> TabController::FindBrowserForHostPage(QWidget* page) const {
  for (const auto& tab : tabs_)
    if (tab.host_page == page && !tab.closing) return tab.browser;
  return nullptr;
}

QWidget* TabController::FindHostPage(int browser_id) const {
  for (const auto& tab : tabs_)
    if (tab.browser_id == browser_id && !tab.closing) return tab.host_page;
  return nullptr;
}

void TabController::AddressChanged(int id, const QString& url) {
  if (id == active_browser_id_) emit ActiveUrlChanged(url);
}
void TabController::TitleChanged(int id, const QString& title) {
  if (id == active_browser_id_) emit ActiveTitleChanged(title);
}
void TabController::LoadingStateChanged(int id, bool loading, bool back, bool forward) {
  if (id == active_browser_id_) emit ActiveNavigationChanged(loading, back, forward);
}
