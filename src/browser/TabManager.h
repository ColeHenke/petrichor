#ifndef TAB_MANAGER_H_
#define TAB_MANAGER_H_

#include <vector>

#include "include/cef_browser.h"

// Tracks the CEF browser instances that make up the browser's tabs.
// This is a model, not a Qt widget; BrowserHandler forwards CEF lifecycle
// callbacks here and the Qt UI reads the active browser from it.
class TabManager {
 public:
  struct Tab {
    int browser_id;
    CefRefPtr<CefBrowser> browser;
  };

  using TabList = std::vector<Tab>;

  bool AddBrowser(CefRefPtr<CefBrowser> browser);
  bool RemoveBrowser(CefRefPtr<CefBrowser> browser);

  bool SetActiveBrowser(int browser_id);
  CefRefPtr<CefBrowser> GetActiveBrowser() const;
  CefRefPtr<CefBrowser> FindBrowser(int browser_id) const;

  int GetActiveBrowserId() const { return active_browser_id_; }
  std::size_t Count() const { return tabs_.size(); }
  bool IsEmpty() const { return tabs_.empty(); }
  const TabList& GetTabs() const { return tabs_; }

 private:
  TabList tabs_;
  int active_browser_id_ = -1;
};

#endif  // TAB_MANAGER_H_
