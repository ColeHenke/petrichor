#include "browser/TabManager.h"

#include <iostream>

bool TabManager::AddBrowser(CefRefPtr<CefBrowser> browser) {
  if (!browser) {
    return false;
  }

  const int browser_id = browser->GetIdentifier();
  if (FindBrowser(browser_id)) {
    return false;
  }

  tabs_.push_back(Tab{browser_id, browser});

  // The first browser becomes active. Later browsers are selected explicitly
  // by the UI when tab selection is implemented.
  if (active_browser_id_ == -1) {
    active_browser_id_ = browser_id;
  }

  return true;
}

bool TabManager::RemoveBrowser(CefRefPtr<CefBrowser> browser) {
  if (!browser) {
    return false;
  }

  const int browser_id = browser->GetIdentifier();
  for (auto it = tabs_.begin(); it != tabs_.end(); ++it) {
    if (it->browser_id == browser_id && it->browser->IsSame(browser)) {
      tabs_.erase(it);

      if (active_browser_id_ == browser_id) {
        active_browser_id_ = tabs_.empty() ? -1 : tabs_.front().browser_id;
      }

      return true;
    }
  }

  return false;
}

bool TabManager::SetActiveBrowser(int browser_id) {
  if (!FindBrowser(browser_id)) {
    return false;
  }

  active_browser_id_ = browser_id;
  return true;
}

CefRefPtr<CefBrowser> TabManager::GetActiveBrowser() const {
  return FindBrowser(active_browser_id_);
}

CefRefPtr<CefBrowser> TabManager::FindBrowser(int browser_id) const {
  for (const auto& tab : tabs_) {
    if (tab.browser_id == browser_id) {
      return tab.browser;
    }
  }

  return nullptr;
}
