#ifndef CQWordle_H
#define CQWordle_H

#include <CConfig.h>

#include <QFrame>
#include <vector>
#include <set>

class CQWordle : public QFrame {
  Q_OBJECT

 public:
  CQWordle();
 ~CQWordle();

  void paintEvent(QPaintEvent *) override;

  void mousePressEvent(QMouseEvent *) override;

  void keyPressEvent(QKeyEvent *) override;

  QSize sizeHint() const override;

 private:
  void init();

  void loadConfig();
  void saveConfig();

  void drawBackground(QPainter *painter);
  void drawGuesses(QPainter *painter);
  void drawKeyboard(QPainter *painter);
  void drawMessage(QPainter *painter);

  void setWord();

  void enterChar(char c);
  void removeChar();
  void enterWord();

 private:
  enum GameState {
    PLAYING,
    WON,
    LOST
  };

  enum class GuessState {
    UNKNOWN,
    WRONG,
    WRONG_PLACE,
    USED,
    RIGHT
  };

  //---

  using WordSet = std::set<QString>;

  using CharUsed = std::map<char, GuessState>;
  using CharRect = std::map<char, QRectF>;

  struct KeyData {
    CharUsed used;
    CharRect rect;
  };

  //---

  struct GuessData {
    QString                 guess;
    std::vector<GuessState> state;

    void init() {
      guess = "     ";

      state.resize(s_wordLen);

      for (int i = 0; i < s_wordLen; ++i)
        state[i] = GuessState::UNKNOWN;
    }

    QString getChar(int i) const {
      return guess.mid(i, 1);
    }

    void setChar(int i, char c) {
      guess[i] = c;
    }

    GuessState getState(int i) const {
      return state[i];
    }

    void score(const QString &word, CharUsed &used) {
      auto word1 = word;

      for (int i = 0; i < s_wordLen; ++i) {
        auto c = guess[i].toLatin1();

        if (used[c] != GuessState::WRONG_PLACE && used[c] != GuessState::RIGHT)
          used[c] = GuessState::USED;

        if (word1[i] == guess[i]) {
          used[c] = GuessState::RIGHT;

          state[i] = GuessState::RIGHT;
          word1[i] = ' ';
        }
        else {
          state[i] = GuessState::UNKNOWN;
        }
      }

      for (int i = 0; i < s_wordLen; ++i) {
        if (state[i] == GuessState::UNKNOWN) {
          auto c = guess[i].toLatin1();

          int pos = word1.indexOf(guess[i]);

          if (pos >= 0) {
            if (used[c] != GuessState::RIGHT)
              used[c] = GuessState::WRONG_PLACE;

            state[i] = GuessState::WRONG_PLACE;

            word1[pos] = ' ';
          }
          else {
            state[i] = GuessState::UNKNOWN;
          }
        }
      }
    }

    bool isCorrect() const {
      for (int i = 0; i < s_wordLen; ++i)
        if (state[i] != GuessState::RIGHT)
          return false;

      return true;
    }
  };

  using Guesses = std::vector<GuessData>;

  //---

  static const int s_wordLen    { 5 };
  static const int s_maxGuesses { 6 };

  //---

  CConfig config_; //!< config data

  int wins_   { 0 }; //!< number of wins
  int losses_ { 0 }; //!< number of losses

  QFont guessFont_;    //!< guess font
  QFont keyboardFont_; //!< keyboard font
  QFont messageFont_;  //!< message font

  WordSet   wordSet_;                            //!< set of allowable words
  GameState gameState_   { GameState::PLAYING }; //!< game state
  int       baseDays_    { 0 };                  //!< base day for word selection
  int       currentDays_ { 0 };                  //!< current day for word selection
  QString   word_;                               //!< word to guess
  bool      illegalWord_ { false };              //!< is entered word illegal
  Guesses   guesses_;                            //!< guesses so far
  int       guessNo_     { 0 };                  //!< current guess index
  int       charNo_      { 0 };                  //!< current guess char index
  KeyData   keyData_;                            //!< keys used and associated keyboard rect
  QRectF    enterRect_;                          //!< enter button rect
  QRectF    delRect_;                            //!< delete button rect

  mutable double gap_ { 0.0 }; //! button gap
};

#endif
