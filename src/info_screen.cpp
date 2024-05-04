#include "info_screen.h"

#include <QDataStream>
#include <QFile>

InformationScreen::InformationScreen(): active_pos_(kMenuPlay), screen_changed_(true),
    active_selection_(2), no_vr_(false), show_menu_(true), setting_invert_(0) {
  info_scr_.resize(kScrWidth * kScrHeight); // If memory lack, exception finishs constructor
  active_pos_ = kMenuPlay;


  LoadResources();

  DrawScreen();
}

InformationScreen::~InformationScreen() {

}

const uint32_t* InformationScreen::GetInfoScr() {
  if (screen_changed_) {
    screen_changed_ = false;
    DrawScreen();
  }
  return info_scr_.data();

}

void InformationScreen::DoRight() {
  ++active_selection_;
  if (active_selection_ >= GetMaxMenuSelections(active_pos_)) {
    active_selection_ = 0;
  }
  screen_changed_ = true;
}

void InformationScreen::DoLeft() {
  if (active_selection_ == 0) {
    active_selection_ = GetMaxMenuSelections(active_pos_) - 1;
  } else {
    --active_selection_;
  }
  assert(active_selection_ >= 0);
  screen_changed_ = true;
}

void InformationScreen::DoUp() {
  switch (active_pos_) {
    case kMenuInvert:
      active_pos_ = kMenuPlay;
      active_selection_ = 2;
      break;
    case kMenuPlay:
      active_pos_ = kMenuInvert;
      active_selection_ = setting_invert_;
      break;
  }
  screen_changed_ = true;
}

void InformationScreen::DoDown() {
  switch (active_pos_) {
    case kMenuInvert:
      active_pos_ = kMenuPlay;
      active_selection_ = 2;
      break;
    case kMenuPlay:
      active_pos_ = kMenuInvert;
      active_selection_ = setting_invert_;
      break;
  }
  screen_changed_ = true;
}

InformationScreen::Action InformationScreen::DoSelectAndGetAction(int& value) {
  screen_changed_ = true;

  switch (active_pos_) {
    case kMenuInvert:
      setting_invert_ = active_selection_;
      value = setting_invert_;
      return kInvertAction;
      break;
    case kMenuPlay:
      switch (active_selection_) {
        case 0:
          value = -300;
          return kRePositionAction;
          break;
        case 1:
          value = -30;
          return kRePositionAction;
          break;
        case 2:
          return kPlayAction;
          break;
        case 3:
          value = +30;
          return kRePositionAction;
          break;
        case 4:
          value = +300;
          return kRePositionAction;
          break;

      }

      assert(false);
      return kPlayAction;
      break;
  }

  assert(false);
  return kPlayAction;
}

void InformationScreen::SetNoVrWarning(bool no_vr) {
  no_vr_ = no_vr;
  screen_changed_ = true;
}

void InformationScreen::ShowMenu(bool show_menu) {
  show_menu_ = show_menu;
  screen_changed_ = true;
}


void InformationScreen::LoadResources() {
  LoadResFile(invert_0_tile_, "invert_0.data", kTileSize);
  LoadResFile(invert_1_tile_, "invert_1.data", kTileSize);
  LoadResFile(play_tile_, "play.data", kTileSize);

  LoadResFile(fast_backward_tile_, "fastbackward.data", kTileSize);
  LoadResFile(backward_tile_, "backward.data", kTileSize);
  LoadResFile(forward_tile_, "forward.data", kTileSize);
  LoadResFile(fast_forward_tile_, "fastforward.data", kTileSize);

  LoadResFile(selector_tile_, "selector.data", kTileSize);
  LoadResFile(no_vr_tile_, "no_vr.data", kWarningSize);
}

void InformationScreen::LoadResFile(std::vector<uint32_t>& storage, std::string fname, size_t req_size) {
  storage.clear();
  int rawsize = req_size * sizeof(uint32_t);
  assert(rawsize / sizeof(uint32_t) == req_size);
  try {
    std::string ffn = ":/sprite/" + fname;
    QFile f(ffn.c_str());
    if (!f.open(QIODevice::ReadOnly | QIODevice::ExistingOnly)) {
      throw std::runtime_error(std::string("Can't find resource ") + fname);
    }
    QDataStream ds(&f);
    storage.resize(req_size);
    auto rd = ds.readRawData((char*)storage.data(), rawsize);
    if (rd < rawsize) {
      storage.clear();
      throw std::runtime_error(std::string("Not enough data into resource ") + fname);
    }
  }
  catch (std::bad_alloc&) {
    throw std::runtime_error(std::string("Can't load resource ") + fname);
  }
}

void InformationScreen::Tile(const Image& img, size_t x, size_t y, size_t width) {
  AddTile(info_scr_, kScrWidth, img, width, x, y);
}

void InformationScreen::DrawScreen() {
  memset(info_scr_.data(), 0, info_scr_.size() * sizeof(uint32_t));

  if (show_menu_) {
    DrawInvertMenu(active_pos_, active_selection_);
    DrawPlayMenu(active_pos_, active_selection_);
  }

  if (no_vr_) {
    Tile(no_vr_tile_, kScrWidth - kWarningWidth, 0, kWarningWidth);
  }
}

void InformationScreen::DrawInvertMenu(InformationScreen::MenuPosition pos, int sel) {
  size_t ry = kTileHeight * 3;
  if (setting_invert_ == 0) {
    Tile(invert_0_tile_, 0, ry);
  } else {
    Tile(invert_1_tile_, 0, ry);
  }

  if (pos == kMenuInvert && sel >= 0 && sel < 2) {
    Tile(invert_0_tile_, kTileWidth, ry);
    Tile(invert_1_tile_, kTileWidth * 2, ry);
    Tile(selector_tile_, kTileWidth * (sel + 1), ry);
  }
}

void InformationScreen::DrawPlayMenu(InformationScreen::MenuPosition pos, int sel) {
  size_t ry = kTileHeight * 4;
  size_t rx = 250;
  Tile(fast_backward_tile_, rx, ry);
  Tile(backward_tile_, rx + kTileWidth, ry);
  Tile(play_tile_, rx + kTileWidth * 2, ry);
  Tile(forward_tile_, rx + kTileWidth * 3, ry);
  Tile(fast_forward_tile_, rx + kTileWidth * 4, ry);
  if (pos == kMenuPlay && sel >= 0 && sel < 5) {
    Tile(selector_tile_, rx + sel * kTileWidth, ry);
  }
}

int InformationScreen::GetMaxMenuSelections(InformationScreen::MenuPosition pos) {
  switch (pos) {
    case kMenuInvert: return 2;
    case kMenuPlay: return 5;
  }

  assert(false);
  return 0;
}
