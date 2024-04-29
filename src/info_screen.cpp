#include "info_screen.h"

#include <QDataStream>
#include <QFile>

InformationScreen::InformationScreen(): screen_changed_(true), no_vr_(false),
    show_menu_(true), setting_invert_(0), active_pos_(kMenuPlay) {
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

void InformationScreen::DoLeft() {
  ++active_selection_;
  if (active_selection_ >= GetMaxMenuSelections(active_pos_)) {
    active_selection_ = 0;
  }
  screen_changed_ = true;
}

void InformationScreen::DoRight() {
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
    case kMenuInvert: active_pos_ = kMenuPlay; break;
    case kMenuPlay:
      active_pos_ = kMenuInvert;
      active_selection_ = setting_invert_;
      break;
  }
  screen_changed_ = true;
}

void InformationScreen::DoDown() {
  switch (active_pos_) {
    case kMenuInvert: active_pos_ = kMenuPlay; break;
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
      return kPlayAction;
      break;
  }

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


  LoadResFile(selector_tile_, "selector.data", kTileSize);
  LoadResFile(no_vr_tile_, "no_vr.data", kWarningSize);
}

bool InformationScreen::LoadResFile(std::vector<uint32_t>& storage, std::string fname, size_t req_size) {
  storage.clear();
  int rawsize = req_size * sizeof(uint32_t);
  assert(rawsize / sizeof(uint32_t) == req_size);
  try {
    std::string ffn = ":/sprite/" + fname;
    QFile f(ffn.c_str());
    if (!f.open(QIODevice::ReadOnly | QIODevice::ExistingOnly)) { return false; }
    QDataStream ds(&f);
    storage.resize(req_size);
    auto rd = ds.readRawData((char*)storage.data(), rawsize);
    if (rd < rawsize) {
      storage.clear();
      return false;
    }
    return true;
  }
  catch (std::bad_alloc&) {
  }
  return false;
}

void InformationScreen::Tile(const Image& img, size_t x, size_t y, size_t width, size_t height) {
  if (width == 0 || height == 0) { return; }
  if ((x + width) > kScrWidth) { return; }
  if ((y + height) > kScrHeight) { return; }

  uint32_t* dst = info_scr_.data() + y * kScrWidth + x;
  const uint32_t* src = img.data();
  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      struct pixel {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
      };

      auto pres = *(pixel*)(dst + j);
      auto pmsk = *(pixel*)(src + j);
      float sol = pmsk.alpha / 255.0f;
      float tra = 1.0f - sol;
      pres.red = pres.red * tra + pmsk.red * sol;
      pres.green = pres.green * tra + pmsk.green * sol;
      pres.blue = pres.blue * tra + pmsk.blue * sol;
      pres.alpha = 255 - (uint8_t)((255 - pres.alpha) * tra);
      *(pixel*)(dst + j) = pres;
    }
    dst += kScrWidth;
    src += width;
  }
}

void InformationScreen::DrawScreen() {
  memset(info_scr_.data(), 0, info_scr_.size() * sizeof(uint32_t));

  if (show_menu_) {
    DrawInvertMenu(active_pos_, active_selection_);
    DrawPlayMenu(active_pos_, active_selection_);
  }

  if (no_vr_) {
    Tile(no_vr_tile_, kScrWidth - kWarningWidth, 0, kWarningWidth, kWarningHeight);
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
  Tile(play_tile_, 0, ry);
  if (pos == kMenuPlay) {
    Tile(selector_tile_, 0, ry);
  }
}

int InformationScreen::GetMaxMenuSelections(InformationScreen::MenuPosition pos) {
  switch (pos) {
    case kMenuInvert: return 2;
    case kMenuPlay: return 1;
  }

  assert(false);
  return 0;
}
