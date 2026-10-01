#ifndef AUDIO_H
#define AUDIO_H

#include "game.h"
#include "raylib.h"

#include <array>

// Owns the short, synthesized cues and (when needed) the audio device.
// A missing audio device leaves the game fully playable in silence.
class SoundEffects {
public:
  SoundEffects();
  ~SoundEffects();
  SoundEffects(const SoundEffects &) = delete;
  SoundEffects &operator=(const SoundEffects &) = delete;

  bool available() const { return available_; }
  bool enabled() const { return available_ && enabled_; }
  void toggle();
  void play(GameEvent event);
  void stop();

private:
  std::array<Sound, 4> sounds_{};
  bool owns_device_ = false;
  bool available_ = false;
  bool enabled_ = true;
};

#endif
