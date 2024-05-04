#ifndef INFO_SCREEN_H
#define INFO_SCREEN_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "tile.h"

struct InformationState {

  bool InvertUsage;
  bool InvertSelected;
};


class InformationScreen {
 public:
  InformationScreen();
  ~InformationScreen();

  enum Action {
    kNoAction, // Ничего не делаем
    kInvertAction, // Изменить (инвертировать) порядок отображения экранов
    kPlayAction, // Начать проигрывание
    kRePositionAction, // Сменить позицию проигрывания
  };

  const uint32_t* GetInfoScr();
  size_t GetInfoScrWidth() const { return kScrWidth; }
  size_t GetInfoScrHeight() const { return kScrHeight; }

  void DoLeft();
  void DoRight();
  void DoUp();
  void DoDown();

  /*! Проинформировать, что пользователь сделал выбор. Возвращается из функции
  требуетмая реакция и, опционально, числовое значение (если требуется) */
  Action DoSelectAndGetAction(int& value);

  void SetNoVrWarning(bool no_vr);
  void ShowMenu(bool show_menu);

 private:
  InformationScreen(const InformationScreen&) = delete;
  InformationScreen& operator=(const InformationScreen&) = delete;
  InformationScreen(InformationScreen&&) = delete;
  InformationScreen& operator=(InformationScreen&&) = delete;

  using Image = std::vector<uint32_t>;

  enum MenuPosition {
    kMenuInvert,
    kMenuPlay
  };

  const size_t kScrWidth = 900;
  const size_t kScrHeight = 900;
  static const size_t kTileWidth = 80;
  static const size_t kTileHeight = 80;
  static const size_t kWarningWidth = 160;
  static const size_t kWarningHeight = 160;
  const size_t kTileSize = kTileWidth * kTileHeight;
  const size_t kWarningSize = kWarningWidth * kWarningHeight;

  Image info_scr_;
  MenuPosition active_pos_; //!< Текущая выделенная (вертикальная) позиция в списке меню
  int active_selection_; //!< Текущий выделенный (горизонтальный) пункт в меню
  Image invert_0_tile_;
  Image invert_1_tile_;
  Image play_tile_;
  Image fast_backward_tile_;
  Image backward_tile_;
  Image forward_tile_;
  Image fast_forward_tile_;

  Image selector_tile_;
  Image no_vr_tile_;

  std::atomic_bool screen_changed_;
  bool no_vr_;
  bool show_menu_;

  int setting_invert_;

  void LoadResources();

  void LoadResFile(std::vector<uint32_t>& storage, std::string fname, size_t req_size);
  void Tile(const Image& img, size_t x, size_t y, size_t width = kTileWidth);
  void DrawScreen();
  void DrawInvertMenu(MenuPosition pos, int sel);
  void DrawPlayMenu(MenuPosition pos, int sel);



  int GetMaxMenuSelections(MenuPosition pos);
};



#endif // INFO_SCREEN_H
