#include <QApplication>
#include <CQWordle.h>
#include <CWordleValidWords.h>
#include <CWordleUseWords.h>
#include <CConfig.h>

#include <QPainter>
#include <QKeyEvent>

#include <iostream>

// Save Data
//  Last Day Solution (none or all words)
//  Wins per guesses

//---

namespace {

static const int daySeconds = 86400;

int baseDays() {
  struct tm lt;

  lt.tm_hour = 0;
  lt.tm_min  = 0;
  lt.tm_sec  = 0;
  lt.tm_year = 2021 - 1900;
  lt.tm_mon  = 4;
  lt.tm_mday = 27;

  return int(mktime(&lt)/daySeconds);
}

int currentDays() {
  auto t = time(nullptr);

  struct tm lt = *localtime(&t);

  lt.tm_hour = 0;
  lt.tm_min  = 0;
  lt.tm_sec  = 0;

  return int(mktime(&lt)/daySeconds);
}

}

//---

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  auto *wordle = new CQWordle;

  wordle->show();

  app.exec();
}

CQWordle::
CQWordle() :
 config_("CWordle")
{
  init();

  //---

  // set fonts
  guessFont_    = qApp->font();
  keyboardFont_ = guessFont_;
  messageFont_  = guessFont_;

  guessFont_   .setPointSizeF(64);
  keyboardFont_.setPointSizeF(48);
  messageFont_ .setPointSizeF(32);

  //---

  int i = 0;

  while (CWordleValidWords[i] != nullptr) {
    wordSet_.insert(CWordleValidWords[i]);

    ++i;
  }

  //std::cerr << "Num words = " << i << "\n";

  //---

  setWord();

  //---

  loadConfig();
}

CQWordle::
~CQWordle()
{
  saveConfig();
}

void
CQWordle::
init()
{
  keyData_.used.clear();
  keyData_.rect.clear();

  while (guesses_.size() != s_maxGuesses) {
    GuessData guess;

    guess.init();

    guesses_.push_back(guess);
  }

  guessNo_ = 0;
  charNo_  = 0;

  illegalWord_ = false;

  baseDays_    = baseDays();
  currentDays_ = currentDays();
}

void
CQWordle::
loadConfig()
{
  config_.getValue("wins"  , &wins_);
  config_.getValue("losses", &losses_);

  int lastDays;
  config_.getValue("lastDays", &lastDays);

  if (lastDays == currentDays_) {
    config_.getValue("guessNo", &guessNo_);

    std::string guessData;
    config_.getValue("guessData", guessData);

    auto len = int(guessData.size());

    if (len == (guessNo_ + 1)*s_wordLen) {
      int i = 0;

      for (int r = 0; r < guessNo_ + 1; ++r)
        for (int c = 0; c < s_wordLen; ++c)
          guesses_[r].guess[c] = guessData[i++];
    }

    charNo_ = s_wordLen;

    enterWord();
  }
}

void
CQWordle::
saveConfig()
{
  config_.setValue("wins"   , wins_);
  config_.setValue("losses" , losses_);

  std::string guessData;

  if (gameState_ == GameState::WON || gameState_ == GameState::LOST) {
    config_.setValue("lastDays", currentDays_);

    config_.setValue("guessNo", guessNo_);

    for (int r = 0; r < s_maxGuesses; ++r)
      guessData += guesses_[r].guess.toStdString();

    config_.setValue("guessData", guessData);
  }
  else {
    config_.setValue("lastDays", -1);

    config_.setValue("guessNo", 0);

    config_.setValue("guessData", "");
  }

  config_.save();
}

void
CQWordle::
setWord()
{
  auto dt = currentDays_ - baseDays_; // delta day

//std::cerr << d1 << " " << d2 << " " << dt << "\n";
  word_ = CWordleUseWords[dt % CWordleNumUseWords];
}

void
CQWordle::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  QFontMetricsF guessFm(guessFont_);
  gap_ = std::max(guessFm.height()/4.0, 1.0);

  drawBackground(&painter);

  drawGuesses(&painter);

  drawKeyboard(&painter);

  drawMessage(&painter);
}

void
CQWordle::
drawBackground(QPainter *painter)
{
#if 0
  // draw background
  if      (gameState_ == GameState::WON)
    painter->fillRect(rect(), QColor(100, 200, 100));
  else if (gameState_ == GameState::LOST)
    painter->fillRect(rect(), QColor(200, 100, 100));
  else
    painter->fillRect(rect(), QColor(200, 200, 200));
#else
  painter->fillRect(rect(), QColor(200, 200, 200));
#endif
}

void
CQWordle::
drawGuesses(QPainter *painter)
{
  QFontMetricsF guessFm(guessFont_);
  double guessCharSize = guessFm.height()*1.2;

  double guessTdy = (guessFm.ascent() - guessFm.descent())/2.0;

  //---

  // draw guesses
  painter->setFont(guessFont_);

  auto drawChar = [&](double x, double y, const QString &c,
                      const GuessState &state, QRectF &rect) {
    auto bg = QColor(150, 150, 150);

    if      (state == GuessState::WRONG_PLACE)
      bg = QColor(200, 200, 50);
    else if (state == GuessState::RIGHT)
      bg = QColor(50, 200, 50);
    else if (state == GuessState::USED)
      bg = QColor(100, 100, 100);

    painter->setBrush(bg);
    painter->setPen(Qt::black);

    rect = QRectF(x, y, guessCharSize, guessCharSize);

    painter->drawRect(rect);

    double xc = x + guessCharSize/2.0;
    double yc = y + guessCharSize/2.0;

    auto cw = guessFm.horizontalAdvance(c);

    painter->drawText(int(xc - cw/2.0), int(yc + guessTdy), c);
  };

  double left  = (width() - s_wordLen*guessCharSize - (s_wordLen - 1)*gap_)/2;
  double right = left + s_wordLen*guessCharSize + (s_wordLen - 1)*gap_;

  double y = gap_;

  for (int r = 0; r < s_maxGuesses; ++r) {
    double x = left;

    for (int c = 0; c < s_wordLen; ++c) {
      const auto &guessData = guesses_[r];

      QRectF rect;

      drawChar(x, y, guessData.getChar(c), guessData.getState(c), rect);

      x += guessCharSize + gap_;
    }

    if (r == guessNo_) {
      painter->setPen(Qt::blue);

      int y1 = int(y + guessCharSize + 4);

      painter->drawLine(left, y1, right, y1); ++y1;
      painter->drawLine(left, y1, right, y1);
    }

    y += guessCharSize + gap_;
  }

  y += gap_;
}

void
CQWordle::
drawKeyboard(QPainter *painter)
{
  QFontMetricsF keyboardFm(keyboardFont_);
  double keyboardCharSize = keyboardFm.height()*1.2;

  double keyboardTdy = (keyboardFm.ascent() - keyboardFm.descent())/2.0;

  QFontMetricsF messageFm(messageFont_);
  double messageCharSize = messageFm.height()*1.2;

  double kh = 3*(keyboardCharSize + gap_);
  double mh = messageCharSize + gap_;

  //---

  // draw keyboard
  painter->setFont(keyboardFont_);

  auto drawChar = [&](double x, double y, const QString &c,
                      const GuessState &state, QRectF &rect) {
    auto bg = QColor(150, 150, 150);

    if      (state == GuessState::WRONG_PLACE)
      bg = QColor(200, 200, 50);
    else if (state == GuessState::RIGHT)
      bg = QColor(50, 200, 50);
    else if (state == GuessState::USED)
      bg = QColor(100, 100, 100);

    painter->setBrush(bg);
    painter->setPen(Qt::black);

    rect = QRectF(x, y, keyboardCharSize, keyboardCharSize);

    painter->drawRect(rect);

    double xc = x + keyboardCharSize/2.0;
    double yc = y + keyboardCharSize/2.0;

    auto cw = keyboardFm.horizontalAdvance(c);

    painter->drawText(int(xc - cw/2.0), int(yc + keyboardTdy), c);
  };

  auto keyboardLines = QStringList() <<
    "QWERTYUIOP" << "ASDFGHJKL" << "ZXCVBNM";

  double lastX1 = 0.0;
  double lastX2 = 0.0;
  double lastY  = 0.0;

  double y = height() - kh - mh;

  for (const auto &line : keyboardLines) {
    int len = line.length();

    double x = (width() - len*(keyboardCharSize + gap_))/2.0;

    lastX1 = x;

    for (int i = 0; i < len; ++i) {
      auto c = line[i].toLatin1();

      auto state = GuessState::UNKNOWN;

      auto p = keyData_.used.find(c);

      if (p != keyData_.used.end())
        state = (*p).second;

      drawChar(x, y, line.mid(i, 1), state, keyData_.rect[c]);

      x += keyboardCharSize + gap_;
    }

    lastX2 = x;
    lastY  = y;

    y += keyboardCharSize + gap_;
  }

  {
    auto drawButton = [&](double x, double y, const QString &text,
                          int delta, QRectF &rect) {
      painter->setBrush(QColor(150, 150, 150));
      painter->setPen(Qt::black);

      auto w = keyboardFm.horizontalAdvance(text);

      rect = QRectF(x + delta*w, y, w, keyboardCharSize);

      painter->drawRect(rect);

      double xc = x + w/2.0 + delta*w;
      double yc = y + keyboardCharSize/2.0;

      painter->drawText(int(xc - w/2.0), int(yc + keyboardTdy), text);
    };

    drawButton(lastX1 - gap_, lastY, "TRY", -1, enterRect_);
    drawButton(lastX2       , lastY, "DEL", 0 , delRect_);
  }
}

void
CQWordle::
drawMessage(QPainter *painter)
{
  QFontMetricsF messageFm(messageFont_);
  double messageCharSize = messageFm.height()*1.2;

  double messageTdy = (messageFm.ascent() - messageFm.descent())/2.0;

  //---

  // draw message
  painter->setFont(messageFont_);

  auto drawText = [&](double y, const QString &text) {
    auto tw = messageFm.horizontalAdvance(text);

    painter->setPen(Qt::black);

    double x  = (width() - tw)/2.0;
    double yc = y + messageCharSize/2.0;

    painter->drawText(int(x), int(yc + messageTdy), text);
  };

  double mh = messageCharSize + gap_;

  double y = height() - mh + gap_/2.0;

  auto rect = QRectF(0, height() - mh, width(), mh);

  if      (gameState_ == GameState::WON)
    painter->fillRect(rect, QColor(100, 200, 100));
  else if (gameState_ == GameState::LOST)
    painter->fillRect(rect, QColor(200, 100, 100));

  QString message;

  if      (gameState_ == GameState::WON)
    message = "CORRECT";
  else if (gameState_ == GameState::LOST)
    message = "FAILED : Word was " + word_;
  else {
    message = QString("Guess %1").arg(guessNo_ + 1);

    message += QString(" : Wins %1, Losses %2").arg(wins_).arg(losses_);

    if (illegalWord_)
      message += " : Illegal word";
  }

  drawText(y, message);
}

void
CQWordle::
mousePressEvent(QMouseEvent *e)
{
  char c = '\0';

  for (const auto &kr : keyData_.rect) {
    if (kr.second.contains(e->pos())) {
      c = kr.first;
      break;
    }
  }

  if (c != '\0') {
    enterChar(c);

    update();
  }

  if (enterRect_.contains(e->pos())) {
    enterWord();

    update();
  }

  if (delRect_.contains(e->pos())) {
    removeChar();

    update();
  }
}

void
CQWordle::
keyPressEvent(QKeyEvent *e)
{
  if      (e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z) {
    enterChar(char(e->key()));

    update();
  }
  else if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
    removeChar();

    update();
  }
  else if (e->key() == Qt::Key_Return) {
    enterWord();

    update();
  }
  else {
    //std::cerr << "Unexpected key : " << e->key() << "\n";
  }
}

void
CQWordle::
enterChar(char c)
{
  if (gameState_ != GameState::PLAYING)
    return;

  if (charNo_ < 0 || charNo_ >= s_wordLen)
    return;

  auto &guessData = guesses_[guessNo_];

  guessData.setChar(charNo_, c);

  if (charNo_ < s_wordLen)
    ++charNo_;

  illegalWord_ = false;
}

void
CQWordle::
removeChar()
{
  if (gameState_ != GameState::PLAYING)
    return;

  if (charNo_ <= 0 || charNo_ > s_wordLen)
    return;

  auto &guessData = guesses_[guessNo_];

  --charNo_;

  guessData.setChar(charNo_, ' ');

  illegalWord_ = false;
}

void
CQWordle::
enterWord()
{
  if (gameState_ != GameState::PLAYING)
    return;

  if (charNo_ != s_wordLen)
    return;

  auto &guessData = guesses_[guessNo_];

  illegalWord_ = (wordSet_.find(guessData.guess) == wordSet_.end());

  if (illegalWord_)
    return;

  guessData.score(word_, keyData_.used);

  if      (guessData.isCorrect()) {
    gameState_ = GameState::WON;

    ++wins_;

    saveConfig();
  }
  else if (guessNo_ >= s_maxGuesses - 1) {
    gameState_ = GameState::LOST;

    ++losses_;

    saveConfig();
  }
  else {
    charNo_ = 0;

    ++guessNo_;
  }
}

QSize
CQWordle::
sizeHint() const
{
  QFontMetricsF guessFm(guessFont_);
  double guessCharSize = guessFm.height()*1.2;
  gap_ = std::max(guessFm.height()/4.0, 1.0);

  double gw = s_wordLen*(guessCharSize + gap_);
  double gh = s_maxGuesses*(guessCharSize + gap_);

  QFontMetricsF keyboardFm(keyboardFont_);
  double keyboardCharSize = keyboardFm.height()*1.2;

  QFontMetricsF messageFm(messageFont_);
  double messageCharSize = messageFm.height()*1.2;

  double kw = 10*(keyboardCharSize + gap_);
  double kh = 3*(keyboardCharSize + gap_);

  double mh = messageCharSize + gap_;

  return QSize(int(std::max(gw, kw) + gap_), int(gh + gap_ + kh + mh));
}
