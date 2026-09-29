#ifndef TAB_BAR_H_
#define TAB_BAR_H_

#include <QTabBar>
#include <QString>

// Qt presentation for tabs. Browser ownership and lookup belong to TabManager.
class TabBar : public QTabBar {
 public:
  explicit TabBar(QWidget* parent = nullptr);
  ~TabBar() override;

  void AddTab(int browser_id, const QString& title);
  bool RemoveTab(int browser_id);
  bool SetActiveTab(int browser_id);
  int GetBrowserIdAt(int index) const;
};

#endif  // TAB_BAR_H_
