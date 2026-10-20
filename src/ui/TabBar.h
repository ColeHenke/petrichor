#ifndef TAB_BAR_H_
#define TAB_BAR_H_

#include <QTabBar>
#include <QPushButton>

// Qt presentation for tabs. Browser ownership and lookup belong to TabController.
class TabBar : public QTabBar {
  Q_OBJECT

 public:
  explicit TabBar(QWidget* parent = nullptr);
  ~TabBar() override;

  void AddTab(int browser_id, const QString& title);
  bool RemoveTab(int browser_id);
  bool SetActiveTab(int browser_id);
  int GetBrowserIdAt(int index) const;

 signals:
  void NewTabRequested();

 private:
  QPushButton* add_tab_button_ = nullptr;
};

#endif  // TAB_BAR_H_
